#include "GLCanvasRoomMode.h"
#include "GLCanvasTileDoorEditor.h"
#include "GLCanvasWarpEditor.h"

#include <algorithm>
#include <array>
#include <set>

namespace {

float OpacityForIndex(int idx)
{
    static constexpr float opacities[] = {1.0f, 0.5f, 0.0f};
    return opacities[idx % 3];
}

}  // namespace

GLCanvasRoomMode::GLCanvasRoomMode(MyGLCanvas& canvas)
    : m_canvas(canvas)
{
}

bool GLCanvasRoomMode::HandleKeyDown(wxKeyEvent& evt)
{
    uint16_t room_count = m_canvas.m_gd->GetRoomData()->GetRoomCount();
    bool ctrl = evt.ControlDown();
    bool shift = evt.ShiftDown();
    bool alt = evt.AltDown();

    int unicode_key = evt.GetUnicodeKey();
    if (unicode_key == '[' || unicode_key == '{') {
        if (m_canvas.m_selected_tileswap_region_idx >= 0) {
            m_canvas.CycleSelectedTileSwapId(-1);
        } else {
            m_canvas.ReorderSelectedObject(-1);
        }
        m_canvas.Refresh();
        return true;
    }
    if (unicode_key == ']' || unicode_key == '}') {
        if (m_canvas.m_selected_tileswap_region_idx >= 0) {
            m_canvas.CycleSelectedTileSwapId(1);
        } else {
            m_canvas.ReorderSelectedObject(1);
        }
        m_canvas.Refresh();
        return true;
    }

    switch (evt.GetKeyCode()) {
        case '1':
            if (!shift) {
                m_canvas.SetEditorMode(MyGLCanvas::EditorMode::Room);
                m_canvas.Refresh();
                return true;
            }
            break;
        case '2':
            if (!shift) {
                m_canvas.SetEditorMode(MyGLCanvas::EditorMode::Heightmap);
                m_canvas.Refresh();
                return true;
            }
            break;
        case '3':
            if (!shift) {
                m_canvas.SetEditorMode(MyGLCanvas::EditorMode::BackgroundLayer);
                m_canvas.Refresh();
                return true;
            }
            break;
        case '4':
            if (!shift) {
                m_canvas.SetEditorMode(MyGLCanvas::EditorMode::ForegroundLayer);
                m_canvas.Refresh();
                return true;
            }
            break;
        case WXK_F4:
            if (alt) {
                auto* f = wxDynamicCast(m_canvas.GetParent(), wxFrame);
                if (f) {
                    f->Close();
                }
            }
            break;
        case WXK_ESCAPE:
            if (m_canvas.HasPendingObjectAdd()) {
                m_canvas.CancelPendingObjectAdd();
                return true;
            }
            if (m_canvas.m_dragging_entity || m_canvas.m_dragging_warp ||
                m_canvas.m_dragging_door || m_canvas.m_dragging_tileswap_region) {
                return true;
            }
            m_canvas.m_selected_entity_idx = -1;
            m_canvas.m_selected_warp_idx = -1;
            m_canvas.m_selected_tileswap_region_idx = -1;
            m_canvas.m_selected_door_idx = -1;
            m_canvas.m_hovered_entity_idx = -1;
            m_canvas.m_hovered_warp_idx = -1;
            m_canvas.m_hovered_tileswap_region_idx = -1;
            m_canvas.m_hovered_door_idx = -1;
            break;
        case WXK_LEFT:
            m_canvas.PanCameraByStep(1, 0);
            break;
        case WXK_RIGHT:
            m_canvas.PanCameraByStep(-1, 0);
            break;
        case WXK_UP:
            m_canvas.PanCameraByStep(0, 1);
            break;
        case WXK_DOWN:
            m_canvas.PanCameraByStep(0, -1);
            break;
        case WXK_PAGEUP:
            if (!m_canvas.HasPendingObjectAdd() && !m_canvas.m_dragging_entity &&
                !m_canvas.m_dragging_warp && !m_canvas.m_dragging_door &&
                !m_canvas.m_dragging_tileswap_region) {
                m_canvas.NavigateToRoom((m_canvas.m_current_room + 1) % room_count);
            }
            break;
        case WXK_PAGEDOWN:
            if (!m_canvas.HasPendingObjectAdd() && !m_canvas.m_dragging_entity &&
                !m_canvas.m_dragging_warp && !m_canvas.m_dragging_door &&
                !m_canvas.m_dragging_tileswap_region) {
                m_canvas.NavigateToRoom(m_canvas.m_current_room > 0 ? m_canvas.m_current_room - 1 : room_count - 1);
            }
            break;
        case WXK_TAB:
            if (!m_canvas.HasPendingObjectAdd() && !m_canvas.m_dragging_entity &&
                !m_canvas.m_dragging_warp && !m_canvas.m_dragging_door &&
                !m_canvas.m_dragging_tileswap_region) {
                m_canvas.SelectNextObject(ctrl ? -1 : 1);
            }
            break;
        case WXK_SPACE:
            if (m_canvas.m_selected_door_idx >= 0) {
                m_canvas.ToggleSelectedDoorPreview();
            } else {
                m_canvas.ToggleSelectedTileSwapPreview();
            }
            break;
        case 'c':
        case 'C':
            if (ctrl) {
                m_canvas.CopySelectedEntity();
            }
            break;
        case 'v':
        case 'V':
            if (ctrl) {
                m_canvas.PasteEntity();
            }
            break;
        case WXK_INSERT:
            if (shift) {
                m_canvas.AddWarpHalf();
            } else if (ctrl) {
                m_canvas.AddTileSwap();
            } else if (alt) {
                m_canvas.AddDoor();
            } else {
                m_canvas.AddEntity();
            }
            break;
        case WXK_DELETE:
            m_canvas.DeleteSelectedObject();
            break;
        case ',':
        case '<':
            if (m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Door ||
                m_canvas.m_selected_door_idx >= 0) {
                m_canvas.CycleSelectedDoorSize(-1);
            } else if (m_canvas.m_selected_tileswap_region_idx >= 0) {
                m_canvas.CycleSelectedTileSwapShape(-1);
            } else if (m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Warp ||
                       m_canvas.m_selected_warp_idx >= 0) {
                m_canvas.CycleSelectedWarpType(-1);
            } else {
                m_canvas.CycleSelectedEntityId(-1);
            }
            break;
        case '.':
        case '>':
            if (m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Door ||
                m_canvas.m_selected_door_idx >= 0) {
                m_canvas.CycleSelectedDoorSize(1);
            } else if (m_canvas.m_selected_tileswap_region_idx >= 0) {
                m_canvas.CycleSelectedTileSwapShape(1);
            } else if (m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Warp ||
                       m_canvas.m_selected_warp_idx >= 0) {
                m_canvas.CycleSelectedWarpType(1);
            } else {
                m_canvas.CycleSelectedEntityId(1);
            }
            break;
        case '+':
        case '=':
        case WXK_NUMPAD_ADD:
            if (ctrl) {
                int w = 0;
                int h = 0;
                m_canvas.GetClientSize(&w, &h);
                m_canvas.ChangeZoomStep(1, float(w) * 0.5f, float(h) * 0.5f);
            } else {
                m_canvas.m_heightmapRenderer.AdjustZExtent(4.0f);
                m_canvas.RefreshObjectPlacementsFromHeightmap();
            }
            break;
        case '-':
        case WXK_NUMPAD_SUBTRACT:
            if (ctrl) {
                int w = 0;
                int h = 0;
                m_canvas.GetClientSize(&w, &h);
                m_canvas.ChangeZoomStep(-1, float(w) * 0.5f, float(h) * 0.5f);
            } else {
                m_canvas.m_heightmapRenderer.AdjustZExtent(-4.0f);
                m_canvas.RefreshObjectPlacementsFromHeightmap();
            }
            break;
        case 'b':
        case 'B':
            m_canvas.m_bg_opacity_idx = (m_canvas.m_bg_opacity_idx + 1) % 3;
            m_canvas.m_mapRenderer.SetBackgroundOpacity(OpacityForIndex(m_canvas.m_bg_opacity_idx));
            m_canvas.NotifyLayerOpacityChanged();
            break;
        case 'g':
        case 'G':
            m_canvas.m_fg_opacity_idx = (m_canvas.m_fg_opacity_idx + 1) % 3;
            m_canvas.m_mapRenderer.SetForegroundOpacity(OpacityForIndex(m_canvas.m_fg_opacity_idx));
            m_canvas.NotifyLayerOpacityChanged();
            break;
        case 'e':
        case 'E':
            m_canvas.m_sprite_opacity_idx = (m_canvas.m_sprite_opacity_idx + 1) % 3;
            m_canvas.m_spriteRenderer.SetOpacity(OpacityForIndex(m_canvas.m_sprite_opacity_idx));
            m_canvas.NotifyLayerOpacityChanged();
            break;
        case 'x':
        case 'X':
            if (ctrl) {
                m_canvas.CutSelectedEntity();
            } else {
                m_canvas.m_show_hitboxes = !m_canvas.m_show_hitboxes;
            }
            break;
        case 'w':
        case 'W':
            if (m_canvas.m_selected_tileswap_region_idx >= 0 && shift) {
                m_canvas.ResizeSelectedTileSwapByDelta(0, -1);
            } else if (m_canvas.m_selected_tileswap_region_idx >= 0) {
                m_canvas.NudgeSelectedObject(0.0f, -1.0f, 0.0f);
            } else if (ctrl && (m_canvas.m_selected_entity_idx >= 0 ||
                       m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Entity)) {
                m_canvas.SetSelectedEntityOrientation(Landstalker::Orientation::NW);
            } else if (shift && (m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Warp ||
                                 m_canvas.m_selected_warp_idx >= 0)) {
                m_canvas.ResizeSelectedWarp(0.0f, -1.0f);
            } else if (ctrl && m_canvas.m_selected_warp_idx >= 0) {
                m_canvas.RotateSelectedWarp(0.0f, -1.0f);
            } else {
                m_canvas.NudgeSelectedObject(0.0f, -1.0f, 0.0f);
            }
            break;
        case 'a':
        case 'A':
            if (m_canvas.m_selected_tileswap_region_idx >= 0 && shift) {
                m_canvas.ResizeSelectedTileSwapByDelta(-1, 0);
            } else if (m_canvas.m_selected_tileswap_region_idx >= 0) {
                m_canvas.NudgeSelectedObject(-1.0f, 0.0f, 0.0f);
            } else if (ctrl && (m_canvas.m_selected_entity_idx >= 0 ||
                       m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Entity)) {
                m_canvas.SetSelectedEntityOrientation(Landstalker::Orientation::SW);
            } else if (shift && (m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Warp ||
                                 m_canvas.m_selected_warp_idx >= 0)) {
                m_canvas.ResizeSelectedWarp(-1.0f, 0.0f);
            } else if (ctrl && m_canvas.m_selected_warp_idx >= 0) {
                m_canvas.RotateSelectedWarp(-1.0f, 0.0f);
            } else {
                m_canvas.NudgeSelectedObject(-1.0f, 0.0f, 0.0f);
            }
            break;
        case 's':
        case 'S':
            if (m_canvas.m_selected_tileswap_region_idx >= 0 && shift) {
                m_canvas.ResizeSelectedTileSwapByDelta(0, 1);
            } else if (m_canvas.m_selected_tileswap_region_idx >= 0) {
                m_canvas.NudgeSelectedObject(0.0f, 1.0f, 0.0f);
            } else if (ctrl && (m_canvas.m_selected_entity_idx >= 0 ||
                       m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Entity)) {
                m_canvas.SetSelectedEntityOrientation(Landstalker::Orientation::SE);
            } else if (shift && (m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Warp ||
                                 m_canvas.m_selected_warp_idx >= 0)) {
                m_canvas.ResizeSelectedWarp(0.0f, 1.0f);
            } else if (ctrl && m_canvas.m_selected_warp_idx >= 0) {
                m_canvas.RotateSelectedWarp(0.0f, 1.0f);
            } else {
                m_canvas.NudgeSelectedObject(0.0f, 1.0f, 0.0f);
            }
            break;
        case 'd':
        case 'D':
            if (m_canvas.m_selected_tileswap_region_idx >= 0 && shift) {
                m_canvas.ResizeSelectedTileSwapByDelta(1, 0);
            } else if (m_canvas.m_selected_tileswap_region_idx >= 0) {
                m_canvas.NudgeSelectedObject(1.0f, 0.0f, 0.0f);
            } else if (ctrl && (m_canvas.m_selected_entity_idx >= 0 ||
                       m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Entity)) {
                m_canvas.SetSelectedEntityOrientation(Landstalker::Orientation::NE);
            } else if (shift && (m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Warp ||
                                 m_canvas.m_selected_warp_idx >= 0)) {
                m_canvas.ResizeSelectedWarp(1.0f, 0.0f);
            } else if (ctrl && m_canvas.m_selected_warp_idx >= 0) {
                m_canvas.RotateSelectedWarp(1.0f, 0.0f);
            } else {
                m_canvas.NudgeSelectedObject(1.0f, 0.0f, 0.0f);
            }
            break;
        case 'r':
        case 'R':
            m_canvas.NudgeSelectedObject(0.0f, 0.0f, 0.5f);
            break;
        case 'f':
        case 'F':
            if (ctrl) {
                m_canvas.SetSelectedEntityToFloor();
            } else {
                m_canvas.NudgeSelectedObject(0.0f, 0.0f, -0.5f);
            }
            break;
        case 'h':
        case 'H':
            m_canvas.m_show_heightmap = !m_canvas.m_show_heightmap;
            break;
        case 'z':
        case 'Z':
            m_canvas.m_entity_occlusion_idx = (m_canvas.m_entity_occlusion_idx + 1) % 3;
            break;
        case 'o':
        case 'O':
            m_canvas.m_debug_occlusion = !m_canvas.m_debug_occlusion;
            break;
        case 'l':
        case 'L':
            if (m_canvas.m_selected_entity_idx >= 0 && m_canvas.m_selected_entity_idx < static_cast<int>(m_canvas.m_instances.size())) {
                m_canvas.WriteSelectedEntityOcclusionDebugLog();
            } else {
                m_canvas.WriteEntityDrawOrderDebugLog();
            }
            break;
        case 'p':
        case 'P':
            m_canvas.CycleSelectedEntityPalette();
            break;
        case 'q':
        case 'Q':
            m_canvas.WriteEntityDrawOrderDebugLog();
            break;
    }

    m_canvas.Refresh();
    return true;
}

