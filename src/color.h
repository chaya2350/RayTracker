#pragma once
#include "vec3.h"
#include <cstdint>
#include <algorithm>

// Write a single gamma-corrected pixel to a PPM stream
inline void writeColor(std::ostream& out, const Color& color, int samplesPerPixel) {
    double scale = 1.0 / samplesPerPixel;

    // Gamma-correct for gamma = 2.0 (sqrt)
    double r = std::sqrt(color.x * scale);
    double g = std::sqrt(color.y * scale);
    double b = std::sqrt(color.z * scale);

    auto clamp = [](double v) -> int {
        return static_cast<int>(256 * std::clamp(v, 0.0, 0.999));
    };

    out << clamp(r) << ' ' << clamp(g) << ' ' << clamp(b) << '\n';
}
