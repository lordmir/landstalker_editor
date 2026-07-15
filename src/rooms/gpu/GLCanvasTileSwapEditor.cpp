#include "GLCanvasTileDoorEditor.h"
#include "GLCanvasTileDoorEditorSupport.h"


void GLCanvasTileDoorEditor::BeginAddTileSwap()
{
	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	if (!rd) {
		return;
	}
	auto swaps = rd->GetTileSwaps(m_canvas.m_current_room);
	if (swaps.size() >= 32UL) {
		return;
	}
	std::set<int> used_triggers;
	for (const auto& swap : swaps) {
		used_triggers.insert(static_cast<int>(swap.trigger));
	}
	int trigger = -1;
	for (int candidate = 0; candidate <= 31; ++candidate) {
		if (used_triggers.count(candidate) == 0) {
			trigger = candidate;
			break;
		}
	}
	if (trigger < 0) {
		return;
	}

	m_canvas.m_pending_add_swap = Landstalker::TileSwap{};
	m_canvas.m_pending_add_swap.active = true;
	m_canvas.m_pending_add_swap.trigger = static_cast<uint8_t>(trigger);
	m_canvas.m_pending_add_swap.mode = Landstalker::TileSwap::Mode::FLOOR;
	m_canvas.m_pending_add_swap.map = {0, 0, 0, 0, 1, 1};
	m_canvas.m_pending_add_swap.heightmap = {0, 0, 0, 0, 1, 1};
	m_canvas.m_pending_add_type = MyGLCanvas::PendingObjectAddType::TileSwap;
	m_canvas.m_pending_tileswap_part = MyGLCanvas::PendingTileSwapPart::MapSource;
	m_canvas.UpdatePendingObjectAddHover();
	m_canvas.SetCursor(wxCursor(wxCURSOR_CROSS));
	m_canvas.Refresh();
}


void GLCanvasTileDoorEditor::CommitPendingTileSwapStep()
{
	if (m_canvas.m_pending_add_type != MyGLCanvas::PendingObjectAddType::TileSwap) {
		return;
	}

	uint8_t x = static_cast<uint8_t>(std::clamp(m_canvas.m_pending_add_hover_x, 0, 63));
	uint8_t y = static_cast<uint8_t>(std::clamp(m_canvas.m_pending_add_hover_y, 0, 63));
	switch (m_canvas.m_pending_tileswap_part) {
		case MyGLCanvas::PendingTileSwapPart::MapSource:
			m_canvas.m_pending_add_swap.map.src_x = x;
			m_canvas.m_pending_add_swap.map.src_y = y;
			m_canvas.m_pending_tileswap_part = MyGLCanvas::PendingTileSwapPart::MapDestination;
			m_canvas.Refresh();
			return;
		case MyGLCanvas::PendingTileSwapPart::MapDestination:
			m_canvas.m_pending_add_swap.map.dst_x = x;
			m_canvas.m_pending_add_swap.map.dst_y = y;
			m_canvas.m_pending_tileswap_part = MyGLCanvas::PendingTileSwapPart::HeightmapSource;
			m_canvas.Refresh();
			return;
		case MyGLCanvas::PendingTileSwapPart::HeightmapSource:
			m_canvas.m_pending_add_swap.heightmap.src_x = x;
			m_canvas.m_pending_add_swap.heightmap.src_y = y;
			m_canvas.m_pending_tileswap_part = MyGLCanvas::PendingTileSwapPart::HeightmapDestination;
			m_canvas.Refresh();
			return;
		case MyGLCanvas::PendingTileSwapPart::HeightmapDestination:
			m_canvas.m_pending_add_swap.heightmap.dst_x = x;
			m_canvas.m_pending_add_swap.heightmap.dst_y = y;
			break;
	}

	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	if (!rd) {
		m_canvas.CancelPendingObjectAdd();
		return;
	}

	auto swaps = rd->GetTileSwaps(m_canvas.m_current_room);
	m_canvas.CaptureObjectUndoState();
	swaps.push_back(m_canvas.m_pending_add_swap);
	rd->SetTileSwaps(m_canvas.m_current_room, swaps);

	m_canvas.m_selected_entity_idx = -1;
	m_canvas.m_selected_warp_idx = -1;
	m_canvas.m_selected_door_idx = -1;
	int new_swap_idx = static_cast<int>(swaps.size() - 1);
	m_canvas.m_selected_tileswap_region_idx = -1;
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(
		m_canvas.m_gd,
		m_canvas.m_current_room,
		m_canvas.m_mapRenderer,
		m_canvas.m_heightmapRenderer.GetZExtent());
	for (const auto& region : regions) {
		if (region.swap_index == new_swap_idx) {
			m_canvas.m_selected_tileswap_region_idx = region.flat_index;
			break;
		}
	}
	m_canvas.m_hovered_tileswap_region_idx = m_canvas.m_selected_tileswap_region_idx;
	m_canvas.NotifyRoomDataChanged(false, false, true, false);
	m_canvas.NotifySelectionChanged();
	m_canvas.CancelPendingObjectAdd();
}


