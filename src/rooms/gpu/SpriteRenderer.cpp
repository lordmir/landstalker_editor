#include "SpriteRenderer.h"
#include "GLLoader.h"
#include "PixelFont.h"
#include "ShaderSources.h"
#include <landstalker/sprites/SpriteFrame.h>
#include <landstalker/palettes/Palette.h>
#include <landstalker/main/ImageBuffer.h>
#include <iostream>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <utility>

using namespace Landstalker;

namespace {
constexpr int ROOM_PALETTE_ROW = 0;
constexpr int PLAYER_PALETTE_ROW = 1;
constexpr int ENTITY_PALETTE_ROW_OFFSET = 2;
constexpr int PALETTE_COLOURS = 16;
constexpr int PALETTE_TEXTURE_ROWS = 512;

void WritePaletteRow(std::vector<uint8_t>& data, int row, const Palette& palette)
{
    for (int j = 0; j < PALETTE_COLOURS; ++j)
    {
        uint32_t c = palette.getRGBA(j);
        data[(row * PALETTE_COLOURS + j) * 4 + 0] = (c >> 16) & 0xFF;
        data[(row * PALETTE_COLOURS + j) * 4 + 1] = (c >> 8) & 0xFF;
        data[(row * PALETTE_COLOURS + j) * 4 + 2] = (c >> 0) & 0xFF;
        data[(row * PALETTE_COLOURS + j) * 4 + 3] = (c >> 24) & 0xFF;
    }
}

int PaletteRowForInstance(const SpriteInstance& inst)
{
    if (inst.palette == 0)
    {
        return ROOM_PALETTE_ROW;
    }
    if (inst.palette == 2)
    {
        return PLAYER_PALETTE_ROW;
    }
    return ENTITY_PALETTE_ROW_OFFSET + inst.entity_id;
}

struct SpritePoint {
    float x;
    float y;
};

SpritePoint ProjectEntityGridPoint(const SpriteInstance& inst, float x, float y, float z)
{
    float grid_x = x - inst.room_left;
    float grid_y = y - inst.room_top;
    return {
        32.0f * grid_x - 32.0f * grid_y + 512.0f,
        16.0f * grid_x + 16.0f * grid_y + 100.0f - inst.z_extent * z
    };
}

std::pair<GLint, GLint> EntityDepthStencilRange(const SpriteInstance& inst);

void DrawHitboxLine(const SpritePoint& a, const SpritePoint& b, float r, float g, float bl, float alpha)
{
    glColor4f(r, g, bl, alpha);
    glBegin(GL_LINES);
    glVertex2f(a.x, a.y);
    glVertex2f(b.x, b.y);
    glEnd();
}

void DrawDottedLine(const SpritePoint& a, const SpritePoint& b)
{
    float length = std::hypot(b.x - a.x, b.y - a.y);
    int segments = std::max(2, static_cast<int>(std::ceil(length / 4.0f)));
    if ((segments % 2) != 0) {
        ++segments;
    }
    glBegin(GL_LINES);
    for (int i = 0; i < segments; i += 2) {
        float t0 = float(i) / float(segments);
        float t1 = float(i + 1) / float(segments);
        glVertex2f(a.x + (b.x - a.x) * t0, a.y + (b.y - a.y) * t0);
        glVertex2f(a.x + (b.x - a.x) * t1, a.y + (b.y - a.y) * t1);
    }
    glEnd();
}

char HexDigit(int value)
{
    value &= 0x0F;
    return value < 10 ? static_cast<char>('0' + value) : static_cast<char>('A' + value - 10);
}

std::string HexByte(int value)
{
    value = std::clamp(value, 0, 255);
    std::string result;
    result.push_back(HexDigit(value >> 4));
    result.push_back(HexDigit(value));
    return result;
}

void SetShadowFillColor(bool selected, float z_delta, float alpha_scale)
{
    if (selected && z_delta < 0.0f) {
        glColor4f(1.0f, 0.0f, 0.0f, 0.55f * alpha_scale);
    } else if (selected) {
        glColor4f(1.0f, 0.92f, 0.0f, 0.45f * alpha_scale);
    } else if (z_delta < 0.0f) {
        glColor4f(1.0f, 0.0f, 0.0f, 0.45f * alpha_scale);
    } else {
        glColor4f(0.0f, 0.0f, 0.0f, 0.38f * alpha_scale);
    }
}

void DrawShadowSolidOutline(const std::array<SpritePoint, 4>& footprint)
{
    glBegin(GL_LINE_LOOP);
    for (const auto& point : footprint) {
        glVertex2f(point.x, point.y);
    }
    glEnd();
}

void DrawShadowDottedOutline(const std::array<SpritePoint, 4>& footprint)
{
    for (std::size_t i = 0; i < footprint.size(); ++i) {
        DrawDottedLine(footprint[i], footprint[(i + 1) % footprint.size()]);
    }
}

void DrawEntityShadow(const SpriteInstance& inst, bool selected, float shadow_z, bool occluded, bool split_outline_with_stencil)
{
    if (inst.hitbox_base <= 0.0f) {
        return;
    }

    float z_delta = inst.map_z - shadow_z;
    if (std::abs(z_delta) <= 0.01f) {
        return;
    }

    float center_x = inst.map_x + inst.hitbox_offset;
    float center_y = inst.map_y + inst.hitbox_offset;
    float half_base = inst.hitbox_base * 0.5f;
    float min_x = center_x - half_base;
    float min_y = center_y - half_base;
    float max_x = center_x + half_base;
    float max_y = center_y + half_base;
    SpritePoint base_center = ProjectEntityGridPoint(inst, center_x, center_y, inst.map_z);
    SpritePoint shadow_center = ProjectEntityGridPoint(inst, center_x, center_y, shadow_z);
    std::array<SpritePoint, 4> footprint = {
        ProjectEntityGridPoint(inst, min_x, min_y, shadow_z),
        ProjectEntityGridPoint(inst, max_x, min_y, shadow_z),
        ProjectEntityGridPoint(inst, max_x, max_y, shadow_z),
        ProjectEntityGridPoint(inst, min_x, max_y, shadow_z)
    };

    SetShadowFillColor(selected, z_delta, occluded ? 0.42f : 1.0f);
    glBegin(GL_QUADS);
    for (const auto& point : footprint) {
        glVertex2f(point.x, point.y);
    }
    glEnd();

    float alpha_scale = occluded ? 0.42f : 1.0f;
    glLineWidth(selected ? 2.0f : 1.0f);
    if (selected || z_delta < 0.0f) {
        glColor4f(1.0f, 0.05f, 0.05f, 0.95f);
    } else {
        glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
    }
    if (occluded && split_outline_with_stencil) {
        glEnable(GL_STENCIL_TEST);
        glStencilMask(0x00);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilFunc(GL_NOTEQUAL, 1, 0x01);
        DrawShadowSolidOutline(footprint);
        glStencilMask(0xFF);
        glDisable(GL_STENCIL_TEST);
    } else {
        DrawShadowSolidOutline(footprint);
    }

    if (occluded) {
        glLineWidth(selected ? 2.0f : 1.0f);
        if (selected || z_delta < 0.0f) {
            glColor4f(1.0f, 0.05f, 0.05f, (selected ? 0.95f : 0.8f) * 0.42f);
        } else {
            glColor4f(1.0f, 1.0f, 1.0f, 0.85f * 0.42f);
        }
        if (split_outline_with_stencil) {
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0x00);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glStencilFunc(GL_EQUAL, 1, 0x01);
            DrawShadowDottedOutline(footprint);
            glStencilMask(0xFF);
            glDisable(GL_STENCIL_TEST);
        } else {
            DrawShadowDottedOutline(footprint);
        }
    }

