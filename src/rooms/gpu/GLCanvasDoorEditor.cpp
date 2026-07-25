#include "GLCanvasDoorEditor.h"
#include "GLCanvasDoorTileSwapSupport.h"

GLCanvasDoorEditor::GLCanvasDoorEditor(GLCanvas& canvas)
	: m_canvas(canvas)
{
}


void GLCanvasDoorEditor::BeginAddDoor()
{
	m_canvas.SetFocus();
	m_canvas.m_pending_add_type = GLCanvas::PendingObjectAddType::Door;
	m_canvas.m_pending_add_door_size = Landstalker::Door::Size::DOOR_1X4;
	m_canvas.UpdatePendingObjectAddHover();
	m_canvas.SetCursor(wxCursor(wxCURSOR_CROSS));
	m_canvas.Refresh();
}


int GLCanvasDoorEditor::HitTestDoor(const wxPoint& point) const
{
	PickPoint world_point{
		m_canvas.ScreenToWorldX(point.x),
		m_canvas.ScreenToWorldY(point.y)
	};
	auto doors = BuildDoorGeometries(
		m_canvas.m_gd,
		m_canvas.m_current_room,
		m_canvas.m_mapRenderer,
		m_canvas.m_heightmapRenderer.GetZExtent(),
		m_canvas.m_tileswap_preview_map);
	for (int i = static_cast<int>(doors.size()) - 1; i >= 0; --i) {
		const auto& door = doors[static_cast<std::size_t>(i)];
		constexpr float hit_padding = 5.0f;
		bool hit_cell = PointInPolygon(world_point, door.cell_points) ||
			PointInPolygonWinding(world_point, door.cell_points);
		bool hit_region = door.valid && !door.map_points.empty() &&
			(PointInPolygon(world_point, door.map_points) ||
			 PointInPolygonWinding(world_point, door.map_points) ||
			 PointNearPolyline(world_point, door.map_points, hit_padding));
		if (hit_cell || hit_region) {
			return door.index;
		}
	}
	return -1;
}


void GLCanvasDoorEditor::StartDoorDrag(int door_idx, const wxMouseEvent& evt)
{
	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	if (!rd) {
		return;
	}
	auto doors = rd->GetDoors(m_canvas.m_current_room);
	if (door_idx < 0 || door_idx >= static_cast<int>(doors.size())) {
		return;
	}

	m_canvas.CaptureObjectUndoState();
	m_canvas.ClearTileSwapPreview();
	const Landstalker::Door& door = doors[static_cast<std::size_t>(door_idx)];
	m_canvas.m_dragging_door = true;
	m_canvas.m_drag_door_idx = door_idx;
	m_canvas.m_drag_start_mouse = evt.GetPosition();
	m_canvas.m_drag_door_start_x = door.x;
	m_canvas.m_drag_door_start_y = door.y;
	m_canvas.m_selected_door_idx = door_idx;
	m_canvas.m_selected_entity_idx = -1;
	m_canvas.m_selected_warp_idx = -1;
	m_canvas.m_selected_tileswap_region_idx = -1;
	m_canvas.SetCursor(wxCursor(wxCURSOR_HAND));
	if (!m_canvas.HasCapture()) {
		m_canvas.CaptureMouse();
	}
}


void GLCanvasDoorEditor::UpdateDoorDrag(const wxMouseEvent& evt)
{
	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	if (!rd || m_canvas.m_drag_door_idx < 0) {
		EndDoorDrag();
		return;
	}
	auto doors = rd->GetDoors(m_canvas.m_current_room);
	if (m_canvas.m_drag_door_idx >= static_cast<int>(doors.size())) {
		EndDoorDrag();
		return;
	}

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
	int dx = static_cast<int>(std::round(current.x - start.x));
	int dy = static_cast<int>(std::round(current.y - start.y));

	Landstalker::Door& door = doors[static_cast<std::size_t>(m_canvas.m_drag_door_idx)];
	door.x = static_cast<uint8_t>(std::clamp(m_canvas.m_drag_door_start_x + dx, 0, 63));
	door.y = static_cast<uint8_t>(std::clamp(m_canvas.m_drag_door_start_y + dy, 0, 63));
	rd->SetDoors(m_canvas.m_current_room, doors);

	m_canvas.m_selected_door_idx = m_canvas.m_drag_door_idx;
	m_canvas.m_hovered_door_idx = m_canvas.m_drag_door_idx;
	m_canvas.SetCursor(wxCursor(wxCURSOR_HAND));
	m_canvas.Refresh();
}