void GLCanvasRoomMode::HandleMouseMove(const wxMouseEvent& evt)
{
    if (m_canvas.HasPendingObjectAdd()) {
        m_canvas.UpdatePendingObjectAddHover();
        m_canvas.SetCursor(wxCursor(wxCURSOR_CROSS));
        m_canvas.Refresh();
        return;
    }

    if (m_canvas.m_dragging_entity) {
        m_canvas.UpdateEntityDrag(evt);
        return;
    }
    if (m_canvas.m_dragging_warp) {
        m_canvas.UpdateWarpDrag(evt);
        return;
    }
    if (m_canvas.m_dragging_door) {
        m_canvas.UpdateDoorDrag(evt);
        return;
    }
    if (m_canvas.m_dragging_tileswap_region) {
        m_canvas.UpdateTileSwapRegionDrag(evt);
        return;
    }

    if (m_canvas.HitTestRoomInfoLink(evt.GetPosition()) >= 0) {
        m_canvas.SetCursor(wxCursor(wxCURSOR_HAND));
        return;
    }

    m_canvas.m_heightmapRenderer.SetHoverPoint(
        m_canvas.ScreenToWorldX(evt.GetPosition().x),
        m_canvas.ScreenToWorldY(evt.GetPosition().y));

    int hovered_control = m_canvas.HitTestEntityZControl(evt.GetPosition());
    int hovered_body = hovered_control >= 0 ? hovered_control : m_canvas.HitTestEntityBody(evt.GetPosition());
    int hovered_warp_resize = hovered_body < 0 ? m_canvas.HitTestWarpResizeControl(evt.GetPosition()) : 0;
    int hovered_warp = hovered_body < 0 && hovered_warp_resize == 0 ? m_canvas.HitTestWarp(evt.GetPosition()) : -1;
    int hovered = hovered_body >= 0 || hovered_warp >= 0 || hovered_warp_resize != 0 ? hovered_body : m_canvas.HitTestEntity(evt.GetPosition());
    int hovered_tileswap_resize = hovered < 0 && hovered_warp < 0 && hovered_warp_resize == 0
        ? m_canvas.HitTestTileSwapRegionResizeControl(evt.GetPosition())
        : 0;
    int hovered_tileswap_region = hovered < 0 && hovered_warp < 0 && hovered_warp_resize == 0
        && hovered_tileswap_resize == 0
        ? m_canvas.HitTestTileSwapRegion(evt.GetPosition())
        : -1;
    int hovered_door = hovered < 0 && hovered_warp < 0 && hovered_warp_resize == 0
        && hovered_tileswap_resize == 0 && hovered_tileswap_region < 0
        ? m_canvas.HitTestDoor(evt.GetPosition())
        : -1;

    if (hovered != m_canvas.m_hovered_entity_idx || hovered_warp != m_canvas.m_hovered_warp_idx ||
        hovered_tileswap_region != m_canvas.m_hovered_tileswap_region_idx || hovered_door != m_canvas.m_hovered_door_idx) {
        m_canvas.m_hovered_entity_idx = hovered;
        m_canvas.m_hovered_warp_idx = hovered_warp;
        m_canvas.m_hovered_tileswap_region_idx = hovered_tileswap_region;
        m_canvas.m_hovered_door_idx = hovered_door;
    }

    bool z_cursor = hovered_control >= 0 || (m_canvas.m_hovered_entity_idx >= 0 && evt.ControlDown());
    if (z_cursor) {
        m_canvas.SetCursor(wxCursor(wxCURSOR_SIZENS));
    } else if (hovered_warp_resize == 1) {
        m_canvas.SetCursor(wxCursor(wxCURSOR_SIZENWSE));
    } else if (hovered_warp_resize == 2) {
        m_canvas.SetCursor(wxCursor(wxCURSOR_SIZENESW));
    } else if (hovered_tileswap_resize != 0) {
        m_canvas.SetCursor(wxCursor(wxCURSOR_SIZENWSE));
    } else {
        m_canvas.SetCursor(wxCursor((m_canvas.m_hovered_entity_idx >= 0 || m_canvas.m_hovered_warp_idx >= 0 ||
            m_canvas.m_hovered_tileswap_region_idx >= 0 || m_canvas.m_hovered_door_idx >= 0) ? wxCURSOR_HAND : wxCURSOR_ARROW));
    }

    m_canvas.Refresh();
}

