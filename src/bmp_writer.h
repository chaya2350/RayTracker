#pragma once
#include "vec3.h"
#include <vector>
#include <fstream>
#include <cstdint>
#include <algorithm>
#include <string>
#include <cmath>

// Clamp + gamma-correct one channel
static uint8_t toUint8(double v, int samplesPerPixel) {
    double scale = 1.0 / samplesPerPixel;
    double g = std::sqrt(v * scale);   // gamma 2.0
    return static_cast<uint8_t>(256 * std::clamp(g, 0.0, 0.999));
}

// Save pixel buffer as a 24-bit BMP (Windows-native format)
inline void saveBMP(const std::string& filename,
                    const std::vector<Color>& pixels,
                    int width, int height, int samplesPerPixel)
{
    // BMP rows must be padded to a multiple of 4 bytes
    int rowSize    = (width * 3 + 3) & ~3;
    int pixelBytes = rowSize * height;

    // ── File header (14 bytes) ────────────────────────────────────────────────
    uint8_t fileHeader[14] = {
        'B','M',                         // signature
        0,0,0,0,                         // file size (filled below)
        0,0, 0,0,                        // reserved
        54,0,0,0                         // pixel data offset (14 + 40)
    };
    uint32_t fileSize = 54 + pixelBytes;
    fileHeader[2] = fileSize & 0xFF;
    fileHeader[3] = (fileSize >> 8)  & 0xFF;
    fileHeader[4] = (fileSize >> 16) & 0xFF;
    fileHeader[5] = (fileSize >> 24) & 0xFF;

    // ── DIB header (40 bytes, BITMAPINFOHEADER) ───────────────────────────────
    uint8_t dibHeader[40] = {};
    auto writeU32 = [&](uint8_t* p, uint32_t v) {
        p[0] = v & 0xFF; p[1] = (v>>8)&0xFF; p[2] = (v>>16)&0xFF; p[3] = (v>>24)&0xFF;
    };
    auto writeS32 = [&](uint8_t* p, int32_t v) { writeU32(p, static_cast<uint32_t>(v)); };
    auto writeU16 = [&](uint8_t* p, uint16_t v) { p[0] = v & 0xFF; p[1] = (v>>8)&0xFF; };

    writeU32(dibHeader +  0, 40);          // header size
    writeS32(dibHeader +  4, width);
    writeS32(dibHeader +  8, height);      // positive = bottom-up
    writeU16(dibHeader + 12, 1);           // color planes
    writeU16(dibHeader + 14, 24);          // bits per pixel
    writeU32(dibHeader + 16, 0);           // compression (none)
    writeU32(dibHeader + 20, pixelBytes);
    writeS32(dibHeader + 24, 2835);        // ~72 DPI
    writeS32(dibHeader + 28, 2835);
    writeU32(dibHeader + 32, 0);
    writeU32(dibHeader + 36, 0);

    // ── Write file ────────────────────────────────────────────────────────────
    std::ofstream out(filename, std::ios::binary);
    out.write(reinterpret_cast<char*>(fileHeader), sizeof(fileHeader));
    out.write(reinterpret_cast<char*>(dibHeader),  sizeof(dibHeader));

    std::vector<uint8_t> row(rowSize, 0);

    // BMP stores rows bottom-to-top, pixels in BGR order
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            const Color& c = pixels[j * width + i];
            row[i * 3 + 0] = toUint8(c.z, samplesPerPixel);  // B
            row[i * 3 + 1] = toUint8(c.y, samplesPerPixel);  // G
            row[i * 3 + 2] = toUint8(c.x, samplesPerPixel);  // R
        }
        out.write(reinterpret_cast<char*>(row.data()), rowSize);
    }
}