    glLineWidth(selected ? 2.0f : 1.0f);
    if (z_delta < 0.0f) {
        glColor4f(1.0f, 0.05f, 0.05f, (selected ? 0.95f : 0.75f) * alpha_scale);
    } else {
        glColor4f(1.0f, 1.0f, 1.0f, (selected ? 0.95f : 0.65f) * alpha_scale);
    }
    glBegin(GL_LINES);
    glVertex2f(base_center.x, base_center.y);
    glVertex2f(shadow_center.x, shadow_center.y);
    glEnd();
}

void DrawEntityHitbox(
    const SpriteInstance& inst,
    bool selected,
    bool front,
    bool collided,
    bool split_with_stencil,
    GLint stencil_ref,
    GLint stencil_mask)
{
    if (inst.hitbox_base <= 0.0f) {
        return;
    }

    float center_x = inst.map_x + inst.hitbox_offset;
    float center_y = inst.map_y + inst.hitbox_offset;
    float half_base = inst.hitbox_base * 0.5f;
    float min_x = center_x - half_base;
    float min_y = center_y - half_base;
    float max_x = center_x + half_base;
    float max_y = center_y + half_base;
    float bottom_z = inst.map_z;
    float height = std::max(inst.hitbox_height, 0.125f);
    std::array<SpritePoint, 4> bottom = {
        ProjectEntityGridPoint(inst, min_x, min_y, bottom_z),
        ProjectEntityGridPoint(inst, max_x, min_y, bottom_z),
        ProjectEntityGridPoint(inst, max_x, max_y, bottom_z),
        ProjectEntityGridPoint(inst, min_x, max_y, bottom_z)
    };
    std::array<SpritePoint, 4> top = {
        ProjectEntityGridPoint(inst, min_x, min_y, bottom_z + height),
        ProjectEntityGridPoint(inst, max_x, min_y, bottom_z + height),
        ProjectEntityGridPoint(inst, max_x, max_y, bottom_z + height),
        ProjectEntityGridPoint(inst, min_x, max_y, bottom_z + height)
    };

    glLineWidth(1.0f);
    float r = selected ? 1.0f : (collided ? 1.0f : 0.0f);
    float g = selected ? 0.1f : (collided ? 0.75f : 0.95f);
    float b = selected ? 0.05f : (collided ? 0.0f : 1.0f);
    float vertical_g = selected ? 0.05f : (collided ? 0.45f : 0.65f);
    float vertical_b = selected ? 0.05f : (collided ? 0.0f : 0.85f);
    auto draw_line = [&](const SpritePoint& a, const SpritePoint& c, float cr, float cg, float cb, float alpha, bool dotted) {
        if (dotted) {
            glColor4f(cr, cg, cb, alpha);
            DrawDottedLine(a, c);
        } else {
            DrawHitboxLine(a, c, cr, cg, cb, alpha);
        }
    };
    auto draw_segment = [&](std::size_t i, std::size_t next, bool dotted) {
        float alpha_scale = dotted ? 0.42f : 1.0f;
        draw_line(bottom[i], bottom[next], r, g, b, 0.9f * alpha_scale, dotted);
        draw_line(top[i], top[next], r, g, b, 0.9f * alpha_scale, dotted);
        draw_line(
            bottom[i],
            top[i],
            r,
            vertical_g,
            vertical_b,
            0.75f * alpha_scale,
            dotted);
    };
    auto draw_half = [&](bool dotted) {
        if (front) {
            draw_segment(1, 2, dotted);
            draw_segment(2, 3, dotted);
        } else {
            draw_segment(0, 1, dotted);
            draw_segment(3, 0, dotted);
        }
    };

    if (split_with_stencil) {
        glEnable(GL_STENCIL_TEST);
        glStencilMask(0x00);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilFunc(GL_NOTEQUAL, stencil_ref, stencil_mask);
        draw_half(false);
        glStencilFunc(GL_EQUAL, stencil_ref, stencil_mask);
        draw_half(true);
        glStencilMask(0xFF);
        glDisable(GL_STENCIL_TEST);
    } else {
        draw_half(false);
    }
}

