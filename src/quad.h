#pragma once
#include "hittable.h"
#include "material.h"

// A parallelogram (flat quad) in 3D space.
// Defined by a corner point Q and two edge vectors u and v.
//
//   Q+v ──────── Q+u+v
//    |               |
//    |               |
//    Q  ──────────  Q+u
//
// Used for walls, ceilings, floors, and the area light in Cornell Box.
class Quad : public Hittable {
public:
    Quad(const Point3& Q, const Vec3& u, const Vec3& v,
         std::shared_ptr<Material> material)
        : Q(Q), u(u), v(v), material(std::move(material))
    {
        Vec3 n = cross(u, v);     // normal direction (not unit length yet)
        normal = normalize(n);
        D      = dot(normal, Q);  // plane equation: normal·P = D
        w      = n / dot(n, n);   // used to compute barycentric coords
    }

    bool hit(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override {
        double denom = dot(normal, ray.direction);

        // Ray is parallel to the quad's plane — no hit
        if (std::abs(denom) < 1e-8) return false;

        // Distance along the ray to the plane
        double t = (D - dot(normal, ray.origin)) / denom;
        if (t < tMin || t > tMax) return false;

        // Check if the hit point is inside the quad.
        // We project it onto the (u, v) axes using the precomputed w.
        Point3 P       = ray.at(t);
        Vec3   planar  = P - Q;    // vector from corner to hit point
        double alpha   = dot(w, cross(planar, v));  // position along u
        double beta    = dot(w, cross(u, planar));  // position along v

        // alpha and beta must both be in [0,1] to be inside the quad
        if (alpha < 0.0 || alpha > 1.0 || beta < 0.0 || beta > 1.0)
            return false;

        rec.t        = t;
        rec.point    = P;
        rec.material = material;
        rec.setFaceNormal(ray, normal);
        return true;
    }

    bool boundingBox(AABB& outputBox) const override {
        // Build a box that contains all four corners of the quad
        constexpr double eps = 1e-4;
        AABB b1(Q,         Q + u + v);
        AABB b2(Q + u,     Q + v    );
        outputBox = surroundingBox(b1, b2);

        // Pad thin dimensions (a flat horizontal quad has zero height)
        outputBox = AABB(
            outputBox.minimum - Vec3(eps, eps, eps),
            outputBox.maximum + Vec3(eps, eps, eps)
        );
        return true;
    }

private:
    Point3 Q;
    Vec3   u, v;
    Vec3   normal;
    double D;   // plane constant
    Vec3   w;   // w = cross(u,v) / |cross(u,v)|²  — for barycentric coords
    std::shared_ptr<Material> material;
};
