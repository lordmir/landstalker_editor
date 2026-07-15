#include "MapRenderer.h"
#include "GLLoader.h"
#include "ShaderSources.h"
#include <landstalker/main/GameData.h>
#include <algorithm>
#include <iostream>
#include <chrono>

using namespace Landstalker;

MapRenderer::MapRenderer(std::shared_ptr<GameData> gd)
    : m_gd(gd), m_map_shader_program(0), m_map_tex_id(0), m_fg_map_tex_id(0),
      m_blockset_tex_id(0), m_tileset_tex_id(0), m_room_pal_tex_id(0),
    m_anim_meta_tex_id(0), m_anim_meta2_tex_id(0), m_editor_preview_map_tex_id(0), m_tileset_tex_rows(0),
      m_start_time(std::chrono::steady_clock::now()),
      m_room_w(0), m_room_h(0), m_room_left(0), m_room_top(0), m_current_room(0),
      m_bg_opacity(1.0f), m_fg_opacity(1.0f)
{
}

MapRenderer::~MapRenderer() {
    if (m_map_tex_id) glDeleteTextures(1, &m_map_tex_id);
    if (m_fg_map_tex_id) glDeleteTextures(1, &m_fg_map_tex_id);
    if (m_blockset_tex_id) glDeleteTextures(1, &m_blockset_tex_id);
    if (m_tileset_tex_id) glDeleteTextures(1, &m_tileset_tex_id);
    if (m_anim_meta_tex_id) glDeleteTextures(1, &m_anim_meta_tex_id);
    if (m_anim_meta2_tex_id) glDeleteTextures(1, &m_anim_meta2_tex_id);
    if (m_room_pal_tex_id) glDeleteTextures(1, &m_room_pal_tex_id);
    if (m_editor_preview_map_tex_id) glDeleteTextures(1, &m_editor_preview_map_tex_id);
}

void MapRenderer::Init() {
    InitShaders();
}

void MapRenderer::LoadRoom(uint16_t roomnum) {
    auto rd = m_gd->GetRoomData();
    auto map_entry = rd->GetMapForRoom(roomnum);
    if (!map_entry) return;
    auto map = map_entry->GetData();
    if (!map) return;
    UploadRoomMap(roomnum, *map);
}

void MapRenderer::LoadPreviewRoom(uint16_t roomnum, const Tilemap3D& map) {
    UploadRoomMap(roomnum, map);
}

