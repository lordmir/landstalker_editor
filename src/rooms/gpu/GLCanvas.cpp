#include "GLCanvas.h"
#include "GLLoader.h"
#include "GLCanvasEntityEditor.h"
#include "GLCanvasHeightmapHitTest.h"
#include "GLCanvasHeightmapMode.h"
#include "GLCanvasLayerEditMode.h"
#include "GLCanvasObjectCoordinator.h"
#include "GLCanvasObjectSupport.h"
#include "GLCanvasRoomMode.h"
#include "GLCanvasTileDoorEditor.h"
#include "GLCanvasWarpEditor.h"
#include "RoomProjection.h"
#include <rooms/EntityControlFrame.h>
#include <rooms/RoomViewerFrame.h>
#include <rooms/TileSwapControlFrame.h>
#include <rooms/WarpControlFrame.h>
#include <main/EditorFrame.h>
#include <wx/dcclient.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <utility>
#include <wx/log.h>

using namespace Landstalker;

namespace {
// File-local rendering/math helpers used only by GLCanvas.
int GLCanvasAttributes[] = {
    WX_GL_RGBA,
    WX_GL_DOUBLEBUFFER,
    WX_GL_STENCIL_SIZE, 8,
    0
};

using GLCanvasObjectSupport::HitboxBaseToBlocks;
using GLCanvasObjectSupport::HitboxDrawOffset;
using GLCanvasObjectSupport::HitboxHeightToBlocks;

struct EntityBounds {
    float min_x;
    float min_y;
    float max_x;
    float max_y;
    float min_z;
    float max_z;
    float back_depth;
    float front_depth;
};

EntityBounds GetEntityBounds(const SpriteInstance& inst) {
    float center_x = inst.map_x + inst.hitbox_offset;
    float center_y = inst.map_y + inst.hitbox_offset;
    float half_base = inst.hitbox_base * 0.5f;
    float min_x = center_x - half_base;
    float min_y = center_y - half_base;
    float max_x = center_x + half_base;
    float max_y = center_y + half_base;
    return {
        min_x,
        min_y,
        max_x,
        max_y,
        inst.map_z,
        inst.map_z + std::max(inst.hitbox_height, 0.125f),
        min_x + min_y,
        max_x + max_y
    };
}

float OpacityForIndex(int idx) {
    static constexpr float opacities[] = {1.0f, 0.5f, 0.0f};
    return opacities[idx % 3];
}

uint8_t OpacityByteForIndex(int idx) {
    return static_cast<uint8_t>(std::lround(OpacityForIndex(idx) * 255.0f));
}

constexpr std::array<float, 5> kZoomSteps = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f};
constexpr float kHeightmapEditorMaxZExtent = 32.0f;
constexpr float kHeightmapEditorZScaleStep = 0.25f;
constexpr long kTargetFrameMs = 1000 / 60;

float WheelSteps(const wxMouseEvent& evt) {
    int delta = evt.GetWheelDelta();
    if (delta == 0) {
        return evt.GetWheelRotation() > 0 ? 1.0f : -1.0f;
    }
    return static_cast<float>(evt.GetWheelRotation()) / static_cast<float>(delta);
}

using PickPoint = RoomProjection::PickPoint;
using GLCanvasObjectSupport::BuildDoorGeometries;
using GLCanvasObjectSupport::BuildTileSwapRegionGeometries;
using GLCanvasObjectSupport::MakeWarpInstance;
using GLCanvasObjectSupport::SortEntitiesGeometrically;
using RoomProjection::ProjectEntityGridPoint;
using RoomProjection::ProjectWarpGridPoint;
using RoomProjection::ScreenToHeightmapPoint;
using RoomProjection::ScreenToMapPoint;

std::set<uint32_t> FindCollidedEntities(const std::vector<SpriteInstance>& instances) {
    constexpr float epsilon = 0.001f;
    std::set<uint32_t> collided;
    for (std::size_t i = 0; i < instances.size(); ++i) {
        EntityBounds lhs = GetEntityBounds(instances[i]);
        for (std::size_t j = i + 1; j < instances.size(); ++j) {
            EntityBounds rhs = GetEntityBounds(instances[j]);
            bool overlap =
                lhs.min_x < rhs.max_x - epsilon &&
                lhs.max_x > rhs.min_x + epsilon &&
                lhs.min_y < rhs.max_y - epsilon &&
                lhs.max_y > rhs.min_y + epsilon &&
                lhs.min_z < rhs.max_z - epsilon &&
                lhs.max_z > rhs.min_z + epsilon;
            if (overlap) {
                collided.insert(instances[i].instance_id);
                collided.insert(instances[j].instance_id);
            }
        }
    }
    return collided;
}

}  // namespace

wxDEFINE_EVENT(EVT_GPU_EDITOR_MODE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_GPU_LAYER_OPACITY_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_GPU_LAYER_BLOCK_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_GPU_HEIGHTMAP_TARGET_CHANGE, wxCommandEvent);

wxBEGIN_EVENT_TABLE(MyGLCanvas, wxGLCanvas)
    EVT_PAINT(MyGLCanvas::OnPaint)
    EVT_KEY_DOWN(MyGLCanvas::OnKeyDown)
    EVT_MOTION(MyGLCanvas::OnMouseMove)
    EVT_LEFT_DOWN(MyGLCanvas::OnLeftDown)
    EVT_LEFT_DCLICK(MyGLCanvas::OnLeftDClick)
    EVT_LEFT_UP(MyGLCanvas::OnLeftUp)
    EVT_MIDDLE_DOWN(MyGLCanvas::OnMiddleDown)
    EVT_MIDDLE_UP(MyGLCanvas::OnMiddleUp)
    EVT_RIGHT_DOWN(MyGLCanvas::OnRightDown)
    EVT_RIGHT_UP(MyGLCanvas::OnRightUp)
    EVT_LEAVE_WINDOW(MyGLCanvas::OnMouseLeave)
    EVT_MOUSEWHEEL(MyGLCanvas::OnMouseWheel)
    EVT_SIZE(MyGLCanvas::OnSize)
    EVT_IDLE(MyGLCanvas::OnIdle)
wxEND_EVENT_TABLE()

MyGLCanvas::MyGLCanvas(wxWindow* parent, std::shared_ptr<GameData> gd)
    : wxGLCanvas(parent, wxID_ANY, GLCanvasAttributes, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS),
      m_gd(gd),
      m_mapRenderer(gd),
      m_heightmapRenderer(gd),
      m_spriteRenderer(gd),
      m_room_info_overlay(*this)
{
    m_fps_stopwatch.Start();
    m_anim_stopwatch.Start();
    m_last_anim_ms = m_anim_stopwatch.Time();
    m_last_frame_ms = m_last_anim_ms;
}

MyGLCanvas::~MyGLCanvas() {
    if (m_gd) {
        PersistCurrentRoomEdits();
    }
    delete m_context;
}

void MyGLCanvas::SetRoomNum(uint16_t roomnum) {
    if (m_initialized && m_current_room == roomnum) {
        SetFocus();
        Refresh();
        return;
    }
    if (!m_initialized) {
        m_current_room = roomnum;
        Refresh();
        return;
    }
    LoadRoom(roomnum);
    SetFocus();
    Refresh();
}

void MyGLCanvas::SetZoom(double zoom) {
    int best_idx = 0;
    double best_distance = std::abs(static_cast<double>(kZoomSteps[0]) - zoom);
    for (std::size_t i = 1; i < kZoomSteps.size(); ++i) {
        const double distance = std::abs(static_cast<double>(kZoomSteps[i]) - zoom);
        if (distance < best_distance) {
            best_idx = static_cast<int>(i);
            best_distance = distance;
        }
    }

    int old_idx = std::clamp(m_zoom_step_idx, 0, static_cast<int>(kZoomSteps.size()) - 1);
    if (best_idx == old_idx) {
        return;
    }

    int width = 0;
    int height = 0;
    GetClientSize(&width, &height);
    ChangeZoomStep(best_idx - old_idx, static_cast<float>(width) * 0.5f, static_cast<float>(height) * 0.5f);
    Refresh();
}

void MyGLCanvas::SetHeightmapZScale(float scale) {
    float clamped = std::clamp(scale, 0.0f, 1.0f);
    clamped = std::round(clamped / kHeightmapEditorZScaleStep) * kHeightmapEditorZScaleStep;
    clamped = std::clamp(clamped, 0.0f, 1.0f);
    if (std::abs(m_heightmap_z_scale - clamped) < 0.001f) {
        return;
    }

    m_heightmap_z_scale = clamped;
    if (IsHeightmapEditMode()) {
        ApplyHeightmapEditZExtent();
        RefreshObjectPlacementsFromHeightmap();
        UpdateStatusBar();
        Refresh();
    }
}

