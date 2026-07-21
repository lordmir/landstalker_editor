#include "GLCanvasObjectCoordinator.h"

#include <wx/msgdlg.h>

#include <algorithm>
#include <cmath>

#include "GLCanvasObjectSupport.h"
#include "RoomProjection.h"
#include <rooms/EntityControlFrame.h>
#include <rooms/RoomViewerFrame.h>
#include <rooms/TileSwapControlFrame.h>
#include <rooms/WarpControlFrame.h>

using GLCanvasObjectSupport::TileSwapRegionPart;
using RoomProjection::PickPoint;
using RoomProjection::ProjectEntityGridPoint;
using RoomProjection::ProjectWarpGridPoint;

// MyGLCanvas keeps a small room-control-facing API, while all object-selection
// behavior is implemented by the room object's coordinator.
void MyGLCanvas::ClearObjectSelection()
{
	GLCanvasObjectCoordinator(*this).ClearSelection();
}

void MyGLCanvas::SelectEntityByIndex(int selection)
{
	GLCanvasObjectCoordinator(*this).SelectEntityByIndex(selection);
}

void MyGLCanvas::SelectWarpByIndex(int selection)
{
	GLCanvasObjectCoordinator(*this).SelectWarpByIndex(selection);
}

void MyGLCanvas::SelectTileSwapByIndex(int selection)
{
	GLCanvasObjectCoordinator(*this).SelectTileSwapByIndex(selection);
}

void MyGLCanvas::SelectDoorByIndex(int selection)
{
	GLCanvasObjectCoordinator(*this).SelectDoorByIndex(selection);
}

int MyGLCanvas::SelectedEntityListIndex() const
{
	return GLCanvasObjectCoordinator::SelectedEntityListIndex(*this);
}

int MyGLCanvas::SelectedWarpListIndex() const
{
	return GLCanvasObjectCoordinator::SelectedWarpListIndex(*this);
}

int MyGLCanvas::SelectedTileSwapListIndex() const
{
	return GLCanvasObjectCoordinator::SelectedTileSwapListIndex(*this);
}

int MyGLCanvas::SelectedDoorListIndex() const
{
	return GLCanvasObjectCoordinator::SelectedDoorListIndex(*this);
}

void MyGLCanvas::NotifySelectionChanged()
{
	GLCanvasObjectCoordinator(*this).NotifySelectionChanged();
}

void MyGLCanvas::NotifyRoomDataChanged(bool entities, bool warps, bool swaps, bool doors)
{
	GLCanvasObjectCoordinator(*this).NotifyRoomDataChanged(entities, warps, swaps, doors);
}

void MyGLCanvas::RefreshObjectPlacementsFromHeightmap()
{
	GLCanvasObjectCoordinator(*this).RefreshPlacementsFromHeightmap();
}

void MyGLCanvas::PersistCurrentRoomEdits()
{
	GLCanvasObjectCoordinator(*this).PersistCurrentRoomEdits();
}

GLCanvasObjectCoordinator::GLCanvasObjectCoordinator(MyGLCanvas& canvas)
	: m_canvas(canvas)
{
}

void GLCanvasObjectCoordinator::ClearSelection()
{
	m_canvas.m_hovered_entity_idx = -1;
	m_canvas.m_selected_entity_idx = -1;
	m_canvas.m_hovered_warp_idx = -1;
	m_canvas.m_selected_warp_idx = -1;
	m_canvas.m_hovered_tileswap_region_idx = -1;
	m_canvas.m_selected_tileswap_region_idx = -1;
	m_canvas.m_hovered_door_idx = -1;
	m_canvas.m_selected_door_idx = -1;
}

void GLCanvasObjectCoordinator::PrepareForRoomLoad()
{
	m_canvas.CancelPendingObjectAdd();
	m_canvas.m_tileswap_preview_active = false;
	m_canvas.m_tileswap_preview_swap_index = -1;
	m_canvas.m_door_preview_active = false;
	m_canvas.m_door_preview_idx = -1;
	m_canvas.m_tileswap_preview_map.reset();
	m_canvas.m_heightmapRenderer.ClearPreviewMap();
	m_canvas.m_instances.clear();
	m_canvas.m_warps.clear();
	ClearSelection();
}

