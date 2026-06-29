#ifndef SPRITE_INSTANCE_H
#define SPRITE_INSTANCE_H

#include <cstdint>
#include <landstalker/rooms/Entity.h>

struct FrameMetadata {
    float u0, v0, u1, v1;
    int width, height;
    int origin_x, origin_y;
};

struct SpriteAnimationSet {
    std::map<int, std::vector<FrameMetadata>> animations;
};

struct SpriteInstance {
    uint32_t instance_id;
    uint8_t entity_id;
    uint8_t palette;
    float x, y;
    float map_x, map_y, map_z;
    float floor_z;
    float z_extent;
    float hitbox_base, hitbox_height, hitbox_offset;
    float room_left, room_top;
    float dx, dy;
    float scale;
    float anim_timer;
    float anim_speed;
    Landstalker::Orientation orientation;
};

#endif // SPRITE_INSTANCE_H
