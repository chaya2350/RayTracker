#pragma once
#include "ray.h"
#include <numbers>

class Camera {
public:
    Camera(
        Point3 lookFrom,
        Point3 lookAt,
        Vec3   vup,
        double vfov,          // vertical field-of-view in degrees
        double aspectRatio
    ) {
        double theta = vfov * std::numbers::pi / 180.0;
        double h     = std::tan(theta / 2.0);

        double viewportHeight = 2.0 * h;
        double viewportWidth  = aspectRatio * viewportHeight;

        Vec3 w = normalize(lookFrom - lookAt);
        Vec3 u = normalize(cross(vup, w));
        Vec3 v = cross(w, u);

        origin_     = lookFrom;
        horizontal_ = viewportWidth  * u;
        vertical_   = viewportHeight * v;
        lowerLeft_  = origin_ - horizontal_/2 - vertical_/2 - w;
    }

    // s, t in [0, 1] — normalized screen coordinates
    Ray getRay(double s, double t) const {
        Vec3 dir = lowerLeft_ + s*horizontal_ + t*vertical_ - origin_;
        return {origin_, dir};
    }

private:
    Point3 origin_;
    Vec3   horizontal_;
    Vec3   vertical_;
    Point3 lowerLeft_;
};
