#include "GLCanvasLayerEditMode.h"

GLCanvasLayerEditMode::GLCanvasLayerEditMode(MyGLCanvas& canvas)
    : m_canvas(canvas)
{
}

bool GLCanvasLayerEditMode::HandleKeyDown(wxKeyEvent& evt)
{
    bool ctrl = evt.ControlDown();
    bool shift = evt.ShiftDown();

    switch (evt.GetKeyCode()) {
        case '1':
            m_canvas.SetEditorMode(MyGLCanvas::EditorMode::Room);
            m_canvas.Refresh();
            return true;
        case '2':
            m_canvas.SetEditorMode(MyGLCanvas::EditorMode::Heightmap);
            m_canvas.Refresh();
            return true;
        case '3':
            m_canvas.SetEditorMode(MyGLCanvas::EditorMode::BackgroundLayer);
            m_canvas.Refresh();
            return true;
        case '4':
            m_canvas.SetEditorMode(MyGLCanvas::EditorMode::ForegroundLayer);
            m_canvas.Refresh();
            return true;
        case 'i':
        case 'I':
            m_canvas.m_background_show_block_ids = !m_canvas.m_background_show_block_ids;
            m_canvas.Refresh();
            return true;
        case 'h':
        case 'H':
            m_canvas.m_layer_heightmap_overlay = !m_canvas.m_layer_heightmap_overlay;
            m_canvas.Refresh();
            return true;
        case 'b':
        case 'B':
            if (m_canvas.m_editor_mode == MyGLCanvas::EditorMode::ForegroundLayer) {
                m_canvas.m_foreground_show_background_underlay = !m_canvas.m_foreground_show_background_underlay;
                m_canvas.Refresh();
            }
            return true;
        case WXK_ESCAPE:
            if (m_canvas.m_layer_dragging_select || m_canvas.m_layer_dragging_draw ||
                m_canvas.m_layer_dragging_selection_move || m_canvas.m_layer_dragging_line) {
                if (m_canvas.m_layer_dragging_draw) {
                    m_canvas.CommitLayerDrawStroke();
                }
                if (m_canvas.m_layer_dragging_line) {
                    m_canvas.CancelLayerLineDrag();
                }
                if (m_canvas.m_layer_dragging_select) {
                    m_canvas.FinishLayerSelectionDrag();
                }
                if (m_canvas.m_layer_dragging_selection_move) {
                    m_canvas.CancelLayerSelectionMoveDrag();
                }
                m_canvas.m_layer_dragging_select = false;
                m_canvas.m_layer_dragging_draw = false;
                m_canvas.m_layer_dragging_selection_move = false;
                m_canvas.m_layer_dragging_line = false;
                m_canvas.m_layer_last_draw_x = -1;
                m_canvas.m_layer_last_draw_y = -1;
                if (m_canvas.HasCapture()) {
                    m_canvas.ReleaseMouse();
                }
            } else if (m_canvas.m_drawing_tool != MyGLCanvas::DrawingTool::Select) {
                m_canvas.SetDrawingTool(MyGLCanvas::DrawingTool::Select);
            } else {
                m_canvas.ClearEditSelection();
            }
            m_canvas.Refresh();
            return true;
        case 'c':
        case 'C':
            m_canvas.CopySelectedBackgroundBlock();
            m_canvas.Refresh();
            return true;
        case WXK_SPACE:
            m_canvas.PasteSelectedBackgroundBlock();
            m_canvas.Refresh();
            return true;
        case WXK_DELETE:
        case WXK_NUMPAD_DELETE:
            m_canvas.ClearSelectedLayerCells();
            return true;
        case ',':
        case '<':
            m_canvas.AdjustSelectedBlockId(ctrl ? -256 : (shift ? -16 : -1));
            return true;
        case '.':
        case '>':
            m_canvas.AdjustSelectedBlockId(ctrl ? 256 : (shift ? 16 : 1));
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
            if (ctrl) {
                int w = 0;
                int h = 0;
                m_canvas.GetClientSize(&w, &h);
                m_canvas.ChangeZoomStep(1, float(w) * 0.5f, float(h) * 0.5f);
            }
            m_canvas.Refresh();
            return true;
        case '-':
        case WXK_NUMPAD_SUBTRACT:
            if (ctrl) {
                int w = 0;
                int h = 0;
                m_canvas.GetClientSize(&w, &h);
                m_canvas.ChangeZoomStep(-1, float(w) * 0.5f, float(h) * 0.5f);
            }
            m_canvas.Refresh();
            return true;
    }

    return true;
}

