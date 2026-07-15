#ifndef GL_CANVAS_OBJECT_SUPPORT_H
#define GL_CANVAS_OBJECT_SUPPORT_H

#include <cstdint>
#include <memory>
#include <vector>
#include <landstalker/main/GameData.h>
#include <landstalker/3d_maps/Doors.h>
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

// Axis-aligned bounds in projected room space.
struct PickRect {
    float min_x;
    float min_y;
    float max_x;
    float max_y;
};

// Returns the axis-aligned bounds of a projected point set.
PickRect BoundsForPoints(const std::vector<PickPoint>& points);

// Converts raw entity hitbox dimensions to map blocks: the base is stored in
// eighths of a block, the height in sixteenths.
inline float HitboxBaseToBlocks(uint8_t base) {
    return float(base) / 8.0f;
}
inline float HitboxHeightToBlocks(uint8_t height) {
    return float(height) / 16.0f;
}
// Hitboxes 1.5 blocks and wider are anchored a half block in so the drawn
// volume stays centred on the sprite.
inline float HitboxDrawOffset(float hitbox_base) {
    return hitbox_base < 1.5f ? 0.0f : 0.5f;
}

// Screen-space polygon and metadata for a selectable tile-swap region.
struct TileSwapRegionGeometry {
    int flat_index;                        // Index in the flattened selectable region list.
    int swap_index;                        // Index of the owning tile swap in room data.
    TileSwapRegionPart part;               // Which map/heightmap source/destination this geometry describes.
    Landstalker::TileSwap swap;            // Tile-swap data used to build and edit the region.
    std::vector<PickPoint> points;         // Outline points or line segments drawn for the region.
    std::vector<PickPoint> fill_points;    // Polygon used for filled previews and containment tests.
    PickRect bounds;                       // Bounds of points and fill_points combined.
    bool segments;                         // True when points should be interpreted as independent line pairs.
    PickPoint resize_handle;               // Screen-space handle used by resize hit tests.
};

// Screen-space geometry for a selectable door.
struct DoorGeometry {
    int index;                             // Index of the door in room data.
    Landstalker::Door door;                // Door data used to build the geometry.
    bool valid;                            // True when the door maps onto valid room tiles.
    std::vector<PickPoint> cell_points;    // Diamond outline of the door's heightmap cell.
    std::vector<PickPoint> map_points;     // Outline of the map tiles affected by the door.
    PickRect bounds;                       // Bounds of cell_points and map_points combined.
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

// Projects every door in a room. When preview_map is set it is used instead of
// the canonical room tilemap (e.g. while a tile-swap/door preview is active).
std::vector<DoorGeometry> BuildDoorGeometries(
    const std::shared_ptr<Landstalker::GameData>& gd,
    uint16_t room,
    const MapRenderer& map_renderer,
    float z_extent,
    const std::shared_ptr<Landstalker::Tilemap3D>& preview_map = nullptr);

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
