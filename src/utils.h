#pragma once
#include "vec3.h"
#include <random>
#include <limits>

// ── Constants ─────────────────────────────────────────────────────────────────
constexpr double infinity = std::numeric_limits<double>::infinity();

// ── Random helpers ────────────────────────────────────────────────────────────
// thread_local: each thread gets its own RNG — no data race, no mutex needed.
inline double randomDouble() {
    thread_local std::mt19937 rng(std::random_device{}());
    thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
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
