#include "GLCanvasObjectSupport.h"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace Landstalker;

namespace GLCanvasObjectSupport {
namespace {
// Shared geometry/projection helpers used by multiple editor/coordinator units.
using RoomProjection::ProjectHeightmapGridPoint;
using RoomProjection::ProjectRoomGridPoint;

// Combined bounds of a region's outline and fill polygons.
PickRect RegionBounds(const std::vector<PickPoint>& points, const std::vector<PickPoint>& fill_points)
{
    PickRect bounds = points.empty() ? PickRect{0.0f, 0.0f, 0.0f, 0.0f} : BoundsForPoints(points);
    if (!fill_points.empty()) {
        PickRect fill_bounds = BoundsForPoints(fill_points);
        bounds.min_x = std::min(bounds.min_x, fill_bounds.min_x);
        bounds.min_y = std::min(bounds.min_y, fill_bounds.min_y);
        bounds.max_x = std::max(bounds.max_x, fill_bounds.max_x);
        bounds.max_y = std::max(bounds.max_y, fill_bounds.max_y);
    }
    return bounds;
}

PickPoint TileSwapResizeHandlePoint(const std::vector<PickPoint>& points, TileSwap::Mode mode)
{
    if (points.empty()) {
        return {0.0f, 0.0f};
    }
    if (mode == TileSwap::Mode::FLOOR) {
        PickPoint best = points.front();
        for (const auto& point : points) {
            if (point.y > best.y || (point.y == best.y && point.x < best.x)) {
                best = point;
            }
        }
        return best;
    }

    PickRect bounds = BoundsForPoints(points);
    PickPoint target = mode == TileSwap::Mode::WALL_NW
        ? PickPoint{bounds.min_x, bounds.max_y}
        : PickPoint{bounds.max_x, bounds.max_y};

    PickPoint best = points.front();
    float best_dist = std::numeric_limits<float>::max();
    for (const auto& point : points) {
        float dx = point.x - target.x;
        float dy = point.y - target.y;
        float dist = dx * dx + dy * dy;
        if (dist < best_dist) {
            best_dist = dist;
            best = point;
        }
    }
    return best;
}

// Room-space footprint of an entity's hitbox (gamelogic3.asm CalcSpriteHitbox:
// HitBoxXStart/XEnd/YStart/YEnd, a square box centred on CentreX/CentreY).
struct EntityFootprint {
    float min_x;
    float max_x;
    float min_y;
    float max_y;
};

EntityFootprint GetEntityFootprint(const SpriteInstance& inst)
{
    float center_x = inst.map_x + inst.hitbox_offset;
    float center_y = inst.map_y + inst.hitbox_offset;
    float half_base = inst.hitbox_base * 0.5f;
    return {center_x - half_base, center_x + half_base, center_y - half_base, center_y + half_base};
}

// Port of gamelogic3.asm _overlapOrderFix's per-pair test. `anchor` is the entity
// currently sitting at the earlier (higher-priority) draw-list slot; returns true
// if `scanned` must be swapped into that slot ahead of it. Mirrors the asm exactly:
// skip (false) if anchor's footprint already starts at/after scanned's on an axis;
// swap (true) if anchor's footprint is entirely before scanned's on an axis (i.e.
// scanned is further SE and so nearer the camera); otherwise the footprints truly
// overlap in both X and Y, and the higher (larger Z) entity draws in front.
bool ScannedDrawsInFrontOfAnchor(const SpriteInstance& anchor, const SpriteInstance& scanned)
{
    EntityFootprint a = GetEntityFootprint(anchor);
    EntityFootprint b = GetEntityFootprint(scanned);
    if (a.min_x >= b.max_x) {
        return false;
    }
    if (a.min_y >= b.max_y) {
        return false;
    }
    if (a.max_x <= b.min_x) {
        return true;
    }
    if (a.max_y <= b.min_y) {
        return true;
    }
    return anchor.map_z < scanned.map_z;
}

}  // namespace

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

std::vector<TileSwapRegionGeometry> BuildTileSwapRegionGeometries(
    const std::shared_ptr<GameData>& gd,
    uint16_t room,
    const MapRenderer& map_renderer,
    float z_extent)
{
    std::vector<TileSwapRegionGeometry> out;
    auto rd = gd ? gd->GetRoomData() : nullptr;
    if (!rd) {
        return out;
    }
    auto swaps = rd->GetTileSwaps(room);
    if (swaps.empty()) {
        return out;
    }

    auto map_entry = rd->GetMapForRoom(room);
    auto tilemap = map_entry ? map_entry->GetData() : nullptr;
    const float room_left = static_cast<float>(map_renderer.GetRoomLeft());
    const float room_top = static_cast<float>(map_renderer.GetRoomTop());

    auto add_region = [&](int swap_index, TileSwapRegionPart part, const TileSwap& swap, std::vector<PickPoint> points, bool segments = false) {
        if (points.size() < 2) {
            return;
        }
        TileSwapRegionGeometry geom{};
        geom.flat_index = static_cast<int>(out.size());
        geom.swap_index = swap_index;
        geom.part = part;
        geom.swap = swap;
        geom.points = std::move(points);
        geom.fill_points = geom.points;
        geom.bounds = RegionBounds(geom.points, geom.fill_points);
        geom.segments = segments;
        geom.resize_handle = TileSwapResizeHandlePoint(geom.points, swap.mode);
        out.push_back(std::move(geom));
    };

    auto tilemap_points = [&](const TileSwap& swap, TileSwap::Region region) {
        std::vector<PickPoint> points;
        auto poly = swap.GetMapRegionPoly(TileSwap::Region::UNDEFINED, 1, 2);
        auto tile_offset = swap.GetTileOffset(region, tilemap, Tilemap3D::Layer::BG);
        PickPoint anchor = ProjectRoomGridPoint(
            room_left + static_cast<float>(tile_offset.first),
            room_top + static_cast<float>(tile_offset.second),
            0.0f,
            room_left,
            room_top);
        points.reserve(poly.size());
        for (const auto& point : poly) {
            points.push_back({
                anchor.x + static_cast<float>(point.first) * 32.0f,
                anchor.y + static_cast<float>(point.second) * 16.0f
            });
        }
        return points;
    };

    auto heightmap_region = [&](int swap_index, TileSwapRegionPart part, const TileSwap& swap, const TileSwap::CopyOp& op, bool source) {
        if (!tilemap) {
            return;
        }
        int x0 = source ? op.src_x : op.dst_x;
        int y0 = source ? op.src_y : op.dst_y;
        int x1 = x0 + op.width;
        int y1 = y0 + op.height;

        auto height_at = [&](int x, int y) {
            if (x < 0 || y < 0 || x >= tilemap->GetHeightmapWidth() || y >= tilemap->GetHeightmapHeight()) {
                return 0.0f;
            }
            uint8_t z = tilemap->GetHeight({x, y});
            return z == 0xFF ? 0.0f : static_cast<float>(z);
        };

        auto center = [&](int x, int y) {
            return ProjectHeightmapGridPoint(
                static_cast<float>(x) + 0.5f,
                static_cast<float>(y) + 0.5f,
                height_at(x, y),
                room_left,
                room_top,
                z_extent);
        };

        auto offset = [](const PickPoint& point, float x, float y) {
            return PickPoint{point.x + x, point.y + y};
        };

        auto in_region = [&](int x, int y) {
            return x >= x0 && x < x1 && y >= y0 && y < y1;
        };

        std::vector<PickPoint> segments;
        std::vector<PickPoint> fill_points{
            offset(center(x0, y0), 0.0f, -16.0f),
            offset(center(x1 - 1, y0), 32.0f, 0.0f),
            offset(center(x1 - 1, y1 - 1), 0.0f, 16.0f),
            offset(center(x0, y1 - 1), -32.0f, 0.0f)
        };

        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                PickPoint c = center(x, y);
                PickPoint top = offset(c, 0.0f, -16.0f);
                PickPoint right = offset(c, 32.0f, 0.0f);
                PickPoint bottom = offset(c, 0.0f, 16.0f);
                PickPoint left = offset(c, -32.0f, 0.0f);

                if (!in_region(x, y - 1)) {
                    segments.push_back(top);
                    segments.push_back(right);
                }
                if (!in_region(x + 1, y)) {
                    segments.push_back(right);
                    segments.push_back(bottom);
                }
                if (!in_region(x, y + 1)) {
                    segments.push_back(bottom);
                    segments.push_back(left);
                }
                if (!in_region(x - 1, y)) {
                    segments.push_back(left);
                    segments.push_back(top);
                }
            }
        }

