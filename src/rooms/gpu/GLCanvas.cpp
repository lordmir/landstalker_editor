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
#include <cstdio>
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

constexpr bool kLogPendingEntityPlacement = true;

void LogPendingEntityPlacementSnapshot(
    const char* stage,
    const wxPoint& mouse,
    int hover_x,
    int hover_y,
    float plane_z,
    float offset_x,
    float offset_y,
    const SpriteInstance* ghost)
{
    if (!kLogPendingEntityPlacement) {
        return;
    }
    if (!ghost) {
        std::fprintf(
            stderr,
            "[PendingEntity] %s mouse=(%d,%d) hover=(%d,%d) plane_z=%.3f offset=(%.3f,%.3f) ghost=<none>\n",
            stage,
            mouse.x,
            mouse.y,
            hover_x,
            hover_y,
            plane_z,
            offset_x,
            offset_y);
        std::fflush(stderr);
        return;
    }

    const float center_x = ghost->map_x + ghost->hitbox_offset;
    const float center_y = ghost->map_y + ghost->hitbox_offset;
    std::fprintf(
        stderr,
        "[PendingEntity] %s mouse=(%d,%d) hover=(%d,%d) plane_z=%.3f offset=(%.3f,%.3f) "
        "ghost={map=(%.3f,%.3f,%.3f) center=(%.3f,%.3f) floor=%.3f hitbox_offset=%.3f}\n",
        stage,
        mouse.x,
        mouse.y,
        hover_x,
        hover_y,
        plane_z,
        offset_x,
        offset_y,
        ghost->map_x,
        ghost->map_y,
        ghost->map_z,
        center_x,
        center_y,
        ghost->floor_z,
        ghost->hitbox_offset);
    std::fflush(stderr);
}

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
constexpr float kHeightmapEditorMaxZExtent = 32.0f;
constexpr float kHeightmapEditorZScaleStep = 0.25f;
constexpr long kTargetFrameMs = 1000 / 60;
constexpr std::size_t kMaxUndoStates = 100;

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
        m_debug_occlusion(false), m_show_hitboxes(true), m_editor_mode(EditorMode::Room), m_drawing_tool(DrawingTool::Select), m_heightmap_view_mode(HeightmapViewMode::Flat), m_non_heightmap_z_extent(32.0f), m_heightmap_z_scale(0.0f), m_heightmap_tilemap_underlay(false), m_layer_heightmap_overlay(false), m_foreground_show_background_underlay(true), m_background_show_block_ids(false), m_layer_priority_highlight(true),
            m_background_has_selection(false), m_background_selected_x(0), m_background_selected_y(0),
            m_background_has_hover(false), m_background_hover_x(0), m_background_hover_y(0),
            m_background_clipboard_valid(false), m_background_clipboard_block_id(0),
            m_layer_dragging_select(false), m_layer_selection_add(false), m_layer_selection_subtract(false), m_layer_selection_parallelogram(false),
            m_layer_selection_anchor_x(0), m_layer_selection_anchor_y(0),
            m_layer_selection_drag_anchor_x(0), m_layer_selection_drag_anchor_y(0),
            m_layer_dragging_selection_move(false), m_layer_selection_move_anchor_x(-1), m_layer_selection_move_anchor_y(-1),
            m_layer_selection_move_delta_x(0), m_layer_selection_move_delta_y(0),
            m_layer_dragging_draw(false), m_layer_dragging_line(false), m_layer_draw_dirty(false), m_layer_last_draw_x(-1), m_layer_last_draw_y(-1),
            m_layer_line_start_x(-1), m_layer_line_start_y(-1), m_layer_line_end_x(-1), m_layer_line_end_y(-1),
            m_heightmap_clipboard_valid(false), m_heightmap_clipboard_cell(0),
            m_heightmap_dragging_select(false), m_heightmap_dragging_draw(false), m_heightmap_dragging_line(false), m_heightmap_dragging_selection_move(false), m_heightmap_draw_dirty(false),
            m_heightmap_selection_add(false), m_heightmap_selection_subtract(false),
            m_heightmap_selection_anchor_x(0), m_heightmap_selection_anchor_y(0),
            m_heightmap_selection_drag_anchor_x(0), m_heightmap_selection_drag_anchor_y(0),
            m_heightmap_last_draw_x(-1), m_heightmap_last_draw_y(-1),
            m_heightmap_line_start_x(-1), m_heightmap_line_start_y(-1),
            m_heightmap_line_end_x(-1), m_heightmap_line_end_y(-1),
            m_heightmap_selection_move_anchor_x(-1), m_heightmap_selection_move_anchor_y(-1),
            m_heightmap_selection_move_delta_x(0), m_heightmap_selection_move_delta_y(0),
            m_tileswap_preview_active(false), m_tileswap_preview_swap_index(-1),
    m_door_preview_active(false), m_door_preview_idx(-1),
      m_pending_warp_half(false), m_pending_warp_room(0xFFFF), m_pending_warp_instance_id(0),
    m_entity_clipboard_valid(false), m_zoom_step_idx(1), m_gl_init_failed(false), m_initialized(false), m_last_mouse_pos(wxDefaultPosition),
    m_last_anim_ms(0), m_last_frame_ms(0), m_animation_update_count(0), m_render_deferred(false),
    m_restoring_history(false), m_pending_add_type(PendingObjectAddType::None),
    m_pending_tileswap_part(PendingTileSwapPart::MapSource),
    m_pending_add_entity_id(Landstalker::Entity{}.GetType()),
    m_pending_add_entity_palette(Landstalker::Entity{}.GetPalette()),
    m_pending_add_entity_orientation(Landstalker::Entity{}.GetOrientation()),
    m_pending_add_entity_cursor_offset_x(0.0f),
    m_pending_add_entity_cursor_offset_y(0.0f),
    m_pending_add_plane_z(0.0f),
    m_pending_add_floor_snap(true),
    m_pending_add_hover_x(-1), m_pending_add_hover_y(-1),
    m_pending_add_swap_index(-1),
    m_pending_add_warp_width(1.0f),
    m_pending_add_warp_height(1.0f),
    m_pending_add_warp_type(Landstalker::WarpList::Warp::Type::NORMAL),
    m_pending_add_door_size(Door::Size::DOOR_1X4)
{
    m_current_room = 0;
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
        ApplyHeightmapViewMode();
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

void MyGLCanvas::SetDrawingTool(DrawingTool tool) {
    if (m_drawing_tool == tool) {
        return;
    }
    if (m_layer_dragging_draw) {
        CommitLayerDrawStroke();
    }
    m_drawing_tool = tool;
    m_heightmap_dragging_select = false;
    m_heightmap_dragging_draw = false;
    m_heightmap_dragging_line = false;
    m_heightmap_dragging_selection_move = false;
    m_heightmap_draw_dirty = false;
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
    m_heightmap_dragging_line = false;
    m_heightmap_dragging_selection_move = false;
    m_heightmap_line_preview_cells.clear();
    m_heightmap_selection_move_values.clear();
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

static void PostLayerBlockSelection(wxWindow* target, MyGLCanvas* canvas, int block_id) {
    if (!target) {
        return;
    }
    wxCommandEvent evt(EVT_GPU_LAYER_BLOCK_SELECT);
    evt.SetInt(block_id);
    evt.SetClientData(canvas);
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

void MyGLCanvas::OnIdle(wxIdleEvent& evt)
{
    if (!m_initialized || !m_gd || !IsShownOnScreen()) {
        return;
    }

    long now_ms = m_anim_stopwatch.Time();
    const bool frame_due = now_ms - m_last_frame_ms >= kTargetFrameMs;

    if (frame_due || m_render_deferred) {
        if (!frame_due) {
            evt.RequestMore();
            return;
        }

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

bool MyGLCanvas::CanUndo() const {
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

bool MyGLCanvas::CanRedo() const {
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

void MyGLCanvas::CaptureUndoState() {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    if (IsBackgroundLayerHistoryMode()) {
        m_bg_layer_undo_stack.push_back(BuildLayerUndoState(Tilemap3D::Layer::BG));
        if (m_bg_layer_undo_stack.size() > kMaxUndoStates) {
            m_bg_layer_undo_stack.erase(m_bg_layer_undo_stack.begin());
        }
        m_bg_layer_redo_stack.clear();
        NotifyHeightmapTargetChanged();
        return;
    }
    if (IsForegroundLayerHistoryMode()) {
        m_fg_layer_undo_stack.push_back(BuildLayerUndoState(Tilemap3D::Layer::FG));
        if (m_fg_layer_undo_stack.size() > kMaxUndoStates) {
            m_fg_layer_undo_stack.erase(m_fg_layer_undo_stack.begin());
        }
        m_fg_layer_redo_stack.clear();
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

void MyGLCanvas::RestoreUndoState(const std::shared_ptr<Tilemap3D>& state) {
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
    m_heightmap_dragging_select = false;
    m_heightmap_dragging_draw = false;
    m_heightmap_dragging_line = false;
    m_heightmap_dragging_selection_move = false;
    m_heightmap_draw_dirty = false;
    m_heightmap_line_preview_cells.clear();
    m_heightmap_selection_move_values.clear();
    m_heightmap_selection_drag_base.clear();
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

bool MyGLCanvas::IsObjectHistoryMode() const {
    return m_editor_mode == EditorMode::Room;
}

bool MyGLCanvas::IsBackgroundLayerHistoryMode() const {
    return m_editor_mode == EditorMode::BackgroundLayer;
}

bool MyGLCanvas::IsForegroundLayerHistoryMode() const {
    return m_editor_mode == EditorMode::ForegroundLayer;
}

MyGLCanvas::LayerUndoState MyGLCanvas::BuildLayerUndoState(Tilemap3D::Layer layer) const {
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

void MyGLCanvas::RestoreLayerUndoState(const LayerUndoState& state) {
    auto map = CurrentRoomMap();
    if (!map) {
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

    m_layer_dragging_select = false;
    m_layer_dragging_draw = false;
    m_layer_dragging_selection_move = false;
    m_layer_dragging_line = false;
    m_layer_draw_dirty = false;
    m_layer_selection_drag_base.clear();
    m_layer_selection_move_values.clear();
    m_layer_line_preview_cells.clear();
    m_layer_last_draw_x = -1;
    m_layer_last_draw_y = -1;
    m_layer_line_start_x = -1;
    m_layer_line_start_y = -1;
    m_layer_line_end_x = -1;
    m_layer_line_end_y = -1;
    m_layer_selection_move_anchor_x = -1;
    m_layer_selection_move_anchor_y = -1;
    m_layer_selection_move_delta_x = 0;
    m_layer_selection_move_delta_y = 0;

    ClampBackgroundSelection();
    ReloadCurrentRoomMapView();
    NotifyLayerBlockSelected();
    UpdateStatusBar();
    Refresh();
}

std::vector<Entity> MyGLCanvas::BuildCurrentRoomEntities() const {
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

std::vector<WarpList::Warp> MyGLCanvas::BuildCurrentRoomWarps() const {
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

MyGLCanvas::ObjectUndoState MyGLCanvas::BuildObjectUndoState() const {
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

void MyGLCanvas::CaptureObjectUndoState() {
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

void MyGLCanvas::RestoreObjectUndoState(const ObjectUndoState& state) {
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

void MyGLCanvas::ClearUndoRedoHistory() {
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

void MyGLCanvas::Undo() {
    if (!CanUndo()) {
        return;
    }

    if (IsObjectHistoryMode()) {
        m_object_redo_stack.push_back(BuildObjectUndoState());
        auto previous = m_object_undo_stack.back();
        m_object_undo_stack.pop_back();
        RestoreObjectUndoState(previous);
    } else if (IsBackgroundLayerHistoryMode()) {
        m_bg_layer_redo_stack.push_back(BuildLayerUndoState(Tilemap3D::Layer::BG));
        auto previous = m_bg_layer_undo_stack.back();
        m_bg_layer_undo_stack.pop_back();
        RestoreLayerUndoState(previous);
    } else if (IsForegroundLayerHistoryMode()) {
        m_fg_layer_redo_stack.push_back(BuildLayerUndoState(Tilemap3D::Layer::FG));
        auto previous = m_fg_layer_undo_stack.back();
        m_fg_layer_undo_stack.pop_back();
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

void MyGLCanvas::Redo() {
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
        m_bg_layer_undo_stack.push_back(BuildLayerUndoState(Tilemap3D::Layer::BG));
        if (m_bg_layer_undo_stack.size() > kMaxUndoStates) {
            m_bg_layer_undo_stack.erase(m_bg_layer_undo_stack.begin());
        }
        auto next = m_bg_layer_redo_stack.back();
        m_bg_layer_redo_stack.pop_back();
        RestoreLayerUndoState(next);
    } else if (IsForegroundLayerHistoryMode()) {
        m_fg_layer_undo_stack.push_back(BuildLayerUndoState(Tilemap3D::Layer::FG));
        if (m_fg_layer_undo_stack.size() > kMaxUndoStates) {
            m_fg_layer_undo_stack.erase(m_fg_layer_undo_stack.begin());
        }
        auto next = m_fg_layer_redo_stack.back();
        m_fg_layer_redo_stack.pop_back();
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
    if (entity_idx < 0 || entity_idx >= static_cast<int>(m_instances.size())) {
        return;
    }

    CaptureObjectUndoState();
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
    m_heightmapRenderer.SetZExtent(m_heightmap_z_scale * kHeightmapEditorMaxZExtent);
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

bool MyGLCanvas::SelectHeightmapCellAt(const wxPoint& point) {
    int hover_x = -1;
    int hover_y = -1;
    if (!HeightmapCellAt(point, hover_x, hover_y)) {
        return false;
    }

    m_background_selected_x = hover_x;
    m_background_selected_y = hover_y;
    m_heightmap_selection_anchor_x = hover_x;
    m_heightmap_selection_anchor_y = hover_y;
    m_background_has_selection = true;
    m_heightmap_selected_cells.clear();
    m_heightmap_selected_cells.insert({hover_x, hover_y});
    NotifyHeightmapTargetChanged();
    return true;
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

bool MyGLCanvas::SelectBackgroundCellAt(const wxPoint& point) {
    int cell_x = -1;
    int cell_y = -1;
    if (!BackgroundCellAt(point, cell_x, cell_y)) {
        return false;
    }

    m_background_selected_x = cell_x;
    m_background_selected_y = cell_y;
    m_heightmap_selection_anchor_x = cell_x;
    m_heightmap_selection_anchor_y = cell_y;
    m_background_has_selection = true;
    m_layer_selected_cells.clear();
    m_layer_selected_cells.insert({cell_x, cell_y});
    m_layer_selection_anchor_x = cell_x;
    m_layer_selection_anchor_y = cell_y;
    NotifyLayerBlockSelected();
    return true;
}

void MyGLCanvas::ClearEditSelection() {
    m_background_has_selection = false;
    m_background_selected_x = 0;
    m_background_selected_y = 0;
    m_heightmap_selection_anchor_x = 0;
    m_heightmap_selection_anchor_y = 0;
    m_heightmap_selection_drag_anchor_x = 0;
    m_heightmap_selection_drag_anchor_y = 0;
    m_layer_selection_anchor_x = 0;
    m_layer_selection_anchor_y = 0;
    m_layer_selection_drag_anchor_x = 0;
    m_layer_selection_drag_anchor_y = 0;
    m_layer_selection_move_anchor_x = -1;
    m_layer_selection_move_anchor_y = -1;
    m_layer_selection_move_delta_x = 0;
    m_layer_selection_move_delta_y = 0;
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
    m_heightmap_selected_cells.clear();
    m_heightmap_selection_drag_base.clear();
    m_layer_selected_cells.clear();
    m_layer_selection_drag_base.clear();
    m_layer_dragging_selection_move = false;
    m_layer_selection_parallelogram = false;
    m_layer_selection_move_values.clear();
    m_heightmap_line_preview_cells.clear();
    m_heightmap_selection_move_values.clear();
    if (HasCapture()) {
        ReleaseMouse();
    }
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::BeginLayerSelectionDrag(int x, int y, bool add_to_selection, bool subtract_from_selection, bool parallelogram_selection) {
    m_layer_dragging_select = true;
    m_layer_selection_add = add_to_selection && !subtract_from_selection;
    m_layer_selection_subtract = subtract_from_selection;
    m_layer_selection_parallelogram = parallelogram_selection;
    m_layer_selection_drag_base = m_layer_selected_cells;
    m_layer_selection_drag_anchor_x = x;
    m_layer_selection_drag_anchor_y = y;

    if (!m_layer_selection_add && !m_layer_selection_subtract) {
        m_layer_selection_drag_base.clear();
    }

    if (!m_layer_selection_subtract) {
        m_layer_selection_anchor_x = x;
        m_layer_selection_anchor_y = y;
    }
    m_background_selected_x = x;
    m_background_selected_y = y;
    UpdateLayerSelectionDrag(x, y);
    NotifyLayerBlockSelected();
}

void MyGLCanvas::UpdateLayerSelectionDrag(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    if (width <= 0 || height <= 0) {
        return;
    }
    m_background_selected_x = std::clamp(x, 0, width - 1);
    m_background_selected_y = std::clamp(y, 0, height - 1);
    m_background_has_selection = true;

    m_layer_selected_cells = m_layer_selection_drag_base;
    auto apply_cell = [this, width, height](int cell_x, int cell_y) {
        if (cell_x < 0 || cell_y < 0 || cell_x >= width || cell_y >= height) {
            return;
        }
        if (m_layer_selection_subtract) {
            m_layer_selected_cells.erase({cell_x, cell_y});
        } else {
            m_layer_selected_cells.insert({cell_x, cell_y});
        }
    };

    if (m_layer_selection_parallelogram) {
        int anchor_u = m_layer_selection_drag_anchor_x - m_layer_selection_drag_anchor_y;
        int target_u = x - y;
        int min_u = std::min(anchor_u, target_u);
        int max_u = std::max(anchor_u, target_u);
        bool track_x_axis = target_u < anchor_u ||
            (target_u == anchor_u && x < m_layer_selection_drag_anchor_x);
        int anchor_axis = track_x_axis ? m_layer_selection_drag_anchor_x : m_layer_selection_drag_anchor_y;
        int target_axis = track_x_axis ? x : y;
        int min_axis = std::min(anchor_axis, target_axis);
        int max_axis = std::max(anchor_axis, target_axis);
        for (int cell_y = 0; cell_y < height; ++cell_y) {
            for (int cell_x = 0; cell_x < width; ++cell_x) {
                int u = cell_x - cell_y;
                int axis = track_x_axis ? cell_x : cell_y;
                if (u >= min_u && u <= max_u && axis >= min_axis && axis <= max_axis) {
                    apply_cell(cell_x, cell_y);
                }
            }
        }
    } else {
        int min_x = std::min(m_layer_selection_drag_anchor_x, x);
        int max_x = std::max(m_layer_selection_drag_anchor_x, x);
        int min_y = std::min(m_layer_selection_drag_anchor_y, y);
        int max_y = std::max(m_layer_selection_drag_anchor_y, y);
        for (int cell_y = std::max(min_y, 0); cell_y <= std::min(max_y, height - 1); ++cell_y) {
            for (int cell_x = std::max(min_x, 0); cell_x <= std::min(max_x, width - 1); ++cell_x) {
                apply_cell(cell_x, cell_y);
            }
        }
    }

    if (m_layer_selected_cells.empty()) {
        ClearEditSelection();
        return;
    }

    if (m_layer_selected_cells.find({m_layer_selection_anchor_x, m_layer_selection_anchor_y}) == m_layer_selected_cells.end()) {
        const auto& primary = *m_layer_selected_cells.begin();
        m_layer_selection_anchor_x = primary.first;
        m_layer_selection_anchor_y = primary.second;
    }
    m_background_selected_x = m_layer_selection_anchor_x;
    m_background_selected_y = m_layer_selection_anchor_y;
}

void MyGLCanvas::FinishLayerSelectionDrag() {
    m_layer_dragging_select = false;
    m_layer_selection_add = false;
    m_layer_selection_subtract = false;
    m_layer_selection_parallelogram = false;
    m_layer_selection_drag_base.clear();
    NotifyLayerBlockSelected();
}

bool MyGLCanvas::IsLayerCellSelected(int x, int y) const {
    return m_layer_selected_cells.find({x, y}) != m_layer_selected_cells.end();
}

void MyGLCanvas::BeginLayerSelectionMoveDrag(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map || !IsLayerCellSelected(x, y)) {
        return;
    }

    m_layer_dragging_selection_move = true;
    m_layer_selection_move_anchor_x = x;
    m_layer_selection_move_anchor_y = y;
    m_layer_selection_move_delta_x = 0;
    m_layer_selection_move_delta_y = 0;
    m_layer_selection_move_values.clear();

    Tilemap3D::Layer layer = CurrentEditLayer();
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    for (const auto& cell : m_layer_selected_cells) {
        int cell_x = cell.first;
        int cell_y = cell.second;
        if (cell_x < 0 || cell_y < 0 || cell_x >= width || cell_y >= height) {
            continue;
        }
        int block_index = cell_y * width + cell_x;
        if (block_index < 0 || block_index >= map->GetWidth() * map->GetHeight()) {
            continue;
        }
        m_layer_selection_move_values[cell] = map->GetBlock(static_cast<uint16_t>(block_index), layer).value;
    }
}

void MyGLCanvas::UpdateLayerSelectionMoveDrag(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map || !m_layer_dragging_selection_move) {
        return;
    }

    int dx = x - m_layer_selection_move_anchor_x;
    int dy = y - m_layer_selection_move_anchor_y;
    int min_dx = 0;
    int max_dx = 0;
    int min_dy = 0;
    int max_dy = 0;
    bool first = true;
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    for (const auto& cell : m_layer_selected_cells) {
        int cell_min_dx = -cell.first;
        int cell_max_dx = width - 1 - cell.first;
        int cell_min_dy = -cell.second;
        int cell_max_dy = height - 1 - cell.second;
        if (first) {
            min_dx = cell_min_dx;
            max_dx = cell_max_dx;
            min_dy = cell_min_dy;
            max_dy = cell_max_dy;
            first = false;
        } else {
            min_dx = std::max(min_dx, cell_min_dx);
            max_dx = std::min(max_dx, cell_max_dx);
            min_dy = std::max(min_dy, cell_min_dy);
            max_dy = std::min(max_dy, cell_max_dy);
        }
    }

    m_layer_selection_move_delta_x = std::clamp(dx, min_dx, max_dx);
    m_layer_selection_move_delta_y = std::clamp(dy, min_dy, max_dy);
}

void MyGLCanvas::CommitLayerSelectionMoveDrag() {
    auto map = CurrentRoomMap();
    if (!map || !m_layer_dragging_selection_move) {
        CancelLayerSelectionMoveDrag();
        return;
    }

    int dx = m_layer_selection_move_delta_x;
    int dy = m_layer_selection_move_delta_y;
    if (dx == 0 && dy == 0) {
        CancelLayerSelectionMoveDrag();
        return;
    }

    CaptureUndoState();

    Tilemap3D::Layer layer = CurrentEditLayer();
    const int width = m_mapRenderer.GetRoomWidth();
    for (const auto& source : m_layer_selection_move_values) {
        int x = source.first.first;
        int y = source.first.second;
        int block_index = y * width + x;
        map->SetBlock(0, static_cast<uint16_t>(block_index), layer);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetBlock(0, static_cast<uint16_t>(block_index), layer);
        }
    }

    std::set<std::pair<int, int>> moved_selection;
    for (const auto& source : m_layer_selection_move_values) {
        int x = source.first.first + dx;
        int y = source.first.second + dy;
        int block_index = y * width + x;
        map->SetBlock(source.second, static_cast<uint16_t>(block_index), layer);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetBlock(source.second, static_cast<uint16_t>(block_index), layer);
        }
        moved_selection.insert({x, y});
    }

    m_layer_selected_cells = std::move(moved_selection);
    m_layer_selection_anchor_x += dx;
    m_layer_selection_anchor_y += dy;
    m_background_selected_x = m_layer_selection_anchor_x;
    m_background_selected_y = m_layer_selection_anchor_y;
    m_background_has_selection = !m_layer_selected_cells.empty();
    m_layer_dragging_selection_move = false;
    m_layer_selection_move_values.clear();
    m_layer_selection_move_anchor_x = -1;
    m_layer_selection_move_anchor_y = -1;
    m_layer_selection_move_delta_x = 0;
    m_layer_selection_move_delta_y = 0;
    ReloadCurrentRoomMapView();
    NotifyLayerBlockSelected();
}

void MyGLCanvas::CancelLayerSelectionMoveDrag() {
    m_layer_dragging_selection_move = false;
    m_layer_selection_move_anchor_x = -1;
    m_layer_selection_move_anchor_y = -1;
    m_layer_selection_move_delta_x = 0;
    m_layer_selection_move_delta_y = 0;
    m_layer_selection_move_values.clear();
}

std::pair<int, int> MyGLCanvas::SnapLayerLineEnd(int start_x, int start_y, int end_x, int end_y) const {
    int dx = end_x - start_x;
    int dy = end_y - start_y;
    if (dx == 0 && dy == 0) {
        return {end_x, end_y};
    }

    int k = static_cast<int>(std::round(static_cast<double>(dx + dy) * 0.5));

    std::array<std::pair<int, int>, 3> candidates{{
        {end_x, start_y},
        {start_x, end_y},
        {start_x + k, start_y + k}
    }};

    auto distance_sq = [end_x, end_y](const std::pair<int, int>& point) {
        int ddx = point.first - end_x;
        int ddy = point.second - end_y;
        return ddx * ddx + ddy * ddy;
    };
    return *std::min_element(candidates.begin(), candidates.end(), [&](const auto& a, const auto& b) {
        return distance_sq(a) < distance_sq(b);
    });
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildLayerScreenLineCells(int start_x, int start_y, int end_x, int end_y) const {
    std::set<std::pair<int, int>> unique_cells;
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    if (width <= 0 || height <= 0) {
        return {};
    }

    auto to_screen = [](int x, int y) {
        return PickPoint{
            32.0f * static_cast<float>(x - y),
            16.0f * static_cast<float>(x + y)
        };
    };
    auto add_screen_point = [&](float sx, float sy) {
        float map_x = ((sy / 16.0f) + (sx / 32.0f)) * 0.5f;
        float map_y = ((sy / 16.0f) - (sx / 32.0f)) * 0.5f;
        int cell_x = static_cast<int>(std::round(map_x));
        int cell_y = static_cast<int>(std::round(map_y));
        if (cell_x >= 0 && cell_y >= 0 && cell_x < width && cell_y < height) {
            unique_cells.insert({cell_x, cell_y});
        }
    };

    PickPoint start = to_screen(start_x, start_y);
    PickPoint end = to_screen(end_x, end_y);
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    int steps = std::max(1, static_cast<int>(std::ceil(distance / 8.0f)));
    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        add_screen_point(start.x + dx * t, start.y + dy * t);
    }

    return {unique_cells.begin(), unique_cells.end()};
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildLayerScreenCircleCells(
    int start_x,
    int start_y,
    int end_x,
    int end_y,
    bool filled) const {
    std::vector<std::pair<int, int>> cells;
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();

    auto to_screen = [](int x, int y) {
        return PickPoint{
            32.0f * static_cast<float>(x - y),
            16.0f * static_cast<float>(x + y)
        };
    };

    PickPoint start = to_screen(start_x, start_y);
    PickPoint end = to_screen(end_x, end_y);
    float min_x = std::min(start.x, end.x);
    float max_x = std::max(start.x, end.x);
    float min_y = std::min(start.y, end.y);
    float max_y = std::max(start.y, end.y);
    float radius_x = std::max(16.0f, (max_x - min_x + 32.0f) * 0.5f);
    float radius_y = std::max(8.0f, (max_y - min_y + 16.0f) * 0.5f);
    float center_x = (min_x + max_x) * 0.5f;
    float center_y = (min_y + max_y) * 0.5f;

    auto inside = [&](int x, int y) {
        PickPoint point = to_screen(x, y);
        float dx = (point.x - center_x) / radius_x;
        float dy = (point.y - center_y) / radius_y;
        return dx * dx + dy * dy <= 1.0f;
    };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!inside(x, y)) {
                continue;
            }
            bool outline = !inside(x - 1, y) || !inside(x + 1, y) ||
                           !inside(x, y - 1) || !inside(x, y + 1);
            if (filled || outline) {
                cells.push_back({x, y});
            }
        }
    }

    return cells;
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildLayerSkewRectCells(
    int start_x,
    int start_y,
    int end_x,
    int end_y,
    bool filled,
    bool lock_equal) const {
    std::vector<std::pair<int, int>> cells;
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    int anchor_u = start_x - start_y;
    int target_u = end_x - end_y;
    bool track_x_axis = target_u < anchor_u || (target_u == anchor_u && end_x < start_x);
    int anchor_axis = track_x_axis ? start_x : start_y;
    int target_axis = track_x_axis ? end_x : end_y;

    if (lock_equal) {
        int du = target_u - anchor_u;
        int da = target_axis - anchor_axis;
        int size = std::max(std::abs(du), std::abs(da));
        int step_u = du < 0 ? -1 : 1;
        int step_axis = da < 0 ? -1 : 1;
        target_u = anchor_u + step_u * size;
        target_axis = anchor_axis + step_axis * size;
    }

    int min_u = std::min(anchor_u, target_u);
    int max_u = std::max(anchor_u, target_u);
    int min_axis = std::min(anchor_axis, target_axis);
    int max_axis = std::max(anchor_axis, target_axis);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int u = x - y;
            int axis = track_x_axis ? x : y;
            if (u < min_u || u > max_u || axis < min_axis || axis > max_axis) {
                continue;
            }
            if (filled || u == min_u || u == max_u || axis == min_axis || axis == max_axis) {
                cells.push_back({x, y});
            }
        }
    }

    return cells;
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildLayerSkewCircleCells(
    int start_x,
    int start_y,
    int end_x,
    int end_y,
    bool filled,
    bool lock_equal) const {
    std::vector<std::pair<int, int>> cells;
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    int anchor_u = start_x - start_y;
    int target_u = end_x - end_y;
    bool track_x_axis = target_u < anchor_u || (target_u == anchor_u && end_x < start_x);
    int anchor_axis = track_x_axis ? start_x : start_y;
    int target_axis = track_x_axis ? end_x : end_y;

    if (lock_equal) {
        int du = target_u - anchor_u;
        int da = target_axis - anchor_axis;
        int size = std::max(std::abs(du), std::abs(da));
        int step_u = du < 0 ? -1 : 1;
        int step_axis = da < 0 ? -1 : 1;
        target_u = anchor_u + step_u * size;
        target_axis = anchor_axis + step_axis * size;
    }

    int min_u = std::min(anchor_u, target_u);
    int max_u = std::max(anchor_u, target_u);
    int min_axis = std::min(anchor_axis, target_axis);
    int max_axis = std::max(anchor_axis, target_axis);
    float radius_u = std::max(0.5f, (static_cast<float>(max_u - min_u) + 1.0f) * 0.5f);
    float radius_axis = std::max(0.5f, (static_cast<float>(max_axis - min_axis) + 1.0f) * 0.5f);
    float center_u = (static_cast<float>(min_u) + static_cast<float>(max_u) + 1.0f) * 0.5f;
    float center_axis = (static_cast<float>(min_axis) + static_cast<float>(max_axis) + 1.0f) * 0.5f;

    auto inside = [&](int u, int axis) {
        float du = (static_cast<float>(u) + 0.5f - center_u) / radius_u;
        float da = (static_cast<float>(axis) + 0.5f - center_axis) / radius_axis;
        return du * du + da * da <= 1.0f;
    };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int u = x - y;
            int axis = track_x_axis ? x : y;
            if (!inside(u, axis)) {
                continue;
            }
            bool outline = !inside(u - 1, axis) || !inside(u + 1, axis) ||
                           !inside(u, axis - 1) || !inside(u, axis + 1);
            if (filled || outline) {
                cells.push_back({x, y});
            }
        }
    }

    return cells;
}

void MyGLCanvas::BeginLayerLineDrag(int x, int y, bool shift_down, bool alt_down) {
    m_layer_dragging_line = true;
    m_layer_line_start_x = x;
    m_layer_line_start_y = y;
    UpdateLayerLineDrag(x, y, shift_down, alt_down);
}

void MyGLCanvas::UpdateLayerLineDrag(int x, int y, bool shift_down, bool alt_down) {
    auto map = CurrentRoomMap();
    if (!map || !m_layer_dragging_line) {
        return;
    }

    std::pair<int, int> end{x, y};
    if (m_drawing_tool == DrawingTool::Line) {
        end = shift_down ? end : SnapLayerLineEnd(m_layer_line_start_x, m_layer_line_start_y, x, y);
    } else if (shift_down && !alt_down) {
        int dx = x - m_layer_line_start_x;
        int dy = y - m_layer_line_start_y;
        int size = std::max(std::abs(dx), std::abs(dy));
        int step_x = dx < 0 ? -1 : 1;
        int step_y = dy < 0 ? -1 : 1;
        end = {
            m_layer_line_start_x + step_x * size,
            m_layer_line_start_y + step_y * size
        };
    }
    int end_x = end.first;
    int end_y = end.second;

    m_layer_line_end_x = end_x;
    m_layer_line_end_y = end_y;
    switch (m_drawing_tool) {
        case DrawingTool::FilledRect:
            m_layer_line_preview_cells = alt_down ?
                BuildLayerSkewRectCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, true, shift_down) :
                BuildHeightmapRectCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, true);
            break;
        case DrawingTool::OutlineRect:
            m_layer_line_preview_cells = alt_down ?
                BuildLayerSkewRectCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, false, shift_down) :
                BuildHeightmapRectCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, false);
            break;
        case DrawingTool::FilledCircle:
            m_layer_line_preview_cells = alt_down ?
                BuildLayerSkewCircleCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, true, shift_down) :
                BuildLayerScreenCircleCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, true);
            break;
        case DrawingTool::OutlineCircle:
            m_layer_line_preview_cells = alt_down ?
                BuildLayerSkewCircleCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, false, shift_down) :
                BuildLayerScreenCircleCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y, false);
            break;
        default:
            m_layer_line_preview_cells = shift_down ?
                BuildLayerScreenLineCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y) :
                BuildHeightmapLineCells(m_layer_line_start_x, m_layer_line_start_y, end_x, end_y);
            break;
    }
}

void MyGLCanvas::CommitLayerLineDrag() {
    if (!m_layer_dragging_line || !m_background_clipboard_valid) {
        CancelLayerLineDrag();
        return;
    }

    bool changed = false;
    for (const auto& cell : m_layer_line_preview_cells) {
        changed = PasteBackgroundBlockAt(cell.first, cell.second, true) || changed;
    }
    m_layer_dragging_line = false;
    m_layer_line_preview_cells.clear();
    m_layer_line_start_x = -1;
    m_layer_line_start_y = -1;
    m_layer_line_end_x = -1;
    m_layer_line_end_y = -1;
    if (changed) {
        CommitLayerDrawStroke();
    } else {
        m_layer_draw_dirty = false;
    }
}

void MyGLCanvas::CancelLayerLineDrag() {
    m_layer_dragging_line = false;
    m_layer_line_preview_cells.clear();
    m_layer_line_start_x = -1;
    m_layer_line_start_y = -1;
    m_layer_line_end_x = -1;
    m_layer_line_end_y = -1;
    m_layer_draw_dirty = false;
}

void MyGLCanvas::BeginHeightmapSelectionDrag(int x, int y, bool add_to_selection, bool subtract_from_selection) {
    m_heightmap_dragging_select = true;
    m_heightmap_selection_add = add_to_selection && !subtract_from_selection;
    m_heightmap_selection_subtract = subtract_from_selection;
    m_heightmap_selection_drag_base = m_heightmap_selected_cells;
    m_heightmap_selection_drag_anchor_x = x;
    m_heightmap_selection_drag_anchor_y = y;

    if (!m_heightmap_selection_add && !m_heightmap_selection_subtract) {
        m_heightmap_selection_drag_base.clear();
    }

    if (!m_heightmap_selection_subtract) {
        m_heightmap_selection_anchor_x = x;
        m_heightmap_selection_anchor_y = y;
    }
    m_background_selected_x = x;
    m_background_selected_y = y;
    UpdateHeightmapSelectionDrag(x, y);
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::UpdateHeightmapSelectionDrag(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    const int width = map->GetHeightmapWidth();
    const int height = map->GetHeightmapHeight();
    if (width <= 0 || height <= 0) {
        return;
    }

    m_background_selected_x = std::clamp(x, 0, width - 1);
    m_background_selected_y = std::clamp(y, 0, height - 1);
    m_background_has_selection = true;

    int min_x = std::min(m_heightmap_selection_drag_anchor_x, x);
    int max_x = std::max(m_heightmap_selection_drag_anchor_x, x);
    int min_y = std::min(m_heightmap_selection_drag_anchor_y, y);
    int max_y = std::max(m_heightmap_selection_drag_anchor_y, y);

    m_heightmap_selected_cells = m_heightmap_selection_drag_base;
    for (int cell_y = std::max(min_y, 0); cell_y <= std::min(max_y, height - 1); ++cell_y) {
        for (int cell_x = std::max(min_x, 0); cell_x <= std::min(max_x, width - 1); ++cell_x) {
            if (m_heightmap_selection_subtract) {
                m_heightmap_selected_cells.erase({cell_x, cell_y});
            } else {
                m_heightmap_selected_cells.insert({cell_x, cell_y});
            }
        }
    }

    if (m_heightmap_selected_cells.empty()) {
        ClearEditSelection();
        return;
    }

    if (m_heightmap_selected_cells.find({m_heightmap_selection_anchor_x, m_heightmap_selection_anchor_y}) == m_heightmap_selected_cells.end()) {
        const auto& primary = *m_heightmap_selected_cells.begin();
        m_heightmap_selection_anchor_x = primary.first;
        m_heightmap_selection_anchor_y = primary.second;
    }
}

void MyGLCanvas::FinishHeightmapSelectionDrag() {
    m_heightmap_dragging_select = false;
    m_heightmap_selection_add = false;
    m_heightmap_selection_subtract = false;
    m_heightmap_selection_drag_base.clear();
    NotifyHeightmapTargetChanged();
}

bool MyGLCanvas::IsHeightmapCellSelected(int x, int y) const {
    return m_heightmap_selected_cells.find({x, y}) != m_heightmap_selected_cells.end();
}

void MyGLCanvas::BeginHeightmapSelectionMoveDrag(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map || !IsHeightmapCellSelected(x, y)) {
        return;
    }

    m_heightmap_dragging_selection_move = true;
    m_heightmap_selection_move_anchor_x = x;
    m_heightmap_selection_move_anchor_y = y;
    m_heightmap_selection_move_delta_x = 0;
    m_heightmap_selection_move_delta_y = 0;
    m_heightmap_selection_move_values.clear();
    m_heightmap_line_preview_cells.clear();

    for (const auto& cell : m_heightmap_selected_cells) {
        int cell_x = cell.first;
        int cell_y = cell.second;
        if (cell_x < 0 || cell_y < 0 || cell_x >= map->GetHeightmapWidth() || cell_y >= map->GetHeightmapHeight()) {
            continue;
        }
        m_heightmap_selection_move_values[cell] = map->GetHeightmapCell({cell_x, cell_y});
        m_heightmap_line_preview_cells.push_back(cell);
    }
}

void MyGLCanvas::UpdateHeightmapSelectionMoveDrag(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map || !m_heightmap_dragging_selection_move) {
        return;
    }

    int dx = x - m_heightmap_selection_move_anchor_x;
    int dy = y - m_heightmap_selection_move_anchor_y;
    int min_dx = 0;
    int max_dx = 0;
    int min_dy = 0;
    int max_dy = 0;
    bool first = true;
    for (const auto& cell : m_heightmap_selected_cells) {
        int cell_x = cell.first;
        int cell_y = cell.second;
        int cell_min_dx = -cell_x;
        int cell_max_dx = map->GetHeightmapWidth() - 1 - cell_x;
        int cell_min_dy = -cell_y;
        int cell_max_dy = map->GetHeightmapHeight() - 1 - cell_y;
        if (first) {
            min_dx = cell_min_dx;
            max_dx = cell_max_dx;
            min_dy = cell_min_dy;
            max_dy = cell_max_dy;
            first = false;
        } else {
            min_dx = std::max(min_dx, cell_min_dx);
            max_dx = std::min(max_dx, cell_max_dx);
            min_dy = std::max(min_dy, cell_min_dy);
            max_dy = std::min(max_dy, cell_max_dy);
        }
    }

    m_heightmap_selection_move_delta_x = std::clamp(dx, min_dx, max_dx);
    m_heightmap_selection_move_delta_y = std::clamp(dy, min_dy, max_dy);
    m_heightmap_line_preview_cells.clear();
    for (const auto& cell : m_heightmap_selected_cells) {
        m_heightmap_line_preview_cells.push_back({
            cell.first + m_heightmap_selection_move_delta_x,
            cell.second + m_heightmap_selection_move_delta_y
        });
    }
}

void MyGLCanvas::CommitHeightmapSelectionMoveDrag() {
    auto map = CurrentRoomMap();
    if (!map || !m_heightmap_dragging_selection_move) {
        CancelHeightmapSelectionMoveDrag();
        return;
    }

    int dx = m_heightmap_selection_move_delta_x;
    int dy = m_heightmap_selection_move_delta_y;
    if (dx == 0 && dy == 0) {
        CancelHeightmapSelectionMoveDrag();
        return;
    }

    CaptureUndoState();

    static constexpr uint16_t kClearedHeightmapCell = 0x4000;
    for (const auto& source : m_heightmap_selection_move_values) {
        int x = source.first.first;
        int y = source.first.second;
        map->SetHeightmapCell({x, y}, kClearedHeightmapCell);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, kClearedHeightmapCell);
        }
    }

    std::set<std::pair<int, int>> moved_selection;
    for (const auto& source : m_heightmap_selection_move_values) {
        int x = source.first.first + dx;
        int y = source.first.second + dy;
        map->SetHeightmapCell({x, y}, source.second);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, source.second);
        }
        moved_selection.insert({x, y});
    }

    m_heightmap_selected_cells = std::move(moved_selection);
    if (!m_heightmap_selected_cells.empty()) {
        auto primary = m_heightmap_selected_cells.find({
            m_heightmap_selection_anchor_x + dx,
            m_heightmap_selection_anchor_y + dy
        });
        if (primary == m_heightmap_selected_cells.end()) {
            primary = m_heightmap_selected_cells.begin();
        }
        m_heightmap_selection_anchor_x = primary->first;
        m_heightmap_selection_anchor_y = primary->second;
        m_background_selected_x = primary->first;
        m_background_selected_y = primary->second;
        m_background_has_selection = true;
    }

    m_heightmap_dragging_selection_move = false;
    m_heightmap_selection_move_values.clear();
    m_heightmap_line_preview_cells.clear();
    m_heightmap_selection_move_delta_x = 0;
    m_heightmap_selection_move_delta_y = 0;
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::CancelHeightmapSelectionMoveDrag() {
    m_heightmap_dragging_selection_move = false;
    m_heightmap_selection_move_anchor_x = -1;
    m_heightmap_selection_move_anchor_y = -1;
    m_heightmap_selection_move_delta_x = 0;
    m_heightmap_selection_move_delta_y = 0;
    m_heightmap_selection_move_values.clear();
    m_heightmap_line_preview_cells.clear();
}

bool MyGLCanvas::IsHeightmapBrushTool() const {
    return m_drawing_tool == DrawingTool::Draw ||
           m_drawing_tool == DrawingTool::FloodFill ||
           IsHeightmapShapeTool();
}

bool MyGLCanvas::IsHeightmapShapeTool() const {
    return m_drawing_tool == DrawingTool::Line ||
           m_drawing_tool == DrawingTool::FilledRect ||
           m_drawing_tool == DrawingTool::OutlineRect ||
           m_drawing_tool == DrawingTool::FilledCircle ||
           m_drawing_tool == DrawingTool::OutlineCircle;
}

bool MyGLCanvas::IsHeightmapPreviewTool() const {
    return IsHeightmapBrushTool() || m_drawing_tool == DrawingTool::Stamp;
}

std::pair<int, int> MyGLCanvas::SnapHeightmapLineEnd(int start_x, int start_y, int end_x, int end_y) const {
    int dx = end_x - start_x;
    int dy = end_y - start_y;
    int abs_dx = std::abs(dx);
    int abs_dy = std::abs(dy);

    if (abs_dx == 0 && abs_dy == 0) {
        return {end_x, end_y};
    }
    if (abs_dx * 2 < abs_dy) {
        return {start_x, end_y};
    }
    if (abs_dy * 2 < abs_dx) {
        return {end_x, start_y};
    }

    int distance = std::max(abs_dx, abs_dy);
    int snapped_x = start_x + (dx < 0 ? -distance : distance);
    int snapped_y = start_y + (dy < 0 ? -distance : distance);
    return {snapped_x, snapped_y};
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildHeightmapLineCells(int start_x, int start_y, int end_x, int end_y) const {
    std::vector<std::pair<int, int>> cells;
    int dx = std::abs(end_x - start_x);
    int sx = start_x < end_x ? 1 : -1;
    int dy = -std::abs(end_y - start_y);
    int sy = start_y < end_y ? 1 : -1;
    int err = dx + dy;

    while (true) {
        cells.push_back({start_x, start_y});
        if (start_x == end_x && start_y == end_y) {
            break;
        }
        int twice_err = 2 * err;
        if (twice_err >= dy) {
            err += dy;
            start_x += sx;
        }
        if (twice_err <= dx) {
            err += dx;
            start_y += sy;
        }
    }

    return cells;
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildHeightmapRectCells(int start_x, int start_y, int end_x, int end_y, bool filled) const {
    std::vector<std::pair<int, int>> cells;
    int min_x = std::min(start_x, end_x);
    int max_x = std::max(start_x, end_x);
    int min_y = std::min(start_y, end_y);
    int max_y = std::max(start_y, end_y);

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            if (filled || x == min_x || x == max_x || y == min_y || y == max_y) {
                cells.push_back({x, y});
            }
        }
    }
    return cells;
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildHeightmapCircleCells(int start_x, int start_y, int end_x, int end_y, bool filled) const {
    std::vector<std::pair<int, int>> cells;
    int min_x = std::min(start_x, end_x);
    int max_x = std::max(start_x, end_x);
    int min_y = std::min(start_y, end_y);
    int max_y = std::max(start_y, end_y);

    float radius_x = std::max(0.5f, (static_cast<float>(max_x - min_x) + 1.0f) * 0.5f);
    float radius_y = std::max(0.5f, (static_cast<float>(max_y - min_y) + 1.0f) * 0.5f);
    float center_x = (static_cast<float>(min_x) + static_cast<float>(max_x) + 1.0f) * 0.5f;
    float center_y = (static_cast<float>(min_y) + static_cast<float>(max_y) + 1.0f) * 0.5f;

    auto inside = [&](int x, int y) {
        float dx = (static_cast<float>(x) + 0.5f - center_x) / radius_x;
        float dy = (static_cast<float>(y) + 0.5f - center_y) / radius_y;
        return dx * dx + dy * dy <= 1.0f;
    };

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            if (!inside(x, y)) {
                continue;
            }
            bool outline = !inside(x - 1, y) || !inside(x + 1, y) || !inside(x, y - 1) || !inside(x, y + 1);
            if (filled || outline) {
                cells.push_back({x, y});
            }
        }
    }
    return cells;
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildHeightmapFloodFillCells(int x, int y) const {
    std::vector<std::pair<int, int>> cells;
    auto map = CurrentRoomMap();
    if (!map || x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
        return cells;
    }

    uint16_t target = map->GetHeightmapCell({x, y});
    std::set<std::pair<int, int>> visited;
    std::deque<std::pair<int, int>> queue;
    queue.push_back({x, y});
    visited.insert({x, y});

    while (!queue.empty()) {
        auto cell = queue.front();
        queue.pop_front();
        cells.push_back(cell);

        static constexpr std::array<std::pair<int, int>, 4> kNeighbors = {{
            {-1, 0}, {1, 0}, {0, -1}, {0, 1}
        }};
        for (const auto& delta : kNeighbors) {
            int nx = cell.first + delta.first;
            int ny = cell.second + delta.second;
            std::pair<int, int> next{nx, ny};
            if (nx < 0 || ny < 0 || nx >= map->GetHeightmapWidth() || ny >= map->GetHeightmapHeight() ||
                visited.find(next) != visited.end() ||
                map->GetHeightmapCell({nx, ny}) != target) {
                continue;
            }
            visited.insert(next);
            queue.push_back(next);
        }
    }

    return cells;
}

std::map<std::pair<int, int>, uint16_t> MyGLCanvas::BuildHeightmapStampCells(int x, int y) const {
    std::map<std::pair<int, int>, uint16_t> cells;
    auto map = CurrentRoomMap();
    int primary_x = PrimaryHeightmapCellX();
    int primary_y = PrimaryHeightmapCellY();
    if (!map || primary_x < 0 || primary_y < 0) {
        return cells;
    }

    int dx = x - primary_x;
    int dy = y - primary_y;
    for (const auto& source : m_heightmap_selected_cells) {
        int source_x = source.first;
        int source_y = source.second;
        int target_x = source_x + dx;
        int target_y = source_y + dy;
        if (source_x < 0 || source_y < 0 ||
            source_x >= map->GetHeightmapWidth() || source_y >= map->GetHeightmapHeight() ||
            target_x < 0 || target_y < 0 ||
            target_x >= map->GetHeightmapWidth() || target_y >= map->GetHeightmapHeight()) {
            continue;
        }
        cells[{target_x, target_y}] = map->GetHeightmapCell({source_x, source_y});
    }

    return cells;
}

void MyGLCanvas::ApplyHeightmapStampAt(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    bool changed = false;
    for (const auto& cell : BuildHeightmapStampCells(x, y)) {
        int target_x = cell.first.first;
        int target_y = cell.first.second;
        if (map->GetHeightmapCell({target_x, target_y}) == cell.second) {
            continue;
        }
        if (!changed) {
            CaptureUndoState();
        }
        map->SetHeightmapCell({target_x, target_y}, cell.second);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({target_x, target_y}, cell.second);
        }
        changed = true;
    }

    if (!changed) {
        return;
    }
    m_heightmap_draw_dirty = true;
    CommitHeightmapDrawStroke();
}

void MyGLCanvas::ApplyHeightmapFloodFillAt(int x, int y) {
    if (!m_heightmap_clipboard_valid) {
        return;
    }

    bool changed = false;
    for (const auto& cell : BuildHeightmapFloodFillCells(x, y)) {
        changed = PasteHeightmapCellAt(cell.first, cell.second, true) || changed;
    }
    if (changed) {
        CommitHeightmapDrawStroke();
    }
}

void MyGLCanvas::BeginHeightmapLineDrag(int x, int y, bool shift_down) {
    m_heightmap_dragging_line = true;
    m_heightmap_line_start_x = x;
    m_heightmap_line_start_y = y;
    UpdateHeightmapLineDrag(x, y, shift_down);
}

void MyGLCanvas::UpdateHeightmapLineDrag(int x, int y, bool shift_down) {
    auto map = CurrentRoomMap();
    if (!map || m_heightmap_line_start_x < 0 || m_heightmap_line_start_y < 0) {
        m_heightmap_line_preview_cells.clear();
        return;
    }

    std::pair<int, int> end{x, y};
    if (m_drawing_tool == DrawingTool::Line) {
        end = shift_down ? end : SnapHeightmapLineEnd(m_heightmap_line_start_x, m_heightmap_line_start_y, x, y);
    } else if (shift_down) {
        int dx = x - m_heightmap_line_start_x;
        int dy = y - m_heightmap_line_start_y;
        int size = std::max(std::abs(dx), std::abs(dy));
        int step_x = dx < 0 ? -1 : 1;
        int step_y = dy < 0 ? -1 : 1;
        end = {
            m_heightmap_line_start_x + step_x * size,
            m_heightmap_line_start_y + step_y * size
        };
    }
    auto [end_x, end_y] = end;

    m_heightmap_line_end_x = end_x;
    m_heightmap_line_end_y = end_y;
    switch (m_drawing_tool) {
        case DrawingTool::FilledRect:
            m_heightmap_line_preview_cells = BuildHeightmapRectCells(m_heightmap_line_start_x, m_heightmap_line_start_y, end_x, end_y, true);
            break;
        case DrawingTool::OutlineRect:
            m_heightmap_line_preview_cells = BuildHeightmapRectCells(m_heightmap_line_start_x, m_heightmap_line_start_y, end_x, end_y, false);
            break;
        case DrawingTool::FilledCircle:
            m_heightmap_line_preview_cells = BuildHeightmapCircleCells(m_heightmap_line_start_x, m_heightmap_line_start_y, end_x, end_y, true);
            break;
        case DrawingTool::OutlineCircle:
            m_heightmap_line_preview_cells = BuildHeightmapCircleCells(m_heightmap_line_start_x, m_heightmap_line_start_y, end_x, end_y, false);
            break;
        default:
            m_heightmap_line_preview_cells = BuildHeightmapLineCells(m_heightmap_line_start_x, m_heightmap_line_start_y, end_x, end_y);
            break;
    }
}

void MyGLCanvas::CommitHeightmapLineDrag() {
    if (!m_heightmap_dragging_line || !m_heightmap_clipboard_valid) {
        CancelHeightmapLineDrag();
        return;
    }

    bool changed = false;
    for (const auto& cell : m_heightmap_line_preview_cells) {
        changed = PasteHeightmapCellAt(cell.first, cell.second, true) || changed;
    }
    m_heightmap_dragging_line = false;
    m_heightmap_line_preview_cells.clear();
    m_heightmap_line_start_x = -1;
    m_heightmap_line_start_y = -1;
    m_heightmap_line_end_x = -1;
    m_heightmap_line_end_y = -1;
    if (changed) {
        CommitHeightmapDrawStroke();
    } else {
        m_heightmap_draw_dirty = false;
    }
}

void MyGLCanvas::CancelHeightmapLineDrag() {
    m_heightmap_dragging_line = false;
    m_heightmap_line_preview_cells.clear();
    m_heightmap_line_start_x = -1;
    m_heightmap_line_start_y = -1;
    m_heightmap_line_end_x = -1;
    m_heightmap_line_end_y = -1;
    m_heightmap_draw_dirty = false;
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
    m_heightmap_selection_anchor_x = std::clamp(m_heightmap_selection_anchor_x, 0, width - 1);
    m_heightmap_selection_anchor_y = std::clamp(m_heightmap_selection_anchor_y, 0, height - 1);
    if (IsHeightmapEditMode() && map) {
        for (auto it = m_heightmap_selected_cells.begin(); it != m_heightmap_selected_cells.end(); ) {
            if (it->first < 0 || it->second < 0 || it->first >= width || it->second >= height) {
                it = m_heightmap_selected_cells.erase(it);
            } else {
                ++it;
            }
        }
        if (m_heightmap_selected_cells.empty()) {
            m_heightmap_selected_cells.insert({m_heightmap_selection_anchor_x, m_heightmap_selection_anchor_y});
        }
    }
    m_background_has_selection = true;
}

void MyGLCanvas::MoveBackgroundSelection(int dx, int dy) {
    if (!m_background_has_selection) {
        m_background_selected_x = 0;
        m_background_selected_y = 0;
        m_heightmap_selection_anchor_x = 0;
        m_heightmap_selection_anchor_y = 0;
        m_background_has_selection = true;
    }
    m_background_selected_x += dx;
    m_background_selected_y += dy;
    ClampBackgroundSelection();
    m_heightmap_selection_anchor_x = m_background_selected_x;
    m_heightmap_selection_anchor_y = m_background_selected_y;
    if (IsHeightmapEditMode()) {
        m_heightmap_selected_cells.clear();
        m_heightmap_selected_cells.insert({m_heightmap_selection_anchor_x, m_heightmap_selection_anchor_y});
        NotifyHeightmapTargetChanged();
    }
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

int MyGLCanvas::PrimaryHeightmapCellX() const {
    auto map = CurrentRoomMap();
    if (!map || !m_background_has_selection) {
        return -1;
    }
    if (m_heightmap_selection_anchor_x < 0 || m_heightmap_selection_anchor_x >= map->GetHeightmapWidth()) {
        return -1;
    }
    return m_heightmap_selection_anchor_x;
}

int MyGLCanvas::PrimaryHeightmapCellY() const {
    auto map = CurrentRoomMap();
    if (!map || !m_background_has_selection) {
        return -1;
    }
    if (m_heightmap_selection_anchor_y < 0 || m_heightmap_selection_anchor_y >= map->GetHeightmapHeight()) {
        return -1;
    }
    return m_heightmap_selection_anchor_y;
}

namespace {
uint8_t HeightmapCellType(uint16_t value) {
    return static_cast<uint8_t>(value & 0x00FF);
}

uint8_t HeightmapCellHeight(uint16_t value) {
    return static_cast<uint8_t>((value >> 8) & 0x0F);
}

uint8_t HeightmapCellProps(uint16_t value) {
    return static_cast<uint8_t>((value >> 12) & 0x0F);
}

uint16_t WithHeightmapCellType(uint16_t value, uint8_t type) {
    return static_cast<uint16_t>((value & 0xFF00) | type);
}

uint16_t WithHeightmapCellHeight(uint16_t value, uint8_t height) {
    return static_cast<uint16_t>((value & 0xF0FF) | ((height & 0x0F) << 8));
}

uint16_t WithHeightmapCellProps(uint16_t value, uint8_t props) {
    return static_cast<uint16_t>((value & 0x0FFF) | ((props & 0x0F) << 12));
}
}

uint16_t MyGLCanvas::SelectedHeightmapCellValue() const {
    if (IsHeightmapBrushTool() && m_heightmap_clipboard_valid) {
        return m_heightmap_clipboard_cell;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (map && x >= 0 && y >= 0) {
        return map->GetHeightmapCell({x, y});
    }
    return 0;
}

bool MyGLCanvas::HasSelectedLayerCell() const {
    return !m_layer_selected_cells.empty() && SelectedBackgroundBlockIndex() >= 0;
}

bool MyGLCanvas::HasSelectedHeightmapCell() const {
    return !m_heightmap_selected_cells.empty() && PrimaryHeightmapCellX() >= 0 && PrimaryHeightmapCellY() >= 0;
}

bool MyGLCanvas::HasHeightmapEditTarget() const {
    return HasSelectedHeightmapCell() || (IsHeightmapBrushTool() && m_heightmap_clipboard_valid);
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
    if (IsHeightmapBrushTool()) {
        return m_heightmap_clipboard_valid && HeightmapCellHeight(m_heightmap_clipboard_cell) < 15;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (map && x >= 0 && y >= 0) {
        return map->GetHeight({x, y}) < 15;
    }
    return false;
}

bool MyGLCanvas::CanDecreaseSelectedHeightmapHeight() const {
    if (IsHeightmapBrushTool()) {
        return m_heightmap_clipboard_valid && HeightmapCellHeight(m_heightmap_clipboard_cell) > 0;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (map && x >= 0 && y >= 0) {
        return map->GetHeight({x, y}) > 0;
    }
    return false;
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
    return HeightmapCellType(SelectedHeightmapCellValue());
}

bool MyGLCanvas::IsSelectedHeightmapPlayerPassable() const {
    return (HeightmapCellProps(SelectedHeightmapCellValue()) & 0x04) == 0;
}

bool MyGLCanvas::IsSelectedHeightmapNpcPassable() const {
    return (HeightmapCellProps(SelectedHeightmapCellValue()) & 0x02) == 0;
}

bool MyGLCanvas::IsSelectedHeightmapRaftTrack() const {
    return (HeightmapCellProps(SelectedHeightmapCellValue()) & 0x01) == 0;
}

void MyGLCanvas::SetSelectedHeightmapType(uint8_t type) {
    if (IsHeightmapBrushTool()) {
        if (m_heightmap_clipboard_valid) {
            m_heightmap_clipboard_cell = WithHeightmapCellType(m_heightmap_clipboard_cell, type);
            NotifyHeightmapTargetChanged();
            Refresh();
        }
        return;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (!map || x < 0 || y < 0) return;
    CaptureUndoState();
    map->SetCellType({x, y}, type);
    ApplyPrimaryHeightmapTypeToSelection();
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::ToggleSelectedHeightmapPlayerPassable() {
    if (IsHeightmapBrushTool()) {
        if (m_heightmap_clipboard_valid) {
            uint8_t props = HeightmapCellProps(m_heightmap_clipboard_cell);
            m_heightmap_clipboard_cell = WithHeightmapCellProps(m_heightmap_clipboard_cell, IsSelectedHeightmapPlayerPassable() ? (props | 0x04) : (props & ~0x04));
            NotifyHeightmapTargetChanged();
            Refresh();
        }
        return;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (!map || x < 0 || y < 0) return;
    CaptureUndoState();
    uint8_t props = map->GetCellProps({x, y});
    map->SetCellProps({x, y}, IsSelectedHeightmapPlayerPassable() ? (props | 0x04) : (props & ~0x04));
    ApplyPrimaryHeightmapPropsToSelection();
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::ToggleSelectedHeightmapNpcPassable() {
    if (IsHeightmapBrushTool()) {
        if (m_heightmap_clipboard_valid) {
            uint8_t props = HeightmapCellProps(m_heightmap_clipboard_cell);
            m_heightmap_clipboard_cell = WithHeightmapCellProps(m_heightmap_clipboard_cell, IsSelectedHeightmapNpcPassable() ? (props | 0x02) : (props & ~0x02));
            NotifyHeightmapTargetChanged();
            Refresh();
        }
        return;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (!map || x < 0 || y < 0) return;
    CaptureUndoState();
    uint8_t props = map->GetCellProps({x, y});
    map->SetCellProps({x, y}, IsSelectedHeightmapNpcPassable() ? (props | 0x02) : (props & ~0x02));
    ApplyPrimaryHeightmapPropsToSelection();
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::ToggleSelectedHeightmapRaftTrack() {
    if (IsHeightmapBrushTool()) {
        if (m_heightmap_clipboard_valid) {
            uint8_t props = HeightmapCellProps(m_heightmap_clipboard_cell);
            m_heightmap_clipboard_cell = WithHeightmapCellProps(m_heightmap_clipboard_cell, IsSelectedHeightmapRaftTrack() ? (props | 0x01) : (props & ~0x01));
            NotifyHeightmapTargetChanged();
            Refresh();
        }
        return;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (!map || x < 0 || y < 0) return;
    CaptureUndoState();
    uint8_t props = map->GetCellProps({x, y});
    map->SetCellProps({x, y}, IsSelectedHeightmapRaftTrack() ? (props | 0x01) : (props & ~0x01));
    ApplyPrimaryHeightmapPropsToSelection();
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::AdjustSelectedHeightmapHeight(int delta) {
    if (IsHeightmapBrushTool()) {
        if (m_heightmap_clipboard_valid) {
            int height = std::clamp(static_cast<int>(HeightmapCellHeight(m_heightmap_clipboard_cell)) + delta, 0, 15);
            m_heightmap_clipboard_cell = WithHeightmapCellHeight(m_heightmap_clipboard_cell, static_cast<uint8_t>(height));
            NotifyHeightmapTargetChanged();
            Refresh();
        }
        return;
    }
    auto map = CurrentRoomMap();
    int x = PrimaryHeightmapCellX();
    int y = PrimaryHeightmapCellY();
    if (!map || x < 0 || y < 0) return;
    CaptureUndoState();
    int height = std::clamp(static_cast<int>(map->GetHeight({x, y})) + delta, 0, 15);
    map->SetHeight({x, y}, static_cast<uint8_t>(height));
    ApplyPrimaryHeightmapHeightToSelection();
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::ClearSelectedHeightmapCells() {
    if (!HasSelectedHeightmapCell()) {
        return;
    }

    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    bool changed = false;
    static constexpr uint16_t kClearedHeightmapCell = 0x4000;
    for (const auto& cell : m_heightmap_selected_cells) {
        int x = cell.first;
        int y = cell.second;
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight() ||
            map->GetHeightmapCell({x, y}) == kClearedHeightmapCell) {
            continue;
        }
        if (!changed) {
            CaptureUndoState();
        }
        map->SetHeightmapCell({x, y}, kClearedHeightmapCell);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, kClearedHeightmapCell);
        }
        changed = true;
    }

    if (!changed) {
        return;
    }
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::ClearSelectedLayerCells() {
    auto map = CurrentRoomMap();
    if (!map || !HasSelectedLayerCell()) {
        return;
    }

    bool changed = false;
    Tilemap3D::Layer layer = CurrentEditLayer();
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    for (const auto& cell : m_layer_selected_cells) {
        int x = cell.first;
        int y = cell.second;
        if (x < 0 || y < 0 || x >= width || y >= height) {
            continue;
        }
        int block_index = y * width + x;
        if (block_index < 0 || block_index >= map->GetWidth() * map->GetHeight()) {
            continue;
        }
        if (map->GetBlock(static_cast<uint16_t>(block_index), layer).value == 0) {
            continue;
        }
        if (!changed) {
            CaptureUndoState();
        }
        map->SetBlock(0, static_cast<uint16_t>(block_index), layer);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetBlock(0, static_cast<uint16_t>(block_index), layer);
        }
        changed = true;
    }

    if (!changed) {
        return;
    }
    ReloadCurrentRoomMapView();
    NotifyLayerBlockSelected();
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
    CaptureUndoState();
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
    CaptureUndoState();
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
    CaptureUndoState();
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
    CaptureUndoState();
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
    CaptureUndoState();
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
    CaptureUndoState();
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
    CaptureUndoState();
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
    return static_cast<uint16_t>(map->GetBlock(static_cast<uint16_t>(block_index), CurrentEditLayer()).value & 0x03FF);
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
    m_background_clipboard_block_id = static_cast<uint16_t>(map->GetBlock(static_cast<uint16_t>(block_index), CurrentEditLayer()).value & 0x03FF);
    m_background_clipboard_valid = true;
    NotifyLayerBlockSelected();
}

void MyGLCanvas::CopyBackgroundBlockAt(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map || x < 0 || y < 0 || x >= m_mapRenderer.GetRoomWidth() || y >= m_mapRenderer.GetRoomHeight()) {
        return;
    }
    int block_index = y * m_mapRenderer.GetRoomWidth() + x;
    if (block_index < 0 || block_index >= map->GetWidth() * map->GetHeight()) {
        return;
    }
    m_background_clipboard_block_id = static_cast<uint16_t>(map->GetBlock(static_cast<uint16_t>(block_index), CurrentEditLayer()).value & 0x03FF);
    m_background_clipboard_valid = true;
    PostLayerBlockSelection(EventTarget(), this, static_cast<int>(m_background_clipboard_block_id));
}

void MyGLCanvas::SetSelectedBlockId(int block) {
    if (block < 0) {
        m_background_clipboard_valid = false;
        m_background_clipboard_block_id = 0;
        Refresh();
        return;
    }
    m_background_clipboard_block_id = static_cast<uint16_t>(block & 0x03FF);
    m_background_clipboard_valid = true;
    if (IsLayerEditMode()) {
        PostLayerBlockSelection(EventTarget(), this, static_cast<int>(m_background_clipboard_block_id));
    }
    Refresh();
}

void MyGLCanvas::AdjustSelectedBlockId(int delta) {
    int block = m_background_clipboard_valid ? static_cast<int>(m_background_clipboard_block_id) : static_cast<int>(SelectedBackgroundBlockId());
    SetSelectedBlockId((block + delta) & 0x03FF);
}

void MyGLCanvas::CopySelectedHeightmapCell() {
    uint16_t value = SelectedHeightmapCellValue();
    if (!m_background_has_selection) {
        return;
    }
    m_heightmap_clipboard_cell = value;
    m_heightmap_clipboard_valid = true;
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::SetSelectedHeightmapCell(uint16_t value, bool refresh_object_placements) {
    if (IsHeightmapBrushTool()) {
        m_heightmap_clipboard_cell = value;
        m_heightmap_clipboard_valid = true;
        NotifyHeightmapTargetChanged();
        Refresh();
        return;
    }
    if (!HasSelectedHeightmapCell()) {
        return;
    }

    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    bool changed = false;
    for (const auto& cell : m_heightmap_selected_cells) {
        int x = cell.first;
        int y = cell.second;
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
            continue;
        }
        if (map->GetHeightmapCell({x, y}) == value) {
            continue;
        }
        if (!changed) {
            CaptureUndoState();
        }
        map->SetHeightmapCell({x, y}, value);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, value);
        }
        changed = true;
    }

    if (!changed) {
        return;
    }
    UpdateHeightmapClipboardFromSelectedCell();
    ReloadCurrentRoomMapView();
    if (refresh_object_placements) {
        RefreshObjectPlacementsFromHeightmap();
    }
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    Refresh();
}

void MyGLCanvas::ApplyPrimaryHeightmapTypeToSelection() {
    auto map = CurrentRoomMap();
    int primary_x = PrimaryHeightmapCellX();
    int primary_y = PrimaryHeightmapCellY();
    if (!map || primary_x < 0 || primary_y < 0) return;

    uint8_t type = map->GetCellType({primary_x, primary_y});
    for (const auto& cell : m_heightmap_selected_cells) {
        int x = cell.first;
        int y = cell.second;
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) continue;
        map->SetCellType({x, y}, type);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, map->GetHeightmapCell({x, y}));
        }
    }
}

void MyGLCanvas::ApplyPrimaryHeightmapPropsToSelection() {
    auto map = CurrentRoomMap();
    int primary_x = PrimaryHeightmapCellX();
    int primary_y = PrimaryHeightmapCellY();
    if (!map || primary_x < 0 || primary_y < 0) return;

    uint8_t props = map->GetCellProps({primary_x, primary_y});
    for (const auto& cell : m_heightmap_selected_cells) {
        int x = cell.first;
        int y = cell.second;
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) continue;
        map->SetCellProps({x, y}, props);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, map->GetHeightmapCell({x, y}));
        }
    }
}

void MyGLCanvas::ApplyPrimaryHeightmapHeightToSelection() {
    auto map = CurrentRoomMap();
    int primary_x = PrimaryHeightmapCellX();
    int primary_y = PrimaryHeightmapCellY();
    if (!map || primary_x < 0 || primary_y < 0) return;

    uint8_t height = map->GetHeight({primary_x, primary_y});
    for (const auto& cell : m_heightmap_selected_cells) {
        int x = cell.first;
        int y = cell.second;
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) continue;
        map->SetHeight({x, y}, height);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetHeightmapCell({x, y}, map->GetHeightmapCell({x, y}));
        }
    }
}

