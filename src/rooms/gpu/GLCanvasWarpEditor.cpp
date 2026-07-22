#include "GLCanvasWarpEditor.h"
#include "GLLoader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

#include "GLCanvasObjectSupport.h"
#include "PixelFont.h"
#include "RoomProjection.h"

namespace {

using PickPoint = RoomProjection::PickPoint;
using RoomProjection::ProjectWarpGridPoint;
using RoomProjection::ScreenToMapPoint;

struct PickRect {
	float min_x;
	float min_y;
	float max_x;
	float max_y;
};

std::array<PickPoint, 4> WarpQuad(const WarpInstance& warp, float z_offset = 0.0f)
{
	float x0 = warp.x;
	float y0 = warp.y;
	float x1 = warp.x + warp.width;
	float y1 = warp.y + warp.height;
	float z = warp.floor_z + z_offset;
	return {
		ProjectWarpGridPoint(warp, x0, y0, z),
		ProjectWarpGridPoint(warp, x1, y0, z),
		ProjectWarpGridPoint(warp, x1, y1, z),
		ProjectWarpGridPoint(warp, x0, y1, z)
	};
}

PickRect RectAroundPoint(const PickPoint& point, float half_size)
{
	return {
		point.x - half_size,
		point.y - half_size,
		point.x + half_size,
		point.y + half_size
	};
}

PickRect WarpResizeControlRect(const WarpInstance& warp, int axis)
{
	float z = warp.floor_z;
	PickPoint point = axis == 1
		? ProjectWarpGridPoint(warp, warp.x + warp.width, warp.y + warp.height * 0.5f, z)
		: ProjectWarpGridPoint(warp, warp.x + warp.width * 0.5f, warp.y + warp.height, z);
	return RectAroundPoint(point, 6.0f);
}

bool PointInRect(const PickPoint& point, const PickRect& rect)
{
	return point.x >= rect.min_x &&
		   point.x <= rect.max_x &&
		   point.y >= rect.min_y &&
		   point.y <= rect.max_y;
}

float Cross(const PickPoint& a, const PickPoint& b, const PickPoint& c)
{
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool PointInQuad(const PickPoint& point, const std::array<PickPoint, 4>& quad)
{
	bool has_positive = false;
	bool has_negative = false;
	for (std::size_t i = 0; i < quad.size(); ++i) {
		float cross = Cross(quad[i], quad[(i + 1) % quad.size()], point);
		has_positive = has_positive || cross > 0.0f;
		has_negative = has_negative || cross < 0.0f;
		if (has_positive && has_negative) {
			return false;
		}
	}
	return true;
}

bool PointInWarp(const WarpInstance& warp, const PickPoint& point)
{
	return PointInQuad(point, WarpQuad(warp));
}

bool WarpResizeAxisUsable(const WarpInstance& warp, int axis)
{
	if (axis == 1) {
		return std::round(warp.height) <= 1.0f;
	}
	if (axis == 2) {
		return std::round(warp.width) <= 1.0f;
	}
	return false;
}

std::string HexWord(uint16_t value)
{
	constexpr char digits[] = "0123456789ABCDEF";
	std::string out;
	out.push_back(digits[(value >> 12) & 0x0F]);
	out.push_back(digits[(value >> 8) & 0x0F]);
	out.push_back(digits[(value >> 4) & 0x0F]);
	out.push_back(digits[value & 0x0F]);
	return out;
}

using PixelFont::DrawOverlayText;

}  // namespace

void MyGLCanvas::UpdateWarpFloor(WarpInstance& warp)
{
	warp.floor_z = FloorUnderRect(warp.x, warp.y, warp.x + warp.width, warp.y + warp.height);
}

GLCanvasWarpEditor::GLCanvasWarpEditor(MyGLCanvas& canvas)
	: m_canvas(canvas)
{
}

void GLCanvasWarpEditor::BeginAddWarpHalf()
{
	m_canvas.SetFocus();
	m_canvas.m_pending_add_type = MyGLCanvas::PendingObjectAddType::Warp;
	m_canvas.m_pending_add_warp_width = 1.0f;
	m_canvas.m_pending_add_warp_height = 1.0f;
	m_canvas.m_pending_add_warp_type = Landstalker::WarpList::Warp::Type::NORMAL;
	m_canvas.UpdatePendingObjectAddHover();
	m_canvas.SetCursor(wxCursor(wxCURSOR_CROSS));
	m_canvas.Refresh();
}

int GLCanvasWarpEditor::HitTestWarpResizeControl(const wxPoint& point) const
{
	if (m_canvas.m_selected_warp_idx < 0 || m_canvas.m_selected_warp_idx >= static_cast<int>(m_canvas.m_warps.size())) {
		return 0;
	}

	PickPoint world_point{
		m_canvas.ScreenToWorldX(point.x),
		m_canvas.ScreenToWorldY(point.y)
	};
	const WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)];
	for (int axis = 1; axis <= 2; ++axis) {
		if (WarpResizeAxisUsable(warp, axis) && PointInRect(world_point, WarpResizeControlRect(warp, axis))) {
			return axis;
		}
	}
	return 0;
}

