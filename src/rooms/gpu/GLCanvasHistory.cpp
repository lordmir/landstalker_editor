#include "GLCanvas.h"
#include "GLCanvasObjectSupport.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>

using namespace Landstalker;

namespace {
constexpr std::size_t kMaxUndoStates = 100;
}

bool GLCanvas::CanUndo() const {
    if (IsObjectHistoryMode()) {
        return !m_object_undo_stack.empty();
    }
    if (IsBackgroundLayerHistoryMode()) {
        return !m_bg_layer_undo_stack.empty();
    }
    if (IsForegroundLayerHistoryMode()) {
        return !m_fg_layer_undo_stack.empty();
    }
    return !m_map_undo_stack.empty();
}

bool GLCanvas::CanRedo() const {
    if (IsObjectHistoryMode()) {
        return !m_object_redo_stack.empty();
    }
    if (IsBackgroundLayerHistoryMode()) {
        return !m_bg_layer_redo_stack.empty();
    }
    if (IsForegroundLayerHistoryMode()) {
        return !m_fg_layer_redo_stack.empty();
    }
    return !m_map_redo_stack.empty();
}

void GLCanvas::CaptureUndoState(bool structural) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    if (IsBackgroundLayerHistoryMode()) {
        m_bg_layer_undo_stack.push_back(structural
            ? BuildFullMapLayerUndoState(Tilemap3D::Layer::BG)
            : BuildLayerUndoState(Tilemap3D::Layer::BG));
        if (m_bg_layer_undo_stack.size() > kMaxUndoStates) {
            m_bg_layer_undo_stack.erase(m_bg_layer_undo_stack.begin());
        }
        m_bg_layer_redo_stack.clear();
        if (structural) {
            InvalidateCrossLayerHistory();
        }
        NotifyHeightmapTargetChanged();
        return;
    }
    if (IsForegroundLayerHistoryMode()) {
        m_fg_layer_undo_stack.push_back(structural
            ? BuildFullMapLayerUndoState(Tilemap3D::Layer::FG)
            : BuildLayerUndoState(Tilemap3D::Layer::FG));
        if (m_fg_layer_undo_stack.size() > kMaxUndoStates) {
            m_fg_layer_undo_stack.erase(m_fg_layer_undo_stack.begin());
        }
        m_fg_layer_redo_stack.clear();
        if (structural) {
            InvalidateCrossLayerHistory();
        }
        NotifyHeightmapTargetChanged();
        return;
    }

    m_map_undo_stack.push_back(std::make_shared<Tilemap3D>(*map));
    if (m_map_undo_stack.size() > kMaxUndoStates) {
        m_map_undo_stack.erase(m_map_undo_stack.begin());
    }
    m_map_redo_stack.clear();
    NotifyHeightmapTargetChanged();
}

void GLCanvas::RestoreUndoState(const std::shared_ptr<Tilemap3D>& state) {
    auto map = CurrentRoomMap();
    if (!map || !state) {
        return;
    }

    *map = *state;
    m_tileswap_preview_active = false;
    m_tileswap_preview_swap_index = -1;
    m_door_preview_active = false;
    m_door_preview_idx = -1;
    m_tileswap_preview_map.reset();
    m_heightmapRenderer.ClearPreviewMap();
    ResetHeightmapEditState();

    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    m_heightmapRenderer.LoadRoom(m_current_room);
    RefreshObjectPlacementsFromHeightmap();
    UpdateHeightmapClipboardFromSelectedCell();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    NotifyLayerBlockSelected();
    UpdateStatusBar();
    Refresh();
}

bool GLCanvas::IsObjectHistoryMode() const {
    return m_editor_mode == EditorMode::Room;
}

bool GLCanvas::IsBackgroundLayerHistoryMode() const {
    return m_editor_mode == EditorMode::BackgroundLayer;
}

bool GLCanvas::IsForegroundLayerHistoryMode() const {
    return m_editor_mode == EditorMode::ForegroundLayer;
}

