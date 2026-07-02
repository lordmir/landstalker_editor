#ifndef ROOM_PROJECTION_H
#define ROOM_PROJECTION_H

namespace RoomProjection {

// A point in projected room/screen-space coordinates before camera transform.
struct PickPoint {
    float x;
    float y;
};

// Projects a room-grid coordinate into the isometric GPU room plane.
inline PickPoint ProjectRoomGridPoint(float x, float y, float z, float room_left, float room_top, float z_extent = 32.0f)
{
    float grid_x = x - room_left;
    float grid_y = y - room_top;
    return {
        32.0f * grid_x - 32.0f * grid_y + 512.0f,
        16.0f * grid_x + 16.0f * grid_y + 100.0f - z_extent * z
    };
}

// Projects a heightmap coordinate, including the heightmap-to-room origin offset.
inline PickPoint ProjectHeightmapGridPoint(float x, float y, float z, float room_left, float room_top, float z_extent)
{
    float grid_x = x - room_left + 12.0f;
    float grid_y = y - room_top + 12.0f;
    return {
        32.0f * grid_x - 32.0f * grid_y + 512.0f,
        16.0f * grid_x + 16.0f * grid_y + 100.0f - z_extent * z
    };
}

// Projects an entity-relative coordinate using room/z metadata from the instance.
template <typename EntityLike>
PickPoint ProjectEntityGridPoint(const EntityLike& inst, float x, float y, float z)
{
    return ProjectRoomGridPoint(x, y, z, inst.room_left, inst.room_top, inst.z_extent);
}

// Projects a warp-relative coordinate using room/z metadata from the warp instance.
template <typename WarpLike>
PickPoint ProjectWarpGridPoint(const WarpLike& warp, float x, float y, float z)
{
    return ProjectRoomGridPoint(x, y, z, warp.room_left, warp.room_top, warp.z_extent);
}

// Converts projected room-space coordinates back into map-grid coordinates.
inline PickPoint ScreenToMapPoint(float world_x, float world_y, float z, float room_left, float room_top, float z_extent = 32.0f)
{
    float a = (world_x - 512.0f) / 32.0f;
    float b = (world_y - 100.0f + z_extent * z) / 16.0f;
    return {
        (a + b) * 0.5f + room_left,
        (b - a) * 0.5f + room_top
    };
}

// Converts projected room-space coordinates back into heightmap-grid coordinates.
inline PickPoint ScreenToHeightmapPoint(float world_x, float world_y, float room_left, float room_top)
{
    return ScreenToMapPoint(world_x, world_y, 0.0f, room_left - 12.0f, room_top - 12.0f);
}

}  // namespace RoomProjection

#endif  // ROOM_PROJECTION_H