void MyGLCanvas::AdjustHeightmapZScale(int delta) {
    SetHeightmapZScale(m_heightmap_z_scale + static_cast<float>(delta) * kHeightmapEditorZScaleStep);
}

void MyGLCanvas::SetAlpha(bool visible) {
    if (m_alpha == visible) {
        return;
    }
    m_alpha = visible;
    Refresh();
}

void MyGLCanvas::SetBackgroundOpacity(float opacity) {
    m_mapRenderer.SetBackgroundOpacity(std::clamp(opacity, 0.0f, 1.0f));
    Refresh();
}

void MyGLCanvas::SetForegroundOpacity(float opacity) {
    m_mapRenderer.SetForegroundOpacity(std::clamp(opacity, 0.0f, 1.0f));
    Refresh();
}

void MyGLCanvas::SetSpriteOpacity(float opacity) {
    m_spriteRenderer.SetOpacity(std::clamp(opacity, 0.0f, 1.0f));
    Refresh();
}

uint8_t MyGLCanvas::GetBackgroundOpacityByte() const {
    return OpacityByteForIndex(m_bg_opacity_idx);
}

uint8_t MyGLCanvas::GetForegroundOpacityByte() const {
    return OpacityByteForIndex(m_fg_opacity_idx);
}

uint8_t MyGLCanvas::GetSpriteOpacityByte() const {
    return OpacityByteForIndex(m_sprite_opacity_idx);
}

void MyGLCanvas::SetHeightmapVisible(bool visible) {
    if (m_show_heightmap == visible) {
        return;
    }
    m_show_heightmap = visible;
    Refresh();
}

void MyGLCanvas::SetEntitiesVisible(bool visible) {
    if (m_show_entities == visible) {
        return;
    }
    m_show_entities = visible;
    Refresh();
}

void MyGLCanvas::SetEntitiesHitboxVisible(bool visible) {
    if (m_show_hitboxes == visible) {
        return;
    }
    m_show_hitboxes = visible;
    Refresh();
}

void MyGLCanvas::SetWarpsVisible(bool visible) {
    if (m_show_warps == visible) {
        return;
    }
    m_show_warps = visible;
    Refresh();
}

void MyGLCanvas::SetTileSwapsVisible(bool visible) {
    if (m_show_tile_swaps == visible) {
        return;
    }
    m_show_tile_swaps = visible;
    Refresh();
}

void MyGLCanvas::SetLayerPriorityHighlight(bool enabled) {
    if (m_layer_priority_highlight == enabled) {
        return;
    }
    m_layer_priority_highlight = enabled;
    Refresh();

    wxWindow* target = EventTarget();
    if (target) {
        wxCommandEvent evt(EVT_GPU_EDITOR_MODE_CHANGE);
        evt.SetInt(static_cast<int>(m_editor_mode));
        evt.SetClientData(this);
        wxPostEvent(target, evt);
    }
}

void MyGLCanvas::ToggleLayerPriorityHighlight() {
    SetLayerPriorityHighlight(!m_layer_priority_highlight);
}

void MyGLCanvas::ResetLayerEditState() {
    m_layer_dragging_select = false;
    m_layer_selection_add = false;
    m_layer_selection_subtract = false;
    m_layer_selection_parallelogram = false;
    m_layer_selection_drag_base.clear();
    m_layer_dragging_selection_move = false;
    m_layer_selection_move_anchor_x = -1;
    m_layer_selection_move_anchor_y = -1;
    m_layer_selection_move_delta_x = 0;
    m_layer_selection_move_delta_y = 0;
    m_layer_selection_move_values.clear();
    m_layer_dragging_draw = false;
    m_layer_dragging_line = false;
    m_layer_draw_dirty = false;
    m_layer_last_draw_x = -1;
    m_layer_last_draw_y = -1;
    m_layer_line_start_x = -1;
    m_layer_line_start_y = -1;
    m_layer_line_end_x = -1;
    m_layer_line_end_y = -1;
    m_layer_line_preview_cells.clear();
}

void MyGLCanvas::ResetHeightmapEditState() {
    m_heightmap_dragging_select = false;
    m_heightmap_dragging_draw = false;
    m_heightmap_dragging_line = false;
    m_heightmap_dragging_selection_move = false;
    m_heightmap_draw_dirty = false;
    m_heightmap_selection_add = false;
    m_heightmap_selection_subtract = false;
    m_heightmap_last_draw_x = -1;
    m_heightmap_last_draw_y = -1;
    m_heightmap_line_start_x = -1;
    m_heightmap_line_start_y = -1;
    m_heightmap_line_end_x = -1;
    m_heightmap_line_end_y = -1;
    m_heightmap_selection_move_anchor_x = -1;
    m_heightmap_selection_move_anchor_y = -1;
    m_heightmap_selection_move_delta_x = 0;
    m_heightmap_selection_move_delta_y = 0;
    m_heightmap_line_preview_cells.clear();
    m_heightmap_selection_move_values.clear();
    m_heightmap_selection_drag_base.clear();
}

void MyGLCanvas::SetDrawingTool(DrawingTool tool) {
    if (m_drawing_tool == tool) {
        return;
    }
    if (m_layer_dragging_draw) {
        CommitLayerDrawStroke();
    }
    m_drawing_tool = tool;
    ResetHeightmapEditState();
    ResetLayerEditState();
    if (HasCapture()) {
        ReleaseMouse();
    }
    Refresh();
    wxWindow* target = EventTarget();
    if (target) {
        wxCommandEvent evt(EVT_GPU_EDITOR_MODE_CHANGE);
        evt.SetInt(static_cast<int>(m_editor_mode));
        evt.SetClientData(this);
        wxPostEvent(target, evt);
    }
}

void MyGLCanvas::LoadRoom(uint16_t roomnum) {
    LoadRoomFromGameData(roomnum, true, true);
}

void MyGLCanvas::NavigateToRoom(uint16_t roomnum) {
    if (!m_gd) {
        return;
    }
    LoadRoom(roomnum);
    SetFocus();
    NotifyRoomNavigationChanged();
}

void MyGLCanvas::ReloadCurrentRoomFromGameData() {
    LoadRoomFromGameData(m_current_room, false, false);
    Refresh();
}

void MyGLCanvas::CommitPendingEdits() {
    if (m_gd && m_initialized) {
        PersistCurrentRoomEdits();
    }
}

void MyGLCanvas::ClearObjectSelection() {
    m_hovered_entity_idx = -1;
    m_selected_entity_idx = -1;
    m_hovered_warp_idx = -1;
    m_selected_warp_idx = -1;
    m_hovered_tileswap_region_idx = -1;
    m_selected_tileswap_region_idx = -1;
    m_hovered_door_idx = -1;
    m_selected_door_idx = -1;
}

void MyGLCanvas::SelectEntityByIndex(int selection) {
    ClearObjectSelection();
    int idx = FindInstanceIndex(static_cast<uint32_t>(selection));
    if (idx >= 0) {
        m_selected_entity_idx = idx;
        FocusCameraOnSelectedObjectIfNeeded();
    }
    UpdateStatusBar();
    Refresh();
}

void MyGLCanvas::SelectWarpByIndex(int selection) {
    ClearObjectSelection();
    if (selection > 0) {
        for (std::size_t i = 0; i < m_warps.size(); ++i) {
            if (m_warps[i].warp_key == static_cast<uint32_t>(selection)) {
                m_selected_warp_idx = static_cast<int>(i);
                break;
            }
        }
        FocusCameraOnSelectedObjectIfNeeded();
    }
    UpdateStatusBar();
    Refresh();
}

void MyGLCanvas::SelectTileSwapByIndex(int selection) {
    ClearObjectSelection();
    int swap_idx = selection - 1;
    if (swap_idx >= 0) {
        auto regions = BuildTileSwapRegionGeometries(m_gd, m_current_room, m_mapRenderer, m_heightmapRenderer.GetZExtent());
        for (const auto& region : regions) {
            if (region.swap_index == swap_idx) {
                m_selected_tileswap_region_idx = region.flat_index;
                break;
            }
        }
        FocusCameraOnSelectedObjectIfNeeded();
    }
    UpdateStatusBar();
    Refresh();
}

