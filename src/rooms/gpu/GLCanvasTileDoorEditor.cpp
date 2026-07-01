#include "GLCanvasTileDoorEditor.h"
#include "GLLoader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <set>

#include "GLCanvasObjectSupport.h"
#include "PixelFont.h"

using GLCanvasObjectSupport::TileSwapRegionPart;

namespace {

struct PickPoint {
	float x;
	float y;
};

struct PickRect {
	float min_x;
	float min_y;
	float max_x;
	float max_y;
};

PickRect RectAroundPoint(const PickPoint& point, float half_size)
{
	return {
		point.x - half_size,
		point.y - half_size,
		point.x + half_size,
		point.y + half_size
	};
}

PickRect TileSwapRegionResizeControlRect(const GLCanvasObjectSupport::TileSwapRegionGeometry& region)
{
	return RectAroundPoint({region.resize_handle.x, region.resize_handle.y}, 6.0f);
}

PickPoint ProjectRoomGridPoint(float x, float y, float z, float room_left, float room_top, float z_extent = 32.0f)
{
	float grid_x = x - room_left;
	float grid_y = y - room_top;
	return {
		32.0f * grid_x - 32.0f * grid_y + 512.0f,
		16.0f * grid_x + 16.0f * grid_y + 100.0f - z_extent * z
	};
}

PickPoint ProjectHeightmapGridPoint(float x, float y, float z, float room_left, float room_top, float z_extent)
{
	float grid_x = x - room_left + 12.0f;
	float grid_y = y - room_top + 12.0f;
	return {
		32.0f * grid_x - 32.0f * grid_y + 512.0f,
		16.0f * grid_x + 16.0f * grid_y + 100.0f - z_extent * z
	};
}

PickRect BoundsForPoints(const std::vector<PickPoint>& points)
{
	PickRect bounds{
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest()
	};
	for (const auto& point : points) {
		bounds.min_x = std::min(bounds.min_x, point.x);
		bounds.min_y = std::min(bounds.min_y, point.y);
		bounds.max_x = std::max(bounds.max_x, point.x);
		bounds.max_y = std::max(bounds.max_y, point.y);
	}
	return bounds;
}

const char* TileSwapPartLabel(TileSwapRegionPart part)
{
	switch (part) {
		case TileSwapRegionPart::TilemapSource:
			return "MAP SRC";
		case TileSwapRegionPart::TilemapDestination:
			return "MAP DST";
		case TileSwapRegionPart::HeightmapSource:
			return "HM SRC";
		case TileSwapRegionPart::HeightmapDestination:
			return "HM DST";
	}
	return "";
}

bool TileSwapMapDestinationContains(const Landstalker::TileSwap& swap, Landstalker::Tilemap3D::Layer layer, int rel_x, int rel_y)
{
	switch (swap.mode) {
		case Landstalker::TileSwap::Mode::FLOOR:
			return rel_x >= 0 && rel_x < swap.map.width &&
				   rel_y >= 0 && rel_y < swap.map.height;
		case Landstalker::TileSwap::Mode::WALL_NE:
			if (layer == Landstalker::Tilemap3D::Layer::BG) {
				return rel_x - rel_y >= 0 && rel_x - rel_y < swap.map.width &&
					   rel_y >= 0 && rel_y < swap.map.height;
			}
			return rel_x - rel_y - 1 >= 0 && rel_x - rel_y - 1 < swap.map.width &&
				   rel_y >= 0 && rel_y < swap.map.height;
		case Landstalker::TileSwap::Mode::WALL_NW:
			if (layer == Landstalker::Tilemap3D::Layer::BG) {
				return rel_x >= 0 && rel_x < swap.map.height &&
					   rel_y > rel_x && rel_y <= rel_x + swap.map.width;
			}
			return rel_x >= 0 && rel_x < swap.map.height &&
				   rel_y >= rel_x && rel_y < rel_x + swap.map.width;
	}
	return false;
}

void DrawTileSwapPreviewClipped(const Landstalker::TileSwap& swap, Landstalker::Tilemap3D& tilemap, Landstalker::Tilemap3D::Layer layer)
{
	const int width = tilemap.GetWidth();
	const int height = tilemap.GetHeight();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int map_x = x + tilemap.GetLeft();
			int map_y = y + tilemap.GetTop();
			int rel_x = map_x - swap.map.dst_x;
			int rel_y = map_y - swap.map.dst_y;
			if (!TileSwapMapDestinationContains(swap, layer, rel_x, rel_y)) {
				continue;
			}

			int src_map_x = swap.map.src_x + rel_x;
			int src_map_y = swap.map.src_y + rel_y;
			int src_x = src_map_x - tilemap.GetLeft();
			int src_y = src_map_y - tilemap.GetTop();
			uint16_t block = 0;
			if (src_x >= 0 && src_y >= 0 && src_x < width && src_y < height) {
				block = tilemap.GetBlock({src_x, src_y}, layer);
			}
			tilemap.SetBlock({block, {x, y}}, layer);
		}
	}
}

