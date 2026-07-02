#include "GLCanvas.h"
#include "GLCanvasEntityEditor.h"
#include "GLCanvasHeightmapHitTest.h"
#include "GLCanvasObjectCoordinator.h"
#include "GLCanvasObjectSupport.h"
#include "GLCanvasRoomInfoOverlay.h"
#include "GLCanvasTileDoorEditor.h"
#include "GLCanvasWarpEditor.h"
#include "RoomProjection.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <utility>

using namespace Landstalker;

namespace {
using PickPoint = RoomProjection::PickPoint;
using RoomProjection::ProjectHeightmapGridPoint;
using RoomProjection::ProjectRoomGridPoint;
using RoomProjection::ScreenToHeightmapPoint;
using RoomProjection::ScreenToMapPoint;
}  // namespace

void MyGLCanvas::UpdateEntityDrag(const wxMouseEvent& evt) {
    GLCanvasEntityEditor(*this).UpdateEntityDrag(evt);
}

void MyGLCanvas::EndEntityDrag() {
    GLCanvasEntityEditor(*this).EndEntityDrag();
}

void MyGLCanvas::StartWarpDrag(int warp_idx, const wxMouseEvent& evt) {
    GLCanvasWarpEditor(*this).StartWarpDrag(warp_idx, evt);
}

void MyGLCanvas::StartWarpResizeDrag(int warp_idx, int axis, const wxMouseEvent& evt) {
    GLCanvasWarpEditor(*this).StartWarpResizeDrag(warp_idx, axis, evt);
}

void MyGLCanvas::UpdateWarpDrag(const wxMouseEvent& evt) {
    GLCanvasWarpEditor(*this).UpdateWarpDrag(evt);
}

void MyGLCanvas::EndWarpDrag() {
    GLCanvasWarpEditor(*this).EndWarpDrag();
}

void MyGLCanvas::StartDoorDrag(int door_idx, const wxMouseEvent& evt) {
    GLCanvasTileDoorEditor(*this).StartDoorDrag(door_idx, evt);
}

void MyGLCanvas::UpdateDoorDrag(const wxMouseEvent& evt) {
    GLCanvasTileDoorEditor(*this).UpdateDoorDrag(evt);
}

void MyGLCanvas::EndDoorDrag() {
    GLCanvasTileDoorEditor(*this).EndDoorDrag();
}

void MyGLCanvas::StartTileSwapRegionDrag(int region_idx, int resize_axis, const wxMouseEvent& evt) {
    GLCanvasTileDoorEditor(*this).StartTileSwapRegionDrag(region_idx, resize_axis, evt);
}

void MyGLCanvas::UpdateTileSwapRegionDrag(const wxMouseEvent& evt) {
    GLCanvasTileDoorEditor(*this).UpdateTileSwapRegionDrag(evt);
}

void MyGLCanvas::EndTileSwapRegionDrag() {
    GLCanvasTileDoorEditor(*this).EndTileSwapRegionDrag();
}

bool MyGLCanvas::HasPendingObjectAdd() const {
    return m_pending_add_type != PendingObjectAddType::None;
}