void MyGLCanvas::CopyHeightmapCellAt(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map || x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
        return;
    }
    m_heightmap_clipboard_cell = map->GetHeightmapCell({x, y});
    m_heightmap_clipboard_valid = true;
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::UpdateHeightmapClipboardFromSelectedCell() {
    if (!HasSelectedHeightmapCell()) {
        return;
    }
    m_heightmap_clipboard_cell = SelectedHeightmapCellValue();
    m_heightmap_clipboard_valid = true;
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::ClearBackgroundClipboard() {
    m_background_clipboard_valid = false;
    m_background_clipboard_block_id = 0;
    m_heightmap_clipboard_valid = false;
    m_heightmap_clipboard_cell = 0;
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::ReloadCurrentRoomMapView() {
    if (m_tileswap_preview_map) {
        m_mapRenderer.LoadPreviewRoom(m_current_room, *m_tileswap_preview_map);
    } else {
        m_mapRenderer.LoadRoom(m_current_room);
    }
}

void MyGLCanvas::PasteSelectedBackgroundBlock() {
    PasteBackgroundBlockAt(m_background_selected_x, m_background_selected_y);
}

std::vector<std::pair<int, int>> MyGLCanvas::BuildLayerFloodFillCells(int x, int y) const {
    std::vector<std::pair<int, int>> cells;
    auto map = CurrentRoomMap();
    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    if (!map || x < 0 || y < 0 || x >= width || y >= height) {
        return cells;
    }

    int start_index = y * width + x;
    if (start_index < 0 || start_index >= map->GetWidth() * map->GetHeight()) {
        return cells;
    }

    Tilemap3D::Layer layer = CurrentEditLayer();
    uint16_t target = map->GetBlock(static_cast<uint16_t>(start_index), layer).value;
    std::set<std::pair<int, int>> visited;
    std::deque<std::pair<int, int>> queue;
    queue.push_back({x, y});
    visited.insert({x, y});

    while (!queue.empty()) {
        auto cell = queue.front();
        queue.pop_front();
        cells.push_back(cell);

        static constexpr std::array<std::pair<int, int>, 6> kNeighbors = {{
            {1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, -1}
        }};
        for (const auto& delta : kNeighbors) {
            int nx = cell.first + delta.first;
            int ny = cell.second + delta.second;
            std::pair<int, int> next{nx, ny};
            int block_index = ny * width + nx;
            if (nx < 0 || ny < 0 || nx >= width || ny >= height ||
                block_index < 0 || block_index >= map->GetWidth() * map->GetHeight() ||
                visited.find(next) != visited.end() ||
                map->GetBlock(static_cast<uint16_t>(block_index), layer).value != target) {
                continue;
            }
            visited.insert(next);
            queue.push_back(next);
        }
    }

    return cells;
}

void MyGLCanvas::ApplyLayerFloodFillAt(int x, int y) {
    if (!m_background_clipboard_valid) {
        return;
    }

    bool changed = false;
    for (const auto& cell : BuildLayerFloodFillCells(x, y)) {
        changed = PasteBackgroundBlockAt(cell.first, cell.second, true) || changed;
    }
    if (changed) {
        CommitLayerDrawStroke();
    }
}

std::map<std::pair<int, int>, uint16_t> MyGLCanvas::BuildLayerStampCells(int x, int y) const {
    std::map<std::pair<int, int>, uint16_t> cells;
    auto map = CurrentRoomMap();
    if (!map || m_layer_selected_cells.empty() || m_layer_selection_anchor_x < 0 || m_layer_selection_anchor_y < 0) {
        return cells;
    }

    const int width = m_mapRenderer.GetRoomWidth();
    const int height = m_mapRenderer.GetRoomHeight();
    const int dx = x - m_layer_selection_anchor_x;
    const int dy = y - m_layer_selection_anchor_y;
    Tilemap3D::Layer layer = CurrentEditLayer();
    for (const auto& source : m_layer_selected_cells) {
        int source_x = source.first;
        int source_y = source.second;
        int target_x = source_x + dx;
        int target_y = source_y + dy;
        if (source_x < 0 || source_y < 0 || source_x >= width || source_y >= height ||
            target_x < 0 || target_y < 0 || target_x >= width || target_y >= height) {
            continue;
        }

        int source_index = source_y * width + source_x;
        if (source_index < 0 || source_index >= map->GetWidth() * map->GetHeight()) {
            continue;
        }
        cells[{target_x, target_y}] = map->GetBlock(static_cast<uint16_t>(source_index), layer).value;
    }

    return cells;
}

void MyGLCanvas::ApplyLayerStampAt(int x, int y) {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }

    bool changed = false;
    const int width = m_mapRenderer.GetRoomWidth();
    Tilemap3D::Layer layer = CurrentEditLayer();
    for (const auto& cell : BuildLayerStampCells(x, y)) {
        int target_x = cell.first.first;
        int target_y = cell.first.second;
        int target_index = target_y * width + target_x;
        if (target_index < 0 || target_index >= map->GetWidth() * map->GetHeight()) {
            continue;
        }
        if (map->GetBlock(static_cast<uint16_t>(target_index), layer).value == cell.second) {
            continue;
        }
        if (!m_layer_draw_dirty) {
            CaptureUndoState();
        }
        map->SetBlock(cell.second, static_cast<uint16_t>(target_index), layer);
        if (m_tileswap_preview_map) {
            m_tileswap_preview_map->SetBlock(cell.second, static_cast<uint16_t>(target_index), layer);
        }
        m_layer_draw_dirty = true;
        changed = true;
    }

    if (changed) {
        ReloadCurrentRoomMapView();
    }
}

bool MyGLCanvas::PasteBackgroundBlockAt(int x, int y, bool defer_updates) {
    auto map = CurrentRoomMap();
    if (!m_background_clipboard_valid || !map || x < 0 || y < 0 ||
        x >= m_mapRenderer.GetRoomWidth() || y >= m_mapRenderer.GetRoomHeight()) {
        return false;
    }

    int block_index = y * m_mapRenderer.GetRoomWidth() + x;
    if (block_index < 0 || block_index >= map->GetWidth() * map->GetHeight()) {
        return false;
    }

    Tilemap3D::Layer layer = CurrentEditLayer();
    if (map->GetBlock(static_cast<uint16_t>(block_index), layer).value == m_background_clipboard_block_id) {
        return false;
    }
    if (!defer_updates || !m_layer_draw_dirty) {
        CaptureUndoState();
    }
    map->SetBlock(m_background_clipboard_block_id, static_cast<uint16_t>(block_index), layer);
    if (m_tileswap_preview_map) {
        m_tileswap_preview_map->SetBlock(m_background_clipboard_block_id, static_cast<uint16_t>(block_index), layer);
    }
    if (defer_updates) {
        m_layer_draw_dirty = true;
        ReloadCurrentRoomMapView();
        return true;
    }
    ReloadCurrentRoomMapView();
    return true;
}

void MyGLCanvas::CommitLayerDrawStroke() {
    if (!m_layer_draw_dirty) {
        return;
    }
    m_layer_draw_dirty = false;
    ReloadCurrentRoomMapView();
}

void MyGLCanvas::PasteSelectedHeightmapCell() {
    auto map = CurrentRoomMap();
    int x = SelectedHeightmapCellX();
    int y = SelectedHeightmapCellY();
    if (!map || x < 0 || y < 0) {
        return;
    }
    PasteHeightmapCellAt(x, y);
}

bool MyGLCanvas::PasteHeightmapCellAt(int x, int y, bool defer_updates) {
    auto map = CurrentRoomMap();
    if (!m_heightmap_clipboard_valid || !map || x < 0 || y < 0 ||
        x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
        return false;
    }
    if (map->GetHeightmapCell({x, y}) == m_heightmap_clipboard_cell) {
        return false;
    }
    if (!defer_updates || !m_heightmap_draw_dirty) {
        CaptureUndoState();
    }
    map->SetHeightmapCell({x, y}, m_heightmap_clipboard_cell);
    if (m_tileswap_preview_map) {
        m_tileswap_preview_map->SetHeightmapCell({x, y}, m_heightmap_clipboard_cell);
    }
    if (defer_updates) {
        m_heightmap_draw_dirty = true;
        return true;
    }
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
    return true;
}

void MyGLCanvas::CommitHeightmapDrawStroke() {
    if (!m_heightmap_draw_dirty) {
        return;
    }
    m_heightmap_draw_dirty = false;
    ReloadCurrentRoomMapView();
    RefreshObjectPlacementsFromHeightmap();
    NotifyHeightmapChanged(false);
    NotifyHeightmapTargetChanged();
}

void MyGLCanvas::ClearCurrentTilemap() {
    auto map = CurrentRoomMap();
    if (!map) {
        return;
    }
    CaptureUndoState();
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
    CaptureUndoState();
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
    CaptureUndoState();
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
    CaptureUndoState();
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
    CaptureUndoState();
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
    CaptureUndoState();
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
    CaptureUndoState();
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

        // Strong hover cue for layer edit mode: soft fill + thick double outline.
        glColor4f(0.20f, 0.68f, 1.0f, 0.20f);
        glBegin(GL_QUADS);
        glVertex2f(left, top);
        glVertex2f(right, top);
        glVertex2f(right, bottom);
        glVertex2f(left, bottom);
        glEnd();

        glLineWidth(3.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 0.96f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(left, top);
        glVertex2f(right, top);
        glVertex2f(right, bottom);
        glVertex2f(left, bottom);
        glEnd();

        glLineWidth(2.0f);
        glColor4f(0.10f, 0.78f, 1.0f, 0.96f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(left + 1.0f, top + 1.0f);
        glVertex2f(right - 1.0f, top + 1.0f);
        glVertex2f(right - 1.0f, bottom - 1.0f);
        glVertex2f(left + 1.0f, bottom - 1.0f);
        glEnd();
        glLineWidth(1.0f);
    }

    if (m_background_has_selection) {
        const bool dim_selection = m_drawing_tool == DrawingTool::Draw;
        auto draw_selected_cell = [&](int x, int y, float fill_r, float fill_g, float fill_b, float fill_a,
                                      float line_r, float line_g, float line_b, float line_a, float line_width) {
            PickPoint origin = block_screen_origin(x, y);
            float left = origin.x;
            float top = origin.y;
            float right = left + 32.0f * zoom;
            float bottom = top + 32.0f * zoom;
            if (fill_a > 0.0f) {
                glColor4f(fill_r, fill_g, fill_b, fill_a);
                glBegin(GL_QUADS);
                glVertex2f(left, top);
                glVertex2f(right, top);
                glVertex2f(right, bottom);
                glVertex2f(left, bottom);
                glEnd();
            }
            glLineWidth(line_width);
            glColor4f(line_r, line_g, line_b, line_a);
            glBegin(GL_LINE_LOOP);
            glVertex2f(left, top);
            glVertex2f(right, top);
            glVertex2f(right, bottom);
            glVertex2f(left, bottom);
            glEnd();
        };
        if (!m_layer_selected_cells.empty()) {
            for (const auto& cell : m_layer_selected_cells) {
                draw_selected_cell(
                    cell.first,
                    cell.second,
                    1.0f,
                    0.86f,
                    0.0f,
                    dim_selection ? 0.08f : (m_layer_dragging_selection_move ? 0.08f : 0.20f),
                    1.0f,
                    0.82f,
                    0.0f,
                    dim_selection ? 0.42f : (m_layer_dragging_selection_move ? 0.34f : 0.9f),
                    1.5f);
            }
            if (m_layer_dragging_selection_move) {
                for (const auto& cell : m_layer_selected_cells) {
                    draw_selected_cell(
                        cell.first + m_layer_selection_move_delta_x,
                        cell.second + m_layer_selection_move_delta_y,
                        1.0f,
                        0.95f,
                        0.0f,
                        0.24f,
                        1.0f,
                        0.98f,
                        0.22f,
                        0.98f,
                        2.0f);
                }
                for (const auto& source : m_layer_selection_move_values) {
                    int x = source.first.first;
                    int y = source.first.second;
                    int replacement_source_x = x - m_layer_selection_move_delta_x;
                    int replacement_source_y = y - m_layer_selection_move_delta_y;
                    if (IsLayerCellSelected(replacement_source_x, replacement_source_y)) {
                        continue;
                    }

                    PickPoint origin = block_screen_origin(x, y);
                    float left = origin.x;
                    float top = origin.y;
                    float right = left + 32.0f * zoom;
                    float bottom = top + 32.0f * zoom;
                    glColor4f(0.04f, 0.04f, 0.04f, 0.38f);
                    glBegin(GL_QUADS);
                    glVertex2f(left, top);
                    glVertex2f(right, top);
                    glVertex2f(right, bottom);
                    glVertex2f(left, bottom);
                    glEnd();
                    glLineWidth(2.0f);
                    glColor4f(1.0f, 1.0f, 1.0f, 0.62f);
                    glBegin(GL_LINES);
                    glVertex2f(left + 5.0f * zoom, top + 5.0f * zoom);
                    glVertex2f(right - 5.0f * zoom, bottom - 5.0f * zoom);
                    glVertex2f(right - 5.0f * zoom, top + 5.0f * zoom);
                    glVertex2f(left + 5.0f * zoom, bottom - 5.0f * zoom);
                    glEnd();
                }
            }
            int primary_x = m_layer_selection_anchor_x;
            int primary_y = m_layer_selection_anchor_y;
            if (m_layer_dragging_selection_move) {
                primary_x += m_layer_selection_move_delta_x;
                primary_y += m_layer_selection_move_delta_y;
            }
            draw_selected_cell(
                primary_x,
                primary_y,
                1.0f,
                0.62f,
                0.0f,
                dim_selection ? 0.10f : 0.24f,
                1.0f,
                1.0f,
                1.0f,
                dim_selection ? 0.55f : 0.98f,
                2.5f);
        } else {
            draw_selected_cell(
                m_background_selected_x,
                m_background_selected_y,
                1.0f,
                0.78f,
                0.0f,
                dim_selection ? 0.10f : 0.24f,
                1.0f,
                1.0f,
                1.0f,
                dim_selection ? 0.55f : 0.98f,
                2.0f);
        }
        glLineWidth(1.0f);
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

    if (m_drawing_tool == DrawingTool::Draw && m_background_clipboard_valid && !m_layer_dragging_draw) {
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
    if (!map) {
        return;
    }

    std::vector<std::pair<int, int>> preview_cells;
    std::map<std::pair<int, int>, uint16_t> preview_values;
    if (m_heightmap_dragging_selection_move) {
        for (const auto& source : m_heightmap_selection_move_values) {
            std::pair<int, int> target{
                source.first.first + m_heightmap_selection_move_delta_x,
                source.first.second + m_heightmap_selection_move_delta_y
            };
            preview_cells.push_back(target);
            preview_values[target] = source.second;
        }
    } else if (!m_heightmap_dragging_draw && IsHeightmapPreviewTool()) {
        if (m_heightmap_dragging_line) {
            preview_cells = m_heightmap_line_preview_cells;
        } else if (m_drawing_tool == DrawingTool::FloodFill && m_heightmap_clipboard_valid) {
            preview_cells = BuildHeightmapFloodFillCells(
                m_heightmapRenderer.GetHoverX(),
                m_heightmapRenderer.GetHoverY());
        } else if (m_drawing_tool == DrawingTool::Stamp) {
            for (const auto& cell : BuildHeightmapStampCells(
                     m_heightmapRenderer.GetHoverX(),
                     m_heightmapRenderer.GetHoverY())) {
                preview_cells.push_back(cell.first);
                preview_values[cell.first] = cell.second;
            }
        } else {
            int hover_x = m_heightmapRenderer.GetHoverX();
            int hover_y = m_heightmapRenderer.GetHoverY();
            if (m_heightmap_clipboard_valid &&
                hover_x >= 0 && hover_y >= 0 &&
                hover_x < map->GetHeightmapWidth() && hover_y < map->GetHeightmapHeight()) {
                preview_cells.push_back({hover_x, hover_y});
            }
        }
    }

    bool has_selection = m_background_has_selection && !m_heightmap_selected_cells.empty();
    int primary_x = has_selection ? PrimaryHeightmapCellX() : -1;
    int primary_y = has_selection ? PrimaryHeightmapCellY() : -1;
    has_selection = has_selection && primary_x >= 0 && primary_y >= 0;

    if (!has_selection && preview_cells.empty()) {
        return;
    }

    float room_left = static_cast<float>(m_mapRenderer.GetRoomLeft());
    float room_top = static_cast<float>(m_mapRenderer.GetRoomTop());

    float zoom = std::max(ZoomFactor(), 0.0001f);

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

    auto draw_cell_diamond = [&](int x, int y, float fill_alpha, float line_width, int z_override = -1) {
        if (x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
            return;
        }
        uint8_t z = z_override >= 0 ? static_cast<uint8_t>(z_override) : map->GetHeight({x, y});
        PickPoint center = ProjectHeightmapGridPoint(
            static_cast<float>(x) + 0.5f,
            static_cast<float>(y) + 0.5f,
            z,
            room_left,
            room_top,
            m_heightmapRenderer.GetZExtent());
        float cx = center.x * zoom + m_cam_x;
        float cy = center.y * zoom + m_cam_y;
        if (fill_alpha > 0.0f) {
            glBegin(GL_QUADS);
            glVertex2f(cx, cy - 16.0f * zoom);
            glVertex2f(cx + 32.0f * zoom, cy);
            glVertex2f(cx, cy + 16.0f * zoom);
            glVertex2f(cx - 32.0f * zoom, cy);
            glEnd();
        }
        glLineWidth(line_width);
        glBegin(GL_LINE_LOOP);
        glVertex2f(cx, cy - 16.0f * zoom);
        glVertex2f(cx + 32.0f * zoom, cy);
        glVertex2f(cx, cy + 16.0f * zoom);
        glVertex2f(cx - 32.0f * zoom, cy);
        glEnd();
    };

    auto preview_cell_z = [&](const std::pair<int, int>& cell) {
        auto it = preview_values.find(cell);
        if (it != preview_values.end()) {
            return static_cast<int>(HeightmapCellHeight(it->second));
        }
        return static_cast<int>(HeightmapCellHeight(m_heightmap_clipboard_cell));
    };

    if (!preview_cells.empty()) {
        glColor4f(1.0f, 1.0f, 1.0f, 0.18f);
        for (const auto& preview_cell : preview_cells) {
            draw_cell_diamond(preview_cell.first, preview_cell.second, 0.18f, 2.5f, preview_cell_z(preview_cell));
        }
        glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
        for (const auto& preview_cell : preview_cells) {
            draw_cell_diamond(preview_cell.first, preview_cell.second, 0.0f, 2.5f, preview_cell_z(preview_cell));
        }
    }

    if (m_heightmap_dragging_selection_move) {
        for (const auto& source : m_heightmap_selection_move_values) {
            int x = source.first.first;
            int y = source.first.second;
            int replacement_source_x = x - m_heightmap_selection_move_delta_x;
            int replacement_source_y = y - m_heightmap_selection_move_delta_y;
            if (IsHeightmapCellSelected(replacement_source_x, replacement_source_y)) {
                continue;
            }

            int z = static_cast<int>(HeightmapCellHeight(source.second));
            PickPoint center = ProjectHeightmapGridPoint(
                static_cast<float>(x) + 0.5f,
                static_cast<float>(y) + 0.5f,
                static_cast<float>(z),
                room_left,
                room_top,
                m_heightmapRenderer.GetZExtent());
            float cx = center.x * zoom + m_cam_x;
            float cy = center.y * zoom + m_cam_y;
            glColor4f(0.04f, 0.04f, 0.04f, 0.38f);
            draw_cell_diamond(x, y, 0.38f, 2.0f, z);
            glLineWidth(2.0f);
            glColor4f(1.0f, 1.0f, 1.0f, 0.62f);
            glBegin(GL_LINES);
            glVertex2f(cx - 14.0f * zoom, cy - 7.0f * zoom);
            glVertex2f(cx + 14.0f * zoom, cy + 7.0f * zoom);
            glVertex2f(cx + 14.0f * zoom, cy - 7.0f * zoom);
            glVertex2f(cx - 14.0f * zoom, cy + 7.0f * zoom);
            glEnd();
        }
    }

    if (!has_selection) {
        glLineWidth(1.0f);
        return;
    }

    const bool inactive_selection = IsHeightmapBrushTool();
    for (const auto& selected_cell : m_heightmap_selected_cells) {
        int x = selected_cell.first;
        int y = selected_cell.second;
        bool primary = x == primary_x && y == primary_y;
        if (inactive_selection) {
            glColor4f(primary ? 0.55f : 0.42f, primary ? 0.46f : 0.42f, primary ? 0.12f : 0.42f, primary ? 0.72f : 0.58f);
        } else {
            glColor4f(primary ? 1.0f : 1.0f, primary ? 0.82f : 1.0f, primary ? 0.1f : 1.0f, primary ? 1.0f : 0.92f);
        }
        draw_cell_diamond(x, y, 0.0f, 2.0f);
    }

    glLineWidth(1.0f);
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
    ApplyEntityDragStep(
        inst,
        evt.GetPosition(),
        z_axis_only,
        m_drag_start_mouse,
        m_drag_start_x,
        m_drag_start_y,
        m_drag_start_z,
        m_drag_plane_z,
        m_drag_cursor_offset_x,
        m_drag_cursor_offset_y,
        m_drag_floor_snap);
    UpdateEntityProjection(inst);
    SortEntitiesGeometrically(m_instances);
    entity_idx = FindInstanceIndex(m_drag_instance_id);
    m_selected_entity_idx = entity_idx;
    m_hovered_entity_idx = entity_idx;
    Refresh();
}

void MyGLCanvas::ApplyEntityDragStep(
    SpriteInstance& inst,
    const wxPoint& mouse_pos,
    bool z_axis_only,
    const wxPoint& drag_start_mouse,
    float drag_start_x,
    float drag_start_y,
    float drag_start_z,
    float drag_plane_z,
    float drag_cursor_offset_x,
    float drag_cursor_offset_y,
    bool drag_floor_snap) const {
    auto snap_half = [](float value) {
        return std::round(value * 2.0f) * 0.5f;
    };
    auto clamp_map_pos = [](float value) {
        return std::clamp(value, 0.0f, 63.5f);
    };

    if (z_axis_only) {
        float dy = static_cast<float>(mouse_pos.y - drag_start_mouse.y);
        inst.map_x = clamp_map_pos(drag_start_x);
        inst.map_y = clamp_map_pos(drag_start_y);
        inst.map_z = std::clamp(snap_half(drag_start_z - dy / 32.0f), 0.0f, 15.5f);
    } else {
        float world_x = ScreenToWorldX(mouse_pos.x);
        float world_y = ScreenToWorldY(mouse_pos.y);
        PickPoint cursor_map = ScreenToMapPoint(world_x, world_y, drag_plane_z, inst.room_left, inst.room_top, inst.z_extent);
        float hitbox_center_x = cursor_map.x + drag_cursor_offset_x;
        float hitbox_center_y = cursor_map.y + drag_cursor_offset_y;
        inst.map_x = clamp_map_pos(snap_half(hitbox_center_x - inst.hitbox_offset));
        inst.map_y = clamp_map_pos(snap_half(hitbox_center_y - inst.hitbox_offset));
    }

    inst.floor_z = FloorUnderHitbox(
        inst.map_x + inst.hitbox_offset,
        inst.map_y + inst.hitbox_offset,
        inst.hitbox_base * 0.5f);
    if (!z_axis_only && drag_floor_snap) {
        inst.map_z = std::clamp(inst.floor_z, 0.0f, 15.5f);
    } else if (!z_axis_only) {
        inst.map_z = std::clamp(drag_start_z, 0.0f, 15.5f);
    }
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

    CaptureObjectUndoState();
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

    CaptureObjectUndoState();
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

    CaptureObjectUndoState();
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

    CaptureObjectUndoState();
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

bool MyGLCanvas::HasPendingObjectAdd() const {
    return m_pending_add_type != PendingObjectAddType::None;
}

void MyGLCanvas::UpdatePendingObjectAddHover() {
    if (!HasPendingObjectAdd()) {
        return;
    }

    if (m_pending_add_type == PendingObjectAddType::TileSwap &&
        (m_pending_tileswap_part == PendingTileSwapPart::MapSource ||
         m_pending_tileswap_part == PendingTileSwapPart::MapDestination)) {
        PickPoint point = ScreenToMapPoint(
            ScreenToWorldX(m_last_mouse_pos.x),
            ScreenToWorldY(m_last_mouse_pos.y),
            0.0f,
            static_cast<float>(m_mapRenderer.GetRoomLeft()),
            static_cast<float>(m_mapRenderer.GetRoomTop()));
        m_pending_add_hover_x = std::clamp(static_cast<int>(std::floor(point.x)), 0, 63);
        m_pending_add_hover_y = std::clamp(static_cast<int>(std::floor(point.y)), 0, 63);
        return;
    }

    PickPoint point = ScreenToMapPoint(
        ScreenToWorldX(m_last_mouse_pos.x),
        ScreenToWorldY(m_last_mouse_pos.y),
        m_pending_add_plane_z,
        static_cast<float>(m_mapRenderer.GetRoomLeft()),
        static_cast<float>(m_mapRenderer.GetRoomTop()),
        m_heightmapRenderer.GetZExtent());
    m_pending_add_hover_x = std::clamp(static_cast<int>(std::floor(point.x)), 0, 63);
    m_pending_add_hover_y = std::clamp(static_cast<int>(std::floor(point.y)), 0, 63);
}

bool MyGLCanvas::BuildPendingEntityPreviewInstance(SpriteInstance& inst) {
    if (m_pending_add_type != PendingObjectAddType::Entity) {
        return false;
    }
    if (m_pending_add_hover_x < 0 || m_pending_add_hover_y < 0) {
        return false;
    }

    inst = SpriteInstance{};
    inst.instance_id = static_cast<uint32_t>(m_room_entities.size() + 1);
    inst.entity_id = m_pending_add_entity_id;
    inst.palette = std::min<uint8_t>(m_pending_add_entity_palette, 3);
    inst.z_extent = m_heightmapRenderer.GetZExtent();
    inst.room_left = static_cast<float>(m_mapRenderer.GetRoomLeft());
    inst.room_top = static_cast<float>(m_mapRenderer.GetRoomTop());
    inst.dx = 0.0f;
    inst.dy = 0.0f;
    inst.scale = 2.0f;
    inst.anim_timer = 0.0f;
    inst.anim_speed = 1.0f;
    inst.orientation = m_pending_add_entity_orientation;
    RefreshEntityMetadata(inst);

    // Place ghost exactly on the hovered cell center — same coordinate source as the cell highlight.
    float cx = static_cast<float>(m_pending_add_hover_x) + 0.5f;
    float cy = static_cast<float>(m_pending_add_hover_y) + 0.5f;
    inst.map_x = std::clamp(cx - inst.hitbox_offset, 0.0f, 63.5f);
    inst.map_y = std::clamp(cy - inst.hitbox_offset, 0.0f, 63.5f);
    inst.floor_z = FloorUnderHitbox(cx, cy, inst.hitbox_base * 0.5f);
    inst.map_z = std::clamp(inst.floor_z, 0.0f, 15.5f);

    UpdateEntityProjection(inst);

    return true;
}

bool MyGLCanvas::BuildPendingWarpPreviewInstance(WarpInstance& inst) {
    if (m_pending_add_type != PendingObjectAddType::Warp) {
        return false;
    }
    if (m_pending_add_hover_x < 0 || m_pending_add_hover_y < 0) {
        return false;
    }
    // Use room-grid coords from UpdatePendingObjectAddHover (ScreenToMapPoint) so that
    // ProjectWarpGridPoint renders the ghost centred on the cursor.
    float half_w = std::round(m_pending_add_warp_width) * 0.5f;
    float half_h = std::round(m_pending_add_warp_height) * 0.5f;
    float cursor_x = static_cast<float>(m_pending_add_hover_x) + 0.5f;
    float cursor_y = static_cast<float>(m_pending_add_hover_y) + 0.5f;
    auto [x, y] = FindNearestFreeWarpCell(cursor_x - half_w, cursor_y - half_h);
    Landstalker::WarpList::Warp warp{};
    warp.room1 = m_current_room;
    warp.x1 = static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(x)), 0, 63));
    warp.y1 = static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(y)), 0, 63));
    warp.room2 = 0xFFFF;
    warp.x_size = static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(m_pending_add_warp_width)), 1, 3));
    warp.y_size = static_cast<uint8_t>(std::clamp(static_cast<int>(std::round(m_pending_add_warp_height)), 1, 3));
    warp.type = m_pending_add_warp_type;
    inst = GLCanvasObjectSupport::MakeWarpInstance(
        warp, m_current_room, 0,
        float(m_mapRenderer.GetRoomLeft()), float(m_mapRenderer.GetRoomTop()),
        m_heightmapRenderer.GetZExtent());
    UpdateWarpFloor(inst);
    return true;
}

