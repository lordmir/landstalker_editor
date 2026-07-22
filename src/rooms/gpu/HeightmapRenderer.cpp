#include "HeightmapRenderer.h"
#include "GLLoader.h"
#include "PixelFont.h"
#include <algorithm>
#include <cctype>
#include <cmath>

using namespace Landstalker;

namespace {
// Upper bound for the projected height of one heightmap Z unit, in pixels.
constexpr float kMaxZExtent = 128.0f;

struct HeightmapPoint {
    float x;
    float y;
};

struct HeightmapCell {
    int x;
    int y;
    uint8_t z;
    uint8_t restriction;
    uint8_t floor_type;
    HeightmapPoint center;
};

struct HeightmapColor {
    float r;
    float g;
    float b;
};

const HeightmapColor kOutlineColor{0.2f, 0.2f, 0.2f};
const HeightmapColor kTextColor{0.82f, 0.82f, 0.82f};
const HeightmapColor kMagentaText{1.0f, 0.15f, 1.0f};
const HeightmapColor kYellowText{1.0f, 0.95f, 0.15f};

float Cross(const HeightmapPoint& a, const HeightmapPoint& b, const HeightmapPoint& c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool PointInQuad(const HeightmapPoint& point, const HeightmapPoint (&quad)[4])
{
    bool has_positive = false;
    bool has_negative = false;
    for (std::size_t i = 0; i < 4; ++i) {
        float cross = Cross(quad[i], quad[(i + 1) % 4], point);
        has_positive = has_positive || cross > 0.0f;
        has_negative = has_negative || cross < 0.0f;
        if (has_positive && has_negative) {
            return false;
        }
    }
    return true;
}

bool HeightmapDrawOrder(const HeightmapCell& lhs, const HeightmapCell& rhs)
{
    int lhs_depth = lhs.x + lhs.y;
    int rhs_depth = rhs.x + rhs.y;
    if (lhs_depth != rhs_depth) {
        return lhs_depth > rhs_depth;
    }

    if (lhs.z != rhs.z) {
        return lhs.z > rhs.z;
    }

    if (lhs.y != rhs.y) {
        return lhs.y > rhs.y;
    }

    return lhs.x > rhs.x;
}

bool IsVisibleCell(const HeightmapCell& cell)
{
    return cell.z != 0xFF && !(cell.restriction == 4 && cell.z == 0);
}

bool IsInvalidHeightmapCell(const HeightmapCell& cell)
{
    return cell.restriction == 4 && cell.z == 0;
}

bool IsDrawableCell(const HeightmapCell& cell)
{
    return cell.z != 0xFF;
}

GLint DepthStencilValue(const HeightmapCell& cell)
{
    return std::clamp(cell.x + cell.y + 25, 1, 255);
}

float HeightLight(uint8_t z)
{
    return std::clamp<float>(0.78f + (z / 15.0f) * 0.22f, 0.0f, 1.0f);
}

HeightmapColor RestrictionColor(uint8_t restriction)
{
    switch (restriction) {
        case 4: return {1.0f, 0.12f, 0.08f};
        case 0: return {0.1f, 1.0f, 0.16f};
        case 2: return {0.15f, 0.3f, 1.0f};
        case 6: return {1.0f, 0.1f, 1.0f};
        default: return {1.0f, 0.92f, 0.08f};
    }
}

HeightmapColor ApplyLight(HeightmapColor color, float light)
{
    return {
        std::clamp(color.r * light, 0.0f, 1.0f),
        std::clamp(color.g * light, 0.0f, 1.0f),
        std::clamp(color.b * light, 0.0f, 1.0f)
    };
}

void DrawLine(const HeightmapPoint& a, const HeightmapPoint& b, float r, float g, float bl, float alpha, float width, float opacity = 1.0f)
{
    glColor4f(r, g, bl, alpha * opacity);
    glLineWidth(width);
    glBegin(GL_LINES);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glEnd();
}

void DrawColoredLine(const HeightmapPoint& a, const HeightmapPoint& b, const HeightmapColor& color, float alpha, float width, float opacity = 1.0f)
{
    DrawLine(a, b, color.r, color.g, color.b, alpha, width, opacity);
}

void DrawVerticalFaceFill(
    const HeightmapPoint& a,
    const HeightmapPoint& b,
    const HeightmapPoint& c,
    const HeightmapPoint& d,
    uint8_t z0,
    uint8_t z1,
    float face_light,
    float opacity = 1.0f)
{
    if (z0 == z1) {
        return;
    }

    float light = std::clamp<float>(HeightLight(std::max(z0, z1)) * face_light, 0.0f, 1.0f);
    glColor4f(light, light, light, 0.35f * opacity);
    glBegin(GL_QUADS);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glVertex2f(c.x, c.y);
    glVertex2f(d.x, d.y);
    glEnd();
}

void DrawTopFace(const HeightmapCell& cell, float opacity = 1.0f)
{
    if (IsInvalidHeightmapCell(cell)) {
        return;
    }

    float px = cell.center.x;
    float py = cell.center.y;
    HeightmapColor color = ApplyLight(RestrictionColor(cell.restriction), HeightLight(cell.z) * 1.08f);

    glColor4f(color.r, color.g, color.b, 0.4f * opacity);
    glBegin(GL_QUADS);
    glVertex2f(px, py - 16.0f); glVertex2f(px + 32.0f, py);
    glVertex2f(px, py + 16.0f); glVertex2f(px - 32.0f, py);
    glEnd();
}

void DrawHoverOutline(const HeightmapCell& cell, float opacity = 1.0f)
{
    HeightmapPoint top = {cell.center.x, cell.center.y - 16.0f};
    HeightmapPoint right = {cell.center.x + 32.0f, cell.center.y};
    HeightmapPoint bottom = {cell.center.x, cell.center.y + 16.0f};
    HeightmapPoint left = {cell.center.x - 32.0f, cell.center.y};

    glColor4f(1.0f, 0.95f, 0.0f, opacity);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(top.x, top.y);
    glVertex2f(right.x, right.y);
    glVertex2f(bottom.x, bottom.y);
    glVertex2f(left.x, left.y);
    glEnd();
}

void DrawRegionEdge(const HeightmapCell& cell, const HeightmapPoint& a, const HeightmapPoint& b, float opacity = 1.0f)
{
    DrawColoredLine(
        {cell.center.x + a.x, cell.center.y + a.y},
        {cell.center.x + b.x, cell.center.y + b.y},
        kOutlineColor, 1.0f, 2.0f, opacity);
}

char HexDigit(uint8_t value)
{
    value &= 0x0F;
    return value < 10 ? static_cast<char>('0' + value) : static_cast<char>('A' + value - 10);
}

void DrawHexGlyph(char c, float x, float y, float scale, const HeightmapColor& color)
{
    const auto* glyph = PixelFont::Glyph(c);
    if (!glyph) {
        return;
    }

    x = std::round(x);
    y = std::round(y);
    glColor4f(color.r, color.g, color.b, 1.0f);
    glBegin(GL_QUADS);
    for (int row = 0; row < PixelFont::GLYPH_HEIGHT; ++row) {
        uint8_t bits = (*glyph)[row];
        for (int col = 0; col < PixelFont::GLYPH_WIDTH; ++col) {
            uint8_t mask = static_cast<uint8_t>(1u << (PixelFont::GLYPH_WIDTH - 1 - col));
            if ((bits & mask) == 0) {
                continue;
            }

            float px = x + float(col) * scale;
            float py = y + float(row) * scale;
            glVertex2f(px, py);
            glVertex2f(px + scale, py);
            glVertex2f(px + scale, py + scale);
            glVertex2f(px, py + scale);
        }
    }
    glEnd();
}

void DrawCellText(const HeightmapCell& cell)
{
    if (IsInvalidHeightmapCell(cell)) {
        return;
    }

    constexpr float scale = 1.0f;
    constexpr float advance = 6.0f * scale;
    constexpr float row_gap = 2.0f * scale;
    constexpr float glyph_height = 6.0f * scale;
    constexpr float block_height = glyph_height * 2.0f + row_gap;

    const float top_y = cell.center.y - block_height * 0.5f;
    const float floor_x = cell.center.x - advance;
    const float lower_x = cell.center.x - advance * 1.5f;
    const float lower_y = top_y + glyph_height + row_gap;
    HeightmapColor floor_color = cell.floor_type != 0 ? kMagentaText : kTextColor;
    HeightmapColor restriction_color = cell.restriction != 4 ? kYellowText : kTextColor;

    DrawHexGlyph(HexDigit(cell.floor_type >> 4), floor_x, top_y, scale, floor_color);
    DrawHexGlyph(HexDigit(cell.floor_type), floor_x + advance, top_y, scale, floor_color);
    DrawHexGlyph(HexDigit(cell.z), lower_x, lower_y, scale, kTextColor);
    DrawHexGlyph(HexDigit(cell.restriction), lower_x + advance * 2.0f, lower_y, scale, restriction_color);
}

HeightmapPoint OffsetPoint(const HeightmapPoint& point, float x, float y)
{
    return {point.x + x, point.y + y};
}

void DisableFixedFunctionTexturing()
{
    for (int i = 0; i <= 5; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE0);
}

void ConfigureNoOverlapStencil()
{
    glClearStencil(0);
    glStencilMask(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

void ConfigureDepthStencilBuild()
{
    glClearStencil(0);
    glStencilMask(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

void FinishDepthStencilBuild()
{
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);
}
}

HeightmapRenderer::HeightmapRenderer(std::shared_ptr<GameData> gd)
    : m_gd(gd), m_current_room(0), m_room_w(0), m_room_h(0), m_room_left(0), m_room_top(0), m_hover_x(-1), m_hover_y(-1), m_z_extent(32.0f)
{
}

void HeightmapRenderer::LoadRoom(uint16_t roomnum)
{
    auto rd = m_gd->GetRoomData();
    auto map_entry = rd->GetMapForRoom(roomnum);
    if (!map_entry) {
        return;
    }

    auto map = map_entry->GetData();
    m_current_room = roomnum;
    m_room_w = map->GetWidth();
    m_room_h = map->GetHeight();
    m_room_left = map->GetLeft();
    m_room_top = map->GetTop();
    ClearHover();
}

void HeightmapRenderer::SetPreviewMap(std::shared_ptr<Tilemap3D> map)
{
    m_preview_map = std::move(map);
}

void HeightmapRenderer::ClearPreviewMap()
{
    m_preview_map.reset();
}

std::shared_ptr<Tilemap3D> HeightmapRenderer::CurrentMap() const
{
    if (m_preview_map) {
        return m_preview_map;
    }
    return m_gd->GetRoomData()->GetMapForRoom(m_current_room)->GetData();
}

void HeightmapRenderer::SetHoverPoint(float x, float y)
{
    m_hover_x = -1;
    m_hover_y = -1;

    if (m_room_w <= 0 || m_room_h <= 0) {
        return;
    }

    auto map = CurrentMap();
    HeightmapPoint point{x, y};
    float best_depth = 0.0f;
    auto project = [&](float cell_x, float cell_y, uint8_t z) {
        float hmx = cell_x - m_room_left + 12.5f;
        float hmy = cell_y - m_room_top + 12.5f;
        return HeightmapPoint{
            32.0f * hmx - 32.0f * hmy + 512.0f,
            16.0f * hmx + 16.0f * hmy - m_z_extent * z + 100.0f
        };
    };

    for (int y_cell = 0; y_cell < map->GetHeightmapHeight(); ++y_cell) {
        for (int x_cell = 0; x_cell < map->GetHeightmapWidth(); ++x_cell) {
            uint8_t z = map->GetHeight({x_cell, y_cell});
            uint16_t cell_value = map->GetHeightmapCell({x_cell, y_cell});
            HeightmapCell cell{
                x_cell,
                y_cell,
                z,
                static_cast<uint8_t>(cell_value >> 12),
                static_cast<uint8_t>(cell_value & 0x00FF),
                project(float(x_cell), float(y_cell), z)
            };
            if (!IsDrawableCell(cell)) {
                continue;
            }

            HeightmapPoint quad[4] = {
                {cell.center.x, cell.center.y - 16.0f},
                {cell.center.x + 32.0f, cell.center.y},
                {cell.center.x, cell.center.y + 16.0f},
                {cell.center.x - 32.0f, cell.center.y}
            };
            if (PointInQuad(point, quad)) {
                // Cells at different heights can project onto the same screen space, so
                // taking every hit in turn leaves whichever happened to be scanned last -
                // often one standing behind the cell the user is pointing at. Keep the
                // front-most instead: raising a cell moves it up the screen and bringing
                // it forward moves it down, so the greatest screen y is nearest the
                // viewer, which is also the one drawn on top.
                if (m_hover_x < 0 || cell.center.y > best_depth) {
                    best_depth = cell.center.y;
                    m_hover_x = x_cell;
                    m_hover_y = y_cell;
                }
            }
        }
    }
}

void HeightmapRenderer::ClearHover()
{
    m_hover_x = -1;
    m_hover_y = -1;
}

void HeightmapRenderer::SetZExtent(float value)
{
    m_z_extent = std::clamp(value, 0.0f, kMaxZExtent);
}

void HeightmapRenderer::Render()
{
    RenderInternal(1.0f, true);
}

void HeightmapRenderer::RenderOverlay(float opacity)
{
    RenderInternal(std::clamp(opacity, 0.0f, 1.0f), false);
}

void HeightmapRenderer::RenderInternal(float opacity, bool show_cell_text)
{
    if (m_room_w <= 0 || m_room_h <= 0) {
        return;
    }

    float mat[16] = {
        32.0f, 16.0f, 0.0f, 0.0f,
       -32.0f, 16.0f, 0.0f, 0.0f,
       0.0f, -m_z_extent, 0.0f, 0.0f,
       512.0f, 100.0f, 0.0f, 1.0f,
    };

    auto map = CurrentMap();

    auto project = [&](float x, float y, uint8_t z) {
        float hmx = x - m_room_left + 12.5f;
        float hmy = y - m_room_top + 12.5f;
        return HeightmapPoint{
            mat[0] * hmx + mat[4] * hmy + mat[8] * z + mat[12],
            mat[1] * hmx + mat[5] * hmy + mat[9] * z + mat[13]
        };
    };

    auto get_cell = [&](int x, int y) {
        uint8_t z = map->GetHeight({x, y});
        uint16_t cell = map->GetHeightmapCell({x, y});
        return HeightmapCell{
            x,
            y,
            z,
            static_cast<uint8_t>(cell >> 12),
            static_cast<uint8_t>(cell & 0x00FF),
            project(float(x), float(y), z)
        };
    };

    auto neighbor_height = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
            return uint8_t{0};
        }
        uint8_t z = map->GetHeight({x, y});
        if (z == 0xFF) {
            return uint8_t{0};
        }
        return z;
    };

    auto neighbor_matches_height = [&](const HeightmapCell& cell, int x, int y) {
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
            return false;
        }

        HeightmapCell neighbor = get_cell(x, y);
        return IsVisibleCell(neighbor) && neighbor.z == cell.z;
    };

    GLint stencil_bits = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);
    bool use_stencil = stencil_bits > 0;