void MyGLCanvas::UpdatePendingObjectAddHover() {
    if (!HasPendingObjectAdd()) {
        return;
    }

    if (m_pending_add_type == PendingObjectAddType::TileSwap) {
        if (m_pending_tileswap_part == PendingTileSwapPart::MapSource ||
            m_pending_tileswap_part == PendingTileSwapPart::MapDestination) {
            PickPoint point = ScreenToMapPoint(
                ScreenToWorldX(m_last_mouse_pos.x),
                ScreenToWorldY(m_last_mouse_pos.y),
                0.0f,
                static_cast<float>(m_mapRenderer.GetRoomLeft()),
                static_cast<float>(m_mapRenderer.GetRoomTop()));
            m_pending_add_hover_x = std::clamp(static_cast<int>(std::floor(point.x)), 0, 63);
            m_pending_add_hover_y = std::clamp(static_cast<int>(std::floor(point.y)), 0, 63);
        } else {
            // HeightmapSource / HeightmapDestination — use local heightmap cell coords
            // (same system as door.x/y and tileswap.heightmap.src_x/y)
            auto [hx, hy] = MouseHeightmapCell();
            m_pending_add_hover_x = hx;
            m_pending_add_hover_y = hy;
        }
        return;
    }

    PickPoint point = ScreenToMapPoint(
        ScreenToWorldX(m_last_mouse_pos.x),
        ScreenToWorldY(m_last_mouse_pos.y),
        m_pending_add_plane_z,
        static_cast<float>(m_mapRenderer.GetRoomLeft()),
        static_cast<float>(m_mapRenderer.GetRoomTop()),
        m_heightmapRenderer.GetZExtent());
    m_pending_add_hover_x = std::clamp(static_cast<int>(std::floor(point.x)), 0, 63);
    m_pending_add_hover_y = std::clamp(static_cast<int>(std::floor(point.y)), 0, 63);
}

bool MyGLCanvas::BuildPendingEntityPreviewInstance(SpriteInstance& inst) {
    if (m_pending_add_type != PendingObjectAddType::Entity) {
        return false;
    }
    if (m_pending_add_hover_x < 0 || m_pending_add_hover_y < 0) {
        return false;
    }

    inst = SpriteInstance{};
    inst.instance_id = static_cast<uint32_t>(m_room_entities.size() + 1);
    inst.entity_id = m_pending_add_entity_id;
    inst.palette = std::min<uint8_t>(m_pending_add_entity_palette, 3);
    inst.z_extent = m_heightmapRenderer.GetZExtent();
    inst.room_left = static_cast<float>(m_mapRenderer.GetRoomLeft());
    inst.room_top = static_cast<float>(m_mapRenderer.GetRoomTop());
    inst.dx = 0.0f;
    inst.dy = 0.0f;
    inst.scale = 2.0f;
    inst.anim_timer = 0.0f;
    inst.anim_speed = 1.0f;
    inst.orientation = m_pending_add_entity_orientation;
    RefreshEntityMetadata(inst);

    // Place ghost exactly on the hovered cell center — same coordinate source as the cell highlight.
    float cx = static_cast<float>(m_pending_add_hover_x) + 0.5f;
    float cy = static_cast<float>(m_pending_add_hover_y) + 0.5f;
    inst.map_x = std::clamp(cx - inst.hitbox_offset, 0.0f, 63.5f);
    inst.map_y = std::clamp(cy - inst.hitbox_offset, 0.0f, 63.5f);
    inst.floor_z = FloorUnderHitbox(cx, cy, inst.hitbox_base * 0.5f);
    inst.map_z = std::clamp(inst.floor_z, 0.0f, 15.5f);

    UpdateEntityProjection(inst);

    return true;
}

bool MyGLCanvas::BuildPendingWarpPreviewInstance(WarpInstance& inst) {
    if (m_pending_add_type != PendingObjectAddType::Warp) {
        return false;
    }
    if (m_pending_add_hover_x < 0 || m_pending_add_hover_y < 0) {
        return false;
    }
    // Use room-grid coords from UpdatePendingObjectAddHover (ScreenToMapPoint) so that
    // ProjectWarpGridPoint renders the ghost centred on the cursor.
    float half_w = std::round(m_pending_add_warp_width) * 0.5f;
    float half_h = std::round(m_pending_add_warp_height) * 0.5f;
    float cursor_x = static_cast<float>(m_pending_add_hover_x) + 0.5f;
    float cursor_y = static_cast<float>(m_pending_add_hover_y) + 0.5f;
    auto [x, y] = FindNearestFreeWarpCell(cursor_x - half_w, cursor_y - half_h);
    Landstalker::WarpList::Warp warp{};
    warp.room1 = m_current_room;
    warp.x1 = static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(x)), 0, 63));
    warp.y1 = static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(y)), 0, 63));
    warp.room2 = 0xFFFF;
    warp.x_size = static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(m_pending_add_warp_width)), 1, 3));
    warp.y_size = static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(m_pending_add_warp_height)), 1, 3));
    warp.type = m_pending_add_warp_type;
    inst = GLCanvasObjectSupport::MakeWarpInstance(
        warp, m_current_room, 0,
        float(m_mapRenderer.GetRoomLeft()), float(m_mapRenderer.GetRoomTop()),
        m_heightmapRenderer.GetZExtent());
    UpdateWarpFloor(inst);
    return true;
}

