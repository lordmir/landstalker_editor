#include "GLCanvasHeightmapMode.h"

#include <algorithm>

GLCanvasHeightmapMode::GLCanvasHeightmapMode(MyGLCanvas& canvas)
    : m_canvas(canvas)
{
}

bool GLCanvasHeightmapMode::HandleKeyDown(wxKeyEvent& evt)
{
    bool ctrl = evt.ControlDown();
    bool shift = evt.ShiftDown();

    if (shift) {
        switch (evt.GetKeyCode()) {
            case '1':
                m_canvas.m_heightmap_view_mode = MyGLCanvas::HeightmapViewMode::Flat;
                m_canvas.ApplyHeightmapViewMode();
                m_canvas.Refresh();
                return true;
            case '2':
                m_canvas.m_heightmap_view_mode = MyGLCanvas::HeightmapViewMode::Raised;
                m_canvas.ApplyHeightmapViewMode();
                m_canvas.Refresh();
                return true;
            case '3':
                m_canvas.m_heightmap_view_mode = MyGLCanvas::HeightmapViewMode::Full;
                m_canvas.ApplyHeightmapViewMode();
                m_canvas.Refresh();
                return true;
            case '4':
                m_canvas.m_heightmap_view_mode = MyGLCanvas::HeightmapViewMode::FullWithTilemap;
                m_canvas.ApplyHeightmapViewMode();
                m_canvas.Refresh();
                return true;
        }
    }

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
        case WXK_ESCAPE:
            m_canvas.ClearBackgroundClipboard();
            m_canvas.Refresh();
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

void GLCanvasHeightmapMode::HandleMouseMove(const wxMouseEvent& evt)
{
    m_canvas.m_heightmapRenderer.SetHoverPoint(
        m_canvas.ScreenToWorldX(evt.GetPosition().x),
        m_canvas.ScreenToWorldY(evt.GetPosition().y));
    m_canvas.SetCursor(wxCursor(wxCURSOR_ARROW));
    m_canvas.Refresh();
}

void GLCanvasHeightmapMode::HandleLeftDown(const wxMouseEvent& evt)
{
    if (m_canvas.SelectHeightmapCellAt(evt.GetPosition())) {
        m_canvas.PasteSelectedHeightmapCell();
        m_canvas.UpdateStatusBar();
    }
    m_canvas.Refresh();
}

void GLCanvasHeightmapMode::HandleRightDown(const wxMouseEvent& evt)
{
    if (m_canvas.SelectHeightmapCellAt(evt.GetPosition())) {
        m_canvas.CopySelectedHeightmapCell();
        m_canvas.UpdateStatusBar();
    }
    m_canvas.Refresh();
}

void GLCanvasHeightmapMode::Render(int width, int height)
{
    std::shared_ptr<Landstalker::Tilemap3D> hover_preview;
    bool has_hover_preview = false;
    if (m_canvas.m_heightmap_clipboard_valid) {
        auto map = m_canvas.CurrentRoomMap();
        int x = m_canvas.m_heightmapRenderer.GetHoverX();
        int y = m_canvas.m_heightmapRenderer.GetHoverY();
        if (map && x >= 0 && y >= 0 && x < map->GetHeightmapWidth() && y < map->GetHeightmapHeight()) {
            uint16_t current = map->GetHeightmapCell({x, y});
            if (current != m_canvas.m_heightmap_clipboard_cell) {
                hover_preview = std::make_shared<Landstalker::Tilemap3D>(*map);
                hover_preview->SetHeightmapCell({x, y}, m_canvas.m_heightmap_clipboard_cell);
                m_canvas.m_heightmapRenderer.SetPreviewMap(hover_preview);
                has_hover_preview = true;
            }
        }
    }

    if (m_canvas.m_heightmap_view_mode == MyGLCanvas::HeightmapViewMode::FullWithTilemap) {
        m_canvas.m_mapRenderer.RenderBackgroundWithOpacity(0.2f);
        m_canvas.m_mapRenderer.RenderForegroundWithOpacity(0.2f);
    }
    m_canvas.m_heightmapRenderer.Render();
    if (has_hover_preview) {
        m_canvas.m_heightmapRenderer.ClearPreviewMap();
    }
    m_canvas.RenderHeightmapEditorOverlay(width, height);
    m_canvas.SwapBuffers();
}