GLCanvas::LayerUndoState GLCanvas::BuildLayerUndoState(Tilemap3D::Layer layer) const {
    LayerUndoState state{};
    state.layer = layer;

    auto map = CurrentRoomMap();
    if (!map) {
        return state;
    }

    int count = map->GetWidth() * map->GetHeight();
    state.blocks.reserve(static_cast<std::size_t>(std::max(0, count)));
    for (int i = 0; i < count; ++i) {
        state.blocks.push_back(map->GetBlock(static_cast<uint16_t>(i), layer).value);
    }
    return state;
}

GLCanvas::LayerUndoState GLCanvas::BuildFullMapLayerUndoState(Tilemap3D::Layer layer) const {
    LayerUndoState state{};
    state.layer = layer;

    auto map = CurrentRoomMap();
    if (map) {
        state.full_map = std::make_shared<Tilemap3D>(*map);
    }
    return state;
}

void GLCanvas::InvalidateCrossLayerHistory() {
    // Structural edits shift block indices in both layers, so blocks-only
    // snapshots captured for the other layer no longer line up with the map.
    if (IsBackgroundLayerHistoryMode()) {
        m_fg_layer_undo_stack.clear();
        m_fg_layer_redo_stack.clear();
    } else if (IsForegroundLayerHistoryMode()) {
        m_bg_layer_undo_stack.clear();
        m_bg_layer_redo_stack.clear();
    }
}

void GLCanvas::RestoreLayerUndoState(const LayerUndoState& state) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    if (state.full_map) {
        ResetLayerEditState();
        InvalidateCrossLayerHistory();
        RestoreUndoState(state.full_map);
        // The map dimensions may have changed; drop selection cells that now
        // fall outside the grid.
        const int width = m_mapRenderer.GetRoomWidth();
        const int height = m_mapRenderer.GetRoomHeight();
        for (auto it = m_layer_selected_cells.begin(); it != m_layer_selected_cells.end(); ) {
            if (it->first < 0 || it->second < 0 || it->first >= width || it->second >= height) {
                it = m_layer_selected_cells.erase(it);
            } else {
                ++it;
            }
        }
        return;
    }

    int count = map->GetWidth() * map->GetHeight();
    int restore_count = std::min<int>(count, static_cast<int>(state.blocks.size()));
    for (int i = 0; i < restore_count; ++i) {
        map->SetBlock(state.blocks[static_cast<std::size_t>(i)], static_cast<uint16_t>(i), state.layer);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetBlock(state.blocks[static_cast<std::size_t>(i)], static_cast<uint16_t>(i), state.layer);
        }
    }

    ResetLayerEditState();

    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    NotifyLayerBlockSelected();
    UpdateStatusBar();
    Refresh();
}

std::vector<Entity> GLCanvas::BuildCurrentRoomEntities() const {
    if (m_instances.empty()) {
        return m_room_entities;
    }

    std::vector<Entity> entities(m_instances.size());
    for (const auto& inst : m_instances) {
        std::size_t idx = inst.instance_id > 0 ? std::size_t(inst.instance_id - 1) : entities.size();
        if (idx >= entities.size()) {
            continue;
        }
        Entity entity = idx < m_room_entities.size() ? m_room_entities[idx] : Entity{};
        entity.SetType(inst.entity_id);
        entity.SetPalette(std::min<uint8_t>(inst.palette, 3));
        entity.SetOrientation(inst.orientation);
        entity.SetXDbl(inst.map_x);
        entity.SetYDbl(inst.map_y);
        entity.SetZDbl(inst.map_z);
        entities[idx] = entity;
    }
    return entities;
}

std::vector<WarpList::Warp> GLCanvas::BuildCurrentRoomWarps() const {
    std::vector<WarpList::Warp> warps;
    std::map<uint32_t, std::size_t> warp_slots;
    for (const auto& inst : m_warps) {
        uint32_t key = inst.warp_key != 0 ? inst.warp_key : inst.instance_id;
        auto slot_it = warp_slots.find(key);
        if (slot_it == warp_slots.end()) {
            warp_slots[key] = warps.size();
            warps.push_back(inst.warp);
            slot_it = warp_slots.find(key);
        }
        WarpList::Warp& warp = warps[slot_it->second];
        if (inst.current_room_is_room1) {
            warp.room1 = m_current_room;
            warp.x1 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.x)), 0, 63));
            warp.y1 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.y)), 0, 63));
        } else {
            warp.room2 = m_current_room;
            warp.x2 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.x)), 0, 63));
            warp.y2 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.y)), 0, 63));
        }
        warp.x_size = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.width)), 1, 63));
        warp.y_size = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.height)), 1, 63));
    }
    warps.erase(
        std::remove_if(
            warps.begin(),
            warps.end(),
            [](const auto& warp) {
                return warp.room1 == 0xFFFF || warp.room2 == 0xFFFF || !warp.IsValid();
            }),
        warps.end());
    return warps;
}