void MyGLCanvas::CancelPendingObjectAdd() {
    m_pending_add_type = PendingObjectAddType::None;
    m_pending_tileswap_part = PendingTileSwapPart::MapSource;
    m_pending_add_hover_x = -1;
    m_pending_add_hover_y = -1;
    m_pending_add_plane_z = 0.0f;
    m_pending_add_floor_snap = true;
    m_pending_add_entity_cursor_offset_x = 0.0f;
    m_pending_add_entity_cursor_offset_y = 0.0f;
    m_pending_add_swap = TileSwap{};
    m_pending_add_start_x = 0.0f;
    m_pending_add_start_y = 0.0f;
    m_pending_add_start_z = 0.0f;
    m_pending_add_mouse_start = wxPoint(-1, -1);
    m_pending_add_warp_width = 1.0f;
    m_pending_add_warp_height = 1.0f;
    m_pending_add_warp_type = Landstalker::WarpList::Warp::Type::NORMAL;
    m_pending_add_door_size = Door::Size::DOOR_1X4;
    SetCursor(wxCursor(wxCURSOR_ARROW));
    Refresh();
}

void MyGLCanvas::CommitPendingObjectAdd() {
    if (!HasPendingObjectAdd()) {
        return;
    }

    UpdatePendingObjectAddHover();
    switch (m_pending_add_type) {
        case PendingObjectAddType::Entity:
            if (m_room_entities.size() < 15) {
                SpriteInstance ghost{};
                if (!BuildPendingEntityPreviewInstance(ghost)) {
                    return;
                }
                CaptureObjectUndoState();
                GLCanvasEntityEditor(*this).AddEntity(ghost);
                NotifyRoomDataChanged(true, false, false, false);
                NotifySelectionChanged();
            }
            CancelPendingObjectAdd();
            return;
        case PendingObjectAddType::Warp:
            CaptureObjectUndoState();
            GLCanvasWarpEditor(*this).AddWarpHalf();
            NotifyRoomDataChanged(false, true, false, false);
            NotifySelectionChanged();
            CancelPendingObjectAdd();
            SetFocus();
            return;
        case PendingObjectAddType::Door:
            CaptureObjectUndoState();
            GLCanvasTileDoorEditor(*this).AddDoor();
            NotifyRoomDataChanged(false, false, false, true);
            NotifySelectionChanged();
            CancelPendingObjectAdd();
            return;
        case PendingObjectAddType::TileSwap:
            GLCanvasTileDoorEditor(*this).CommitPendingTileSwapStep();
            return;
        case PendingObjectAddType::None:
            return;
    }
}