int GLCanvasTileDoorEditor::HitTestTileSwapRegion(const wxPoint& point) const
{
	PickPoint world_point{
		m_canvas.ScreenToWorldX(point.x),
		m_canvas.ScreenToWorldY(point.y)
	};
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(
		m_canvas.m_gd,
		m_canvas.m_current_room,
		m_canvas.m_mapRenderer,
		m_canvas.m_heightmapRenderer.GetZExtent());
	for (int i = static_cast<int>(regions.size()) - 1; i >= 0; --i) {
		const auto& region = regions[static_cast<std::size_t>(i)];
		constexpr float hit_padding = 5.0f;
		if (world_point.x < region.bounds.min_x - hit_padding ||
			world_point.x > region.bounds.max_x + hit_padding ||
			world_point.y < region.bounds.min_y - hit_padding ||
			world_point.y > region.bounds.max_y + hit_padding) {
			continue;
		}
		bool hit = PointInPolygon(world_point, region.fill_points) ||
			PointInPolygonWinding(world_point, region.fill_points) ||
			(region.segments
				? PointNearSegments(world_point, region.points, hit_padding)
				: PointNearPolyline(world_point, region.points, hit_padding));
		if (hit) {
			return region.flat_index;
		}
	}
	return -1;
}


int GLCanvasTileDoorEditor::HitTestTileSwapRegionResizeControl(const wxPoint& point) const
{
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(
		m_canvas.m_gd,
		m_canvas.m_current_room,
		m_canvas.m_mapRenderer,
		m_canvas.m_heightmapRenderer.GetZExtent());
	if (m_canvas.m_selected_tileswap_region_idx < 0 ||
		m_canvas.m_selected_tileswap_region_idx >= static_cast<int>(regions.size())) {
		return 0;
	}
	PickPoint world_point{
		m_canvas.ScreenToWorldX(point.x),
		m_canvas.ScreenToWorldY(point.y)
	};
	const auto& region = regions[static_cast<std::size_t>(m_canvas.m_selected_tileswap_region_idx)];
	if (PointInRect(world_point, TileSwapRegionResizeControlRect(region))) {
		return 1;
	}
	return 0;
}


void GLCanvasTileDoorEditor::StartTileSwapRegionDrag(int region_idx, int resize_axis, const wxMouseEvent& evt)
{
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(
		m_canvas.m_gd,
		m_canvas.m_current_room,
		m_canvas.m_mapRenderer,
		m_canvas.m_heightmapRenderer.GetZExtent());
	if (region_idx < 0 || region_idx >= static_cast<int>(regions.size())) {
		return;
	}

	m_canvas.CaptureObjectUndoState();
	const auto& region = regions[static_cast<std::size_t>(region_idx)];
	auto metrics = GLCanvasObjectSupport::MetricsForTileSwapRegion(region.swap, region.part);
	m_canvas.m_dragging_tileswap_region = true;
	m_canvas.m_drag_tileswap_region_idx = region_idx;
	m_canvas.m_drag_tileswap_resize_axis = resize_axis;
	m_canvas.m_drag_start_mouse = evt.GetPosition();
	m_canvas.m_drag_tileswap_start_x = metrics.x;
	m_canvas.m_drag_tileswap_start_y = metrics.y;
	m_canvas.m_drag_tileswap_start_width = metrics.width;
	m_canvas.m_drag_tileswap_start_height = metrics.height;
	m_canvas.m_selected_tileswap_region_idx = region_idx;
	m_canvas.m_selected_entity_idx = -1;
	m_canvas.m_selected_warp_idx = -1;
	m_canvas.SetCursor(wxCursor(resize_axis != 0 ? wxCURSOR_SIZENWSE : wxCURSOR_HAND));
	if (!m_canvas.HasCapture()) {
		m_canvas.CaptureMouse();
	}
}