void MyGLCanvas::SelectDoorByIndex(int selection) {
    ClearObjectSelection();
    int door_idx = selection - 1;
    if (door_idx >= 0) {
        auto doors = BuildDoorGeometries(
            m_gd,
            m_current_room,
            m_mapRenderer,
            m_heightmapRenderer.GetZExtent(),
            m_tileswap_preview_map);
        for (const auto& door : doors) {
            if (door.index == door_idx) {
                m_selected_door_idx = door.index;
                break;
            }
        }
        FocusCameraOnSelectedObjectIfNeeded();
    }
    UpdateStatusBar();
    Refresh();
}

int MyGLCanvas::SelectedEntityListIndex() const {
    if (m_selected_entity_idx >= 0 && m_selected_entity_idx < static_cast<int>(m_instances.size())) {
        return static_cast<int>(m_instances[static_cast<std::size_t>(m_selected_entity_idx)].instance_id);
    }
    return -1;
}

int MyGLCanvas::SelectedWarpListIndex() const {
    if (m_selected_warp_idx >= 0 && m_selected_warp_idx < static_cast<int>(m_warps.size())) {
        return static_cast<int>(m_warps[static_cast<std::size_t>(m_selected_warp_idx)].warp_key);
    }
    return -1;
}

int MyGLCanvas::SelectedTileSwapListIndex() const {
    if (m_selected_tileswap_region_idx >= 0) {
        auto regions = BuildTileSwapRegionGeometries(m_gd, m_current_room, m_mapRenderer, m_heightmapRenderer.GetZExtent());
        if (m_selected_tileswap_region_idx < static_cast<int>(regions.size())) {
            return regions[static_cast<std::size_t>(m_selected_tileswap_region_idx)].swap_index + 1;
        }
    }
    return -1;
}

int MyGLCanvas::SelectedDoorListIndex() const {
    return m_selected_door_idx >= 0 ? m_selected_door_idx + 1 : -1;
}

bool MyGLCanvas::SelectObjectAt(const wxPoint& point) {
    int entity_idx = HitTestEntityZControl(point);
    if (entity_idx < 0) {
        entity_idx = HitTestEntityBody(point);
    }
    if (entity_idx < 0) {
        entity_idx = HitTestEntity(point);
    }
    if (entity_idx >= 0) {
        ClearObjectSelection();
        m_selected_entity_idx = entity_idx;
        return true;
    }

    int warp_idx = HitTestWarp(point);
    if (warp_idx >= 0) {
        ClearObjectSelection();
        m_selected_warp_idx = warp_idx;
        return true;
    }

    int tileswap_idx = HitTestTileSwapRegion(point);
    if (tileswap_idx >= 0) {
        ClearObjectSelection();
        m_selected_tileswap_region_idx = tileswap_idx;
        m_hovered_tileswap_region_idx = tileswap_idx;
        return true;
    }

    int door_idx = HitTestDoor(point);
    if (door_idx >= 0) {
        ClearObjectSelection();
        m_selected_door_idx = door_idx;
        m_hovered_door_idx = door_idx;
        return true;
    }

    return false;
}

wxWindow* MyGLCanvas::EventTarget() const {
    for (wxWindow* window = GetParent(); window != nullptr; window = window->GetParent()) {
        if (dynamic_cast<RoomViewerFrame*>(window) != nullptr) {
            return window;
        }
    }
    return GetParent();
}

bool MyGLCanvas::OpenSelectedObjectProperties() {
    wxWindow* target = EventTarget();
    if (!target) {
        return false;
    }

    auto post_open = [&](const wxEventType& event_type, int selection) {
        wxCommandEvent evt(event_type);
        evt.SetInt(selection);
        evt.SetExtraLong(selection);
        evt.SetClientData(this);
        wxPostEvent(target, evt);
    };

    int selection = SelectedEntityListIndex();
    if (selection > 0) {
        CommitPendingEdits();
        post_open(EVT_ENTITY_OPEN_PROPERTIES, selection);
        return true;
    }

    selection = SelectedWarpListIndex();
    if (selection > 0) {
        CommitPendingEdits();
        post_open(EVT_WARP_OPEN_PROPERTIES, selection);
        return true;
    }

    selection = SelectedTileSwapListIndex();
    if (selection > 0) {
        CommitPendingEdits();
        post_open(EVT_TILESWAP_OPEN_PROPERTIES, selection);
        return true;
    }

    selection = SelectedDoorListIndex();
    if (selection > 0) {
        CommitPendingEdits();
        post_open(EVT_DOOR_OPEN_PROPERTIES, selection);
        return true;
    }

    return false;
}

void MyGLCanvas::CancelActiveDrag() {
    m_dragging_entity = false;
    m_dragging_warp = false;
    m_dragging_door = false;
    m_dragging_tileswap_region = false;
    m_dragging_pan = false;
    ResetHeightmapEditState();
    ResetLayerEditState();
    if (HasCapture()) {
        ReleaseMouse();
    }
    SetCursor(wxCursor(wxCURSOR_ARROW));
}

void MyGLCanvas::NotifySelectionChanged() {
    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    auto post_selection = [&](const wxEventType& event_type, int selection) {
        wxCommandEvent evt(event_type);
        evt.SetInt(selection);
        evt.SetExtraLong(selection);
        evt.SetClientData(this);
        wxPostEvent(target, evt);
    };

    int selection = SelectedEntityListIndex();
    if (selection > 0) {
        post_selection(EVT_ENTITY_SELECT, selection);
        return;
    }

    selection = SelectedWarpListIndex();
    if (selection > 0) {
        post_selection(EVT_WARP_SELECT, selection);
        return;
    }

    selection = SelectedTileSwapListIndex();
    if (selection > 0) {
        post_selection(EVT_TILESWAP_SELECT, selection);
        return;
    }

    selection = SelectedDoorListIndex();
    if (selection > 0) {
        post_selection(EVT_DOOR_SELECT, selection);
    }
}

void MyGLCanvas::NotifyRoomNavigationChanged() {
    if (!m_gd) {
        return;
    }
    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    auto room = m_gd->GetRoomData()->GetRoom(m_current_room);
    if (!room) {
        return;
    }

    wxCommandEvent evt(EVT_GO_TO_NAV_ITEM);
    evt.SetString(wxString(L"Rooms/") + room->GetDisplayName());
    evt.SetInt(static_cast<int>(m_current_room));
    evt.SetClientData(this);
    wxPostEvent(target, evt);

    wxCommandEvent props_evt(EVT_PROPERTIES_UPDATE);
    props_evt.SetClientData(target);
    wxPostEvent(target, props_evt);
}

void MyGLCanvas::NotifyRoomDataChanged(bool entities, bool warps, bool swaps, bool doors) {
    CommitPendingEdits();
    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    auto post_update = [&](const wxEventType& event_type, int selection) {
        wxCommandEvent evt(event_type);
        evt.SetInt(selection);
        evt.SetExtraLong(selection);
        evt.SetClientData(this);
        wxPostEvent(target, evt);
    };

    if (entities) {
        post_update(EVT_ENTITY_UPDATE, SelectedEntityListIndex());
    }
    if (warps) {
        post_update(EVT_WARP_UPDATE, SelectedWarpListIndex());
    }
    if (swaps) {
        post_update(EVT_TILESWAP_UPDATE, SelectedTileSwapListIndex());
    }
    if (doors) {
        post_update(EVT_DOOR_UPDATE, SelectedDoorListIndex());
    }
}

void MyGLCanvas::NotifyHeightmapChanged(bool /*moved*/) {
    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    wxCommandEvent props_evt(EVT_PROPERTIES_UPDATE);
    props_evt.SetClientData(target);
    wxPostEvent(target, props_evt);
}

void MyGLCanvas::NotifyHeightmapTargetChanged() {
    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    wxCommandEvent evt(EVT_GPU_HEIGHTMAP_TARGET_CHANGE);
    evt.SetClientData(this);
    wxPostEvent(target, evt);
}

void MyGLCanvas::NotifyLayerOpacityChanged() {
    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    wxCommandEvent evt(EVT_GPU_LAYER_OPACITY_CHANGE);
    evt.SetClientData(this);
    wxPostEvent(target, evt);
}

void MyGLCanvas::NotifyLayerBlockSelected() {
    if (!IsLayerEditMode() || !m_background_has_selection) {
        return;
    }

    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    wxCommandEvent evt(EVT_GPU_LAYER_BLOCK_SELECT);
    evt.SetInt(static_cast<int>(SelectedBackgroundBlockId()));
    evt.SetClientData(this);
    wxPostEvent(target, evt);
}