int GLCanvasWarpEditor::HitTestWarp(const wxPoint& point) const
{
	PickPoint world_point{
		m_canvas.ScreenToWorldX(point.x),
		m_canvas.ScreenToWorldY(point.y)
	};

	for (int i = static_cast<int>(m_canvas.m_warps.size()) - 1; i >= 0; --i) {
		if (PointInWarp(m_canvas.m_warps[static_cast<std::size_t>(i)], world_point)) {
			return i;
		}
	}
	return -1;
}

void GLCanvasWarpEditor::StartWarpDrag(int warp_idx, const wxMouseEvent& evt)
{
	if (warp_idx < 0 || warp_idx >= static_cast<int>(m_canvas.m_warps.size())) {
		return;
	}

	m_canvas.CaptureObjectUndoState();
	WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(warp_idx)];
	m_canvas.m_dragging_warp = true;
	m_canvas.m_drag_warp_instance_id = warp.instance_id;
	m_canvas.m_drag_warp_resize_axis = 0;
	m_canvas.m_drag_start_mouse = evt.GetPosition();
	m_canvas.m_drag_warp_start_x = warp.x;
	m_canvas.m_drag_warp_start_y = warp.y;
	m_canvas.m_drag_warp_start_width = warp.width;
	m_canvas.m_drag_warp_start_height = warp.height;
	m_canvas.m_drag_warp_start_floor_z = warp.floor_z;
	m_canvas.SetCursor(wxCursor(wxCURSOR_HAND));
	if (!m_canvas.HasCapture()) {
		m_canvas.CaptureMouse();
	}
}

void GLCanvasWarpEditor::StartWarpResizeDrag(int warp_idx, int axis, const wxMouseEvent& evt)
{
	if (warp_idx < 0 || warp_idx >= static_cast<int>(m_canvas.m_warps.size())) {
		return;
	}

	m_canvas.CaptureObjectUndoState();
	WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(warp_idx)];
	m_canvas.m_dragging_warp = true;
	m_canvas.m_drag_warp_instance_id = warp.instance_id;
	m_canvas.m_drag_warp_resize_axis = axis;
	m_canvas.m_drag_start_mouse = evt.GetPosition();
	m_canvas.m_drag_warp_start_x = warp.x;
	m_canvas.m_drag_warp_start_y = warp.y;
	m_canvas.m_drag_warp_start_width = warp.width;
	m_canvas.m_drag_warp_start_height = warp.height;
	m_canvas.m_drag_warp_start_floor_z = warp.floor_z;
	m_canvas.m_selected_warp_idx = warp_idx;
	m_canvas.m_selected_entity_idx = -1;
	m_canvas.SetCursor(wxCursor(axis == 1 ? wxCURSOR_SIZENWSE : wxCURSOR_SIZENESW));
	if (!m_canvas.HasCapture()) {
		m_canvas.CaptureMouse();
	}
}