void MapRenderer::UploadRoomMap(uint16_t roomnum, const Tilemap3D& map) {
    auto rd = m_gd->GetRoomData(); auto room = rd->GetRoom(roomnum); if (!room) return;
    auto tileset_entry = rd->GetTilesetForRoom(roomnum);
    if(!tileset_entry) return; auto tileset = tileset_entry->GetData();
    auto blockset = rd->GetCombinedBlocksetForRoom(roomnum); 
    auto palette_entry = rd->GetPaletteForRoom(roomnum); if(!palette_entry) return;
    auto pal = palette_entry->GetData();
    
    m_current_room = roomnum;
    m_room_w = map.GetWidth(); m_room_h = map.GetHeight();
    m_room_left = map.GetLeft(); m_room_top = map.GetTop();
    std::vector<uint8_t> map_rgba(m_room_w * m_room_h * 4, 0);
    for(size_t i=0; i<static_cast<size_t>(m_room_w * m_room_h); ++i) {
        uint16_t block = map.GetBlock(static_cast<uint16_t>(i), Tilemap3D::Layer::BG).value;
        map_rgba[i*4 + 0] = block & 0xFF;
        map_rgba[i*4 + 1] = (block >> 8) & 0xFF;
    }
    if (m_map_tex_id) glDeleteTextures(1, &m_map_tex_id);
    glGenTextures(1, &m_map_tex_id); glBindTexture(GL_TEXTURE_2D, m_map_tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_room_w, m_room_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, map_rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<uint8_t> fg_map_rgba(m_room_w * m_room_h * 4, 0);
    for(size_t i=0; i<static_cast<size_t>(m_room_w * m_room_h); ++i) {
        uint16_t block = map.GetBlock(static_cast<uint16_t>(i), Tilemap3D::Layer::FG).value;
        fg_map_rgba[i*4 + 0] = block & 0xFF;
        fg_map_rgba[i*4 + 1] = (block >> 8) & 0xFF;
    }
    if (m_fg_map_tex_id) glDeleteTextures(1, &m_fg_map_tex_id);
    glGenTextures(1, &m_fg_map_tex_id); glBindTexture(GL_TEXTURE_2D, m_fg_map_tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_room_w, m_room_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, fg_map_rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<uint8_t> bs_data(2 * blockset->size() * 2 * 4, 0);
    for(size_t i=0; i<blockset->size(); ++i) {
        for(int ty=0; ty<2; ++ty) for(int tx=0; tx<2; ++tx) {
            uint16_t val = blockset->at(i).GetTile(tx, ty).GetTileValue();
            int base = (i * 2 + ty) * 2 * 4 + tx * 4;
            bs_data[base + 0] = val & 0xFF; bs_data[base + 1] = (val >> 8) & 0xFF;
        }
    }
    if (m_blockset_tex_id) glDeleteTextures(1, &m_blockset_tex_id);
    glGenTextures(1, &m_blockset_tex_id); glBindTexture(GL_TEXTURE_2D, m_blockset_tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, blockset->size() * 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, bs_data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<uint8_t> ts_data;
    size_t base_tile_count = tileset->GetTileCount();
    ts_data.reserve(base_tile_count * 64);
    for(size_t i=0; i<base_tile_count; ++i) {
        auto pix = tileset->GetTile(Tile(i)); ts_data.insert(ts_data.end(), pix.begin(), pix.end());
    }

    // Build per-tile animation metadata and append animated frames after the base tileset.
    const auto& anim_entries = rd->GetAnimatedTilesets(tileset_entry->GetName());
    std::vector<uint8_t> anim_meta_1(base_tile_count * 4, 0);
    std::vector<uint8_t> anim_meta_2(base_tile_count * 4, 0);
    size_t appended_tile_count = 0;
    for (const auto& anim_entry : anim_entries) {
        auto anim = anim_entry->GetData();
        if (!anim) continue;
        size_t start_tile = anim->GetStartTile().GetIndex();
        size_t frame_tiles = anim->GetFrameSizeTiles();
        size_t frame_count = anim->GetAnimationFrames();
        size_t speed = anim->GetAnimationSpeed();
        if (frame_count == 0 || frame_tiles == 0) continue;

        size_t anim_start_index = base_tile_count + appended_tile_count;
        for (size_t f = 0; f < frame_count; ++f) {
            for (size_t t = 0; t < frame_tiles; ++t) {
                size_t tile_index = start_tile + t;
                auto pix = anim->GetTile(Tile(static_cast<uint16_t>(tile_index)), static_cast<uint8_t>(f));
                ts_data.insert(ts_data.end(), pix.begin(), pix.end());
            }
        }

        for (size_t t = 0; t < frame_tiles; ++t) {
            size_t tile_index = start_tile + t;
            if (tile_index >= base_tile_count) continue;
            size_t meta_idx = tile_index * 4;
            size_t frame_offset = anim_start_index + t;
            anim_meta_1[meta_idx + 0] = static_cast<uint8_t>(frame_count);
            anim_meta_1[meta_idx + 1] = static_cast<uint8_t>(speed);
            anim_meta_1[meta_idx + 2] = static_cast<uint8_t>(frame_tiles);
            anim_meta_1[meta_idx + 3] = static_cast<uint8_t>(frame_offset & 0xFF);
            anim_meta_2[meta_idx + 0] = static_cast<uint8_t>((frame_offset >> 8) & 0xFF);
        }

        appended_tile_count += frame_count * frame_tiles;
    }

    if (base_tile_count > 0) {
        if (m_anim_meta_tex_id) glDeleteTextures(1, &m_anim_meta_tex_id);
        glGenTextures(1, &m_anim_meta_tex_id); glBindTexture(GL_TEXTURE_2D, m_anim_meta_tex_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)base_tile_count, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, anim_meta_1.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        if (m_anim_meta2_tex_id) glDeleteTextures(1, &m_anim_meta2_tex_id);
        glGenTextures(1, &m_anim_meta2_tex_id); glBindTexture(GL_TEXTURE_2D, m_anim_meta2_tex_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)base_tile_count, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, anim_meta_2.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    if (m_tileset_tex_id) glDeleteTextures(1, &m_tileset_tex_id);
    glGenTextures(1, &m_tileset_tex_id); glBindTexture(GL_TEXTURE_2D, m_tileset_tex_id);
    m_tileset_tex_rows = static_cast<int>(ts_data.size() / 8);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 8, m_tileset_tex_rows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ts_data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<uint8_t> rpal_data(16 * 4, 0);
    for(int i=0; i<16; ++i) {
        uint32_t c = pal->getRGBA(i);
        rpal_data[i*4 + 0] = (c >> 16) & 0xFF; rpal_data[i*4 + 1] = (c >> 8) & 0xFF;
        rpal_data[i*4 + 2] = (c >> 0) & 0xFF; rpal_data[i*4 + 3] = (c >> 24) & 0xFF;
    }
    if (m_room_pal_tex_id) glDeleteTextures(1, &m_room_pal_tex_id);
    glGenTextures(1, &m_room_pal_tex_id); glBindTexture(GL_TEXTURE_2D, m_room_pal_tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, rpal_data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void MapRenderer::BindMapShaderState(GLuint map_tex_id, float map_w, float map_h) {
    glUseProgram(m_map_shader_program);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, map_tex_id); glUniform1i(glGetUniformLocation(m_map_shader_program, "u_map"), 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_blockset_tex_id); glUniform1i(glGetUniformLocation(m_map_shader_program, "u_blockset"), 1);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m_tileset_tex_id); glUniform1i(glGetUniformLocation(m_map_shader_program, "u_tileset"), 2);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, m_room_pal_tex_id); glUniform1i(glGetUniformLocation(m_map_shader_program, "u_palette"), 3);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, m_anim_meta_tex_id); glUniform1i(glGetUniformLocation(m_map_shader_program, "u_anim_metadata1"), 4);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, m_anim_meta2_tex_id); glUniform1i(glGetUniformLocation(m_map_shader_program, "u_anim_metadata2"), 5);
    glUniform2f(glGetUniformLocation(m_map_shader_program, "u_map_size"), map_w, map_h);
    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_tileset_height"), m_tileset_tex_rows);
    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_require_priority"), 0);
    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_highlight_priority"), 0);
    {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> dt = now - m_start_time;
        glUniform1f(glGetUniformLocation(m_map_shader_program, "u_time"), dt.count());
    }
    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_num_tiles"), (int)(m_gd->GetRoomData()->GetTilesetForRoom(m_current_room)->GetData()->GetTileCount()));
    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_num_blocks"), (int)(m_gd->GetRoomData()->GetCombinedBlocksetForRoom(m_current_room)->size()));
    glUniform1f(glGetUniformLocation(m_map_shader_program, "u_alpha"), 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void MapRenderer::DrawRoomQuads(float x_offset) const {
    // Every block in the room grid is drawn as a 32x32 square quad. Map moves
    // of one block translate to (32, 16) / (-32, 16) pixels, so the quads
    // overlap; the fragment shader maps them into isometric diamond shapes
    // using the map coordinates passed as texture coordinates.
    glBegin(GL_QUADS);
    for (int y = 0; y < m_room_h; ++y) {
        for (int x = 0; x < m_room_w; ++x) {
            float px = 32.0f * x - 32.0f * y + 512.0f + x_offset;
            float py = 16.0f * x + 16.0f * y + 100.0f;
            glTexCoord2f((float)x, (float)y); glVertex2f(px, py);
            glTexCoord2f((float)x + 1.0f, (float)y); glVertex2f(px + 32.0f, py);
            glTexCoord2f((float)x + 1.0f, (float)y + 1.0f); glVertex2f(px + 32.0f, py + 32.0f);
            glTexCoord2f((float)x, (float)y + 1.0f); glVertex2f(px, py + 32.0f);
        }
    }
    glEnd();
}

void MapRenderer::UploadBlockPreviewTexture(uint16_t block_id) {
    if (!m_editor_preview_map_tex_id) {
        glGenTextures(1, &m_editor_preview_map_tex_id);
    }

    uint8_t preview_rgba[4] = {
        static_cast<uint8_t>(block_id & 0xFF),
        static_cast<uint8_t>((block_id >> 8) & 0xFF),
        0,
        0
    };
    glBindTexture(GL_TEXTURE_2D, m_editor_preview_map_tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, preview_rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void MapRenderer::Render() {
    if (m_bg_opacity <= 0.0f && m_fg_opacity <= 0.0f) {
        return;
    }
    if (!m_map_tex_id || !m_map_shader_program) {
        return;
    }

    BindMapShaderState(m_map_tex_id, (float)m_room_w, (float)m_room_h);

    if (m_bg_opacity > 0.0f) {
        glUniform1f(glGetUniformLocation(m_map_shader_program, "u_alpha"), m_bg_opacity);
        DrawRoomQuads(0.0f);
    }

    // The foreground layer is offset by -32px in X (1 block) in Landstalker's engine.
    if (m_fg_map_tex_id && m_fg_opacity > 0.0f) {
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_fg_map_tex_id);
        glUniform1f(glGetUniformLocation(m_map_shader_program, "u_alpha"), m_fg_opacity);
        DrawRoomQuads(-32.0f);
    }

    glUseProgram(0);
}

void MapRenderer::RenderBackgroundOnly() {
    RenderBackgroundWithOpacity(1.0f);
}

void MapRenderer::RenderBackgroundWithOpacity(float alpha) {
    if (!m_map_tex_id || !m_map_shader_program) {
        return;
    }

    BindMapShaderState(m_map_tex_id, (float)m_room_w, (float)m_room_h);
    glUniform1f(glGetUniformLocation(m_map_shader_program, "u_alpha"), std::clamp(alpha, 0.0f, 1.0f));
    DrawRoomQuads(0.0f);
    glUseProgram(0);
}

void MapRenderer::RenderForegroundOnly() {
    RenderForegroundWithOpacity(1.0f);
}

void MapRenderer::RenderForegroundWithOpacity(float alpha) {
    if (!m_fg_map_tex_id || !m_map_shader_program) {
        return;
    }

    BindMapShaderState(m_fg_map_tex_id, (float)m_room_w, (float)m_room_h);
    glUniform1f(glGetUniformLocation(m_map_shader_program, "u_alpha"), std::clamp(alpha, 0.0f, 1.0f));
    DrawRoomQuads(-32.0f);
    glUseProgram(0);
}

void MapRenderer::RenderPriorityHighlight(Tilemap3D::Layer layer, float r, float g, float b, float alpha) {
    GLuint map_tex_id = layer == Tilemap3D::Layer::FG ? m_fg_map_tex_id : m_map_tex_id;
    if (!map_tex_id || !m_map_shader_program) {
        return;
    }

    BindMapShaderState(map_tex_id, (float)m_room_w, (float)m_room_h);
    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_require_priority"), 1);
    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_highlight_priority"), 1);
    glUniform4f(glGetUniformLocation(m_map_shader_program, "u_highlight_color"), r, g, b, alpha);
    DrawRoomQuads(layer == Tilemap3D::Layer::FG ? -32.0f : 0.0f);

    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_require_priority"), 0);
    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_highlight_priority"), 0);
    glUseProgram(0);
}

void MapRenderer::RenderBlockGhost(uint16_t block_id, int block_x, int block_y, float alpha, Tilemap3D::Layer layer) {
    if (!m_map_shader_program || block_x < 0 || block_y < 0 || block_x >= m_room_w || block_y >= m_room_h) {
        return;
    }

    UploadBlockPreviewTexture(block_id);

    float layer_x_offset = layer == Tilemap3D::Layer::FG ? -32.0f : 0.0f;
    float px = 32.0f * block_x - 32.0f * block_y + 512.0f + layer_x_offset;
    float py = 16.0f * block_x + 16.0f * block_y + 100.0f;

    BindMapShaderState(m_editor_preview_map_tex_id, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(m_map_shader_program, "u_alpha"), alpha);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(px, py);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(px + 32.0f, py);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(px + 32.0f, py + 32.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(px, py + 32.0f);
    glEnd();

    glUseProgram(0);
}

void MapRenderer::RenderBlockPriorityHighlight(uint16_t block_id, int block_x, int block_y, Tilemap3D::Layer layer, float r, float g, float b, float alpha) {
    if (!m_map_shader_program || block_x < 0 || block_y < 0 || block_x >= m_room_w || block_y >= m_room_h) {
        return;
    }

    UploadBlockPreviewTexture(block_id);

    float layer_x_offset = layer == Tilemap3D::Layer::FG ? -32.0f : 0.0f;
    float px = 32.0f * block_x - 32.0f * block_y + 512.0f + layer_x_offset;
    float py = 16.0f * block_x + 16.0f * block_y + 100.0f;

    BindMapShaderState(m_editor_preview_map_tex_id, 1.0f, 1.0f);
    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_require_priority"), 1);
    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_highlight_priority"), 1);
    glUniform4f(glGetUniformLocation(m_map_shader_program, "u_highlight_color"), r, g, b, alpha);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(px, py);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(px + 32.0f, py);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(px + 32.0f, py + 32.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(px, py + 32.0f);
    glEnd();

    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_require_priority"), 0);
    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_highlight_priority"), 0);
    glUseProgram(0);
}

void MapRenderer::BuildForegroundCoverageStencil() {
    if (!m_fg_map_tex_id || !m_map_shader_program) {
        return;
    }

    BindMapShaderState(m_fg_map_tex_id, (float)m_room_w, (float)m_room_h);
    glUniform1i(glGetUniformLocation(m_map_shader_program, "u_require_priority"), 1);

    // This pass writes only to stencil (no color) for FG-priority pixels.
    // Later passes can test bit 0x04 to know a sprite fragment is behind FG.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0x04);
    glStencilFunc(GL_ALWAYS, 0x04, 0x04);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    DrawRoomQuads(-32.0f);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);
    glUseProgram(0);
}

void MapRenderer::InitShaders() {
    m_map_shader_program = CreateShader("map.vert", GpuShaders::kMapVertex, "map.frag", GpuShaders::kMapFragment);
}

GLuint MapRenderer::CreateShader(const char* vs_name, const char* vs_src, const char* fs_name, const char* fs_src) {
    // Standard OpenGL shader lifecycle: compile vertex+fragment shaders,
    // then link them into a single executable GPU program object.
    GLuint vs = glCreateShader(GL_VERTEX_SHADER); glShaderSource(vs, 1, &vs_src, nullptr); glCompileShader(vs);
    GLint status; glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) { char log[512]; glGetShaderInfoLog(vs, 512, nullptr, log); std::cerr << "VS Error (" << vs_name << "): " << log << std::endl; }
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fs, 1, &fs_src, nullptr); glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) { char log[512]; glGetShaderInfoLog(fs, 512, nullptr, log); std::cerr << "FS Error (" << fs_name << "): " << log << std::endl; }
    GLuint prog = glCreateProgram(); glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) { char log[512]; glGetProgramInfoLog(prog, 512, nullptr, log); std::cerr << "Link Error: " << log << std::endl; }
    return prog;
}