        if (segments.empty()) {
            return;
        }

        TileSwapRegionGeometry geom{};
        geom.flat_index = static_cast<int>(out.size());
        geom.swap_index = swap_index;
        geom.part = part;
        geom.swap = swap;
        geom.points = std::move(segments);
        geom.fill_points = std::move(fill_points);
        geom.bounds = RegionBounds(geom.points, geom.fill_points);
        geom.segments = true;
        geom.resize_handle = geom.fill_points[2];
        out.push_back(std::move(geom));
    };

    for (std::size_t i = 0; i < swaps.size(); ++i) {
        const TileSwap& swap = swaps[i];
        add_region(static_cast<int>(i), TileSwapRegionPart::TilemapSource, swap, tilemap_points(swap, TileSwap::Region::SOURCE));
        add_region(static_cast<int>(i), TileSwapRegionPart::TilemapDestination, swap, tilemap_points(swap, TileSwap::Region::DESTINATION));
        heightmap_region(static_cast<int>(i), TileSwapRegionPart::HeightmapSource, swap, swap.heightmap, true);
        heightmap_region(static_cast<int>(i), TileSwapRegionPart::HeightmapDestination, swap, swap.heightmap, false);
    }

    return out;
}

std::vector<DoorGeometry> BuildDoorGeometries(
    const std::shared_ptr<GameData>& gd,
    uint16_t room,
    const MapRenderer& map_renderer,
    float z_extent,
    const std::shared_ptr<Tilemap3D>& preview_map)
{
    std::vector<DoorGeometry> out;
    auto rd = gd ? gd->GetRoomData() : nullptr;
    if (!rd) {
        return out;
    }

    auto doors = rd->GetDoors(room);
    auto map_entry = rd->GetMapForRoom(room);
    auto tilemap = preview_map ? preview_map : (map_entry ? map_entry->GetData() : nullptr);
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

        geom.bounds = RegionBounds(geom.cell_points, geom.map_points);
        out.push_back(std::move(geom));
    }

    return out;
}