void GLCanvasObjectCoordinator::LoadRoomObjects(uint16_t roomnum)
{
	auto sprite_data = m_canvas.m_gd->GetSpriteData();
	auto entities = sprite_data->GetRoomEntities(roomnum);
	m_canvas.m_room_entities = entities;
	float mat[9] = {32.0f, 16.0f, 0.0f, -32.0f, 16.0f, 0.0f, 512.0f, 100.0f, 1.0f};
	uint32_t instance_id = 1;
	for (const auto& entity : entities) {
		float entity_x = float(entity.GetXDbl());
		float entity_y = float(entity.GetYDbl());
		float entity_z = float(entity.GetZDbl());
		float hitbox_base = 1.0f;
		float hitbox_height = 1.0f;
		if (sprite_data->IsEntity(entity.GetType())) {
			auto hitbox = sprite_data->GetEntityHitbox(entity.GetType());
			hitbox_base = GLCanvasObjectSupport::HitboxBaseToBlocks(hitbox.base);
			hitbox_height = GLCanvasObjectSupport::HitboxHeightToBlocks(hitbox.height);
		}
		float hitbox_offset = GLCanvasObjectSupport::HitboxDrawOffset(hitbox_base);
		float floor_z = m_canvas.FloorUnderHitbox(
			entity_x + hitbox_offset,
			entity_y + hitbox_offset,
			hitbox_base * 0.5f);
		float ex_block = entity_x + hitbox_offset - m_canvas.m_mapRenderer.GetRoomLeft();
		float ey_block = entity_y + hitbox_offset - m_canvas.m_mapRenderer.GetRoomTop();
		float px = mat[0] * ex_block + mat[3] * ey_block + mat[6];
		float py = mat[1] * ex_block + mat[4] * ey_block + mat[7] - entity_z * 32.0f;

		SpriteInstance inst{};
		inst.instance_id = instance_id++;
		inst.entity_id = entity.GetType();
		inst.palette = entity.GetPalette();
		inst.x = px;
		inst.y = py;
		inst.map_x = entity_x;
		inst.map_y = entity_y;
		inst.map_z = entity_z;
		inst.floor_z = floor_z;
		inst.z_extent = m_canvas.m_heightmapRenderer.GetZExtent();
		inst.hitbox_base = hitbox_base;
		inst.hitbox_height = hitbox_height;
		inst.hitbox_offset = hitbox_offset;
		inst.room_left = float(m_canvas.m_mapRenderer.GetRoomLeft());
		inst.room_top = float(m_canvas.m_mapRenderer.GetRoomTop());
		inst.dx = 0.0f;
		inst.dy = 0.0f;
		inst.scale = 2.0f;
		inst.anim_timer = 0.0f;
		inst.anim_speed = 1.0f;
		inst.orientation = entity.GetOrientation();
		m_canvas.m_instances.push_back(inst);
	}

	uint32_t warp_instance_id = 1;
	uint32_t warp_key = 1;
	for (const auto& warp : m_canvas.m_gd->GetRoomData()->GetWarpsForRoom(roomnum)) {
		WarpInstance inst = GLCanvasObjectSupport::MakeWarpInstance(
			warp,
			roomnum,
			warp_instance_id++,
			float(m_canvas.m_mapRenderer.GetRoomLeft()),
			float(m_canvas.m_mapRenderer.GetRoomTop()),
			m_canvas.m_heightmapRenderer.GetZExtent(),
			warp_key);
		m_canvas.UpdateWarpFloor(inst);
		m_canvas.m_warps.push_back(inst);
		if (warp.room1 == roomnum && warp.room2 == roomnum && warp.IsValid()) {
			WarpInstance dest_inst = GLCanvasObjectSupport::MakeWarpInstance(
				warp,
				roomnum,
				warp_instance_id++,
				float(m_canvas.m_mapRenderer.GetRoomLeft()),
				float(m_canvas.m_mapRenderer.GetRoomTop()),
				m_canvas.m_heightmapRenderer.GetZExtent(),
				warp_key,
				2);
			m_canvas.UpdateWarpFloor(dest_inst);
			m_canvas.m_warps.push_back(dest_inst);
		}
		++warp_key;
	}
	if (m_canvas.m_pending_warp_half && m_canvas.m_pending_warp_room == roomnum) {
		m_canvas.m_pending_warp_instance_id = warp_instance_id++;
		WarpInstance inst = GLCanvasObjectSupport::MakeWarpInstance(
			m_canvas.m_pending_warp,
			roomnum,
			m_canvas.m_pending_warp_instance_id,
			float(m_canvas.m_mapRenderer.GetRoomLeft()),
			float(m_canvas.m_mapRenderer.GetRoomTop()),
			m_canvas.m_heightmapRenderer.GetZExtent(),
			warp_key);
		m_canvas.UpdateWarpFloor(inst);
		m_canvas.m_warps.push_back(inst);
	}

	GLCanvasObjectSupport::SortEntitiesGeometrically(m_canvas.m_instances);
}

void GLCanvasObjectCoordinator::RefreshPlacementsFromHeightmap()
{
	for (auto& inst : m_canvas.m_instances) {
		inst.z_extent = m_canvas.m_heightmapRenderer.GetZExtent();
		inst.floor_z = m_canvas.FloorUnderHitbox(
			inst.map_x + inst.hitbox_offset,
			inst.map_y + inst.hitbox_offset,
			inst.hitbox_base * 0.5f);
		m_canvas.UpdateEntityProjection(inst);
	}

	for (auto& warp : m_canvas.m_warps) {
		warp.z_extent = m_canvas.m_heightmapRenderer.GetZExtent();
		m_canvas.UpdateWarpFloor(warp);
	}
}

void GLCanvasObjectCoordinator::PersistCurrentRoomEdits()
{
	if (m_canvas.m_room_entities.empty() && m_canvas.m_instances.empty() && m_canvas.m_warps.empty()) {
		return;
	}

	const auto warps = m_canvas.BuildCurrentRoomWarps();
	if (Landstalker::WarpList::HasDuplicateWarps(warps)) {
		wxMessageBox("That connection already exists. Warp direction does not create a distinct warp.",
			"Duplicate Warp", wxOK | wxICON_ERROR, &m_canvas);
		m_canvas.ReloadCurrentRoomFromGameData();
		return;
	}

	std::vector<Landstalker::Entity> entities = m_canvas.BuildCurrentRoomEntities();
	m_canvas.m_gd->GetSpriteData()->SetRoomEntities(m_canvas.m_current_room, entities);
	m_canvas.m_room_entities = entities;

	// A pending warp half is not persisted (it has no destination yet), so its
	// latest editor position/size must be captured before it is filtered out.
	if (m_canvas.m_pending_warp_half && m_canvas.m_pending_warp_room == m_canvas.m_current_room) {
		int pending_idx = m_canvas.FindWarpIndex(m_canvas.m_pending_warp_instance_id);
		if (pending_idx >= 0) {
			const WarpInstance& inst = m_canvas.m_warps[static_cast<std::size_t>(pending_idx)];
			if (inst.DestinationRoom() == 0xFFFF) {
				Landstalker::WarpList::Warp warp = inst.warp;
				uint8_t x = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.x)), 0, 63));
				uint8_t y = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.y)), 0, 63));
				if (inst.current_room_is_room1) {
					warp.room1 = m_canvas.m_current_room;
					warp.x1 = x;
					warp.y1 = y;
				} else {
					warp.room2 = m_canvas.m_current_room;
					warp.x2 = x;
					warp.y2 = y;
				}
				warp.x_size = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.width)), 1, 63));
				warp.y_size = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(inst.height)), 1, 63));
				m_canvas.m_pending_warp = warp;
			}
		}
	}

	m_canvas.m_gd->GetRoomData()->SetWarpsForRoom(
		m_canvas.m_current_room,
		warps);
}

