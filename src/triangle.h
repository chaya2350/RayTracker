#pragma once
#include "hittable.h"
#include "material.h"

// A single triangle defined by three vertices.
// Uses the Möller–Trumbore algorithm for ray intersection —
// one of the most elegant pieces of math in computer graphics.
class Triangle : public Hittable {
public:
    Point3 v0, v1, v2;
    std::shared_ptr<Material> material;

    Triangle(const Point3& v0, const Point3& v1, const Point3& v2,
             std::shared_ptr<Material> material)
        : v0(v0), v1(v1), v2(v2), material(std::move(material)) {}

    bool hit(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override {
        // Edge vectors from v0
        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;

        // Möller–Trumbore: solve ray = v0 + u*edge1 + v*edge2
        Vec3   h = cross(ray.direction, edge2);
        double a = dot(edge1, h);

        // Ray is parallel to triangle — no hit
        if (std::abs(a) < 1e-8) return false;

        double f = 1.0 / a;
        Vec3   s = ray.origin - v0;
        double u = f * dot(s, h);

        // u outside [0,1] — miss
        if (u < 0.0 || u > 1.0) return false;

        Vec3   q = cross(s, edge1);
        double v = f * dot(ray.direction, q);

        // v outside [0,1] or u+v > 1 — miss
        if (v < 0.0 || u + v > 1.0) return false;

        // t = distance along ray to intersection
        double t = f * dot(edge2, q);
        if (t < tMin || t > tMax) return false;

        // Hit! Fill in the record
        rec.t     = t;
        rec.point = ray.at(t);
        rec.material = material;

        // Face normal = cross product of edges (right-hand rule)
        Vec3 outwardNormal = normalize(cross(edge1, edge2));
        rec.setFaceNormal(ray, outwardNormal);

        return true;
    }

    bool boundingBox(AABB& outputBox) const override {
        // Small epsilon padding — degenerate triangles (flat on one axis)
        // would produce zero-thickness boxes that fail intersection tests
        constexpr double eps = 1e-4;
        outputBox = AABB(
            Point3(std::min({v0.x, v1.x, v2.x}) - eps,
                   std::min({v0.y, v1.y, v2.y}) - eps,
                   std::min({v0.z, v1.z, v2.z}) - eps),
            Point3(std::max({v0.x, v1.x, v2.x}) + eps,
                   std::max({v0.y, v1.y, v2.y}) + eps,
                   std::max({v0.z, v1.z, v2.z}) + eps)
        );
        return true;
    }
};