void DisableFixedFunctionTexturing()
{
    for (int i = 0; i <= 1; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE0);
}

void DrawSpriteQuad(float sx, float sy, float sw, float sh, float u0, float u1, const FrameMetadata& meta)
{
    glBegin(GL_QUADS);
    glTexCoord2f(u0, meta.v0);
    glVertex2f(sx, sy);
    glTexCoord2f(u1, meta.v0);
    glVertex2f(sx + sw, sy);
    glTexCoord2f(u1, meta.v1);
    glVertex2f(sx + sw, sy + sh);
    glTexCoord2f(u0, meta.v1);
    glVertex2f(sx, sy + sh);
    glEnd();
}

void DrawSelectedSpriteHighlight(const SpriteInstance& inst, int collision_warning)
{
    if (collision_warning == 0) {
        return;
    }

    glUseProgram(0);
    DisableFixedFunctionTexturing();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float center_x = inst.map_x + inst.hitbox_offset;
    float center_y = inst.map_y + inst.hitbox_offset;
    float half_base = inst.hitbox_base * 0.5f;
    float min_x = center_x - half_base;
    float min_y = center_y - half_base;
    float max_x = center_x + half_base;
    float max_y = center_y + half_base;
    float top_z = inst.map_z + std::max(inst.hitbox_height, 0.125f);
    std::array<SpritePoint, 8> points = {
        ProjectEntityGridPoint(inst, min_x, min_y, inst.map_z),
        ProjectEntityGridPoint(inst, max_x, min_y, inst.map_z),
        ProjectEntityGridPoint(inst, max_x, max_y, inst.map_z),
        ProjectEntityGridPoint(inst, min_x, max_y, inst.map_z),
        ProjectEntityGridPoint(inst, min_x, min_y, top_z),
        ProjectEntityGridPoint(inst, max_x, min_y, top_z),
        ProjectEntityGridPoint(inst, max_x, max_y, top_z),
        ProjectEntityGridPoint(inst, min_x, max_y, top_z)
    };
    float sx = points.front().x;
    float sy = points.front().y;
    float ex = points.front().x;
    float ey = points.front().y;
    for (const auto& point : points) {
        sx = std::min(sx, point.x);
        sy = std::min(sy, point.y);
        ex = std::max(ex, point.x);
        ey = std::max(ey, point.y);
    }

    float r = 1.0f;
    float g = collision_warning == 2 ? 0.05f : (collision_warning == 1 ? 0.55f : 0.95f);
    float b = collision_warning == 0 ? 0.05f : 0.0f;

    glColor4f(r, g, b, collision_warning == 0 ? 0.18f : 0.28f);
    glBegin(GL_QUADS);
    glVertex2f(sx, sy);
    glVertex2f(ex, sy);
    glVertex2f(ex, ey);
    glVertex2f(sx, ey);
    glEnd();

    glLineWidth(2.0f);
    glColor4f(r, g, b, 0.95f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(sx, sy);
    glVertex2f(ex, sy);
    glVertex2f(ex, ey);
    glVertex2f(sx, ey);
    glEnd();
    glLineWidth(1.0f);
}

void DrawInvalidEntityMarker(const SpriteInstance& inst, bool selected)
{
    glUseProgram(0);
    DisableFixedFunctionTexturing();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float size = selected ? 18.0f : 14.0f;
    float thickness = selected ? 3.0f : 2.0f;
    glLineWidth(thickness);
    glColor4f(1.0f, 0.05f, 0.05f, selected ? 1.0f : 0.82f);
    glBegin(GL_LINES);
    glVertex2f(inst.x - size, inst.y - size);
    glVertex2f(inst.x + size, inst.y + size);
    glVertex2f(inst.x + size, inst.y - size);
    glVertex2f(inst.x - size, inst.y + size);
    glEnd();

    glLineWidth(1.0f);
}

std::pair<GLint, GLint> EntityDepthStencilRange(const SpriteInstance& inst)
{
    float center_x = inst.map_x + inst.hitbox_offset;
    float center_y = inst.map_y + inst.hitbox_offset;
    float half_base = inst.hitbox_base * 0.5f;
    float min_depth = (center_x - half_base) + (center_y - half_base);
    float max_depth = (center_x + half_base) + (center_y + half_base) + 25.0f;
    int rear_edge_padding = std::abs(inst.map_z - inst.floor_z) <= 0.01f ? 1 : 0;
    return {
        std::clamp(static_cast<int>(std::ceil(min_depth)) + rear_edge_padding, 0, 255),
        std::clamp(static_cast<int>(std::ceil(max_depth)), 0, 255)
    };
}
}

