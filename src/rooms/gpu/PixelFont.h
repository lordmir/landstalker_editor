#pragma once

#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <string>

#include "GLLoader.h"

namespace PixelFont {
constexpr int GLYPH_WIDTH = 5;
constexpr int GLYPH_HEIGHT = 7;
// Horizontal advance between glyphs, in pixels at scale 1.
constexpr float GLYPH_ADVANCE = 6.0f;

inline const std::array<uint8_t, GLYPH_HEIGHT>* Glyph(char c)
{
    switch (std::toupper(static_cast<unsigned char>(c))) {
        case '0': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
            return &g;
        }
        case '1': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
            return &g;
        }
        case '2': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
            return &g;
        }
        case '3': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
            return &g;
        }
        case '4': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
            return &g;
        }
        case '5': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
            return &g;
        }
        case '6': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E};
            return &g;
        }
        case '7': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
            return &g;
        }
        case '8': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
            return &g;
        }
        case '9': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E};
            return &g;
        }
        case 'A': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
            return &g;
        }
        case 'B': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
            return &g;
        }
        case 'C': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F};
            return &g;
        }
        case 'D': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
            return &g;
        }
        case 'E': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
            return &g;
        }
        case 'F': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
            return &g;
        }
        case 'G': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x0F, 0x10, 0x10, 0x13, 0x11, 0x11, 0x0E};
            return &g;
        }
        case 'H': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
            return &g;
        }
        case 'I': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F};
            return &g;
        }
        case 'J': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E};
            return &g;
        }
        case 'K': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
            return &g;
        }
        case 'L': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
            return &g;
        }
        case 'M': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
            return &g;
        }
        case 'N': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
            return &g;
        }
        case 'O': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
            return &g;
        }
        case 'P': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
            return &g;
        }
        case 'Q': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
            return &g;
        }
        case 'R': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
            return &g;
        }
        case 'S': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
            return &g;
        }
        case 'T': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
            return &g;
        }
        case 'U': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
            return &g;
        }
        case 'V': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
            return &g;
        }
        case 'W': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A};
            return &g;
        }
        case 'X': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
            return &g;
        }
        case 'Y': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
            return &g;
        }
        case 'Z': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};
            return &g;
        }
        case ':': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00};
            return &g;
        }
        case '.': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
            return &g;
        }
        case '-': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00};
            return &g;
        }
        case '/': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10};
            return &g;
        }
        case '(': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02};
            return &g;
        }
        case ')': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08};
            return &g;
        }
        case ',': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08};
            return &g;
        }
        case '\'': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00};
            return &g;
        }
        case '&': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x06, 0x09, 0x0A, 0x04, 0x0B, 0x11, 0x0E};
            return &g;
        }
        case ' ': {
            static constexpr std::array<uint8_t, GLYPH_HEIGHT> g = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            return &g;
        }
        default:
            return nullptr;
    }
}

// Draws a single glyph as immediate-mode quads using the current GL color.
inline void DrawOverlayGlyph(char c, float x, float y, float scale)
{
    const auto* glyph = Glyph(c);
    if (!glyph) {
        return;
    }

    glBegin(GL_QUADS);
    for (int row = 0; row < GLYPH_HEIGHT; ++row) {
        uint8_t bits = (*glyph)[row];
        for (int col = 0; col < GLYPH_WIDTH; ++col) {
            uint8_t mask = static_cast<uint8_t>(1u << (GLYPH_WIDTH - 1 - col));
            if ((bits & mask) == 0) {
                continue;
            }

            float px = x + float(col) * scale;
            float py = y + float(row) * scale;
            glVertex2f(px, py);
            glVertex2f(px + scale, py);
            glVertex2f(px + scale, py + scale);
            glVertex2f(px, py + scale);
        }
    }
    glEnd();
}

// Draws a text run using the current GL color. Coordinates are rounded to
// whole pixels so the bitmap font stays crisp.
inline void DrawOverlayText(const std::string& text, float x, float y, float scale)
{
    x = std::round(x);
    y = std::round(y);
    for (std::size_t i = 0; i < text.size(); ++i) {
        DrawOverlayGlyph(text[i], x + float(i) * GLYPH_ADVANCE * scale, y, scale);
    }
}

// Formats a 16-bit value as four uppercase hex digits for overlay display.
inline std::string HexWord(uint16_t value)
{
    constexpr char digits[] = "0123456789ABCDEF";
    std::string out;
    out.push_back(digits[(value >> 12) & 0x0F]);
    out.push_back(digits[(value >> 8) & 0x0F]);
    out.push_back(digits[(value >> 4) & 0x0F]);
    out.push_back(digits[value & 0x0F]);
    return out;
}
} // namespace PixelFont
