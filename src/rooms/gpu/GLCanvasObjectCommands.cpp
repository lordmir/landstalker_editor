#include "GLCanvas.h"
#include "GLCanvasEntityEditor.h"
#include "GLCanvasHeightmapHitTest.h"
#include "GLCanvasObjectCoordinator.h"
#include "GLCanvasObjectSupport.h"
#include "GLCanvasRoomInfoOverlay.h"
#include "GLCanvasDoorEditor.h"
#include "GLCanvasTileSwapEditor.h"
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

void GLCanvas::UpdateEntityDrag(const wxMouseEvent& evt) {
    GLCanvasEntityEditor(*this).UpdateEntityDrag(evt);
}

void GLCanvas::EndEntityDrag() {
    GLCanvasEntityEditor(*this).EndEntityDrag();
}

void GLCanvas::StartWarpDrag(int warp_idx, const wxMouseEvent& evt) {
    GLCanvasWarpEditor(*this).StartWarpDrag(warp_idx, evt);
}

void GLCanvas::StartWarpResizeDrag(int warp_idx, int axis, const wxMouseEvent& evt) {
    GLCanvasWarpEditor(*this).StartWarpResizeDrag(warp_idx, axis, evt);
}

void GLCanvas::UpdateWarpDrag(const wxMouseEvent& evt) {
    GLCanvasWarpEditor(*this).UpdateWarpDrag(evt);
}

void GLCanvas::EndWarpDrag() {
    GLCanvasWarpEditor(*this).EndWarpDrag();
}

void GLCanvas::StartDoorDrag(int door_idx, const wxMouseEvent& evt) {
    GLCanvasDoorEditor(*this).StartDoorDrag(door_idx, evt);
}

void GLCanvas::UpdateDoorDrag(const wxMouseEvent& evt) {
    GLCanvasDoorEditor(*this).UpdateDoorDrag(evt);
}

void GLCanvas::EndDoorDrag() {
    GLCanvasDoorEditor(*this).EndDoorDrag();
}

void GLCanvas::StartTileSwapRegionDrag(int region_idx, int resize_axis, const wxMouseEvent& evt) {
    GLCanvasTileSwapEditor(*this).StartTileSwapRegionDrag(region_idx, resize_axis, evt);
}

void GLCanvas::UpdateTileSwapRegionDrag(const wxMouseEvent& evt) {
    GLCanvasTileSwapEditor(*this).UpdateTileSwapRegionDrag(evt);
}

void GLCanvas::EndTileSwapRegionDrag() {
    GLCanvasTileSwapEditor(*this).EndTileSwapRegionDrag();
}

bool GLCanvas::HasPendingObjectAdd() const {
    return m_pending_add_type != PendingObjectAddType::None;
}

