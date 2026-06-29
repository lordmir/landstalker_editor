#include "GLCanvasObjectSupport.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>

using namespace Landstalker;

namespace GLCanvasObjectSupport {
namespace {
// Shared geometry/projection helpers used by multiple editor/coordinator units.

struct PickRect {
    float min_x;
    float min_y;
    float max_x;
    float max_y;
};

struct TileSwapRegionBounds {
    PickRect bounds;
};

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

float EntityFrontDepthKey(const SpriteInstance& inst)
{
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
    float front_depth;
};

EntityBounds GetEntityBounds(const SpriteInstance& inst)
{
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
        max_x + max_y
    };
}

bool EntityDrawOrder(const SpriteInstance& lhs, const SpriteInstance& rhs)
{
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

bool EntityMustDrawBefore(const SpriteInstance& lhs, const SpriteInstance& rhs)
{
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

}  // namespace

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
    std::stable_sort(instances.begin(), instances.end(), EntityDrawOrder);

    const std::size_t count = instances.size();
    std::vector<std::vector<std::size_t>> after(count);
    std::vector<std::size_t> incoming(count, 0);
    for (std::size_t i = 0; i < count; ++i) {
        for (std::size_t j = i + 1; j < count; ++j) {
            bool i_before_j = EntityMustDrawBefore(instances[i], instances[j]);
            bool j_before_i = EntityMustDrawBefore(instances[j], instances[i]);
            if (i_before_j == j_before_i) {
                continue;
            }
            std::size_t before = i_before_j ? i : j;
            std::size_t later = i_before_j ? j : i;
            after[before].push_back(later);
            ++incoming[later];
        }
    }

    std::deque<std::size_t> ready;
    for (std::size_t i = 0; i < count; ++i) {
        if (incoming[i] == 0) {
            ready.push_back(i);
        }
    }

    std::vector<SpriteInstance> sorted;
    sorted.reserve(count);
    std::vector<bool> emitted(count, false);
    while (!ready.empty()) {
        std::size_t index = ready.front();
        ready.pop_front();
        if (emitted[index]) {
            continue;
        }
        emitted[index] = true;
        sorted.push_back(instances[index]);
        for (std::size_t later : after[index]) {
            if (--incoming[later] == 0) {
                ready.push_back(later);
            }
        }
    }

    for (std::size_t i = 0; i < count; ++i) {
        if (!emitted[i]) {
            sorted.push_back(instances[i]);
        }
    }
    instances = std::move(sorted);
}

}  // namespace GLCanvasObjectSupport