void GLCanvasTileDoorEditor::UpdateTileSwapRegionDrag(const wxMouseEvent& evt)
{
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(
		m_canvas.m_gd,
		m_canvas.m_current_room,
		m_canvas.m_mapRenderer,
		m_canvas.m_heightmapRenderer.GetZExtent());
	if (m_canvas.m_drag_tileswap_region_idx < 0 ||
		m_canvas.m_drag_tileswap_region_idx >= static_cast<int>(regions.size())) {
		EndTileSwapRegionDrag();
		return;
	}

	const auto& region = regions[static_cast<std::size_t>(m_canvas.m_drag_tileswap_region_idx)];
	int dx = 0;
	int dy = 0;
	if (region.part == TileSwapRegionPart::TilemapSource ||
		region.part == TileSwapRegionPart::TilemapDestination) {
		PickPoint start = ScreenToMapPoint(
			m_canvas.ScreenToWorldX(m_canvas.m_drag_start_mouse.x),
			m_canvas.ScreenToWorldY(m_canvas.m_drag_start_mouse.y),
			0.0f,
			static_cast<float>(m_canvas.m_mapRenderer.GetRoomLeft()),
			static_cast<float>(m_canvas.m_mapRenderer.GetRoomTop()));
		PickPoint current = ScreenToMapPoint(
			m_canvas.ScreenToWorldX(evt.GetPosition().x),
			m_canvas.ScreenToWorldY(evt.GetPosition().y),
			0.0f,
			static_cast<float>(m_canvas.m_mapRenderer.GetRoomLeft()),
			static_cast<float>(m_canvas.m_mapRenderer.GetRoomTop()));
		dx = static_cast<int>(std::trunc(current.x - start.x));
		dy = static_cast<int>(std::trunc(current.y - start.y));
	} else {
		PickPoint start = ScreenToHeightmapPoint(
			m_canvas.ScreenToWorldX(m_canvas.m_drag_start_mouse.x),
			m_canvas.ScreenToWorldY(m_canvas.m_drag_start_mouse.y),
			static_cast<float>(m_canvas.m_mapRenderer.GetRoomLeft()),
			static_cast<float>(m_canvas.m_mapRenderer.GetRoomTop()));
		PickPoint current = ScreenToHeightmapPoint(
			m_canvas.ScreenToWorldX(evt.GetPosition().x),
			m_canvas.ScreenToWorldY(evt.GetPosition().y),
			static_cast<float>(m_canvas.m_mapRenderer.GetRoomLeft()),
			static_cast<float>(m_canvas.m_mapRenderer.GetRoomTop()));
		dx = static_cast<int>(std::trunc(current.x - start.x));
		dy = static_cast<int>(std::trunc(current.y - start.y));
	}

	if (m_canvas.m_drag_tileswap_resize_axis != 0) {
		int horizontal_width_delta = static_cast<int>(std::trunc(
			(m_canvas.ScreenToWorldX(evt.GetPosition().x) - m_canvas.ScreenToWorldX(m_canvas.m_drag_start_mouse.x)) / 32.0f));
		int vertical_height_delta = static_cast<int>(std::trunc(
			(m_canvas.ScreenToWorldY(evt.GetPosition().y) - m_canvas.ScreenToWorldY(m_canvas.m_drag_start_mouse.y)) / 32.0f));

		ClearTileSwapPreview();
		auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
		if (!rd) {
			return;
		}
		auto swaps = rd->GetTileSwaps(m_canvas.m_current_room);
		if (region.swap_index < 0 || region.swap_index >= static_cast<int>(swaps.size())) {
			return;
		}

		auto clamp_size = [&](int value, int origin) {
			return std::clamp(value, 1, std::max(1, 64 - origin));
		};

		auto apply_size = [&](std::vector<Landstalker::TileSwap>& target_swaps, int width, int height) {
			Landstalker::TileSwap& swap = target_swaps[static_cast<std::size_t>(region.swap_index)];
			auto metrics = GLCanvasObjectSupport::MetricsForTileSwapRegion(swap, region.part);
			int clamped_w = clamp_size(width, metrics.x);
			int clamped_h = clamp_size(height, metrics.y);
			if (region.part == TileSwapRegionPart::TilemapSource ||
				region.part == TileSwapRegionPart::TilemapDestination) {
				swap.map.width = static_cast<uint8_t>(clamped_w);
				swap.map.height = static_cast<uint8_t>(clamped_h);
			} else {
				swap.heightmap.width = static_cast<uint8_t>(clamped_w);
				swap.heightmap.height = static_cast<uint8_t>(clamped_h);
			}
		};

		int base_w = std::max(1, m_canvas.m_drag_tileswap_start_width);
		int base_h = std::max(1, m_canvas.m_drag_tileswap_start_height);

		auto choose_best_nw_mapping = [&]() {
			struct Candidate {
				int w;
				int h;
			};
			std::array<Candidate, 2> candidates = {{
				{base_w + horizontal_width_delta, base_h + vertical_height_delta},
				{base_w - horizontal_width_delta, base_h + vertical_height_delta}
			}};

			PickPoint cursor{
				m_canvas.ScreenToWorldX(evt.GetPosition().x),
				m_canvas.ScreenToWorldY(evt.GetPosition().y)
			};

			float best_dist = std::numeric_limits<float>::max();
			std::vector<Landstalker::TileSwap> best_swaps = swaps;
			for (const auto& candidate : candidates) {
				auto test_swaps = swaps;
				apply_size(test_swaps, candidate.w, candidate.h);
				rd->SetTileSwaps(m_canvas.m_current_room, test_swaps);
				auto test_regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(
					m_canvas.m_gd,
					m_canvas.m_current_room,
					m_canvas.m_mapRenderer,
					m_canvas.m_heightmapRenderer.GetZExtent());
				if (m_canvas.m_drag_tileswap_region_idx < 0 ||
					m_canvas.m_drag_tileswap_region_idx >= static_cast<int>(test_regions.size())) {
					continue;
				}
				PickPoint handle = test_regions[static_cast<std::size_t>(m_canvas.m_drag_tileswap_region_idx)].resize_handle;
				float dxh = handle.x - cursor.x;
				float dyh = handle.y - cursor.y;
				float dist = dxh * dxh + dyh * dyh;
				if (dist < best_dist) {
					best_dist = dist;
					best_swaps = std::move(test_swaps);
				}
			}
			rd->SetTileSwaps(m_canvas.m_current_room, best_swaps);
		};

		if (region.swap.mode == Landstalker::TileSwap::Mode::FLOOR) {
			apply_size(swaps, base_w + dx, base_h + dy);
			rd->SetTileSwaps(m_canvas.m_current_room, swaps);
		} else if (region.swap.mode == Landstalker::TileSwap::Mode::WALL_NW) {
			choose_best_nw_mapping();
		} else {
			apply_size(swaps, base_w + horizontal_width_delta, base_h + vertical_height_delta);
			rd->SetTileSwaps(m_canvas.m_current_room, swaps);
		}
	} else {
		ClearTileSwapPreview();
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
		int x = std::clamp(m_canvas.m_drag_tileswap_start_x + dx, 0, std::max(0, 64 - metrics.width));
		int y = std::clamp(m_canvas.m_drag_tileswap_start_y + dy, 0, std::max(0, 64 - metrics.height));
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
	}

	m_canvas.m_selected_tileswap_region_idx = m_canvas.m_drag_tileswap_region_idx;
	m_canvas.m_hovered_tileswap_region_idx = m_canvas.m_drag_tileswap_region_idx;
	m_canvas.SetCursor(wxCursor(m_canvas.m_drag_tileswap_resize_axis != 0 ? wxCURSOR_SIZENWSE : wxCURSOR_HAND));
	m_canvas.Refresh();
}