void GLCanvasWarpEditor::UpdateWarpDrag(const wxMouseEvent& evt)
{
	int warp_idx = m_canvas.FindWarpIndex(m_canvas.m_drag_warp_instance_id);
	if (warp_idx < 0) {
		EndWarpDrag();
		return;
	}

	WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(warp_idx)];
	auto snap_cell = [](float value) {
		return std::round(value);
	};
	auto clamp_map_pos = [](float value) {
		return std::clamp(value, 0.0f, 63.5f);
	};

	PickPoint center = ScreenToMapPoint(
		m_canvas.ScreenToWorldX(evt.GetPosition().x),
		m_canvas.ScreenToWorldY(evt.GetPosition().y),
		m_canvas.m_drag_warp_start_floor_z,
		warp.room_left,
		warp.room_top,
		warp.z_extent);
	if (m_canvas.m_drag_warp_resize_axis == 1) {
		warp.x = m_canvas.m_drag_warp_start_x;
		warp.y = m_canvas.m_drag_warp_start_y;
		warp.height = GLCanvasObjectSupport::ValidWarpHeight(m_canvas.m_drag_warp_start_height, m_canvas.m_drag_warp_start_width);
		warp.width = std::min(GLCanvasObjectSupport::ValidWarpWidth(center.x - m_canvas.m_drag_warp_start_x, warp.height), 63.5f - m_canvas.m_drag_warp_start_x);
		m_canvas.SetCursor(wxCursor(wxCURSOR_SIZENWSE));
	} else if (m_canvas.m_drag_warp_resize_axis == 2) {
		warp.x = m_canvas.m_drag_warp_start_x;
		warp.y = m_canvas.m_drag_warp_start_y;
		warp.width = GLCanvasObjectSupport::ValidWarpWidth(m_canvas.m_drag_warp_start_width, m_canvas.m_drag_warp_start_height);
		warp.height = std::min(GLCanvasObjectSupport::ValidWarpHeight(center.y - m_canvas.m_drag_warp_start_y, warp.width), 63.5f - m_canvas.m_drag_warp_start_y);
		m_canvas.SetCursor(wxCursor(wxCURSOR_SIZENESW));
	} else {
		warp.x = clamp_map_pos(snap_cell(center.x - warp.width * 0.5f));
		warp.y = clamp_map_pos(snap_cell(center.y - warp.height * 0.5f));
		m_canvas.SetCursor(wxCursor(wxCURSOR_HAND));
	}
	m_canvas.UpdateWarpFloor(warp);

	m_canvas.m_selected_warp_idx = warp_idx;
	m_canvas.m_selected_entity_idx = -1;
	m_canvas.m_hovered_warp_idx = warp_idx;
	m_canvas.m_hovered_entity_idx = -1;
	m_canvas.Refresh();
}

void GLCanvasWarpEditor::EndWarpDrag()
{
	if (!m_canvas.m_dragging_warp) {
		return;
	}

	uint32_t dragged_id = m_canvas.m_drag_warp_instance_id;
	m_canvas.m_dragging_warp = false;
	m_canvas.m_drag_warp_resize_axis = 0;
	if (m_canvas.HasCapture()) {
		m_canvas.ReleaseMouse();
	}
	m_canvas.m_selected_warp_idx = m_canvas.FindWarpIndex(dragged_id);
	m_canvas.m_hovered_warp_idx = m_canvas.m_selected_warp_idx;
	m_canvas.SetCursor(wxCursor(m_canvas.m_hovered_warp_idx >= 0 ? wxCURSOR_HAND : wxCURSOR_ARROW));
	m_canvas.NotifyRoomDataChanged(false, true, false, false);
	m_canvas.Refresh();
}