void MyGLCanvas::LoadRoomFromGameData(uint16_t roomnum, bool persist_edits, bool center_camera) {
    if (!m_gd) {
        return;
    }
    const bool room_changed = !m_initialized || m_current_room != roomnum;
    if (persist_edits && m_initialized) {
        PersistCurrentRoomEdits();
    }
    // Cancel any in-progress insertion or drag before switching rooms.
    CancelPendingObjectAdd();
    CancelActiveDrag();
    m_tileswap_preview_active = false;
    m_tileswap_preview_swap_index = -1;
    m_door_preview_active = false;
    m_door_preview_idx = -1;
    m_tileswap_preview_map.reset();
    m_heightmapRenderer.ClearPreviewMap();
    m_current_room = roomnum;
    if (room_changed && !m_restoring_history) {
        ClearUndoRedoHistory();
    }
    m_mapRenderer.LoadRoom(roomnum);
    m_heightmapRenderer.LoadRoom(roomnum);
    m_spriteRenderer.LoadRoom(roomnum);
    m_instances.clear();
    m_warps.clear();
    m_hovered_entity_idx = -1;
    m_selected_entity_idx = -1;
    m_hovered_warp_idx = -1;
    m_selected_warp_idx = -1;
    m_hovered_tileswap_region_idx = -1;
    m_selected_tileswap_region_idx = -1;
    m_hovered_door_idx = -1;
    m_selected_door_idx = -1;
    if (room_changed) {
        m_background_has_selection = false;
        m_background_selected_x = 0;
        m_background_selected_y = 0;
        m_heightmap_selection_anchor_x = 0;
        m_heightmap_selection_anchor_y = 0;
        m_heightmap_selection_drag_anchor_x = 0;
        m_heightmap_selection_drag_anchor_y = 0;
        m_heightmap_selected_cells.clear();
        m_heightmap_clipboard_valid = false;
        m_heightmap_clipboard_cell = 0;
        NotifyHeightmapTargetChanged();
    }
    auto sd = m_gd->GetSpriteData();
    auto entities = sd->GetRoomEntities(roomnum);
    m_room_entities = entities;
    float mat[9] = { 32.0f, 16.0f, 0.0f, -32.0f, 16.0f, 0.0f, 512.0f, 100.0f, 1.0f };
    uint32_t instance_id = 1;
    for (const auto& e : entities) {
        float entity_x = float(e.GetXDbl());
        float entity_y = float(e.GetYDbl());
        float entity_z = float(e.GetZDbl());
        float hitbox_base = 1.0f;
        float hitbox_height = 1.0f;
        if (sd->IsEntity(e.GetType())) {
            auto hitbox = sd->GetEntityHitbox(e.GetType());
            hitbox_base = HitboxBaseToBlocks(hitbox.base);
            hitbox_height = HitboxHeightToBlocks(hitbox.height);
        }
        float hitbox_offset = HitboxDrawOffset(hitbox_base);
        float floor_z = FloorUnderHitbox(
            entity_x + hitbox_offset,
            entity_y + hitbox_offset,
            hitbox_base * 0.5f);
        float ex_block = entity_x + hitbox_offset - m_mapRenderer.GetRoomLeft();
        float ey_block = entity_y + hitbox_offset - m_mapRenderer.GetRoomTop();
        float ez_block = entity_z;
        float px = mat[0] * ex_block + mat[3] * ey_block + mat[6];
        float py = mat[1] * ex_block + mat[4] * ey_block + mat[7] - ez_block * 32.0f;

        SpriteInstance inst{};
        inst.instance_id = instance_id++;
        inst.entity_id = e.GetType();
        inst.palette = e.GetPalette();
        inst.x = px;
        inst.y = py;
        inst.map_x = entity_x;
        inst.map_y = entity_y;
        inst.map_z = entity_z;
        inst.floor_z = floor_z;
        inst.z_extent = m_heightmapRenderer.GetZExtent();
        inst.hitbox_base = hitbox_base;
        inst.hitbox_height = hitbox_height;
        inst.hitbox_offset = hitbox_offset;
        inst.room_left = float(m_mapRenderer.GetRoomLeft());
        inst.room_top = float(m_mapRenderer.GetRoomTop());
        inst.dx = 0.0f;
        inst.dy = 0.0f;
        inst.scale = 2.0f;
        inst.anim_timer = 0.0f;
        inst.anim_speed = 1.0f;
        inst.orientation = e.GetOrientation();
        m_instances.push_back(inst);
    }

    uint32_t warp_instance_id = 1;
    uint32_t warp_key = 1;
    for (const auto& warp : m_gd->GetRoomData()->GetWarpsForRoom(roomnum)) {
        WarpInstance inst = MakeWarpInstance(
            warp,
            roomnum,
            warp_instance_id++,
            float(m_mapRenderer.GetRoomLeft()),
            float(m_mapRenderer.GetRoomTop()),
            m_heightmapRenderer.GetZExtent(),
            warp_key);
        UpdateWarpFloor(inst);
        m_warps.push_back(inst);
        if (warp.room1 == roomnum && warp.room2 == roomnum && warp.IsValid()) {
            WarpInstance dest_inst = MakeWarpInstance(
                warp,
                roomnum,
                warp_instance_id++,
                float(m_mapRenderer.GetRoomLeft()),
                float(m_mapRenderer.GetRoomTop()),
                m_heightmapRenderer.GetZExtent(),
                warp_key,
                2);
            UpdateWarpFloor(dest_inst);
            m_warps.push_back(dest_inst);
        }
        ++warp_key;
    }
    if (m_pending_warp_half && m_pending_warp_room == roomnum) {
        m_pending_warp_instance_id = warp_instance_id++;
        WarpInstance inst = MakeWarpInstance(
            m_pending_warp,
            roomnum,
            m_pending_warp_instance_id,
            float(m_mapRenderer.GetRoomLeft()),
            float(m_mapRenderer.GetRoomTop()),
            m_heightmapRenderer.GetZExtent(),
            warp_key++);
        UpdateWarpFloor(inst);
        m_warps.push_back(inst);
    }

    SortEntitiesGeometrically(m_instances);
    if (center_camera) {
        CenterCameraOnRoom();
    }
    ClampBackgroundSelection();
    UpdateStatusBar();
}

void MyGLCanvas::PanCameraByStep(int dx, int dy, float speed) {
    m_cam_x += speed * static_cast<float>(dx);
    m_cam_y += speed * static_cast<float>(dy);
    m_cam_x = std::round(m_cam_x);
    m_cam_y = std::round(m_cam_y);
}

void MyGLCanvas::ChangeZoomStep(int delta, float anchor_x, float anchor_y) {
    int old_idx = std::clamp(m_zoom_step_idx, 0, static_cast<int>(kZoomSteps.size()) - 1);
    int new_idx = std::clamp(old_idx + delta, 0, static_cast<int>(kZoomSteps.size()) - 1);
    if (new_idx == old_idx) {
        return;
    }
    float old_zoom = kZoomSteps[static_cast<std::size_t>(old_idx)];
    float new_zoom = kZoomSteps[static_cast<std::size_t>(new_idx)];
    float world_x = (anchor_x - m_cam_x) / old_zoom;
    float world_y = (anchor_y - m_cam_y) / old_zoom;
    m_zoom_step_idx = new_idx;
    m_cam_x = anchor_x - world_x * new_zoom;
    m_cam_y = anchor_y - world_y * new_zoom;
    m_cam_x = std::round(m_cam_x);
    m_cam_y = std::round(m_cam_y);
}

void MyGLCanvas::ResizeSelectedTileSwapByDelta(int dw, int dh) {
    if (m_selected_tileswap_region_idx < 0) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasTileDoorEditor(*this).ResizeSelectedTileSwapByDelta(dw, dh);
    NotifyRoomDataChanged(false, false, true, false);
}

