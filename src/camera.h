#pragma once
#include "ray.h"
#include "utils.h"
#include <numbers>

// Returns a random point inside a unit disk (for lens simulation)
inline Vec3 randomInUnitDisk() {
    while (true) {
        Vec3 p = { randomDouble(-1,1), randomDouble(-1,1), 0 };
        if (p.lengthSquared() < 1.0) return p;
    }
}

class Camera {
public:
    // aperture  : diameter of the virtual lens. 0 = pinhole (no blur)
    // focusDist : distance at which everything is perfectly sharp
    Camera(
        Point3 lookFrom,
        Point3 lookAt,
        Vec3   vup,
        double vfov,
        double aspectRatio,
        double aperture   = 0.0,
        double focusDist  = 1.0
    ) {
        double theta = vfov * std::numbers::pi / 180.0;
        double h     = std::tan(theta / 2.0);

        double viewportHeight = 2.0 * h;
        double viewportWidth  = aspectRatio * viewportHeight;

        w_ = normalize(lookFrom - lookAt);
        u_ = normalize(cross(vup, w_));
        v_ = cross(w_, u_);

        origin_     = lookFrom;
        horizontal_ = focusDist * viewportWidth  * u_;
        vertical_   = focusDist * viewportHeight * v_;
        lowerLeft_  = origin_ - horizontal_/2 - vertical_/2 - focusDist * w_;

        lensRadius_ = aperture / 2.0;
    }

    // Shoot a ray from a random point on the lens through pixel (s, t)
    // With aperture=0 this degenerates to a perfect pinhole camera
    Ray getRay(double s, double t) const {
        Vec3 rd     = lensRadius_ * randomInUnitDisk();
        Vec3 offset = u_ * rd.x + v_ * rd.y;   // random point on lens

        Vec3 dir = lowerLeft_ + s*horizontal_ + t*vertical_
                   - origin_ - offset;
        return { origin_ + offset, dir };
    }

private:
    Point3 origin_;
    Vec3   horizontal_;
    Vec3   vertical_;
    Point3 lowerLeft_;
    Vec3   u_, v_, w_;
    double lensRadius_;
};