void GLCanvasWarpEditor::AddWarpHalf()
{
	// Warp creation is two-step: place one endpoint now, then complete on next insert.
	Landstalker::WarpList::Warp warp;
	// Use room-grid coords (m_pending_add_hover_x/y from ScreenToMapPoint) to match
	// ProjectWarpGridPoint, which has no heightmap offset. Centre on the cursor.
	float half_w = std::round(m_canvas.m_pending_add_warp_width) * 0.5f;
	float half_h = std::round(m_canvas.m_pending_add_warp_height) * 0.5f;
	float cursor_x, cursor_y;
	if (m_canvas.m_pending_add_hover_x >= 0 && m_canvas.m_pending_add_hover_y >= 0) {
		cursor_x = static_cast<float>(m_canvas.m_pending_add_hover_x) + 0.5f;
		cursor_y = static_cast<float>(m_canvas.m_pending_add_hover_y) + 0.5f;
	} else {
		auto [hx, hy] = m_canvas.MouseHeightmapCell();
		cursor_x = static_cast<float>(hx) + 12.5f;
		cursor_y = static_cast<float>(hy) + 12.5f;
	}
	auto [x, y] = FindNearestFreeWarpCell(cursor_x - half_w, cursor_y - half_h);
	bool update_pending_instance = false;
	if (!m_canvas.m_pending_warp_half) {
		warp.room1 = m_canvas.m_current_room;
		warp.x1 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(x)), 0, 63));
		warp.y1 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(y)), 0, 63));
		warp.room2 = 0xFFFF;
		warp.x_size = static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(m_canvas.m_pending_add_warp_width)), 1, 3));
		warp.y_size = static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(m_canvas.m_pending_add_warp_height)), 1, 3));
		warp.type = m_canvas.m_pending_add_warp_type;
		m_canvas.m_pending_warp = warp;
		m_canvas.m_pending_warp_half = true;
		m_canvas.m_pending_warp_room = m_canvas.m_current_room;
		m_canvas.m_pending_warp_instance_id = static_cast<uint32_t>(m_canvas.m_warps.size() + 1);
	} else {
		if (m_canvas.m_pending_warp_room == m_canvas.m_current_room && m_canvas.m_pending_warp_instance_id != 0) {
			int pending_idx = m_canvas.FindWarpIndex(m_canvas.m_pending_warp_instance_id);
			if (pending_idx >= 0) {
				const WarpInstance& pending = m_canvas.m_warps[static_cast<std::size_t>(pending_idx)];
				m_canvas.m_pending_warp.x1 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(pending.x)), 0, 63));
				m_canvas.m_pending_warp.y1 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(pending.y)), 0, 63));
				m_canvas.m_pending_warp.x_size = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(pending.width)), 1, 63));
				m_canvas.m_pending_warp.y_size = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(pending.height)), 1, 63));
				m_canvas.m_pending_warp.type = pending.warp.type;
			}
		}
		warp = m_canvas.m_pending_warp;
		warp.room2 = m_canvas.m_current_room;
		warp.x2 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(x)), 0, 63));
		warp.y2 = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(std::round(y)), 0, 63));
		update_pending_instance = m_canvas.m_pending_warp_room == m_canvas.m_current_room;
		m_canvas.m_pending_warp_half = false;
		m_canvas.m_pending_warp_room = 0xFFFF;
	}

	if (update_pending_instance) {
		int pending_idx = m_canvas.FindWarpIndex(m_canvas.m_pending_warp_instance_id);
		if (pending_idx < 0) {
			for (std::size_t i = 0; i < m_canvas.m_warps.size(); ++i) {
				const WarpInstance& candidate = m_canvas.m_warps[i];
				if (candidate.current_room_is_room1 &&
					candidate.warp.room1 == warp.room1 &&
					candidate.warp.room2 == 0xFFFF &&
					candidate.warp.x1 == warp.x1 &&
					candidate.warp.y1 == warp.y1) {
					pending_idx = static_cast<int>(i);
					break;
				}
			}
		}
		uint32_t key = m_canvas.m_pending_warp_instance_id;
		if (pending_idx >= 0) {
			// A half placed in this session has no key yet - MakeWarpInstance defaults
			// warp_key to 0, and only the room load path assigns real keys. Adopting that
			// zero would leave both endpoints of this warp keyed by their instance ids in
			// BuildCurrentRoomWarps, which then emits the single connection twice and
			// trips the duplicate check, so the warp is rejected and thrown away. Only
			// take the pending half's key when it actually has one.
			const uint32_t pending_key = m_canvas.m_warps[static_cast<std::size_t>(pending_idx)].warp_key;
			if (pending_key != 0) {
				key = pending_key;
			}
			WarpInstance first_inst = GLCanvasObjectSupport::MakeWarpInstance(
				warp,
				m_canvas.m_current_room,
				m_canvas.m_warps[static_cast<std::size_t>(pending_idx)].instance_id,
				float(m_canvas.m_mapRenderer.GetRoomLeft()),
				float(m_canvas.m_mapRenderer.GetRoomTop()),
				m_canvas.m_heightmapRenderer.GetZExtent(),
				key,
				1);
			m_canvas.UpdateWarpFloor(first_inst);
			m_canvas.m_warps[static_cast<std::size_t>(pending_idx)] = first_inst;
		} else {
			WarpInstance first_inst = GLCanvasObjectSupport::MakeWarpInstance(
				warp,
				m_canvas.m_current_room,
				static_cast<uint32_t>(m_canvas.m_warps.size() + 1),
				float(m_canvas.m_mapRenderer.GetRoomLeft()),
				float(m_canvas.m_mapRenderer.GetRoomTop()),
				m_canvas.m_heightmapRenderer.GetZExtent(),
				key,
				1);
			m_canvas.UpdateWarpFloor(first_inst);
			m_canvas.m_warps.push_back(first_inst);
		}
		WarpInstance second_inst = GLCanvasObjectSupport::MakeWarpInstance(
			warp,
			m_canvas.m_current_room,
			static_cast<uint32_t>(m_canvas.m_warps.size() + 1),
			float(m_canvas.m_mapRenderer.GetRoomLeft()),
			float(m_canvas.m_mapRenderer.GetRoomTop()),
			m_canvas.m_heightmapRenderer.GetZExtent(),
			key,
			2);
		m_canvas.UpdateWarpFloor(second_inst);
		m_canvas.m_warps.push_back(second_inst);
		m_canvas.m_selected_warp_idx = static_cast<int>(m_canvas.m_warps.size()) - 1;
	} else {
		WarpInstance inst = GLCanvasObjectSupport::MakeWarpInstance(
			warp,
			m_canvas.m_current_room,
			static_cast<uint32_t>(m_canvas.m_warps.size() + 1),
			float(m_canvas.m_mapRenderer.GetRoomLeft()),
			float(m_canvas.m_mapRenderer.GetRoomTop()),
			m_canvas.m_heightmapRenderer.GetZExtent());
		m_canvas.UpdateWarpFloor(inst);
		m_canvas.m_warps.push_back(inst);
		m_canvas.m_selected_warp_idx = static_cast<int>(m_canvas.m_warps.size()) - 1;
	}
	if (warp.IsValid()) {
		for (std::size_t i = 0; i < m_canvas.m_warps.size(); ) {
			const WarpInstance& candidate = m_canvas.m_warps[i];
			if (static_cast<int>(i) != m_canvas.m_selected_warp_idx &&
				candidate.warp.room1 == warp.room1 &&
				candidate.warp.room2 == 0xFFFF &&
				candidate.warp.x1 == warp.x1 &&
				candidate.warp.y1 == warp.y1) {
				m_canvas.m_warps.erase(m_canvas.m_warps.begin() + static_cast<std::ptrdiff_t>(i));
				if (m_canvas.m_selected_warp_idx > static_cast<int>(i)) {
					--m_canvas.m_selected_warp_idx;
				}
				continue;
			}
			++i;
		}
		for (std::size_t i = 0; i < m_canvas.m_warps.size(); ++i) {
			m_canvas.m_warps[i].instance_id = static_cast<uint32_t>(i + 1);
		}
	}
	if (!m_canvas.m_pending_warp_half) {
		m_canvas.m_pending_warp_instance_id = 0;
		if (warp.IsValid()) {
			m_canvas.PersistCurrentRoomEdits();
		}
	}
	m_canvas.m_selected_entity_idx = -1;
}

