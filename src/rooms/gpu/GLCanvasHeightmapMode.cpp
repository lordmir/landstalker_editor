#include "GLCanvasHeightmapMode.h"

#include <algorithm>
#include <array>
#include <iterator>

namespace {
static constexpr std::array<uint8_t, 4> kRestrictionCycle = {0x0, 0x2, 0x4, 0x6};
}

GLCanvasHeightmapMode::GLCanvasHeightmapMode(GLCanvas& canvas)
    : m_canvas(canvas)
{
}

uint8_t GLCanvasHeightmapMode::SelectedRestriction() const
{
    return static_cast<uint8_t>((m_canvas.SelectedHeightmapCellValue() >> 12) & 0x0F);
}

void GLCanvasHeightmapMode::SetSelectedHeightmapCell(uint16_t value, bool refresh_object_placements)
{
    m_canvas.SetSelectedHeightmapCell(value, refresh_object_placements);
}

void GLCanvasHeightmapMode::AdjustSelectedHeightmapType(int delta)
{
    uint16_t value = m_canvas.SelectedHeightmapCellValue();
    int type = std::clamp(static_cast<int>(value & 0x00FF) + delta, 0, 0xFF);
    SetSelectedHeightmapCell(static_cast<uint16_t>((value & 0xFF00) | type), false);
}

void GLCanvasHeightmapMode::AdjustSelectedHeightmapRestriction(int delta)
{
    uint16_t value = m_canvas.SelectedHeightmapCellValue();
    int restriction = std::clamp(static_cast<int>((value >> 12) & 0x0F) + delta, 0, 0x0F);
    SetSelectedHeightmapCell(static_cast<uint16_t>((value & 0x0FFF) | (restriction << 12)), true);
}

void GLCanvasHeightmapMode::CycleSelectedHeightmapRestriction(int direction)
{
    uint8_t current = SelectedRestriction();
    auto it = std::find(kRestrictionCycle.begin(), kRestrictionCycle.end(), current);
    int index = it == kRestrictionCycle.end() ? 0 : static_cast<int>(std::distance(kRestrictionCycle.begin(), it));
    index = (index + direction + static_cast<int>(kRestrictionCycle.size())) % static_cast<int>(kRestrictionCycle.size());

    uint16_t value = m_canvas.SelectedHeightmapCellValue();
    SetSelectedHeightmapCell(static_cast<uint16_t>((value & 0x0FFF) | (kRestrictionCycle[static_cast<std::size_t>(index)] << 12)), true);
}

void GLCanvasHeightmapMode::ClearSelectedHeightmapCell()
{
    m_canvas.ClearSelectedHeightmapCells();
}

