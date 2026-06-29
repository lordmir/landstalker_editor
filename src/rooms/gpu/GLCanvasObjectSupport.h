#ifndef GL_CANVAS_OBJECT_SUPPORT_H
#define GL_CANVAS_OBJECT_SUPPORT_H

#include <cstdint>
#include <memory>
#include <vector>
#include <landstalker/main/GameData.h>
#include <landstalker/3d_maps/TileSwaps.h>
#include "SpriteInstance.h"
#include "MapRenderer.h"
#include "GLCanvas.h"

namespace GLCanvasObjectSupport {

enum class TileSwapRegionPart {
    TilemapSource = 0,
    TilemapDestination = 1,
    HeightmapSource = 2,
    HeightmapDestination = 3
};

struct PickPoint {
    float x;
    float y;
};

struct TileSwapRegionGeometry {
    int flat_index;
    int swap_index;
    TileSwapRegionPart part;
    Landstalker::TileSwap swap;
    std::vector<PickPoint> points;
    std::vector<PickPoint> fill_points;
    bool segments;
    PickPoint resize_handle;
};

struct TileSwapRegionMetrics {
    int x;
    int y;
    int width;
    int height;
};

std::vector<TileSwapRegionGeometry> BuildTileSwapRegionGeometries(
    const std::shared_ptr<Landstalker::GameData>& gd,
    uint16_t room,
    const MapRenderer& map_renderer,
    float z_extent);

TileSwapRegionMetrics MetricsForTileSwapRegion(const Landstalker::TileSwap& swap, TileSwapRegionPart part);

WarpInstance MakeWarpInstance(
    const Landstalker::WarpList::Warp& warp,
    uint16_t current_room,
    uint32_t instance_id,
    float room_left,
    float room_top,
    float z_extent = 32.0f,
    uint32_t warp_key = 0,
    int side_override = 0);

float ValidWarpWidth(float requested_width, float current_height);
float ValidWarpHeight(float requested_height, float current_width);
void ClampWarpToValidSize(WarpInstance& warp);

void SortEntitiesGeometrically(std::vector<SpriteInstance>& instances);

}  // namespace GLCanvasObjectSupport

#endif  // GL_CANVAS_OBJECT_SUPPORT_H