void GLCanvasTileDoorEditor::EndTileSwapRegionDrag()
{
	if (!m_canvas.m_dragging_tileswap_region) {
		return;
	}
	m_canvas.m_dragging_tileswap_region = false;
	m_canvas.m_drag_tileswap_resize_axis = 0;
	if (m_canvas.HasCapture()) {
		m_canvas.ReleaseMouse();
	}
	m_canvas.SetCursor(wxCursor(m_canvas.m_hovered_tileswap_region_idx >= 0 ? wxCURSOR_HAND : wxCURSOR_ARROW));
	m_canvas.NotifyRoomDataChanged(false, false, true, false);
	m_canvas.Refresh();
}


void GLCanvasTileDoorEditor::AddTileSwap()
{
	ClearTileSwapPreview();
	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	if (!rd) {
		return;
	}

	auto swaps = rd->GetTileSwaps(m_canvas.m_current_room);
	std::set<int> used_triggers;
	for (const auto& swap : swaps) {
		used_triggers.insert(static_cast<int>(swap.trigger));
	}

	int trigger = -1;
	for (int candidate = 0; candidate <= 31; ++candidate) {
		if (used_triggers.count(candidate) == 0) {
			trigger = candidate;
			break;
		}
	}
	if (trigger < 0) {
		return;
	}

	auto cell_used = [&swaps](int x, int y) {
		for (const auto& swap : swaps) {
			auto contains = [x, y](const Landstalker::TileSwap::CopyOp& op, bool source) {
				int rx = source ? op.src_x : op.dst_x;
				int ry = source ? op.src_y : op.dst_y;
				return x >= rx && x < rx + op.width && y >= ry && y < ry + op.height;
			};
			if (contains(swap.map, true) || contains(swap.map, false) ||
				contains(swap.heightmap, true) || contains(swap.heightmap, false)) {
				return true;
			}
		}
		return false;
	};

	auto [preferred_x, preferred_y] = m_canvas.MouseHeightmapCell();
	preferred_x = std::clamp(preferred_x, 0, 62);
	preferred_y = std::clamp(preferred_y, 0, 63);
	int src_x = preferred_x;
	int src_y = preferred_y;
	bool found = false;
	int best_dist = std::numeric_limits<int>::max();
	for (int y = 0; y < 64; ++y) {
		for (int x = 0; x < 63; ++x) {
			if (!cell_used(x, y) && !cell_used(x + 1, y)) {
				int dx = x - preferred_x;
				int dy = y - preferred_y;
				int dist = dx * dx + dy * dy;
				if (dist < best_dist) {
					best_dist = dist;
					src_x = x;
					src_y = y;
					found = true;
				}
			}
		}
	}
	if (!found) {
		return;
	}

	Landstalker::TileSwap swap;
	swap.trigger = static_cast<uint8_t>(trigger);
	swap.mode = Landstalker::TileSwap::Mode::FLOOR;
	swap.map = {
		static_cast<uint8_t>(src_x),
		static_cast<uint8_t>(src_y),
		static_cast<uint8_t>(src_x + 1),
		static_cast<uint8_t>(src_y),
		1,
		1
	};
	swap.heightmap = swap.map;
	swaps.push_back(swap);
	rd->SetTileSwaps(m_canvas.m_current_room, swaps);

	m_canvas.m_selected_entity_idx = -1;
	m_canvas.m_selected_warp_idx = -1;
	m_canvas.m_selected_tileswap_region_idx = static_cast<int>((swaps.size() - 1) * 4);
	m_canvas.m_hovered_tileswap_region_idx = m_canvas.m_selected_tileswap_region_idx;
}


