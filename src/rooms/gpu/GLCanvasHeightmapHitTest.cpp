#include "GLCanvasHeightmapHitTest.h"

#include "GLCanvas.h"

#include <algorithm>
#include <cmath>

GLCanvasHeightmapHitTest::GLCanvasHeightmapHitTest(const GLCanvas& canvas)
    : m_canvas(canvas)
{
}

float GLCanvasHeightmapHitTest::FloorUnderRect(float min_x, float min_y, float max_x, float max_y) const
{
    auto map_entry = m_canvas.m_gd->GetRoomData()->GetMapForRoom(m_canvas.m_current_room);
    if (!map_entry) {
        return 0.0f;
    }

    auto map = m_canvas.m_tileswap_preview_map ? m_canvas.m_tileswap_preview_map : map_entry->GetData();
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

float GLCanvasHeightmapHitTest::FloorUnderPoint(float x, float y) const
{
    auto map_entry = m_canvas.m_gd->GetRoomData()->GetMapForRoom(m_canvas.m_current_room);
    if (!map_entry) {
        return 0.0f;
    }

    auto map = m_canvas.m_tileswap_preview_map ? m_canvas.m_tileswap_preview_map : map_entry->GetData();
    constexpr float heightmap_entity_offset = 12.0f;
    int cell_x = static_cast<int>(std::floor(x - heightmap_entity_offset));
    int cell_y = static_cast<int>(std::floor(y - heightmap_entity_offset));
    if (cell_x < 0 || cell_y < 0 || cell_x >= map->GetHeightmapWidth() || cell_y >= map->GetHeightmapHeight()) {
        return 0.0f;
    }

    uint8_t height = map->GetHeight({cell_x, cell_y});
    return height == 0xFF ? 0.0f : float(height);
}

bool GLCanvasHeightmapHitTest::ShadowOccludedByHeightmap(float min_x, float min_y, float max_x, float max_y, float z) const
{
    auto map_entry = m_canvas.m_gd->GetRoomData()->GetMapForRoom(m_canvas.m_current_room);
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

bool GLCanvasHeightmapHitTest::EntityCollidesWithHeightmap(const SpriteInstance& inst) const
{
    auto map_entry = m_canvas.m_gd->GetRoomData()->GetMapForRoom(m_canvas.m_current_room);
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

float GLCanvasHeightmapHitTest::FloorUnderHitbox(float center_x, float center_y, float half_base) const
{
    return FloorUnderRect(center_x - half_base, center_y - half_base, center_x + half_base, center_y + half_base);
}