std::pair<float, float> GLCanvasWarpEditor::FindNearestFreeWarpCell(float preferred_x, float preferred_y) const
{
	// Keep placement somewhere the room actually exists. Searching the whole 0..63 range
	// drops warps off the edge of any smaller room - a map created from scratch is only
	// 16x16, so most of that range is nowhere.
	//
	// The bound is the heightmap the warp will stand on, taken together with the tilemap
	// it is drawn over, because the two do not always cover each other. Warp coordinates
	// are heightmap cells offset by 12 (see FloorUnderRect), and the tilemap starts at the
	// heightmap origin (left, top), so in warp coordinates they span [12, 12 + heightmap
	// size) and [left, left + map size) respectively. The union is deliberate: the shipped
	// game has two warps - rooms 207 and 210 - sitting outside their room's heightmap, and
	// bounding to the heightmap alone would make placements like those impossible.
	constexpr int heightmap_origin = 12;
	int min_x = 0;
	int min_y = 0;
	int max_x = 63;
	int max_y = 63;
	if (const auto map = m_canvas.CurrentRoomMap()) {
		const int hm_left = heightmap_origin;
		const int hm_top = heightmap_origin;
		const int hm_right = hm_left + static_cast<int>(map->GetHeightmapWidth()) - 1;
		const int hm_bottom = hm_top + static_cast<int>(map->GetHeightmapHeight()) - 1;
		const int map_left = static_cast<int>(map->GetLeft());
		const int map_top = static_cast<int>(map->GetTop());
		const int map_right = map_left + static_cast<int>(map->GetWidth()) - 1;
		const int map_bottom = map_top + static_cast<int>(map->GetHeight()) - 1;
		min_x = std::clamp(std::min(hm_left, map_left), 0, 63);
		min_y = std::clamp(std::min(hm_top, map_top), 0, 63);
		max_x = std::clamp(std::max(hm_right, map_right), min_x, 63);
		max_y = std::clamp(std::max(hm_bottom, map_bottom), min_y, 63);
	}
	int start_x = std::clamp(static_cast<int>(std::round(preferred_x)), min_x, max_x);
	int start_y = std::clamp(static_cast<int>(std::round(preferred_y)), min_y, max_y);

	auto cell_free = [this](int x, int y) {
		float fx = static_cast<float>(x);
		float fy = static_cast<float>(y);
		for (const auto& warp : m_canvas.m_warps) {
			if (fx < warp.x + warp.width &&
				fx + 1.0f > warp.x &&
				fy < warp.y + warp.height &&
				fy + 1.0f > warp.y) {
				return false;
			}
		}
		return true;
	};

	int best_x = start_x;
	int best_y = start_y;
	int best_dist = std::numeric_limits<int>::max();
	for (int y = min_y; y <= max_y; ++y) {
		for (int x = min_x; x <= max_x; ++x) {
			if (!cell_free(x, y)) {
				continue;
			}
			int dx = x - start_x;
			int dy = y - start_y;
			int dist = dx * dx + dy * dy;
			if (dist < best_dist) {
				best_dist = dist;
				best_x = x;
				best_y = y;
			}
		}
	}

	return {static_cast<float>(best_x), static_cast<float>(best_y)};
}