void GLCanvasRoomMode::HandleLeftDown(const wxMouseEvent& evt)
{
    if (m_canvas.HasPendingObjectAdd()) {
        bool pending_entity_add = m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Entity;
        m_canvas.CommitPendingObjectAdd();
        m_canvas.SetFocus();
        if (pending_entity_add && m_canvas.m_selected_entity_idx >= 0) {
            m_canvas.StartEntityDrag(m_canvas.m_selected_entity_idx, evt, evt.ControlDown());
        }
        m_canvas.Refresh();
        return;
    }

    int room_info_room = m_canvas.HitTestRoomInfoLink(evt.GetPosition());
    if (room_info_room >= 0) {
        m_canvas.NavigateToRoom(static_cast<uint16_t>(room_info_room));
        m_canvas.Refresh();
        return;
    }

    int control_idx = m_canvas.HitTestEntityZControl(evt.GetPosition());
    m_canvas.m_selected_entity_idx = control_idx >= 0 ? control_idx : m_canvas.HitTestEntityBody(evt.GetPosition());
    if (m_canvas.m_selected_entity_idx >= 0) {
        m_canvas.m_selected_warp_idx = -1;
        m_canvas.m_selected_tileswap_region_idx = -1;
        m_canvas.m_selected_door_idx = -1;
        m_canvas.StartEntityDrag(m_canvas.m_selected_entity_idx, evt, control_idx >= 0 || evt.ControlDown());
    } else {
        int resize_axis = m_canvas.HitTestWarpResizeControl(evt.GetPosition());
        if (resize_axis != 0 && m_canvas.m_selected_warp_idx >= 0) {
            m_canvas.m_selected_tileswap_region_idx = -1;
            m_canvas.m_selected_door_idx = -1;
            m_canvas.StartWarpResizeDrag(m_canvas.m_selected_warp_idx, resize_axis, evt);
            m_canvas.Refresh();
            return;
        }

        m_canvas.m_selected_warp_idx = m_canvas.HitTestWarp(evt.GetPosition());
        if (m_canvas.m_selected_warp_idx >= 0) {
            m_canvas.m_selected_tileswap_region_idx = -1;
            m_canvas.m_selected_door_idx = -1;
            m_canvas.StartWarpDrag(m_canvas.m_selected_warp_idx, evt);
        } else {
            m_canvas.m_selected_entity_idx = m_canvas.HitTestEntity(evt.GetPosition());
            if (m_canvas.m_selected_entity_idx >= 0) {
                m_canvas.m_selected_warp_idx = -1;
                m_canvas.m_selected_tileswap_region_idx = -1;
                m_canvas.m_selected_door_idx = -1;
                m_canvas.StartEntityDrag(m_canvas.m_selected_entity_idx, evt, evt.ControlDown(), true);
            } else {
                int tileswap_resize_axis = m_canvas.HitTestTileSwapRegionResizeControl(evt.GetPosition());
                if (tileswap_resize_axis != 0 && m_canvas.m_selected_tileswap_region_idx >= 0) {
                    m_canvas.m_selected_door_idx = -1;
                    m_canvas.StartTileSwapRegionDrag(m_canvas.m_selected_tileswap_region_idx, tileswap_resize_axis, evt);
                    m_canvas.Refresh();
                    return;
                }
                m_canvas.m_selected_tileswap_region_idx = m_canvas.HitTestTileSwapRegion(evt.GetPosition());
                m_canvas.m_hovered_tileswap_region_idx = m_canvas.m_selected_tileswap_region_idx;
                if (m_canvas.m_selected_tileswap_region_idx >= 0) {
                    m_canvas.m_selected_door_idx = -1;
                    m_canvas.StartTileSwapRegionDrag(m_canvas.m_selected_tileswap_region_idx, 0, evt);
                } else {
                    m_canvas.m_selected_door_idx = m_canvas.HitTestDoor(evt.GetPosition());
                    m_canvas.m_hovered_door_idx = m_canvas.m_selected_door_idx;
                    if (m_canvas.m_selected_door_idx >= 0) {
                        m_canvas.StartDoorDrag(m_canvas.m_selected_door_idx, evt);
                    } else {
                        m_canvas.m_dragging_pan = true;
                        m_canvas.m_drag_pan_start_mouse = evt.GetPosition();
                        m_canvas.m_drag_pan_start_cam_x = m_canvas.m_cam_x;
                        m_canvas.m_drag_pan_start_cam_y = m_canvas.m_cam_y;
                        m_canvas.SetCursor(wxCursor(wxCURSOR_SIZING));
                        if (!m_canvas.HasCapture()) {
                            m_canvas.CaptureMouse();
                        }
                    }
                }
            }
        }
    }
    m_canvas.Refresh();
}