void MyGLCanvas::CancelPendingObjectAdd() {
    m_pending_add_type = PendingObjectAddType::None;
    m_pending_tileswap_part = PendingTileSwapPart::MapSource;
    m_pending_add_hover_x = -1;
    m_pending_add_hover_y = -1;
    m_pending_add_plane_z = 0.0f;
    m_pending_add_floor_snap = true;
    m_pending_add_entity_cursor_offset_x = 0.0f;
    m_pending_add_entity_cursor_offset_y = 0.0f;
    m_pending_add_swap = TileSwap{};
    m_pending_add_swap_index = -1;
    m_pending_add_start_x = 0.0f;
    m_pending_add_start_y = 0.0f;
    m_pending_add_start_z = 0.0f;
    m_pending_add_mouse_start = wxPoint(-1, -1);
    m_pending_add_warp_width = 1.0f;
    m_pending_add_warp_height = 1.0f;
    m_pending_add_warp_type = Landstalker::WarpList::Warp::Type::NORMAL;
    m_pending_add_door_size = Door::Size::DOOR_1X4;
    SetCursor(wxCursor(wxCURSOR_ARROW));
    Refresh();
}

void MyGLCanvas::CommitPendingObjectAdd() {
    if (!HasPendingObjectAdd()) {
        return;
    }

    if (m_pending_add_type == PendingObjectAddType::Entity) {
        LogPendingEntityPlacementSnapshot(
            "commit-start",
            m_last_mouse_pos,
            m_pending_add_hover_x,
            m_pending_add_hover_y,
            m_pending_add_plane_z,
            m_pending_add_entity_cursor_offset_x,
            m_pending_add_entity_cursor_offset_y,
            nullptr);
    }

    UpdatePendingObjectAddHover();
    switch (m_pending_add_type) {
        case PendingObjectAddType::Entity:
            if (m_room_entities.size() < 15) {
                SpriteInstance ghost{};
                if (!BuildPendingEntityPreviewInstance(ghost)) {
                    LogPendingEntityPlacementSnapshot(
                        "commit-build-preview-failed",
                        m_last_mouse_pos,
                        m_pending_add_hover_x,
                        m_pending_add_hover_y,
                        m_pending_add_plane_z,
                        m_pending_add_entity_cursor_offset_x,
                        m_pending_add_entity_cursor_offset_y,
                        nullptr);
                    return;
                }
                LogPendingEntityPlacementSnapshot(
                    "commit-preview",
                    m_last_mouse_pos,
                    m_pending_add_hover_x,
                    m_pending_add_hover_y,
                    m_pending_add_plane_z,
                    m_pending_add_entity_cursor_offset_x,
                    m_pending_add_entity_cursor_offset_y,
                    &ghost);
                CaptureObjectUndoState();
                GLCanvasEntityEditor(*this).AddEntity(ghost);
                NotifyRoomDataChanged(true, false, false, false);
                NotifySelectionChanged();
            }
            CancelPendingObjectAdd();
            return;
        case PendingObjectAddType::Warp:
            CaptureObjectUndoState();
            GLCanvasWarpEditor(*this).AddWarpHalf();
            NotifyRoomDataChanged(false, true, false, false);
            NotifySelectionChanged();
            CancelPendingObjectAdd();
            SetFocus();
            return;
        case PendingObjectAddType::Door:
            CaptureObjectUndoState();
            GLCanvasTileDoorEditor(*this).AddDoor();
            NotifyRoomDataChanged(false, false, false, true);
            NotifySelectionChanged();
            CancelPendingObjectAdd();
            return;
        case PendingObjectAddType::TileSwap:
            break;
        case PendingObjectAddType::None:
            return;
    }

    uint8_t x = static_cast<uint8_t>(std::clamp(m_pending_add_hover_x, 0, 63));
    uint8_t y = static_cast<uint8_t>(std::clamp(m_pending_add_hover_y, 0, 63));
    switch (m_pending_tileswap_part) {
        case PendingTileSwapPart::MapSource:
            m_pending_add_swap.map.src_x = x;
            m_pending_add_swap.map.src_y = y;
            m_pending_tileswap_part = PendingTileSwapPart::MapDestination;
            Refresh();
            return;
        case PendingTileSwapPart::MapDestination:
            m_pending_add_swap.map.dst_x = x;
            m_pending_add_swap.map.dst_y = y;
            m_pending_tileswap_part = PendingTileSwapPart::HeightmapSource;
            Refresh();
            return;
        case PendingTileSwapPart::HeightmapSource:
            m_pending_add_swap.heightmap.src_x = x;
            m_pending_add_swap.heightmap.src_y = y;
            m_pending_tileswap_part = PendingTileSwapPart::HeightmapDestination;
            Refresh();
            return;
        case PendingTileSwapPart::HeightmapDestination:
            m_pending_add_swap.heightmap.dst_x = x;
            m_pending_add_swap.heightmap.dst_y = y;
            break;
    }

    auto rd = m_gd ? m_gd->GetRoomData() : nullptr;
    if (!rd) {
        CancelPendingObjectAdd();
        return;
    }
    auto swaps = rd->GetTileSwaps(m_current_room);
    CaptureObjectUndoState();
    swaps.push_back(m_pending_add_swap);
    rd->SetTileSwaps(m_current_room, swaps);
    m_selected_entity_idx = -1;
    m_selected_warp_idx = -1;
    m_selected_door_idx = -1;
    m_selected_tileswap_region_idx = static_cast<int>((swaps.size() - 1) * 4);
    m_hovered_tileswap_region_idx = m_selected_tileswap_region_idx;
    NotifyRoomDataChanged(false, false, true, false);
    NotifySelectionChanged();
    CancelPendingObjectAdd();
}