void DrawHeightmapSwapPreviewClipped(const Landstalker::TileSwap& swap, Landstalker::Tilemap3D& tilemap)
{
	for (int y = 0; y < tilemap.GetHeightmapHeight(); ++y) {
		for (int x = 0; x < tilemap.GetHeightmapWidth(); ++x) {
			int rel_x = x - swap.heightmap.dst_x;
			int rel_y = y - swap.heightmap.dst_y;
			if (rel_x < 0 || rel_y < 0 ||
				rel_x >= swap.heightmap.width || rel_y >= swap.heightmap.height) {
				continue;
			}

			Landstalker::HMPoint2D src(rel_x + swap.heightmap.src_x, rel_y + swap.heightmap.src_y);
			if (tilemap.IsHMPointValid(src)) {
				tilemap.SetHeightmapCell({x, y}, tilemap.GetHeightmapCell(src));
			}
		}
	}
}

std::string HexByte(uint8_t value)
{
	constexpr char digits[] = "0123456789ABCDEF";
	std::string out;
	out.push_back(digits[(value >> 4) & 0x0F]);
	out.push_back(digits[value & 0x0F]);
	return out;
}

void DrawOverlayGlyph(char c, float x, float y, float scale)
{
	const auto* glyph = PixelFont::Glyph(c);
	if (!glyph) {
		return;
	}

	glBegin(GL_QUADS);
	for (int row = 0; row < PixelFont::kGlyphHeight; ++row) {
		uint8_t bits = (*glyph)[row];
		for (int col = 0; col < PixelFont::kGlyphWidth; ++col) {
			uint8_t mask = static_cast<uint8_t>(1u << (PixelFont::kGlyphWidth - 1 - col));
			if ((bits & mask) == 0) {
				continue;
			}

			float px = x + float(col) * scale;
			float py = y + float(row) * scale;
			glVertex2f(px, py);
			glVertex2f(px + scale, py);
			glVertex2f(px + scale, py + scale);
			glVertex2f(px, py + scale);
		}
	}
	glEnd();
}

void DrawOverlayText(const std::string& text, float x, float y, float scale)
{
	constexpr float glyph_advance = 6.0f;
	x = std::round(x);
	y = std::round(y);
	for (std::size_t i = 0; i < text.size(); ++i) {
		DrawOverlayGlyph(text[i], x + float(i) * glyph_advance * scale, y, scale);
	}
}

void DrawDashedClosedPolyline(const std::vector<PickPoint>& points, float dash_len = 8.0f, float gap_len = 5.0f)
{
	if (points.size() < 2) {
		return;
	}
	glBegin(GL_LINES);
	for (std::size_t i = 0; i < points.size(); ++i) {
		PickPoint a = points[i];
		PickPoint b = points[(i + 1) % points.size()];
		float dx = b.x - a.x;
		float dy = b.y - a.y;
		float len = std::sqrt(dx * dx + dy * dy);
		if (len <= 0.0001f) {
			continue;
		}
		float ux = dx / len;
		float uy = dy / len;
		for (float pos = 0.0f; pos < len; pos += dash_len + gap_len) {
			float end = std::min(pos + dash_len, len);
			glVertex2f(a.x + ux * pos, a.y + uy * pos);
			glVertex2f(a.x + ux * end, a.y + uy * end);
		}
	}
	glEnd();
}