void GLCanvasLayerEditMode::HandleMouseMove(const wxMouseEvent& evt)
{
    m_canvas.m_heightmapRenderer.ClearHover();
    int cell_x = -1;
    int cell_y = -1;
    if (m_canvas.BackgroundCellAt(evt.GetPosition(), cell_x, cell_y)) {
        m_canvas.m_background_has_hover = true;
        m_canvas.m_background_hover_x = cell_x;
        m_canvas.m_background_hover_y = cell_y;
    } else {
        m_canvas.m_background_has_hover = false;
    }

    int drag_x = m_canvas.m_background_hover_x;
    int drag_y = m_canvas.m_background_hover_y;
    bool has_drag_cell = m_canvas.m_background_has_hover;
    if ((m_canvas.m_layer_dragging_select || m_canvas.m_layer_dragging_line) &&
        m_canvas.BackgroundVirtualCellAt(evt.GetPosition(), drag_x, drag_y)) {
        has_drag_cell = true;
    }

    if (m_canvas.m_layer_dragging_selection_move && m_canvas.m_background_has_hover) {
        m_canvas.UpdateLayerSelectionMoveDrag(m_canvas.m_background_hover_x, m_canvas.m_background_hover_y);
    } else if (m_canvas.m_layer_dragging_line && has_drag_cell) {
        m_canvas.UpdateLayerLineDrag(drag_x, drag_y, evt.ShiftDown(), evt.AltDown());
    } else if (m_canvas.m_layer_dragging_select && has_drag_cell) {
        m_canvas.UpdateLayerSelectionDrag(drag_x, drag_y);
    } else if (m_canvas.m_layer_dragging_draw &&
        m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::Draw &&
        m_canvas.m_background_has_hover &&
        (m_canvas.m_background_hover_x != m_canvas.m_layer_last_draw_x ||
         m_canvas.m_background_hover_y != m_canvas.m_layer_last_draw_y)) {
        if (m_canvas.PasteBackgroundBlockAt(m_canvas.m_background_hover_x, m_canvas.m_background_hover_y, true)) {
            m_canvas.m_layer_last_draw_x = m_canvas.m_background_hover_x;
            m_canvas.m_layer_last_draw_y = m_canvas.m_background_hover_y;
        }
    } else if (m_canvas.m_layer_dragging_draw &&
        m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::Stamp &&
        m_canvas.m_background_has_hover &&
        (m_canvas.m_background_hover_x != m_canvas.m_layer_last_draw_x ||
         m_canvas.m_background_hover_y != m_canvas.m_layer_last_draw_y)) {
        m_canvas.ApplyLayerStampAt(m_canvas.m_background_hover_x, m_canvas.m_background_hover_y);
        m_canvas.m_layer_last_draw_x = m_canvas.m_background_hover_x;
        m_canvas.m_layer_last_draw_y = m_canvas.m_background_hover_y;
    }
    m_canvas.SetCursor(wxCursor(
        m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::Select &&
        !evt.ShiftDown() &&
        m_canvas.m_background_has_hover &&
        m_canvas.IsLayerCellSelected(m_canvas.m_background_hover_x, m_canvas.m_background_hover_y) ?
        wxCURSOR_HAND : wxCURSOR_ARROW));
    m_canvas.Refresh();
}

void GLCanvasLayerEditMode::HandleLeftDown(const wxMouseEvent& evt)
{
    int x = -1;
    int y = -1;
    if (!m_canvas.BackgroundCellAt(evt.GetPosition(), x, y)) {
        m_canvas.Refresh();
        return;
    }

    if (m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::Draw) {
        m_canvas.m_layer_dragging_draw = true;
        m_canvas.m_layer_last_draw_x = x;
        m_canvas.m_layer_last_draw_y = y;
        m_canvas.PasteBackgroundBlockAt(x, y, true);
    } else if (m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::FloodFill) {
        m_canvas.ApplyLayerFloodFillAt(x, y);
    } else if (m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::Stamp) {
        m_canvas.m_layer_dragging_draw = true;
        m_canvas.m_layer_last_draw_x = x;
        m_canvas.m_layer_last_draw_y = y;
        m_canvas.ApplyLayerStampAt(x, y);
    } else if (m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::Line ||
               m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::FilledRect ||
               m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::OutlineRect ||
               m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::FilledCircle ||
               m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::OutlineCircle) {
        m_canvas.BeginLayerLineDrag(x, y, evt.ShiftDown(), evt.AltDown());
    } else if (m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::Select &&
               !evt.ShiftDown() &&
               m_canvas.IsLayerCellSelected(x, y)) {
        m_canvas.BeginLayerSelectionMoveDrag(x, y);
    } else {
        m_canvas.BeginLayerSelectionDrag(x, y, evt.ShiftDown(), evt.ControlDown(), evt.AltDown());
    }

    if ((m_canvas.m_layer_dragging_select || m_canvas.m_layer_dragging_draw ||
         m_canvas.m_layer_dragging_selection_move || m_canvas.m_layer_dragging_line) && !m_canvas.HasCapture()) {
        m_canvas.CaptureMouse();
    }
    m_canvas.Refresh();
}