bool GLCanvasHeightmapMode::HandleKeyDown(wxKeyEvent& evt)
{
    bool shift = evt.ShiftDown();

    if (shift) {
        // Shift+1..4 apply view presets: flat, raised, full-height, and
        // full-height with the tilemap drawn underneath.
        auto apply_view_preset = [this](float z_scale, bool tilemap_underlay) {
            m_canvas.SetHeightmapZScale(z_scale);
            m_canvas.m_heightmap_tilemap_underlay = tilemap_underlay;
            m_canvas.Refresh();
            return true;
        };
        switch (evt.GetKeyCode()) {
            case '1':
                return apply_view_preset(0.0f, false);
            case '2':
                return apply_view_preset(0.5f, false);
            case '3':
                return apply_view_preset(1.0f, false);
            case '4':
                return apply_view_preset(1.0f, true);
        }
    }

    switch (evt.GetKeyCode()) {
        case '1':
            m_canvas.SetEditorMode(GLCanvas::EditorMode::Room);
            m_canvas.Refresh();
            return true;
        case '2':
            m_canvas.SetEditorMode(GLCanvas::EditorMode::Heightmap);
            m_canvas.Refresh();
            return true;
        case '3':
            m_canvas.SetEditorMode(GLCanvas::EditorMode::BackgroundLayer);
            m_canvas.Refresh();
            return true;
        case '4':
            m_canvas.SetEditorMode(GLCanvas::EditorMode::ForegroundLayer);
            m_canvas.Refresh();
            return true;
        case WXK_ESCAPE:
            if (m_canvas.m_heightmap_dragging_line || m_canvas.m_heightmap_dragging_selection_move) {
                m_canvas.CancelHeightmapLineDrag();
                m_canvas.CancelHeightmapSelectionMoveDrag();
                if (m_canvas.HasCapture()) {
                    m_canvas.ReleaseMouse();
                }
            } else if (m_canvas.IsHeightmapPreviewTool()) {
                m_canvas.SetDrawingTool(GLCanvas::DrawingTool::Select);
            } else {
                m_canvas.ClearEditSelection();
            }
            m_canvas.Refresh();
            return true;
        case 'b':
        case 'B':
            m_canvas.m_heightmap_tilemap_underlay = !m_canvas.m_heightmap_tilemap_underlay;
            m_canvas.Refresh();
            return true;
        case '[':
        case '{':
            AdjustSelectedHeightmapType(-1);
            return true;
        case ']':
        case '}':
            AdjustSelectedHeightmapType(1);
            return true;
        case ',':
        case '<':
            AdjustSelectedHeightmapRestriction(-1);
            return true;
        case '.':
        case '>':
            AdjustSelectedHeightmapRestriction(1);
            return true;
        case 'r':
        case 'R':
            CycleSelectedHeightmapRestriction(-1);
            return true;
        case 'f':
        case 'F':
            CycleSelectedHeightmapRestriction(1);
            return true;
        case WXK_PAGEUP:
            m_canvas.AdjustSelectedHeightmapHeight(1);
            return true;
        case WXK_PAGEDOWN:
            m_canvas.AdjustSelectedHeightmapHeight(-1);
            return true;
        case WXK_DELETE:
        case WXK_NUMPAD_DELETE:
            ClearSelectedHeightmapCell();
            return true;
        case 'c':
        case 'C':
            m_canvas.CopySelectedHeightmapCell();
            m_canvas.Refresh();
            return true;
        case WXK_SPACE:
            m_canvas.PasteSelectedHeightmapCell();
            m_canvas.Refresh();
            return true;
        case 'w':
        case 'W':
            m_canvas.MoveBackgroundSelection(0, -1);
            m_canvas.Refresh();
            return true;
        case 'a':
        case 'A':
            m_canvas.MoveBackgroundSelection(-1, 0);
            m_canvas.Refresh();
            return true;
        case 's':
        case 'S':
            m_canvas.MoveBackgroundSelection(0, 1);
            m_canvas.Refresh();
            return true;
        case 'd':
        case 'D':
            m_canvas.MoveBackgroundSelection(1, 0);
            m_canvas.Refresh();
            return true;
        case WXK_LEFT:
            m_canvas.PanCameraByStep(1, 0);
            m_canvas.Refresh();
            return true;
        case WXK_RIGHT:
            m_canvas.PanCameraByStep(-1, 0);
            m_canvas.Refresh();
            return true;
        case WXK_UP:
            m_canvas.PanCameraByStep(0, 1);
            m_canvas.Refresh();
            return true;
        case WXK_DOWN:
            m_canvas.PanCameraByStep(0, -1);
            m_canvas.Refresh();
            return true;
        case '+':
        case '=':
        case WXK_NUMPAD_ADD:
            m_canvas.AdjustHeightmapZScale(1);
            return true;
        case '-':
        case WXK_NUMPAD_SUBTRACT:
            m_canvas.AdjustHeightmapZScale(-1);
            return true;
    }

    return true;
}