void DrawClosedPolyline(const std::vector<PickPoint>& points)
{
	if (points.size() < 2) {
		return;
	}
	glBegin(GL_LINE_LOOP);
	for (const auto& point : points) {
		glVertex2f(point.x, point.y);
	}
	glEnd();
}

void DrawSegments(const std::vector<PickPoint>& points)
{
	if (points.size() < 2) {
		return;
	}
	glBegin(GL_LINES);
	for (std::size_t i = 0; i + 1 < points.size(); i += 2) {
		glVertex2f(points[i].x, points[i].y);
		glVertex2f(points[i + 1].x, points[i + 1].y);
	}
	glEnd();
}

void DrawDashedSegments(const std::vector<PickPoint>& points, float dash_len = 8.0f, float gap_len = 5.0f)
{
	if (points.size() < 2) {
		return;
	}
	glBegin(GL_LINES);
	for (std::size_t i = 0; i + 1 < points.size(); i += 2) {
		PickPoint a = points[i];
		PickPoint b = points[i + 1];
		float dx = b.x - a.x;
		float dy = b.y - a.y;
		float len = std::sqrt(dx * dx + dy * dy);
		if (len <= 0.0001f) {
			continue;
		}
		float ux = dx / len;
		float uy = dy / len;
		for (float pos = 0.0f; pos < len; pos += dash_len + gap_len) {
			float end = std::min(pos + dash_len, len);
			glVertex2f(a.x + ux * pos, a.y + uy * pos);
			glVertex2f(a.x + ux * end, a.y + uy * end);
		}
	}
	glEnd();
}

}  // namespace

GLCanvasTileDoorEditor::GLCanvasTileDoorEditor(MyGLCanvas& canvas)
	: m_canvas(canvas)
{
}

void GLCanvasTileDoorEditor::ClearTileSwapPreview()
{
	// Previews temporarily mutate render state; this restores canonical room data.
	if (!m_canvas.m_tileswap_preview_active && !m_canvas.m_door_preview_active && !m_canvas.m_tileswap_preview_map) {
		return;
	}

	m_canvas.m_tileswap_preview_active = false;
	m_canvas.m_tileswap_preview_swap_index = -1;
	m_canvas.m_door_preview_active = false;
	m_canvas.m_door_preview_idx = -1;
	m_canvas.m_tileswap_preview_map.reset();
	m_canvas.m_heightmapRenderer.ClearPreviewMap();
	if (m_canvas.m_initialized) {
		m_canvas.m_mapRenderer.LoadRoom(m_canvas.m_current_room);
		m_canvas.RefreshObjectPlacementsFromHeightmap();
	}
}

void GLCanvasTileDoorEditor::CycleSelectedDoorSize(int delta)
{
	ClearTileSwapPreview();
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

void GLCanvasTileDoorEditor::AddDoor()
{
	ClearTileSwapPreview();
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

void GLCanvasTileDoorEditor::RenderPendingDoorGhost()
{
	if (m_canvas.m_pending_add_type != MyGLCanvas::PendingObjectAddType::Door) {
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

void GLCanvasTileDoorEditor::ToggleSelectedDoorPreview()
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
		ClearTileSwapPreview();
		return;
	}

	ClearTileSwapPreview();

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

void GLCanvasTileDoorEditor::RenderSelectedDoorTooltip()
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
		std::string{"ID:"} + HexByte(static_cast<uint8_t>(std::clamp(m_canvas.m_selected_door_idx + 1, 0, 255))),
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

	std::vector<PickPoint> all_points;
	all_points.reserve(region.points.size() + region.fill_points.size());
	for (const auto& p : region.points) {
		all_points.push_back({p.x, p.y});
	}
	for (const auto& p : region.fill_points) {
		all_points.push_back({p.x, p.y});
	}
	if (all_points.empty()) {
		return;
	}
	PickRect bounds = BoundsForPoints(all_points);

	std::array<std::string, 6> lines = {
		std::string{"ID:"} + HexByte(swap.trigger),
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
