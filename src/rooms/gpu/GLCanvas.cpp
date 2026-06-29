#include "GLCanvas.h"
#include "GLLoader.h"
#include "GLCanvasObjectSupport.h"
#include "PixelFont.h"
#include <rooms/EntityControlFrame.h>
#include <rooms/RoomViewerFrame.h>
#include <rooms/TileSwapControlFrame.h>
#include <rooms/WarpControlFrame.h>
#include <main/EditorFrame.h>
#include <wx/dcclient.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <utility>
#include <wx/log.h>
#include <landstalker/misc/Utils.h>

using namespace Landstalker;

namespace {
// File-local rendering/math helpers used only by GLCanvas.
int GLCanvasAttributes[] = {
    WX_GL_RGBA,
    WX_GL_DOUBLEBUFFER,
    WX_GL_STENCIL_SIZE, 8,
    0
};

float HitboxBaseToBlocks(uint8_t base) {
    return float(base) / 8.0f;
}

float HitboxHeightToBlocks(uint8_t height) {
    return float(height) / 16.0f;
}

float HitboxDrawOffset(float hitbox_base) {
    return hitbox_base < 1.5f ? 0.0f : 0.5f;
}

float EntityFrontDepthKey(const SpriteInstance& inst) {
    float center_x = inst.map_x + inst.hitbox_offset;
    float center_y = inst.map_y + inst.hitbox_offset;
    float half_base = inst.hitbox_base * 0.5f;
    return center_x + center_y + half_base * 2.0f;
}

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

const char* OcclusionModeName(int idx) {
    static constexpr const char* names[] = {"TOP", "GHOST", "HIDE"};
    return names[idx % 3];
}

std::pair<GLint, GLint> EntityOcclusionDebugDepthRange(const SpriteInstance& inst)
{
    float center_x = inst.map_x + inst.hitbox_offset;
    float center_y = inst.map_y + inst.hitbox_offset;
    float half_base = inst.hitbox_base * 0.5f;
    float min_depth = (center_x - half_base) + (center_y - half_base);
    float max_depth = (center_x + half_base) + (center_y + half_base) + 25.0f;
    int rear_edge_padding = std::abs(inst.map_z - inst.floor_z) <= 0.01f ? 1 : 0;
    return {
        std::clamp(static_cast<int>(std::ceil(min_depth)) + rear_edge_padding, 0, 255),
        std::clamp(static_cast<int>(std::ceil(max_depth)), 0, 255)
    };
}

struct RoomInfoRow {
    std::string label;
    std::string value;
    uint16_t room;
};

std::string HexFlagWord(uint16_t value)
{
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << value;
    return out.str();
}

std::string RoomDisplayText(const std::shared_ptr<GameData>& gd, uint16_t room)
{
    auto rd = gd ? gd->GetRoomData() : nullptr;
    if (!rd || room >= rd->GetRoomCount()) {
        return {};
    }

    std::ostringstream out;
    out << room << " " << wstr_to_utf8(rd->GetRoomDisplayName(room));
    return out.str();
}

std::vector<RoomInfoRow> BuildRoomInfoRows(const std::shared_ptr<GameData>& gd, uint16_t room)
{
    std::vector<RoomInfoRow> rows;
    auto rd = gd ? gd->GetRoomData() : nullptr;
    if (!rd || room >= rd->GetRoomCount()) {
        return rows;
    }

    auto add_destination_row = [&](const std::string& label, uint16_t destination_room) {
        if (destination_room >= rd->GetRoomCount()) {
            return;
        }
        rows.push_back({label, RoomDisplayText(gd, destination_room), destination_room});
    };

    if (rd->HasFallDestination(room)) {
        add_destination_row("Fall Destination", rd->GetFallDestination(room));
    }
    if (rd->HasClimbDestination(room)) {
        add_destination_row("Climb Destination", rd->GetClimbDestination(room));
    }

    for (const auto& transition : rd->GetTransitions(room)) {
        uint16_t other_room = transition.src_rm == room ? transition.dst_rm : transition.src_rm;
        if (other_room >= rd->GetRoomCount()) {
            continue;
        }

        rows.push_back({
            std::string{"Transition when Flag "} + HexFlagWord(transition.flag) + " is " + (transition.src_rm == room ? "SET" : "CLEAR"),
            RoomDisplayText(gd, other_room),
            other_room
        });
    }

    return rows;
}

float WheelSteps(const wxMouseEvent& evt) {
    int delta = evt.GetWheelDelta();
    if (delta == 0) {
        return evt.GetWheelRotation() > 0 ? 1.0f : -1.0f;
    }
    return static_cast<float>(evt.GetWheelRotation()) / static_cast<float>(delta);
}

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

enum class TileSwapRegionPart {
    TilemapSource = 0,
    TilemapDestination = 1,
    HeightmapSource = 2,
    HeightmapDestination = 3
};

GLCanvasObjectSupport::TileSwapRegionPart ToSupportTileSwapRegionPart(TileSwapRegionPart part)
{
    switch (part) {
        case TileSwapRegionPart::TilemapSource:
            return GLCanvasObjectSupport::TileSwapRegionPart::TilemapSource;
        case TileSwapRegionPart::TilemapDestination:
            return GLCanvasObjectSupport::TileSwapRegionPart::TilemapDestination;
        case TileSwapRegionPart::HeightmapSource:
            return GLCanvasObjectSupport::TileSwapRegionPart::HeightmapSource;
        case TileSwapRegionPart::HeightmapDestination:
            return GLCanvasObjectSupport::TileSwapRegionPart::HeightmapDestination;
    }
    return GLCanvasObjectSupport::TileSwapRegionPart::TilemapSource;
}

TileSwapRegionPart FromSupportTileSwapRegionPart(GLCanvasObjectSupport::TileSwapRegionPart part)
{
    switch (part) {
        case GLCanvasObjectSupport::TileSwapRegionPart::TilemapSource:
            return TileSwapRegionPart::TilemapSource;
        case GLCanvasObjectSupport::TileSwapRegionPart::TilemapDestination:
            return TileSwapRegionPart::TilemapDestination;
        case GLCanvasObjectSupport::TileSwapRegionPart::HeightmapSource:
            return TileSwapRegionPart::HeightmapSource;
        case GLCanvasObjectSupport::TileSwapRegionPart::HeightmapDestination:
            return TileSwapRegionPart::HeightmapDestination;
    }
    return TileSwapRegionPart::TilemapSource;
}

struct TileSwapRegionGeometry {
    int flat_index;
    int swap_index;
    TileSwapRegionPart part;
    TileSwap swap;
    std::vector<PickPoint> points;
    std::vector<PickPoint> fill_points;
    PickRect bounds;
    bool segments;
    PickPoint resize_handle;
};

struct TileSwapRegionMetrics {
    int x;
    int y;
    int width;
    int height;
};

struct DoorGeometry {
    int index;
    Door door;
    bool valid;
    std::vector<PickPoint> cell_points;
    std::vector<PickPoint> map_points;
    PickRect bounds;
};

PickPoint ProjectEntityGridPoint(const SpriteInstance& inst, float x, float y, float z)
{
    float grid_x = x - inst.room_left;
    float grid_y = y - inst.room_top;
    return {
        32.0f * grid_x - 32.0f * grid_y + 512.0f,
        16.0f * grid_x + 16.0f * grid_y + 100.0f - inst.z_extent * z
    };
}

PickPoint ProjectWarpGridPoint(const WarpInstance& warp, float x, float y, float z);

PickPoint ScreenToMapPoint(float world_x, float world_y, float z, float room_left, float room_top, float z_extent = 32.0f)
{
    float a = (world_x - 512.0f) / 32.0f;
    float b = (world_y - 100.0f + z_extent * z) / 16.0f;
    return {
        (a + b) * 0.5f + room_left,
        (b - a) * 0.5f + room_top
    };
}

PickRect EntityZControlRect(const SpriteInstance& inst)
{
    float center_x = inst.map_x + inst.hitbox_offset;
    float center_y = inst.map_y + inst.hitbox_offset;
    float top_z = inst.map_z + std::max(inst.hitbox_height, 0.125f);
    PickPoint top_center = ProjectEntityGridPoint(inst, center_x, center_y, top_z);
    constexpr float half_size = 6.0f;
    constexpr float y_offset = 14.0f;
    return {
        top_center.x - half_size,
        top_center.y - y_offset - half_size,
        top_center.x + half_size,
        top_center.y - y_offset + half_size
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

TileSwapRegionMetrics MetricsForTileSwapRegion(const TileSwap& swap, TileSwapRegionPart part)
{
    auto metrics = GLCanvasObjectSupport::MetricsForTileSwapRegion(swap, ToSupportTileSwapRegionPart(part));
    return {metrics.x, metrics.y, metrics.width, metrics.height};
}

PickPoint PolygonEdgeHandle(const std::vector<PickPoint>& points, int axis)
{
    if (points.empty()) {
        return {0.0f, 0.0f};
    }
    if (points.size() == 1) {
        return points.front();
    }

    float best_key = std::numeric_limits<float>::lowest();
    PickPoint best{0.0f, 0.0f};
    for (std::size_t i = 0; i < points.size(); ++i) {
        const PickPoint& a = points[i];
        const PickPoint& b = points[(i + 1) % points.size()];
        PickPoint midpoint{(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f};
        float key = axis == 1 ? midpoint.x : midpoint.y;
        if (key > best_key) {
            best_key = key;
            best = midpoint;
        }
    }
    return best;
}

float ValidWarpWidth(float requested_width, float current_height)
{
    return GLCanvasObjectSupport::ValidWarpWidth(requested_width, current_height);
}

float ValidWarpHeight(float requested_height, float current_width)
{
    return GLCanvasObjectSupport::ValidWarpHeight(requested_height, current_width);
}

void ClampWarpToValidSize(WarpInstance& warp)
{
    GLCanvasObjectSupport::ClampWarpToValidSize(warp);
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

std::string HexByte(uint8_t value)
{
    constexpr char digits[] = "0123456789ABCDEF";
    std::string out;
    out.push_back(digits[(value >> 4) & 0x0F]);
    out.push_back(digits[value & 0x0F]);
    return out;
}

PickPoint ProjectWarpGridPoint(const WarpInstance& warp, float x, float y, float z)
{
    float grid_x = x - warp.room_left;
    float grid_y = y - warp.room_top;
    return {
        32.0f * grid_x - 32.0f * grid_y + 512.0f,
        16.0f * grid_x + 16.0f * grid_y + 100.0f - warp.z_extent * z
    };
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

PickPoint ScreenToHeightmapPoint(float world_x, float world_y, float room_left, float room_top)
{
    return ScreenToMapPoint(world_x, world_y, 0.0f, room_left - 12.0f, room_top - 12.0f);
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

const char* TileSwapShapeLabel(TileSwap::Mode mode)
{
    switch (mode) {
        case TileSwap::Mode::FLOOR:
            return "FLOOR";
        case TileSwap::Mode::WALL_NE:
            return "WNE";
        case TileSwap::Mode::WALL_NW:
            return "WNW";
    }
    return "UNK";
}

std::vector<TileSwapRegionGeometry> BuildTileSwapRegionGeometries(
    const std::shared_ptr<GameData>& gd,
    uint16_t room,
    const MapRenderer& map_renderer,
    float z_extent)
{
    std::vector<TileSwapRegionGeometry> out;
    auto shared = GLCanvasObjectSupport::BuildTileSwapRegionGeometries(gd, room, map_renderer, z_extent);
    out.reserve(shared.size());
    for (const auto& region : shared) {
        TileSwapRegionGeometry local{};
        local.flat_index = region.flat_index;
        local.swap_index = region.swap_index;
        local.part = FromSupportTileSwapRegionPart(region.part);
        local.swap = region.swap;
        local.points.reserve(region.points.size());
        for (const auto& point : region.points) {
            local.points.push_back({point.x, point.y});
        }
        local.fill_points.reserve(region.fill_points.size());
        for (const auto& point : region.fill_points) {
            local.fill_points.push_back({point.x, point.y});
        }
        local.bounds = local.points.empty() ? PickRect{0.0f, 0.0f, 0.0f, 0.0f} : BoundsForPoints(local.points);
        if (!local.fill_points.empty()) {
            PickRect fill_bounds = BoundsForPoints(local.fill_points);
            local.bounds.min_x = std::min(local.bounds.min_x, fill_bounds.min_x);
            local.bounds.min_y = std::min(local.bounds.min_y, fill_bounds.min_y);
            local.bounds.max_x = std::max(local.bounds.max_x, fill_bounds.max_x);
            local.bounds.max_y = std::max(local.bounds.max_y, fill_bounds.max_y);
        }
        local.segments = region.segments;
        local.resize_handle = {region.resize_handle.x, region.resize_handle.y};
        out.push_back(std::move(local));
    }
    return out;
}

std::vector<DoorGeometry> BuildDoorGeometries(
    const std::shared_ptr<GameData>& gd,
    uint16_t room,
    const MapRenderer& map_renderer,
    float z_extent,
    const std::shared_ptr<const Tilemap3D>& override_tilemap)
{
    std::vector<DoorGeometry> out;
    auto rd = gd ? gd->GetRoomData() : nullptr;
    if (!rd) {
        return out;
    }
    auto doors = rd->GetDoors(room);
    if (doors.empty()) {
        return out;
    }

    std::shared_ptr<const Tilemap3D> tilemap = override_tilemap;
    if (!tilemap) {
        auto map_entry = rd->GetMapForRoom(room);
        tilemap = map_entry ? map_entry->GetData() : nullptr;
    }
    if (!tilemap) {
        return out;
    }

    const float room_left = static_cast<float>(map_renderer.GetRoomLeft());
    const float room_top = static_cast<float>(map_renderer.GetRoomTop());

    auto height_at = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= tilemap->GetHeightmapWidth() || y >= tilemap->GetHeightmapHeight()) {
            return 0.0f;
        }
        uint8_t z = tilemap->GetHeight({x, y});
        return z == 0xFF ? 0.0f : static_cast<float>(z);
    };

    auto offset = [](const PickPoint& point, float x, float y) {
        return PickPoint{point.x + x, point.y + y};
    };

    for (std::size_t i = 0; i < doors.size(); ++i) {
        const Door& door = doors[i];
        int x = static_cast<int>(door.x);
        int y = static_cast<int>(door.y);
        PickPoint center = ProjectHeightmapGridPoint(
            static_cast<float>(x) + 0.5f,
            static_cast<float>(y) + 0.5f,
            height_at(x, y),
            room_left,
            room_top,
            z_extent);

        DoorGeometry geom{};
        geom.index = static_cast<int>(i);
        geom.door = door;
        geom.cell_points = {
            offset(center, 0.0f, -16.0f),
            offset(center, 32.0f, 0.0f),
            offset(center, 0.0f, 16.0f),
            offset(center, -32.0f, 0.0f)
        };

        auto [valid, poly] = door.GetMapRegionPoly(tilemap, 1, 2);
        geom.valid = valid;
        auto tile_offset = door.GetTileOffset(tilemap, Tilemap3D::Layer::BG);
        PickPoint anchor = ProjectRoomGridPoint(
            room_left + static_cast<float>(tile_offset.first),
            room_top + static_cast<float>(tile_offset.second),
            0.0f,
            room_left,
            room_top);
        geom.map_points.reserve(poly.size());
        for (const auto& point : poly) {
            geom.map_points.push_back({
                anchor.x + static_cast<float>(point.first) * 32.0f,
                anchor.y + static_cast<float>(point.second) * 16.0f
            });
        }

        geom.bounds = BoundsForPoints(geom.cell_points);
        if (!geom.map_points.empty()) {
            PickRect map_bounds = BoundsForPoints(geom.map_points);
            geom.bounds.min_x = std::min(geom.bounds.min_x, map_bounds.min_x);
            geom.bounds.min_y = std::min(geom.bounds.min_y, map_bounds.min_y);
            geom.bounds.max_x = std::max(geom.bounds.max_x, map_bounds.max_x);
            geom.bounds.max_y = std::max(geom.bounds.max_y, map_bounds.max_y);
        }
        out.push_back(std::move(geom));
    }

    return out;
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

bool PointInEntityHitbox(const SpriteInstance& inst, const PickPoint& point, bool include_shadow = true)
{
    if (inst.hitbox_base <= 0.0f) {
        return false;
    }

    float center_x = inst.map_x + inst.hitbox_offset;
    float center_y = inst.map_y + inst.hitbox_offset;
    float half_base = inst.hitbox_base * 0.5f;
    float min_x = center_x - half_base;
    float min_y = center_y - half_base;
    float max_x = center_x + half_base;
    float max_y = center_y + half_base;
    float bottom_z = inst.map_z;
    float height = std::max(inst.hitbox_height, 0.125f);
    std::array<PickPoint, 4> bottom = {
        ProjectEntityGridPoint(inst, min_x, min_y, bottom_z),
        ProjectEntityGridPoint(inst, max_x, min_y, bottom_z),
        ProjectEntityGridPoint(inst, max_x, max_y, bottom_z),
        ProjectEntityGridPoint(inst, min_x, max_y, bottom_z)
    };
    std::array<PickPoint, 4> top = {
        ProjectEntityGridPoint(inst, min_x, min_y, bottom_z + height),
        ProjectEntityGridPoint(inst, max_x, min_y, bottom_z + height),
        ProjectEntityGridPoint(inst, max_x, max_y, bottom_z + height),
        ProjectEntityGridPoint(inst, min_x, max_y, bottom_z + height)
    };
    std::array<PickPoint, 4> shadow = {
        ProjectEntityGridPoint(inst, min_x, min_y, inst.floor_z),
        ProjectEntityGridPoint(inst, max_x, min_y, inst.floor_z),
        ProjectEntityGridPoint(inst, max_x, max_y, inst.floor_z),
        ProjectEntityGridPoint(inst, min_x, max_y, inst.floor_z)
    };

    if (PointInQuad(point, bottom) || PointInQuad(point, top) || (include_shadow && PointInQuad(point, shadow))) {
        return true;
    }

    for (std::size_t i = 0; i < bottom.size(); ++i) {
        std::array<PickPoint, 4> side = {
            bottom[i],
            bottom[(i + 1) % bottom.size()],
            top[(i + 1) % top.size()],
            top[i]
        };
        if (PointInQuad(point, side)) {
            return true;
        }
    }
    return false;
}

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

bool PointInWarp(const WarpInstance& warp, const PickPoint& point)
{
    return PointInQuad(point, WarpQuad(warp));
}

WarpInstance MakeWarpInstance(
    const Landstalker::WarpList::Warp& warp,
    uint16_t current_room,
    uint32_t instance_id,
    float room_left,
    float room_top,
    float z_extent = 32.0f,
    uint32_t warp_key = 0,
    int side_override = 0)
{
    return GLCanvasObjectSupport::MakeWarpInstance(
        warp,
        current_room,
        instance_id,
        room_left,
        room_top,
        z_extent,
        warp_key,
        side_override);
}

bool EntityDrawOrder(const SpriteInstance& lhs, const SpriteInstance& rhs) {
    const float lhs_depth = EntityFrontDepthKey(lhs);
    const float rhs_depth = EntityFrontDepthKey(rhs);
    if (lhs_depth != rhs_depth) {
        return lhs_depth < rhs_depth;
    }

    if (std::abs(lhs.map_z - rhs.map_z) > 0.001f) {
        return lhs.map_z > rhs.map_z;
    }

    if (lhs.map_y != rhs.map_y) {
        return lhs.map_y < rhs.map_y;
    }

    return lhs.map_x < rhs.map_x;
}

bool EntityMustDrawBefore(const SpriteInstance& lhs, const SpriteInstance& rhs) {
    constexpr float epsilon = 0.001f;
    EntityBounds lhs_bounds = GetEntityBounds(lhs);
    EntityBounds rhs_bounds = GetEntityBounds(rhs);

    bool lhs_behind = lhs_bounds.max_x <= rhs_bounds.min_x + epsilon ||
                      lhs_bounds.max_y <= rhs_bounds.min_y + epsilon;
    bool rhs_behind = rhs_bounds.max_x <= lhs_bounds.min_x + epsilon ||
                      rhs_bounds.max_y <= lhs_bounds.min_y + epsilon;
    if (lhs_behind != rhs_behind) {
        return lhs_behind;
    }

    bool footprints_overlap = lhs_bounds.min_x < rhs_bounds.max_x - epsilon &&
                              lhs_bounds.max_x > rhs_bounds.min_x + epsilon &&
                              lhs_bounds.min_y < rhs_bounds.max_y - epsilon &&
                              lhs_bounds.max_y > rhs_bounds.min_y + epsilon;
    bool z_ranges_overlap = lhs_bounds.min_z < rhs_bounds.max_z - epsilon &&
                            lhs_bounds.max_z > rhs_bounds.min_z + epsilon;
    if (footprints_overlap && z_ranges_overlap &&
        std::abs(lhs_bounds.front_depth - rhs_bounds.front_depth) > epsilon) {
        return lhs_bounds.front_depth < rhs_bounds.front_depth;
    }
    if (footprints_overlap && !z_ranges_overlap && std::abs(lhs.map_z - rhs.map_z) > epsilon) {
        return lhs.map_z > rhs.map_z;
    }

    if (std::abs(lhs_bounds.front_depth - rhs_bounds.front_depth) > epsilon) {
        return lhs_bounds.front_depth < rhs_bounds.front_depth;
    }

    if (std::abs(lhs.map_z - rhs.map_z) > epsilon) {
        return lhs.map_z > rhs.map_z;
    }

    return EntityDrawOrder(lhs, rhs);
}

void SortEntitiesGeometrically(std::vector<SpriteInstance>& instances) {
    GLCanvasObjectSupport::SortEntitiesGeometrically(instances);
}

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

void DrawStencilOverlay(float cam_x, float cam_y, int width, int height, GLint ref, GLint mask, float r, float g, float b, float a)
{
    // Draw a fullscreen tint only where the stencil test passes.
    // We lock stencil writes (mask 0x00) so this pass only reads stencil state.
    glUseProgram(0);
    for (int i = 0; i <= 5; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0x00);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_EQUAL, ref, mask);
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(-cam_x, -cam_y);
    glVertex2f(float(width) - cam_x, -cam_y);
    glVertex2f(float(width) - cam_x, float(height) - cam_y);
    glVertex2f(-cam_x, float(height) - cam_y);
    glEnd();
    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);
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

}

#include "GLCanvasEntityEditor.h"
#include "GLCanvasWarpEditor.h"
#include "GLCanvasTileDoorEditor.h"
#include "GLCanvasObjectCoordinator.h"
#include "GLCanvasHeightmapMode.h"
#include "GLCanvasLayerEditMode.h"
#include "GLCanvasRoomMode.h"

wxDEFINE_EVENT(EVT_GPU_EDITOR_MODE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_GPU_LAYER_OPACITY_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_GPU_LAYER_BLOCK_SELECT, wxCommandEvent);

wxBEGIN_EVENT_TABLE(MyGLCanvas, wxGLCanvas)
    EVT_PAINT(MyGLCanvas::OnPaint)
    EVT_TIMER(wxID_ANY, MyGLCanvas::OnTimer)
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
wxEND_EVENT_TABLE()

MyGLCanvas::MyGLCanvas(wxWindow* parent, std::shared_ptr<GameData> gd)
    : wxGLCanvas(parent, wxID_ANY, GLCanvasAttributes, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS),
    m_gd(gd), m_context(nullptr), m_mapRenderer(gd), m_heightmapRenderer(gd), m_spriteRenderer(gd),
      m_frame_count(0), m_fps(0.0f), m_current_room(0), m_cam_x(0.0f), m_cam_y(0.0f),
      m_alpha(false), m_show_heightmap(false), m_show_entities(true), m_show_warps(true), m_show_tile_swaps(true),
      m_hovered_entity_idx(-1), m_selected_entity_idx(-1), m_hovered_warp_idx(-1), m_selected_warp_idx(-1),
      m_hovered_tileswap_region_idx(-1), m_selected_tileswap_region_idx(-1),
      m_hovered_door_idx(-1), m_selected_door_idx(-1),
      m_dragging_entity(false), m_drag_z_axis_only(false), m_drag_instance_id(0),
      m_drag_start_x(0.0f), m_drag_start_y(0.0f), m_drag_start_z(0.0f),
      m_drag_plane_z(0.0f), m_drag_cursor_offset_x(0.0f), m_drag_cursor_offset_y(0.0f), m_drag_floor_snap(false),
      m_dragging_warp(false), m_drag_warp_instance_id(0), m_drag_warp_resize_axis(0),
      m_drag_warp_start_x(0.0f), m_drag_warp_start_y(0.0f),
      m_drag_warp_start_width(0.0f), m_drag_warp_start_height(0.0f), m_drag_warp_start_floor_z(0.0f),
      m_dragging_door(false), m_drag_door_idx(-1), m_drag_door_start_x(0), m_drag_door_start_y(0),
      m_dragging_tileswap_region(false), m_drag_tileswap_region_idx(-1), m_drag_tileswap_resize_axis(0),
      m_drag_tileswap_start_x(0), m_drag_tileswap_start_y(0), m_drag_tileswap_start_width(1), m_drag_tileswap_start_height(1),
    m_dragging_pan(false), m_drag_pan_start_mouse(wxDefaultPosition), m_drag_pan_start_cam_x(0.0f), m_drag_pan_start_cam_y(0.0f),
    m_bg_opacity_idx(0), m_fg_opacity_idx(0), m_sprite_opacity_idx(0), m_entity_occlusion_idx(1),
        m_debug_occlusion(false), m_show_hitboxes(true), m_editor_mode(EditorMode::Room), m_heightmap_view_mode(HeightmapViewMode::Flat), m_non_heightmap_z_extent(32.0f), m_foreground_show_background_underlay(true), m_background_show_block_ids(false), m_layer_priority_highlight(true),
            m_background_has_selection(false), m_background_selected_x(0), m_background_selected_y(0),
            m_background_has_hover(false), m_background_hover_x(0), m_background_hover_y(0),
            m_background_clipboard_valid(false), m_background_clipboard_block_id(0),
            m_heightmap_clipboard_valid(false), m_heightmap_clipboard_cell(0),
            m_tileswap_preview_active(false), m_tileswap_preview_swap_index(-1),
      m_door_preview_active(false), m_door_preview_idx(-1),
      m_timer(this), m_pending_warp_half(false), m_pending_warp_room(0xFFFF), m_pending_warp_instance_id(0),
    m_entity_clipboard_valid(false), m_zoom_step_idx(1), m_gl_init_failed(false), m_initialized(false), m_last_mouse_pos(wxDefaultPosition)
{
    m_current_room = 0;
    m_fps_stopwatch.Start();
    m_timer.Start(1000.0 / 60.0);
}

MyGLCanvas::~MyGLCanvas() {
    if (m_gd) {
        PersistCurrentRoomEdits();
    }
    delete m_context;
}

void MyGLCanvas::SetRoomNum(uint16_t roomnum) {
    if (m_initialized && m_current_room == roomnum) {
        Refresh();
        return;
    }
    if (!m_initialized) {
        m_current_room = roomnum;
        Refresh();
        return;
    }
    LoadRoom(roomnum);
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

void MyGLCanvas::LoadRoom(uint16_t roomnum) {
    LoadRoomFromGameData(roomnum, true, true);
}

void MyGLCanvas::NavigateToRoom(uint16_t roomnum) {
    if (!m_gd) {
        return;
    }
    LoadRoom(roomnum);
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
    m_tileswap_preview_active = false;
    m_tileswap_preview_swap_index = -1;
    m_door_preview_active = false;
    m_door_preview_idx = -1;
    m_tileswap_preview_map.reset();
    m_heightmapRenderer.ClearPreviewMap();
    m_current_room = roomnum;
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
        m_heightmap_clipboard_valid = false;
        m_heightmap_clipboard_cell = 0;
    }
    m_dragging_entity = false;
    m_dragging_warp = false;
    m_dragging_door = false;
    m_dragging_tileswap_region = false;
    SetCursor(wxCursor(wxCURSOR_ARROW));
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
    m_room_stopwatch.Start();
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
    auto regions = BuildTileSwapRegionGeometries(m_gd, m_current_room, m_mapRenderer, m_heightmapRenderer.GetZExtent());
    if (m_selected_tileswap_region_idx < 0 ||
        m_selected_tileswap_region_idx >= static_cast<int>(regions.size())) {
        return;
    }
    TileSwapRegionMetrics metrics = MetricsForTileSwapRegion(
        regions[static_cast<std::size_t>(m_selected_tileswap_region_idx)].swap,
        regions[static_cast<std::size_t>(m_selected_tileswap_region_idx)].part);
    ResizeSelectedTileSwapRegion(
        dw == 0 ? 0.0f : static_cast<float>(metrics.width + dw),
        dh == 0 ? 0.0f : static_cast<float>(metrics.height + dh));
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
    evt.SetString(wxString::Format("MODE: %s | CUR: %d,%d | FPS: %.2f | Entities: %zu | Room: %d (%ls) | Cam: %.0f, %.0f | HM: %s %.0f | BG: %.1f FG: %.1f SPR: %.1f OCC: %s DBG: %s BOX: %s",
        mode_name, cursor_x, cursor_y,
        m_fps, m_instances.size(), m_current_room, name.c_str(), m_cam_x, m_cam_y,
        m_show_heightmap ? "ON" : "OFF", m_heightmapRenderer.GetZExtent(),
        OpacityForIndex(m_bg_opacity_idx), OpacityForIndex(m_fg_opacity_idx), OpacityForIndex(m_sprite_opacity_idx),
        OcclusionModeName(m_entity_occlusion_idx), m_debug_occlusion ? "ON" : "OFF", m_show_hitboxes ? "ON" : "OFF"));
    evt.SetInt(0);
    evt.SetClientData(target);
    wxPostEvent(target, evt);
}

void MyGLCanvas::RenderStencilOverlay(int width, int height, GLint ref, GLint mask, float r, float g, float b, float a) const {
    DrawStencilOverlay(m_cam_x, m_cam_y, width, height, ref, mask, r, g, b, a);
}

std::set<uint32_t> MyGLCanvas::FindCollidedEntityIds() const {
    return FindCollidedEntities(m_instances);
}

void MyGLCanvas::OnTimer(wxTimerEvent&) {
    if (!m_initialized || !m_gd || !IsShownOnScreen()) {
        return;
    }

    // Recompute floors/projections periodically instead of every paint.
    // This avoids heavy per-frame work, which is especially noticeable at high zoom.
    if ((m_frame_count & 0x07) == 0) {
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
            inst.anim_timer += inst.anim_speed / 60.0f * 12.0f; 
            if (inst.anim_timer >= frames.size()) inst.anim_timer -= frames.size(); 
        }
    }
    m_frame_count++;
    if (m_fps_stopwatch.Time() >= 1000) {
        m_fps = (m_frame_count*1000.0f)/m_fps_stopwatch.Time();
        m_frame_count = 0;
        m_fps_stopwatch.Start();
    }
    UpdateStatusBar();
    Refresh();
}

bool MyGLCanvas::HandleKeyDown(wxKeyEvent& evt) {
    if (!IsAnyEditMode() && evt.GetKeyCode() == WXK_RETURN) {
        if (OpenSelectedObjectProperties()) {
            return true;
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
            int old_idx = std::clamp(m_zoom_step_idx, 0, static_cast<int>(kZoomSteps.size()) - 1);
            int new_idx = std::clamp(old_idx + delta, 0, static_cast<int>(kZoomSteps.size()) - 1);
            if (new_idx != old_idx) {
                float old_zoom = kZoomSteps[static_cast<std::size_t>(old_idx)];
                float new_zoom = kZoomSteps[static_cast<std::size_t>(new_idx)];
                float anchor_x = static_cast<float>(evt.GetPosition().x);
                float anchor_y = static_cast<float>(evt.GetPosition().y);
                float world_x = (anchor_x - m_cam_x) / old_zoom;
                float world_y = (anchor_y - m_cam_y) / old_zoom;
                m_zoom_step_idx = new_idx;
                m_cam_x = anchor_x - world_x * new_zoom;
                m_cam_y = anchor_y - world_y * new_zoom;
                m_cam_x = std::round(m_cam_x);
                m_cam_y = std::round(m_cam_y);
            }
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
    if (m_dragging_entity) {
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
    if (m_dragging_entity || m_dragging_warp || m_dragging_door || m_dragging_tileswap_region || m_dragging_pan) {
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
    if (entity_idx < 0 || entity_idx >= static_cast<int>(m_instances.size())) {
        return;
    }

    SpriteInstance& inst = m_instances[static_cast<std::size_t>(entity_idx)];
    m_dragging_entity = true;
    m_drag_z_axis_only = z_axis_only;
    m_drag_instance_id = inst.instance_id;
    m_drag_start_mouse = evt.GetPosition();
    m_drag_start_x = inst.map_x;
    m_drag_start_y = inst.map_y;
    m_drag_start_z = inst.map_z;
    m_drag_plane_z = shadow_drag ? inst.floor_z : inst.map_z;
    PickPoint cursor_map = ScreenToMapPoint(
        ScreenToWorldX(evt.GetPosition().x),
        ScreenToWorldY(evt.GetPosition().y),
        m_drag_plane_z,
        inst.room_left,
        inst.room_top,
        inst.z_extent);
    m_drag_cursor_offset_x = inst.map_x + inst.hitbox_offset - cursor_map.x;
    m_drag_cursor_offset_y = inst.map_y + inst.hitbox_offset - cursor_map.y;
    m_drag_floor_snap = std::abs(inst.map_z - inst.floor_z) <= 0.01f;
    SetCursor(wxCursor(z_axis_only ? wxCURSOR_SIZENS : wxCURSOR_HAND));
    if (!HasCapture()) {
        CaptureMouse();
    }
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

void MyGLCanvas::ApplyHeightmapViewMode() {
    switch (m_heightmap_view_mode) {
        case HeightmapViewMode::Flat:
            m_heightmapRenderer.SetZExtent(0.0f);
            break;
        case HeightmapViewMode::Raised:
            m_heightmapRenderer.SetZExtent(10.0f);
            break;
        case HeightmapViewMode::Full:
        case HeightmapViewMode::FullWithTilemap:
            m_heightmapRenderer.SetZExtent(32.0f);
            break;
    }
}

void MyGLCanvas::SetEditorMode(EditorMode mode) {
    if (m_editor_mode == mode) {
        return;
    }

    if (m_editor_mode != EditorMode::Heightmap && mode == EditorMode::Heightmap) {
        m_non_heightmap_z_extent = m_heightmapRenderer.GetZExtent();
        ApplyHeightmapViewMode();
    } else if (m_editor_mode == EditorMode::Heightmap && mode != EditorMode::Heightmap) {
        m_heightmapRenderer.SetZExtent(m_non_heightmap_z_extent);
    }

    m_editor_mode = mode;
    if (IsAnyEditMode()) {
        if (m_background_has_selection) {
            ClampBackgroundSelection();
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

bool MyGLCanvas::SelectHeightmapCellAt(const wxPoint& point) {
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

    m_background_selected_x = hover_x;
    m_background_selected_y = hover_y;
    m_background_has_selection = true;
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

bool MyGLCanvas::SelectBackgroundCellAt(const wxPoint& point) {
    int cell_x = -1;
    int cell_y = -1;
    if (!BackgroundCellAt(point, cell_x, cell_y)) {
        return false;
    }

    m_background_selected_x = cell_x;
    m_background_selected_y = cell_y;
    m_background_has_selection = true;
    NotifyLayerBlockSelected();
    return true;
}

void MyGLCanvas::ClampBackgroundSelection() {
    auto map = CurrentRoomMap();
    int width = IsHeightmapEditMode() && map ? map->GetHeightmapWidth() : m_mapRenderer.GetRoomWidth();
    int height = IsHeightmapEditMode() && map ? map->GetHeightmapHeight() : m_mapRenderer.GetRoomHeight();
    if (width <= 0 || height <= 0) {
        m_background_has_selection = false;
        m_background_selected_x = 0;
        m_background_selected_y = 0;
        return;
    }
    m_background_selected_x = std::clamp(m_background_selected_x, 0, width - 1);
    m_background_selected_y = std::clamp(m_background_selected_y, 0, height - 1);
    m_background_has_selection = true;
}

void MyGLCanvas::MoveBackgroundSelection(int dx, int dy) {
    if (!m_background_has_selection) {
        m_background_selected_x = 0;
        m_background_selected_y = 0;
        m_background_has_selection = true;
    }
    m_background_selected_x += dx;
    m_background_selected_y += dy;
    ClampBackgroundSelection();
    NotifyLayerBlockSelected();
}

std::shared_ptr<Tilemap3D> MyGLCanvas::CurrentRoomMap() const {
    auto rd = m_gd ? m_gd->GetRoomData() : nullptr;
    if (!rd) {
        return nullptr;
    }
    auto map_entry = rd->GetMapForRoom(m_current_room);
    return map_entry ? map_entry->GetData() : nullptr;
}

int MyGLCanvas::SelectedBackgroundBlockIndex() const {
    if (!m_background_has_selection || m_background_selected_x < 0 || m_background_selected_y < 0) {
        return -1;
    }
    if (m_background_selected_x >= m_mapRenderer.GetRoomWidth() || m_background_selected_y >= m_mapRenderer.GetRoomHeight()) {
        return -1;
    }
    return m_background_selected_y * m_mapRenderer.GetRoomWidth() + m_background_selected_x;
}

int MyGLCanvas::SelectedHeightmapCellX() const {
    auto map = CurrentRoomMap();
    if (!map || !m_background_has_selection) {
        return -1;
    }
    if (m_background_selected_x < 0 || m_background_selected_x >= map->GetHeightmapWidth()) {
        return -1;
    }
    return m_background_selected_x;
}

int MyGLCanvas::SelectedHeightmapCellY() const {
    auto map = CurrentRoomMap();
    if (!map || !m_background_has_selection) {
        return -1;
    }
    if (m_background_selected_y < 0 || m_background_selected_y >= map->GetHeightmapHeight()) {
        return -1;
    }
    return m_background_selected_y;
}

uint16_t MyGLCanvas::SelectedHeightmapCellValue() const {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    if (!map || x < 0 || y < 0) {
        return 0;
    }
    return map->GetHeightmapCell({x, y});
}

bool MyGLCanvas::HasSelectedLayerCell() const {
    return SelectedBackgroundBlockIndex() >= 0;
}

bool MyGLCanvas::HasSelectedHeightmapCell() const {
    return SelectedHeightmapCellX() >= 0 && SelectedHeightmapCellY() >= 0;
}

bool MyGLCanvas::CanInsertSelectedHeightmapRow() const {
    auto map = CurrentRoomMap();
    return HasSelectedHeightmapCell() && map && map->GetHeightmapWidth() < 64;
}

bool MyGLCanvas::CanInsertSelectedHeightmapColumn() const {
    auto map = CurrentRoomMap();
    return HasSelectedHeightmapCell() && map && map->GetHeightmapHeight() < 64;
}

bool MyGLCanvas::CanDeleteSelectedHeightmapRow() const {
    auto map = CurrentRoomMap();
    return HasSelectedHeightmapCell() && map && map->GetHeightmapWidth() > 1;
}

bool MyGLCanvas::CanDeleteSelectedHeightmapColumn() const {
    auto map = CurrentRoomMap();
    return HasSelectedHeightmapCell() && map && map->GetHeightmapHeight() > 1;
}

bool MyGLCanvas::CanIncreaseSelectedHeightmapHeight() const {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    return map && x >= 0 && y >= 0 && map->GetHeight({x, y}) < 15;
}

bool MyGLCanvas::CanDecreaseSelectedHeightmapHeight() const {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    return map && x >= 0 && y >= 0 && map->GetHeight({x, y}) > 0;
}

bool MyGLCanvas::CanDeleteSelectedTilemapRow() const {
    auto map = CurrentRoomMap();
    return HasSelectedLayerCell() && map && map->GetWidth() > 1;
}

bool MyGLCanvas::CanDeleteSelectedTilemapColumn() const {
    auto map = CurrentRoomMap();
    return HasSelectedLayerCell() && map && map->GetHeight() > 1;
}

uint8_t MyGLCanvas::GetSelectedHeightmapType() const {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    return map && x >= 0 && y >= 0 ? map->GetCellType({x, y}) : 0;
}

bool MyGLCanvas::IsSelectedHeightmapPlayerPassable() const {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    return map && x >= 0 && y >= 0 && (map->GetCellProps({x, y}) & 0x04) == 0;
}

bool MyGLCanvas::IsSelectedHeightmapNpcPassable() const {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    return map && x >= 0 && y >= 0 && (map->GetCellProps({x, y}) & 0x02) == 0;
}

bool MyGLCanvas::IsSelectedHeightmapRaftTrack() const {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    return map && x >= 0 && y >= 0 && (map->GetCellProps({x, y}) & 0x01) == 0;
}

void MyGLCanvas::SetSelectedHeightmapType(uint8_t type) {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    if (!map || x < 0 || y < 0) {
        return;
    }
    map->SetCellType({x, y}, type);
    ReloadCurrentRoomMapView();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::ToggleSelectedHeightmapPlayerPassable() {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    if (!map || x < 0 || y < 0) {
        return;
    }
    uint8_t props = map->GetCellProps({x, y});
    map->SetCellProps({x, y}, IsSelectedHeightmapPlayerPassable() ? (props | 0x04) : (props & ~0x04));
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::ToggleSelectedHeightmapNpcPassable() {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    if (!map || x < 0 || y < 0) {
        return;
    }
    uint8_t props = map->GetCellProps({x, y});
    map->SetCellProps({x, y}, IsSelectedHeightmapNpcPassable() ? (props | 0x02) : (props & ~0x02));
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::ToggleSelectedHeightmapRaftTrack() {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    if (!map || x < 0 || y < 0) {
        return;
    }
    uint8_t props = map->GetCellProps({x, y});
    map->SetCellProps({x, y}, IsSelectedHeightmapRaftTrack() ? (props | 0x01) : (props & ~0x01));
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::AdjustSelectedHeightmapHeight(int delta) {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    if (!map || x < 0 || y < 0) {
        return;
    }
    int height = std::clamp(static_cast<int>(map->GetHeight({x, y})) + delta, 0, 15);
    map->SetHeight({x, y}, static_cast<uint8_t>(height));
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

bool MyGLCanvas::CanNudgeHeightmap(int left_delta, int top_delta) const {
    auto map = CurrentRoomMap();
    if (!map) {
        return false;
    }
    int new_left = static_cast<int>(map->GetLeft()) + left_delta;
    int new_top = static_cast<int>(map->GetTop()) + top_delta;
    return new_left >= 0 && new_left <= 63 && new_top >= 0 && new_top <= 63;
}

void MyGLCanvas::NudgeHeightmap(int left_delta, int top_delta) {
    auto map = CurrentRoomMap();
    if (!map || !CanNudgeHeightmap(left_delta, top_delta)) {
        return;
    }
    map->SetLeft(static_cast<uint8_t>(static_cast<int>(map->GetLeft()) + left_delta));
    map->SetTop(static_cast<uint8_t>(static_cast<int>(map->GetTop()) + top_delta));
    ReloadCurrentRoomMapView();
    m_heightmapRenderer.LoadRoom(m_current_room);
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(true);
    Refresh();
}

void MyGLCanvas::InsertSelectedHeightmapRowBefore() {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    if (!map || x < 0 || map->GetHeightmapWidth() >= 64) {
        return;
    }
    map->InsertHeightmapRow(static_cast<uint8_t>(x));
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::InsertSelectedHeightmapRowAfter() {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    if (!map || x < 0 || map->GetHeightmapWidth() >= 64) {
        return;
    }
    map->InsertHeightmapRow(static_cast<uint8_t>(x));
    ++m_background_selected_x;
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::DeleteSelectedHeightmapRow() {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    if (!map || x < 0 || map->GetHeightmapWidth() <= 1) {
        return;
    }
    map->DeleteHeightmapRow(static_cast<uint8_t>(x));
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::InsertSelectedHeightmapColumnBefore() {
    auto map = CurrentRoomMap();
    int y = SelectedHeightmapCellY();
    if (!map || y < 0 || map->GetHeightmapHeight() >= 64) {
        return;
    }
    map->InsertHeightmapColumn(static_cast<uint8_t>(y));
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::InsertSelectedHeightmapColumnAfter() {
    auto map = CurrentRoomMap();
    int y = SelectedHeightmapCellY();
    if (!map || y < 0 || map->GetHeightmapHeight() >= 64) {
        return;
    }
    map->InsertHeightmapColumn(static_cast<uint8_t>(y));
    ++m_background_selected_y;
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

void MyGLCanvas::DeleteSelectedHeightmapColumn() {
    auto map = CurrentRoomMap();
    int y = SelectedHeightmapCellY();
    if (!map || y < 0 || map->GetHeightmapHeight() <= 1) {
        return;
    }
    map->DeleteHeightmapColumn(static_cast<uint8_t>(y));
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    Refresh();
}

uint16_t MyGLCanvas::SelectedBackgroundBlockId() const {
    auto map = CurrentRoomMap();
    int block_index = SelectedBackgroundBlockIndex();
    if (!map || block_index < 0) {
        return 0;
    }
    if (block_index >= map->GetWidth() * map->GetHeight()) {
        return 0;
    }
    return map->GetBlock(static_cast<uint16_t>(block_index), CurrentEditLayer()).value;
}

void MyGLCanvas::CopySelectedBackgroundBlock() {
    int block_index = SelectedBackgroundBlockIndex();
    auto map = CurrentRoomMap();
    if (!map || block_index < 0) {
        return;
    }
    if (block_index >= map->GetWidth() * map->GetHeight()) {
        return;
    }
    m_background_clipboard_block_id = map->GetBlock(static_cast<uint16_t>(block_index), CurrentEditLayer()).value;
    m_background_clipboard_valid = true;
}

void MyGLCanvas::SetSelectedBlockId(int block) {
    if (block < 0) {
        m_background_clipboard_valid = false;
        m_background_clipboard_block_id = 0;
        Refresh();
        return;
    }
    m_background_clipboard_block_id = static_cast<uint16_t>(block);
    m_background_clipboard_valid = true;
    Refresh();
}

void MyGLCanvas::CopySelectedHeightmapCell() {
    uint16_t value = SelectedHeightmapCellValue();
    if (!m_background_has_selection) {
        return;
    }
    m_heightmap_clipboard_cell = value;
    m_heightmap_clipboard_valid = true;
}

void MyGLCanvas::ClearBackgroundClipboard() {
    m_background_clipboard_valid = false;
    m_background_clipboard_block_id = 0;
    m_heightmap_clipboard_valid = false;
    m_heightmap_clipboard_cell = 0;
}

void MyGLCanvas::ReloadCurrentRoomMapView() {
    if (m_tileswap_preview_map) {
        m_mapRenderer.LoadPreviewRoom(m_current_room, *m_tileswap_preview_map);
    } else {
        m_mapRenderer.LoadRoom(m_current_room);
    }
}

void MyGLCanvas::PasteSelectedBackgroundBlock() {
    auto map = CurrentRoomMap();
    int block_index = SelectedBackgroundBlockIndex();
    if (!m_background_clipboard_valid || !map || block_index < 0) {
        return;
    }
    Tilemap3D::Layer layer = CurrentEditLayer();
    map->SetBlock(m_background_clipboard_block_id, static_cast<uint16_t>(block_index), layer);
    if (m_tileswap_preview_map) {
        m_tileswap_preview_map->SetBlock(m_background_clipboard_block_id, static_cast<uint16_t>(block_index), layer);
    }
    ReloadCurrentRoomMapView();
}

void MyGLCanvas::PasteSelectedHeightmapCell() {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    if (!m_heightmap_clipboard_valid || !map || x < 0 || y < 0) {
        return;
    }
    map->SetHeightmapCell({x, y}, m_heightmap_clipboard_cell);
    if (m_tileswap_preview_map) {
        m_tileswap_preview_map->SetHeightmapCell({x, y}, m_heightmap_clipboard_cell);
    }
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
}

void MyGLCanvas::ClearCurrentTilemap() {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }
    map->ClearTilemap();
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::InsertSelectedTilemapRowBefore() {
    auto map = CurrentRoomMap();
    if (!map || !HasSelectedLayerCell() || map->GetWidth() >= 64) {
        return;
    }
    map->InsertTilemapRow(m_background_selected_x);
    ++m_background_selected_x;
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::InsertSelectedTilemapRowAfter() {
    auto map = CurrentRoomMap();
    if (!map || !HasSelectedLayerCell() || map->GetWidth() >= 64) {
        return;
    }
    map->InsertTilemapRow(m_background_selected_x + 1);
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::DeleteSelectedTilemapRow() {
    auto map = CurrentRoomMap();
    if (!map || !CanDeleteSelectedTilemapRow()) {
        return;
    }
    map->DeleteTilemapRow(m_background_selected_x);
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::InsertSelectedTilemapColumnBefore() {
    auto map = CurrentRoomMap();
    if (!map || !HasSelectedLayerCell() || map->GetHeight() >= 64) {
        return;
    }
    map->InsertTilemapColumn(m_background_selected_y);
    ++m_background_selected_y;
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::InsertSelectedTilemapColumnAfter() {
    auto map = CurrentRoomMap();
    if (!map || !HasSelectedLayerCell() || map->GetHeight() >= 64) {
        return;
    }
    map->InsertTilemapColumn(m_background_selected_y + 1);
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::DeleteSelectedTilemapColumn() {
    auto map = CurrentRoomMap();
    if (!map || !CanDeleteSelectedTilemapColumn()) {
        return;
    }
    map->DeleteTilemapColumn(m_background_selected_y);
    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    Refresh();
}

void MyGLCanvas::RenderBackgroundEditorOverlay(int width, int height) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    float zoom = std::max(ZoomFactor(), 0.0001f);
    Tilemap3D::Layer layer = CurrentEditLayer();
    auto block_screen_origin = [this, zoom, layer](int x, int y) {
        float x_offset = layer == Tilemap3D::Layer::FG ? -32.0f : 0.0f;
        float px = 32.0f * static_cast<float>(x) - 32.0f * static_cast<float>(y) + 512.0f + x_offset;
        float py = 16.0f * static_cast<float>(x) + 16.0f * static_cast<float>(y) + 100.0f;
        return PickPoint{px * zoom + m_cam_x, py * zoom + m_cam_y};
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
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glLineWidth(1.0f);
    glColor4f(0.78f, 0.78f, 0.78f, 0.7f);
    glBegin(GL_LINES);
    for (int y = 0; y < m_mapRenderer.GetRoomHeight(); ++y) {
        for (int x = 0; x < m_mapRenderer.GetRoomWidth(); ++x) {
            PickPoint origin = block_screen_origin(x, y);
            float left = origin.x;
            float top = origin.y;
            float right = left + 32.0f * zoom;
            float bottom = top + 32.0f * zoom;
            glVertex2f(left, top);
            glVertex2f(right, top);
            glVertex2f(right, top);
            glVertex2f(right, bottom);
            glVertex2f(right, bottom);
            glVertex2f(left, bottom);
            glVertex2f(left, bottom);
            glVertex2f(left, top);
        }
    }
    glEnd();

    if (m_background_has_hover) {
        PickPoint origin = block_screen_origin(m_background_hover_x, m_background_hover_y);
        float left = origin.x;
        float top = origin.y;
        float right = left + 32.0f * zoom;
        float bottom = top + 32.0f * zoom;
        glColor4f(0.35f, 0.75f, 1.0f, 0.85f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(left, top);
        glVertex2f(right, top);
        glVertex2f(right, bottom);
        glVertex2f(left, bottom);
        glEnd();
    }

    if (m_background_has_selection) {
        PickPoint origin = block_screen_origin(m_background_selected_x, m_background_selected_y);
        float left = origin.x;
        float top = origin.y;
        float right = left + 32.0f * zoom;
        float bottom = top + 32.0f * zoom;
        glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(left, top);
        glVertex2f(right, top);
        glVertex2f(right, bottom);
        glVertex2f(left, bottom);
        glEnd();
    }

    if (m_background_show_block_ids) {
        constexpr float scale = 1.0f;
        constexpr float glyph_advance = 6.0f;
        constexpr float glyph_height = 7.0f;
        for (int y = 0; y < m_mapRenderer.GetRoomHeight(); ++y) {
            for (int x = 0; x < m_mapRenderer.GetRoomWidth(); ++x) {
                int block_index = y * m_mapRenderer.GetRoomWidth() + x;
                if (block_index < 0 || block_index >= map->GetWidth() * map->GetHeight()) {
                    continue;
                }
                std::string label = HexWord(map->GetBlock(static_cast<uint16_t>(block_index), layer).value);
                PickPoint origin = block_screen_origin(x, y);
                float label_width = static_cast<float>(label.size()) * glyph_advance * scale;
                float text_x = origin.x + (32.0f * zoom - label_width) * 0.5f;
                float text_y = origin.y + (32.0f * zoom - glyph_height * scale) * 0.5f;
                glColor4f(0.05f, 0.05f, 0.05f, 0.78f);
                glBegin(GL_QUADS);
                glVertex2f(text_x - 2.0f, text_y - 1.0f);
                glVertex2f(text_x + label_width + 2.0f, text_y - 1.0f);
                glVertex2f(text_x + label_width + 2.0f, text_y + glyph_height + 1.0f);
                glVertex2f(text_x - 2.0f, text_y + glyph_height + 1.0f);
                glEnd();
                glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
                DrawOverlayText(label, text_x, text_y, scale);
            }
        }
    }

    if (m_background_clipboard_valid) {
        int preview_x = -1;
        int preview_y = -1;
        if (m_background_has_hover) {
            preview_x = m_background_hover_x;
            preview_y = m_background_hover_y;
        } else if (m_background_has_selection) {
            preview_x = m_background_selected_x;
            preview_y = m_background_selected_y;
        }

        const int block_index = preview_y * m_mapRenderer.GetRoomWidth() + preview_x;
        const bool valid_preview_cell = preview_x >= 0 && preview_y >= 0 &&
            preview_x < m_mapRenderer.GetRoomWidth() && preview_y < m_mapRenderer.GetRoomHeight() &&
            block_index >= 0 && block_index < map->GetWidth() * map->GetHeight();
        const uint16_t current_block = valid_preview_cell ?
            map->GetBlock(static_cast<uint16_t>(block_index), layer).value : 0;

        if (valid_preview_cell && current_block != m_background_clipboard_block_id) {
            PickPoint origin = block_screen_origin(preview_x, preview_y);
            float left = origin.x;
            float top = origin.y;
            float right = left + 32.0f * zoom;
            float bottom = top + 32.0f * zoom;
            glColor4f(1.0f, 1.0f, 1.0f, 0.12f);
            glBegin(GL_QUADS);
            glVertex2f(left, top);
            glVertex2f(right, top);
            glVertex2f(right, bottom);
            glVertex2f(left, bottom);
            glEnd();
            glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
            DrawOverlayText(HexWord(m_background_clipboard_block_id), left + 2.0f, top + 2.0f, 1.0f);
        }
    }
}

void MyGLCanvas::RenderHeightmapEditorOverlay(int width, int height) {
    auto map = CurrentRoomMap();
    if (!map || !m_background_has_selection) {
        return;
    }

    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    if (x < 0 || y < 0) {
        return;
    }

    uint8_t z = map->GetHeight({x, y});
    float room_left = static_cast<float>(m_mapRenderer.GetRoomLeft());
    float room_top = static_cast<float>(m_mapRenderer.GetRoomTop());
    PickPoint center = ProjectHeightmapGridPoint(
        static_cast<float>(x) + 0.5f,
        static_cast<float>(y) + 0.5f,
        z,
        room_left,
        room_top,
        m_heightmapRenderer.GetZExtent());

    float zoom = std::max(ZoomFactor(), 0.0001f);
    float cx = center.x * zoom + m_cam_x;
    float cy = center.y * zoom + m_cam_y;

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
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor4f(1.0f, 1.0f, 1.0f, 0.98f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cx, cy - 16.0f * zoom);
    glVertex2f(cx + 32.0f * zoom, cy);
    glVertex2f(cx, cy + 16.0f * zoom);
    glVertex2f(cx - 32.0f * zoom, cy);
    glEnd();

}

void MyGLCanvas::UpdateEntityDrag(const wxMouseEvent& evt) {
    int entity_idx = FindInstanceIndex(m_drag_instance_id);
    if (entity_idx < 0) {
        EndEntityDrag();
        return;
    }

    SpriteInstance& inst = m_instances[static_cast<std::size_t>(entity_idx)];
    bool z_axis_only = m_drag_z_axis_only || evt.ControlDown() || evt.RightIsDown();
    SetCursor(wxCursor(z_axis_only ? wxCURSOR_SIZENS : wxCURSOR_HAND));
    auto snap_half = [](float value) {
        return std::round(value * 2.0f) * 0.5f;
    };
    auto clamp_map_pos = [](float value) {
        return std::clamp(value, 0.0f, 63.5f);
    };

    if (z_axis_only) {
        float dy = static_cast<float>(evt.GetPosition().y - m_drag_start_mouse.y);
        inst.map_x = clamp_map_pos(m_drag_start_x);
        inst.map_y = clamp_map_pos(m_drag_start_y);
        inst.map_z = std::clamp(snap_half(m_drag_start_z - dy / 32.0f), 0.0f, 15.5f);
    } else {
        float world_x = ScreenToWorldX(evt.GetPosition().x);
        float world_y = ScreenToWorldY(evt.GetPosition().y);
        PickPoint cursor_map = ScreenToMapPoint(world_x, world_y, m_drag_plane_z, inst.room_left, inst.room_top, inst.z_extent);
        float hitbox_center_x = cursor_map.x + m_drag_cursor_offset_x;
        float hitbox_center_y = cursor_map.y + m_drag_cursor_offset_y;
        inst.map_x = clamp_map_pos(snap_half(hitbox_center_x - inst.hitbox_offset));
        inst.map_y = clamp_map_pos(snap_half(hitbox_center_y - inst.hitbox_offset));
    }

    inst.floor_z = FloorUnderHitbox(
        inst.map_x + inst.hitbox_offset,
        inst.map_y + inst.hitbox_offset,
        inst.hitbox_base * 0.5f);
    if (!z_axis_only && m_drag_floor_snap) {
        inst.map_z = std::clamp(inst.floor_z, 0.0f, 15.5f);
    } else if (!z_axis_only) {
        inst.map_z = std::clamp(m_drag_start_z, 0.0f, 15.5f);
    }
    UpdateEntityProjection(inst);
    SortEntitiesGeometrically(m_instances);
    entity_idx = FindInstanceIndex(m_drag_instance_id);
    m_selected_entity_idx = entity_idx;
    m_hovered_entity_idx = entity_idx;
    Refresh();
}

void MyGLCanvas::EndEntityDrag() {
    if (!m_dragging_entity) {
        return;
    }

    uint32_t dragged_id = m_drag_instance_id;
    m_dragging_entity = false;
    if (HasCapture()) {
        ReleaseMouse();
    }
    SortEntitiesGeometrically(m_instances);
    m_selected_entity_idx = FindInstanceIndex(dragged_id);
    m_hovered_entity_idx = m_selected_entity_idx;
    SetCursor(wxCursor(m_hovered_entity_idx >= 0 ? wxCURSOR_HAND : wxCURSOR_ARROW));
    NotifyRoomDataChanged(true, false, false, false);
    Refresh();
}

void MyGLCanvas::StartWarpDrag(int warp_idx, const wxMouseEvent& evt) {
    if (warp_idx < 0 || warp_idx >= static_cast<int>(m_warps.size())) {
        return;
    }

    WarpInstance& warp = m_warps[static_cast<std::size_t>(warp_idx)];
    m_dragging_warp = true;
    m_drag_warp_instance_id = warp.instance_id;
    m_drag_warp_resize_axis = 0;
    m_drag_start_mouse = evt.GetPosition();
    m_drag_warp_start_x = warp.x;
    m_drag_warp_start_y = warp.y;
    m_drag_warp_start_width = warp.width;
    m_drag_warp_start_height = warp.height;
    m_drag_warp_start_floor_z = warp.floor_z;
    SetCursor(wxCursor(wxCURSOR_HAND));
    if (!HasCapture()) {
        CaptureMouse();
    }
}

void MyGLCanvas::StartWarpResizeDrag(int warp_idx, int axis, const wxMouseEvent& evt) {
    if (warp_idx < 0 || warp_idx >= static_cast<int>(m_warps.size())) {
        return;
    }

    WarpInstance& warp = m_warps[static_cast<std::size_t>(warp_idx)];
    m_dragging_warp = true;
    m_drag_warp_instance_id = warp.instance_id;
    m_drag_warp_resize_axis = axis;
    m_drag_start_mouse = evt.GetPosition();
    m_drag_warp_start_x = warp.x;
    m_drag_warp_start_y = warp.y;
    m_drag_warp_start_width = warp.width;
    m_drag_warp_start_height = warp.height;
    m_drag_warp_start_floor_z = warp.floor_z;
    m_selected_warp_idx = warp_idx;
    m_selected_entity_idx = -1;
    SetCursor(wxCursor(axis == 1 ? wxCURSOR_SIZENWSE : wxCURSOR_SIZENESW));
    if (!HasCapture()) {
        CaptureMouse();
    }
}

void MyGLCanvas::UpdateWarpDrag(const wxMouseEvent& evt) {
    int warp_idx = FindWarpIndex(m_drag_warp_instance_id);
    if (warp_idx < 0) {
        EndWarpDrag();
        return;
    }

    WarpInstance& warp = m_warps[static_cast<std::size_t>(warp_idx)];
    auto snap_cell = [](float value) {
        return std::round(value);
    };
    auto clamp_map_pos = [](float value) {
        return std::clamp(value, 0.0f, 63.5f);
    };

    float world_x = ScreenToWorldX(evt.GetPosition().x);
    float world_y = ScreenToWorldY(evt.GetPosition().y);
    float a = (world_x - 512.0f) / 32.0f;
    float b = (world_y - 100.0f + warp.z_extent * m_drag_warp_start_floor_z) / 16.0f;
    float center_x = (a + b) * 0.5f + warp.room_left;
    float center_y = (b - a) * 0.5f + warp.room_top;
    if (m_drag_warp_resize_axis == 1) {
        warp.x = m_drag_warp_start_x;
        warp.y = m_drag_warp_start_y;
        warp.height = ValidWarpHeight(m_drag_warp_start_height, m_drag_warp_start_width);
        warp.width = std::min(ValidWarpWidth(center_x - m_drag_warp_start_x, warp.height), 63.5f - m_drag_warp_start_x);
        SetCursor(wxCursor(wxCURSOR_SIZENWSE));
    } else if (m_drag_warp_resize_axis == 2) {
        warp.x = m_drag_warp_start_x;
        warp.y = m_drag_warp_start_y;
        warp.width = ValidWarpWidth(m_drag_warp_start_width, m_drag_warp_start_height);
        warp.height = std::min(ValidWarpHeight(center_y - m_drag_warp_start_y, warp.width), 63.5f - m_drag_warp_start_y);
        SetCursor(wxCursor(wxCURSOR_SIZENESW));
    } else {
        warp.x = clamp_map_pos(snap_cell(center_x - warp.width * 0.5f));
        warp.y = clamp_map_pos(snap_cell(center_y - warp.height * 0.5f));
        SetCursor(wxCursor(wxCURSOR_HAND));
    }
    UpdateWarpFloor(warp);

    m_selected_warp_idx = warp_idx;
    m_selected_entity_idx = -1;
    m_hovered_warp_idx = warp_idx;
    m_hovered_entity_idx = -1;
    Refresh();
}

void MyGLCanvas::EndWarpDrag() {
    if (!m_dragging_warp) {
        return;
    }

    uint32_t dragged_id = m_drag_warp_instance_id;
    m_dragging_warp = false;
    m_drag_warp_resize_axis = 0;
    if (HasCapture()) {
        ReleaseMouse();
    }
    m_selected_warp_idx = FindWarpIndex(dragged_id);
    m_hovered_warp_idx = m_selected_warp_idx;
    SetCursor(wxCursor(m_hovered_warp_idx >= 0 ? wxCURSOR_HAND : wxCURSOR_ARROW));
    NotifyRoomDataChanged(false, true, false, false);
    Refresh();
}

void MyGLCanvas::StartDoorDrag(int door_idx, const wxMouseEvent& evt) {
    auto rd = m_gd ? m_gd->GetRoomData() : nullptr;
    if (!rd) {
        return;
    }
    auto doors = rd->GetDoors(m_current_room);
    if (door_idx < 0 || door_idx >= static_cast<int>(doors.size())) {
        return;
    }

    ClearTileSwapPreview();
    const Door& door = doors[static_cast<std::size_t>(door_idx)];
    m_dragging_door = true;
    m_drag_door_idx = door_idx;
    m_drag_start_mouse = evt.GetPosition();
    m_drag_door_start_x = door.x;
    m_drag_door_start_y = door.y;
    m_selected_door_idx = door_idx;
    m_selected_entity_idx = -1;
    m_selected_warp_idx = -1;
    m_selected_tileswap_region_idx = -1;
    SetCursor(wxCursor(wxCURSOR_HAND));
    if (!HasCapture()) {
        CaptureMouse();
    }
}

void MyGLCanvas::UpdateDoorDrag(const wxMouseEvent& evt) {
    auto rd = m_gd ? m_gd->GetRoomData() : nullptr;
    if (!rd || m_drag_door_idx < 0) {
        EndDoorDrag();
        return;
    }
    auto doors = rd->GetDoors(m_current_room);
    if (m_drag_door_idx >= static_cast<int>(doors.size())) {
        EndDoorDrag();
        return;
    }

    PickPoint start = ScreenToHeightmapPoint(
        ScreenToWorldX(m_drag_start_mouse.x),
        ScreenToWorldY(m_drag_start_mouse.y),
        static_cast<float>(m_mapRenderer.GetRoomLeft()),
        static_cast<float>(m_mapRenderer.GetRoomTop()));
    PickPoint current = ScreenToHeightmapPoint(
        ScreenToWorldX(evt.GetPosition().x),
        ScreenToWorldY(evt.GetPosition().y),
        static_cast<float>(m_mapRenderer.GetRoomLeft()),
        static_cast<float>(m_mapRenderer.GetRoomTop()));
    int dx = static_cast<int>(std::round(current.x - start.x));
    int dy = static_cast<int>(std::round(current.y - start.y));

    Door& door = doors[static_cast<std::size_t>(m_drag_door_idx)];
    door.x = static_cast<uint8_t>(std::clamp(m_drag_door_start_x + dx, 0, 63));
    door.y = static_cast<uint8_t>(std::clamp(m_drag_door_start_y + dy, 0, 63));
    rd->SetDoors(m_current_room, doors);

    m_selected_door_idx = m_drag_door_idx;
    m_hovered_door_idx = m_drag_door_idx;
    SetCursor(wxCursor(wxCURSOR_HAND));
    Refresh();
}

void MyGLCanvas::EndDoorDrag() {
    if (!m_dragging_door) {
        return;
    }

    m_dragging_door = false;
    if (HasCapture()) {
        ReleaseMouse();
    }
    m_hovered_door_idx = m_selected_door_idx;
    SetCursor(wxCursor(m_hovered_door_idx >= 0 ? wxCURSOR_HAND : wxCURSOR_ARROW));
    NotifyRoomDataChanged(false, false, false, true);
    Refresh();
}

void MyGLCanvas::StartTileSwapRegionDrag(int region_idx, int resize_axis, const wxMouseEvent& evt) {
    auto regions = BuildTileSwapRegionGeometries(m_gd, m_current_room, m_mapRenderer, m_heightmapRenderer.GetZExtent());
    if (region_idx < 0 || region_idx >= static_cast<int>(regions.size())) {
        return;
    }

    const auto& region = regions[static_cast<std::size_t>(region_idx)];
    TileSwapRegionMetrics metrics = MetricsForTileSwapRegion(region.swap, region.part);
    m_dragging_tileswap_region = true;
    m_drag_tileswap_region_idx = region_idx;
    m_drag_tileswap_resize_axis = resize_axis;
    m_drag_start_mouse = evt.GetPosition();
    m_drag_tileswap_start_x = metrics.x;
    m_drag_tileswap_start_y = metrics.y;
    m_drag_tileswap_start_width = metrics.width;
    m_drag_tileswap_start_height = metrics.height;
    m_selected_tileswap_region_idx = region_idx;
    m_selected_entity_idx = -1;
    m_selected_warp_idx = -1;
    SetCursor(wxCursor(resize_axis != 0 ? wxCURSOR_SIZENWSE : wxCURSOR_HAND));
    if (!HasCapture()) {
        CaptureMouse();
    }
}

void MyGLCanvas::UpdateTileSwapRegionDrag(const wxMouseEvent& evt) {
    auto regions = BuildTileSwapRegionGeometries(m_gd, m_current_room, m_mapRenderer, m_heightmapRenderer.GetZExtent());
    if (m_drag_tileswap_region_idx < 0 || m_drag_tileswap_region_idx >= static_cast<int>(regions.size())) {
        EndTileSwapRegionDrag();
        return;
    }

    const auto& region = regions[static_cast<std::size_t>(m_drag_tileswap_region_idx)];
    int dx = 0;
    int dy = 0;
    if (region.part == TileSwapRegionPart::TilemapSource ||
        region.part == TileSwapRegionPart::TilemapDestination) {
        PickPoint start = ScreenToMapPoint(
            ScreenToWorldX(m_drag_start_mouse.x),
            ScreenToWorldY(m_drag_start_mouse.y),
            0.0f,
            static_cast<float>(m_mapRenderer.GetRoomLeft()),
            static_cast<float>(m_mapRenderer.GetRoomTop()));
        PickPoint current = ScreenToMapPoint(
            ScreenToWorldX(evt.GetPosition().x),
            ScreenToWorldY(evt.GetPosition().y),
            0.0f,
            static_cast<float>(m_mapRenderer.GetRoomLeft()),
            static_cast<float>(m_mapRenderer.GetRoomTop()));
        dx = static_cast<int>(std::trunc(current.x - start.x));
        dy = static_cast<int>(std::trunc(current.y - start.y));
    } else {
        PickPoint start = ScreenToHeightmapPoint(
            ScreenToWorldX(m_drag_start_mouse.x),
            ScreenToWorldY(m_drag_start_mouse.y),
            static_cast<float>(m_mapRenderer.GetRoomLeft()),
            static_cast<float>(m_mapRenderer.GetRoomTop()));
        PickPoint current = ScreenToHeightmapPoint(
            ScreenToWorldX(evt.GetPosition().x),
            ScreenToWorldY(evt.GetPosition().y),
            static_cast<float>(m_mapRenderer.GetRoomLeft()),
            static_cast<float>(m_mapRenderer.GetRoomTop()));
        dx = static_cast<int>(std::trunc(current.x - start.x));
        dy = static_cast<int>(std::trunc(current.y - start.y));
    }

    if (m_drag_tileswap_resize_axis != 0) {
        int horizontal_width_delta = static_cast<int>(std::trunc(
            (ScreenToWorldX(evt.GetPosition().x) - ScreenToWorldX(m_drag_start_mouse.x)) / 32.0f));
        int vertical_height_delta = static_cast<int>(std::trunc(
            (ScreenToWorldY(evt.GetPosition().y) - ScreenToWorldY(m_drag_start_mouse.y)) / 32.0f));

        ClearTileSwapPreview();
        auto rd = m_gd ? m_gd->GetRoomData() : nullptr;
        if (!rd) {
            return;
        }
        auto swaps = rd->GetTileSwaps(m_current_room);
        if (region.swap_index < 0 || region.swap_index >= static_cast<int>(swaps.size())) {
            return;
        }

        auto clamp_size = [&](int value, int origin) {
            return std::clamp(value, 1, std::max(1, 64 - origin));
        };

        auto apply_size = [&](std::vector<TileSwap>& target_swaps, int width, int height) {
            TileSwap& swap = target_swaps[static_cast<std::size_t>(region.swap_index)];
            TileSwapRegionMetrics metrics = MetricsForTileSwapRegion(swap, region.part);
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

        int base_w = std::max(1, m_drag_tileswap_start_width);
        int base_h = std::max(1, m_drag_tileswap_start_height);

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
                ScreenToWorldX(evt.GetPosition().x),
                ScreenToWorldY(evt.GetPosition().y)
            };

            float best_dist = std::numeric_limits<float>::max();
            std::vector<TileSwap> best_swaps = swaps;
            for (const auto& candidate : candidates) {
                auto test_swaps = swaps;
                apply_size(test_swaps, candidate.w, candidate.h);
                rd->SetTileSwaps(m_current_room, test_swaps);
                auto test_regions = BuildTileSwapRegionGeometries(m_gd, m_current_room, m_mapRenderer, m_heightmapRenderer.GetZExtent());
                if (m_drag_tileswap_region_idx < 0 ||
                    m_drag_tileswap_region_idx >= static_cast<int>(test_regions.size())) {
                    continue;
                }
                PickPoint handle = test_regions[static_cast<std::size_t>(m_drag_tileswap_region_idx)].resize_handle;
                float dxh = handle.x - cursor.x;
                float dyh = handle.y - cursor.y;
                float dist = dxh * dxh + dyh * dyh;
                if (dist < best_dist) {
                    best_dist = dist;
                    best_swaps = std::move(test_swaps);
                }
            }
            rd->SetTileSwaps(m_current_room, best_swaps);
        };

        if (region.swap.mode == TileSwap::Mode::FLOOR) {
            apply_size(swaps, base_w + dx, base_h + dy);
            rd->SetTileSwaps(m_current_room, swaps);
        } else if (region.swap.mode == TileSwap::Mode::WALL_NW) {
            choose_best_nw_mapping();
        } else {
            apply_size(swaps, base_w + horizontal_width_delta, base_h + vertical_height_delta);
            rd->SetTileSwaps(m_current_room, swaps);
        }
    } else {
        ClearTileSwapPreview();
        auto rd = m_gd ? m_gd->GetRoomData() : nullptr;
        if (!rd) {
            return;
        }
        auto swaps = rd->GetTileSwaps(m_current_room);
        if (region.swap_index < 0 || region.swap_index >= static_cast<int>(swaps.size())) {
            return;
        }
        TileSwap& swap = swaps[static_cast<std::size_t>(region.swap_index)];
        TileSwapRegionMetrics metrics = MetricsForTileSwapRegion(swap, region.part);
        int x = std::clamp(m_drag_tileswap_start_x + dx, 0, std::max(0, 64 - metrics.width));
        int y = std::clamp(m_drag_tileswap_start_y + dy, 0, std::max(0, 64 - metrics.height));
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
        rd->SetTileSwaps(m_current_room, swaps);
    }

    m_selected_tileswap_region_idx = m_drag_tileswap_region_idx;
    m_hovered_tileswap_region_idx = m_drag_tileswap_region_idx;
    SetCursor(wxCursor(m_drag_tileswap_resize_axis != 0 ? wxCURSOR_SIZENWSE : wxCURSOR_HAND));
    Refresh();
}

void MyGLCanvas::EndTileSwapRegionDrag() {
    if (!m_dragging_tileswap_region) {
        return;
    }
    m_dragging_tileswap_region = false;
    m_drag_tileswap_resize_axis = 0;
    if (HasCapture()) {
        ReleaseMouse();
    }
    SetCursor(wxCursor(m_hovered_tileswap_region_idx >= 0 ? wxCURSOR_HAND : wxCURSOR_ARROW));
    NotifyRoomDataChanged(false, false, true, false);
    Refresh();
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
    float ex_block = inst.map_x + inst.hitbox_offset - inst.room_left;
    float ey_block = inst.map_y + inst.hitbox_offset - inst.room_top;
    inst.x = 32.0f * ex_block - 32.0f * ey_block + 512.0f;
    inst.y = 16.0f * ex_block + 16.0f * ey_block + 100.0f - inst.map_z * inst.z_extent;
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
    auto sd = m_gd->GetSpriteData();
    if (sd->IsEntity(inst.entity_id)) {
        auto hitbox = sd->GetEntityHitbox(inst.entity_id);
        inst.hitbox_base = HitboxBaseToBlocks(hitbox.base);
        inst.hitbox_height = HitboxHeightToBlocks(hitbox.height);
    } else {
        inst.hitbox_base = 1.0f;
        inst.hitbox_height = 1.0f;
    }
    inst.hitbox_offset = HitboxDrawOffset(inst.hitbox_base);
    inst.floor_z = FloorUnderHitbox(
        inst.map_x + inst.hitbox_offset,
        inst.map_y + inst.hitbox_offset,
        inst.hitbox_base * 0.5f);
    UpdateEntityProjection(inst);
}

void MyGLCanvas::PersistCurrentRoomEdits() {
    if (m_room_entities.empty() && m_instances.empty() && m_warps.empty()) {
        return;
    }

    std::vector<Landstalker::Entity> entities(m_instances.size());
    for (const auto& inst : m_instances) {
        std::size_t idx = inst.instance_id > 0 ? std::size_t(inst.instance_id - 1) : entities.size();
        if (idx >= entities.size()) {
            continue;
        }
        Landstalker::Entity entity = idx < m_room_entities.size() ? m_room_entities[idx] : Landstalker::Entity{};
        entity.SetType(inst.entity_id);
        entity.SetPalette(std::min<uint8_t>(inst.palette, 3));
        entity.SetOrientation(inst.orientation);
        entity.SetXDbl(inst.map_x);
        entity.SetYDbl(inst.map_y);
        entity.SetZDbl(inst.map_z);
        entities[idx] = entity;
    }
    m_gd->GetSpriteData()->SetRoomEntities(m_current_room, entities);
    m_room_entities = entities;

    std::vector<Landstalker::WarpList::Warp> warps;
    std::map<uint32_t, std::size_t> warp_slots;
    for (const auto& inst : m_warps) {
        uint32_t key = inst.warp_key != 0 ? inst.warp_key : inst.instance_id;
        auto slot_it = warp_slots.find(key);
        if (slot_it == warp_slots.end()) {
            warp_slots[key] = warps.size();
            warps.push_back(inst.warp);
            slot_it = warp_slots.find(key);
        }
        Landstalker::WarpList::Warp& warp = warps[slot_it->second];
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
        if (m_pending_warp_half &&
            m_pending_warp_room == m_current_room &&
            inst.instance_id == m_pending_warp_instance_id &&
            inst.DestinationRoom() == 0xFFFF) {
            m_pending_warp = warp;
        }
    }
    warps.erase(
        std::remove_if(
            warps.begin(),
            warps.end(),
            [](const auto& warp) {
                return warp.room1 == 0xFFFF || warp.room2 == 0xFFFF || !warp.IsValid();
            }),
        warps.end());
    m_gd->GetRoomData()->SetWarpsForRoom(m_current_room, warps);
}


// Thin forwarding layer: input handlers stay on MyGLCanvas while edit rules
// live in dedicated editor/coordinator classes.
void MyGLCanvas::AddEntity() {
    GLCanvasEntityEditor(*this).AddEntity();
    NotifyRoomDataChanged(true, false, false, false);
    NotifySelectionChanged();
}

void MyGLCanvas::CopySelectedEntity() {
    GLCanvasEntityEditor(*this).CopySelectedEntity();
}

void MyGLCanvas::PasteEntity() {
    GLCanvasEntityEditor(*this).PasteEntity();
    NotifyRoomDataChanged(true, false, false, false);
    NotifySelectionChanged();
}

void MyGLCanvas::CutSelectedEntity() {
    if (m_selected_entity_idx < 0 || m_selected_entity_idx >= static_cast<int>(m_instances.size())) {
        return;
    }

    CopySelectedEntity();
    DeleteSelectedObject();
}

void MyGLCanvas::DeleteSelectedObject() {
    const bool had_entity = m_selected_entity_idx >= 0;
    const bool had_warp = m_selected_warp_idx >= 0;
    const bool had_swap = m_selected_tileswap_region_idx >= 0;
    const bool had_door = m_selected_door_idx >= 0;
    GLCanvasObjectCoordinator(*this).DeleteSelectedObject();
    NotifyRoomDataChanged(had_entity, had_warp, had_swap, had_door);
    NotifySelectionChanged();
}

void MyGLCanvas::ReorderSelectedObject(int delta) {
    const bool had_entity = m_selected_entity_idx >= 0;
    const bool had_warp = m_selected_warp_idx >= 0;
    const bool had_door = m_selected_door_idx >= 0;
    GLCanvasObjectCoordinator(*this).ReorderSelectedObject(delta);
    NotifyRoomDataChanged(had_entity, had_warp, false, had_door);
    NotifySelectionChanged();
}

void MyGLCanvas::SelectNextObject(int direction) {
    GLCanvasObjectCoordinator(*this).SelectNextObject(direction);
    NotifySelectionChanged();
    UpdateStatusBar();
}

void MyGLCanvas::SelectNextTileSwapRegion(int direction) {
    GLCanvasObjectCoordinator(*this).SelectNextTileSwapRegion(direction);
    NotifySelectionChanged();
    UpdateStatusBar();
}

void MyGLCanvas::CycleSelectedEntityId(int delta) {
    GLCanvasEntityEditor(*this).CycleSelectedEntityId(delta);
    NotifyRoomDataChanged(true, false, false, false);
}

void MyGLCanvas::CycleSelectedEntityPalette() {
    GLCanvasEntityEditor(*this).CycleSelectedEntityPalette();
    NotifyRoomDataChanged(true, false, false, false);
}

void MyGLCanvas::SetSelectedEntityOrientation(Landstalker::Orientation orientation) {
    GLCanvasEntityEditor(*this).SetSelectedEntityOrientation(orientation);
    NotifyRoomDataChanged(true, false, false, false);
}

void MyGLCanvas::SetSelectedEntityToFloor() {
    GLCanvasEntityEditor(*this).SetSelectedEntityToFloor();
    NotifyRoomDataChanged(true, false, false, false);
}

void MyGLCanvas::AddWarpHalf() {
    GLCanvasWarpEditor(*this).AddWarpHalf();
    NotifyRoomDataChanged(false, true, false, false);
    NotifySelectionChanged();
}

std::pair<float, float> MyGLCanvas::FindNearestFreeWarpCell(float preferred_x, float preferred_y) const {
    return GLCanvasWarpEditor(const_cast<MyGLCanvas&>(*this)).FindNearestFreeWarpCell(preferred_x, preferred_y);
}

std::pair<int, int> MyGLCanvas::MouseHeightmapCell() const {
    if (m_last_mouse_pos.x < 0 || m_last_mouse_pos.y < 0) {
        return {
            std::clamp(m_mapRenderer.GetRoomLeft() + m_mapRenderer.GetRoomWidth() / 2, 0, 63),
            std::clamp(m_mapRenderer.GetRoomTop() + m_mapRenderer.GetRoomHeight() / 2, 0, 63)
        };
    }
    PickPoint point = ScreenToHeightmapPoint(
        ScreenToWorldX(m_last_mouse_pos.x),
        ScreenToWorldY(m_last_mouse_pos.y),
        static_cast<float>(m_mapRenderer.GetRoomLeft()),
        static_cast<float>(m_mapRenderer.GetRoomTop()));
    return {
        std::clamp(static_cast<int>(std::round(point.x)), 0, 63),
        std::clamp(static_cast<int>(std::round(point.y)), 0, 63)
    };
}

void MyGLCanvas::ResizeSelectedWarp(float dx, float dy) {
    GLCanvasWarpEditor(*this).ResizeSelectedWarp(dx, dy);
    NotifyRoomDataChanged(false, true, false, false);
}

void MyGLCanvas::RotateSelectedWarp(float dx, float dy) {
    GLCanvasWarpEditor(*this).RotateSelectedWarp(dx, dy);
    NotifyRoomDataChanged(false, true, false, false);
}

void MyGLCanvas::CycleSelectedWarpType(int delta) {
    GLCanvasWarpEditor(*this).CycleSelectedWarpType(delta);
    NotifyRoomDataChanged(false, true, false, false);
}

void MyGLCanvas::CycleSelectedDoorSize(int delta) {
    GLCanvasTileDoorEditor(*this).CycleSelectedDoorSize(delta);
    NotifyRoomDataChanged(false, false, false, true);
}

void MyGLCanvas::AddDoor() {
    GLCanvasTileDoorEditor(*this).AddDoor();
    NotifyRoomDataChanged(false, false, false, true);
    NotifySelectionChanged();
}

void MyGLCanvas::AddTileSwap() {
    GLCanvasTileDoorEditor(*this).AddTileSwap();
    NotifyRoomDataChanged(false, false, true, false);
    NotifySelectionChanged();
}

void MyGLCanvas::CycleSelectedTileSwapShape(int delta) {
    GLCanvasTileDoorEditor(*this).CycleSelectedTileSwapShape(delta);
    NotifyRoomDataChanged(false, false, true, false);
}

void MyGLCanvas::CycleSelectedTileSwapId(int delta) {
    GLCanvasTileDoorEditor(*this).CycleSelectedTileSwapId(delta);
    NotifyRoomDataChanged(false, false, true, false);
}

void MyGLCanvas::ResizeSelectedTileSwapRegion(float requested_width, float requested_height) {
    GLCanvasTileDoorEditor(*this).ResizeSelectedTileSwapRegion(requested_width, requested_height);
    NotifyRoomDataChanged(false, false, true, false);
}

void MyGLCanvas::ToggleSelectedTileSwapPreview() {
    GLCanvasTileDoorEditor(*this).ToggleSelectedTileSwapPreview();
}

void MyGLCanvas::ToggleSelectedDoorPreview() {
    GLCanvasTileDoorEditor(*this).ToggleSelectedDoorPreview();
}

void MyGLCanvas::ClearTileSwapPreview() {
    GLCanvasTileDoorEditor(*this).ClearTileSwapPreview();
}

void MyGLCanvas::NudgeSelectedObject(float dx, float dy, float dz) {
    const bool had_entity = m_selected_entity_idx >= 0;
    const bool had_warp = m_selected_warp_idx >= 0;
    const bool had_swap = m_selected_tileswap_region_idx >= 0;
    const bool had_door = m_selected_door_idx >= 0;
    GLCanvasObjectCoordinator(*this).NudgeSelectedObject(dx, dy, dz);
    NotifyRoomDataChanged(had_entity, had_warp, had_swap, had_door);
}

void MyGLCanvas::RenderWarps() {
    GLCanvasWarpEditor(*this).RenderWarps();
}

void MyGLCanvas::RenderEntityControls() {
    auto draw_control = [this](int entity_idx, bool selected) {
        if (entity_idx < 0 || entity_idx >= static_cast<int>(m_instances.size())) {
            return;
        }

        const SpriteInstance& inst = m_instances[static_cast<std::size_t>(entity_idx)];
        PickRect rect = EntityZControlRect(inst);
        float center_x = inst.map_x + inst.hitbox_offset;
        float center_y = inst.map_y + inst.hitbox_offset;
        float top_z = inst.map_z + std::max(inst.hitbox_height, 0.125f);
        PickPoint top_center = ProjectEntityGridPoint(inst, center_x, center_y, top_z);
        PickPoint handle_center{
            (rect.min_x + rect.max_x) * 0.5f,
            (rect.min_y + rect.max_y) * 0.5f
        };

        glColor4f(selected ? 1.0f : 0.85f, selected ? 0.2f : 0.9f, selected ? 0.2f : 1.0f, 0.95f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glVertex2f(top_center.x, top_center.y);
        glVertex2f(handle_center.x, handle_center.y);
        glEnd();

        glColor4f(0.02f, 0.02f, 0.02f, 0.55f);
        glBegin(GL_QUADS);
        glVertex2f(rect.min_x, rect.min_y);
        glVertex2f(rect.max_x, rect.min_y);
        glVertex2f(rect.max_x, rect.max_y);
        glVertex2f(rect.min_x, rect.max_y);
        glEnd();

        glColor4f(selected ? 1.0f : 0.85f, selected ? 0.2f : 0.9f, selected ? 0.2f : 1.0f, 0.95f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(rect.min_x, rect.min_y);
        glVertex2f(rect.max_x, rect.min_y);
        glVertex2f(rect.max_x, rect.max_y);
        glVertex2f(rect.min_x, rect.max_y);
        glEnd();
        glLineWidth(1.0f);
    };

    glUseProgram(0);
    for (int i = 0; i <= 5; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    draw_control(m_selected_entity_idx, true);
}

void MyGLCanvas::RenderSelectedEntityTooltip() {
    GLCanvasEntityEditor(*this).RenderSelectedEntityTooltip();
}

void MyGLCanvas::RenderSelectedWarpTooltip() {
    GLCanvasWarpEditor(*this).RenderSelectedWarpTooltip();
}

void MyGLCanvas::RenderSelectedDoorTooltip() {
    GLCanvasTileDoorEditor(*this).RenderSelectedDoorTooltip();
}

void MyGLCanvas::RenderSelectedTileSwapRegionTooltip() {
    GLCanvasTileDoorEditor(*this).RenderSelectedTileSwapRegionTooltip();
}

void MyGLCanvas::RenderRoomInfoTable(int width, int height) {
    auto rows = BuildRoomInfoRows(m_gd, m_current_room);
    m_room_info_links.clear();
    if (rows.empty()) {
        return;
    }

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
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    constexpr float scale = 1.0f;
    constexpr float glyph_advance = 6.0f;
    constexpr float row_height = 10.0f;
    constexpr float padding = 6.0f;
    constexpr float column_gap = 12.0f;
    constexpr float start_x = 8.0f;
    constexpr float start_y = 8.0f;

    float label_width = 0.0f;
    float value_width = 0.0f;
    for (const auto& row : rows) {
        label_width = std::max(label_width, float(row.label.size()) * glyph_advance * scale);
        value_width = std::max(value_width, float(row.value.size()) * glyph_advance * scale);
    }

    float table_width = padding * 2.0f + label_width + column_gap + value_width;
    float table_height = padding * 2.0f + float(rows.size()) * row_height;
    float label_x = start_x + padding;
    float value_x = label_x + label_width + column_gap;
    int hovered_room = HitTestRoomInfoLink(m_last_mouse_pos);

    glColor4f(0.02f, 0.02f, 0.03f, 0.78f);
    glBegin(GL_QUADS);
    glVertex2f(start_x, start_y);
    glVertex2f(start_x + table_width, start_y);
    glVertex2f(start_x + table_width, start_y + table_height);
    glVertex2f(start_x, start_y + table_height);
    glEnd();

    glColor4f(0.45f, 0.45f, 0.5f, 0.85f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(start_x, start_y);
    glVertex2f(start_x + table_width, start_y);
    glVertex2f(start_x + table_width, start_y + table_height);
    glVertex2f(start_x, start_y + table_height);
    glEnd();

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        float row_y = start_y + padding + float(i) * row_height;
        float value_text_width = float(row.value.size()) * glyph_advance * scale;
        wxRect value_rect(
            static_cast<int>(std::floor(value_x)),
            static_cast<int>(std::floor(row_y)),
            static_cast<int>(std::ceil(value_text_width)),
            static_cast<int>(std::ceil(row_height)));

        if (row.room == hovered_room) {
            glColor4f(1.0f, 1.0f, 1.0f, 0.09f);
            glBegin(GL_QUADS);
            glVertex2f(float(value_rect.GetLeft()), float(value_rect.GetTop()));
            glVertex2f(float(value_rect.GetRight()), float(value_rect.GetTop()));
            glVertex2f(float(value_rect.GetRight()), float(value_rect.GetBottom()));
            glVertex2f(float(value_rect.GetLeft()), float(value_rect.GetBottom()));
            glEnd();
        }

        glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
        DrawOverlayText(row.label, label_x, row_y, scale);
        glColor4f(1.0f, 0.95f, 0.18f, 0.98f);
        DrawOverlayText(row.value, value_x, row_y, scale);

        m_room_info_links.push_back({value_rect, row.room});
    }
}

void MyGLCanvas::WriteSelectedEntityOcclusionDebugLog() {
    if (m_selected_entity_idx < 0 || m_selected_entity_idx >= static_cast<int>(m_instances.size())) {
        wxLogMessage("Select an entity before writing occlusion debug logs.");
        return;
    }

    const SpriteInstance& inst = m_instances[static_cast<std::size_t>(m_selected_entity_idx)];
    auto depth_range = EntityOcclusionDebugDepthRange(inst);
    float center_x = inst.map_x + inst.hitbox_offset;
    float center_y = inst.map_y + inst.hitbox_offset;
    float half_base = inst.hitbox_base * 0.5f;
    constexpr const char* path = "/tmp/landstalker_occlusion_debug.log";
    m_heightmapRenderer.WriteEntityOcclusionDebugLog(
        path,
        depth_range.first,
        depth_range.second,
        inst.map_z,
        center_x - half_base,
        center_y - half_base,
        center_x + half_base,
        center_y + half_base,
        inst.map_z + std::max(inst.hitbox_height, 0.125f));

    std::array<PickPoint, 8> projected_bounds = {
        ProjectEntityGridPoint(inst, center_x - half_base, center_y - half_base, inst.map_z),
        ProjectEntityGridPoint(inst, center_x + half_base, center_y - half_base, inst.map_z),
        ProjectEntityGridPoint(inst, center_x + half_base, center_y + half_base, inst.map_z),
        ProjectEntityGridPoint(inst, center_x - half_base, center_y + half_base, inst.map_z),
        ProjectEntityGridPoint(inst, center_x - half_base, center_y - half_base, inst.map_z + std::max(inst.hitbox_height, 0.125f)),
        ProjectEntityGridPoint(inst, center_x + half_base, center_y - half_base, inst.map_z + std::max(inst.hitbox_height, 0.125f)),
        ProjectEntityGridPoint(inst, center_x + half_base, center_y + half_base, inst.map_z + std::max(inst.hitbox_height, 0.125f)),
        ProjectEntityGridPoint(inst, center_x - half_base, center_y + half_base, inst.map_z + std::max(inst.hitbox_height, 0.125f))
    };
    float min_x = projected_bounds.front().x;
    float min_y = projected_bounds.front().y;
    float max_x = projected_bounds.front().x;
    float max_y = projected_bounds.front().y;
    for (const auto& point : projected_bounds) {
        min_x = std::min(min_x, point.x);
        min_y = std::min(min_y, point.y);
        max_x = std::max(max_x, point.x);
        max_y = std::max(max_y, point.y);
    }

    constexpr const char* fg_path = "/tmp/landstalker_foreground_priority_debug.log";
    m_mapRenderer.WriteForegroundPriorityDebugLog(fg_path, min_x, min_y, max_x, max_y);
    wxLogMessage("Wrote occlusion debug logs to %s and %s", path, fg_path);
}

void MyGLCanvas::WriteEntityDrawOrderDebugLog() {
    constexpr const char* path = "/tmp/landstalker_entity_draw_order.log";
    std::ofstream log(path, std::ios::out | std::ios::trunc);
    if (!log.is_open()) {
        wxLogMessage("Failed to write entity draw-order log to %s", path);
        return;
    }

    log << "instance_id,map_x,map_y,map_z,floor_z,hitbox_base,hitbox_height,depth_key,min_x,min_y,max_x,max_y,min_z,max_z\n";
    for (const auto& inst : m_instances) {
        EntityBounds bounds = GetEntityBounds(inst);
        float depth_key = EntityFrontDepthKey(inst);
        log << inst.instance_id << ","
            << inst.map_x << ","
            << inst.map_y << ","
            << inst.map_z << ","
            << inst.floor_z << ","
            << inst.hitbox_base << ","
            << inst.hitbox_height << ","
            << depth_key << ","
            << bounds.min_x << ","
            << bounds.min_y << ","
            << bounds.max_x << ","
            << bounds.max_y << ","
            << bounds.min_z << ","
            << bounds.max_z << "\n";
    }

    log << "\nrelations,lhs_id,rhs_id,lhs_before_rhs,rhs_before_lhs\n";
    for (std::size_t i = 0; i < m_instances.size(); ++i) {
        for (std::size_t j = i + 1; j < m_instances.size(); ++j) {
            const auto& lhs = m_instances[i];
            const auto& rhs = m_instances[j];
            log << lhs.instance_id << ","
                << rhs.instance_id << ","
                << (EntityMustDrawBefore(lhs, rhs) ? 1 : 0) << ","
                << (EntityMustDrawBefore(rhs, lhs) ? 1 : 0) << "\n";
        }
    }

    wxLogMessage("Wrote entity draw-order log to %s", path);
}

float MyGLCanvas::FloorUnderRect(float min_x, float min_y, float max_x, float max_y) const {
    auto map_entry = m_gd->GetRoomData()->GetMapForRoom(m_current_room);
    if (!map_entry) {
        return 0.0f;
    }

    auto map = m_tileswap_preview_map ? m_tileswap_preview_map : map_entry->GetData();
    constexpr float heightmap_entity_offset = 12.0f;
    float hm_min_x = min_x - heightmap_entity_offset;
    float hm_min_y = min_y - heightmap_entity_offset;
    float hm_max_x = max_x - heightmap_entity_offset;
    float hm_max_y = max_y - heightmap_entity_offset;
    int min_cell_x = static_cast<int>(std::floor(hm_min_x));
    int min_cell_y = static_cast<int>(std::floor(hm_min_y));
    int max_cell_x = static_cast<int>(std::floor(std::nextafter(hm_max_x, hm_min_x)));
    int max_cell_y = static_cast<int>(std::floor(std::nextafter(hm_max_y, hm_min_y)));
    uint8_t floor_z = 0;

    for (int y = min_cell_y; y <= max_cell_y; ++y) {
        for (int x = min_cell_x; x <= max_cell_x; ++x) {
            if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
                continue;
            }
            uint8_t height = map->GetHeight({x, y});
            if (height != 0xFF) {
                floor_z = std::max(floor_z, height);
            }
        }
    }

    return float(floor_z);
}

float MyGLCanvas::FloorUnderPoint(float x, float y) const {
    auto map_entry = m_gd->GetRoomData()->GetMapForRoom(m_current_room);
    if (!map_entry) {
        return 0.0f;
    }

    auto map = m_tileswap_preview_map ? m_tileswap_preview_map : map_entry->GetData();
    constexpr float heightmap_entity_offset = 12.0f;
    int cell_x = static_cast<int>(std::floor(x - heightmap_entity_offset));
    int cell_y = static_cast<int>(std::floor(y - heightmap_entity_offset));
    if (cell_x < 0 || cell_y < 0 || cell_x >= map->GetHeightmapWidth() || cell_y >= map->GetHeightmapHeight()) {
        return 0.0f;
    }

    uint8_t height = map->GetHeight({cell_x, cell_y});
    return height == 0xFF ? 0.0f : float(height);
}

bool MyGLCanvas::ShadowOccludedByHeightmap(float min_x, float min_y, float max_x, float max_y, float z) const {
    auto map_entry = m_gd->GetRoomData()->GetMapForRoom(m_current_room);
    if (!map_entry) {
        return false;
    }

    auto map = map_entry->GetData();
    constexpr float heightmap_entity_offset = 12.0f;
    float hm_min_x = min_x - heightmap_entity_offset;
    float hm_min_y = min_y - heightmap_entity_offset;
    float hm_max_x = max_x - heightmap_entity_offset;
    float hm_max_y = max_y - heightmap_entity_offset;
    float shadow_back_depth = hm_min_x + hm_min_y;
    float shadow_front_depth = hm_max_x + hm_max_y;

    auto visible_cell = [&](int x, int y, uint8_t height, uint8_t restriction) {
        return height != 0xFF && !(restriction == 4 && height == 0) && height > z;
    };
    auto overlaps_shadow = [&](int x, int y) {
        return float(x) < hm_max_x &&
               float(x + 1) > hm_min_x &&
               float(y) < hm_max_y &&
               float(y + 1) > hm_min_y;
    };
    auto cell_is_in_front = [&](int x, int y) {
        return !overlaps_shadow(x, y) &&
               float(x + 1) > hm_min_x &&
               float(y + 1) > hm_min_y;
    };

    for (int y = 0; y < map->GetHeightmapHeight(); ++y) {
        for (int x = 0; x < map->GetHeightmapWidth(); ++x) {
            uint8_t height = map->GetHeight({x, y});
            uint16_t value = map->GetHeightmapCell({x, y});
            uint8_t restriction = static_cast<uint8_t>(value >> 12);
            float cell_depth = float(x + y);
            if (cell_depth < shadow_back_depth || cell_depth > shadow_front_depth + 1.0f) {
                continue;
            }
            if (visible_cell(x, y, height, restriction) && cell_is_in_front(x, y)) {
                return true;
            }
        }
    }

    return false;
}

bool MyGLCanvas::EntityCollidesWithHeightmap(const SpriteInstance& inst) const {
    auto map_entry = m_gd->GetRoomData()->GetMapForRoom(m_current_room);
    if (!map_entry || inst.hitbox_base <= 0.0f) {
        return false;
    }

    auto map = map_entry->GetData();
    constexpr float heightmap_entity_offset = 12.0f;
    constexpr float epsilon = 0.001f;
    float center_x = inst.map_x + inst.hitbox_offset;
    float center_y = inst.map_y + inst.hitbox_offset;
    float half_base = inst.hitbox_base * 0.5f;
    float hm_min_x = center_x - half_base - heightmap_entity_offset;
    float hm_min_y = center_y - half_base - heightmap_entity_offset;
    float hm_max_x = center_x + half_base - heightmap_entity_offset;
    float hm_max_y = center_y + half_base - heightmap_entity_offset;
    int min_cell_x = static_cast<int>(std::floor(hm_min_x));
    int min_cell_y = static_cast<int>(std::floor(hm_min_y));
    int max_cell_x = static_cast<int>(std::floor(std::nextafter(hm_max_x, hm_min_x)));
    int max_cell_y = static_cast<int>(std::floor(std::nextafter(hm_max_y, hm_min_y)));

    for (int y = min_cell_y; y <= max_cell_y; ++y) {
        for (int x = min_cell_x; x <= max_cell_x; ++x) {
            if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
                continue;
            }
            uint8_t height = map->GetHeight({x, y});
            uint16_t value = map->GetHeightmapCell({x, y});
            uint8_t restriction = static_cast<uint8_t>(value >> 12);
            if (height == 0xFF || (restriction == 4 && height == 0)) {
                continue;
            }
            if (float(height) > inst.map_z + epsilon) {
                return true;
            }
        }
    }

    return false;
}

float MyGLCanvas::FloorUnderHitbox(float center_x, float center_y, float half_base) const {
    return FloorUnderRect(center_x - half_base, center_y - half_base, center_x + half_base, center_y + half_base);
}

int MyGLCanvas::FindInstanceIndex(uint32_t instance_id) const {
    for (std::size_t i = 0; i < m_instances.size(); ++i) {
        if (m_instances[i].instance_id == instance_id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int MyGLCanvas::FindWarpIndex(uint32_t instance_id) const {
    for (std::size_t i = 0; i < m_warps.size(); ++i) {
        if (m_warps[i].instance_id == instance_id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int MyGLCanvas::HitTestRoomInfoLink(const wxPoint& point) const {
    for (const auto& link : m_room_info_links) {
        if (link.rect.Contains(point)) {
            return static_cast<int>(link.room);
        }
    }
    return -1;
}

int MyGLCanvas::HitTestEntity(const wxPoint& point) const {
    PickPoint world_point{
        ScreenToWorldX(point.x),
        ScreenToWorldY(point.y)
    };

    for (int i = static_cast<int>(m_instances.size()) - 1; i >= 0; --i) {
        if (PointInEntityHitbox(m_instances[static_cast<std::size_t>(i)], world_point)) {
            return i;
        }
    }
    return -1;
}

int MyGLCanvas::HitTestEntityBody(const wxPoint& point) const {
    PickPoint world_point{
        ScreenToWorldX(point.x),
        ScreenToWorldY(point.y)
    };

    for (int i = static_cast<int>(m_instances.size()) - 1; i >= 0; --i) {
        if (PointInEntityHitbox(m_instances[static_cast<std::size_t>(i)], world_point, false)) {
            return i;
        }
    }
    return -1;
}

int MyGLCanvas::HitTestEntityZControl(const wxPoint& point) const {
    PickPoint world_point{
        ScreenToWorldX(point.x),
        ScreenToWorldY(point.y)
    };

    if (m_selected_entity_idx >= 0 && m_selected_entity_idx < static_cast<int>(m_instances.size()) &&
        PointInRect(world_point, EntityZControlRect(m_instances[static_cast<std::size_t>(m_selected_entity_idx)]))) {
        return m_selected_entity_idx;
    }

    return -1;
}

int MyGLCanvas::HitTestWarpResizeControl(const wxPoint& point) const {
    if (m_selected_warp_idx < 0 || m_selected_warp_idx >= static_cast<int>(m_warps.size())) {
        return 0;
    }

    PickPoint world_point{
        ScreenToWorldX(point.x),
        ScreenToWorldY(point.y)
    };
    const WarpInstance& warp = m_warps[static_cast<std::size_t>(m_selected_warp_idx)];
    for (int axis = 1; axis <= 2; ++axis) {
        if (WarpResizeAxisUsable(warp, axis) && PointInRect(world_point, WarpResizeControlRect(warp, axis))) {
            return axis;
        }
    }
    return 0;
}

int MyGLCanvas::HitTestWarp(const wxPoint& point) const {
    PickPoint world_point{
        ScreenToWorldX(point.x),
        ScreenToWorldY(point.y)
    };

    for (int i = static_cast<int>(m_warps.size()) - 1; i >= 0; --i) {
        if (PointInWarp(m_warps[static_cast<std::size_t>(i)], world_point)) {
            return i;
        }
    }
    return -1;
}

int MyGLCanvas::HitTestTileSwapRegion(const wxPoint& point) const {
    PickPoint world_point{
        ScreenToWorldX(point.x),
        ScreenToWorldY(point.y)
    };
    auto regions = BuildTileSwapRegionGeometries(m_gd, m_current_room, m_mapRenderer, m_heightmapRenderer.GetZExtent());
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

int MyGLCanvas::HitTestTileSwapRegionResizeControl(const wxPoint& point) const {
    auto regions = BuildTileSwapRegionGeometries(m_gd, m_current_room, m_mapRenderer, m_heightmapRenderer.GetZExtent());
    if (m_selected_tileswap_region_idx < 0 ||
        m_selected_tileswap_region_idx >= static_cast<int>(regions.size())) {
        return 0;
    }
    PickPoint world_point{
        ScreenToWorldX(point.x),
        ScreenToWorldY(point.y)
    };
    const auto& region = regions[static_cast<std::size_t>(m_selected_tileswap_region_idx)];
    PickRect rect = RectAroundPoint(region.resize_handle, 6.0f);
    if (PointInRect(world_point, rect)) {
        return 1;
    }
    return 0;
}

int MyGLCanvas::HitTestDoor(const wxPoint& point) const {
    PickPoint world_point{
        ScreenToWorldX(point.x),
        ScreenToWorldY(point.y)
    };
    auto doors = BuildDoorGeometries(
        m_gd,
        m_current_room,
        m_mapRenderer,
        m_heightmapRenderer.GetZExtent(),
        m_tileswap_preview_map);
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

void MyGLCanvas::RenderTileSwapOutlines() {
    GLCanvasTileDoorEditor(*this).RenderTileSwapOutlines();
}

void MyGLCanvas::RenderDoors() {
    auto doors = BuildDoorGeometries(
        m_gd,
        m_current_room,
        m_mapRenderer,
        m_heightmapRenderer.GetZExtent(),
        m_tileswap_preview_map);
    if (doors.empty()) {
        return;
    }

    glUseProgram(0);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& door : doors) {
        bool selected = door.index == m_selected_door_idx;
        bool hovered = door.index == m_hovered_door_idx;

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