void GLCanvasHeightmapMode::HandleMouseMove(const wxMouseEvent& evt)
{
    int prev_hover_x = m_canvas.m_heightmapRenderer.GetHoverX();
    int prev_hover_y = m_canvas.m_heightmapRenderer.GetHoverY();
    m_canvas.m_heightmapRenderer.SetHoverPoint(
        m_canvas.ScreenToWorldX(evt.GetPosition().x),
        m_canvas.ScreenToWorldY(evt.GetPosition().y));
    if (m_canvas.m_heightmapRenderer.GetHoverX() != prev_hover_x ||
        m_canvas.m_heightmapRenderer.GetHoverY() != prev_hover_y) {
        m_canvas.UpdateStatusBar();
    }
    int x = -1;
    int y = -1;
    // Prefer the draw-order pick; the flat-plane virtual cell is only a fallback
    // so drags can extend past the map edge. Overriding a successful pick with
    // the virtual cell would target a different cell than the one clicked
    // whenever the cell under the cursor is raised.
    bool has_cell = m_canvas.HeightmapCellAt(evt.GetPosition(), x, y);
    if (!has_cell &&
        (m_canvas.m_heightmap_dragging_select || m_canvas.m_heightmap_dragging_line) &&
        m_canvas.HeightmapVirtualCellAt(evt.GetPosition(), x, y)) {
        has_cell = true;
    }
    if (has_cell) {
        if (m_canvas.m_heightmap_dragging_select) {
            m_canvas.UpdateHeightmapSelectionDrag(x, y);
        } else if (m_canvas.m_heightmap_dragging_selection_move) {
            m_canvas.UpdateHeightmapSelectionMoveDrag(x, y);
        } else if (m_canvas.m_heightmap_dragging_line) {
            m_canvas.UpdateHeightmapLineDrag(x, y, evt.ShiftDown());
        } else if (m_canvas.m_heightmap_dragging_draw &&
                   m_canvas.m_drawing_tool == GLCanvas::DrawingTool::Stamp &&
                   (x != m_canvas.m_heightmap_last_draw_x || y != m_canvas.m_heightmap_last_draw_y)) {
            m_canvas.ApplyHeightmapStampAt(x, y);
            m_canvas.UpdateStatusBar();
            m_canvas.m_heightmap_last_draw_x = x;
            m_canvas.m_heightmap_last_draw_y = y;
        } else if (m_canvas.m_heightmap_dragging_draw &&
                   (x != m_canvas.m_heightmap_last_draw_x || y != m_canvas.m_heightmap_last_draw_y)) {
            if (m_canvas.PasteHeightmapCellAt(x, y, true)) {
                m_canvas.UpdateStatusBar();
            }
            m_canvas.m_heightmap_last_draw_x = x;
            m_canvas.m_heightmap_last_draw_y = y;
        }
    }
    int cursor_x = -1;
    int cursor_y = -1;
    bool has_cursor_cell = m_canvas.HeightmapCellAt(evt.GetPosition(), cursor_x, cursor_y);
    m_canvas.SetCursor(wxCursor(
        m_canvas.m_drawing_tool == GLCanvas::DrawingTool::Select &&
        !evt.ShiftDown() &&
        has_cursor_cell &&
        m_canvas.IsHeightmapCellSelected(cursor_x, cursor_y) ? wxCURSOR_HAND : wxCURSOR_ARROW));
    m_canvas.Refresh();
}

