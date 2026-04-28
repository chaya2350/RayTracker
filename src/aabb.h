#pragma once
#include "ray.h"
#include <algorithm>

// Axis-Aligned Bounding Box — a box aligned to the world axes.
// Used to quickly reject rays that can't possibly hit any object inside.
class AABB {
public:
    Point3 minimum;
    Point3 maximum;

    AABB() = default;
    AABB(const Point3& minimum, const Point3& maximum)
        : minimum(minimum), maximum(maximum) {}

    // Slab method: test all 3 axis pairs, check if intervals overlap.
    // If any axis has no overlap -> ray misses the box entirely.
    bool hit(const Ray& ray, double tMin, double tMax) const {
        for (int a = 0; a < 3; ++a) {
            double origin    = (&ray.origin.x)[a];
            double direction = (&ray.direction.x)[a];
            double min       = (&minimum.x)[a];
            double max       = (&maximum.x)[a];

            double invD = 1.0 / direction;
            double t0   = (min - origin) * invD;
            double t1   = (max - origin) * invD;

            if (invD < 0) std::swap(t0, t1);

            tMin = t0 > tMin ? t0 : tMin;
            tMax = t1 < tMax ? t1 : tMax;

            if (tMax <= tMin) return false;
        }
        return true;
    }
};

// Merge two boxes into the smallest box that contains both
inline AABB surroundingBox(const AABB& box0, const AABB& box1) {
    Point3 small(
        std::min(box0.minimum.x, box1.minimum.x),
        std::min(box0.minimum.y, box1.minimum.y),
        std::min(box0.minimum.z, box1.minimum.z)
    );
    Point3 big(
        std::max(box0.maximum.x, box1.maximum.x),
        std::max(box0.maximum.y, box1.maximum.y),
        std::max(box0.maximum.z, box1.maximum.z)
    );
    return { small, big };
}
