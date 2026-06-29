#ifndef HEIGHTMAP_RENDERER_H
#define HEIGHTMAP_RENDERER_H

#include <cstdint>
#include <memory>
#include <landstalker/main/GameData.h>
#include <landstalker/3d_maps/Tilemap3D.h>
#include "GLLoader.h"

class HeightmapRenderer {
public:
    explicit HeightmapRenderer(std::shared_ptr<Landstalker::GameData> gd);

    void LoadRoom(uint16_t roomnum);
    void SetPreviewMap(std::shared_ptr<Landstalker::Tilemap3D> map);
    void ClearPreviewMap();
    void Render();
    void BuildDepthStencil();
    void BuildEntityOcclusionStencil(
        GLint entity_back_depth,
        GLint entity_front_depth,
        float entity_z,
        float entity_min_x,
        float entity_min_y,
        float entity_max_x,
        float entity_max_y,
        float entity_top_z,
        float sprite_min_x,
        float sprite_min_y,
        float sprite_max_x,
        float sprite_max_y);
    void RenderEntityOcclusionDebug(
        GLint entity_back_depth,
        GLint entity_front_depth,
        float entity_z,
        float entity_min_x,
        float entity_min_y,
        float entity_max_x,
        float entity_max_y,
        float entity_top_z);
    void WriteEntityOcclusionDebugLog(
        const char* path,
        GLint entity_back_depth,
        GLint entity_front_depth,
        float entity_z,
        float entity_min_x,
        float entity_min_y,
        float entity_max_x,
        float entity_max_y,
        float entity_top_z) const;
    void SetHoverPoint(float x, float y);
    void ClearHover();
    int GetHoverX() const { return m_hover_x; }
    int GetHoverY() const { return m_hover_y; }
    void SetZExtent(float value);
    void AdjustZExtent(float delta);
    float GetZExtent() const { return m_z_extent; }

private:
    std::shared_ptr<Landstalker::Tilemap3D> CurrentMap() const;
    std::shared_ptr<Landstalker::GameData> m_gd;
    std::shared_ptr<Landstalker::Tilemap3D> m_preview_map;
    uint16_t m_current_room;
    int m_room_w;
    int m_room_h;
    int m_room_left;
    int m_room_top;
    int m_hover_x;
    int m_hover_y;
    float m_z_extent;
};

#endif // HEIGHTMAP_RENDERER_H
