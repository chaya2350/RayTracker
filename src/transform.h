#pragma once
#include "hittable.h"
#include "aabb.h"
#include <cmath>

// ── Translate ─────────────────────────────────────────────────────────────────
// Wraps any Hittable and shifts it by an offset vector.
// Trick: instead of moving the geometry, we move the ray in the opposite
// direction, test the hit, then shift the result back.
class Translate : public Hittable {
public:
    Translate(std::shared_ptr<Hittable> object, const Vec3& offset)
        : object(std::move(object)), offset(offset) {}

    bool hit(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override {
        // Move ray backward by offset — equivalent to moving object forward
        Ray movedRay(ray.origin - offset, ray.direction);

        if (!object->hit(movedRay, tMin, tMax, rec))
            return false;

        // Shift the hit point back to world space
        rec.point += offset;
        rec.setFaceNormal(movedRay, rec.normal);
        return true;
    }

    bool boundingBox(AABB& outputBox) const override {
        if (!object->boundingBox(outputBox)) return false;
        outputBox = AABB(outputBox.minimum + offset,
                         outputBox.maximum + offset);
        return true;
    }

private:
    std::shared_ptr<Hittable> object;
    Vec3 offset;
};


// ── RotateY ───────────────────────────────────────────────────────────────────
// Wraps any Hittable and rotates it around the Y axis by `angleDeg` degrees.
// Same trick: rotate the ray into object space, test, rotate result back.
//
//   [ cos θ   0   sin θ ] [ x ]
//   [   0     1     0   ] [ y ]
//   [-sin θ   0   cos θ ] [ z ]
class RotateY : public Hittable {
public:
    RotateY(std::shared_ptr<Hittable> object, double angleDeg)
        : object(std::move(object))
    {
        double radians = angleDeg * std::numbers::pi / 180.0;
        sinT = std::sin(radians);
        cosT = std::cos(radians);

        // Compute the rotated bounding box by rotating all 8 corners
        AABB bbox;
        this->object->boundingBox(bbox);  // use this->object — the parameter was moved!

        Point3 mn( infinity,  infinity,  infinity);
        Point3 mx(-infinity, -infinity, -infinity);

        for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
        for (int k = 0; k < 2; ++k) {
            double x = i ? bbox.maximum.x : bbox.minimum.x;
            double y = j ? bbox.maximum.y : bbox.minimum.y;
            double z = k ? bbox.maximum.z : bbox.minimum.z;

            double rx =  cosT * x + sinT * z;
            double rz = -sinT * x + cosT * z;

            mn.x = std::min(mn.x, rx);  mn.y = std::min(mn.y, y);  mn.z = std::min(mn.z, rz);
            mx.x = std::max(mx.x, rx);  mx.y = std::max(mx.y, y);  mx.z = std::max(mx.z, rz);
        }
        rotatedBox = AABB(mn, mx);
    }

    bool hit(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override {
        // Rotate ray into object space (inverse rotation = transpose = -θ)
        //   x' =  cos θ * x - sin θ * z
        //   z' =  sin θ * x + cos θ * z
        Vec3 orig(
             cosT * ray.origin.x - sinT * ray.origin.z,
             ray.origin.y,
             sinT * ray.origin.x + cosT * ray.origin.z
        );
        Vec3 dir(
             cosT * ray.direction.x - sinT * ray.direction.z,
             ray.direction.y,
             sinT * ray.direction.x + cosT * ray.direction.z
        );

        Ray rotatedRay(orig, dir);
        if (!object->hit(rotatedRay, tMin, tMax, rec))
            return false;

        // Rotate hit point and normal back to world space (forward rotation)
        rec.point = Vec3(
             cosT * rec.point.x + sinT * rec.point.z,
             rec.point.y,
            -sinT * rec.point.x + cosT * rec.point.z
        );
        Vec3 worldNormal(
             cosT * rec.normal.x + sinT * rec.normal.z,
             rec.normal.y,
            -sinT * rec.normal.x + cosT * rec.normal.z
        );
        rec.setFaceNormal(rotatedRay, worldNormal);
        return true;
    }

    bool boundingBox(AABB& outputBox) const override {
        outputBox = rotatedBox;
        return true;
    }

private:
    std::shared_ptr<Hittable> object;
    double sinT, cosT;
    AABB   rotatedBox;
};