void GLCanvasWarpEditor::ResizeSelectedWarp(float dx, float dy)
{
	if (m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Warp) {
		if (dx != 0.0f) {
			m_canvas.m_pending_add_warp_height = GLCanvasObjectSupport::ValidWarpHeight(m_canvas.m_pending_add_warp_height, m_canvas.m_pending_add_warp_width);
			m_canvas.m_pending_add_warp_width = GLCanvasObjectSupport::ValidWarpWidth(m_canvas.m_pending_add_warp_width + dx, m_canvas.m_pending_add_warp_height);
		}
		if (dy != 0.0f) {
			m_canvas.m_pending_add_warp_width = GLCanvasObjectSupport::ValidWarpWidth(m_canvas.m_pending_add_warp_width, m_canvas.m_pending_add_warp_height);
			m_canvas.m_pending_add_warp_height = GLCanvasObjectSupport::ValidWarpHeight(m_canvas.m_pending_add_warp_height + dy, m_canvas.m_pending_add_warp_width);
		}
		m_canvas.Refresh();
		return;
	}
	if (m_canvas.m_selected_warp_idx < 0 || m_canvas.m_selected_warp_idx >= static_cast<int>(m_canvas.m_warps.size())) {
		return;
	}
	WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)];
	if (dx != 0.0f) {
		warp.height = GLCanvasObjectSupport::ValidWarpHeight(warp.height, warp.width);
		warp.width = GLCanvasObjectSupport::ValidWarpWidth(warp.width + dx, warp.height);
	}
	if (dy != 0.0f) {
		warp.width = GLCanvasObjectSupport::ValidWarpWidth(warp.width, warp.height);
		warp.height = GLCanvasObjectSupport::ValidWarpHeight(warp.height + dy, warp.width);
	}
	GLCanvasObjectSupport::ClampWarpToValidSize(warp);
	m_canvas.UpdateWarpFloor(warp);
	if (warp.warp_key != 0) {
		for (auto& other : m_canvas.m_warps) {
			if (other.instance_id != warp.instance_id && other.warp_key == warp.warp_key) {
				other.width = warp.width;
				other.height = warp.height;
				m_canvas.UpdateWarpFloor(other);
			}
		}
	}
}