void GLCanvasRoomMode::HandleRightDown(const wxMouseEvent& evt)
{
    if (m_canvas.HasPendingObjectAdd()) {
        m_canvas.CancelPendingObjectAdd();
        return;
    }

    int room_info_room = m_canvas.HitTestRoomInfoLink(evt.GetPosition());
    if (room_info_room >= 0) {
        m_canvas.NavigateToRoom(static_cast<uint16_t>(room_info_room));
        m_canvas.Refresh();
        return;
    }

    int warp_idx = m_canvas.HitTestWarp(evt.GetPosition());
    if (warp_idx >= 0) {
        m_canvas.m_selected_warp_idx = warp_idx;
        m_canvas.m_selected_entity_idx = -1;
        m_canvas.m_selected_tileswap_region_idx = -1;
        m_canvas.m_selected_door_idx = -1;
        m_canvas.NavigateToRoom(m_canvas.m_warps[static_cast<std::size_t>(warp_idx)].DestinationRoom());
        m_canvas.Refresh();
        return;
    }

    m_canvas.m_selected_entity_idx = m_canvas.HitTestEntityBody(evt.GetPosition());
    if (m_canvas.m_selected_entity_idx < 0) {
        m_canvas.m_selected_entity_idx = m_canvas.HitTestEntity(evt.GetPosition());
    }
    if (m_canvas.m_selected_entity_idx >= 0) {
        m_canvas.m_selected_warp_idx = -1;
        m_canvas.m_selected_tileswap_region_idx = -1;
        m_canvas.m_selected_door_idx = -1;
        m_canvas.StartEntityDrag(m_canvas.m_selected_entity_idx, evt, true);
    } else {
        int tileswap_region = m_canvas.HitTestTileSwapRegion(evt.GetPosition());
        if (tileswap_region >= 0) {
            m_canvas.m_selected_entity_idx = -1;
            m_canvas.m_selected_warp_idx = -1;
            m_canvas.m_selected_tileswap_region_idx = tileswap_region;
            m_canvas.m_selected_door_idx = -1;
            m_canvas.m_hovered_tileswap_region_idx = tileswap_region;
            m_canvas.ToggleSelectedTileSwapPreview();
        } else {
            int door_idx = m_canvas.HitTestDoor(evt.GetPosition());
            if (door_idx >= 0) {
                m_canvas.m_selected_entity_idx = -1;
                m_canvas.m_selected_warp_idx = -1;
                m_canvas.m_selected_tileswap_region_idx = -1;
                m_canvas.m_selected_door_idx = door_idx;
                m_canvas.m_hovered_door_idx = door_idx;
                m_canvas.ToggleSelectedDoorPreview();
            }
        }
    }
    m_canvas.Refresh();
}