void GLCanvasDoorEditor::EndDoorDrag()
{
	if (!m_canvas.m_dragging_door) {
		return;
	}

	m_canvas.m_dragging_door = false;
	if (m_canvas.HasCapture()) {
		m_canvas.ReleaseMouse();
	}
	m_canvas.m_hovered_door_idx = m_canvas.m_selected_door_idx;
	m_canvas.SetCursor(wxCursor(m_canvas.m_hovered_door_idx >= 0 ? wxCURSOR_HAND : wxCURSOR_ARROW));
	m_canvas.NotifyRoomDataChanged(false, false, false, true);
	m_canvas.Refresh();
}


void GLCanvasDoorEditor::CycleSelectedDoorSize(int delta)
{
	if (m_canvas.m_pending_add_type == GLCanvas::PendingObjectAddType::Door) {
		static constexpr std::array<Landstalker::Door::Size, 4> sizes{
			Landstalker::Door::Size::DOOR_1X4,
			Landstalker::Door::Size::DOOR_2X4,
			Landstalker::Door::Size::DOOR_2X5,
			Landstalker::Door::Size::DOOR_1X0
		};
		auto it = std::find(sizes.begin(), sizes.end(), m_canvas.m_pending_add_door_size);
		int idx = it == sizes.end() ? 0 : static_cast<int>(std::distance(sizes.begin(), it));
		idx = (idx + delta + static_cast<int>(sizes.size())) % static_cast<int>(sizes.size());
		m_canvas.m_pending_add_door_size = sizes[static_cast<std::size_t>(idx)];
		m_canvas.Refresh();
		return;
	}
	m_canvas.ClearTileSwapPreview();
	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	if (!rd || m_canvas.m_selected_door_idx < 0) {
		return;
	}
	auto doors = rd->GetDoors(m_canvas.m_current_room);
	if (m_canvas.m_selected_door_idx >= static_cast<int>(doors.size()) || delta == 0) {
		return;
	}

	static constexpr std::array<Landstalker::Door::Size, 4> sizes{
		Landstalker::Door::Size::DOOR_1X4,
		Landstalker::Door::Size::DOOR_2X4,
		Landstalker::Door::Size::DOOR_2X5,
		Landstalker::Door::Size::DOOR_1X0
	};

	Landstalker::Door& door = doors[static_cast<std::size_t>(m_canvas.m_selected_door_idx)];
	auto it = std::find(sizes.begin(), sizes.end(), door.size);
	int idx = it == sizes.end() ? 0 : static_cast<int>(std::distance(sizes.begin(), it));
	idx = (idx + delta + static_cast<int>(sizes.size())) % static_cast<int>(sizes.size());
	door.size = sizes[static_cast<std::size_t>(idx)];
	rd->SetDoors(m_canvas.m_current_room, doors);
}


void GLCanvasDoorEditor::AddDoor()
{
	m_canvas.ClearTileSwapPreview();
	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	if (!rd) {
		return;
	}

	auto doors = rd->GetDoors(m_canvas.m_current_room);
	auto [preferred_x, preferred_y] = m_canvas.MouseHeightmapCell();
	auto cell_used = [&doors](int x, int y) {
		for (const auto& door : doors) {
			if (static_cast<int>(door.x) == x && static_cast<int>(door.y) == y) {
				return true;
			}
		}
		return false;
	};

	int best_x = preferred_x;
	int best_y = preferred_y;
	int best_dist = std::numeric_limits<int>::max();
	bool found = false;
	for (int y = 0; y <= 63; ++y) {
		for (int x = 0; x <= 63; ++x) {
			if (cell_used(x, y)) {
				continue;
			}
			int dx = x - preferred_x;
			int dy = y - preferred_y;
			int dist = dx * dx + dy * dy;
			if (dist < best_dist) {
				best_dist = dist;
				best_x = x;
				best_y = y;
				found = true;
			}
		}
	}
	if (!found) {
		return;
	}

	doors.emplace_back(
		static_cast<uint8_t>(best_x),
		static_cast<uint8_t>(best_y),
		m_canvas.m_pending_add_door_size);
	rd->SetDoors(m_canvas.m_current_room, doors);

	m_canvas.m_selected_entity_idx = -1;
	m_canvas.m_selected_warp_idx = -1;
	m_canvas.m_selected_tileswap_region_idx = -1;
	m_canvas.m_hovered_entity_idx = -1;
	m_canvas.m_hovered_warp_idx = -1;
	m_canvas.m_hovered_tileswap_region_idx = -1;
	m_canvas.m_selected_door_idx = static_cast<int>(doors.size() - 1);
	m_canvas.m_hovered_door_idx = m_canvas.m_selected_door_idx;
}


