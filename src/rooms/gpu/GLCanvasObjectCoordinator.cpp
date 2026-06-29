#include "GLCanvasObjectCoordinator.h"

#include <algorithm>
#include <cmath>

#include "GLCanvasObjectSupport.h"

using GLCanvasObjectSupport::TileSwapRegionPart;

GLCanvasObjectCoordinator::GLCanvasObjectCoordinator(MyGLCanvas& canvas)
	: m_canvas(canvas)
{
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

	m_canvas.FocusCameraOnSelectedObjectIfNeeded();
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