    auto draw_height_step = [&](const HeightmapCell& high, uint8_t low_z, HeightmapPoint edge_start, HeightmapPoint edge_end, float face_light) {
        HeightmapPoint low_center = project(float(high.x), float(high.y), low_z);
        DrawVerticalFaceFill(
            OffsetPoint(high.center, edge_start.x, edge_start.y),
            OffsetPoint(high.center, edge_end.x, edge_end.y),
            OffsetPoint(low_center, edge_end.x, edge_end.y),
            OffsetPoint(low_center, edge_start.x, edge_start.y),
            high.z,
            low_z,
            face_light,
            opacity);
    };

    auto continuous_east_wall = [&](int x, int y, uint8_t high_z, uint8_t low_z) {
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
            return false;
        }

        HeightmapCell neighbor = get_cell(x, y);
        if (!IsVisibleCell(neighbor)) {
            return false;
        }

        uint8_t neighbor_low_z = neighbor_height(x + 1, y);
        if (neighbor.z <= neighbor_low_z) {
            return false;
        }

        return std::max(low_z, neighbor_low_z) < std::min(high_z, neighbor.z);
    };

    auto continuous_south_wall = [&](int x, int y, uint8_t high_z, uint8_t low_z) {
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
            return false;
        }

        HeightmapCell neighbor = get_cell(x, y);
        if (!IsVisibleCell(neighbor)) {
            return false;
        }

        uint8_t neighbor_low_z = neighbor_height(x, y + 1);
        if (neighbor.z <= neighbor_low_z) {
            return false;
        }

        return std::max(low_z, neighbor_low_z) < std::min(high_z, neighbor.z);
    };

    auto draw_east_wall = [&](const HeightmapCell& cell) {
        uint8_t low_z = neighbor_height(cell.x + 1, cell.y);
        if (cell.z <= low_z) {
            return;
        }

        HeightmapPoint low_center = project(float(cell.x), float(cell.y), low_z);
        HeightmapPoint top_a = OffsetPoint(cell.center, 32.0f, 0.0f);
        HeightmapPoint top_b = OffsetPoint(cell.center, 0.0f, 16.0f);
        HeightmapPoint bottom_a = OffsetPoint(low_center, 32.0f, 0.0f);
        HeightmapPoint bottom_b = OffsetPoint(low_center, 0.0f, 16.0f);

        if (!continuous_east_wall(cell.x, cell.y - 1, cell.z, low_z)) {
            DrawColoredLine(top_a, bottom_a, kOutlineColor, 1.0f, 1.0f, opacity);
        }
        if (!continuous_east_wall(cell.x, cell.y + 1, cell.z, low_z)) {
            DrawColoredLine(top_b, bottom_b, kOutlineColor, 1.0f, 1.0f, opacity);
        }
        DrawColoredLine(bottom_a, bottom_b, kOutlineColor, 1.0f, 1.0f, opacity);
        draw_height_step(cell, low_z, {32.0f, 0.0f}, {0.0f, 16.0f}, 0.82f);
    };

    auto draw_south_wall = [&](const HeightmapCell& cell) {
        uint8_t low_z = neighbor_height(cell.x, cell.y + 1);
        if (cell.z <= low_z) {
            return;
        }

        HeightmapPoint low_center = project(float(cell.x), float(cell.y), low_z);
        HeightmapPoint top_a = OffsetPoint(cell.center, 0.0f, 16.0f);
        HeightmapPoint top_b = OffsetPoint(cell.center, -32.0f, 0.0f);
        HeightmapPoint bottom_a = OffsetPoint(low_center, 0.0f, 16.0f);
        HeightmapPoint bottom_b = OffsetPoint(low_center, -32.0f, 0.0f);

        if (!continuous_south_wall(cell.x + 1, cell.y, cell.z, low_z)) {
            DrawColoredLine(top_a, bottom_a, kOutlineColor, 1.0f, 1.0f, opacity);
        }
        if (!continuous_south_wall(cell.x - 1, cell.y, cell.z, low_z)) {
            DrawColoredLine(top_b, bottom_b, kOutlineColor, 1.0f, 1.0f, opacity);
        }
        DrawColoredLine(bottom_a, bottom_b, kOutlineColor, 1.0f, 1.0f, opacity);
        draw_height_step(cell, low_z, {0.0f, 16.0f}, {-32.0f, 0.0f}, 0.55f);
    };

    glUseProgram(0);
    DisableFixedFunctionTexturing();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0x00);
    glStencilFunc(GL_EQUAL, 1, 0x01);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    std::vector<HeightmapCell> cells;
    cells.reserve(map->GetHeightmapWidth() * map->GetHeightmapHeight());
    for (int y = 0; y < map->GetHeightmapHeight(); ++y) {
        for (int x = 0; x < map->GetHeightmapWidth(); ++x) {
            HeightmapCell cell = get_cell(x, y);
            if (IsDrawableCell(cell)) {
                cells.push_back(cell);
            }
        }
    }
    std::stable_sort(cells.begin(), cells.end(), HeightmapDrawOrder);

    if (use_stencil) {
        ConfigureNoOverlapStencil();
    }

    for (const auto& cell : cells) {
        if (!neighbor_matches_height(cell, cell.x, cell.y - 1)) {
            DrawRegionEdge(cell, {0.0f, -16.0f}, {32.0f, 0.0f}, opacity);
        }
        if (!neighbor_matches_height(cell, cell.x + 1, cell.y)) {
            DrawRegionEdge(cell, {32.0f, 0.0f}, {0.0f, 16.0f}, opacity);
        }
        if (!neighbor_matches_height(cell, cell.x, cell.y + 1)) {
            DrawRegionEdge(cell, {0.0f, 16.0f}, {-32.0f, 0.0f}, opacity);
        }
        if (!neighbor_matches_height(cell, cell.x - 1, cell.y)) {
            DrawRegionEdge(cell, {-32.0f, 0.0f}, {0.0f, -16.0f}, opacity);
        }

        if (show_cell_text) {
            DrawCellText(cell);
        }
        DrawTopFace(cell, opacity);
        draw_east_wall(cell);
        draw_south_wall(cell);
    }

    if (use_stencil) {
        glDisable(GL_STENCIL_TEST);
    }

    if (m_hover_x >= 0 && m_hover_y >= 0 &&
        m_hover_x < map->GetHeightmapWidth() &&
        m_hover_y < map->GetHeightmapHeight()) {
        HeightmapCell hover_cell = get_cell(m_hover_x, m_hover_y);
        if (IsDrawableCell(hover_cell)) {
            DrawHoverOutline(hover_cell, opacity);
        }
    }

    if (use_stencil) {
        BuildDepthStencil();
    }

    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
}