TileSwapRegionMetrics MetricsForTileSwapRegion(const TileSwap& swap, TileSwapRegionPart part)
{
    switch (part) {
        case TileSwapRegionPart::TilemapSource:
            return {swap.map.src_x, swap.map.src_y, swap.map.width, swap.map.height};
        case TileSwapRegionPart::TilemapDestination:
            return {swap.map.dst_x, swap.map.dst_y, swap.map.width, swap.map.height};
        case TileSwapRegionPart::HeightmapSource:
            return {swap.heightmap.src_x, swap.heightmap.src_y, swap.heightmap.width, swap.heightmap.height};
        case TileSwapRegionPart::HeightmapDestination:
            return {swap.heightmap.dst_x, swap.heightmap.dst_y, swap.heightmap.width, swap.heightmap.height};
    }
    return {0, 0, 1, 1};
}

WarpInstance MakeWarpInstance(
    const WarpList::Warp& warp,
    uint16_t current_room,
    uint32_t instance_id,
    float room_left,
    float room_top,
    float z_extent,
    uint32_t warp_key,
    int side_override)
{
    bool current_room_is_room1 = side_override == 0 ? warp.room1 == current_room : side_override == 1;
    WarpInstance inst{};
    inst.instance_id = instance_id;
    inst.warp_key = warp_key != 0 ? warp_key : instance_id;
    inst.warp = warp;
    inst.current_room_is_room1 = current_room_is_room1;
    inst.x = float(current_room_is_room1 ? warp.x1 : warp.x2);
    inst.y = float(current_room_is_room1 ? warp.y1 : warp.y2);
    inst.width = float(std::max<uint8_t>(warp.x_size, 1));
    inst.height = float(std::max<uint8_t>(warp.y_size, 1));
    inst.z_extent = z_extent;
    inst.room_left = room_left;
    inst.room_top = room_top;
    return inst;
}

