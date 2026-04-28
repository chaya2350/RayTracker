#pragma once
#include "ray.h"
#include <memory>

class Material; // forward declaration — material.h includes us

// Holds all info about a ray-object intersection
struct HitRecord {
    Point3 point;
    Vec3   normal;
    std::shared_ptr<Material> material;
    double t = 0.0;
    bool   frontFace = true;

    // Ensure normal always points against the incoming ray
    void setFaceNormal(const Ray& ray, const Vec3& outwardNormal) {
        frontFace = dot(ray.direction, outwardNormal) < 0;
        normal    = frontFace ? outwardNormal : -outwardNormal;
    }
};

// Abstract base — everything that a ray can hit
class Hittable {
public:
    virtual ~Hittable() = default;
    virtual bool hit(const Ray& ray, double tMin, double tMax, HitRecord& rec) const = 0;
};