void GLCanvasDoorEditor::RenderPendingDoorGhost()
{
	if (m_canvas.m_pending_add_type != GLCanvas::PendingObjectAddType::Door) {
		return;
	}
	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	if (!rd) {
		return;
	}

	auto [preferred_x, preferred_y] = m_canvas.MouseHeightmapCell();
	auto doors = rd->GetDoors(m_canvas.m_current_room);
	auto cell_used = [&doors](int x, int y) {
		for (const auto& door : doors) {
			if (static_cast<int>(door.x) == x && static_cast<int>(door.y) == y) {
				return true;
			}
		}
		return false;
	};

	int best_x = preferred_x;
	int best_y = preferred_y;
	int best_dist = std::numeric_limits<int>::max();
	for (int gy = 0; gy <= 63; ++gy) {
		for (int gx = 0; gx <= 63; ++gx) {
			if (cell_used(gx, gy)) {
				continue;
			}
			int dx = gx - preferred_x;
			int dy = gy - preferred_y;
			int dist = dx * dx + dy * dy;
			if (dist < best_dist) {
				best_dist = dist;
				best_x = gx;
				best_y = gy;
			}
		}
	}

	auto map = m_canvas.CurrentRoomMap();
	auto height_at = [&](int x, int y) -> float {
		if (!map || x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
			return 0.0f;
		}
		uint8_t z = map->GetHeight({x, y});
		return z == 0xFF ? 0.0f : static_cast<float>(z);
	};

	const float room_left = static_cast<float>(m_canvas.m_mapRenderer.GetRoomLeft());
	const float room_top = static_cast<float>(m_canvas.m_mapRenderer.GetRoomTop());
	const float z_extent = m_canvas.m_heightmapRenderer.GetZExtent();
	PickPoint center = ProjectHeightmapGridPoint(
		static_cast<float>(best_x) + 0.5f,
		static_cast<float>(best_y) + 0.5f,
		height_at(best_x, best_y),
		room_left,
		room_top,
		z_extent);

	glUseProgram(0);
	for (int i = 0; i <= 5; ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glDisable(GL_TEXTURE_2D);
	}
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(0.0f, 0.55f, 0.22f, 0.35f);
	glBegin(GL_QUADS);
	glVertex2f(center.x, center.y - 16.0f);
	glVertex2f(center.x + 32.0f, center.y);
	glVertex2f(center.x, center.y + 16.0f);
	glVertex2f(center.x - 32.0f, center.y);
	glEnd();

	glColor4f(0.0f, 0.55f, 0.22f, 0.9f);
	glLineWidth(2.5f);
	glBegin(GL_LINE_LOOP);
	glVertex2f(center.x, center.y - 16.0f);
	glVertex2f(center.x + 32.0f, center.y);
	glVertex2f(center.x, center.y + 16.0f);
	glVertex2f(center.x - 32.0f, center.y);
	glEnd();

	glLineWidth(1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}


void GLCanvasDoorEditor::ToggleSelectedDoorPreview()
{
	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	if (!rd || m_canvas.m_selected_door_idx < 0) {
		return;
	}

	auto doors = rd->GetDoors(m_canvas.m_current_room);
	if (m_canvas.m_selected_door_idx >= static_cast<int>(doors.size())) {
		return;
	}

	if (m_canvas.m_door_preview_active && m_canvas.m_door_preview_idx == m_canvas.m_selected_door_idx) {
		m_canvas.ClearTileSwapPreview();
		return;
	}

	m_canvas.ClearTileSwapPreview();

	auto map_entry = rd->GetMapForRoom(m_canvas.m_current_room);
	if (!map_entry) {
		return;
	}
	auto map = map_entry->GetData();
	if (!map) {
		return;
	}

	m_canvas.m_tileswap_preview_map = std::make_shared<Landstalker::Tilemap3D>(*map);
	doors[static_cast<std::size_t>(m_canvas.m_selected_door_idx)].DrawDoor(*m_canvas.m_tileswap_preview_map, Landstalker::Tilemap3D::Layer::FG);

	m_canvas.m_door_preview_active = true;
	m_canvas.m_door_preview_idx = m_canvas.m_selected_door_idx;
	m_canvas.m_mapRenderer.LoadPreviewRoom(m_canvas.m_current_room, *m_canvas.m_tileswap_preview_map);
	m_canvas.m_heightmapRenderer.SetPreviewMap(m_canvas.m_tileswap_preview_map);
	m_canvas.RefreshObjectPlacementsFromHeightmap();
}


void GLCanvasDoorEditor::RenderDoors()
{
	auto doors = BuildDoorGeometries(
		m_canvas.m_gd,
		m_canvas.m_current_room,
		m_canvas.m_mapRenderer,
		m_canvas.m_heightmapRenderer.GetZExtent(),
		m_canvas.m_tileswap_preview_map);
	if (doors.empty()) {
		return;
	}

	glUseProgram(0);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (const auto& door : doors) {
		bool selected = door.index == m_canvas.m_selected_door_idx;
		bool hovered = door.index == m_canvas.m_hovered_door_idx;

		if (door.valid) {
			glColor4f(0.0f, 0.28f, 0.12f, selected || hovered ? 0.52f : 0.34f);
		} else {
			glColor4f(1.0f, 0.05f, 0.05f, selected || hovered ? 0.55f : 0.38f);
		}
		glBegin(GL_POLYGON);
		for (const auto& point : door.cell_points) {
			glVertex2f(point.x, point.y);
		}
		glEnd();

		glLineWidth(selected ? 3.5f : (hovered ? 2.5f : 1.5f));
		glColor4f(door.valid ? 0.0f : 1.0f, door.valid ? 0.45f : 0.1f, door.valid ? 0.18f : 0.1f, 0.95f);
		DrawClosedPolyline(door.cell_points);

		if (door.valid && door.map_points.size() >= 2) {
			glColor4f(1.0f, 0.62f, 1.0f, selected || hovered ? 1.0f : 0.8f);
			glLineWidth(selected ? 3.0f : (hovered ? 2.25f : 1.35f));
			DrawDashedClosedPolyline(door.map_points, selected ? 10.0f : 8.0f, selected ? 3.0f : 5.0f);
		}
	}

	glLineWidth(1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}


void GLCanvasDoorEditor::RenderSelectedDoorTooltip()
{
	auto rd = m_canvas.m_gd ? m_canvas.m_gd->GetRoomData() : nullptr;
	if (!rd || m_canvas.m_selected_door_idx < 0) {
		return;
	}

	auto doors = rd->GetDoors(m_canvas.m_current_room);
	if (m_canvas.m_selected_door_idx >= static_cast<int>(doors.size())) {
		return;
	}

	const Landstalker::Door& door = doors[static_cast<std::size_t>(m_canvas.m_selected_door_idx)];
	float room_left = static_cast<float>(m_canvas.m_mapRenderer.GetRoomLeft());
	float room_top = static_cast<float>(m_canvas.m_mapRenderer.GetRoomTop());
	float z_extent = m_canvas.m_heightmapRenderer.GetZExtent();

	auto map_entry = rd->GetMapForRoom(m_canvas.m_current_room);
	auto tilemap = map_entry ? map_entry->GetData() : nullptr;
	float z = 0.0f;
	if (tilemap) {
		int x = static_cast<int>(door.x);
		int y = static_cast<int>(door.y);
		if (x >= 0 && y >= 0 && x < tilemap->GetHeightmapWidth() && y < tilemap->GetHeightmapHeight()) {
			uint8_t hz = tilemap->GetHeight({x, y});
			z = hz == 0xFF ? 0.0f : static_cast<float>(hz);
		}
	}
	PickPoint anchor = ProjectHeightmapGridPoint(
		static_cast<float>(door.x) + 0.5f,
		static_cast<float>(door.y) + 0.5f,
		z,
		room_left,
		room_top,
		z_extent);

	const char* type_label = "Invalid";
	if (tilemap) {
		auto [valid, poly] = door.GetMapRegionPoly(tilemap, 1, 2);
		(void)poly;
		if (valid) {
			type_label = "Valid";
		}
	}

	std::array<std::string, 4> lines = {
		Landstalker::StrPrintf("ID:%02X", std::clamp(m_canvas.m_selected_door_idx + 1, 0, 255)),
		std::string{"X:"} + std::to_string(static_cast<int>(door.x)),
		std::string{"Y:"} + std::to_string(static_cast<int>(door.y)),
		std::string{"T:"} + type_label
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
	float x = anchor.x * zoom + m_canvas.m_cam_x + 12.0f;
	float y = anchor.y * zoom + m_canvas.m_cam_y - 20.0f;

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