SpriteRenderer::SpriteRenderer(std::shared_ptr<GameData> gd)
    : m_gd(gd), m_texture_id(0), m_pal_texture_id(0), m_shader_program(0), m_tex_w(0), m_tex_h(0), m_opacity(1.0f), m_palette_rows(PALETTE_TEXTURE_ROWS),
      m_current_room(0), m_room_left(0), m_room_top(0)
{
}

SpriteRenderer::~SpriteRenderer()
{
    if (m_texture_id)
        glDeleteTextures(1, &m_texture_id);
    if (m_pal_texture_id)
        glDeleteTextures(1, &m_pal_texture_id);
}

void SpriteRenderer::Init()
{
    InitTexture();
    InitShaders();
}

void SpriteRenderer::LoadRoom(uint16_t roomnum)
{
    m_current_room = roomnum;
    auto map_entry = m_gd->GetRoomData()->GetMapForRoom(roomnum);
    if (map_entry) {
        auto map = map_entry->GetData();
        m_room_left = map->GetLeft();
        m_room_top = map->GetTop();
    }

    if (!m_pal_texture_id)
    {
        return;
    }

    auto palette_entry = m_gd->GetRoomData()->GetPaletteForRoom(roomnum);
    if (!palette_entry)
    {
        return;
    }

    std::vector<uint8_t> row(PALETTE_COLOURS * 4, 0);
    WritePaletteRow(row, 0, *palette_entry->GetData());
    glBindTexture(GL_TEXTURE_2D, m_pal_texture_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, ROOM_PALETTE_ROW, PALETTE_COLOURS, 1, GL_RGBA, GL_UNSIGNED_BYTE, row.data());
}