void GLCanvasLayerEditMode::HandleLeftUp(const wxMouseEvent& evt)
{
    if (m_canvas.m_layer_dragging_select) {
        int cell_x = -1;
        int cell_y = -1;
        if (m_canvas.BackgroundVirtualCellAt(evt.GetPosition(), cell_x, cell_y)) {
            m_canvas.UpdateLayerSelectionDrag(cell_x, cell_y);
        }
        m_canvas.FinishLayerSelectionDrag();
    }
    if (m_canvas.m_layer_dragging_draw) {
        m_canvas.CommitLayerDrawStroke();
    }
    if (m_canvas.m_layer_dragging_selection_move) {
        int cell_x = -1;
        int cell_y = -1;
        if (m_canvas.BackgroundCellAt(evt.GetPosition(), cell_x, cell_y)) {
            m_canvas.UpdateLayerSelectionMoveDrag(cell_x, cell_y);
        }
        m_canvas.CommitLayerSelectionMoveDrag();
    }
    if (m_canvas.m_layer_dragging_line) {
        int cell_x = -1;
        int cell_y = -1;
        if (m_canvas.BackgroundVirtualCellAt(evt.GetPosition(), cell_x, cell_y)) {
            m_canvas.UpdateLayerLineDrag(cell_x, cell_y, evt.ShiftDown(), evt.AltDown());
        }
        m_canvas.CommitLayerLineDrag();
    }
    m_canvas.m_layer_dragging_select = false;
    m_canvas.m_layer_dragging_draw = false;
    m_canvas.m_layer_dragging_selection_move = false;
    m_canvas.m_layer_dragging_line = false;
    m_canvas.m_layer_last_draw_x = -1;
    m_canvas.m_layer_last_draw_y = -1;
    if (m_canvas.HasCapture()) {
        m_canvas.ReleaseMouse();
    }
    m_canvas.Refresh();
}

void GLCanvasLayerEditMode::HandleRightDown(const wxMouseEvent& evt)
{
    if (m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::Select) {
        m_canvas.SetDrawingTool(MyGLCanvas::DrawingTool::Draw);
        m_canvas.UpdateStatusBar();
        return;
    }
    if (m_canvas.m_layer_dragging_select || m_canvas.m_layer_dragging_draw ||
        m_canvas.m_layer_dragging_selection_move || m_canvas.m_layer_dragging_line) {
        if (m_canvas.m_layer_dragging_draw) {
            m_canvas.CommitLayerDrawStroke();
        }
        if (m_canvas.m_layer_dragging_line) {
            m_canvas.CancelLayerLineDrag();
        }
        if (m_canvas.m_layer_dragging_select) {
            m_canvas.FinishLayerSelectionDrag();
        }
        if (m_canvas.m_layer_dragging_selection_move) {
            m_canvas.CancelLayerSelectionMoveDrag();
        }
        m_canvas.m_layer_dragging_select = false;
        m_canvas.m_layer_dragging_draw = false;
        m_canvas.m_layer_dragging_selection_move = false;
        m_canvas.m_layer_dragging_line = false;
        m_canvas.m_layer_last_draw_x = -1;
        m_canvas.m_layer_last_draw_y = -1;
        if (m_canvas.HasCapture()) {
            m_canvas.ReleaseMouse();
        }
        m_canvas.Refresh();
        return;
    }
    int x = -1;
    int y = -1;
    if (m_canvas.BackgroundCellAt(evt.GetPosition(), x, y)) {
        m_canvas.CopyBackgroundBlockAt(x, y);
    }
    m_canvas.Refresh();
}

void GLCanvasLayerEditMode::HandleMouseLeave(const wxMouseEvent& /*evt*/)
{
    if (m_canvas.m_layer_dragging_select || m_canvas.m_layer_dragging_draw ||
        m_canvas.m_layer_dragging_selection_move || m_canvas.m_layer_dragging_line ||
        m_canvas.m_dragging_pan) {
        return;
    }
    m_canvas.m_heightmapRenderer.ClearHover();
    m_canvas.m_background_has_hover = false;
    m_canvas.SetCursor(wxCursor(wxCURSOR_ARROW));
    m_canvas.Refresh();
}

