#ifndef GL_CANVAS_DOOR_TILE_SWAP_SUPPORT_H
#define GL_CANVAS_DOOR_TILE_SWAP_SUPPORT_H

#include "GLCanvas.h"
#include "GLLoader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <set>

#include "GLCanvasObjectSupport.h"
#include "PixelFont.h"
#include "RoomProjection.h"

#include <landstalker/misc/Utils.h>

using GLCanvasObjectSupport::TileSwapRegionPart;

namespace {

using PickPoint = RoomProjection::PickPoint;
using PickRect = GLCanvasObjectSupport::PickRect;
using DoorGeometry = GLCanvasObjectSupport::DoorGeometry;
using GLCanvasObjectSupport::BoundsForPoints;
using GLCanvasObjectSupport::BuildDoorGeometries;
using PixelFont::DrawOverlayText;
using RoomProjection::ProjectHeightmapGridPoint;
using RoomProjection::ProjectRoomGridPoint;
using RoomProjection::ScreenToHeightmapPoint;
using RoomProjection::ScreenToMapPoint;

PickRect RectAroundPoint(const PickPoint& point, float half_size)
{
	return {
		point.x - half_size,
		point.y - half_size,
		point.x + half_size,
		point.y + half_size
	};
}

bool PointInRect(const PickPoint& point, const PickRect& rect)
{
	return point.x >= rect.min_x &&
		   point.x <= rect.max_x &&
		   point.y >= rect.min_y &&
		   point.y <= rect.max_y;
}

PickRect TileSwapRegionResizeControlRect(const GLCanvasObjectSupport::TileSwapRegionGeometry& region)
{
	return RectAroundPoint({region.resize_handle.x, region.resize_handle.y}, 6.0f);
}

float DistanceToSegment(const PickPoint& point, const PickPoint& a, const PickPoint& b)
{
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	float len_sq = dx * dx + dy * dy;
	if (len_sq <= 0.0001f) {
		float px = point.x - a.x;
		float py = point.y - a.y;
		return std::sqrt(px * px + py * py);
	}
	float t = std::clamp(((point.x - a.x) * dx + (point.y - a.y) * dy) / len_sq, 0.0f, 1.0f);
	float closest_x = a.x + t * dx;
	float closest_y = a.y + t * dy;
	float px = point.x - closest_x;
	float py = point.y - closest_y;
	return std::sqrt(px * px + py * py);
}

bool PointNearPolyline(const PickPoint& point, const std::vector<PickPoint>& points, float threshold)
{
	if (points.size() < 2) {
		return false;
	}
	for (std::size_t i = 0; i < points.size(); ++i) {
		if (DistanceToSegment(point, points[i], points[(i + 1) % points.size()]) <= threshold) {
			return true;
		}
	}
	return false;
}

bool PointNearSegments(const PickPoint& point, const std::vector<PickPoint>& points, float threshold)
{
	if (points.size() < 2) {
		return false;
	}
	for (std::size_t i = 0; i + 1 < points.size(); i += 2) {
		if (DistanceToSegment(point, points[i], points[i + 1]) <= threshold) {
			return true;
		}
	}
	return false;
}

bool PointInPolygon(const PickPoint& point, const std::vector<PickPoint>& polygon)
{
	if (polygon.size() < 3) {
		return false;
	}
	bool inside = false;
	for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
		const PickPoint& a = polygon[i];
		const PickPoint& b = polygon[j];
		bool crosses = ((a.y > point.y) != (b.y > point.y)) &&
			(point.x < (b.x - a.x) * (point.y - a.y) / ((b.y - a.y) == 0.0f ? 0.0001f : (b.y - a.y)) + a.x);
		if (crosses) {
			inside = !inside;
		}
	}
	return inside;
}

bool PointInPolygonWinding(const PickPoint& point, const std::vector<PickPoint>& polygon)
{
	if (polygon.size() < 3) {
		return false;
	}
	int winding = 0;
	for (std::size_t i = 0; i < polygon.size(); ++i) {
		const PickPoint& a = polygon[i];
		const PickPoint& b = polygon[(i + 1) % polygon.size()];
		float cross = (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);
		if (a.y <= point.y) {
			if (b.y > point.y && cross > 0.0f) {
				++winding;
			}
		} else if (b.y <= point.y && cross < 0.0f) {
			--winding;
		}
	}
	return winding != 0;
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

#endif  // GL_CANVAS_DOOR_TILE_SWAP_SUPPORT_H