void HeightmapRenderer::BuildDepthStencil()
{
    if (m_room_w <= 0 || m_room_h <= 0) {
        return;
    }

    GLint stencil_bits = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);
    if (stencil_bits <= 0) {
        return;
    }

    float mat[16] = {
        32.0f, 16.0f, 0.0f, 0.0f,
       -32.0f, 16.0f, 0.0f, 0.0f,
       0.0f, -m_z_extent, 0.0f, 0.0f,
       512.0f, 100.0f, 0.0f, 1.0f,
    };

    auto map = CurrentMap();

    auto project = [&](float x, float y, uint8_t z) {
        float hmx = x - m_room_left + 12.5f;
        float hmy = y - m_room_top + 12.5f;
        return HeightmapPoint{
            mat[0] * hmx + mat[4] * hmy + mat[8] * z + mat[12],
            mat[1] * hmx + mat[5] * hmy + mat[9] * z + mat[13]
        };
    };

    auto get_cell = [&](int x, int y) {
        uint8_t z = map->GetHeight({x, y});
        uint16_t cell = map->GetHeightmapCell({x, y});
        return HeightmapCell{
            x,
            y,
            z,
            static_cast<uint8_t>(cell >> 12),
            static_cast<uint8_t>(cell & 0x00FF),
            project(float(x), float(y), z)
        };
    };

    auto neighbor_height = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
            return uint8_t{0};
        }
        uint8_t z = map->GetHeight({x, y});
        if (z == 0xFF) {
            return uint8_t{0};
        }
        return z;
    };

    auto draw_height_step = [&](const HeightmapCell& high, uint8_t low_z, HeightmapPoint edge_start, HeightmapPoint edge_end, float face_light) {
        HeightmapPoint low_center = project(float(high.x), float(high.y), low_z);
        DrawVerticalFaceFill(
            OffsetPoint(high.center, edge_start.x, edge_start.y),
            OffsetPoint(high.center, edge_end.x, edge_end.y),
            OffsetPoint(low_center, edge_end.x, edge_end.y),
            OffsetPoint(low_center, edge_start.x, edge_start.y),
            high.z,
            low_z,
            face_light);
    };

    auto draw_east_wall = [&](const HeightmapCell& cell) {
        uint8_t low_z = neighbor_height(cell.x + 1, cell.y);
        if (cell.z <= low_z) {
            return;
        }

        draw_height_step(cell, low_z, {32.0f, 0.0f}, {0.0f, 16.0f}, 0.82f);
    };

    auto draw_south_wall = [&](const HeightmapCell& cell) {
        uint8_t low_z = neighbor_height(cell.x, cell.y + 1);
        if (cell.z <= low_z) {
            return;
        }

        draw_height_step(cell, low_z, {0.0f, 16.0f}, {-32.0f, 0.0f}, 0.55f);
    };

    std::vector<HeightmapCell> cells;
    cells.reserve(map->GetHeightmapWidth() * map->GetHeightmapHeight());
    for (int y = 0; y < map->GetHeightmapHeight(); ++y) {
        for (int x = 0; x < map->GetHeightmapWidth(); ++x) {
            HeightmapCell cell = get_cell(x, y);
            if (IsVisibleCell(cell)) {
                cells.push_back(cell);
            }
        }
    }
    std::stable_sort(cells.begin(), cells.end(), HeightmapDrawOrder);

    // Populate the stencil buffer with per-cell depth buckets so later passes
    // can cheaply test whether a fragment is behind terrain.
    glUseProgram(0);
    DisableFixedFunctionTexturing();
    ConfigureDepthStencilBuild();
    for (auto it = cells.rbegin(); it != cells.rend(); ++it) {
        const auto& cell = *it;
        glStencilFunc(GL_ALWAYS, DepthStencilValue(cell), 0xFF);
        DrawTopFace(cell);
        draw_east_wall(cell);
        draw_south_wall(cell);
    }
    FinishDepthStencilBuild();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
}

