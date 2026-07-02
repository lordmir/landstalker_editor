#ifndef SPRITE_INSTANCE_H
#define SPRITE_INSTANCE_H

#include <cstdint>
#include <landstalker/rooms/Entity.h>

// Texture-frame metadata for a single sprite animation frame.
struct FrameMetadata {
    // Normalized texture coordinates for the frame rectangle.
    float u0, v0, u1, v1;
    // Frame dimensions in source pixels.
    int width, height;
    // Sprite origin offset used to align the frame to the entity anchor.
    int origin_x, origin_y;
};

// Animation metadata grouped by animation/orientation key.
struct SpriteAnimationSet {
    // Maps an animation key to its ordered list of rendered frames.
    std::map<int, std::vector<FrameMetadata>> animations;
};

// Runtime GPU-editor representation of a room entity.
struct SpriteInstance {
    // Stable editor-side identifier used to preserve selection across sorting.
    uint32_t instance_id;
    // Landstalker entity type and palette index.
    uint8_t entity_id;
    uint8_t palette;
    // Projected sprite anchor position.
    float x, y;
    // Entity position in room/map coordinates.
    float map_x, map_y, map_z;
    // Heightmap floor under the entity and current projection Z extent.
    float floor_z;
    float z_extent;
    // Collision/hitbox dimensions in room-grid units.
    float hitbox_base, hitbox_height, hitbox_offset;
    // Room origin used when projecting the entity.
    float room_left, room_top;
    // Render offset for sprite origin/alignment.
    float dx, dy;
    // Render scale applied to sprite pixels.
    float scale;
    // Current animation timer and speed.
    float anim_timer;
    float anim_speed;
    // Facing/orientation used to choose animation frames.
    Landstalker::Orientation orientation;
};

#endif // SPRITE_INSTANCE_H
