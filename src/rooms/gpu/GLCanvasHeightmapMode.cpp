#include "GLCanvasHeightmapMode.h"

#include <algorithm>
#include <array>
#include <iterator>

namespace {
static constexpr std::array<uint8_t, 4> kRestrictionCycle = {0x0, 0x2, 0x4, 0x6};
}

GLCanvasHeightmapMode::GLCanvasHeightmapMode(MyGLCanvas& canvas)
    : m_canvas(canvas)
{
}

uint8_t GLCanvasHeightmapMode::SelectedRestriction() const
{
    return static_cast<uint8_t>((m_canvas.SelectedHeightmapCellValue() >> 12) & 0x0F);
}

void GLCanvasHeightmapMode::SetSelectedHeightmapCell(uint16_t value, bool refresh_object_placements)
{
    auto map = m_canvas.CurrentRoomMap();
    int x = m_canvas.SelectedHeightmapCellX();
    int y = m_canvas.SelectedHeightmapCellY();
    if (!map || x < 0 || y < 0) {
        return;
    }

    if (map->GetHeightmapCell({x, y}) == value) {
        return;
    }

    map->SetHeightmapCell({x, y}, value);
    m_canvas.UpdateHeightmapClipboardFromSelectedCell();
    m_canvas.ReloadCurrentRoomMapView();
    if (refresh_object_placements) {
        m_canvas.RefreshObjectPlacementsFromHeightmap();
    }
    m_canvas.NotifyHeightmapChanged(false);
    m_canvas.Refresh();
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
    SetSelectedHeightmapCell(0x4000, true);
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