void GLCanvasObjectCoordinator::SelectEntityByIndex(int selection)
{
	ClearSelection();
	int idx = m_canvas.FindInstanceIndex(static_cast<uint32_t>(selection));
	if (idx >= 0) {
		m_canvas.m_selected_entity_idx = idx;
		FocusCameraOnSelectedObjectIfNeeded();
	}
	m_canvas.UpdateStatusBar();
	m_canvas.Refresh();
}

void GLCanvasObjectCoordinator::SelectWarpByIndex(int selection)
{
	ClearSelection();
	if (selection > 0) {
		for (std::size_t i = 0; i < m_canvas.m_warps.size(); ++i) {
			if (m_canvas.m_warps[i].warp_key == static_cast<uint32_t>(selection)) {
				m_canvas.m_selected_warp_idx = static_cast<int>(i);
				break;
			}
		}
		FocusCameraOnSelectedObjectIfNeeded();
	}
	m_canvas.UpdateStatusBar();
	m_canvas.Refresh();
}

void GLCanvasObjectCoordinator::SelectTileSwapByIndex(int selection)
{
	ClearSelection();
	int swap_idx = selection - 1;
	if (swap_idx >= 0) {
		auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(
			m_canvas.m_gd,
			m_canvas.m_current_room,
			m_canvas.m_mapRenderer,
			m_canvas.m_heightmapRenderer.GetZExtent());
		for (const auto& region : regions) {
			if (region.swap_index == swap_idx) {
				m_canvas.m_selected_tileswap_region_idx = region.flat_index;
				break;
			}
		}
		FocusCameraOnSelectedObjectIfNeeded();
	}
	m_canvas.UpdateStatusBar();
	m_canvas.Refresh();
}

void GLCanvasObjectCoordinator::SelectDoorByIndex(int selection)
{
	ClearSelection();
	int door_idx = selection - 1;
	if (door_idx >= 0) {
		auto doors = GLCanvasObjectSupport::BuildDoorGeometries(
			m_canvas.m_gd,
			m_canvas.m_current_room,
			m_canvas.m_mapRenderer,
			m_canvas.m_heightmapRenderer.GetZExtent(),
			m_canvas.m_tileswap_preview_map);
		for (const auto& door : doors) {
			if (door.index == door_idx) {
				m_canvas.m_selected_door_idx = door.index;
				break;
			}
		}
		FocusCameraOnSelectedObjectIfNeeded();
	}
	m_canvas.UpdateStatusBar();
	m_canvas.Refresh();
}

int GLCanvasObjectCoordinator::SelectedEntityListIndex(const MyGLCanvas& canvas)
{
	if (canvas.m_selected_entity_idx >= 0 &&
		canvas.m_selected_entity_idx < static_cast<int>(canvas.m_instances.size())) {
		return static_cast<int>(canvas.m_instances[static_cast<std::size_t>(canvas.m_selected_entity_idx)].instance_id);
	}
	return -1;
}

int GLCanvasObjectCoordinator::SelectedWarpListIndex(const MyGLCanvas& canvas)
{
	if (canvas.m_selected_warp_idx >= 0 &&
		canvas.m_selected_warp_idx < static_cast<int>(canvas.m_warps.size())) {
		return static_cast<int>(canvas.m_warps[static_cast<std::size_t>(canvas.m_selected_warp_idx)].warp_key);
	}
	return -1;
}

int GLCanvasObjectCoordinator::SelectedTileSwapListIndex(const MyGLCanvas& canvas)
{
	if (canvas.m_selected_tileswap_region_idx >= 0) {
		auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(
			canvas.m_gd,
			canvas.m_current_room,
			canvas.m_mapRenderer,
			canvas.m_heightmapRenderer.GetZExtent());
		if (canvas.m_selected_tileswap_region_idx < static_cast<int>(regions.size())) {
			return regions[static_cast<std::size_t>(canvas.m_selected_tileswap_region_idx)].swap_index + 1;
		}
	}
	return -1;
}

int GLCanvasObjectCoordinator::SelectedDoorListIndex(const MyGLCanvas& canvas)
{
	return canvas.m_selected_door_idx >= 0 ? canvas.m_selected_door_idx + 1 : -1;
}

bool GLCanvasObjectCoordinator::SelectAt(const wxPoint& point)
{
	int entity_idx = m_canvas.HitTestEntityZControl(point);
	if (entity_idx < 0) {
		entity_idx = m_canvas.HitTestEntityBody(point);
	}
	if (entity_idx < 0) {
		entity_idx = m_canvas.HitTestEntity(point);
	}
	if (entity_idx >= 0) {
		ClearSelection();
		m_canvas.m_selected_entity_idx = entity_idx;
		return true;
	}

	int warp_idx = m_canvas.HitTestWarp(point);
	if (warp_idx >= 0) {
		ClearSelection();
		m_canvas.m_selected_warp_idx = warp_idx;
		return true;
	}

	int tileswap_idx = m_canvas.HitTestTileSwapRegion(point);
	if (tileswap_idx >= 0) {
		ClearSelection();
		m_canvas.m_selected_tileswap_region_idx = tileswap_idx;
		m_canvas.m_hovered_tileswap_region_idx = tileswap_idx;
		return true;
	}

	int door_idx = m_canvas.HitTestDoor(point);
	if (door_idx >= 0) {
		ClearSelection();
		m_canvas.m_selected_door_idx = door_idx;
		m_canvas.m_hovered_door_idx = door_idx;
		return true;
	}

	return false;
}