void MyGLCanvas::UpdateStatusBar() {
    wxWindow* target = EventTarget();
    if (!target) {
        return;
    }

    auto name = m_gd->GetRoomData()->GetRoomDisplayName(m_current_room);
    const char* mode_name = "ROOM";
    if (m_editor_mode == EditorMode::BackgroundLayer) {
        mode_name = "BG_EDIT";
    } else if (m_editor_mode == EditorMode::ForegroundLayer) {
        mode_name = "FG_EDIT";
    } else if (m_editor_mode == EditorMode::Heightmap) {
        mode_name = "HM_EDIT";
    }

    int cursor_x = -1;
    int cursor_y = -1;
    if (IsAnyEditMode() && m_background_has_selection) {
        cursor_x = m_background_selected_x;
        cursor_y = m_background_selected_y;
    }

    wxCommandEvent evt(EVT_STATUSBAR_UPDATE);
    const char* occlusion_names[] = {"TOP", "GHOST", "HIDE"};
    evt.SetString(wxString::Format("MODE: %s | CUR: %d,%d | FPS: %.2f | Entities: %zu | Room: %d (%ls) | Cam: %.0f, %.0f | HM: %s %.0f | BG: %.1f FG: %.1f SPR: %.1f OCC: %s BOX: %s",
        mode_name, cursor_x, cursor_y,
        m_fps, m_instances.size(), m_current_room, name.c_str(), m_cam_x, m_cam_y,
        m_show_heightmap ? "ON" : "OFF", m_heightmapRenderer.GetZExtent(),
        OpacityForIndex(m_bg_opacity_idx), OpacityForIndex(m_fg_opacity_idx), OpacityForIndex(m_sprite_opacity_idx),
        occlusion_names[m_entity_occlusion_idx % 3], m_show_hitboxes ? "ON" : "OFF"));
    evt.SetInt(0);
    evt.SetClientData(target);
    wxPostEvent(target, evt);
}

std::set<uint32_t> MyGLCanvas::FindCollidedEntityIds() const {
    return FindCollidedEntities(m_instances);
}

void MyGLCanvas::OnIdle(wxIdleEvent& evt)
{
    if (!m_initialized || !m_gd || !IsShownOnScreen()) {
        return;
    }

    long now_ms = m_anim_stopwatch.Time();
    if (now_ms - m_last_frame_ms >= kTargetFrameMs) {
        float dt = std::clamp((now_ms - m_last_anim_ms) / 1000.0f, 0.0f, 0.1f);
        m_last_anim_ms = now_ms;
        m_render_deferred = false;

        UpdateAnimations(dt);
        Refresh(false);
    }

    evt.RequestMore();
}

void MyGLCanvas::UpdateAnimations(float dt)
{
    // Recompute floors/projections periodically instead of every paint.
    // This avoids heavy per-frame work, which is especially noticeable at high zoom.
    if ((m_animation_update_count & 0x07) == 0) {
        RefreshObjectPlacementsFromHeightmap();
    }

    auto sd = m_gd->GetSpriteData();
    for (auto& inst : m_instances) {
        if (!sd->IsEntity(inst.entity_id)) {
            continue;
        }
        uint8_t sid = sd->GetSpriteFromEntity(inst.entity_id);
        auto flags = sd->GetSpriteAnimationFlags(sid); auto anims = sd->GetSpriteAnimations(sid);
        bool has_away = !flags.do_not_rotate && !sd->IsEntityItem(inst.entity_id);
        int towards = 0, away = 0;
        if (flags.has_full_animations) { towards = 1; away = 1; }
        if (has_away) { towards = towards * 2 + 1; away = away * 2; }
        int aid = (inst.dy < 0) ? away : towards;
        if (aid >= (int)anims.size()) aid = 0;
        const auto& frames = sd->GetSpriteAnimationFrames(anims[aid]);
        if (!frames.empty() && !sd->IsEntityItem(inst.entity_id)) { 
            inst.anim_timer += inst.anim_speed * 8.0f * dt;
            while (inst.anim_timer >= frames.size()) inst.anim_timer -= frames.size();
        }
    }

    ++m_animation_update_count;
}

void MyGLCanvas::RecordRenderedFrame()
{
    m_last_frame_ms = m_anim_stopwatch.Time();
    m_frame_count++;
    if (m_fps_stopwatch.Time() >= 1000) {
        m_fps = (m_frame_count*1000.0f)/m_fps_stopwatch.Time();
        m_frame_count = 0;
        m_fps_stopwatch.Start();
        UpdateStatusBar();
    }
}

bool MyGLCanvas::HandleKeyDown(wxKeyEvent& evt) {
    if (evt.ControlDown() && !evt.AltDown()) {
        int key = evt.GetKeyCode();
        if (key == 'Z') {
            Undo();
            UpdateStatusBar();
            return true;
        }
        if (key == 'Y') {
            Redo();
            UpdateStatusBar();
            return true;
        }
    }

    if (!IsAnyEditMode() && evt.GetKeyCode() == WXK_RETURN) {
        if (!m_dragging_entity && !m_dragging_warp && !m_dragging_door &&
            !m_dragging_tileswap_region && !HasPendingObjectAdd()) {
            if (OpenSelectedObjectProperties()) {
                return true;
            }
        }
    }

    bool handled = false;
    if (IsHeightmapEditMode()) {
        handled = GLCanvasHeightmapMode(*this).HandleKeyDown(evt);
    } else if (IsLayerEditMode()) {
        handled = GLCanvasLayerEditMode(*this).HandleKeyDown(evt);
    } else {
        handled = GLCanvasRoomMode(*this).HandleKeyDown(evt);
    }
    UpdateStatusBar();
    return handled;
}

void MyGLCanvas::OnKeyDown(wxKeyEvent& evt) {
    evt.Skip(!HandleKeyDown(evt));
}

void MyGLCanvas::OnMouseWheel(wxMouseEvent& evt) {
    if (evt.ControlDown()) {
        float steps = WheelSteps(evt);
        int delta = steps > 0.0f ? 1 : (steps < 0.0f ? -1 : 0);
        if (delta != 0) {
            ChangeZoomStep(
                delta,
                static_cast<float>(evt.GetPosition().x),
                static_cast<float>(evt.GetPosition().y));
        }
        Refresh();
        return;
    }

    constexpr float wheel_pan_speed = 80.0f;
    float movement = WheelSteps(evt) * wheel_pan_speed;

    if (evt.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL) {
        m_cam_x += movement;
    } else {
        m_cam_y += movement;
    }
    m_cam_x = std::round(m_cam_x);
    m_cam_y = std::round(m_cam_y);

    Refresh();
}

void MyGLCanvas::OnSize(wxSizeEvent& evt) {
    if (m_initialized) {
        CenterCameraOnRoom();
        Refresh();
    }
    evt.Skip();
}

void MyGLCanvas::OnMouseMove(wxMouseEvent& evt) {
    m_last_mouse_pos = evt.GetPosition();
    if (m_dragging_pan) {
        m_cam_x = m_drag_pan_start_cam_x + static_cast<float>(evt.GetPosition().x - m_drag_pan_start_mouse.x);
        m_cam_y = m_drag_pan_start_cam_y + static_cast<float>(evt.GetPosition().y - m_drag_pan_start_mouse.y);
        m_cam_x = std::round(m_cam_x);
        m_cam_y = std::round(m_cam_y);
        UpdateStatusBar();
        Refresh();
        return;
    }
    if (IsHeightmapEditMode()) {
        GLCanvasHeightmapMode(*this).HandleMouseMove(evt);
        UpdateStatusBar();
        evt.Skip();
        return;
    }
    if (IsLayerEditMode()) {
        GLCanvasLayerEditMode(*this).HandleMouseMove(evt);
        UpdateStatusBar();
        evt.Skip();
        return;
    }

    GLCanvasRoomMode(*this).HandleMouseMove(evt);
    UpdateStatusBar();
    evt.Skip();
}

void MyGLCanvas::OnLeftDown(wxMouseEvent& evt) {
    SetFocus();
    m_last_mouse_pos = evt.GetPosition();
    if (IsHeightmapEditMode()) {
        GLCanvasHeightmapMode(*this).HandleLeftDown(evt);
        UpdateStatusBar();
        return;
    }
    if (IsLayerEditMode()) {
        GLCanvasLayerEditMode(*this).HandleLeftDown(evt);
        UpdateStatusBar();
        return;
    }
    GLCanvasRoomMode(*this).HandleLeftDown(evt);
    NotifySelectionChanged();
    UpdateStatusBar();
}

void MyGLCanvas::OnLeftDClick(wxMouseEvent& evt) {
    SetFocus();
    m_last_mouse_pos = evt.GetPosition();
    if (IsAnyEditMode()) {
        evt.Skip();
        return;
    }
    if (SelectObjectAt(evt.GetPosition())) {
        CancelActiveDrag();
        NotifySelectionChanged();
        OpenSelectedObjectProperties();
        UpdateStatusBar();
        Refresh();
        return;
    }
    evt.Skip();
}