void MyGLCanvas::RenderPendingObjectAddOverlay() {
    if (!HasPendingObjectAdd() || m_pending_add_hover_x < 0 || m_pending_add_hover_y < 0) {
        return;
    }

    float zoom = std::max(ZoomFactor(), 0.0001f);
    float room_left = static_cast<float>(m_mapRenderer.GetRoomLeft());
    float room_top = static_cast<float>(m_mapRenderer.GetRoomTop());
    auto height_at = [&](int x, int y) {
        auto map = CurrentRoomMap();
        if (!map || x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
            return 0.0f;
        }
        uint8_t height = map->GetHeight({x, y});
        return height == 0xFF ? 0.0f : static_cast<float>(height);
    };
    // Heightmap segments: isometric diamond shape.
    auto draw_diamond = [&](int x, int y, float r, float g, float b, float a) {
        PickPoint center = ProjectHeightmapGridPoint(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, height_at(x, y), room_left, room_top, m_heightmapRenderer.GetZExtent());
        float cx = center.x * zoom + m_cam_x;
        float cy = center.y * zoom + m_cam_y;
        glColor4f(r, g, b, a * 0.32f);
        glBegin(GL_QUADS);
        glVertex2f(cx, cy - 16.0f * zoom);
        glVertex2f(cx + 32.0f * zoom, cy);
        glVertex2f(cx, cy + 16.0f * zoom);
        glVertex2f(cx - 32.0f * zoom, cy);
        glEnd();
        glColor4f(r, g, b, a);
        glLineWidth(2.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(cx, cy - 16.0f * zoom);
        glVertex2f(cx + 32.0f * zoom, cy);
        glVertex2f(cx, cy + 16.0f * zoom);
        glVertex2f(cx - 32.0f * zoom, cy);
        glEnd();
    };
    // Tilemap (layer swap) segments: screen-aligned rectangle matching the tile footprint.
    auto draw_square = [&](int x, int y, float r, float g, float b, float a) {
        PickPoint center = ProjectRoomGridPoint(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, 0.0f, room_left, room_top);
        float cx = center.x * zoom + m_cam_x;
        float cy = center.y * zoom + m_cam_y;
        float hw = 16.0f * zoom;
        float hh = 16.0f * zoom;
        glColor4f(r, g, b, a * 0.32f);
        glBegin(GL_QUADS);
        glVertex2f(cx - hw, cy - hh);
        glVertex2f(cx + hw, cy - hh);
        glVertex2f(cx + hw, cy + hh);
        glVertex2f(cx - hw, cy + hh);
        glEnd();
        glColor4f(r, g, b, a);
        glLineWidth(2.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(cx - hw, cy - hh);
        glVertex2f(cx + hw, cy - hh);
        glVertex2f(cx + hw, cy + hh);
        glVertex2f(cx - hw, cy + hh);
        glEnd();
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
    int width = 0;
    int height = 0;
    GetClientSize(&width, &height);
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (m_pending_add_type == PendingObjectAddType::TileSwap) {
        if (m_pending_tileswap_part != PendingTileSwapPart::MapSource) {
            draw_square(m_pending_add_swap.map.src_x, m_pending_add_swap.map.src_y, 0.25f, 0.75f, 1.0f, 0.95f);
        }
        if (m_pending_tileswap_part == PendingTileSwapPart::HeightmapSource ||
            m_pending_tileswap_part == PendingTileSwapPart::HeightmapDestination) {
            draw_square(m_pending_add_swap.map.dst_x, m_pending_add_swap.map.dst_y, 0.1f, 1.0f, 0.45f, 0.95f);
        }
        if (m_pending_tileswap_part == PendingTileSwapPart::HeightmapDestination) {
            draw_diamond(m_pending_add_swap.heightmap.src_x, m_pending_add_swap.heightmap.src_y, 1.0f, 0.75f, 0.2f, 0.95f);
        }
        bool current_is_heightmap = m_pending_tileswap_part == PendingTileSwapPart::HeightmapSource ||
            m_pending_tileswap_part == PendingTileSwapPart::HeightmapDestination;
        if (current_is_heightmap) {
            draw_diamond(m_pending_add_hover_x, m_pending_add_hover_y, 1.0f, 1.0f, 1.0f, 0.95f);
        } else {
            draw_square(m_pending_add_hover_x, m_pending_add_hover_y, 1.0f, 1.0f, 1.0f, 0.95f);
        }
    } else if (m_pending_add_type == PendingObjectAddType::Entity) {
        SpriteInstance ghost{};
        if (BuildPendingEntityPreviewInstance(ghost)) {
            float center_x = ghost.map_x + ghost.hitbox_offset;
            float center_y = ghost.map_y + ghost.hitbox_offset;
            PickPoint center = ProjectRoomGridPoint(
                center_x,
                center_y,
                ghost.map_z,
                room_left,
                room_top,
                ghost.z_extent);
            float cx = center.x * zoom + m_cam_x;
            float cy = center.y * zoom + m_cam_y;
            glColor4f(1.0f, 1.0f, 1.0f, 0.30f);
            glBegin(GL_QUADS);
            glVertex2f(cx, cy - 16.0f * zoom);
            glVertex2f(cx + 32.0f * zoom, cy);
            glVertex2f(cx, cy + 16.0f * zoom);
            glVertex2f(cx - 32.0f * zoom, cy);
            glEnd();
            glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
            glLineWidth(2.5f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(cx, cy - 16.0f * zoom);
            glVertex2f(cx + 32.0f * zoom, cy);
            glVertex2f(cx, cy + 16.0f * zoom);
            glVertex2f(cx - 32.0f * zoom, cy);
            glEnd();
            GLCanvasEntityEditor(*this).RenderEntityTooltipForInstance(ghost);
        }
    }
    glLineWidth(1.0f);
}


// Thin forwarding layer: input handlers stay on MyGLCanvas while edit rules
// live in dedicated editor/coordinator classes.
void MyGLCanvas::AddEntity() {
    GLCanvasEntityEditor(*this).BeginAddEntity();
}

void MyGLCanvas::CopySelectedEntity() {
    GLCanvasEntityEditor(*this).CopySelectedEntity();
}

void MyGLCanvas::PasteEntity() {
    if (!m_entity_clipboard_valid || m_room_entities.size() >= 15) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasEntityEditor(*this).PasteEntity();
    NotifyRoomDataChanged(true, false, false, false);
    NotifySelectionChanged();
}

void MyGLCanvas::CutSelectedEntity() {
    if (m_selected_entity_idx < 0 || m_selected_entity_idx >= static_cast<int>(m_instances.size())) {
        return;
    }

    CopySelectedEntity();
    DeleteSelectedObject();
}

void MyGLCanvas::DeleteSelectedObject() {
    const bool had_entity = m_selected_entity_idx >= 0;
    const bool had_warp = m_selected_warp_idx >= 0;
    const bool had_swap = m_selected_tileswap_region_idx >= 0;
    const bool had_door = m_selected_door_idx >= 0;
    if (!had_entity && !had_warp && !had_swap && !had_door) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasObjectCoordinator(*this).DeleteSelectedObject();
    NotifyRoomDataChanged(had_entity, had_warp, had_swap, had_door);
    NotifySelectionChanged();
}

void MyGLCanvas::ReorderSelectedObject(int delta) {
    const bool had_entity = m_selected_entity_idx >= 0;
    const bool had_warp = m_selected_warp_idx >= 0;
    const bool had_swap = m_selected_tileswap_region_idx >= 0;
    const bool had_door = m_selected_door_idx >= 0;
    if (!had_entity && !had_warp && !had_swap && !had_door) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasObjectCoordinator(*this).ReorderSelectedObject(delta);
    NotifyRoomDataChanged(had_entity, had_warp, had_swap, had_door);
    NotifySelectionChanged();
}

void MyGLCanvas::SelectNextObject(int direction) {
    GLCanvasObjectCoordinator(*this).SelectNextObject(direction);
    NotifySelectionChanged();
    UpdateStatusBar();
}

void MyGLCanvas::SelectNextTileSwapRegion(int direction) {
    GLCanvasObjectCoordinator(*this).SelectNextTileSwapRegion(direction);
    NotifySelectionChanged();
    UpdateStatusBar();
}

void MyGLCanvas::CycleSelectedEntityId(int delta) {
    bool selected_entity = m_selected_entity_idx >= 0 && m_selected_entity_idx < static_cast<int>(m_instances.size());
    if (selected_entity) {
        CaptureObjectUndoState();
    }
    GLCanvasEntityEditor(*this).CycleSelectedEntityId(delta);
    if (selected_entity) {
        NotifyRoomDataChanged(true, false, false, false);
    }
}

void MyGLCanvas::CycleSelectedEntityPalette() {
    bool selected_entity = m_selected_entity_idx >= 0 && m_selected_entity_idx < static_cast<int>(m_instances.size());
    if (selected_entity) {
        CaptureObjectUndoState();
    }
    GLCanvasEntityEditor(*this).CycleSelectedEntityPalette();
    if (selected_entity) {
        NotifyRoomDataChanged(true, false, false, false);
    }
}

void MyGLCanvas::SetSelectedEntityOrientation(Landstalker::Orientation orientation) {
    bool selected_entity = m_selected_entity_idx >= 0 && m_selected_entity_idx < static_cast<int>(m_instances.size());
    if (selected_entity) {
        CaptureObjectUndoState();
    }
    GLCanvasEntityEditor(*this).SetSelectedEntityOrientation(orientation);
    if (selected_entity) {
        NotifyRoomDataChanged(true, false, false, false);
    }
}

void MyGLCanvas::SetSelectedEntityToFloor() {
    if (m_selected_entity_idx < 0 || m_selected_entity_idx >= static_cast<int>(m_instances.size())) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasEntityEditor(*this).SetSelectedEntityToFloor();
    NotifyRoomDataChanged(true, false, false, false);
}

void MyGLCanvas::AddWarpHalf() {
    GLCanvasWarpEditor(*this).BeginAddWarpHalf();
}

std::pair<float, float> MyGLCanvas::FindNearestFreeWarpCell(float preferred_x, float preferred_y) const {
    return GLCanvasWarpEditor(const_cast<MyGLCanvas&>(*this)).FindNearestFreeWarpCell(preferred_x, preferred_y);
}

std::pair<int, int> MyGLCanvas::MouseHeightmapCell() const {
    if (m_last_mouse_pos.x < 0 || m_last_mouse_pos.y < 0) {
        return {
            std::clamp(m_mapRenderer.GetRoomWidth() / 2, 0, 63),
            std::clamp(m_mapRenderer.GetRoomHeight() / 2, 0, 63)
        };
    }

    int picked_x = -1;
    int picked_y = -1;
    if (const_cast<MyGLCanvas*>(this)->HeightmapCellAt(m_last_mouse_pos, picked_x, picked_y)) {
        return {
            std::clamp(picked_x, 0, 63),
            std::clamp(picked_y, 0, 63)
        };
    }

    PickPoint point = ScreenToHeightmapPoint(
        ScreenToWorldX(m_last_mouse_pos.x),
        ScreenToWorldY(m_last_mouse_pos.y),
        static_cast<float>(m_mapRenderer.GetRoomLeft()),
        static_cast<float>(m_mapRenderer.GetRoomTop()));
    return {
        std::clamp(static_cast<int>(std::floor(point.x)), 0, 63),
        std::clamp(static_cast<int>(std::floor(point.y)), 0, 63)
    };
}

void MyGLCanvas::ResizeSelectedWarp(float dx, float dy) {
    bool selected_warp = m_selected_warp_idx >= 0 && m_selected_warp_idx < static_cast<int>(m_warps.size());
    if (selected_warp) {
        CaptureObjectUndoState();
    }
    GLCanvasWarpEditor(*this).ResizeSelectedWarp(dx, dy);
    if (selected_warp) {
        NotifyRoomDataChanged(false, true, false, false);
    }
}

void MyGLCanvas::RotateSelectedWarp(float dx, float dy) {
    if (m_selected_warp_idx < 0 || m_selected_warp_idx >= static_cast<int>(m_warps.size())) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasWarpEditor(*this).RotateSelectedWarp(dx, dy);
    NotifyRoomDataChanged(false, true, false, false);
}

void MyGLCanvas::CycleSelectedWarpType(int delta) {
    bool selected_warp = m_selected_warp_idx >= 0 && m_selected_warp_idx < static_cast<int>(m_warps.size());
    if (selected_warp) {
        CaptureObjectUndoState();
    }
    GLCanvasWarpEditor(*this).CycleSelectedWarpType(delta);
    if (selected_warp) {
        NotifyRoomDataChanged(false, true, false, false);
    }
}

void MyGLCanvas::CycleSelectedDoorSize(int delta) {
    bool selected_door = m_selected_door_idx >= 0;
    if (selected_door) {
        CaptureObjectUndoState();
    }
    GLCanvasTileDoorEditor(*this).CycleSelectedDoorSize(delta);
    if (selected_door) {
        NotifyRoomDataChanged(false, false, false, true);
    }
}

void MyGLCanvas::AddDoor() {
    GLCanvasTileDoorEditor(*this).BeginAddDoor();
}

void MyGLCanvas::AddTileSwap() {
    GLCanvasTileDoorEditor(*this).BeginAddTileSwap();
}

void MyGLCanvas::CycleSelectedTileSwapShape(int delta) {
    if (m_selected_tileswap_region_idx < 0) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasTileDoorEditor(*this).CycleSelectedTileSwapShape(delta);
    NotifyRoomDataChanged(false, false, true, false);
}

void MyGLCanvas::CycleSelectedTileSwapId(int delta) {
    if (m_selected_tileswap_region_idx < 0) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasTileDoorEditor(*this).CycleSelectedTileSwapId(delta);
    NotifyRoomDataChanged(false, false, true, false);
}

void MyGLCanvas::ResizeSelectedTileSwapRegion(float requested_width, float requested_height) {
    if (m_selected_tileswap_region_idx < 0) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasTileDoorEditor(*this).ResizeSelectedTileSwapRegion(requested_width, requested_height);
    NotifyRoomDataChanged(false, false, true, false);
}

void MyGLCanvas::ToggleSelectedTileSwapPreview() {
    GLCanvasTileDoorEditor(*this).ToggleSelectedTileSwapPreview();
}

void MyGLCanvas::ToggleSelectedDoorPreview() {
    GLCanvasTileDoorEditor(*this).ToggleSelectedDoorPreview();
}

void MyGLCanvas::ClearTileSwapPreview() {
    GLCanvasTileDoorEditor(*this).ClearTileSwapPreview();
}

void MyGLCanvas::NudgeSelectedObject(float dx, float dy, float dz) {
    const bool had_entity = m_selected_entity_idx >= 0;
    const bool had_warp = m_selected_warp_idx >= 0;
    const bool had_swap = m_selected_tileswap_region_idx >= 0;
    const bool had_door = m_selected_door_idx >= 0;
    if (!had_entity && !had_warp && !had_swap && !had_door) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasObjectCoordinator(*this).NudgeSelectedObject(dx, dy, dz);
    NotifyRoomDataChanged(had_entity, had_warp, had_swap, had_door);
}

void MyGLCanvas::RenderWarps() {
    GLCanvasWarpEditor(*this).RenderWarps();
}

void MyGLCanvas::RenderEntityControls() {
    GLCanvasEntityEditor(*this).RenderEntityControls();
}

void MyGLCanvas::RenderSelectedEntityTooltip() {
    GLCanvasEntityEditor(*this).RenderSelectedEntityTooltip();
}

void MyGLCanvas::RenderSelectedWarpTooltip() {
    GLCanvasWarpEditor(*this).RenderSelectedWarpTooltip();
}

void MyGLCanvas::RenderSelectedDoorTooltip() {
    GLCanvasTileDoorEditor(*this).RenderSelectedDoorTooltip();
}

void MyGLCanvas::RenderSelectedTileSwapRegionTooltip() {
    GLCanvasTileDoorEditor(*this).RenderSelectedTileSwapRegionTooltip();
}

void MyGLCanvas::RenderRoomInfoTable(int width, int height) {
    m_room_info_overlay.Render(width, height);
}

float MyGLCanvas::FloorUnderRect(float min_x, float min_y, float max_x, float max_y) const {
    return GLCanvasHeightmapHitTest(*this).FloorUnderRect(min_x, min_y, max_x, max_y);
}

float MyGLCanvas::FloorUnderPoint(float x, float y) const {
    return GLCanvasHeightmapHitTest(*this).FloorUnderPoint(x, y);
}

bool MyGLCanvas::ShadowOccludedByHeightmap(float min_x, float min_y, float max_x, float max_y, float z) const {
    return GLCanvasHeightmapHitTest(*this).ShadowOccludedByHeightmap(min_x, min_y, max_x, max_y, z);
}

bool MyGLCanvas::EntityCollidesWithHeightmap(const SpriteInstance& inst) const {
    return GLCanvasHeightmapHitTest(*this).EntityCollidesWithHeightmap(inst);
}

float MyGLCanvas::FloorUnderHitbox(float center_x, float center_y, float half_base) const {
    return GLCanvasHeightmapHitTest(*this).FloorUnderHitbox(center_x, center_y, half_base);
}

int MyGLCanvas::FindInstanceIndex(uint32_t instance_id) const {
    for (std::size_t i = 0; i < m_instances.size(); ++i) {
        if (m_instances[i].instance_id == instance_id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int MyGLCanvas::FindWarpIndex(uint32_t instance_id) const {
    for (std::size_t i = 0; i < m_warps.size(); ++i) {
        if (m_warps[i].instance_id == instance_id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int MyGLCanvas::HitTestRoomInfoLink(const wxPoint& point) const {
    return m_room_info_overlay.HitTest(point);
}

int MyGLCanvas::HitTestEntity(const wxPoint& point) const {
    return GLCanvasEntityEditor(const_cast<MyGLCanvas&>(*this)).HitTestEntity(point);
}

int MyGLCanvas::HitTestEntityBody(const wxPoint& point) const {
    return GLCanvasEntityEditor(const_cast<MyGLCanvas&>(*this)).HitTestEntityBody(point);
}

int MyGLCanvas::HitTestEntityZControl(const wxPoint& point) const {
    return GLCanvasEntityEditor(const_cast<MyGLCanvas&>(*this)).HitTestEntityZControl(point);
}

int MyGLCanvas::HitTestWarpResizeControl(const wxPoint& point) const {
    return GLCanvasWarpEditor(const_cast<MyGLCanvas&>(*this)).HitTestWarpResizeControl(point);
}

int MyGLCanvas::HitTestWarp(const wxPoint& point) const {
    return GLCanvasWarpEditor(const_cast<MyGLCanvas&>(*this)).HitTestWarp(point);
}

int MyGLCanvas::HitTestTileSwapRegion(const wxPoint& point) const {
    return GLCanvasTileDoorEditor(const_cast<MyGLCanvas&>(*this)).HitTestTileSwapRegion(point);
}

int MyGLCanvas::HitTestTileSwapRegionResizeControl(const wxPoint& point) const {
    return GLCanvasTileDoorEditor(const_cast<MyGLCanvas&>(*this)).HitTestTileSwapRegionResizeControl(point);
}

int MyGLCanvas::HitTestDoor(const wxPoint& point) const {
    return GLCanvasTileDoorEditor(const_cast<MyGLCanvas&>(*this)).HitTestDoor(point);
}

void MyGLCanvas::RenderTileSwapOutlines() {
    GLCanvasTileDoorEditor(*this).RenderTileSwapOutlines();
}

void MyGLCanvas::RenderDoors() {
    GLCanvasTileDoorEditor(*this).RenderDoors();
}