bool GLCanvasObjectCoordinator::OpenSelectedProperties()
{
	wxWindow* target = m_canvas.EventTarget();
	if (!target) {
		return false;
	}

	auto post_open = [&](const wxEventType& event_type, int selection) {
		wxCommandEvent evt(event_type);
		evt.SetInt(selection);
		evt.SetExtraLong(selection);
		evt.SetClientData(&m_canvas);
		wxPostEvent(target, evt);
	};

	int selection = SelectedEntityListIndex(m_canvas);
	if (selection > 0) {
		m_canvas.CommitPendingEdits();
		post_open(EVT_ENTITY_OPEN_PROPERTIES, selection);
		return true;
	}

	selection = SelectedWarpListIndex(m_canvas);
	if (selection > 0) {
		m_canvas.CommitPendingEdits();
		post_open(EVT_WARP_OPEN_PROPERTIES, selection);
		return true;
	}

	selection = SelectedTileSwapListIndex(m_canvas);
	if (selection > 0) {
		m_canvas.CommitPendingEdits();
		post_open(EVT_TILESWAP_OPEN_PROPERTIES, selection);
		return true;
	}

	selection = SelectedDoorListIndex(m_canvas);
	if (selection > 0) {
		m_canvas.CommitPendingEdits();
		post_open(EVT_DOOR_OPEN_PROPERTIES, selection);
		return true;
	}

	return false;
}

void GLCanvasObjectCoordinator::NotifySelectionChanged()
{
	wxWindow* target = m_canvas.EventTarget();
	if (!target) {
		return;
	}

	auto post_selection = [&](const wxEventType& event_type, int selection) {
		wxCommandEvent evt(event_type);
		evt.SetInt(selection);
		evt.SetExtraLong(selection);
		evt.SetClientData(&m_canvas);
		wxPostEvent(target, evt);
	};

	int selection = SelectedEntityListIndex(m_canvas);
	if (selection > 0) {
		post_selection(EVT_ENTITY_SELECT, selection);
		return;
	}

	selection = SelectedWarpListIndex(m_canvas);
	if (selection > 0) {
		post_selection(EVT_WARP_SELECT, selection);
		return;
	}

	selection = SelectedTileSwapListIndex(m_canvas);
	if (selection > 0) {
		post_selection(EVT_TILESWAP_SELECT, selection);
		return;
	}

	selection = SelectedDoorListIndex(m_canvas);
	if (selection > 0) {
		post_selection(EVT_DOOR_SELECT, selection);
	}
}

void GLCanvasObjectCoordinator::NotifyRoomDataChanged(bool entities, bool warps, bool swaps, bool doors)
{
	m_canvas.CommitPendingEdits();
	wxWindow* target = m_canvas.EventTarget();
	if (!target) {
		return;
	}

	auto post_update = [&](const wxEventType& event_type, int selection) {
		wxCommandEvent evt(event_type);
		evt.SetInt(selection);
		evt.SetExtraLong(selection);
		evt.SetClientData(&m_canvas);
		wxPostEvent(target, evt);
	};

	if (entities) {
		post_update(EVT_ENTITY_UPDATE, SelectedEntityListIndex(m_canvas));
	}
	if (warps) {
		post_update(EVT_WARP_UPDATE, SelectedWarpListIndex(m_canvas));
	}
	if (swaps) {
		post_update(EVT_TILESWAP_UPDATE, SelectedTileSwapListIndex(m_canvas));
	}
	if (doors) {
		post_update(EVT_DOOR_UPDATE, SelectedDoorListIndex(m_canvas));
	}
}

void GLCanvasObjectCoordinator::FocusCameraOnSelectedObjectIfNeeded()
{
	if (m_canvas.m_selected_entity_idx >= 0 &&
		m_canvas.m_selected_entity_idx < static_cast<int>(m_canvas.m_instances.size())) {
		const SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)];
		float center_x = inst.map_x + inst.hitbox_offset;
		float center_y = inst.map_y + inst.hitbox_offset;
		float half_base = std::max(inst.hitbox_base * 0.5f, 0.5f);
		float top_z = inst.map_z + std::max(inst.hitbox_height, 0.125f);
		PickPoint p0 = ProjectEntityGridPoint(inst, center_x - half_base, center_y - half_base, top_z);
		PickPoint p1 = ProjectEntityGridPoint(inst, center_x + half_base, center_y + half_base, inst.map_z);
		m_canvas.EnsureWorldRectVisible(
			std::min(p0.x, p1.x),
			std::min(p0.y, p1.y),
			std::max(p0.x, p1.x),
			std::max(p0.y, p1.y));
		return;
	}

	if (m_canvas.m_selected_warp_idx >= 0 &&
		m_canvas.m_selected_warp_idx < static_cast<int>(m_canvas.m_warps.size())) {
		const WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)];
		float z = warp.floor_z;
		PickPoint p0 = ProjectWarpGridPoint(warp, warp.x, warp.y, z);
		PickPoint p1 = ProjectWarpGridPoint(warp, warp.x + warp.width, warp.y + warp.height, z);
		m_canvas.EnsureWorldRectVisible(
			std::min(p0.x, p1.x),
			std::min(p0.y, p1.y),
			std::max(p0.x, p1.x),
			std::max(p0.y, p1.y));
		return;
	}

	if (m_canvas.m_selected_tileswap_region_idx >= 0) {
		auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(
			m_canvas.m_gd,
			m_canvas.m_current_room,
			m_canvas.m_mapRenderer,
			m_canvas.m_heightmapRenderer.GetZExtent());
		if (m_canvas.m_selected_tileswap_region_idx < static_cast<int>(regions.size())) {
			const auto& region = regions[static_cast<std::size_t>(m_canvas.m_selected_tileswap_region_idx)];
			m_canvas.EnsureWorldRectVisible(
				region.bounds.min_x,
				region.bounds.min_y,
				region.bounds.max_x,
				region.bounds.max_y);
		}
		return;
	}

	if (m_canvas.m_selected_door_idx >= 0) {
		auto doors = GLCanvasObjectSupport::BuildDoorGeometries(
			m_canvas.m_gd,
			m_canvas.m_current_room,
			m_canvas.m_mapRenderer,
			m_canvas.m_heightmapRenderer.GetZExtent(),
			m_canvas.m_tileswap_preview_map);
		for (const auto& door : doors) {
			if (door.index == m_canvas.m_selected_door_idx) {
				m_canvas.EnsureWorldRectVisible(
					door.bounds.min_x,
					door.bounds.min_y,
					door.bounds.max_x,
					door.bounds.max_y);
				break;
			}
		}
	}
}