void GLCanvas::UpdatePendingObjectAddHover() {
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

bool GLCanvas::BuildPendingEntityPreviewInstance(SpriteInstance& inst) {
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

bool GLCanvas::BuildPendingWarpPreviewInstance(WarpInstance& inst) {
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

void GLCanvas::CancelPendingObjectAdd() {
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

void GLCanvas::CommitPendingObjectAdd() {
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
            GLCanvasDoorEditor(*this).AddDoor();
            NotifyRoomDataChanged(false, false, false, true);
            NotifySelectionChanged();
            CancelPendingObjectAdd();
            return;
        case PendingObjectAddType::TileSwap:
            GLCanvasTileSwapEditor(*this).CommitPendingTileSwapStep();
            return;
        case PendingObjectAddType::None:
            return;
    }
}

void GLCanvas::RenderPendingObjectAddOverlay() {
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


// Thin forwarding layer: input handlers stay on GLCanvas while edit rules
// live in dedicated editor/coordinator classes.
void GLCanvas::AddEntity() {
    GLCanvasEntityEditor(*this).BeginAddEntity();
}

void GLCanvas::CopySelectedEntity() {
    GLCanvasEntityEditor(*this).CopySelectedEntity();
}

void GLCanvas::PasteEntity() {
    if (!m_entity_clipboard_valid || m_room_entities.size() >= 15) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasEntityEditor(*this).PasteEntity();
    NotifyRoomDataChanged(true, false, false, false);
    NotifySelectionChanged();
}

void GLCanvas::CutSelectedEntity() {
    if (m_selected_entity_idx < 0 || m_selected_entity_idx >= static_cast<int>(m_instances.size())) {
        return;
    }

    CopySelectedEntity();
    DeleteSelectedObject();
}

void GLCanvas::DeleteSelectedObject() {
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

void GLCanvas::ReorderSelectedObject(int delta) {
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

void GLCanvas::SelectNextObject(int direction) {
    GLCanvasObjectCoordinator(*this).SelectNextObject(direction);
    NotifySelectionChanged();
    UpdateStatusBar();
}

void GLCanvas::CycleSelectedEntityId(int delta) {
    bool selected_entity = m_selected_entity_idx >= 0 && m_selected_entity_idx < static_cast<int>(m_instances.size());
    if (selected_entity) {
        CaptureObjectUndoState();
    }
    GLCanvasEntityEditor(*this).CycleSelectedEntityId(delta);
    if (selected_entity) {
        NotifyRoomDataChanged(true, false, false, false);
    }
}

void GLCanvas::CycleSelectedEntityPalette() {
    bool selected_entity = m_selected_entity_idx >= 0 && m_selected_entity_idx < static_cast<int>(m_instances.size());
    if (selected_entity) {
        CaptureObjectUndoState();
    }
    GLCanvasEntityEditor(*this).CycleSelectedEntityPalette();
    if (selected_entity) {
        NotifyRoomDataChanged(true, false, false, false);
    }
}

void GLCanvas::SetSelectedEntityOrientation(Landstalker::Orientation orientation) {
    bool selected_entity = m_selected_entity_idx >= 0 && m_selected_entity_idx < static_cast<int>(m_instances.size());
    if (selected_entity) {
        CaptureObjectUndoState();
    }
    GLCanvasEntityEditor(*this).SetSelectedEntityOrientation(orientation);
    if (selected_entity) {
        NotifyRoomDataChanged(true, false, false, false);
    }
}

void GLCanvas::SetSelectedEntityToFloor() {
    if (m_selected_entity_idx < 0 || m_selected_entity_idx >= static_cast<int>(m_instances.size())) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasEntityEditor(*this).SetSelectedEntityToFloor();
    NotifyRoomDataChanged(true, false, false, false);
}

void GLCanvas::AddWarpHalf() {
    GLCanvasWarpEditor(*this).BeginAddWarpHalf();
}

std::pair<float, float> GLCanvas::FindNearestFreeWarpCell(float preferred_x, float preferred_y) const {
    return GLCanvasWarpEditor(const_cast<GLCanvas&>(*this)).FindNearestFreeWarpCell(preferred_x, preferred_y);
}

std::pair<int, int> GLCanvas::MouseHeightmapCell() const {
    if (m_last_mouse_pos.x < 0 || m_last_mouse_pos.y < 0) {
        return {
            std::clamp(m_mapRenderer.GetRoomWidth() / 2, 0, 63),
            std::clamp(m_mapRenderer.GetRoomHeight() / 2, 0, 63)
        };
    }

    int picked_x = -1;
    int picked_y = -1;
    if (const_cast<GLCanvas*>(this)->HeightmapCellAt(m_last_mouse_pos, picked_x, picked_y)) {
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

void GLCanvas::ResizeSelectedWarp(float dx, float dy) {
    bool selected_warp = m_selected_warp_idx >= 0 && m_selected_warp_idx < static_cast<int>(m_warps.size());
    if (selected_warp) {
        CaptureObjectUndoState();
    }
    GLCanvasWarpEditor(*this).ResizeSelectedWarp(dx, dy);
    if (selected_warp) {
        NotifyRoomDataChanged(false, true, false, false);
    }
}

void GLCanvas::RotateSelectedWarp(float dx, float dy) {
    if (m_selected_warp_idx < 0 || m_selected_warp_idx >= static_cast<int>(m_warps.size())) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasWarpEditor(*this).RotateSelectedWarp(dx, dy);
    NotifyRoomDataChanged(false, true, false, false);
}

void GLCanvas::CycleSelectedWarpType(int delta) {
    bool selected_warp = m_selected_warp_idx >= 0 && m_selected_warp_idx < static_cast<int>(m_warps.size());
    if (selected_warp) {
        CaptureObjectUndoState();
    }
    GLCanvasWarpEditor(*this).CycleSelectedWarpType(delta);
    if (selected_warp) {
        NotifyRoomDataChanged(false, true, false, false);
    }
}

void GLCanvas::CycleSelectedDoorSize(int delta) {
    bool selected_door = m_selected_door_idx >= 0;
    if (selected_door) {
        CaptureObjectUndoState();
    }
    GLCanvasDoorEditor(*this).CycleSelectedDoorSize(delta);
    if (selected_door) {
        NotifyRoomDataChanged(false, false, false, true);
    }
}

void GLCanvas::AddDoor() {
    GLCanvasDoorEditor(*this).BeginAddDoor();
}

void GLCanvas::AddTileSwap() {
    GLCanvasTileSwapEditor(*this).BeginAddTileSwap();
}

void GLCanvas::CycleSelectedTileSwapShape(int delta) {
    if (m_selected_tileswap_region_idx < 0) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasTileSwapEditor(*this).CycleSelectedTileSwapShape(delta);
    NotifyRoomDataChanged(false, false, true, false);
}

void GLCanvas::CycleSelectedTileSwapId(int delta) {
    if (m_selected_tileswap_region_idx < 0) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasTileSwapEditor(*this).CycleSelectedTileSwapId(delta);
    NotifyRoomDataChanged(false, false, true, false);
}

void GLCanvas::ResizeSelectedTileSwapRegion(float requested_width, float requested_height) {
    if (m_selected_tileswap_region_idx < 0) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasTileSwapEditor(*this).ResizeSelectedTileSwapRegion(requested_width, requested_height);
    NotifyRoomDataChanged(false, false, true, false);
}

void GLCanvas::ToggleSelectedTileSwapPreview() {
    GLCanvasTileSwapEditor(*this).ToggleSelectedTileSwapPreview();
}

void GLCanvas::ToggleSelectedDoorPreview() {
    GLCanvasDoorEditor(*this).ToggleSelectedDoorPreview();
}

void GLCanvas::ClearTileSwapPreview() {
    // Door and tile-swap previews share the same preview map/state, so a single
    // canvas-owned clear serves both editors.
    if (!m_tileswap_preview_active && !m_door_preview_active && !m_tileswap_preview_map) {
        return;
    }
    m_tileswap_preview_active = false;
    m_tileswap_preview_swap_index = -1;
    m_door_preview_active = false;
    m_door_preview_idx = -1;
    m_tileswap_preview_map.reset();
    m_heightmapRenderer.ClearPreviewMap();
    if (m_initialized) {
        m_mapRenderer.LoadRoom(m_current_room);
        RefreshObjectPlacementsFromHeightmap();
    }
}

void GLCanvas::NudgeSelectedObject(float dx, float dy, float dz) {
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

void GLCanvas::RenderWarps() {
    GLCanvasWarpEditor(*this).RenderWarps();
}

void GLCanvas::RenderEntityControls() {
    GLCanvasEntityEditor(*this).RenderEntityControls();
}

void GLCanvas::RenderSelectedEntityTooltip() {
    GLCanvasEntityEditor(*this).RenderSelectedEntityTooltip();
}

void GLCanvas::RenderSelectedWarpTooltip() {
    GLCanvasWarpEditor(*this).RenderSelectedWarpTooltip();
}

void GLCanvas::RenderSelectedDoorTooltip() {
    GLCanvasDoorEditor(*this).RenderSelectedDoorTooltip();
}

void GLCanvas::RenderSelectedTileSwapRegionTooltip() {
    GLCanvasTileSwapEditor(*this).RenderSelectedTileSwapRegionTooltip();
}

void GLCanvas::RenderRoomInfoTable(int width, int height) {
    m_room_info_overlay.Render(width, height);
}

float GLCanvas::FloorUnderRect(float min_x, float min_y, float max_x, float max_y) const {
    return GLCanvasHeightmapHitTest(*this).FloorUnderRect(min_x, min_y, max_x, max_y);
}

float GLCanvas::FloorUnderPoint(float x, float y) const {
    return GLCanvasHeightmapHitTest(*this).FloorUnderPoint(x, y);
}

bool GLCanvas::ShadowOccludedByHeightmap(float min_x, float min_y, float max_x, float max_y, float z) const {
    return GLCanvasHeightmapHitTest(*this).ShadowOccludedByHeightmap(min_x, min_y, max_x, max_y, z);
}

bool GLCanvas::EntityCollidesWithHeightmap(const SpriteInstance& inst) const {
    return GLCanvasHeightmapHitTest(*this).EntityCollidesWithHeightmap(inst);
}

float GLCanvas::FloorUnderHitbox(float center_x, float center_y, float half_base) const {
    return GLCanvasHeightmapHitTest(*this).FloorUnderHitbox(center_x, center_y, half_base);
}

int GLCanvas::FindInstanceIndex(uint32_t instance_id) const {
    for (std::size_t i = 0; i < m_instances.size(); ++i) {
        if (m_instances[i].instance_id == instance_id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int GLCanvas::FindWarpIndex(uint32_t instance_id) const {
    for (std::size_t i = 0; i < m_warps.size(); ++i) {
        if (m_warps[i].instance_id == instance_id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int GLCanvas::HitTestRoomInfoLink(const wxPoint& point) const {
    return m_room_info_overlay.HitTest(point);
}

int GLCanvas::HitTestEntity(const wxPoint& point) const {
    return GLCanvasEntityEditor(const_cast<GLCanvas&>(*this)).HitTestEntity(point);
}

int GLCanvas::HitTestEntityBody(const wxPoint& point) const {
    return GLCanvasEntityEditor(const_cast<GLCanvas&>(*this)).HitTestEntityBody(point);
}

int GLCanvas::HitTestEntityZControl(const wxPoint& point) const {
    return GLCanvasEntityEditor(const_cast<GLCanvas&>(*this)).HitTestEntityZControl(point);
}

int GLCanvas::HitTestWarpResizeControl(const wxPoint& point) const {
    return GLCanvasWarpEditor(const_cast<GLCanvas&>(*this)).HitTestWarpResizeControl(point);
}

int GLCanvas::HitTestWarp(const wxPoint& point) const {
    return GLCanvasWarpEditor(const_cast<GLCanvas&>(*this)).HitTestWarp(point);
}

int GLCanvas::HitTestTileSwapRegion(const wxPoint& point) const {
    return GLCanvasTileSwapEditor(const_cast<GLCanvas&>(*this)).HitTestTileSwapRegion(point);
}

int GLCanvas::HitTestTileSwapRegionResizeControl(const wxPoint& point) const {
    return GLCanvasTileSwapEditor(const_cast<GLCanvas&>(*this)).HitTestTileSwapRegionResizeControl(point);
}

int GLCanvas::HitTestDoor(const wxPoint& point) const {
    return GLCanvasDoorEditor(const_cast<GLCanvas&>(*this)).HitTestDoor(point);
}

void GLCanvas::RenderTileSwapOutlines() {
    GLCanvasTileSwapEditor(*this).RenderTileSwapOutlines();
}

void GLCanvas::RenderDoors() {
    GLCanvasDoorEditor(*this).RenderDoors();
}

