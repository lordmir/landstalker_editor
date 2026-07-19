#include "GLCanvas.h"
#include "PixelFont.h"
#include "RoomProjection.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace Landstalker;

namespace {
using PickPoint = RoomProjection::PickPoint;
using RoomProjection::ProjectHeightmapGridPoint;

uint8_t HeightmapCellHeight(uint16_t value) {
    return static_cast<uint8_t>((value >> 8) & 0x0F);
}

std::string HexWord(uint16_t value)
{
    constexpr char digits[] = "0123456789ABCDEF";
    std::string out;
    out.push_back(digits[(value >> 12) & 0x0F]);
    out.push_back(digits[(value >> 8) & 0x0F]);
    out.push_back(digits[(value >> 4) & 0x0F]);
    out.push_back(digits[value & 0x0F]);
    return out;
}

using PixelFont::DrawOverlayText;

}  // namespace

void MyGLCanvas::RenderBackgroundEditorOverlay(int width, int height) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    float zoom = std::max(ZoomFactor(), 0.0001f);
    Tilemap3D::Layer layer = CurrentEditLayer();
    auto block_screen_origin = [this, zoom, layer](int x, int y) {
        float x_offset = layer == Tilemap3D::Layer::FG ? -32.0f : 0.0f;
        float px = 32.0f * static_cast<float>(x) - 32.0f * static_cast<float>(y) + 512.0f + x_offset;
        float py = 16.0f * static_cast<float>(x) + 16.0f * static_cast<float>(y) + 100.0f;
        return PickPoint{px * zoom + m_cam_x, py * zoom + m_cam_y};
    };

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

    glLineWidth(1.0f);
    glColor4f(0.78f, 0.78f, 0.78f, 0.7f);
    glBegin(GL_LINES);
    for (int y = 0; y < m_mapRenderer.GetRoomHeight(); ++y) {
        for (int x = 0; x < m_mapRenderer.GetRoomWidth(); ++x) {
            PickPoint origin = block_screen_origin(x, y);
            float left = origin.x;
            float top = origin.y;
            float right = left + 32.0f * zoom;
            float bottom = top + 32.0f * zoom;
            glVertex2f(left, top);
            glVertex2f(right, top);
            glVertex2f(right, top);
            glVertex2f(right, bottom);
            glVertex2f(right, bottom);
            glVertex2f(left, bottom);
            glVertex2f(left, bottom);
            glVertex2f(left, top);
        }
    }
    glEnd();

    if (m_background_has_hover) {
        PickPoint origin = block_screen_origin(m_background_hover_x, m_background_hover_y);
        float left = origin.x;
        float top = origin.y;
        float right = left + 32.0f * zoom;
        float bottom = top + 32.0f * zoom;

        // Strong hover cue for layer edit mode: soft fill + thick double outline.
        glColor4f(0.20f, 0.68f, 1.0f, 0.20f);
        glBegin(GL_QUADS);
        glVertex2f(left, top);
        glVertex2f(right, top);
        glVertex2f(right, bottom);
        glVertex2f(left, bottom);
        glEnd();

        glLineWidth(3.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 0.96f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(left, top);
        glVertex2f(right, top);
        glVertex2f(right, bottom);
        glVertex2f(left, bottom);
        glEnd();

        glLineWidth(2.0f);
        glColor4f(0.10f, 0.78f, 1.0f, 0.96f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(left + 1.0f, top + 1.0f);
        glVertex2f(right - 1.0f, top + 1.0f);
        glVertex2f(right - 1.0f, bottom - 1.0f);
        glVertex2f(left + 1.0f, bottom - 1.0f);
        glEnd();
        glLineWidth(1.0f);
    }

    if (m_background_has_selection) {
        const bool dim_selection = m_drawing_tool == DrawingTool::Draw;
        auto draw_selected_cell = [&](int x, int y, float fill_r, float fill_g, float fill_b, float fill_a,
                                      float line_r, float line_g, float line_b, float line_a, float line_width) {
            PickPoint origin = block_screen_origin(x, y);
            float left = origin.x;
            float top = origin.y;
            float right = left + 32.0f * zoom;
            float bottom = top + 32.0f * zoom;
            if (fill_a > 0.0f) {
                glColor4f(fill_r, fill_g, fill_b, fill_a);
                glBegin(GL_QUADS);
                glVertex2f(left, top);
                glVertex2f(right, top);
                glVertex2f(right, bottom);
                glVertex2f(left, bottom);
                glEnd();
            }
            glLineWidth(line_width);
            glColor4f(line_r, line_g, line_b, line_a);
            glBegin(GL_LINE_LOOP);
            glVertex2f(left, top);
            glVertex2f(right, top);
            glVertex2f(right, bottom);
            glVertex2f(left, bottom);
            glEnd();
        };
        if (!m_layer_selected_cells.empty()) {
            for (const auto& cell : m_layer_selected_cells) {
                draw_selected_cell(
                    cell.first,
                    cell.second,
                    1.0f,
                    0.86f,
                    0.0f,
                    dim_selection ? 0.08f : (m_layer_dragging_selection_move ? 0.08f : 0.20f),
                    1.0f,
                    0.82f,
                    0.0f,
                    dim_selection ? 0.42f : (m_layer_dragging_selection_move ? 0.34f : 0.9f),
                    1.5f);
            }
            if (m_layer_dragging_selection_move) {
                for (const auto& cell : m_layer_selected_cells) {
                    draw_selected_cell(
                        cell.first + m_layer_selection_move_delta_x,
                        cell.second + m_layer_selection_move_delta_y,
                        1.0f,
                        0.95f,
                        0.0f,
                        0.24f,
                        1.0f,
                        0.98f,
                        0.22f,
                        0.98f,
                        2.0f);
                }
                for (const auto& source : m_layer_selection_move_values) {
                    int x = source.first.first;
                    int y = source.first.second;
                    int replacement_source_x = x - m_layer_selection_move_delta_x;
                    int replacement_source_y = y - m_layer_selection_move_delta_y;
                    if (IsLayerCellSelected(replacement_source_x, replacement_source_y)) {
                        continue;
                    }

                    PickPoint origin = block_screen_origin(x, y);
                    float left = origin.x;
                    float top = origin.y;
                    float right = left + 32.0f * zoom;
                    float bottom = top + 32.0f * zoom;
                    glColor4f(0.04f, 0.04f, 0.04f, 0.38f);
                    glBegin(GL_QUADS);
                    glVertex2f(left, top);
                    glVertex2f(right, top);
                    glVertex2f(right, bottom);
                    glVertex2f(left, bottom);
                    glEnd();
                    glLineWidth(2.0f);
                    glColor4f(1.0f, 1.0f, 1.0f, 0.62f);
                    glBegin(GL_LINES);
                    glVertex2f(left + 5.0f * zoom, top + 5.0f * zoom);
                    glVertex2f(right - 5.0f * zoom, bottom - 5.0f * zoom);
                    glVertex2f(right - 5.0f * zoom, top + 5.0f * zoom);
                    glVertex2f(left + 5.0f * zoom, bottom - 5.0f * zoom);
                    glEnd();
                }
            }
            int primary_x = m_layer_selection_anchor_x;
            int primary_y = m_layer_selection_anchor_y;
            if (m_layer_dragging_selection_move) {
                primary_x += m_layer_selection_move_delta_x;
                primary_y += m_layer_selection_move_delta_y;
            }
            draw_selected_cell(
                primary_x,
                primary_y,
                1.0f,
                0.62f,
                0.0f,
                dim_selection ? 0.10f : 0.24f,
                1.0f,
                1.0f,
                1.0f,
                dim_selection ? 0.55f : 0.98f,
                2.5f);
        } else {
            draw_selected_cell(
                m_background_selected_x,
                m_background_selected_y,
                1.0f,
                0.78f,
                0.0f,
                dim_selection ? 0.10f : 0.24f,
                1.0f,
                1.0f,
                1.0f,
                dim_selection ? 0.55f : 0.98f,
                2.0f);
        }
        glLineWidth(1.0f);
    }

    if (m_background_show_block_ids) {
        constexpr float scale = 1.0f;
        constexpr float glyph_advance = PixelFont::GLYPH_ADVANCE;
        constexpr float glyph_height = 7.0f;
        for (int y = 0; y < m_mapRenderer.GetRoomHeight(); ++y) {
            for (int x = 0; x < m_mapRenderer.GetRoomWidth(); ++x) {
                int block_index = y * m_mapRenderer.GetRoomWidth() + x;
                if (block_index < 0 || block_index >= map->GetWidth() * map->GetHeight()) {
                    continue;
                }
                std::string label = HexWord(map->GetBlock(static_cast<uint16_t>(block_index), layer).value);
                PickPoint origin = block_screen_origin(x, y);
                float label_width = static_cast<float>(label.size()) * glyph_advance * scale;
                float text_x = origin.x + (32.0f * zoom - label_width) * 0.5f;
                float text_y = origin.y + (32.0f * zoom - glyph_height * scale) * 0.5f;
                glColor4f(0.05f, 0.05f, 0.05f, 0.78f);
                glBegin(GL_QUADS);
                glVertex2f(text_x - 2.0f, text_y - 1.0f);
                glVertex2f(text_x + label_width + 2.0f, text_y - 1.0f);
                glVertex2f(text_x + label_width + 2.0f, text_y + glyph_height + 1.0f);
                glVertex2f(text_x - 2.0f, text_y + glyph_height + 1.0f);
                glEnd();
                glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
                DrawOverlayText(label, text_x, text_y, scale);
            }
        }
    }

    if (m_drawing_tool == DrawingTool::Draw && m_background_clipboard_valid && !m_layer_dragging_draw) {
        int preview_x = -1;
        int preview_y = -1;
        if (m_background_has_hover) {
            preview_x = m_background_hover_x;
            preview_y = m_background_hover_y;
        } else if (m_background_has_selection) {
            preview_x = m_background_selected_x;
            preview_y = m_background_selected_y;
        }

        const int block_index = preview_y * m_mapRenderer.GetRoomWidth() + preview_x;
        const bool valid_preview_cell = preview_x >= 0 && preview_y >= 0 &&
            preview_x < m_mapRenderer.GetRoomWidth() && preview_y < m_mapRenderer.GetRoomHeight() &&
            block_index >= 0 && block_index < map->GetWidth() * map->GetHeight();
        const uint16_t current_block = valid_preview_cell ?
            map->GetBlock(static_cast<uint16_t>(block_index), layer).value : 0;

        if (valid_preview_cell && current_block != m_background_clipboard_block_id) {
            PickPoint origin = block_screen_origin(preview_x, preview_y);
            float left = origin.x;
            float top = origin.y;
            float right = left + 32.0f * zoom;
            float bottom = top + 32.0f * zoom;
            glColor4f(1.0f, 1.0f, 1.0f, 0.12f);
            glBegin(GL_QUADS);
            glVertex2f(left, top);
            glVertex2f(right, top);
            glVertex2f(right, bottom);
            glVertex2f(left, bottom);
            glEnd();
            glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
            DrawOverlayText(HexWord(m_background_clipboard_block_id), left + 2.0f, top + 2.0f, 1.0f);
        }
    }
}

void MyGLCanvas::RenderHeightmapEditorOverlay(int width, int height) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    std::vector<std::pair<int, int>> preview_cells;
    std::map<std::pair<int, int>, uint16_t> preview_values;
    if (m_heightmap_dragging_selection_move) {
        for (const auto& source : m_heightmap_selection_move_values) {
            std::pair<int, int> target{
                source.first.first + m_heightmap_selection_move_delta_x,
                source.first.second + m_heightmap_selection_move_delta_y
            };
            preview_cells.push_back(target);
            preview_values[target] = source.second;
        }
    } else if (!m_heightmap_dragging_draw && IsHeightmapPreviewTool()) {
        if (m_heightmap_dragging_line) {
            preview_cells = m_heightmap_line_preview_cells;
        } else if (m_drawing_tool == DrawingTool::FloodFill && m_heightmap_clipboard_valid) {
            preview_cells = BuildHeightmapFloodFillCells(
                m_heightmapRenderer.GetHoverX(),
                m_heightmapRenderer.GetHoverY());
        } else if (m_drawing_tool == DrawingTool::Stamp) {
            for (const auto& cell : BuildHeightmapStampCells(
                     m_heightmapRenderer.GetHoverX(),
                     m_heightmapRenderer.GetHoverY())) {
                preview_cells.push_back(cell.first);
                preview_values[cell.first] = cell.second;
            }
        } else {
            int hover_x = m_heightmapRenderer.GetHoverX();
            int hover_y = m_heightmapRenderer.GetHoverY();
            if (m_heightmap_clipboard_valid &&
                hover_x >= 0 && hover_y >= 0 &&
                hover_x < map->GetHeightmapWidth() && hover_y < map->GetHeightmapHeight()) {
                preview_cells.push_back({hover_x, hover_y});
            }
        }
    }

    bool has_selection = m_background_has_selection && !m_heightmap_selected_cells.empty();
    int primary_x = has_selection ? PrimaryHeightmapCellX() : -1;
    int primary_y = has_selection ? PrimaryHeightmapCellY() : -1;
    has_selection = has_selection && primary_x >= 0 && primary_y >= 0;

    if (!has_selection && preview_cells.empty()) {
        return;
    }

    float room_left = static_cast<float>(m_mapRenderer.GetRoomLeft());
    float room_top = static_cast<float>(m_mapRenderer.GetRoomTop());

    float zoom = std::max(ZoomFactor(), 0.0001f);

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

    auto draw_cell_diamond = [&](int x, int y, float fill_alpha, float line_width, int z_override = -1) {
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
            return;
        }
        uint8_t z = z_override >= 0 ? static_cast<uint8_t>(z_override) : map->GetHeight({x, y});
        PickPoint center = ProjectHeightmapGridPoint(
            static_cast<float>(x) + 0.5f,
            static_cast<float>(y) + 0.5f,
            z,
            room_left,
            room_top,
            m_heightmapRenderer.GetZExtent());
        float cx = center.x * zoom + m_cam_x;
        float cy = center.y * zoom + m_cam_y;
        if (fill_alpha > 0.0f) {
            glBegin(GL_QUADS);
            glVertex2f(cx, cy - 16.0f * zoom);
            glVertex2f(cx + 32.0f * zoom, cy);
            glVertex2f(cx, cy + 16.0f * zoom);
            glVertex2f(cx - 32.0f * zoom, cy);
            glEnd();
        }
        glLineWidth(line_width);
        glBegin(GL_LINE_LOOP);
        glVertex2f(cx, cy - 16.0f * zoom);
        glVertex2f(cx + 32.0f * zoom, cy);
        glVertex2f(cx, cy + 16.0f * zoom);
        glVertex2f(cx - 32.0f * zoom, cy);
        glEnd();
    };

    auto preview_cell_z = [&](const std::pair<int, int>& cell) {
        auto it = preview_values.find(cell);
        if (it != preview_values.end()) {
            return static_cast<int>(HeightmapCellHeight(it->second));
        }
        return static_cast<int>(HeightmapCellHeight(m_heightmap_clipboard_cell));
    };

    if (!preview_cells.empty()) {
        glColor4f(1.0f, 1.0f, 1.0f, 0.18f);
        for (const auto& preview_cell : preview_cells) {
            draw_cell_diamond(preview_cell.first, preview_cell.second, 0.18f, 2.5f, preview_cell_z(preview_cell));
        }
        glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
        for (const auto& preview_cell : preview_cells) {
            draw_cell_diamond(preview_cell.first, preview_cell.second, 0.0f, 2.5f, preview_cell_z(preview_cell));
        }
    }

    if (m_heightmap_dragging_selection_move) {
        for (const auto& source : m_heightmap_selection_move_values) {
            int x = source.first.first;
            int y = source.first.second;
            int replacement_source_x = x - m_heightmap_selection_move_delta_x;
            int replacement_source_y = y - m_heightmap_selection_move_delta_y;
            if (IsHeightmapCellSelected(replacement_source_x, replacement_source_y)) {
                continue;
            }

            int z = static_cast<int>(HeightmapCellHeight(source.second));
            PickPoint center = ProjectHeightmapGridPoint(
                static_cast<float>(x) + 0.5f,
                static_cast<float>(y) + 0.5f,
                static_cast<float>(z),
                room_left,
                room_top,
                m_heightmapRenderer.GetZExtent());
            float cx = center.x * zoom + m_cam_x;
            float cy = center.y * zoom + m_cam_y;
            glColor4f(0.04f, 0.04f, 0.04f, 0.38f);
            draw_cell_diamond(x, y, 0.38f, 2.0f, z);
            glLineWidth(2.0f);
            glColor4f(1.0f, 1.0f, 1.0f, 0.62f);
            glBegin(GL_LINES);
            glVertex2f(cx - 14.0f * zoom, cy - 7.0f * zoom);
            glVertex2f(cx + 14.0f * zoom, cy + 7.0f * zoom);
            glVertex2f(cx + 14.0f * zoom, cy - 7.0f * zoom);
            glVertex2f(cx - 14.0f * zoom, cy + 7.0f * zoom);
            glEnd();
        }
    }

    if (!has_selection) {
        glLineWidth(1.0f);
        return;
    }

    const bool inactive_selection = IsHeightmapBrushTool();
    for (const auto& selected_cell : m_heightmap_selected_cells) {
        int x = selected_cell.first;
        int y = selected_cell.second;
        bool primary = x == primary_x && y == primary_y;
        if (inactive_selection) {
            glColor4f(primary ? 0.55f : 0.42f, primary ? 0.46f : 0.42f, primary ? 0.12f : 0.42f, primary ? 0.72f : 0.58f);
        } else {
            glColor4f(primary ? 1.0f : 1.0f, primary ? 0.82f : 1.0f, primary ? 0.1f : 1.0f, primary ? 1.0f : 0.92f);
        }
        draw_cell_diamond(x, y, 0.0f, 2.0f);
    }

    glLineWidth(1.0f);
}