void GLCanvasObjectCoordinator::DeleteSelectedObject()
{
	if (m_canvas.m_selected_entity_idx >= 0 && m_canvas.m_selected_entity_idx < static_cast<int>(m_canvas.m_instances.size())) {
		uint32_t id = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)].instance_id;
		std::size_t slot = id > 0 ? std::size_t(id - 1) : m_canvas.m_room_entities.size();
		if (slot < m_canvas.m_room_entities.size()) {
			m_canvas.m_room_entities.erase(m_canvas.m_room_entities.begin() + static_cast<std::ptrdiff_t>(slot));
		}
		m_canvas.m_instances.erase(m_canvas.m_instances.begin() + m_canvas.m_selected_entity_idx);
		for (auto& inst : m_canvas.m_instances) {
			if (inst.instance_id > id) {
				--inst.instance_id;
			}
		}
		m_canvas.m_selected_entity_idx = -1;
		m_canvas.m_hovered_entity_idx = -1;
		return;
	}

	if (m_canvas.m_selected_warp_idx >= 0 && m_canvas.m_selected_warp_idx < static_cast<int>(m_canvas.m_warps.size())) {
		const WarpInstance& selected = m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)];
		if (m_canvas.m_pending_warp_half &&
			m_canvas.m_current_room == m_canvas.m_pending_warp_room &&
			selected.instance_id == m_canvas.m_pending_warp_instance_id &&
			selected.DestinationRoom() == 0xFFFF) {
			m_canvas.m_pending_warp_half = false;
			m_canvas.m_pending_warp_room = 0xFFFF;
			m_canvas.m_pending_warp_instance_id = 0;
		}
		m_canvas.m_warps.erase(m_canvas.m_warps.begin() + m_canvas.m_selected_warp_idx);
		for (std::size_t i = 0; i < m_canvas.m_warps.size(); ++i) {
			m_canvas.m_warps[i].instance_id = static_cast<uint32_t>(i + 1);
		}
		m_canvas.m_selected_warp_idx = -1;
		m_canvas.m_hovered_warp_idx = -1;
		return;
	}

	if (m_canvas.m_selected_tileswap_region_idx >= 0) {
		m_canvas.ClearTileSwapPreview();
		auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(m_canvas.m_gd, m_canvas.m_current_room, m_canvas.m_mapRenderer, m_canvas.m_heightmapRenderer.GetZExtent());
		if (m_canvas.m_selected_tileswap_region_idx >= static_cast<int>(regions.size())) {
			m_canvas.m_selected_tileswap_region_idx = -1;
			m_canvas.m_hovered_tileswap_region_idx = -1;
			return;
		}
		int swap_index = regions[static_cast<std::size_t>(m_canvas.m_selected_tileswap_region_idx)].swap_index;
		auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
		if (!rd) {
			return;
		}
		auto swaps = rd->GetTileSwaps(m_canvas.m_current_room);
		if (swap_index >= 0 && swap_index < static_cast<int>(swaps.size())) {
			swaps.erase(swaps.begin() + swap_index);
			rd->SetTileSwaps(m_canvas.m_current_room, swaps);
		}
		m_canvas.m_selected_tileswap_region_idx = -1;
		m_canvas.m_hovered_tileswap_region_idx = -1;
		return;
	}

	if (m_canvas.m_selected_door_idx >= 0) {
		m_canvas.ClearTileSwapPreview();
		auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
		if (!rd) {
			return;
		}
		auto doors = rd->GetDoors(m_canvas.m_current_room);
		if (m_canvas.m_selected_door_idx >= 0 && m_canvas.m_selected_door_idx < static_cast<int>(doors.size())) {
			doors.erase(doors.begin() + m_canvas.m_selected_door_idx);
			rd->SetDoors(m_canvas.m_current_room, doors);
		}
		m_canvas.m_selected_door_idx = -1;
		m_canvas.m_hovered_door_idx = -1;
	}
}