void GLCanvasTileDoorEditor::CycleSelectedTileSwapShape(int delta)
{
	ClearTileSwapPreview();
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(m_canvas.m_gd, m_canvas.m_current_room, m_canvas.m_mapRenderer, m_canvas.m_heightmapRenderer.GetZExtent());
	if (m_canvas.m_selected_tileswap_region_idx < 0 ||
		m_canvas.m_selected_tileswap_region_idx >= static_cast<int>(regions.size())) {
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
	int mode = static_cast<int>(swaps[static_cast<std::size_t>(region.swap_index)].mode);
	mode = (mode + delta + 3) % 3;
	swaps[static_cast<std::size_t>(region.swap_index)].mode = static_cast<Landstalker::TileSwap::Mode>(mode);
	rd->SetTileSwaps(m_canvas.m_current_room, swaps);
}


void GLCanvasTileDoorEditor::CycleSelectedTileSwapId(int delta)
{
	ClearTileSwapPreview();
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(m_canvas.m_gd, m_canvas.m_current_room, m_canvas.m_mapRenderer, m_canvas.m_heightmapRenderer.GetZExtent());
	if (m_canvas.m_selected_tileswap_region_idx < 0 ||
		m_canvas.m_selected_tileswap_region_idx >= static_cast<int>(regions.size())) {
		return;
	}

	const auto& region = regions[static_cast<std::size_t>(m_canvas.m_selected_tileswap_region_idx)];
	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	if (!rd) {
		return;
	}
	auto swaps = rd->GetTileSwaps(m_canvas.m_current_room);
	if (region.swap_index < 0 || region.swap_index >= static_cast<int>(swaps.size()) || delta == 0) {
		return;
	}

	std::set<int> used_triggers;
	for (int i = 0; i < static_cast<int>(swaps.size()); ++i) {
		if (i != region.swap_index) {
			used_triggers.insert(static_cast<int>(swaps[static_cast<std::size_t>(i)].trigger));
		}
	}

	int id = static_cast<int>(swaps[static_cast<std::size_t>(region.swap_index)].trigger);
	int step = delta >= 0 ? 1 : -1;
	for (int attempt = 0; attempt < 32; ++attempt) {
		int candidate = (id + step * (attempt + 1) + 32) % 32;
		if (used_triggers.count(candidate) == 0) {
			swaps[static_cast<std::size_t>(region.swap_index)].trigger = static_cast<uint8_t>(candidate);
			rd->SetTileSwaps(m_canvas.m_current_room, swaps);
			return;
		}
	}

	int candidate = (id + step + 32) % 32;
	for (int i = 0; i < static_cast<int>(swaps.size()); ++i) {
		if (i == region.swap_index) {
			continue;
		}
		if (static_cast<int>(swaps[static_cast<std::size_t>(i)].trigger) == candidate) {
			swaps[static_cast<std::size_t>(i)].trigger = static_cast<uint8_t>(id);
			swaps[static_cast<std::size_t>(region.swap_index)].trigger = static_cast<uint8_t>(candidate);
			rd->SetTileSwaps(m_canvas.m_current_room, swaps);
			return;
		}
	}
}


void GLCanvasTileDoorEditor::ResizeSelectedTileSwapByDelta(int dw, int dh)
{
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(m_canvas.m_gd, m_canvas.m_current_room, m_canvas.m_mapRenderer, m_canvas.m_heightmapRenderer.GetZExtent());
	if (m_canvas.m_selected_tileswap_region_idx < 0 ||
		m_canvas.m_selected_tileswap_region_idx >= static_cast<int>(regions.size())) {
		return;
	}
	auto metrics = GLCanvasObjectSupport::MetricsForTileSwapRegion(
		regions[static_cast<std::size_t>(m_canvas.m_selected_tileswap_region_idx)].swap,
		regions[static_cast<std::size_t>(m_canvas.m_selected_tileswap_region_idx)].part);
	ResizeSelectedTileSwapRegion(
		dw == 0 ? 0.0f : static_cast<float>(metrics.width + dw),
		dh == 0 ? 0.0f : static_cast<float>(metrics.height + dh));
}


void GLCanvasTileDoorEditor::ResizeSelectedTileSwapRegion(float requested_width, float requested_height)
{
	ClearTileSwapPreview();
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(m_canvas.m_gd, m_canvas.m_current_room, m_canvas.m_mapRenderer, m_canvas.m_heightmapRenderer.GetZExtent());
	if (m_canvas.m_selected_tileswap_region_idx < 0 ||
		m_canvas.m_selected_tileswap_region_idx >= static_cast<int>(regions.size())) {
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
	int width = metrics.width;
	int height = metrics.height;
	if (requested_width > 0.0f) {
		width = std::clamp(static_cast<int>(std::round(requested_width)), 1, std::max(1, 64 - metrics.x));
	}
	if (requested_height > 0.0f) {
		height = std::clamp(static_cast<int>(std::round(requested_height)), 1, std::max(1, 64 - metrics.y));
	}
	if (region.part == TileSwapRegionPart::TilemapSource ||
		region.part == TileSwapRegionPart::TilemapDestination) {
		swap.map.width = static_cast<uint8_t>(width);
		swap.map.height = static_cast<uint8_t>(height);
	} else {
		swap.heightmap.width = static_cast<uint8_t>(width);
		swap.heightmap.height = static_cast<uint8_t>(height);
	}
	rd->SetTileSwaps(m_canvas.m_current_room, swaps);
}


void GLCanvasTileDoorEditor::ToggleSelectedTileSwapPreview()
{
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(m_canvas.m_gd, m_canvas.m_current_room, m_canvas.m_mapRenderer, m_canvas.m_heightmapRenderer.GetZExtent());
	if (m_canvas.m_selected_tileswap_region_idx < 0 ||
		m_canvas.m_selected_tileswap_region_idx >= static_cast<int>(regions.size())) {
		return;
	}

	int swap_index = regions[static_cast<std::size_t>(m_canvas.m_selected_tileswap_region_idx)].swap_index;
	if (m_canvas.m_tileswap_preview_active && m_canvas.m_tileswap_preview_swap_index == swap_index) {
		ClearTileSwapPreview();
		return;
	}

	ClearTileSwapPreview();

	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	if (!rd) {
		return;
	}
	auto map_entry = rd->GetMapForRoom(m_canvas.m_current_room);
	if (!map_entry) {
		return;
	}
	auto map = map_entry->GetData();
	if (!map) {
		return;
	}
	auto swaps = rd->GetTileSwaps(m_canvas.m_current_room);
	if (swap_index < 0 || swap_index >= static_cast<int>(swaps.size())) {
		return;
	}

	m_canvas.m_tileswap_preview_map = std::make_shared<Landstalker::Tilemap3D>(*map);
	const Landstalker::TileSwap& swap = swaps[static_cast<std::size_t>(swap_index)];
	DrawTileSwapPreviewClipped(swap, *m_canvas.m_tileswap_preview_map, Landstalker::Tilemap3D::Layer::BG);
	DrawTileSwapPreviewClipped(swap, *m_canvas.m_tileswap_preview_map, Landstalker::Tilemap3D::Layer::FG);
	DrawHeightmapSwapPreviewClipped(swap, *m_canvas.m_tileswap_preview_map);

	m_canvas.m_tileswap_preview_active = true;
	m_canvas.m_tileswap_preview_swap_index = swap_index;
	m_canvas.m_mapRenderer.LoadPreviewRoom(m_canvas.m_current_room, *m_canvas.m_tileswap_preview_map);
	m_canvas.m_heightmapRenderer.SetPreviewMap(m_canvas.m_tileswap_preview_map);
	m_canvas.RefreshObjectPlacementsFromHeightmap();
}


void GLCanvasTileDoorEditor::RenderTileSwapOutlines()
{
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(
		m_canvas.m_gd,
		m_canvas.m_current_room,
		m_canvas.m_mapRenderer,
		m_canvas.m_heightmapRenderer.GetZExtent());
	if (regions.empty()) {
		return;
	}

	glUseProgram(0);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int hover_swap = -1;
	if (m_canvas.m_hovered_tileswap_region_idx >= 0 &&
		m_canvas.m_hovered_tileswap_region_idx < static_cast<int>(regions.size())) {
		hover_swap = regions[static_cast<std::size_t>(m_canvas.m_hovered_tileswap_region_idx)].swap_index;
	}

	auto colour_for_part = [](TileSwapRegionPart part) {
		switch (part) {
			case TileSwapRegionPart::TilemapSource:
				return std::array<float, 3>{0.15f, 0.45f, 1.0f};
			case TileSwapRegionPart::TilemapDestination:
				return std::array<float, 3>{1.0f, 0.15f, 0.15f};
			case TileSwapRegionPart::HeightmapSource:
				return std::array<float, 3>{0.75f, 0.25f, 1.0f};
			case TileSwapRegionPart::HeightmapDestination:
				return std::array<float, 3>{0.0f, 0.85f, 0.8f};
		}
		return std::array<float, 3>{1.0f, 1.0f, 1.0f};
	};

	for (const auto& region : regions) {
		bool selected = region.flat_index == m_canvas.m_selected_tileswap_region_idx;
		bool hovered_group = region.swap_index == hover_swap;
		auto colour = colour_for_part(region.part);
		float brighten = hovered_group || selected ? 1.2f : 1.0f;
		glColor4f(
			std::min(colour[0] * brighten, 1.0f),
			std::min(colour[1] * brighten, 1.0f),
			std::min(colour[2] * brighten, 1.0f),
			hovered_group || selected ? 1.0f : 0.8f);
		glLineWidth(selected ? 4.0f : (hovered_group ? 2.5f : 1.5f));
		std::vector<PickPoint> points;
		points.reserve(region.points.size());
		for (const auto& point : region.points) {
			points.push_back({point.x, point.y});
		}
		if (m_canvas.m_tileswap_preview_active && region.segments) {
			DrawSegments(points);
		} else if (m_canvas.m_tileswap_preview_active) {
			DrawClosedPolyline(points);
		} else if (region.segments) {
			DrawDashedSegments(points, selected ? 10.0f : 8.0f, selected ? 3.0f : 5.0f);
		} else {
			DrawDashedClosedPolyline(points, selected ? 10.0f : 8.0f, selected ? 3.0f : 5.0f);
		}

		if (selected) {
			PickRect rect = TileSwapRegionResizeControlRect(region);
			glColor4f(0.02f, 0.02f, 0.025f, 0.68f);
			glBegin(GL_QUADS);
			glVertex2f(rect.min_x, rect.min_y);
			glVertex2f(rect.max_x, rect.min_y);
			glVertex2f(rect.max_x, rect.max_y);
			glVertex2f(rect.min_x, rect.max_y);
			glEnd();

			glColor4f(1.0f, 0.95f, 0.2f, 0.95f);
			glLineWidth(2.0f);
			glBegin(GL_LINE_LOOP);
			glVertex2f(rect.min_x, rect.min_y);
			glVertex2f(rect.max_x, rect.min_y);
			glVertex2f(rect.max_x, rect.max_y);
			glVertex2f(rect.min_x, rect.max_y);
			glEnd();
		}
	}

	glLineWidth(1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}


void GLCanvasTileDoorEditor::RenderSelectedTileSwapRegionTooltip()
{
	auto regions = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(
		m_canvas.m_gd,
		m_canvas.m_current_room,
		m_canvas.m_mapRenderer,
		m_canvas.m_heightmapRenderer.GetZExtent());
	if (m_canvas.m_selected_tileswap_region_idx < 0 ||
		m_canvas.m_selected_tileswap_region_idx >= static_cast<int>(regions.size())) {
		return;
	}

	const auto& region = regions[static_cast<std::size_t>(m_canvas.m_selected_tileswap_region_idx)];
	const Landstalker::TileSwap& swap = region.swap;
	auto metrics = GLCanvasObjectSupport::MetricsForTileSwapRegion(swap, region.part);

	if (region.points.empty() && region.fill_points.empty()) {
		return;
	}
	const PickRect& bounds = region.bounds;

	std::array<std::string, 6> lines = {
		Landstalker::StrPrintf("ID:%02X", swap.trigger),
		std::string{TileSwapPartLabel(region.part)},
		std::string{"X:"} + std::to_string(metrics.x),
		std::string{"Y:"} + std::to_string(metrics.y),
		std::string{"W:"} + std::to_string(metrics.width),
		std::string{"H:"} + std::to_string(metrics.height)
	};

	float zoom = std::max(m_canvas.ZoomFactor(), 0.0001f);
	float scale = 1.0f;
	float line_height = 10.0f;
	float char_width = 6.0f * scale;
	float text_w = 0.0f;
	for (const auto& line : lines) {
		text_w = std::max(text_w, float(line.size()) * char_width);
	}
	float text_h = float(lines.size()) * line_height;
	float x = bounds.max_x * zoom + m_canvas.m_cam_x + 10.0f;
	float y = bounds.min_y * zoom + m_canvas.m_cam_y - 4.0f;

	int w = 0;
	int h = 0;
	m_canvas.GetClientSize(&w, &h);

	glUseProgram(0);
	for (int i = 0; i <= 5; ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glDisable(GL_TEXTURE_2D);
	}
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glColor4f(0.02f, 0.02f, 0.025f, 0.72f);
	glBegin(GL_QUADS);
	glVertex2f(x - 5.0f, y - 5.0f);
	glVertex2f(x + text_w + 5.0f, y - 5.0f);
	glVertex2f(x + text_w + 5.0f, y + text_h + 2.0f);
	glVertex2f(x - 5.0f, y + text_h + 2.0f);
	glEnd();

	glLineWidth(1.0f);
	glColor4f(0.9f, 0.9f, 0.88f, 0.95f);
	for (std::size_t i = 0; i < lines.size(); ++i) {
		DrawOverlayText(lines[i], x, y + float(i) * line_height, scale);
	}
}


