#include "GLCanvasLayerEditMode.h"

GLCanvasLayerEditMode::GLCanvasLayerEditMode(MyGLCanvas& canvas)
    : m_canvas(canvas)
{
}

bool GLCanvasLayerEditMode::HandleKeyDown(wxKeyEvent& evt)
{
    bool ctrl = evt.ControlDown();

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
            m_canvas.ToggleLayerPriorityHighlight();
            return true;
        case 'b':
        case 'B':
            if (m_canvas.m_editor_mode == MyGLCanvas::EditorMode::ForegroundLayer) {
                m_canvas.m_foreground_show_background_underlay = !m_canvas.m_foreground_show_background_underlay;
                m_canvas.Refresh();
            }
            return true;
        case WXK_ESCAPE:
            m_canvas.ClearBackgroundClipboard();
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
    m_canvas.SetCursor(wxCursor(wxCURSOR_ARROW));
    m_canvas.Refresh();
}

void GLCanvasLayerEditMode::HandleLeftDown(const wxMouseEvent& evt)
{
    m_canvas.SelectBackgroundCellAt(evt.GetPosition());
    m_canvas.PasteSelectedBackgroundBlock();
    m_canvas.Refresh();
}

void GLCanvasLayerEditMode::HandleRightDown(const wxMouseEvent& evt)
{
    m_canvas.SelectBackgroundCellAt(evt.GetPosition());
    m_canvas.CopySelectedBackgroundBlock();
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

    if (m_canvas.m_background_clipboard_valid) {
        int preview_x = -1;
        int preview_y = -1;
        if (m_canvas.m_background_has_hover) {
            preview_x = m_canvas.m_background_hover_x;
            preview_y = m_canvas.m_background_hover_y;
        } else if (m_canvas.m_background_has_selection) {
            preview_x = m_canvas.m_background_selected_x;
            preview_y = m_canvas.m_background_selected_y;
        }

        const int width = m_canvas.m_mapRenderer.GetRoomWidth();
        const int height = m_canvas.m_mapRenderer.GetRoomHeight();
        const int block_index = preview_y * width + preview_x;
        auto map = m_canvas.CurrentRoomMap();
        const bool valid_preview_cell = map && preview_x >= 0 && preview_y >= 0 &&
            preview_x < width && preview_y < height &&
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

    m_canvas.RenderBackgroundEditorOverlay(width, height);
    m_canvas.SwapBuffers();
}