void GLCanvasWarpEditor::RotateSelectedWarp(float dx, float dy)
{
	if (m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Warp) {
		return;
	}
	if (m_canvas.m_selected_warp_idx < 0 || m_canvas.m_selected_warp_idx >= static_cast<int>(m_canvas.m_warps.size())) {
		return;
	}
	WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)];
	warp.x = std::clamp(warp.x + dx, 0.0f, 63.5f);
	warp.y = std::clamp(warp.y + dy, 0.0f, 63.5f);
	m_canvas.UpdateWarpFloor(warp);
}

void GLCanvasWarpEditor::CycleSelectedWarpType(int delta)
{
	if (m_canvas.m_pending_add_type == MyGLCanvas::PendingObjectAddType::Warp) {
		int type = static_cast<int>(m_canvas.m_pending_add_warp_type);
		type = (type + delta + 3) % 3;
		m_canvas.m_pending_add_warp_type = static_cast<Landstalker::WarpList::Warp::Type>(type);
		m_canvas.Refresh();
		return;
	}
	if (m_canvas.m_selected_warp_idx < 0 || m_canvas.m_selected_warp_idx >= static_cast<int>(m_canvas.m_warps.size())) {
		return;
	}
	WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)];
	int type = static_cast<int>(warp.warp.type);
	type = (type + delta + 3) % 3;
	warp.warp.type = static_cast<Landstalker::WarpList::Warp::Type>(type);
	if (warp.warp_key != 0) {
		for (auto& other : m_canvas.m_warps) {
			if (other.instance_id != warp.instance_id && other.warp_key == warp.warp_key) {
				other.warp.type = warp.warp.type;
			}
		}
	}
}

