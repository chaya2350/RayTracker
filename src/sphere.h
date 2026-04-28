#pragma once
#include "hittable.h"

class Sphere : public Hittable {
public:
    Point3 center;
    double radius;

    Sphere(const Point3& center, double radius)
        : center(center), radius(std::abs(radius)) {}

    bool hit(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override {
        Vec3 oc = ray.origin - center;

        // Quadratic formula: |ray(t) - center|^2 = radius^2
        double a  = ray.direction.lengthSquared();
        double hb = dot(oc, ray.direction);   // half-b
        double c  = oc.lengthSquared() - radius * radius;
        double discriminant = hb*hb - a*c;

        if (discriminant < 0) return false;

        double sqrtD = std::sqrt(discriminant);

        // Find the nearest root in [tMin, tMax]
        double root = (-hb - sqrtD) / a;
        if (root <= tMin || root >= tMax) {
            root = (-hb + sqrtD) / a;
            if (root <= tMin || root >= tMax) return false;
        }

        rec.t     = root;
        rec.point = ray.at(root);
        rec.setFaceNormal(ray, (rec.point - center) / radius);
        return true;
    }
};