void GLCanvasObjectCoordinator::ReorderSelectedObject(int delta)
{
	if (m_canvas.m_selected_entity_idx >= 0 && m_canvas.m_selected_entity_idx < static_cast<int>(m_canvas.m_instances.size())) {
		uint32_t id = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)].instance_id;
		int slot = static_cast<int>(id) - 1;
		int new_slot = std::clamp(slot + delta, 0, static_cast<int>(m_canvas.m_room_entities.size()) - 1);
		if (new_slot == slot) {
			return;
		}
		std::swap(m_canvas.m_room_entities[static_cast<std::size_t>(slot)], m_canvas.m_room_entities[static_cast<std::size_t>(new_slot)]);
		for (auto& inst : m_canvas.m_instances) {
			if (inst.instance_id == static_cast<uint32_t>(slot + 1)) {
				inst.instance_id = static_cast<uint32_t>(new_slot + 1);
			} else if (inst.instance_id == static_cast<uint32_t>(new_slot + 1)) {
				inst.instance_id = static_cast<uint32_t>(slot + 1);
			}
		}
		m_canvas.m_selected_entity_idx = m_canvas.FindInstanceIndex(static_cast<uint32_t>(new_slot + 1));
		return;
	}

	if (m_canvas.m_selected_warp_idx >= 0 && m_canvas.m_selected_warp_idx < static_cast<int>(m_canvas.m_warps.size())) {
		int new_idx = std::clamp(m_canvas.m_selected_warp_idx + delta, 0, static_cast<int>(m_canvas.m_warps.size()) - 1);
		if (new_idx != m_canvas.m_selected_warp_idx) {
			std::swap(m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)], m_canvas.m_warps[static_cast<std::size_t>(new_idx)]);
			m_canvas.m_selected_warp_idx = new_idx;
		}
		return;
	}

	if (m_canvas.m_selected_tileswap_region_idx >= 0) {
		auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
		if (!rd) {
			return;
		}
		auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(m_canvas.m_gd, m_canvas.m_current_room, m_canvas.m_mapRenderer, m_canvas.m_heightmapRenderer.GetZExtent());
		if (m_canvas.m_selected_tileswap_region_idx >= static_cast<int>(regions.size())) {
			m_canvas.m_selected_tileswap_region_idx = -1;
			m_canvas.m_hovered_tileswap_region_idx = -1;
			return;
		}
		const auto& region = regions[static_cast<std::size_t>(m_canvas.m_selected_tileswap_region_idx)];
		auto swaps = rd->GetTileSwaps(m_canvas.m_current_room);
		int new_idx = std::clamp(region.swap_index + delta, 0, static_cast<int>(swaps.size()) - 1);
		if (region.swap_index >= 0 && region.swap_index < static_cast<int>(swaps.size()) && new_idx != region.swap_index) {
			std::iter_swap(swaps.begin() + region.swap_index, swaps.begin() + new_idx);
			rd->SetTileSwaps(m_canvas.m_current_room, swaps);
			m_canvas.m_selected_tileswap_region_idx += (new_idx - region.swap_index) * 4;
			m_canvas.m_hovered_tileswap_region_idx = m_canvas.m_selected_tileswap_region_idx;
		}
		return;
	}

	if (m_canvas.m_selected_door_idx >= 0) {
		auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
		if (!rd) {
			return;
		}
		auto doors = rd->GetDoors(m_canvas.m_current_room);
		if (m_canvas.m_selected_door_idx >= static_cast<int>(doors.size())) {
			m_canvas.m_selected_door_idx = -1;
			m_canvas.m_hovered_door_idx = -1;
			return;
		}
		int new_idx = std::clamp(m_canvas.m_selected_door_idx + delta, 0, static_cast<int>(doors.size()) - 1);
		if (new_idx != m_canvas.m_selected_door_idx) {
			std::swap(doors[static_cast<std::size_t>(m_canvas.m_selected_door_idx)], doors[static_cast<std::size_t>(new_idx)]);
			rd->SetDoors(m_canvas.m_current_room, doors);
			m_canvas.m_selected_door_idx = new_idx;
			m_canvas.m_hovered_door_idx = new_idx;
		}
	}
}

