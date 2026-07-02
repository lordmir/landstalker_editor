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
#include "RoomProjection.h"

namespace GLCanvasObjectSupport {

// Identifies which source or destination rectangle a tile-swap overlay represents.
enum class TileSwapRegionPart {
    TilemapSource = 0,        // Source map tiles copied by the swap.
    TilemapDestination = 1,   // Destination map tiles overwritten by the swap.
    HeightmapSource = 2,      // Source heightmap cells copied by the swap.
    HeightmapDestination = 3  // Destination heightmap cells overwritten by the swap.
};

// Shared projected point type used for object hit testing and overlay drawing.
using PickPoint = RoomProjection::PickPoint;

// Screen-space polygon and metadata for a selectable tile-swap region.
struct TileSwapRegionGeometry {
    int flat_index;                        // Index in the flattened selectable region list.
    int swap_index;                        // Index of the owning tile swap in room data.
    TileSwapRegionPart part;               // Which map/heightmap source/destination this geometry describes.
    Landstalker::TileSwap swap;            // Tile-swap data used to build and edit the region.
    std::vector<PickPoint> points;         // Outline points or line segments drawn for the region.
    std::vector<PickPoint> fill_points;    // Polygon used for filled previews and containment tests.
    bool segments;                         // True when points should be interpreted as independent line pairs.
    PickPoint resize_handle;               // Screen-space handle used by resize hit tests.
};

// Integer map/heightmap rectangle extracted from a tile-swap part.
struct TileSwapRegionMetrics {
    int x;       // Left/source column of the region.
    int y;       // Top/source row of the region.
    int width;   // Region width in map tiles or heightmap cells.
    int height;  // Region height in map tiles or heightmap cells.
};

// Projects every editable tile-swap source and destination region for a room.
std::vector<TileSwapRegionGeometry> BuildTileSwapRegionGeometries(
    const std::shared_ptr<Landstalker::GameData>& gd,
    uint16_t room,
    const MapRenderer& map_renderer,
    float z_extent);

// Extracts the editable rectangle for a specific tile-swap part.
TileSwapRegionMetrics MetricsForTileSwapRegion(const Landstalker::TileSwap& swap, TileSwapRegionPart part);

// Converts ROM warp data into the canvas runtime shape used for selection and rendering.
WarpInstance MakeWarpInstance(
    const Landstalker::WarpList::Warp& warp,
    uint16_t current_room,
    uint32_t instance_id,
    float room_left,
    float room_top,
    float z_extent = 32.0f,
    uint32_t warp_key = 0,
    int side_override = 0);

// Enforces the editor's legal warp footprint ratios.
float ValidWarpWidth(float requested_width, float current_height);
// Enforces the editor's legal warp footprint ratios.
float ValidWarpHeight(float requested_height, float current_width);
// Clamps a warp instance to a legal editable footprint.
void ClampWarpToValidSize(WarpInstance& warp);

// Orders entity sprites so the renderer draws overlapping objects consistently.
void SortEntitiesGeometrically(std::vector<SpriteInstance>& instances);

}  // namespace GLCanvasObjectSupport

#endif  // GL_CANVAS_OBJECT_SUPPORT_H