void GLCanvasRoomMode::Render(int width, int height)
{
    // Delegate drawing in strict layer order to keep occlusion deterministic.
    // Earlier passes also prepare stencil information used by later passes.
    m_canvas.m_mapRenderer.Render(m_canvas.m_cam_x, m_canvas.m_cam_y);
    if (m_canvas.m_show_heightmap) {
        m_canvas.m_heightmapRenderer.Render();
    }
    if (m_canvas.m_show_tile_swaps) {
        m_canvas.RenderTileSwapOutlines();
        m_canvas.RenderDoors();
    }
    if (m_canvas.m_show_warps) {
        m_canvas.RenderWarps();
    }
    if (m_canvas.HasPendingObjectAdd()) {
        GLCanvasWarpEditor(m_canvas).RenderPendingWarpGhost();
        GLCanvasTileDoorEditor(m_canvas).RenderPendingDoorGhost();
    }

    SpriteRenderer::OcclusionMode occlusion_mode = SpriteRenderer::OcclusionMode::AlwaysOnTop;
    switch (m_canvas.m_entity_occlusion_idx) {
        case 1:
            occlusion_mode = SpriteRenderer::OcclusionMode::ObscuredTransparent;
            break;
        case 2:
            occlusion_mode = SpriteRenderer::OcclusionMode::ObscuredHidden;
            break;
        case 0:
        default:
            occlusion_mode = SpriteRenderer::OcclusionMode::AlwaysOnTop;
            break;
    }

    const GLint sprite_occlusion_ref = m_canvas.m_show_heightmap ? 0x01 : 0x05;
    const GLint sprite_occlusion_mask = m_canvas.m_show_heightmap ? 0x01 : 0x05;
    auto build_entity_occlusion_stencil = [this](
        GLint entity_back_depth,
        GLint entity_front_depth,
        float entity_z,
        float entity_min_x,
        float entity_min_y,
        float entity_max_x,
        float entity_max_y,
        float entity_top_z,
        float sprite_min_x,
        float sprite_min_y,
        float sprite_max_x,
        float sprite_max_y) {
        m_canvas.m_heightmapRenderer.BuildEntityOcclusionStencil(
            entity_back_depth,
            entity_front_depth,
            entity_z,
            entity_min_x,
            entity_min_y,
            entity_max_x,
            entity_max_y,
            entity_top_z,
            sprite_min_x,
            sprite_min_y,
            sprite_max_x,
            sprite_max_y);
        if (!m_canvas.m_show_heightmap) {
            m_canvas.m_mapRenderer.BuildForegroundCoverageStencil();
        }
    };
    auto floor_at_point = [this](float x, float y) {
        return m_canvas.FloorUnderPoint(x, y);
    };
    auto shadow_occluded = [this](float min_x, float min_y, float max_x, float max_y, float z) {
        return m_canvas.ShadowOccludedByHeightmap(min_x, min_y, max_x, max_y, z);
    };
    auto build_shadow_occlusion_stencil = [this](float min_x, float min_y, float max_x, float max_y, float z, float screen_min_x, float screen_min_y, float screen_max_x, float screen_max_y) {
        GLint back_depth = std::clamp(static_cast<int>(std::floor(min_x + min_y + 1.0f)), 0, 255);
        GLint front_depth = std::clamp(static_cast<int>(std::ceil(max_x + max_y + 25.0f)), 0, 255);
        m_canvas.m_heightmapRenderer.BuildEntityOcclusionStencil(
            back_depth,
            front_depth,
            z,
            min_x,
            min_y,
            max_x,
            max_y,
            z + 0.125f,
            screen_min_x,
            screen_min_y,
            screen_max_x,
            screen_max_y);
    };

    bool pending_entity_preview = m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Entity;
    if (m_canvas.m_show_entities || m_canvas.m_show_hitboxes || pending_entity_preview) {
        std::set<uint32_t> collided_entities = m_canvas.FindCollidedEntityIds();
        int selected_collision_warning = 0;
        if (m_canvas.m_selected_entity_idx >= 0 &&
            m_canvas.m_selected_entity_idx < static_cast<int>(m_canvas.m_instances.size())) {
            const SpriteInstance& selected = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)];
            if (m_canvas.EntityCollidesWithHeightmap(selected)) {
                selected_collision_warning = 2;
            } else if (collided_entities.count(selected.instance_id) != 0) {
                selected_collision_warning = 1;
            }
        }

        if (m_canvas.m_show_entities || m_canvas.m_show_hitboxes) {
            m_canvas.m_spriteRenderer.Render(
                m_canvas.m_instances,
                m_canvas.m_cam_x,
                m_canvas.m_cam_y,
                m_canvas.m_selected_entity_idx,
                selected_collision_warning,
                occlusion_mode,
                m_canvas.m_show_entities,
                m_canvas.m_show_hitboxes,
                sprite_occlusion_ref,
                sprite_occlusion_mask,
                build_entity_occlusion_stencil,
                floor_at_point,
                shadow_occluded,
                build_shadow_occlusion_stencil,
                [&collided_entities](uint32_t instance_id) {
                    return collided_entities.count(instance_id) != 0;
                });

            m_canvas.RenderEntityControls();
            m_canvas.RenderSelectedEntityTooltip();
        }

        SpriteInstance ghost{};
        if (m_canvas.BuildPendingEntityPreviewInstance(ghost)) {
            // Restore the world-space ModelView matrix, which may have been reset to
            // identity by RenderSelectedEntityTooltip (or RenderEntityControls) for HUD drawing.
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glTranslatef(m_canvas.m_cam_x, m_canvas.m_cam_y, 0.0f);
            glScalef(m_canvas.ZoomFactor(), m_canvas.ZoomFactor(), 1.0f);

            std::vector<SpriteInstance> ghost_instances{ghost};
            m_canvas.m_spriteRenderer.SetOpacity(0.45f);
            m_canvas.m_spriteRenderer.Render(
                ghost_instances,
                m_canvas.m_cam_x,
                m_canvas.m_cam_y,
                -1,
                0,
                occlusion_mode,
                true,
                m_canvas.m_show_hitboxes,
                sprite_occlusion_ref,
                sprite_occlusion_mask,
                build_entity_occlusion_stencil,
                floor_at_point,
                shadow_occluded,
                build_shadow_occlusion_stencil,
                [](uint32_t) {
                    return false;
                });
            m_canvas.m_spriteRenderer.SetOpacity(OpacityForIndex(m_canvas.m_sprite_opacity_idx));
        }
    }
    if (m_canvas.m_show_warps) {
        m_canvas.RenderSelectedWarpTooltip();
    }
    if (m_canvas.m_show_tile_swaps) {
        m_canvas.RenderSelectedDoorTooltip();
        m_canvas.RenderSelectedTileSwapRegionTooltip();
    }
    m_canvas.RenderPendingObjectAddOverlay();

    if (m_canvas.m_debug_occlusion &&
        m_canvas.m_selected_entity_idx >= 0 &&
        m_canvas.m_selected_entity_idx < static_cast<int>(m_canvas.m_instances.size())) {
        const auto& selected = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)];
        float center_x = selected.map_x + selected.hitbox_offset;
        float center_y = selected.map_y + selected.hitbox_offset;
        float half_base = selected.hitbox_base * 0.5f;
        float top_z = selected.map_z + std::max(selected.hitbox_height, 0.125f);

        float min_depth = (center_x - half_base) + (center_y - half_base);
        float max_depth = (center_x + half_base) + (center_y + half_base) + 25.0f;
        int rear_edge_padding = std::abs(selected.map_z - selected.floor_z) <= 0.01f ? 1 : 0;
        auto depth_range = std::pair<GLint, GLint>{
            std::clamp(static_cast<int>(std::ceil(min_depth)) + rear_edge_padding, 0, 255),
            std::clamp(static_cast<int>(std::ceil(max_depth)), 0, 255)
        };

        auto project = [&selected](float x, float y, float z) {
            float grid_x = x - selected.room_left;
            float grid_y = y - selected.room_top;
            return std::pair<float, float>{
                32.0f * grid_x - 32.0f * grid_y + 512.0f,
                16.0f * grid_x + 16.0f * grid_y + 100.0f - selected.z_extent * z
            };
        };
        std::array<std::pair<float, float>, 8> clip_bounds = {
            project(center_x - half_base, center_y - half_base, selected.map_z),
            project(center_x + half_base, center_y - half_base, selected.map_z),
            project(center_x + half_base, center_y + half_base, selected.map_z),
            project(center_x - half_base, center_y + half_base, selected.map_z),
            project(center_x - half_base, center_y - half_base, top_z),
            project(center_x + half_base, center_y - half_base, top_z),
            project(center_x + half_base, center_y + half_base, top_z),
            project(center_x - half_base, center_y + half_base, top_z)
        };

        float clip_min_x = clip_bounds.front().first;
        float clip_min_y = clip_bounds.front().second;
        float clip_max_x = clip_bounds.front().first;
        float clip_max_y = clip_bounds.front().second;
        for (const auto& point : clip_bounds) {
            clip_min_x = std::min(clip_min_x, point.first);
            clip_min_y = std::min(clip_min_y, point.second);
            clip_max_x = std::max(clip_max_x, point.first);
            clip_max_y = std::max(clip_max_y, point.second);
        }

        if (!m_canvas.m_show_heightmap) {
            m_canvas.m_heightmapRenderer.BuildEntityOcclusionStencil(
                depth_range.first,
                depth_range.second,
                selected.map_z,
                center_x - half_base,
                center_y - half_base,
                center_x + half_base,
                center_y + half_base,
                top_z,
                clip_min_x,
                clip_min_y,
                clip_max_x,
                clip_max_y);
            m_canvas.m_mapRenderer.BuildForegroundCoverageStencil();
            m_canvas.RenderStencilOverlay(width, height, 0x04, 0x04, 0.0f, 0.8f, 1.0f, 0.22f);
            m_canvas.RenderStencilOverlay(width, height, 0x05, 0x05, 1.0f, 0.0f, 1.0f, 0.38f);
        }

        m_canvas.m_heightmapRenderer.RenderEntityOcclusionDebug(
            depth_range.first,
            depth_range.second,
            selected.map_z,
            center_x - half_base,
            center_y - half_base,
            center_x + half_base,
            center_y + half_base,
            top_z);
    }

    m_canvas.RenderRoomInfoTable(width, height);
    m_canvas.SwapBuffers();
    m_canvas.RecordRenderedFrame();
}