void MyGLCanvas::OnLeftUp(wxMouseEvent& evt) {
    if (m_heightmap_dragging_select || m_heightmap_dragging_draw || m_heightmap_dragging_line || m_heightmap_dragging_selection_move) {
        if (m_heightmap_dragging_select) {
            int cell_x = -1;
            int cell_y = -1;
            if (HeightmapVirtualCellAt(evt.GetPosition(), cell_x, cell_y)) {
                UpdateHeightmapSelectionDrag(cell_x, cell_y);
            }
            FinishHeightmapSelectionDrag();
        }
        if (m_heightmap_dragging_draw) {
            CommitHeightmapDrawStroke();
        }
        if (m_heightmap_dragging_line) {
            int cell_x = -1;
            int cell_y = -1;
            if (HeightmapVirtualCellAt(evt.GetPosition(), cell_x, cell_y)) {
                UpdateHeightmapLineDrag(cell_x, cell_y, evt.ShiftDown());
            }
            CommitHeightmapLineDrag();
        }
        if (m_heightmap_dragging_selection_move) {
            CommitHeightmapSelectionMoveDrag();
        }
        m_heightmap_dragging_draw = false;
        m_heightmap_last_draw_x = -1;
        m_heightmap_last_draw_y = -1;
        if (HasCapture()) {
            ReleaseMouse();
        }
        Refresh();
    } else if (m_layer_dragging_select || m_layer_dragging_draw || m_layer_dragging_selection_move || m_layer_dragging_line) {
        if (m_layer_dragging_select) {
            int cell_x = -1;
            int cell_y = -1;
            if (BackgroundVirtualCellAt(evt.GetPosition(), cell_x, cell_y)) {
                UpdateLayerSelectionDrag(cell_x, cell_y);
            }
            FinishLayerSelectionDrag();
        }
        if (m_layer_dragging_draw) {
            CommitLayerDrawStroke();
        }
        if (m_layer_dragging_selection_move) {
            int cell_x = -1;
            int cell_y = -1;
            if (BackgroundCellAt(evt.GetPosition(), cell_x, cell_y)) {
                UpdateLayerSelectionMoveDrag(cell_x, cell_y);
            }
            CommitLayerSelectionMoveDrag();
        }
        if (m_layer_dragging_line) {
            int cell_x = -1;
            int cell_y = -1;
            if (BackgroundVirtualCellAt(evt.GetPosition(), cell_x, cell_y)) {
                UpdateLayerLineDrag(cell_x, cell_y, evt.ShiftDown(), evt.AltDown());
            }
            CommitLayerLineDrag();
        }
        m_layer_dragging_select = false;
        m_layer_dragging_draw = false;
        m_layer_dragging_selection_move = false;
        m_layer_dragging_line = false;
        m_layer_last_draw_x = -1;
        m_layer_last_draw_y = -1;
        if (HasCapture()) {
            ReleaseMouse();
        }
        Refresh();
    } else if (m_dragging_entity) {
        EndEntityDrag();
    } else if (m_dragging_warp) {
        EndWarpDrag();
    } else if (m_dragging_door) {
        EndDoorDrag();
    } else if (m_dragging_tileswap_region) {
        EndTileSwapRegionDrag();
    } else if (m_dragging_pan) {
        m_dragging_pan = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
        SetCursor(wxCursor(wxCURSOR_ARROW));
    }
    evt.Skip();
}

void MyGLCanvas::OnMiddleDown(wxMouseEvent& evt) {
    SetFocus();
    m_last_mouse_pos = evt.GetPosition();
    m_dragging_pan = true;
    m_drag_pan_start_mouse = evt.GetPosition();
    m_drag_pan_start_cam_x = m_cam_x;
    m_drag_pan_start_cam_y = m_cam_y;
    SetCursor(wxCursor(wxCURSOR_SIZING));
    if (!HasCapture()) {
        CaptureMouse();
    }
}

void MyGLCanvas::OnMiddleUp(wxMouseEvent& evt) {
    if (m_dragging_pan) {
        m_dragging_pan = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
        SetCursor(wxCursor(wxCURSOR_ARROW));
        Refresh();
    }
    evt.Skip();
}

void MyGLCanvas::OnRightDown(wxMouseEvent& evt) {
    SetFocus();
    m_last_mouse_pos = evt.GetPosition();
    if (m_heightmap_dragging_line || m_heightmap_dragging_selection_move) {
        if (m_heightmap_dragging_line) {
            CancelHeightmapLineDrag();
        }
        if (m_heightmap_dragging_selection_move) {
            CancelHeightmapSelectionMoveDrag();
        }
        if (HasCapture()) {
            ReleaseMouse();
        }
        Refresh();
        UpdateStatusBar();
        return;
    }

    if (IsAnyEditMode() && m_drawing_tool == DrawingTool::Select) {
        SetDrawingTool(DrawingTool::Draw);
        UpdateStatusBar();
        return;
    }

    if (IsHeightmapEditMode()) {
        GLCanvasHeightmapMode(*this).HandleRightDown(evt);
        UpdateStatusBar();
        return;
    }
    if (IsLayerEditMode()) {
        GLCanvasLayerEditMode(*this).HandleRightDown(evt);
        UpdateStatusBar();
        return;
    }
    GLCanvasRoomMode(*this).HandleRightDown(evt);
    NotifySelectionChanged();
    UpdateStatusBar();
}

void MyGLCanvas::OnRightUp(wxMouseEvent& evt) {
    if (m_dragging_entity) {
        EndEntityDrag();
    }
    evt.Skip();
}

void MyGLCanvas::OnMouseLeave(wxMouseEvent& evt) {
    if (m_dragging_entity || m_dragging_warp || m_dragging_door || m_dragging_tileswap_region || m_dragging_pan ||
        m_heightmap_dragging_select || m_heightmap_dragging_draw || m_heightmap_dragging_line || m_heightmap_dragging_selection_move) {
        evt.Skip();
        return;
    }

    if (IsAnyEditMode()) {
        m_heightmapRenderer.ClearHover();
        m_background_has_hover = false;
        SetCursor(wxCursor(wxCURSOR_ARROW));
        Refresh();
        evt.Skip();
        return;
    }

    m_hovered_entity_idx = -1;
    m_hovered_warp_idx = -1;
    m_hovered_tileswap_region_idx = -1;
    m_hovered_door_idx = -1;
    m_heightmapRenderer.ClearHover();
    SetCursor(wxCursor(wxCURSOR_ARROW));
    Refresh();
    evt.Skip();
}

void MyGLCanvas::StartEntityDrag(int entity_idx, const wxMouseEvent& evt, bool z_axis_only, bool shadow_drag) {
    GLCanvasEntityEditor(*this).StartEntityDrag(entity_idx, evt, z_axis_only, shadow_drag);
}

bool MyGLCanvas::IsLayerEditMode() const {
    return m_editor_mode == EditorMode::BackgroundLayer || m_editor_mode == EditorMode::ForegroundLayer;
}

bool MyGLCanvas::IsHeightmapEditMode() const {
    return m_editor_mode == EditorMode::Heightmap;
}

bool MyGLCanvas::IsAnyEditMode() const {
    return IsLayerEditMode() || IsHeightmapEditMode();
}

Tilemap3D::Layer MyGLCanvas::CurrentEditLayer() const {
    return m_editor_mode == EditorMode::ForegroundLayer ? Tilemap3D::Layer::FG : Tilemap3D::Layer::BG;
}

void MyGLCanvas::ApplyHeightmapEditZExtent() {
    m_heightmapRenderer.SetZExtent(m_heightmap_z_scale * kHeightmapEditorMaxZExtent);
}

void MyGLCanvas::SetEditorMode(EditorMode mode) {
    if (m_editor_mode == mode) {
        return;
    }

    if (m_editor_mode != EditorMode::Heightmap && mode == EditorMode::Heightmap) {
        m_non_heightmap_z_extent = m_heightmapRenderer.GetZExtent();
        ApplyHeightmapEditZExtent();
    } else if (m_editor_mode == EditorMode::Heightmap && mode != EditorMode::Heightmap) {
        m_heightmapRenderer.SetZExtent(m_non_heightmap_z_extent);
    }

    m_editor_mode = mode;
    if (IsAnyEditMode()) {
        // Entering heightmap/layer modes should start with no active cell selection.
        m_background_has_selection = false;
        m_heightmap_selected_cells.clear();
        m_heightmap_selection_drag_base.clear();
        m_layer_selection_drag_base.clear();
        m_layer_selection_move_values.clear();
        m_heightmap_selection_move_values.clear();
        NotifyHeightmapTargetChanged();
        if (m_drawing_tool != DrawingTool::Select) {
            SetDrawingTool(DrawingTool::Select);
        }
        if (IsHeightmapEditMode()) {
            m_heightmapRenderer.SetHoverPoint(ScreenToWorldX(m_last_mouse_pos.x), ScreenToWorldY(m_last_mouse_pos.y));
        } else {
            m_heightmapRenderer.ClearHover();
        }
        if (!IsLayerEditMode()) {
            m_background_has_hover = false;
        }
        SetCursor(wxCursor(wxCURSOR_ARROW));
    }
    UpdateStatusBar();
    Refresh();

    wxCommandEvent evt(EVT_GPU_EDITOR_MODE_CHANGE);
    evt.SetInt(static_cast<int>(m_editor_mode));
    evt.SetClientData(this);
    wxWindow* target = EventTarget();
    if (target) {
        wxPostEvent(target, evt);
    }
}