GLCanvas::ObjectUndoState GLCanvas::BuildObjectUndoState() const {
    auto rd = m_gd ? m_gd->GetRoomData() : nullptr;
    ObjectUndoState state{};
    state.entities = BuildCurrentRoomEntities();
    state.warps = BuildCurrentRoomWarps();
    state.swaps = rd ? rd->GetTileSwaps(m_current_room) : std::vector<TileSwap>{};
    state.doors = rd ? rd->GetDoors(m_current_room) : std::vector<Door>{};
    state.pending_warp_half = m_pending_warp_half;
    state.pending_warp_room = m_pending_warp_room;
    state.pending_warp_instance_id = m_pending_warp_instance_id;
    state.pending_warp = m_pending_warp;
    state.selected_entity = SelectedEntityListIndex();
    state.selected_warp = SelectedWarpListIndex();
    state.selected_tileswap = SelectedTileSwapListIndex();
    state.selected_door = SelectedDoorListIndex();
    return state;
}

void GLCanvas::CaptureObjectUndoState() {
    if (!m_gd) {
        return;
    }

    m_object_undo_stack.push_back(BuildObjectUndoState());
    if (m_object_undo_stack.size() > kMaxUndoStates) {
        m_object_undo_stack.erase(m_object_undo_stack.begin());
    }
    m_object_redo_stack.clear();
    NotifyHeightmapTargetChanged();
}

void GLCanvas::RestoreObjectUndoState(const ObjectUndoState& state) {
    auto sd = m_gd ? m_gd->GetSpriteData() : nullptr;
    auto rd = m_gd ? m_gd->GetRoomData() : nullptr;
    if (!sd || !rd) {
        return;
    }

    sd->SetRoomEntities(m_current_room, state.entities);
    rd->SetWarpsForRoom(m_current_room, state.warps);
    rd->SetTileSwaps(m_current_room, state.swaps);
    rd->SetDoors(m_current_room, state.doors);

    m_restoring_history = true;
    LoadRoomFromGameData(m_current_room, false, false);
    m_restoring_history = false;

    m_pending_warp_half = state.pending_warp_half;
    m_pending_warp_room = state.pending_warp_room;
    m_pending_warp_instance_id = state.pending_warp_instance_id;
    m_pending_warp = state.pending_warp;
    if (m_pending_warp_half && m_pending_warp_room == m_current_room) {
        uint32_t instance_id = m_pending_warp_instance_id != 0
            ? m_pending_warp_instance_id
            : static_cast<uint32_t>(m_warps.size() + 1);
        WarpInstance inst = GLCanvasObjectSupport::MakeWarpInstance(
            m_pending_warp,
            m_current_room,
            instance_id,
            float(m_mapRenderer.GetRoomLeft()),
            float(m_mapRenderer.GetRoomTop()),
            m_heightmapRenderer.GetZExtent());
        UpdateWarpFloor(inst);
        m_warps.push_back(inst);
    }

    ClearObjectSelection();
    if (state.selected_entity > 0) {
        SelectEntityByIndex(state.selected_entity);
    } else if (state.selected_warp > 0) {
        SelectWarpByIndex(state.selected_warp);
    } else if (state.selected_tileswap > 0) {
        SelectTileSwapByIndex(state.selected_tileswap);
    } else if (state.selected_door > 0) {
        SelectDoorByIndex(state.selected_door);
    }

    NotifyRoomDataChanged(true, true, true, true);
    NotifySelectionChanged();
    UpdateStatusBar();
    Refresh();
}