void MyGLCanvas::RenderPendingObjectAddOverlay() {
    if (!HasPendingObjectAdd() || m_pending_add_hover_x < 0 || m_pending_add_hover_y < 0) {
        return;
    }

    float zoom = std::max(ZoomFactor(), 0.0001f);
    float room_left = static_cast<float>(m_mapRenderer.GetRoomLeft());
    float room_top = static_cast<float>(m_mapRenderer.GetRoomTop());
    auto height_at = [&](int x, int y) {
        auto map = CurrentRoomMap();
        if (!map || x < 0 || y < 0 || x >= map->GetHeightmapWidth() || y >= map->GetHeightmapHeight()) {
            return 0.0f;
        }
        uint8_t height = map->GetHeight({x, y});
        return height == 0xFF ? 0.0f : static_cast<float>(height);
    };
    auto draw_diamond = [&](int x, int y, bool heightmap, float r, float g, float b, float a) {
        PickPoint center = heightmap
            ? ProjectHeightmapGridPoint(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, height_at(x, y), room_left, room_top, m_heightmapRenderer.GetZExtent())
            : ProjectRoomGridPoint(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, 0.0f, room_left, room_top);
        float cx = center.x * zoom + m_cam_x;
        float cy = center.y * zoom + m_cam_y;
        glColor4f(r, g, b, a * 0.32f);
        glBegin(GL_QUADS);
        glVertex2f(cx, cy - 16.0f * zoom);
        glVertex2f(cx + 32.0f * zoom, cy);
        glVertex2f(cx, cy + 16.0f * zoom);
        glVertex2f(cx - 32.0f * zoom, cy);
        glEnd();
        glColor4f(r, g, b, a);
        glLineWidth(2.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(cx, cy - 16.0f * zoom);
        glVertex2f(cx + 32.0f * zoom, cy);
        glVertex2f(cx, cy + 16.0f * zoom);
        glVertex2f(cx - 32.0f * zoom, cy);
        glEnd();
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
    int width = 0;
    int height = 0;
    GetClientSize(&width, &height);
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (m_pending_add_type == PendingObjectAddType::TileSwap) {
        if (m_pending_tileswap_part != PendingTileSwapPart::MapSource) {
            draw_diamond(m_pending_add_swap.map.src_x, m_pending_add_swap.map.src_y, false, 0.25f, 0.75f, 1.0f, 0.95f);
        }
        if (m_pending_tileswap_part == PendingTileSwapPart::HeightmapSource ||
            m_pending_tileswap_part == PendingTileSwapPart::HeightmapDestination) {
            draw_diamond(m_pending_add_swap.map.dst_x, m_pending_add_swap.map.dst_y, false, 0.1f, 1.0f, 0.45f, 0.95f);
        }
        if (m_pending_tileswap_part == PendingTileSwapPart::HeightmapDestination) {
            draw_diamond(m_pending_add_swap.heightmap.src_x, m_pending_add_swap.heightmap.src_y, true, 1.0f, 0.75f, 0.2f, 0.95f);
        }
        bool current_is_heightmap = m_pending_tileswap_part == PendingTileSwapPart::HeightmapSource ||
            m_pending_tileswap_part == PendingTileSwapPart::HeightmapDestination;
        draw_diamond(m_pending_add_hover_x, m_pending_add_hover_y, current_is_heightmap, 1.0f, 1.0f, 1.0f, 0.95f);
    } else if (m_pending_add_type == PendingObjectAddType::Entity) {
        SpriteInstance ghost{};
        if (BuildPendingEntityPreviewInstance(ghost)) {
            float center_x = ghost.map_x + ghost.hitbox_offset;
            float center_y = ghost.map_y + ghost.hitbox_offset;
            PickPoint center = ProjectRoomGridPoint(
                center_x,
                center_y,
                ghost.map_z,
                room_left,
                room_top,
                ghost.z_extent);
            float cx = center.x * zoom + m_cam_x;
            float cy = center.y * zoom + m_cam_y;
            glColor4f(1.0f, 1.0f, 1.0f, 0.30f);
            glBegin(GL_QUADS);
            glVertex2f(cx, cy - 16.0f * zoom);
            glVertex2f(cx + 32.0f * zoom, cy);
            glVertex2f(cx, cy + 16.0f * zoom);
            glVertex2f(cx - 32.0f * zoom, cy);
            glEnd();
            glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
            glLineWidth(2.5f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(cx, cy - 16.0f * zoom);
            glVertex2f(cx + 32.0f * zoom, cy);
            glVertex2f(cx, cy + 16.0f * zoom);
            glVertex2f(cx - 32.0f * zoom, cy);
            glEnd();
            GLCanvasEntityEditor(*this).RenderEntityTooltipForInstance(ghost);
        }
    }
    glLineWidth(1.0f);
}


// Thin forwarding layer: input handlers stay on MyGLCanvas while edit rules
// live in dedicated editor/coordinator classes.
void MyGLCanvas::AddEntity() {
    if (m_room_entities.size() >= 15) {
        return;
    }
    SetFocus();
    m_pending_add_type = PendingObjectAddType::Entity;

    // Initialize plane and cursor offset like StartEntityDrag.
    float room_left = static_cast<float>(m_mapRenderer.GetRoomLeft());
    float room_top = static_cast<float>(m_mapRenderer.GetRoomTop());
    float z_extent = m_heightmapRenderer.GetZExtent();

    m_pending_add_floor_snap = true;

    float seed_center_x = 0.5f;
    float seed_center_y = 0.5f;

    // Compute a coarse room position first so the floor height comes from the same cell.
    if (m_last_mouse_pos.x >= 0 && m_last_mouse_pos.y >= 0) {
        float world_x = ScreenToWorldX(m_last_mouse_pos.x);
        float world_y = ScreenToWorldY(m_last_mouse_pos.y);
        PickPoint coarse = ScreenToMapPoint(world_x, world_y, 0.0f, room_left, room_top, z_extent);
        seed_center_x = std::clamp(coarse.x, 0.0f, 63.5f);
        seed_center_y = std::clamp(coarse.y, 0.0f, 63.5f);
    }

    m_pending_add_plane_z = std::clamp(FloorUnderPoint(seed_center_x, seed_center_y), 0.0f, 15.5f);
    m_pending_add_entity_cursor_offset_x = 0.0f;
    m_pending_add_entity_cursor_offset_y = 0.0f;

    UpdatePendingObjectAddHover();

    if (m_pending_add_hover_x >= 0 && m_pending_add_hover_y >= 0) {
        seed_center_x = static_cast<float>(m_pending_add_hover_x) + 0.5f;
        seed_center_y = static_cast<float>(m_pending_add_hover_y) + 0.5f;
    } else if (m_last_mouse_pos.x >= 0 && m_last_mouse_pos.y >= 0) {
        float world_x = ScreenToWorldX(m_last_mouse_pos.x);
        float world_y = ScreenToWorldY(m_last_mouse_pos.y);
        PickPoint precise = ScreenToMapPoint(world_x, world_y, m_pending_add_plane_z, room_left, room_top, z_extent);
        seed_center_x = std::clamp(precise.x, 0.0f, 63.5f);
        seed_center_y = std::clamp(precise.y, 0.0f, 63.5f);
    }

    // Initialize the starting position to the hover cell center.
    m_pending_add_start_x = seed_center_x;
    m_pending_add_start_y = seed_center_y;
    m_pending_add_start_z = m_pending_add_plane_z;
    m_pending_add_mouse_start = m_last_mouse_pos;

    SetCursor(wxCursor(wxCURSOR_CROSS));
    Refresh();
}

void MyGLCanvas::CopySelectedEntity() {
    GLCanvasEntityEditor(*this).CopySelectedEntity();
}

void MyGLCanvas::PasteEntity() {
    if (!m_entity_clipboard_valid || m_room_entities.size() >= 15) {
        return;
    }
    CaptureObjectUndoState();
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
    if (!had_entity && !had_warp && !had_swap && !had_door) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasObjectCoordinator(*this).DeleteSelectedObject();
    NotifyRoomDataChanged(had_entity, had_warp, had_swap, had_door);
    NotifySelectionChanged();
}

void MyGLCanvas::ReorderSelectedObject(int delta) {
    const bool had_entity = m_selected_entity_idx >= 0;
    const bool had_warp = m_selected_warp_idx >= 0;
    const bool had_swap = m_selected_tileswap_region_idx >= 0;
    const bool had_door = m_selected_door_idx >= 0;
    if (!had_entity && !had_warp && !had_swap && !had_door) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasObjectCoordinator(*this).ReorderSelectedObject(delta);
    NotifyRoomDataChanged(had_entity, had_warp, had_swap, had_door);
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
    if (m_pending_add_type == PendingObjectAddType::Entity) {
        m_pending_add_entity_id = static_cast<uint8_t>((int(m_pending_add_entity_id) + delta + 256) & 0xFF);
        Refresh();
        return;
    }
    if (m_selected_entity_idx < 0 || m_selected_entity_idx >= static_cast<int>(m_instances.size())) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasEntityEditor(*this).CycleSelectedEntityId(delta);
    NotifyRoomDataChanged(true, false, false, false);
}

void MyGLCanvas::CycleSelectedEntityPalette() {
    if (m_pending_add_type == PendingObjectAddType::Entity) {
        m_pending_add_entity_palette = static_cast<uint8_t>((m_pending_add_entity_palette + 1) % 4);
        Refresh();
        return;
    }
    if (m_selected_entity_idx < 0 || m_selected_entity_idx >= static_cast<int>(m_instances.size())) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasEntityEditor(*this).CycleSelectedEntityPalette();
    NotifyRoomDataChanged(true, false, false, false);
}

void MyGLCanvas::SetSelectedEntityOrientation(Landstalker::Orientation orientation) {
    if (m_pending_add_type == PendingObjectAddType::Entity) {
        m_pending_add_entity_orientation = orientation;
        Refresh();
        return;
    }
    if (m_selected_entity_idx < 0 || m_selected_entity_idx >= static_cast<int>(m_instances.size())) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasEntityEditor(*this).SetSelectedEntityOrientation(orientation);
    NotifyRoomDataChanged(true, false, false, false);
}

void MyGLCanvas::SetSelectedEntityToFloor() {
    if (m_selected_entity_idx < 0 || m_selected_entity_idx >= static_cast<int>(m_instances.size())) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasEntityEditor(*this).SetSelectedEntityToFloor();
    NotifyRoomDataChanged(true, false, false, false);
}

void MyGLCanvas::AddWarpHalf() {
    SetFocus();
    m_pending_add_type = PendingObjectAddType::Warp;
    m_pending_add_warp_width = 1.0f;
    m_pending_add_warp_height = 1.0f;
    m_pending_add_warp_type = Landstalker::WarpList::Warp::Type::NORMAL;
    UpdatePendingObjectAddHover();
    SetCursor(wxCursor(wxCURSOR_CROSS));
    Refresh();
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

    int picked_x = -1;
    int picked_y = -1;
    if (const_cast<MyGLCanvas*>(this)->HeightmapCellAt(m_last_mouse_pos, picked_x, picked_y)) {
        return {
            std::clamp(m_mapRenderer.GetRoomLeft() + picked_x, 0, 63),
            std::clamp(m_mapRenderer.GetRoomTop() + picked_y, 0, 63)
        };
    }

    PickPoint point = ScreenToHeightmapPoint(
        ScreenToWorldX(m_last_mouse_pos.x),
        ScreenToWorldY(m_last_mouse_pos.y),
        static_cast<float>(m_mapRenderer.GetRoomLeft()),
        static_cast<float>(m_mapRenderer.GetRoomTop()));
    return {
        std::clamp(static_cast<int>(std::floor(point.x)), 0, 63),
        std::clamp(static_cast<int>(std::floor(point.y)), 0, 63)
    };
}

void MyGLCanvas::ResizeSelectedWarp(float dx, float dy) {
    if (m_pending_add_type == PendingObjectAddType::Warp) {
        if (dx != 0.0f) {
            m_pending_add_warp_height = GLCanvasObjectSupport::ValidWarpHeight(m_pending_add_warp_height, m_pending_add_warp_width);
            m_pending_add_warp_width = GLCanvasObjectSupport::ValidWarpWidth(m_pending_add_warp_width + dx, m_pending_add_warp_height);
        }
        if (dy != 0.0f) {
            m_pending_add_warp_width = GLCanvasObjectSupport::ValidWarpWidth(m_pending_add_warp_width, m_pending_add_warp_height);
            m_pending_add_warp_height = GLCanvasObjectSupport::ValidWarpHeight(m_pending_add_warp_height + dy, m_pending_add_warp_width);
        }
        Refresh();
        return;
    }
    if (m_selected_warp_idx < 0 || m_selected_warp_idx >= static_cast<int>(m_warps.size())) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasWarpEditor(*this).ResizeSelectedWarp(dx, dy);
    NotifyRoomDataChanged(false, true, false, false);
}

void MyGLCanvas::RotateSelectedWarp(float dx, float dy) {
    if (m_pending_add_type == PendingObjectAddType::Warp) {
        return; // position is cursor-controlled
    }
    if (m_selected_warp_idx < 0 || m_selected_warp_idx >= static_cast<int>(m_warps.size())) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasWarpEditor(*this).RotateSelectedWarp(dx, dy);
    NotifyRoomDataChanged(false, true, false, false);
}

void MyGLCanvas::CycleSelectedWarpType(int delta) {
    if (m_pending_add_type == PendingObjectAddType::Warp) {
        int type = static_cast<int>(m_pending_add_warp_type);
        type = (type + delta + 3) % 3;
        m_pending_add_warp_type = static_cast<Landstalker::WarpList::Warp::Type>(type);
        Refresh();
        return;
    }
    if (m_selected_warp_idx < 0 || m_selected_warp_idx >= static_cast<int>(m_warps.size())) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasWarpEditor(*this).CycleSelectedWarpType(delta);
    NotifyRoomDataChanged(false, true, false, false);
}

void MyGLCanvas::CycleSelectedDoorSize(int delta) {
    if (m_pending_add_type == PendingObjectAddType::Door) {
        static constexpr std::array<Door::Size, 4> sizes{
            Door::Size::DOOR_1X4,
            Door::Size::DOOR_2X4,
            Door::Size::DOOR_2X5,
            Door::Size::DOOR_1X0
        };
        auto it = std::find(sizes.begin(), sizes.end(), m_pending_add_door_size);
        int idx = it == sizes.end() ? 0 : static_cast<int>(std::distance(sizes.begin(), it));
        idx = (idx + delta + static_cast<int>(sizes.size())) % static_cast<int>(sizes.size());
        m_pending_add_door_size = sizes[static_cast<std::size_t>(idx)];
        Refresh();
        return;
    }
    if (m_selected_door_idx < 0) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasTileDoorEditor(*this).CycleSelectedDoorSize(delta);
    NotifyRoomDataChanged(false, false, false, true);
}

void MyGLCanvas::AddDoor() {
    SetFocus();
    m_pending_add_type = PendingObjectAddType::Door;
    m_pending_add_door_size = Door::Size::DOOR_1X4;
    UpdatePendingObjectAddHover();
    SetCursor(wxCursor(wxCURSOR_CROSS));
    Refresh();
}

void MyGLCanvas::AddTileSwap() {
    auto rd = m_gd ? m_gd->GetRoomData() : nullptr;
    if (!rd) {
        return;
    }
    auto swaps = rd->GetTileSwaps(m_current_room);
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

    m_pending_add_swap = TileSwap{};
    m_pending_add_swap.trigger = static_cast<uint8_t>(trigger);
    m_pending_add_swap.mode = TileSwap::Mode::FLOOR;
    m_pending_add_swap.map = {0, 0, 0, 0, 1, 1};
    m_pending_add_swap.heightmap = {0, 0, 0, 0, 1, 1};
    m_pending_add_type = PendingObjectAddType::TileSwap;
    m_pending_tileswap_part = PendingTileSwapPart::MapSource;
    UpdatePendingObjectAddHover();
    SetCursor(wxCursor(wxCURSOR_CROSS));
    Refresh();
}

void MyGLCanvas::CycleSelectedTileSwapShape(int delta) {
    if (m_selected_tileswap_region_idx < 0) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasTileDoorEditor(*this).CycleSelectedTileSwapShape(delta);
    NotifyRoomDataChanged(false, false, true, false);
}

void MyGLCanvas::CycleSelectedTileSwapId(int delta) {
    if (m_selected_tileswap_region_idx < 0) {
        return;
    }
    CaptureObjectUndoState();
    GLCanvasTileDoorEditor(*this).CycleSelectedTileSwapId(delta);
    NotifyRoomDataChanged(false, false, true, false);
}

void MyGLCanvas::ResizeSelectedTileSwapRegion(float requested_width, float requested_height) {
    if (m_selected_tileswap_region_idx < 0) {
        return;
    }
    CaptureObjectUndoState();
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
    if (!had_entity && !had_warp && !had_swap && !had_door) {
        return;
    }
    CaptureObjectUndoState();
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

    auto visible_cell = [&](int /*x*/, int /*y*/, uint8_t height, uint8_t restriction) {
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