void GLCanvasObjectCoordinator::SelectNextObject(int direction)
{
	// Build one unified tab-order across entity/warp/tile-swap/door selections.
	enum class SelectionType {
		Entity,
		Warp,
		TileSwapRegion,
		Door
	};

	struct TabEntry {
		SelectionType type;
		int index;
		int id;
		int part_order;
	};

	auto region_part_order = [](TileSwapRegionPart part) {
		switch (part) {
			case TileSwapRegionPart::TilemapSource: return 0;
			case TileSwapRegionPart::TilemapDestination: return 1;
			case TileSwapRegionPart::HeightmapSource: return 2;
			case TileSwapRegionPart::HeightmapDestination: return 3;
		}
		return 4;
	};

	auto type_order = [](SelectionType type) {
		switch (type) {
			case SelectionType::Entity: return 0;
			case SelectionType::Warp: return 1;
			case SelectionType::TileSwapRegion: return 2;
			case SelectionType::Door: return 3;
		}
		return 4;
	};

	std::vector<TabEntry> order;
	order.reserve(m_canvas.m_instances.size() + m_canvas.m_warps.size());

	for (const auto& inst : m_canvas.m_instances) {
		order.push_back({
			SelectionType::Entity,
			static_cast<int>(inst.instance_id),
			static_cast<int>(inst.instance_id),
			0
		});
	}

	for (const auto& warp : m_canvas.m_warps) {
		order.push_back({
			SelectionType::Warp,
			static_cast<int>(warp.instance_id),
			static_cast<int>(warp.instance_id),
			0
		});
	}

	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(m_canvas.m_gd, m_canvas.m_current_room, m_canvas.m_mapRenderer, m_canvas.m_heightmapRenderer.GetZExtent());
	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	std::vector<Landstalker::TileSwap> swaps = rd ? rd->GetTileSwaps(m_canvas.m_current_room) : std::vector<Landstalker::TileSwap>{};
	for (const auto& region : regions) {
		if (region.swap_index < 0 || region.swap_index >= static_cast<int>(swaps.size())) {
			continue;
		}
		const Landstalker::TileSwap& swap = swaps[static_cast<std::size_t>(region.swap_index)];
		order.push_back({
			SelectionType::TileSwapRegion,
			region.flat_index,
			static_cast<int>(swap.trigger),
			region_part_order(region.part)
		});
	}

	if (rd) {
		auto doors = rd->GetDoors(m_canvas.m_current_room);
		for (int i = 0; i < static_cast<int>(doors.size()); ++i) {
			order.push_back({SelectionType::Door, i, i + 1, 0});
		}
	}

	if (order.empty()) {
		m_canvas.m_selected_entity_idx = -1;
		m_canvas.m_selected_warp_idx = -1;
		m_canvas.m_selected_tileswap_region_idx = -1;
		m_canvas.m_selected_door_idx = -1;
		return;
	}

	std::stable_sort(order.begin(), order.end(), [&](const TabEntry& lhs, const TabEntry& rhs) {
		int lhs_type = type_order(lhs.type);
		int rhs_type = type_order(rhs.type);
		if (lhs_type != rhs_type) {
			return lhs_type < rhs_type;
		}
		if (lhs.id != rhs.id) {
			return lhs.id < rhs.id;
		}
		if (lhs.part_order != rhs.part_order) {
			return lhs.part_order < rhs.part_order;
		}
		return lhs.index < rhs.index;
	});

	int current = -1;
	if (m_canvas.m_selected_entity_idx >= 0 && m_canvas.m_selected_entity_idx < static_cast<int>(m_canvas.m_instances.size())) {
		int selected_id = static_cast<int>(m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)].instance_id);
		for (int i = 0; i < static_cast<int>(order.size()); ++i) {
			if (order[static_cast<std::size_t>(i)].type == SelectionType::Entity &&
				order[static_cast<std::size_t>(i)].id == selected_id) {
				current = i;
				break;
			}
		}
	} else if (m_canvas.m_selected_warp_idx >= 0 && m_canvas.m_selected_warp_idx < static_cast<int>(m_canvas.m_warps.size())) {
		int selected_id = static_cast<int>(m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)].instance_id);
		for (int i = 0; i < static_cast<int>(order.size()); ++i) {
			if (order[static_cast<std::size_t>(i)].type == SelectionType::Warp &&
				order[static_cast<std::size_t>(i)].id == selected_id) {
				current = i;
				break;
			}
		}
	} else if (m_canvas.m_selected_tileswap_region_idx >= 0) {
		for (int i = 0; i < static_cast<int>(order.size()); ++i) {
			if (order[static_cast<std::size_t>(i)].type == SelectionType::TileSwapRegion &&
				order[static_cast<std::size_t>(i)].index == m_canvas.m_selected_tileswap_region_idx) {
				current = i;
				break;
			}
		}
	} else if (m_canvas.m_selected_door_idx >= 0) {
		for (int i = 0; i < static_cast<int>(order.size()); ++i) {
			if (order[static_cast<std::size_t>(i)].type == SelectionType::Door &&
				order[static_cast<std::size_t>(i)].index == m_canvas.m_selected_door_idx) {
				current = i;
				break;
			}
		}
	}

	int step = direction >= 0 ? 1 : -1;
	int next = -1;
	if (current < 0) {
		next = step > 0 ? 0 : static_cast<int>(order.size()) - 1;
	} else {
		next = (current + step + static_cast<int>(order.size())) % static_cast<int>(order.size());
	}

	const TabEntry& target = order[static_cast<std::size_t>(next)];
	if (target.type == SelectionType::Entity) {
		m_canvas.m_selected_entity_idx = m_canvas.FindInstanceIndex(static_cast<uint32_t>(target.id));
		m_canvas.m_hovered_entity_idx = m_canvas.m_selected_entity_idx;
		m_canvas.m_selected_warp_idx = -1;
		m_canvas.m_hovered_warp_idx = -1;
		m_canvas.m_selected_tileswap_region_idx = -1;
		m_canvas.m_hovered_tileswap_region_idx = -1;
		m_canvas.m_selected_door_idx = -1;
		m_canvas.m_hovered_door_idx = -1;
	} else if (target.type == SelectionType::Warp) {
		m_canvas.m_selected_entity_idx = -1;
		m_canvas.m_hovered_entity_idx = -1;
		m_canvas.m_selected_warp_idx = m_canvas.FindWarpIndex(static_cast<uint32_t>(target.id));
		m_canvas.m_hovered_warp_idx = m_canvas.m_selected_warp_idx;
		m_canvas.m_selected_tileswap_region_idx = -1;
		m_canvas.m_hovered_tileswap_region_idx = -1;
		m_canvas.m_selected_door_idx = -1;
		m_canvas.m_hovered_door_idx = -1;
	} else if (target.type == SelectionType::TileSwapRegion) {
		m_canvas.m_selected_entity_idx = -1;
		m_canvas.m_hovered_entity_idx = -1;
		m_canvas.m_selected_warp_idx = -1;
		m_canvas.m_hovered_warp_idx = -1;
		m_canvas.m_selected_tileswap_region_idx = target.index;
		m_canvas.m_hovered_tileswap_region_idx = m_canvas.m_selected_tileswap_region_idx;
		m_canvas.m_selected_door_idx = -1;
		m_canvas.m_hovered_door_idx = -1;
	} else {
		m_canvas.m_selected_entity_idx = -1;
		m_canvas.m_hovered_entity_idx = -1;
		m_canvas.m_selected_warp_idx = -1;
		m_canvas.m_hovered_warp_idx = -1;
		m_canvas.m_selected_tileswap_region_idx = -1;
		m_canvas.m_hovered_tileswap_region_idx = -1;
		m_canvas.m_selected_door_idx = target.index;
		m_canvas.m_hovered_door_idx = target.index;
	}

	FocusCameraOnSelectedObjectIfNeeded();
}

void GLCanvasObjectCoordinator::SelectNextTileSwapRegion(int direction)
{
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(m_canvas.m_gd, m_canvas.m_current_room, m_canvas.m_mapRenderer, m_canvas.m_heightmapRenderer.GetZExtent());
	if (regions.empty()) {
		m_canvas.m_selected_tileswap_region_idx = -1;
		return;
	}
	int current = m_canvas.m_selected_tileswap_region_idx >= 0 ? m_canvas.m_selected_tileswap_region_idx : -1;
	int next = (current + direction) % static_cast<int>(regions.size());
	if (next < 0) {
		next += static_cast<int>(regions.size());
	}
	m_canvas.m_selected_entity_idx = -1;
	m_canvas.m_hovered_entity_idx = -1;
	m_canvas.m_selected_warp_idx = -1;
	m_canvas.m_hovered_warp_idx = -1;
	m_canvas.m_selected_tileswap_region_idx = next;
	m_canvas.m_hovered_tileswap_region_idx = next;
}