void GLCanvasWarpEditor::RenderPendingWarpGhost()
{
	WarpInstance ghost{};
	if (!m_canvas.BuildPendingWarpPreviewInstance(ghost)) {
		return;
	}
	glUseProgram(0);
	for (int i = 0; i <= 5; ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glDisable(GL_TEXTURE_2D);
	}
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	auto quad = WarpQuad(ghost);
	glColor4f(1.0f, 0.95f, 0.0f, 0.35f);
	glBegin(GL_QUADS);
	for (const auto& point : quad) {
		glVertex2f(point.x, point.y);
	}
	glEnd();
	glColor4f(1.0f, 0.95f, 0.0f, 0.9f);
	glLineWidth(2.5f);
	glBegin(GL_LINE_LOOP);
	for (const auto& point : quad) {
		glVertex2f(point.x, point.y);
	}
	glEnd();
	glLineWidth(1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void GLCanvasWarpEditor::RenderWarps()
{
	glUseProgram(0);
	for (int i = 0; i <= 5; ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glDisable(GL_TEXTURE_2D);
	}
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (std::size_t i = 0; i < m_canvas.m_warps.size(); ++i) {
		const WarpInstance& warp = m_canvas.m_warps[i];
		auto quad = WarpQuad(warp);
		if (static_cast<int>(i) == m_canvas.m_selected_warp_idx) {
			auto selected_quad = WarpQuad(warp, -0.08f);
			glLineWidth(3.0f);
			glColor4f(1.0f, 0.0f, 0.0f, 0.95f);
			glBegin(GL_LINE_LOOP);
			for (const auto& point : selected_quad) {
				glVertex2f(point.x, point.y);
			}
			glEnd();
		}

		if (warp.DestinationRoom() == 0xFFFF) {
			glColor4f(1.0f, 0.05f, 0.05f, 0.95f);
		} else {
			switch (warp.warp.type) {
				case Landstalker::WarpList::Warp::Type::STAIR_SE:
					glColor4f(0.0f, 0.95f, 1.0f, 0.95f);
					break;
				case Landstalker::WarpList::Warp::Type::STAIR_SW:
					glColor4f(0.1f, 1.0f, 0.1f, 0.95f);
					break;
				case Landstalker::WarpList::Warp::Type::NORMAL:
				default:
					glColor4f(1.0f, 0.95f, 0.0f, 0.95f);
					break;
			}
		}

		glLineWidth(1.0f);
		glBegin(GL_LINE_LOOP);
		for (const auto& point : quad) {
			glVertex2f(point.x, point.y);
		}
		glEnd();

		if (static_cast<int>(i) == m_canvas.m_selected_warp_idx) {
			for (int axis = 1; axis <= 2; ++axis) {
				PickRect rect = WarpResizeControlRect(warp, axis);
				bool usable = WarpResizeAxisUsable(warp, axis);
				glColor4f(0.02f, 0.02f, 0.02f, 0.6f);
				glBegin(GL_QUADS);
				glVertex2f(rect.min_x, rect.min_y);
				glVertex2f(rect.max_x, rect.min_y);
				glVertex2f(rect.max_x, rect.max_y);
				glVertex2f(rect.min_x, rect.max_y);
				glEnd();

				if (!usable) {
					glColor4f(0.8f, 0.1f, 0.1f, 0.75f);
				} else if (axis == 1) {
					glColor4f(1.0f, 0.95f, 0.1f, 0.95f);
				} else {
					glColor4f(0.0f, 0.95f, 1.0f, 0.95f);
				}
				glLineWidth(2.0f);
				glBegin(GL_LINE_LOOP);
				glVertex2f(rect.min_x, rect.min_y);
				glVertex2f(rect.max_x, rect.min_y);
				glVertex2f(rect.max_x, rect.max_y);
				glVertex2f(rect.min_x, rect.max_y);
				glEnd();
				if (!usable) {
					glBegin(GL_LINES);
					glVertex2f(rect.min_x + 2.0f, rect.min_y + 2.0f);
					glVertex2f(rect.max_x - 2.0f, rect.max_y - 2.0f);
					glVertex2f(rect.max_x - 2.0f, rect.min_y + 2.0f);
					glVertex2f(rect.min_x + 2.0f, rect.max_y - 2.0f);
					glEnd();
				}
				glLineWidth(1.0f);
			}
		}
	}

	glLineWidth(1.0f);
}

void GLCanvasWarpEditor::RenderSelectedWarpTooltip()
{
	if (m_canvas.m_selected_warp_idx < 0 || m_canvas.m_selected_warp_idx >= static_cast<int>(m_canvas.m_warps.size())) {
		return;
	}

	const WarpInstance& warp = m_canvas.m_warps[static_cast<std::size_t>(m_canvas.m_selected_warp_idx)];
	PickPoint anchor = ProjectWarpGridPoint(warp, warp.x + warp.width, warp.y + warp.height, warp.floor_z);

	auto fmt = [](float value) {
		std::ostringstream out;
		out << std::fixed << std::setprecision(0) << value;
		return out.str();
	};

	std::array<std::string, 4> lines = {
		std::string{"ID:"} + std::to_string(warp.instance_id),
		std::string{"X:"} + fmt(warp.x),
		std::string{"Y:"} + fmt(warp.y),
		std::string{"D:"} + HexWord(warp.DestinationRoom())
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
	float x = anchor.x * zoom + m_canvas.m_cam_x + 48.0f;
	float y = anchor.y * zoom + m_canvas.m_cam_y - 34.0f;

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

	if (warp.DestinationRoom() == 0xFFFF) {
		glColor4f(1.0f, 0.4f, 0.4f, 0.95f);
	} else {
		glColor4f(0.9f, 0.9f, 0.88f, 0.95f);
	}
	glLineWidth(1.0f);
	for (std::size_t i = 0; i < lines.size(); ++i) {
		DrawOverlayText(lines[i], x, y + float(i) * line_height, scale);
	}
}