void GLCanvasHeightmapMode::HandleLeftDown(const wxMouseEvent& evt)
{
    int x = -1;
    int y = -1;
    if (!m_canvas.HeightmapCellAt(evt.GetPosition(), x, y)) {
        m_canvas.Refresh();
        return;
    }

    if (m_canvas.m_drawing_tool == GLCanvas::DrawingTool::Select &&
        !evt.ShiftDown() &&
        m_canvas.IsHeightmapCellSelected(x, y)) {
        m_canvas.BeginHeightmapSelectionMoveDrag(x, y);
    } else if (m_canvas.m_drawing_tool == GLCanvas::DrawingTool::Draw) {
        m_canvas.m_heightmap_dragging_draw = true;
        m_canvas.m_heightmap_last_draw_x = x;
        m_canvas.m_heightmap_last_draw_y = y;
        m_canvas.PasteHeightmapCellAt(x, y, true);
    } else if (m_canvas.m_drawing_tool == GLCanvas::DrawingTool::FloodFill) {
        m_canvas.ApplyHeightmapFloodFillAt(x, y);
    } else if (m_canvas.m_drawing_tool == GLCanvas::DrawingTool::Stamp) {
        m_canvas.m_heightmap_dragging_draw = true;
        m_canvas.m_heightmap_last_draw_x = x;
        m_canvas.m_heightmap_last_draw_y = y;
        m_canvas.ApplyHeightmapStampAt(x, y);
    } else if (m_canvas.IsHeightmapShapeTool()) {
        m_canvas.BeginHeightmapLineDrag(x, y, evt.ShiftDown());
    } else {
        m_canvas.BeginHeightmapSelectionDrag(x, y, evt.ShiftDown(), evt.ControlDown());
    }
    if ((m_canvas.m_heightmap_dragging_select || m_canvas.m_heightmap_dragging_draw || m_canvas.m_heightmap_dragging_line || m_canvas.m_heightmap_dragging_selection_move) && !m_canvas.HasCapture()) {
        m_canvas.CaptureMouse();
    }
    m_canvas.UpdateStatusBar();
    m_canvas.Refresh();
}

void GLCanvasHeightmapMode::HandleLeftUp(const wxMouseEvent& evt)
{
    auto drag_end_cell = [this, &evt](int& cell_x, int& cell_y) {
        return m_canvas.HeightmapCellAt(evt.GetPosition(), cell_x, cell_y) ||
               m_canvas.HeightmapVirtualCellAt(evt.GetPosition(), cell_x, cell_y);
    };
    if (m_canvas.m_heightmap_dragging_select) {
        int cell_x = -1;
        int cell_y = -1;
        if (drag_end_cell(cell_x, cell_y)) {
            m_canvas.UpdateHeightmapSelectionDrag(cell_x, cell_y);
        }
        m_canvas.FinishHeightmapSelectionDrag();
    }
    if (m_canvas.m_heightmap_dragging_draw) {
        m_canvas.CommitHeightmapDrawStroke();
    }
    if (m_canvas.m_heightmap_dragging_line) {
        int cell_x = -1;
        int cell_y = -1;
        if (drag_end_cell(cell_x, cell_y)) {
            m_canvas.UpdateHeightmapLineDrag(cell_x, cell_y, evt.ShiftDown());
        }
        m_canvas.CommitHeightmapLineDrag();
    }
    if (m_canvas.m_heightmap_dragging_selection_move) {
        m_canvas.CommitHeightmapSelectionMoveDrag();
    }
    m_canvas.m_heightmap_dragging_draw = false;
    m_canvas.m_heightmap_last_draw_x = -1;
    m_canvas.m_heightmap_last_draw_y = -1;
    if (m_canvas.HasCapture()) {
        m_canvas.ReleaseMouse();
    }
    m_canvas.UpdateStatusBar();
    m_canvas.Refresh();
}

void GLCanvasHeightmapMode::HandleRightDown(const wxMouseEvent& evt)
{
    int x = -1;
    int y = -1;
    if (m_canvas.m_heightmap_dragging_line || m_canvas.m_heightmap_dragging_selection_move) {
        m_canvas.CancelHeightmapLineDrag();
        m_canvas.CancelHeightmapSelectionMoveDrag();
        if (m_canvas.HasCapture()) {
            m_canvas.ReleaseMouse();
        }
        m_canvas.Refresh();
        return;
    }
    if (m_canvas.m_drawing_tool == GLCanvas::DrawingTool::Select) {
        m_canvas.SetDrawingTool(GLCanvas::DrawingTool::Draw);
        m_canvas.UpdateStatusBar();
        return;
    }
    if (m_canvas.HeightmapCellAt(evt.GetPosition(), x, y)) {
        m_canvas.CopyHeightmapCellAt(x, y);
        m_canvas.UpdateStatusBar();
    }
    m_canvas.Refresh();
}