void GLCanvasObjectCoordinator::NudgeSelectedObject(float dx, float dy, float dz)
{
	if (m_canvas.m_selected_door_idx >= 0) {
		m_canvas.ClearTileSwapPreview();
		auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
		if (!rd) {
			return;
		}
		auto doors = rd->GetDoors(m_canvas.m_current_room);
		if (m_canvas.m_selected_door_idx < 0 || m_canvas.m_selected_door_idx >= static_cast<int>(doors.size())) {
			m_canvas.m_selected_door_idx = -1;
			m_canvas.m_hovered_door_idx = -1;
			return;
		}
		Landstalker::Door& door = doors[static_cast<std::size_t>(m_canvas.m_selected_door_idx)];
		door.x = static_cast<uint8_t>(std::clamp(static_cast<int>(door.x) + static_cast<int>(dx), 0, 63));
		door.y = static_cast<uint8_t>(std::clamp(static_cast<int>(door.y) + static_cast<int>(dy), 0, 63));
		rd->SetDoors(m_canvas.m_current_room, doors);
		m_canvas.m_hovered_door_idx = m_canvas.m_selected_door_idx;
		return;
	}

	if (m_canvas.m_selected_tileswap_region_idx >= 0) {
		m_canvas.ClearTileSwapPreview();
		auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(m_canvas.m_gd, m_canvas.m_current_room, m_canvas.m_mapRenderer, m_canvas.m_heightmapRenderer.GetZExtent());
		if (m_canvas.m_selected_tileswap_region_idx >= static_cast<int>(regions.size())) {
			m_canvas.m_selected_tileswap_region_idx = -1;
			return;
		}
		const auto& region = regions[static_cast<std::size_t>(m_canvas.m_selected_tileswap_region_idx)];
		auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
		if (!rd) {
			return;
		}
		auto swaps = rd->GetTileSwaps(m_canvas.m_current_room);
		if (region.swap_index < 0 || region.swap_index >= static_cast<int>(swaps.size())) {
			return;
		}
		Landstalker::TileSwap& swap = swaps[static_cast<std::size_t>(region.swap_index)];
		auto metrics = GLCanvasObjectSupport::MetricsForTileSwapRegion(swap, region.part);
		int x = std::clamp(metrics.x + static_cast<int>(dx), 0, std::max(0, 64 - metrics.width));
		int y = std::clamp(metrics.y + static_cast<int>(dy), 0, std::max(0, 64 - metrics.height));
		switch (region.part) {
			case TileSwapRegionPart::TilemapSource:
				swap.map.src_x = static_cast<uint8_t>(x);
				swap.map.src_y = static_cast<uint8_t>(y);
				break;
			case TileSwapRegionPart::TilemapDestination:
				swap.map.dst_x = static_cast<uint8_t>(x);
				swap.map.dst_y = static_cast<uint8_t>(y);
				break;
			case TileSwapRegionPart::HeightmapSource:
				swap.heightmap.src_x = static_cast<uint8_t>(x);
				swap.heightmap.src_y = static_cast<uint8_t>(y);
				break;
			case TileSwapRegionPart::HeightmapDestination:
				swap.heightmap.dst_x = static_cast<uint8_t>(x);
				swap.heightmap.dst_y = static_cast<uint8_t>(y);
				break;
		}
		rd->SetTileSwaps(m_canvas.m_current_room, swaps);
		return;
	}

	if (m_canvas.m_selected_entity_idx >= 0 && m_canvas.m_selected_entity_idx < static_cast<int>(m_canvas.m_instances.size())) {
		uint32_t selected_id = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)].instance_id;
		SpriteInstance& inst = m_canvas.m_instances[static_cast<std::size_t>(m_canvas.m_selected_entity_idx)];
		bool was_on_floor = std::abs(inst.map_z - inst.floor_z) <= 0.01f;

		inst.map_x = std::clamp(inst.map_x + dx * 0.5f, 0.0f, 63.5f);
		inst.map_y = std::clamp(inst.map_y + dy * 0.5f, 0.0f, 63.5f);
		inst.map_z = std::clamp(inst.map_z + dz, 0.0f, 15.5f);
		inst.floor_z = m_canvas.FloorUnderHitbox(
			inst.map_x + inst.hitbox_offset,
			inst.map_y + inst.hitbox_offset,
			inst.hitbox_base * 0.5f);
		if (dz == 0.0f && was_on_floor) {
			inst.map_z = std::clamp(inst.floor_z, 0.0f, 15.5f);
		}

		m_canvas.UpdateEntityProjection(inst);
		GLCanvasObjectSupport::SortEntitiesGeometrically(m_canvas.m_instances);
		m_canvas.m_selected_entity_idx = m_canvas.FindInstanceIndex(selected_id);
		m_canvas.m_hovered_entity_idx = m_canvas.m_selected_entity_idx;
		m_canvas.m_selected_warp_idx = -1;
		m_canvas.m_hovered_warp_idx = -1;
		return;
	}

	if (m_canvas.m_selected_warp_idx >= 0 && m_canvas.m_selected_warp_idx < static_cast<int>(m_canvas.m_warps.size())) {
		WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)];
		warp.x = std::clamp(warp.x + dx, 0.0f, 63.5f);
		warp.y = std::clamp(warp.y + dy, 0.0f, 63.5f);
		m_canvas.UpdateWarpFloor(warp);
		m_canvas.m_selected_entity_idx = -1;
		m_canvas.m_hovered_entity_idx = -1;
	}
}
