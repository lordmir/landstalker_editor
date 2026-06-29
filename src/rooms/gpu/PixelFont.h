#pragma once

#include <array>
#include <cctype>
#include <cstdint>

namespace PixelFont {
constexpr int kGlyphWidth = 5;
constexpr int kGlyphHeight = 7;

inline const std::array<uint8_t, kGlyphHeight>* Glyph(char c)
{
    switch (std::toupper(static_cast<unsigned char>(c))) {
        case '0': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
            return &g;
        }
        case '1': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
            return &g;
        }
        case '2': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
            return &g;
        }
        case '3': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
            return &g;
        }
        case '4': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
            return &g;
        }
        case '5': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
            return &g;
        }
        case '6': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E};
            return &g;
        }
        case '7': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
            return &g;
        }
        case '8': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
            return &g;
        }
        case '9': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E};
            return &g;
        }
        case 'A': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
            return &g;
        }
        case 'B': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
            return &g;
        }
        case 'C': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F};
            return &g;
        }
        case 'D': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
            return &g;
        }
        case 'E': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
            return &g;
        }
        case 'F': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
            return &g;
        }
        case 'G': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x0F, 0x10, 0x10, 0x13, 0x11, 0x11, 0x0E};
            return &g;
        }
        case 'H': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
            return &g;
        }
        case 'I': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F};
            return &g;
        }
        case 'J': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E};
            return &g;
        }
        case 'K': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
            return &g;
        }
        case 'L': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
            return &g;
        }
        case 'M': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
            return &g;
        }
        case 'N': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
            return &g;
        }
        case 'O': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
            return &g;
        }
        case 'P': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
            return &g;
        }
        case 'Q': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
            return &g;
        }
        case 'R': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
            return &g;
        }
        case 'S': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
            return &g;
        }
        case 'T': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
            return &g;
        }
        case 'U': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
            return &g;
        }
        case 'V': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
            return &g;
        }
        case 'W': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A};
            return &g;
        }
        case 'X': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
            return &g;
        }
        case 'Y': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
            return &g;
        }
        case 'Z': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};
            return &g;
        }
        case ':': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00};
            return &g;
        }
        case '.': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
            return &g;
        }
        case '-': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00};
            return &g;
        }
        case '/': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10};
            return &g;
        }
        case '(': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02};
            return &g;
        }
        case ')': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08};
            return &g;
        }
        case ',': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08};
            return &g;
        }
        case '\'': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00};
            return &g;
        }
        case '&': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x06, 0x09, 0x0A, 0x04, 0x0B, 0x11, 0x0E};
            return &g;
        }
        case ' ': {
            static constexpr std::array<uint8_t, kGlyphHeight> g = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            return &g;
        }
        default:
            return nullptr;
    }
}
} // namespace PixelFont