bool MyGLCanvas::HeightmapCellAt(const wxPoint& point, int& cell_x, int& cell_y) {
    auto map = CurrentRoomMap();
    if (!map || point == wxDefaultPosition) {
        return false;
    }

    m_heightmapRenderer.SetHoverPoint(ScreenToWorldX(point.x), ScreenToWorldY(point.y));
    int hover_x = m_heightmapRenderer.GetHoverX();
    int hover_y = m_heightmapRenderer.GetHoverY();
    if (hover_x < 0 || hover_y < 0 || hover_x >= map->GetHeightmapWidth() || hover_y >= map->GetHeightmapHeight()) {
        return false;
    }

    cell_x = hover_x;
    cell_y = hover_y;
    return true;
}

bool MyGLCanvas::HeightmapVirtualCellAt(const wxPoint& point, int& cell_x, int& cell_y) const {
    auto map = CurrentRoomMap();
    if (!map || point == wxDefaultPosition) {
        return false;
    }

    PickPoint map_point = ScreenToHeightmapPoint(
        ScreenToWorldX(point.x),
        ScreenToWorldY(point.y),
        static_cast<float>(m_mapRenderer.GetRoomLeft()),
        static_cast<float>(m_mapRenderer.GetRoomTop()));
    cell_x = static_cast<int>(std::floor(map_point.x));
    cell_y = static_cast<int>(std::floor(map_point.y));
    return true;
}

bool MyGLCanvas::BackgroundCellAt(const wxPoint& point, int& cell_x, int& cell_y) const {
    auto map = CurrentRoomMap();
    if (!map || point == wxDefaultPosition) {
        return false;
    }

    float zoom = std::max(ZoomFactor(), 0.0001f);
    float cell_w = 32.0f * zoom;
    float cell_h = 32.0f * zoom;
    float x_offset = CurrentEditLayer() == Tilemap3D::Layer::FG ? -32.0f : 0.0f;

    bool found = false;
    int best_x = -1;
    int best_y = -1;

    // Match draw order so overlap resolves to the top-most rendered cell.
    for (int y = 0; y < m_mapRenderer.GetRoomHeight(); ++y) {
        for (int x = 0; x < m_mapRenderer.GetRoomWidth(); ++x) {
            float world_x = 32.0f * static_cast<float>(x) - 32.0f * static_cast<float>(y) + 512.0f + x_offset;
            float world_y = 16.0f * static_cast<float>(x) + 16.0f * static_cast<float>(y) + 100.0f;
            float left = world_x * zoom + m_cam_x;
            float top = world_y * zoom + m_cam_y;
            float right = left + cell_w;
            float bottom = top + cell_h;
            if (static_cast<float>(point.x) >= left &&
                static_cast<float>(point.x) <= right &&
                static_cast<float>(point.y) >= top &&
                static_cast<float>(point.y) <= bottom) {
                found = true;
                best_x = x;
                best_y = y;
            }
        }
    }

    if (!found) {
        return false;
    }

    cell_x = best_x;
    cell_y = best_y;
    return true;
}

bool MyGLCanvas::BackgroundVirtualCellAt(const wxPoint& point, int& cell_x, int& cell_y) const {
    auto map = CurrentRoomMap();
    if (!map || point == wxDefaultPosition) {
        return false;
    }

    float x_offset = CurrentEditLayer() == Tilemap3D::Layer::FG ? -32.0f : 0.0f;
    float world_x = ScreenToWorldX(point.x) - x_offset - 16.0f;
    float world_y = ScreenToWorldY(point.y) - 16.0f;
    float a = (world_x - 512.0f) / 32.0f;
    float b = (world_y - 100.0f) / 16.0f;
    cell_x = static_cast<int>(std::round((a + b) * 0.5f));
    cell_y = static_cast<int>(std::round((b - a) * 0.5f));
    return true;
}

float MyGLCanvas::ZoomFactor() const {
    int idx = std::clamp(m_zoom_step_idx, 0, static_cast<int>(kZoomSteps.size()) - 1);
    return kZoomSteps[static_cast<std::size_t>(idx)];
}

float MyGLCanvas::ScreenToWorldX(int screen_x) const {
    return (static_cast<float>(screen_x) - m_cam_x) / ZoomFactor();
}

float MyGLCanvas::ScreenToWorldY(int screen_y) const {
    return (static_cast<float>(screen_y) - m_cam_y) / ZoomFactor();
}

void MyGLCanvas::RefreshObjectPlacementsFromHeightmap() {
    for (auto& inst : m_instances) {
        inst.z_extent = m_heightmapRenderer.GetZExtent();
        inst.floor_z = FloorUnderHitbox(
            inst.map_x + inst.hitbox_offset,
            inst.map_y + inst.hitbox_offset,
            inst.hitbox_base * 0.5f);
        UpdateEntityProjection(inst);
    }

    for (auto& warp : m_warps) {
        warp.z_extent = m_heightmapRenderer.GetZExtent();
        UpdateWarpFloor(warp);
    }
}

void MyGLCanvas::UpdateEntityProjection(SpriteInstance& inst) {
    GLCanvasEntityEditor(*this).UpdateEntityProjection(inst);
}

void MyGLCanvas::UpdateWarpFloor(WarpInstance& warp) {
    warp.floor_z = FloorUnderRect(warp.x, warp.y, warp.x + warp.width, warp.y + warp.height);
}

void MyGLCanvas::CenterCameraOnRoom() {
    int client_w = 0;
    int client_h = 0;
    GetClientSize(&client_w, &client_h);
    if (client_w <= 0 || client_h <= 0 || m_mapRenderer.GetRoomWidth() <= 0 || m_mapRenderer.GetRoomHeight() <= 0) {
        m_cam_x = 0.0f;
        m_cam_y = 0.0f;
        return;
    }

    auto project = [](float x, float y) {
        return PickPoint{
            32.0f * x - 32.0f * y + 512.0f,
            16.0f * x + 16.0f * y + 100.0f
        };
    };

    float room_w = float(m_mapRenderer.GetRoomWidth());
    float room_h = float(m_mapRenderer.GetRoomHeight());
    std::array<PickPoint, 4> corners = {
        project(0.0f, 0.0f),
        project(room_w, 0.0f),
        project(0.0f, room_h),
        project(room_w, room_h)
    };

    float min_x = corners.front().x;
    float max_x = corners.front().x;
    float min_y = corners.front().y;
    float max_y = corners.front().y;
    for (const auto& point : corners) {
        min_x = std::min(min_x, point.x);
        max_x = std::max(max_x, point.x);
        min_y = std::min(min_y, point.y);
        max_y = std::max(max_y, point.y);
    }

    float zoom = ZoomFactor();
    m_cam_x = float(client_w) * 0.5f - ((min_x + max_x) * 0.5f) * zoom;
    m_cam_y = float(client_h) * 0.5f - ((min_y + max_y) * 0.5f) * zoom;
    m_cam_x = std::round(m_cam_x);
    m_cam_y = std::round(m_cam_y);
}

void MyGLCanvas::EnsureWorldRectVisible(float min_x, float min_y, float max_x, float max_y) {
    int client_w = 0;
    int client_h = 0;
    GetClientSize(&client_w, &client_h);
    if (client_w <= 0 || client_h <= 0) {
        return;
    }

    if (min_x > max_x) {
        std::swap(min_x, max_x);
    }
    if (min_y > max_y) {
        std::swap(min_y, max_y);
    }

    float zoom = std::max(ZoomFactor(), 0.0001f);
    float view_min_x = ScreenToWorldX(0);
    float view_min_y = ScreenToWorldY(0);
    float view_max_x = ScreenToWorldX(client_w);
    float view_max_y = ScreenToWorldY(client_h);
    if (view_min_x > view_max_x) {
        std::swap(view_min_x, view_max_x);
    }
    if (view_min_y > view_max_y) {
        std::swap(view_min_y, view_max_y);
    }

    constexpr float margin_px = 36.0f;
    float margin_world = margin_px / zoom;
    bool visible_x = min_x >= view_min_x + margin_world && max_x <= view_max_x - margin_world;
    bool visible_y = min_y >= view_min_y + margin_world && max_y <= view_max_y - margin_world;
    if (visible_x && visible_y) {
        return;
    }

    float center_x = (min_x + max_x) * 0.5f;
    float center_y = (min_y + max_y) * 0.5f;
    m_cam_x = float(client_w) * 0.5f - center_x * zoom;
    m_cam_y = float(client_h) * 0.5f - center_y * zoom;
    m_cam_x = std::round(m_cam_x);
    m_cam_y = std::round(m_cam_y);
}