void HeightmapRenderer::BuildEntityOcclusionStencil(
    GLint entity_back_depth,
    GLint entity_front_depth,
    float entity_z,
    float entity_min_x,
    float entity_min_y,
    float entity_max_x,
    float entity_max_y,
    float sprite_min_x,
    float sprite_min_y,
    float sprite_max_x,
    float sprite_max_y)
{
    if (m_room_w <= 0 || m_room_h <= 0) {
        return;
    }

    GLint stencil_bits = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);
    if (stencil_bits <= 0) {
        return;
    }

    float mat[16] = {
        32.0f, 16.0f, 0.0f, 0.0f,
       -32.0f, 16.0f, 0.0f, 0.0f,
       0.0f, -m_z_extent, 0.0f, 0.0f,
       512.0f, 100.0f, 0.0f, 1.0f,
    };

    auto map = CurrentMap();

    auto project = [&](float x, float y, uint8_t z) {
        float hmx = x - m_room_left + 12.5f;
        float hmy = y - m_room_top + 12.5f;
        return HeightmapPoint{
            mat[0] * hmx + mat[4] * hmy + mat[8] * z + mat[12],
            mat[1] * hmx + mat[5] * hmy + mat[9] * z + mat[13]
        };
    };

    auto get_cell = [&](int x, int y) {
        uint8_t z = map->GetHeight({x, y});
        uint16_t cell = map->GetHeightmapCell({x, y});
        return HeightmapCell{
            x,
            y,
            z,
            static_cast<uint8_t>(cell >> 12),
            static_cast<uint8_t>(cell & 0x00FF),
            project(float(x), float(y), z)
        };
    };

    auto draw_entity_clip_mask = [&]() {
        glBegin(GL_QUADS);
        glVertex2f(sprite_min_x, sprite_min_y);
        glVertex2f(sprite_max_x, sprite_min_y);
        glVertex2f(sprite_max_x, sprite_max_y);
        glVertex2f(sprite_min_x, sprite_max_y);
        glEnd();
    };

    auto neighbor_height = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
            return uint8_t{0};
        }
        uint8_t z = map->GetHeight({x, y});
        if (z == 0xFF) {
            return uint8_t{0};
        }
        return z;
    };

    auto draw_height_step = [&](const HeightmapCell& high, uint8_t low_z, HeightmapPoint edge_start, HeightmapPoint edge_end, float face_light) {
        HeightmapPoint low_center = project(float(high.x), float(high.y), low_z);
        DrawVerticalFaceFill(
            OffsetPoint(high.center, edge_start.x, edge_start.y),
            OffsetPoint(high.center, edge_end.x, edge_end.y),
            OffsetPoint(low_center, edge_end.x, edge_end.y),
            OffsetPoint(low_center, edge_start.x, edge_start.y),
            high.z,
            low_z,
            face_light);
    };

    auto depth_in_range = [&](GLint depth) {
        return depth > entity_back_depth && depth <= entity_front_depth;
    };

    constexpr float heightmap_entity_offset = 12.0f;
    const float entity_hm_min_x = entity_min_x - heightmap_entity_offset;
    const float entity_hm_min_y = entity_min_y - heightmap_entity_offset;
    const float entity_hm_max_x = entity_max_x - heightmap_entity_offset;
    const float entity_hm_max_y = entity_max_y - heightmap_entity_offset;

    auto cell_overlaps_entity_footprint = [&](const HeightmapCell& cell) {
        return float(cell.x) < entity_hm_max_x &&
               float(cell.x + 1) > entity_hm_min_x &&
               float(cell.y) < entity_hm_max_y &&
               float(cell.y + 1) > entity_hm_min_y;
    };

    auto cell_is_in_front = [&](const HeightmapCell& cell) {
        return !cell_overlaps_entity_footprint(cell) &&
               float(cell.x + 1) > entity_hm_min_x &&
               float(cell.y + 1) > entity_hm_min_y;
    };

    auto side_face_can_occlude = [&](const HeightmapCell& cell, uint8_t low_z) {
        return cell.z > low_z;
    };

    auto draw_east_wall = [&](const HeightmapCell& cell) {
        uint8_t low_z = neighbor_height(cell.x + 1, cell.y);
        if (!side_face_can_occlude(cell, low_z) || !cell_is_in_front(cell)) {
            return;
        }
        draw_height_step(cell, low_z, {32.0f, 0.0f}, {0.0f, 16.0f}, 0.82f);
    };

    auto draw_south_wall = [&](const HeightmapCell& cell) {
        uint8_t low_z = neighbor_height(cell.x, cell.y + 1);
        if (!side_face_can_occlude(cell, low_z) || !cell_is_in_front(cell)) {
            return;
        }
        draw_height_step(cell, low_z, {0.0f, 16.0f}, {-32.0f, 0.0f}, 0.55f);
    };

    std::vector<HeightmapCell> cells;
    cells.reserve(map->GetHeightmapWidth() * map->GetHeightmapHeight());
    for (int y = 0; y < map->GetHeightmapHeight(); ++y) {
        for (int x = 0; x < map->GetHeightmapWidth(); ++x) {
            HeightmapCell cell = get_cell(x, y);
            GLint cell_depth = DepthStencilValue(cell);
            if (IsVisibleCell(cell) &&
                cell.z > entity_z &&
                depth_in_range(cell_depth)) {
                cells.push_back(cell);
            }
        }
    }
    std::stable_sort(cells.begin(), cells.end(), HeightmapDrawOrder);

    // Build an entity-local stencil mask:
    // 0x02 marks the projected entity clip area, 0x01 marks occluding terrain.
    // Sprite rendering then tests these bits to split visible vs occluded fragments.
    glUseProgram(0);
    DisableFixedFunctionTexturing();
    glClearStencil(0);
    glStencilMask(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0x02);
    glStencilFunc(GL_ALWAYS, 2, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    draw_entity_clip_mask();

    for (auto it = cells.rbegin(); it != cells.rend(); ++it) {
        glStencilMask(0x01);
        if (cell_is_in_front(*it)) {
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            DrawTopFace(*it);
        }
        glStencilFunc(GL_EQUAL, 3, 0x02);
        draw_east_wall(*it);
        draw_south_wall(*it);
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
}

void HeightmapRenderer::AdjustZExtent(float delta)
{
    SetZExtent(m_z_extent + delta);
}
