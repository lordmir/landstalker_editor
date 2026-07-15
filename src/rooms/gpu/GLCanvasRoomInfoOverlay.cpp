#include "GLCanvasRoomInfoOverlay.h"

#include "GLCanvas.h"
#include "PixelFont.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <landstalker/misc/Utils.h>

using namespace Landstalker;

namespace {

struct RoomInfoRow {
    std::string label;
    std::string value;
    uint16_t room;
};

std::string HexFlagWord(uint16_t value)
{
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << value;
    return out.str();
}

std::string RoomDisplayText(const std::shared_ptr<GameData>& gd, uint16_t room)
{
    auto rd = gd ? gd->GetRoomData() : nullptr;
    if (!rd || room >= rd->GetRoomCount()) {
        return {};
    }

    std::ostringstream out;
    out << room << " " << wstr_to_utf8(rd->GetRoomDisplayName(room));
    return out.str();
}

std::vector<RoomInfoRow> BuildRoomInfoRows(const std::shared_ptr<GameData>& gd, uint16_t room)
{
    std::vector<RoomInfoRow> rows;
    auto rd = gd ? gd->GetRoomData() : nullptr;
    if (!rd || room >= rd->GetRoomCount()) {
        return rows;
    }

    auto add_destination_row = [&](const std::string& label, uint16_t destination_room) {
        if (destination_room >= rd->GetRoomCount()) {
            return;
        }
        rows.push_back({label, RoomDisplayText(gd, destination_room), destination_room});
    };

    if (rd->HasFallDestination(room)) {
        add_destination_row("Fall Destination", rd->GetFallDestination(room));
    }
    if (rd->HasClimbDestination(room)) {
        add_destination_row("Climb Destination", rd->GetClimbDestination(room));
    }

    for (const auto& transition : rd->GetTransitions(room)) {
        uint16_t other_room = transition.src_rm == room ? transition.dst_rm : transition.src_rm;
        if (other_room >= rd->GetRoomCount()) {
            continue;
        }

        rows.push_back({
            std::string{"Transition when Flag "} + HexFlagWord(transition.flag) + " is " + (transition.src_rm == room ? "SET" : "CLEAR"),
            RoomDisplayText(gd, other_room),
            other_room
        });
    }

    return rows;
}

using PixelFont::DrawOverlayText;

}  // namespace

GLCanvasRoomInfoOverlay::GLCanvasRoomInfoOverlay(MyGLCanvas& canvas)
    : m_canvas(canvas)
{
}

void GLCanvasRoomInfoOverlay::Render(int width, int height)
{
    auto rows = BuildRoomInfoRows(m_canvas.m_gd, m_canvas.m_current_room);
    m_links.clear();
    if (rows.empty()) {
        return;
    }

    glUseProgram(0);
    for (int i = 0; i <= 5; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    constexpr float scale = 1.0f;
    constexpr float glyph_advance = PixelFont::kGlyphAdvance;
    constexpr float row_height = 10.0f;
    constexpr float padding = 6.0f;
    constexpr float column_gap = 12.0f;
    constexpr float start_x = 8.0f;
    constexpr float start_y = 8.0f;

    float label_width = 0.0f;
    float value_width = 0.0f;
    for (const auto& row : rows) {
        label_width = std::max(label_width, float(row.label.size()) * glyph_advance * scale);
        value_width = std::max(value_width, float(row.value.size()) * glyph_advance * scale);
    }

    float table_width = padding * 2.0f + label_width + column_gap + value_width;
    float table_height = padding * 2.0f + float(rows.size()) * row_height;
    float label_x = start_x + padding;
    float value_x = label_x + label_width + column_gap;
    int hovered_room = HitTest(m_canvas.m_last_mouse_pos);

    glColor4f(0.02f, 0.02f, 0.03f, 0.78f);
    glBegin(GL_QUADS);
    glVertex2f(start_x, start_y);
    glVertex2f(start_x + table_width, start_y);
    glVertex2f(start_x + table_width, start_y + table_height);
    glVertex2f(start_x, start_y + table_height);
    glEnd();

    glColor4f(0.45f, 0.45f, 0.5f, 0.85f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(start_x, start_y);
    glVertex2f(start_x + table_width, start_y);
    glVertex2f(start_x + table_width, start_y + table_height);
    glVertex2f(start_x, start_y + table_height);
    glEnd();

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        float row_y = start_y + padding + float(i) * row_height;
        float value_text_width = float(row.value.size()) * glyph_advance * scale;
        wxRect value_rect(
            static_cast<int>(std::floor(value_x)),
            static_cast<int>(std::floor(row_y)),
            static_cast<int>(std::ceil(value_text_width)),
            static_cast<int>(std::ceil(row_height)));

        if (row.room == hovered_room) {
            glColor4f(1.0f, 1.0f, 1.0f, 0.09f);
            glBegin(GL_QUADS);
            glVertex2f(float(value_rect.GetLeft()), float(value_rect.GetTop()));
            glVertex2f(float(value_rect.GetRight()), float(value_rect.GetTop()));
            glVertex2f(float(value_rect.GetRight()), float(value_rect.GetBottom()));
            glVertex2f(float(value_rect.GetLeft()), float(value_rect.GetBottom()));
            glEnd();
        }

        glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
        DrawOverlayText(row.label, label_x, row_y, scale);
        glColor4f(1.0f, 0.95f, 0.18f, 0.98f);
        DrawOverlayText(row.value, value_x, row_y, scale);

        m_links.push_back({value_rect, row.room});
    }
}

int GLCanvasRoomInfoOverlay::HitTest(const wxPoint& point) const
{
    for (const auto& link : m_links) {
        if (link.rect.Contains(point)) {
            return static_cast<int>(link.room);
        }
    }
    return -1;
}