void SpriteRenderer::Render(
    const std::vector<SpriteInstance> &instances,
    float cam_x,
    float cam_y,
    int selected_entity_index,
    int selected_collision_warning,
    OcclusionMode occlusion_mode,
    bool show_sprites,
    bool show_hitboxes,
    GLint occlusion_stencil_ref,
    GLint occlusion_stencil_mask,
    const std::function<void(GLint, GLint, float, float, float, float, float, float, float, float, float, float)>& build_entity_occlusion_stencil,
    const std::function<float(float, float)>& floor_at_point,
    const std::function<bool(float, float, float, float, float)>& shadow_occluded,
    const std::function<void(float, float, float, float, float, float, float, float, float)>& build_shadow_occlusion_stencil,
    const std::function<bool(uint32_t)>& entity_collided)
{
    if (m_opacity <= 0.0f && !show_hitboxes) {
        return;
    }

    if (m_texture_id && m_shader_program)
    {
        const bool render_sprites = show_sprites && m_opacity > 0.0f;
        auto bind_sprite_shader = [&](float alpha) {
            glUseProgram(m_shader_program);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_texture_id);
            glUniform1i(glGetUniformLocation(m_shader_program, "u_atlas"), 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_pal_texture_id);
            glUniform1i(glGetUniformLocation(m_shader_program, "u_palettes"), 1);
            glUniform1f(glGetUniformLocation(m_shader_program, "u_alpha"), alpha);
            glActiveTexture(GL_TEXTURE0);
        };

        // Activate sprite shader and bind the atlas and palette textures
        if (render_sprites) {
            bind_sprite_shader(m_opacity);
        }

        // Sprites usually have transparency, so we enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (show_hitboxes) {
            glUseProgram(0);
            DisableFixedFunctionTexturing();
            for (std::size_t inst_index = 0; inst_index < instances.size(); ++inst_index) {
                const auto& inst = instances[inst_index];
                float center_x = inst.map_x + inst.hitbox_offset;
                float center_y = inst.map_y + inst.hitbox_offset;
                float half_base = inst.hitbox_base * 0.5f;
                float min_x = center_x - half_base;
                float min_y = center_y - half_base;
                float max_x = center_x + half_base;
                float max_y = center_y + half_base;
                float shadow_z = floor_at_point ? floor_at_point(center_x, center_y) : inst.floor_z;
                bool occluded = shadow_occluded &&
                    shadow_occluded(min_x, min_y, max_x, max_y, shadow_z);
                if (occluded && build_shadow_occlusion_stencil) {
                    std::array<SpritePoint, 4> footprint = {
                        ProjectEntityGridPoint(inst, min_x, min_y, shadow_z),
                        ProjectEntityGridPoint(inst, max_x, min_y, shadow_z),
                        ProjectEntityGridPoint(inst, max_x, max_y, shadow_z),
                        ProjectEntityGridPoint(inst, min_x, max_y, shadow_z)
                    };
                    float screen_min_x = footprint.front().x;
                    float screen_min_y = footprint.front().y;
                    float screen_max_x = footprint.front().x;
                    float screen_max_y = footprint.front().y;
                    for (const auto& point : footprint) {
                        screen_min_x = std::min(screen_min_x, point.x);
                        screen_min_y = std::min(screen_min_y, point.y);
                        screen_max_x = std::max(screen_max_x, point.x);
                        screen_max_y = std::max(screen_max_y, point.y);
                    }
                    build_shadow_occlusion_stencil(min_x, min_y, max_x, max_y, shadow_z, screen_min_x, screen_min_y, screen_max_x, screen_max_y);
                    glUseProgram(0);
                    DisableFixedFunctionTexturing();
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
                DrawEntityShadow(
                    inst,
                    selected_entity_index == static_cast<int>(inst_index),
                    shadow_z,
                    occluded,
                    occluded && bool(build_shadow_occlusion_stencil));
            }
            if (render_sprites) {
                bind_sprite_shader(m_opacity);
            }
        }

        auto sd = m_gd->GetSpriteData();
        auto render_instance = [&](std::size_t inst_index) {
            const auto &inst = instances[inst_index];
            bool selected = selected_entity_index == static_cast<int>(inst_index);
            bool collided = entity_collided && entity_collided(inst.instance_id);

            if (!sd->IsEntity(inst.entity_id)) {
                if (show_hitboxes) {
                    glUseProgram(0);
                    DisableFixedFunctionTexturing();
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    DrawEntityHitbox(
                        inst,
                        selected,
                        false,
                        collided,
                        false,
                        occlusion_stencil_ref,
                        occlusion_stencil_mask);
                    DrawEntityHitbox(
                        inst,
                        selected,
                        true,
                        collided,
                        false,
                        occlusion_stencil_ref,
                        occlusion_stencil_mask);
                }
                if (render_sprites) {
                    DrawInvalidEntityMarker(inst, selected);
                    bind_sprite_shader(m_opacity);
                }
                return;
            }

            uint8_t sid = sd->GetSpriteFromEntity(inst.entity_id);
            auto flags = sd->GetSpriteAnimationFlags(sid);
            auto anims = sd->GetSpriteAnimations(sid);

            // Logic to determine which animation set (towards vs away) to use based on orientation
            bool has_away = !flags.do_not_rotate && !sd->IsEntityItem(inst.entity_id);
            int towards = 0, away = 0;
            if (flags.has_full_animations)
            {
                towards = 1;
                away = 1;
            }
            if (has_away)
            {
                towards = towards * 2 + 1;
                away = away * 2;
            }
            int aid = (inst.orientation == Orientation::NE || inst.orientation == Orientation::NW) ? away : towards;
            if(sd->IsEntityItem(inst.entity_id))
            {
                aid = sd->GetDefaultEntityAnimationId(inst.entity_id);
            }
            if (aid >= (int)anims.size())
                aid = 0;

            const auto &frames = m_sprite_meta[sid].animations[aid];
            if (frames.empty())
                return;

            // Get the metadata for the current frame of animation
            int fidx = static_cast<int>(inst.anim_timer);
            if (fidx < 0 || fidx >= (int)frames.size())
                fidx = 0;
            if(sd->IsEntityItem(inst.entity_id))
            {
                fidx = sd->GetDefaultEntityFrameId(inst.entity_id);
            }
            const auto &meta = frames[fidx];

            // Handle horizontal flipping for NW/SE orientations
            bool flip = flags.do_not_rotate ? false : (inst.orientation == Orientation::SE || inst.orientation == Orientation::NW);
            float sx = inst.x + (flip ? -(meta.origin_x + meta.width) : meta.origin_x) * inst.scale;
            float sy = inst.y + meta.origin_y * inst.scale, sw = meta.width * inst.scale, sh = meta.height * inst.scale;

            // Calculate UV coordinates for sampling the sprite from the large atlas
            float u0 = flip ? meta.u1 : meta.u0, u1 = flip ? meta.u0 : meta.u1;

            glUniform1f(glGetUniformLocation(m_shader_program, "u_pal_row"), (float(PaletteRowForInstance(inst)) + 0.5f) / float(m_palette_rows));

            float center_x = inst.map_x + inst.hitbox_offset;
            float center_y = inst.map_y + inst.hitbox_offset;
            float half_base = inst.hitbox_base * 0.5f;
            float min_x = center_x - half_base;
            float min_y = center_y - half_base;
            float max_x = center_x + half_base;
            float max_y = center_y + half_base;
            float top_z = inst.map_z + std::max(inst.hitbox_height, 0.125f);
            auto depth_range = EntityDepthStencilRange(inst);
            bool split_hitbox_with_stencil = occlusion_mode != OcclusionMode::AlwaysOnTop && bool(build_entity_occlusion_stencil);
            auto build_occlusion_stencil = [&]() {
                build_entity_occlusion_stencil(
                    depth_range.first,
                    depth_range.second,
                    inst.map_z,
                    min_x,
                    min_y,
                    max_x,
                    max_y,
                    top_z,
                    sx,
                    sy,
                    sx + sw,
                    sy + sh);
            };

            if (show_hitboxes) {
                if (split_hitbox_with_stencil) {
                    build_occlusion_stencil();
                }
                glUseProgram(0);
                DisableFixedFunctionTexturing();
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                DrawEntityHitbox(
                    inst,
                    selected,
                    false,
                    collided,
                    split_hitbox_with_stencil,
                    occlusion_stencil_ref,
                    occlusion_stencil_mask);
                if (render_sprites) {
                    bind_sprite_shader(m_opacity);
                }
            }

            if (render_sprites) {
                if (occlusion_mode == OcclusionMode::AlwaysOnTop) {
                    glDisable(GL_STENCIL_TEST);
                    bind_sprite_shader(m_opacity);
                    DrawSpriteQuad(sx, sy, sw, sh, u0, u1, meta);
                } else if (build_entity_occlusion_stencil) {
                    build_occlusion_stencil();
                    glEnable(GL_STENCIL_TEST);
                    glStencilMask(0x00);
                    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

                    // Pass 1: draw only fragments that are NOT occluded.
                    bind_sprite_shader(m_opacity);
                    glStencilFunc(GL_NOTEQUAL, occlusion_stencil_ref, occlusion_stencil_mask);
                    DrawSpriteQuad(sx, sy, sw, sh, u0, u1, meta);

                    if (occlusion_mode == OcclusionMode::ObscuredTransparent) {
                        // Pass 2: draw occluded fragments with reduced alpha for ghost mode.
                        bind_sprite_shader(m_opacity * 0.35f);
                        glStencilFunc(GL_EQUAL, occlusion_stencil_ref, occlusion_stencil_mask);
                        DrawSpriteQuad(sx, sy, sw, sh, u0, u1, meta);
                    }

                    glStencilMask(0xFF);
                    glDisable(GL_STENCIL_TEST);
                    bind_sprite_shader(m_opacity);
                } else {
                    bind_sprite_shader(m_opacity);
                    DrawSpriteQuad(sx, sy, sw, sh, u0, u1, meta);
                }

                if (selected) {
                    DrawSelectedSpriteHighlight(inst, selected_collision_warning);
                    bind_sprite_shader(m_opacity);
                }
            }

            if (show_hitboxes) {
                glUseProgram(0);
                DisableFixedFunctionTexturing();
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                DrawEntityHitbox(
                    inst,
                    selected,
                    true,
                    collided,
                    split_hitbox_with_stencil,
                    occlusion_stencil_ref,
                    occlusion_stencil_mask);
                if (render_sprites) {
                    bind_sprite_shader(m_opacity);
                }
            }
        };

        for (std::size_t inst_index = 0; inst_index < instances.size(); ++inst_index) {
            if (static_cast<int>(inst_index) != selected_entity_index) {
                render_instance(inst_index);
            }
        }
        if (selected_entity_index >= 0 && selected_entity_index < static_cast<int>(instances.size())) {
            render_instance(static_cast<std::size_t>(selected_entity_index));
        }
        glUseProgram(0);
    }
}

