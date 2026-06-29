#ifndef SPRITE_RENDERER_H
#define SPRITE_RENDERER_H

#include <vector>
#include <map>
#include <memory>
#include <string>
#include <functional>
#include <landstalker/main/GameData.h>
#include "SpriteInstance.h"
#include "GLLoader.h"

class SpriteRenderer {
public:
    enum class OcclusionMode {
        AlwaysOnTop,
        ObscuredTransparent,
        ObscuredHidden
    };

    SpriteRenderer(std::shared_ptr<Landstalker::GameData> gd);
    ~SpriteRenderer();

    void Init();
    void LoadRoom(uint16_t roomnum);
    void Render(
        const std::vector<SpriteInstance>& instances,
        float cam_x,
        float cam_y,
        int selected_entity_index,
        int selected_collision_warning,
        OcclusionMode occlusion_mode,
        bool show_sprites,
        bool show_hitboxes,
        GLint occlusion_stencil_ref = 1,
        GLint occlusion_stencil_mask = 0x01,
        const std::function<void(GLint, GLint, float, float, float, float, float, float, float, float, float, float)>& build_entity_occlusion_stencil = {},
        const std::function<float(float, float)>& floor_at_point = {},
        const std::function<bool(float, float, float, float, float)>& shadow_occluded = {},
        const std::function<void(float, float, float, float, float, float, float, float, float)>& build_shadow_occlusion_stencil = {},
        const std::function<bool(uint32_t)>& entity_collided = {});
    void SetOpacity(float opacity) { m_opacity = opacity; }

private:
    void InitTexture();
    void InitShaders();
    GLuint CreateShader(const char* vs_name, const char* vs_src, const char* fs_name, const char* fs_src);

    std::shared_ptr<Landstalker::GameData> m_gd;
    GLuint m_texture_id;
    GLuint m_pal_texture_id;
    GLuint m_shader_program;
    std::map<uint8_t, SpriteAnimationSet> m_sprite_meta;
    int m_tex_w, m_tex_h;
    float m_opacity;
    int m_palette_rows;
    uint16_t m_current_room;
    int m_room_left;
    int m_room_top;
};

#endif // SPRITE_RENDERER_H
