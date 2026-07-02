#ifndef MAP_RENDERER_H
#define MAP_RENDERER_H

#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <landstalker/main/GameData.h>
#include <landstalker/3d_maps/Tilemap3D.h>
#include "GLLoader.h"

class MapRenderer {
public:
    MapRenderer(std::shared_ptr<Landstalker::GameData> gd);
    ~MapRenderer();

    void Init();
    void LoadRoom(uint16_t roomnum);
    void LoadPreviewRoom(uint16_t roomnum, const Landstalker::Tilemap3D& map);
    void Render(float cam_x, float cam_y);
    void RenderBackgroundOnly();
    void RenderBackgroundWithOpacity(float alpha);
    void RenderForegroundOnly();
    void RenderForegroundWithOpacity(float alpha);
    void RenderPriorityHighlight(Landstalker::Tilemap3D::Layer layer, float r, float g, float b, float alpha);
    void RenderBlockGhost(uint16_t block_id, int block_x, int block_y, float alpha, Landstalker::Tilemap3D::Layer layer);
    void RenderBlockPriorityHighlight(uint16_t block_id, int block_x, int block_y, Landstalker::Tilemap3D::Layer layer, float r, float g, float b, float alpha);
    void BuildForegroundCoverageStencil();
    void SetBackgroundOpacity(float opacity) { m_bg_opacity = opacity; }
    void SetForegroundOpacity(float opacity) { m_fg_opacity = opacity; }

    int GetRoomWidth() const { return m_room_w; }
    int GetRoomHeight() const { return m_room_h; }
    int GetRoomLeft() const { return m_room_left; }
    int GetRoomTop() const { return m_room_top; }

private:
    void UploadRoomMap(uint16_t roomnum, const Landstalker::Tilemap3D& map);
    void InitShaders();
    GLuint CreateShader(const char* vs_name, const char* vs_src, const char* fs_name, const char* fs_src);

    std::shared_ptr<Landstalker::GameData> m_gd;
    GLuint m_map_shader_program;
    GLuint m_map_tex_id;
    GLuint m_fg_map_tex_id;
    GLuint m_blockset_tex_id;
    GLuint m_tileset_tex_id;
    GLuint m_room_pal_tex_id;
    GLuint m_anim_meta_tex_id;
    GLuint m_anim_meta2_tex_id;
    GLuint m_editor_preview_map_tex_id;
    int m_tileset_tex_rows;
    std::chrono::steady_clock::time_point m_start_time;

    int m_room_w, m_room_h, m_room_left, m_room_top;
    uint16_t m_current_room;
    float m_bg_opacity;
    float m_fg_opacity;
};

#endif // MAP_RENDERER_H