void GLCanvas::ClearUndoRedoHistory() {
    m_map_undo_stack.clear();
    m_map_redo_stack.clear();
    m_bg_layer_undo_stack.clear();
    m_bg_layer_redo_stack.clear();
    m_fg_layer_undo_stack.clear();
    m_fg_layer_redo_stack.clear();
    m_object_undo_stack.clear();
    m_object_redo_stack.clear();
    NotifyHeightmapTargetChanged();
}

void GLCanvas::Undo() {
    if (!CanUndo()) {
        return;
    }

    if (IsObjectHistoryMode()) {
        m_object_redo_stack.push_back(BuildObjectUndoState());
        auto previous = m_object_undo_stack.back();
        m_object_undo_stack.pop_back();
        RestoreObjectUndoState(previous);
    } else if (IsBackgroundLayerHistoryMode()) {
        auto previous = m_bg_layer_undo_stack.back();
        m_bg_layer_undo_stack.pop_back();
        // Undoing a structural edit needs a structural redo snapshot too.
        m_bg_layer_redo_stack.push_back(previous.full_map
            ? BuildFullMapLayerUndoState(Tilemap3D::Layer::BG)
            : BuildLayerUndoState(Tilemap3D::Layer::BG));
        RestoreLayerUndoState(previous);
    } else if (IsForegroundLayerHistoryMode()) {
        auto previous = m_fg_layer_undo_stack.back();
        m_fg_layer_undo_stack.pop_back();
        m_fg_layer_redo_stack.push_back(previous.full_map
            ? BuildFullMapLayerUndoState(Tilemap3D::Layer::FG)
            : BuildLayerUndoState(Tilemap3D::Layer::FG));
        RestoreLayerUndoState(previous);
    } else {
        auto map = CurrentRoomMap();
        if (!map) {
            return;
        }
        m_map_redo_stack.push_back(std::make_shared<Tilemap3D>(*map));
        auto previous = m_map_undo_stack.back();
        m_map_undo_stack.pop_back();
        RestoreUndoState(previous);
    }
    NotifyHeightmapTargetChanged();
}

void GLCanvas::Redo() {
    if (!CanRedo()) {
        return;
    }

    if (IsObjectHistoryMode()) {
        m_object_undo_stack.push_back(BuildObjectUndoState());
        if (m_object_undo_stack.size() > kMaxUndoStates) {
            m_object_undo_stack.erase(m_object_undo_stack.begin());
        }
        auto next = m_object_redo_stack.back();
        m_object_redo_stack.pop_back();
        RestoreObjectUndoState(next);
    } else if (IsBackgroundLayerHistoryMode()) {
        auto next = m_bg_layer_redo_stack.back();
        m_bg_layer_redo_stack.pop_back();
        // Redoing a structural edit needs a structural undo snapshot too.
        m_bg_layer_undo_stack.push_back(next.full_map
            ? BuildFullMapLayerUndoState(Tilemap3D::Layer::BG)
            : BuildLayerUndoState(Tilemap3D::Layer::BG));
        if (m_bg_layer_undo_stack.size() > kMaxUndoStates) {
            m_bg_layer_undo_stack.erase(m_bg_layer_undo_stack.begin());
        }
        RestoreLayerUndoState(next);
    } else if (IsForegroundLayerHistoryMode()) {
        auto next = m_fg_layer_redo_stack.back();
        m_fg_layer_redo_stack.pop_back();
        m_fg_layer_undo_stack.push_back(next.full_map
            ? BuildFullMapLayerUndoState(Tilemap3D::Layer::FG)
            : BuildLayerUndoState(Tilemap3D::Layer::FG));
        if (m_fg_layer_undo_stack.size() > kMaxUndoStates) {
            m_fg_layer_undo_stack.erase(m_fg_layer_undo_stack.begin());
        }
        RestoreLayerUndoState(next);
    } else {
        auto map = CurrentRoomMap();
        if (!map) {
            return;
        }
        m_map_undo_stack.push_back(std::make_shared<Tilemap3D>(*map));
        if (m_map_undo_stack.size() > kMaxUndoStates) {
            m_map_undo_stack.erase(m_map_undo_stack.begin());
        }
        auto next = m_map_redo_stack.back();
        m_map_redo_stack.pop_back();
        RestoreUndoState(next);
    }
    NotifyHeightmapTargetChanged();
}