float ValidWarpWidth(float requested_width, float current_height)
{
    if (std::round(current_height) > 1.0f) {
        return 1.0f;
    }
    return std::clamp(std::round(requested_width), 1.0f, 3.0f);
}

float ValidWarpHeight(float requested_height, float current_width)
{
    if (std::round(current_width) > 1.0f) {
        return 1.0f;
    }
    return std::clamp(std::round(requested_height), 1.0f, 3.0f);
}

void ClampWarpToValidSize(WarpInstance& warp)
{
    float rounded_width = std::clamp(std::round(warp.width), 1.0f, 3.0f);
    float rounded_height = std::clamp(std::round(warp.height), 1.0f, 3.0f);
    if (rounded_width > 1.0f && rounded_height > 1.0f) {
        if (warp.width >= warp.height) {
            rounded_height = 1.0f;
        } else {
            rounded_width = 1.0f;
        }
    }
    warp.width = rounded_width;
    warp.height = rounded_height;
}

void SortEntitiesGeometrically(std::vector<SpriteInstance>& instances)
{
    // Port of gamelogic3.asm SortSpritesByDepth's live path (_overlapOrderFix): a
    // selection-style bubble pass over the room's entity slot order, identical to
    // the game's draw-list fix-up. For each slot in turn, scan every later slot
    // and swap in whichever entity the pairwise footprint/Z test says belongs
    // there; earlier slots have priority (as in the VDP sprite table) until the
    // final reverse below.
    //
    // The pass isn't a true total order - two entities the comparator can't
    // strictly place (e.g. identical position) are left as-is rather than swapped.
    // In-game this never matters because ProjectAllSprites rebuilds the draw list
    // from the fixed room-slot order fresh every frame before the sort runs, so
    // its input - and therefore its output - never changes frame to frame. This
    // is called every frame too (GLCanvasRoomMode::Render), but directly on the
    // persistent, already-sorted `instances` vector; without re-deriving the same
    // fixed base order first, a tied pair can flip back and forth forever as each
    // frame's sort runs on the previous frame's output instead. Sorting by
    // instance_id first (assigned once, never touched by this function) restores
    // that fixed base order and makes the result idempotent regardless of the
    // order `instances` arrived in.
    std::stable_sort(instances.begin(), instances.end(),
        [](const SpriteInstance& lhs, const SpriteInstance& rhs) { return lhs.instance_id < rhs.instance_id; });

    // Two things the game does are intentionally not reproduced:
    //  - The disassembly's DrawOrder depth key (HitBoxXStart+HitBoxYStart)/2 +
    //    Height + Z + HitBoxZEnd is computed every frame but the insertion sort
    //    that reads it is dead code (unconditionally skipped) in the live game.
    //  - _riderOrderFix (drawing a platform ahead of whatever's standing on it)
    //    depends on runtime "standing on" state that isn't simulated here.
    const std::size_t count = instances.size();
    for (std::size_t anchor = 0; anchor + 1 < count; ++anchor) {
        for (std::size_t scan = anchor + 1; scan < count; ++scan) {
            if (ScannedDrawsInFrontOfAnchor(instances[anchor], instances[scan])) {
                std::swap(instances[anchor], instances[scan]);
            }
        }
    }

    // The pass above builds the list in VDP sprite-table order (slot 0 = highest
    // priority = drawn on top). SpriteRenderer paints `instances` in vector order
    // with no depth test, so the LAST entry ends up on top instead - reverse to
    // match that convention.
    std::reverse(instances.begin(), instances.end());
}

}  // namespace GLCanvasObjectSupport
