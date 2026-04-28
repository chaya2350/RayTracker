#pragma once
#include "vec3.h"
#include <random>
#include <limits>

// ── Constants ─────────────────────────────────────────────────────────────────
constexpr double infinity = std::numeric_limits<double>::infinity();

// ── Random helpers ────────────────────────────────────────────────────────────
inline double randomDouble() {
    static std::mt19937 rng(42);
    static std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng);
}

inline double randomDouble(double lo, double hi) {
    return lo + (hi - lo) * randomDouble();
}

inline Vec3 randomInUnitSphere() {
    while (true) {
        Vec3 p = { randomDouble(-1,1), randomDouble(-1,1), randomDouble(-1,1) };
        if (p.lengthSquared() < 1.0) return p;
    }
}

inline Vec3 randomUnitVector() { return normalize(randomInUnitSphere()); }
