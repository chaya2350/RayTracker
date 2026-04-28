#pragma once
#include "hittable.h"
#include "utils.h"
#include <cmath>

// ── Abstract base ─────────────────────────────────────────────────────────────
// Every material answers one question:
//   "A ray just hit me — where does it go next, and what color does it pick up?"
class Material {
public:
    virtual ~Material() = default;

    // Returns false if the ray is fully absorbed (no scatter).
    // scattered   = the new ray born at the hit point
    // attenuation = how much each color channel is multiplied (the "tint")
    virtual bool scatter(const Ray& ray, const HitRecord& rec,
                         Color& attenuation, Ray& scattered) const = 0;

    // Light emitted by this material (black by default — most materials don't glow).
    virtual Color emitted() const { return {0, 0, 0}; }
};

// ── Lambertian — matte / diffuse ──────────────────────────────────────────────
// Scatters light in a random direction around the surface normal.
// Cheap and realistic for chalk, concrete, skin, paper.
class Lambertian : public Material {
public:
    Color albedo;   // "albedo" = base color (latin: whiteness)

    explicit Lambertian(const Color& albedo) : albedo(albedo) {}

    bool scatter(const Ray& /*ray*/, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override
    {
        Vec3 scatterDir = rec.normal + randomUnitVector();

        // Catch degenerate scatter direction (when random vector cancels normal)
        if (scatterDir.nearZero()) scatterDir = rec.normal;

        scattered   = Ray(rec.point, scatterDir);
        attenuation = albedo;
        return true;
    }
};

// ── Metal — mirror-like reflection ────────────────────────────────────────────
// Reflects rays like a mirror. fuzz blurs the reflection (0 = perfect chrome).
class Metal : public Material {
public:
    Color  albedo;
    double fuzz;    // 0 = perfect mirror, 1 = very blurry

    Metal(const Color& albedo, double fuzz)
        : albedo(albedo), fuzz(std::clamp(fuzz, 0.0, 1.0)) {}

    bool scatter(const Ray& ray, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override
    {
        // Perfect reflection + small random nudge (fuzz)
        Vec3 reflected = reflect(normalize(ray.direction), rec.normal);
        scattered   = Ray(rec.point, reflected + fuzz * randomInUnitSphere());
        attenuation = albedo;

        // Absorb rays that scatter below the surface
        return dot(scattered.direction, rec.normal) > 0;
    }
};

// ── Dielectric — glass / water / diamond ──────────────────────────────────────
// Bends (refracts) light according to Snell's law.
// At steep angles it reflects instead (Total Internal Reflection).
class Dielectric : public Material {
public:
    double ir;  // Index of Refraction: air=1.0, glass=1.5, diamond=2.4

    explicit Dielectric(double indexOfRefraction) : ir(indexOfRefraction) {}

    bool scatter(const Ray& ray, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override
    {
        attenuation = Color(1.0, 1.0, 1.0);  // glass absorbs nothing

        double refractionRatio = rec.frontFace ? (1.0 / ir) : ir;

        Vec3   unitDir  = normalize(ray.direction);
        double cosTheta = std::min(dot(-unitDir, rec.normal), 1.0);
        double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);

        // Total internal reflection — must reflect, cannot refract
        bool mustReflect = refractionRatio * sinTheta > 1.0;

        Vec3 direction;
        if (mustReflect || schlick(cosTheta, refractionRatio) > randomDouble())
            direction = reflect(unitDir, rec.normal);
        else
            direction = refract(unitDir, rec.normal, refractionRatio);

        scattered = Ray(rec.point, direction);
        return true;
    }

private:
    // Schlick approximation — glass reflects more at glancing angles
    static double schlick(double cosine, double refIdx) {
        double r0 = (1.0 - refIdx) / (1.0 + refIdx);
        r0 *= r0;
        return r0 + (1.0 - r0) * std::pow(1.0 - cosine, 5);
    }
};

// ── DiffuseLight — glowing / emissive surface ─────────────────────────────────
// This material doesn't scatter rays — it only emits light.
// Think: a lamp, the sun, a neon sign.
class DiffuseLight : public Material {
public:
    Color emit;   // color and intensity of the light (values > 1 = bright)

    explicit DiffuseLight(const Color& c) : emit(c) {}

    // Doesn't scatter — light rays hitting a lamp just stop here.
    bool scatter(const Ray&, const HitRecord&, Color&, Ray&) const override {
        return false;
    }

    // But it does emit — this is what illuminates everything else.
    Color emitted() const override { return emit; }
};