void SpriteRenderer::InitTexture()
{
    auto sd = m_gd->GetSpriteData();
    std::map<std::string, FrameMetadata> global_frame_cache;
    struct FrameToRender
    {
        std::string name;
        std::shared_ptr<SpriteFrameEntry> entry;
    };
    std::vector<FrameToRender> queue;
    for (int sid = 0; sid < 256; ++sid)
    {
        if (!sd->IsSprite(sid))
            continue;
        for (const auto &fname : sd->GetSpriteFrames(sid))
        {
            if (!global_frame_cache.count(fname))
            {
                auto fe = sd->GetSpriteFrame(fname);
                if (fe)
                {
                    queue.push_back({fname, fe});
                    global_frame_cache[fname] = {};
                }
            }
        }
    }
    if (queue.empty())
        return;
    m_tex_w = 2048;
    int cur_x = 0, cur_y = 0, row_h = 0;
    for (const auto &it : queue)
    {
        int fw = it.entry->GetData()->GetWidth(), fh = it.entry->GetData()->GetHeight();
        if (cur_x + fw + 1 > m_tex_w)
        {
            cur_x = 0;
            cur_y += row_h + 1;
            row_h = 0;
        }
        row_h = std::max(row_h, fh);
        cur_x += fw + 1;
    }
    int m_tex_h_local = 1;
    while (m_tex_h_local < cur_y + row_h + 1)
        m_tex_h_local <<= 1;
    m_tex_h = m_tex_h_local;
    std::vector<uint8_t> data(m_tex_w * m_tex_h, 0);
    cur_x = 0;
    cur_y = 0;
    row_h = 0;
    for (auto &it : queue)
    {
        auto f = it.entry->GetData();
        int fw = f->GetWidth(), fh = f->GetHeight();
        if (cur_x + fw + 1 > m_tex_w)
        {
            cur_x = 0;
            cur_y += row_h + 1;
            row_h = 0;
        }
        FrameMetadata meta = {(float)cur_x / m_tex_w, (float)cur_y / m_tex_h, (float)(cur_x + fw) / m_tex_w, (float)(cur_y + fh) / m_tex_h, fw, fh, f->GetLeft(), f->GetTop()};
        global_frame_cache[it.name] = meta;
        ImageBuffer ib(fw, fh);
        ib.InsertSprite(-meta.origin_x, -meta.origin_y, 0, *f);
        const auto &pix = ib.GetPixels();
        for (int y = 0; y < fh; ++y)
            for (int x = 0; x < fw; ++x)
                data[(cur_y + y) * m_tex_w + (cur_x + x)] = pix[y * fw + x];
        row_h = std::max(row_h, fh);
        cur_x += fw + 1;
    }
    for (int sid = 0; sid < 256; ++sid)
    {
        if (!sd->IsSprite(sid))
            continue;
        auto anims = sd->GetSpriteAnimations(sid);
        for (size_t aid = 0; aid < anims.size(); ++aid)
        {
            for (const auto &fn : sd->GetSpriteAnimationFrames(anims[aid]))
                if (global_frame_cache.count(fn))
                    m_sprite_meta[sid].animations[aid].push_back(global_frame_cache[fn]);
        }
    }
    glGenTextures(1, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_tex_w, m_tex_h, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    std::vector<uint8_t> pal_data(m_palette_rows * PALETTE_COLOURS * 4, 0);

    auto player_palette = m_gd->GetGraphicsData()->GetPlayerPalette();
    if (player_palette)
    {
        WritePaletteRow(pal_data, PLAYER_PALETTE_ROW, *player_palette->GetData());
    }

    for (int i = 0; i < 256; ++i)
    {
        if (sd->IsEntity(i))
        {
            auto p = sd->GetEntityPalette(i);
            WritePaletteRow(pal_data, ENTITY_PALETTE_ROW_OFFSET + i, *p);
        }
    }
    glGenTextures(1, &m_pal_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_pal_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PALETTE_COLOURS, m_palette_rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, pal_data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void SpriteRenderer::InitShaders()
{
    m_shader_program = CreateShader("sprite.vert", GpuShaders::kSpriteVertex, "sprite.frag", GpuShaders::kSpriteFragment);
}

GLuint SpriteRenderer::CreateShader(const char* vs_name, const char* vs_src, const char* fs_name, const char* fs_src)
{
    // Compile + link follows the same model as MapRenderer: source -> shader objects -> program.
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_src, nullptr);
    glCompileShader(vs);
    GLint status;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        char log[512];
        glGetShaderInfoLog(vs, 512, nullptr, log);
        std::cerr << "VS Error (" << vs_name << "): " << log << std::endl;
    }
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_src, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        char log[512];
        glGetShaderInfoLog(fs, 512, nullptr, log);
        std::cerr << "FS Error (" << fs_name << "): " << log << std::endl;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "Link Error: " << log << std::endl;
    }
    return prog;
}
