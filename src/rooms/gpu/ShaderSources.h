#ifndef SHADER_SOURCES_H
#define SHADER_SOURCES_H

namespace GpuShaders {

inline constexpr const char* kSpriteVertex = R"SHADER(
#version 120
void main() {
    gl_Position = ftransform();
    gl_TexCoord[0] = gl_MultiTexCoord0;
}
)SHADER";

inline constexpr const char* kSpriteFragment = R"SHADER(
#version 120
uniform sampler2D u_atlas;
uniform sampler2D u_palettes;
uniform float u_pal_row;
uniform float u_alpha;
void main() {
    float idx = texture2D(u_atlas, gl_TexCoord[0].xy).r * 255.0;
    if (idx < 0.5) discard;
    gl_FragColor = texture2D(u_palettes, vec2((idx + 0.5)/16.0, u_pal_row));
    gl_FragColor.a *= u_alpha;
}
)SHADER";

inline constexpr const char* kMapVertex = R"SHADER(
#version 120
void main() {
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
    gl_TexCoord[0] = gl_MultiTexCoord0;
}
)SHADER";

inline constexpr const char* kMapFragment = R"SHADER(
#version 120

// u_map: Texture containing the 2D grid of block IDs (R=low byte, G=high byte)
uniform sampler2D u_map;
// u_blockset: Texture containing tile indices for each 16x16 block (2x2 tiles)
uniform sampler2D u_blockset;
// u_tileset: Large atlas containing the raw 8x8 pixel data for all tiles
uniform sampler2D u_tileset;
uniform sampler2D u_anim_metadata1;
uniform sampler2D u_anim_metadata2;
// u_palette: The 16-color palette for the current room
uniform sampler2D u_palette;

// Global dimensions used to normalize texture coordinates
uniform vec2 u_map_size;
uniform int u_num_tiles;
uniform int u_num_blocks;
uniform int u_tileset_height;
uniform float u_time;
uniform float u_alpha;
uniform int u_require_priority;
uniform int u_highlight_priority;
uniform vec4 u_highlight_color;

void main() {
    // map_pos is the coordinate in "blocks" (e.g. 5.5 is halfway through the 6th block)
    vec2 map_pos = gl_TexCoord[0].xy;

    // 1. Identify which block we are in (discrete integer index)
    vec2 block_idx = clamp(floor(map_pos), vec2(0.0), u_map_size - 1.0);

    // 2. Identify where we are INSIDE that block (0.0 to 0.999)
    // We clamp to 0.999 to prevent precision errors from wrapping at the edges
    vec2 fract_pos = clamp(fract(map_pos), 0.0, 0.999);

    // Each block in Landstalker is 16x16 pixels
    vec2 pix_in_block = fract_pos * 16.0;

    // 3. Fetch the Block ID from the map texture
    // The Map texture is essentially a 2D array of 16-bit integers
    vec4 b_data = texture2D(u_map, (block_idx + 0.5) / u_map_size);
    float block_id = floor(b_data.r * 255.5) + floor(b_data.g * 255.5) * 256.0;

    // 4. Identify which of the 4 tiles (2x2) in the block we are hitting
    // tx, ty are 0 or 1
    float tx = clamp(floor(pix_in_block.x / 8.0), 0.0, 1.0);
    float ty = clamp(floor(pix_in_block.y / 8.0), 0.0, 1.0);

    // 5. Fetch the Tile Index (and flip flags) from the blockset texture
    // The Blockset texture is 2 pixels wide (tx) and (u_num_blocks * 2) pixels high (block_id * 2 + ty)
    vec4 t_data = texture2D(u_blockset, vec2((tx + 0.5)/2.0, (block_id * 2.0 + ty + 0.5)/float(u_num_blocks * 2)));
    float val = floor(t_data.r * 255.5) + floor(t_data.g * 255.5) * 256.0;

    // Lower 11 bits are the tile index
    float t_idx = mod(val, 2048.0);
    // Upper bits contain horizontal and vertical flip flags
    bool hflip = mod(floor(val / 2048.0), 2.0) > 0.5;
    bool vflip = mod(floor(val / 4096.0), 2.0) > 0.5;
    bool priority = mod(floor(val / 32768.0), 2.0) > 0.5;
    if (u_require_priority != 0 && !priority) discard;

    // 6. Identify the specific 8x8 pixel coordinate within the tile
    vec2 pix_in_tile = floor(mod(pix_in_block, 8.0));

    // Apply flipping if requested by the block data
    if (hflip) pix_in_tile.x = 7.0 - pix_in_tile.x;
    if (vflip) pix_in_tile.y = 7.0 - pix_in_tile.y;

    // 7. Decode animation metadata for this tile, if present
    vec2 meta_uv = vec2((t_idx + 0.5) / float(u_num_tiles), 0.5);
    vec4 anim_info1 = texture2D(u_anim_metadata1, meta_uv);
    vec4 anim_info2 = texture2D(u_anim_metadata2, meta_uv);
    float anim_frame_count = floor(anim_info1.r * 255.5);
    float anim_speed = floor(anim_info1.g * 255.5);
    float anim_frame_step = floor(anim_info1.b * 255.5);
    float anim_offset_low = floor(anim_info1.a * 255.5);
    float anim_offset_high = floor(anim_info2.r * 255.5);
    float anim_offset = anim_offset_low + anim_offset_high * 256.0;

    float actual_tile_idx = t_idx;
    if (anim_frame_count > 0.5 && anim_frame_step > 0.5) {
        float fps = anim_speed > 0.5 ? (60.0 / anim_speed) : 1.0;
        float current_frame = mod(floor(u_time * fps), anim_frame_count);
        actual_tile_idx = anim_offset + current_frame * anim_frame_step;
    }

    // 8. Fetch the Palette Index for this pixel from the tileset atlas
    // Tileset is 8 pixels wide and stored across u_tileset_height rows
    float c_idx = texture2D(u_tileset, vec2((pix_in_tile.x + 0.5)/8.0, (actual_tile_idx * 8.0 + pix_in_tile.y + 0.5)/float(u_tileset_height))).r * 255.0;

    // 9. Handle Transparency
    // In Landstalker, color index 0 is always transparent
    if (c_idx < 0.5) discard;

    if (u_highlight_priority != 0) {
        gl_FragColor = u_highlight_color;
        gl_FragColor.a *= u_alpha;
        return;
    }

    // 9. Final Color Lookup from the 1D room palette
    gl_FragColor = texture2D(u_palette, vec2((c_idx + 0.5)/16.0, 0.5));
    gl_FragColor.a *= u_alpha;
}
)SHADER";

} // namespace GpuShaders

#endif // SHADER_SOURCES_H
