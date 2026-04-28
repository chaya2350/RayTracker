#pragma once
#include "hittable.h"
#include "material.h"
#include <cmath>

// ── ConstantMedium — participating media (fog, smoke, clouds) ────────────────
// Wraps any Hittable shape and fills its interior with a uniform volume.
//
// Physics: as a ray travels through the medium, at each infinitesimal step
// there is a probability of hitting a "particle" (droplet, soot grain, etc.).
// This gives an exponential distribution for the free-path length:
//
//   hitDistance = -log(random) / density
//
// High density  → short free path → opaque smoke
// Low density   → long free path  → thin fog
class ConstantMedium : public Hittable {
public:
    ConstantMedium(std::shared_ptr<Hittable> boundary,
                   double density,
                   const Color& color)
        : boundary(std::move(boundary))
        , negInvDensity(-1.0 / density)
        , phaseFunction(std::make_shared<Isotropic>(color))
    {}

    bool hit(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override {
        // Step 1: find where the ray ENTERS the boundary shape
        HitRecord rec1;
        if (!boundary->hit(ray, -infinity, infinity, rec1)) return false;

        // Step 2: find where the ray EXITS the boundary shape
        HitRecord rec2;
        if (!boundary->hit(ray, rec1.t + 1e-4, infinity, rec2)) return false;

        // Clamp to the valid ray interval
        double t1 = std::max(rec1.t, tMin);
        double t2 = std::min(rec2.t, tMax);
        if (t1 >= t2) return false;
        t1 = std::max(t1, 0.0);

        // Step 3: how far does the ray travel inside the volume?
        double rayLength         = ray.direction.length();
        double distInsideBoundary = (t2 - t1) * rayLength;

        // Step 4: sample a random free-path length (exponential distribution)
        double hitDist = negInvDensity * std::log(randomDouble());

        // If the sampled distance is longer than the volume — ray passes through
        if (hitDist > distInsideBoundary) return false;

        // Step 5: the ray hits a particle at this point
        rec.t        = t1 + hitDist / rayLength;
        rec.point    = ray.at(rec.t);
        rec.normal   = Vec3(1, 0, 0);  // arbitrary — Isotropic ignores it
        rec.frontFace = true;
        rec.material = phaseFunction;
        return true;
    }

    bool boundingBox(AABB& outputBox) const override {
        return boundary->boundingBox(outputBox);
    }

private:
    std::shared_ptr<Hittable>  boundary;
    double                     negInvDensity;
    std::shared_ptr<Material>  phaseFunction;
};