void GLCanvasHeightmapMode::HandleMouseLeave(const wxMouseEvent& /*evt*/)
{
    if (m_canvas.m_heightmap_dragging_select || m_canvas.m_heightmap_dragging_draw ||
        m_canvas.m_heightmap_dragging_line || m_canvas.m_heightmap_dragging_selection_move ||
        m_canvas.m_dragging_pan) {
        return;
    }
    m_canvas.m_heightmapRenderer.ClearHover();
    m_canvas.m_background_has_hover = false;
    m_canvas.SetCursor(wxCursor(wxCURSOR_ARROW));
    m_canvas.UpdateStatusBar();
    m_canvas.Refresh();
}

void GLCanvasHeightmapMode::Render(int width, int height)
{
    std::shared_ptr<Landstalker::Tilemap3D> hover_preview;
    bool has_hover_preview = false;
    if (((m_canvas.IsHeightmapBrushTool() && m_canvas.m_heightmap_clipboard_valid) ||
         m_canvas.m_drawing_tool == GLCanvas::DrawingTool::Stamp ||
         m_canvas.m_heightmap_dragging_selection_move) &&
        !m_canvas.m_heightmap_dragging_draw) {
        auto map = m_canvas.CurrentRoomMap();
        if (map) {
            auto set_preview_cell = [&](int x, int y, uint16_t value) {
                if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
                    return;
                }
                if (!hover_preview) {
                    hover_preview = std::make_shared<Landstalker::Tilemap3D>(*map);
                }
                hover_preview->SetHeightmapCell({x, y}, value);
            };
            auto apply_brush_preview_cell = [&](int x, int y) {
                set_preview_cell(x, y, m_canvas.m_heightmap_clipboard_cell);
            };

            if (m_canvas.m_heightmap_dragging_selection_move) {
                for (const auto& source : m_canvas.m_heightmap_selection_move_values) {
                    set_preview_cell(source.first.first, source.first.second, GLCanvas::kClearedHeightmapCell);
                    set_preview_cell(
                        source.first.first + m_canvas.m_heightmap_selection_move_delta_x,
                        source.first.second + m_canvas.m_heightmap_selection_move_delta_y,
                        source.second);
                }
            } else if (m_canvas.m_heightmap_dragging_line) {
                for (const auto& cell : m_canvas.m_heightmap_line_preview_cells) {
                    apply_brush_preview_cell(cell.first, cell.second);
                }
            } else if (m_canvas.m_drawing_tool == GLCanvas::DrawingTool::FloodFill) {
                for (const auto& cell : m_canvas.BuildHeightmapFloodFillCells(
                         m_canvas.m_heightmapRenderer.GetHoverX(),
                         m_canvas.m_heightmapRenderer.GetHoverY())) {
                    apply_brush_preview_cell(cell.first, cell.second);
                }
            } else if (m_canvas.m_drawing_tool == GLCanvas::DrawingTool::Stamp) {
                for (const auto& cell : m_canvas.BuildHeightmapStampCells(
                         m_canvas.m_heightmapRenderer.GetHoverX(),
                         m_canvas.m_heightmapRenderer.GetHoverY())) {
                    set_preview_cell(cell.first.first, cell.first.second, cell.second);
                }
            } else {
                apply_brush_preview_cell(m_canvas.m_heightmapRenderer.GetHoverX(), m_canvas.m_heightmapRenderer.GetHoverY());
            }

            if (hover_preview) {
                m_canvas.m_heightmapRenderer.SetPreviewMap(hover_preview);
                has_hover_preview = true;
            }
        }
    }

    if (m_canvas.m_heightmap_tilemap_underlay) {
        m_canvas.m_mapRenderer.RenderBackgroundWithOpacity(0.35f);
        m_canvas.m_mapRenderer.RenderForegroundWithOpacity(0.35f);
    }
    m_canvas.m_heightmapRenderer.Render();
    if (has_hover_preview) {
        m_canvas.m_heightmapRenderer.ClearPreviewMap();
    }
    m_canvas.RenderHeightmapEditorOverlay(width, height);
    m_canvas.SwapBuffers();
    m_canvas.RecordRenderedFrame();
}