void MyGLCanvas::FocusCameraOnSelectedObjectIfNeeded() {
    if (m_selected_entity_idx >= 0 && m_selected_entity_idx < static_cast<int>(m_instances.size())) {
        const SpriteInstance& inst = m_instances[static_cast<std::size_t>(m_selected_entity_idx)];
        float center_x = inst.map_x + inst.hitbox_offset;
        float center_y = inst.map_y + inst.hitbox_offset;
        float half_base = std::max(inst.hitbox_base * 0.5f, 0.5f);
        float top_z = inst.map_z + std::max(inst.hitbox_height, 0.125f);
        PickPoint p0 = ProjectEntityGridPoint(inst, center_x - half_base, center_y - half_base, top_z);
        PickPoint p1 = ProjectEntityGridPoint(inst, center_x + half_base, center_y + half_base, inst.map_z);
        EnsureWorldRectVisible(
            std::min(p0.x, p1.x),
            std::min(p0.y, p1.y),
            std::max(p0.x, p1.x),
            std::max(p0.y, p1.y));
        return;
    }

    if (m_selected_warp_idx >= 0 && m_selected_warp_idx < static_cast<int>(m_warps.size())) {
        const WarpInstance& warp = m_warps[static_cast<std::size_t>(m_selected_warp_idx)];
        float z = warp.floor_z;
        PickPoint p0 = ProjectWarpGridPoint(warp, warp.x, warp.y, z);
        PickPoint p1 = ProjectWarpGridPoint(warp, warp.x + warp.width, warp.y + warp.height, z);
        EnsureWorldRectVisible(
            std::min(p0.x, p1.x),
            std::min(p0.y, p1.y),
            std::max(p0.x, p1.x),
            std::max(p0.y, p1.y));
        return;
    }

    if (m_selected_tileswap_region_idx >= 0) {
        auto regions = BuildTileSwapRegionGeometries(m_gd, m_current_room, m_mapRenderer, m_heightmapRenderer.GetZExtent());
        if (m_selected_tileswap_region_idx < static_cast<int>(regions.size())) {
            const auto& region = regions[static_cast<std::size_t>(m_selected_tileswap_region_idx)];
            EnsureWorldRectVisible(region.bounds.min_x, region.bounds.min_y, region.bounds.max_x, region.bounds.max_y);
        }
        return;
    }

    if (m_selected_door_idx >= 0) {
        auto doors = BuildDoorGeometries(
            m_gd,
            m_current_room,
            m_mapRenderer,
            m_heightmapRenderer.GetZExtent(),
            m_tileswap_preview_map);
        for (const auto& door : doors) {
            if (door.index == m_selected_door_idx) {
                EnsureWorldRectVisible(door.bounds.min_x, door.bounds.min_y, door.bounds.max_x, door.bounds.max_y);
                break;
            }
        }
    }
}

void MyGLCanvas::RefreshEntityMetadata(SpriteInstance& inst) {
    GLCanvasEntityEditor(*this).RefreshEntityMetadata(inst);
}

void MyGLCanvas::PersistCurrentRoomEdits() {
    if (m_room_entities.empty() && m_instances.empty() && m_warps.empty()) {
        return;
    }

    std::vector<Landstalker::Entity> entities = BuildCurrentRoomEntities();
    m_gd->GetSpriteData()->SetRoomEntities(m_current_room, entities);
    m_room_entities = entities;

    // A pending warp half is not persisted (it has no destination yet), so its
    // latest editor position/size must be captured before it is filtered out.
    if (m_pending_warp_half && m_pending_warp_room == m_current_room) {
        int pending_idx = FindWarpIndex(m_pending_warp_instance_id);
        if (pending_idx >= 0) {
            const WarpInstance& inst = m_warps[static_cast<std::size_t>(pending_idx)];
            if (inst.DestinationRoom() == 0xFFFF) {
                Landstalker::WarpList::Warp warp = inst.warp;
                uint8_t x = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.x)), 0, 63));
                uint8_t y = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.y)), 0, 63));
                if (inst.current_room_is_room1) {
                    warp.room1 = m_current_room;
                    warp.x1 = x;
                    warp.y1 = y;
                } else {
                    warp.room2 = m_current_room;
                    warp.x2 = x;
                    warp.y2 = y;
                }
                warp.x_size = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.width)), 1, 63));
                warp.y_size = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.height)), 1, 63));
                m_pending_warp = warp;
            }
        }
    }

    m_gd->GetRoomData()->SetWarpsForRoom(m_current_room, BuildCurrentRoomWarps());
}

void MyGLCanvas::OnPaint(wxPaintEvent&) {
    // This is the entry point for drawing. wxPaintDC is a helper that ensures 
    // the windowing system knows we are drawing.
    wxPaintDC dc(this);
    if (!m_gd) {
        return;
    }
    if (!m_context) {
        m_context = new wxGLContext(this);
    }

    // Set the current OpenGL context to this window.
    if (!m_context || !SetCurrent(*m_context)) {
        m_gl_init_failed = true;
        wxLogError("Failed to make the OpenGL context current.");
        return;
    }
    
    // Perform one-time initialization of shaders and textures
    if (m_gl_init_failed) {
        return;
    }

    if (!m_initialized) {
        if (!InitGLLoader()) {
            m_gl_init_failed = true;
            wxLogError("OpenGL initialization failed. Check GLEW and graphics driver setup.");
            return;
        }
        m_mapRenderer.Init();
        m_spriteRenderer.Init();
        LoadRoom(m_current_room);
        m_initialized = true;
    }

    long now_ms = m_anim_stopwatch.Time();
    if (now_ms - m_last_frame_ms < kTargetFrameMs) {
        m_render_deferred = true;
        return;
    }
    m_render_deferred = false;
    
    // Set up the viewport and simple orthographic projection.
    // wx reports client size in logical pixels; OpenGL needs the backing framebuffer size.
    int w, h; GetClientSize(&w, &h);
    const float content_scale = static_cast<float>(GetContentScaleFactor());
    const int fb_w = std::max(1, static_cast<int>(std::lround(static_cast<float>(w) * content_scale)));
    const int fb_h = std::max(1, static_cast<int>(std::lround(static_cast<float>(h) * content_scale)));
    glViewport(0, 0, fb_w, fb_h); 
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); 
    // This sets up a 2D coordinate system matching the window size in pixels
    glOrtho(0, w, h, 0, -1, 1);
    
    // Clear the screen to a dark blue color
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); 
    glTranslatef(m_cam_x, m_cam_y, 0.0f);
    glScalef(ZoomFactor(), ZoomFactor(), 1.0f);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f); 
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    uint32_t selected_entity_id = 0;
    uint32_t hovered_entity_id = 0;
    if (m_selected_entity_idx >= 0 && m_selected_entity_idx < static_cast<int>(m_instances.size())) {
        selected_entity_id = m_instances[static_cast<std::size_t>(m_selected_entity_idx)].instance_id;
    }
    if (m_hovered_entity_idx >= 0 && m_hovered_entity_idx < static_cast<int>(m_instances.size())) {
        hovered_entity_id = m_instances[static_cast<std::size_t>(m_hovered_entity_idx)].instance_id;
    }
    SortEntitiesGeometrically(m_instances);
    m_selected_entity_idx = selected_entity_id != 0 ? FindInstanceIndex(selected_entity_id) : -1;
    m_hovered_entity_idx = hovered_entity_id != 0 ? FindInstanceIndex(hovered_entity_id) : -1;

    if (IsHeightmapEditMode()) {
        GLCanvasHeightmapMode(*this).Render(w, h);
        return;
    }

    if (IsLayerEditMode()) {
        GLCanvasLayerEditMode(*this).Render(w, h);
        return;
    }

    GLCanvasRoomMode(*this).Render(w, h);
}