void GLCanvasLayerEditMode::Render(int width, int height)
{
    if (m_canvas.m_editor_mode == MyGLCanvas::EditorMode::BackgroundLayer) {
        m_canvas.m_mapRenderer.RenderBackgroundOnly();
        if (m_canvas.m_layer_priority_highlight) {
            m_canvas.m_mapRenderer.RenderPriorityHighlight(m_canvas.CurrentEditLayer(), 1.0f, 0.0f, 0.0f, 0.42f);
        }
    } else {
        if (m_canvas.m_foreground_show_background_underlay) {
            m_canvas.m_mapRenderer.RenderBackgroundWithOpacity(0.25f);
        }
        m_canvas.m_mapRenderer.RenderForegroundOnly();
        if (m_canvas.m_layer_priority_highlight) {
            m_canvas.m_mapRenderer.RenderPriorityHighlight(m_canvas.CurrentEditLayer(), 0.0f, 0.95f, 1.0f, 0.42f);
        }
    }

    if (m_canvas.m_layer_heightmap_overlay) {
        m_canvas.m_heightmapRenderer.RenderOverlay(0.55f);
    }

    if (m_canvas.m_layer_dragging_selection_move) {
        const int w = m_canvas.m_mapRenderer.GetRoomWidth();
        const int h = m_canvas.m_mapRenderer.GetRoomHeight();
        auto map = m_canvas.CurrentRoomMap();
        for (const auto& source : m_canvas.m_layer_selection_move_values) {
            int x = source.first.first + m_canvas.m_layer_selection_move_delta_x;
            int y = source.first.second + m_canvas.m_layer_selection_move_delta_y;
            int block_index = y * w + x;
            const bool valid_preview_cell = map && x >= 0 && y >= 0 &&
                x < w && y < h &&
                block_index >= 0 && block_index < map->GetWidth() * map->GetHeight();
            if (!valid_preview_cell) {
                continue;
            }
            if (map->GetBlock(static_cast<uint16_t>(block_index), m_canvas.CurrentEditLayer()).value == source.second) {
                continue;
            }
            m_canvas.m_mapRenderer.RenderBlockGhost(
                source.second,
                x,
                y,
                0.42f,
                m_canvas.CurrentEditLayer());
        }
    }

    if (m_canvas.m_layer_dragging_line && m_canvas.m_background_clipboard_valid) {
        auto map = m_canvas.CurrentRoomMap();
        const int w = m_canvas.m_mapRenderer.GetRoomWidth();
        const int h = m_canvas.m_mapRenderer.GetRoomHeight();
        for (const auto& cell : m_canvas.m_layer_line_preview_cells) {
            int x = cell.first;
            int y = cell.second;
            int block_index = y * w + x;
            const bool valid_preview_cell = map && x >= 0 && y >= 0 &&
                x < w && y < h &&
                block_index >= 0 && block_index < map->GetWidth() * map->GetHeight();
            if (!valid_preview_cell) {
                continue;
            }
            if (map->GetBlock(static_cast<uint16_t>(block_index), m_canvas.CurrentEditLayer()).value ==
                m_canvas.m_background_clipboard_block_id) {
                continue;
            }
            m_canvas.m_mapRenderer.RenderBlockGhost(
                m_canvas.m_background_clipboard_block_id,
                x,
                y,
                0.42f,
                m_canvas.CurrentEditLayer());
        }
    }

    if (m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::Draw &&
        m_canvas.m_background_clipboard_valid &&
        !m_canvas.m_layer_dragging_draw) {
        int preview_x = -1;
        int preview_y = -1;
        if (m_canvas.m_background_has_hover) {
            preview_x = m_canvas.m_background_hover_x;
            preview_y = m_canvas.m_background_hover_y;
        } else if (m_canvas.m_background_has_selection) {
            preview_x = m_canvas.m_background_selected_x;
            preview_y = m_canvas.m_background_selected_y;
        }

        const int w = m_canvas.m_mapRenderer.GetRoomWidth();
        const int h = m_canvas.m_mapRenderer.GetRoomHeight();
        const int block_index = preview_y * w + preview_x;
        auto map = m_canvas.CurrentRoomMap();
        const bool valid_preview_cell = map && preview_x >= 0 && preview_y >= 0 &&
            preview_x < w && preview_y < h &&
            block_index >= 0 && block_index < map->GetWidth() * map->GetHeight();
        const uint16_t current_block = valid_preview_cell ?
            map->GetBlock(static_cast<uint16_t>(block_index), m_canvas.CurrentEditLayer()).value : 0;

        if (valid_preview_cell && current_block != m_canvas.m_background_clipboard_block_id) {
            m_canvas.m_mapRenderer.RenderBlockGhost(
                m_canvas.m_background_clipboard_block_id,
                preview_x,
                preview_y,
                0.45f,
                m_canvas.CurrentEditLayer());
            if (m_canvas.m_layer_priority_highlight) {
                if (m_canvas.m_editor_mode == MyGLCanvas::EditorMode::BackgroundLayer) {
                    m_canvas.m_mapRenderer.RenderBlockPriorityHighlight(
                        m_canvas.m_background_clipboard_block_id,
                        preview_x,
                        preview_y,
                        m_canvas.CurrentEditLayer(),
                        1.0f,
                        0.0f,
                        0.0f,
                        0.5f);
                } else {
                    m_canvas.m_mapRenderer.RenderBlockPriorityHighlight(
                        m_canvas.m_background_clipboard_block_id,
                        preview_x,
                        preview_y,
                        m_canvas.CurrentEditLayer(),
                        0.0f,
                        0.95f,
                        1.0f,
                        0.5f);
                }
            }
        }
    }

    if (m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::Stamp &&
        !m_canvas.m_layer_dragging_draw) {
        int preview_x = -1;
        int preview_y = -1;
        if (m_canvas.m_background_has_hover) {
            preview_x = m_canvas.m_background_hover_x;
            preview_y = m_canvas.m_background_hover_y;
        } else if (m_canvas.m_background_has_selection) {
            preview_x = m_canvas.m_background_selected_x;
            preview_y = m_canvas.m_background_selected_y;
        }

        auto map = m_canvas.CurrentRoomMap();
        const int w = m_canvas.m_mapRenderer.GetRoomWidth();
        const int h = m_canvas.m_mapRenderer.GetRoomHeight();
        for (const auto& cell : m_canvas.BuildLayerStampCells(preview_x, preview_y)) {
            int x = cell.first.first;
            int y = cell.first.second;
            int block_index = y * w + x;
            const bool valid_preview_cell = map && x >= 0 && y >= 0 &&
                x < w && y < h &&
                block_index >= 0 && block_index < map->GetWidth() * map->GetHeight();
            if (!valid_preview_cell) {
                continue;
            }
            if (map->GetBlock(static_cast<uint16_t>(block_index), m_canvas.CurrentEditLayer()).value == cell.second) {
                continue;
            }
            m_canvas.m_mapRenderer.RenderBlockGhost(
                cell.second,
                x,
                y,
                0.42f,
                m_canvas.CurrentEditLayer());
        }
    }

    if (m_canvas.m_drawing_tool == MyGLCanvas::DrawingTool::FloodFill &&
        m_canvas.m_background_clipboard_valid &&
        !m_canvas.m_layer_dragging_draw) {
        int preview_x = -1;
        int preview_y = -1;
        if (m_canvas.m_background_has_hover) {
            preview_x = m_canvas.m_background_hover_x;
            preview_y = m_canvas.m_background_hover_y;
        } else if (m_canvas.m_background_has_selection) {
            preview_x = m_canvas.m_background_selected_x;
            preview_y = m_canvas.m_background_selected_y;
        }

        auto map = m_canvas.CurrentRoomMap();
        const int w = m_canvas.m_mapRenderer.GetRoomWidth();
        const int h = m_canvas.m_mapRenderer.GetRoomHeight();
        for (const auto& cell : m_canvas.BuildLayerFloodFillCells(preview_x, preview_y)) {
            int x = cell.first;
            int y = cell.second;
            int block_index = y * w + x;
            const bool valid_preview_cell = map && x >= 0 && y >= 0 &&
                x < w && y < h &&
                block_index >= 0 && block_index < map->GetWidth() * map->GetHeight();
            if (!valid_preview_cell) {
                continue;
            }
            if (map->GetBlock(static_cast<uint16_t>(block_index), m_canvas.CurrentEditLayer()).value ==
                m_canvas.m_background_clipboard_block_id) {
                continue;
            }
            m_canvas.m_mapRenderer.RenderBlockGhost(
                m_canvas.m_background_clipboard_block_id,
                x,
                y,
                0.42f,
                m_canvas.CurrentEditLayer());
        }
    }

    m_canvas.RenderBackgroundEditorOverlay(width, height);
    m_canvas.SwapBuffers();
    m_canvas.RecordRenderedFrame();
}
