#include "vec3.h"
#include "ray.h"
#include "camera.h"
#include "hittable_list.h"
#include "sphere.h"
#include "bmp_writer.h"

#include <iostream>
#include <vector>
#include <random>
#include <limits>

// ── Random helpers ────────────────────────────────────────────────────────────
static std::mt19937 rng(42);
static std::uniform_real_distribution<double> dist01(0.0, 1.0);

double randomDouble()              { return dist01(rng); }
double randomDouble(double lo, double hi) { return lo + (hi - lo) * randomDouble(); }

Vec3 randomInUnitSphere() {
    while (true) {
        Vec3 p = {randomDouble(-1,1), randomDouble(-1,1), randomDouble(-1,1)};
        if (p.lengthSquared() < 1.0) return p;
    }
}

Vec3 randomUnitVector() { return normalize(randomInUnitSphere()); }

// ── Ray color (recursive path tracing) ───────────────────────────────────────
Color rayColor(const Ray& ray, const Hittable& world, int depth) {
    if (depth <= 0)
        return {0, 0, 0};   // absorbed — no more light

    HitRecord rec;
    constexpr double tMin = 1e-4;   // shadow-acne fix
    constexpr double tMax = std::numeric_limits<double>::infinity();

    if (world.hit(ray, tMin, tMax, rec)) {
        // Lambertian (matte) diffuse: scatter in random hemisphere around normal
        Vec3 target = rec.point + rec.normal + randomUnitVector();
        Ray  scattered(rec.point, target - rec.point);
        return 0.5 * rayColor(scattered, world, depth - 1);
    }

    // Background: sky gradient — blue at top, white at bottom
    Vec3   unit = normalize(ray.direction);
    double t    = 0.5 * (unit.y + 1.0);
    return (1.0 - t) * Color(1.0, 1.0, 1.0) + t * Color(0.5, 0.7, 1.0);
   // return (1.0 - t) * Color(1.0, 1.0, 1.0) + t * Color(0.1, 0.0, 0.0);
}

// ── Main ──────────────────────────────────────────────────────────────────────
int main() {
    // Image
    constexpr double aspectRatio   = 16.0 / 9.0;
    constexpr int    imageWidth    = 800;
    constexpr int    imageHeight   = static_cast<int>(imageWidth / aspectRatio);
    constexpr int    samplesPerPx  = 100;   // anti-aliasing samples
    constexpr int    maxDepth      = 50;    // max ray bounces

    // World
    HittableList world;
    world.add(std::make_shared<Sphere>(Point3( 0.0,    0.0, -1.0),  0.5));   // center sphere
    world.add(std::make_shared<Sphere>(Point3(-1.0,    0.0, -1.0),  0.5));   // left sphere
    world.add(std::make_shared<Sphere>(Point3( 1.0,    0.0, -1.0),  0.5));   // right sphere
    world.add(std::make_shared<Sphere>(Point3( 0.0, -100.5, -1.0), 100.0));  // ground

    // Camera
    Camera camera(
        {0, 0,  0},   // look from
        {0, 0, -1},   // look at
        {0, 1,  0},   // up
        70.0,         // vfov
        aspectRatio
    );

    // Render → pixel buffer (top-to-bottom, left-to-right)
    std::vector<Color> pixels(imageWidth * imageHeight);

    for (int j = imageHeight - 1; j >= 0; --j) {
        std::cerr << "\rScanlines remaining: " << j << "   " << std::flush;

        for (int i = 0; i < imageWidth; ++i) {
            Color pixelColor(0, 0, 0);

            // Supersample — jitter rays within the pixel
            for (int s = 0; s < samplesPerPx; ++s) {
                double u = (i + randomDouble()) / (imageWidth  - 1);
                double v = (j + randomDouble()) / (imageHeight - 1);
                pixelColor += rayColor(camera.getRay(u, v), world, maxDepth);
            }

            // Store in top-to-bottom order (row 0 = top)
            int row = imageHeight - 1 - j;
            pixels[row * imageWidth + i] = pixelColor;
        }
    }

    saveBMP("output.bmp", pixels, imageWidth, imageHeight, samplesPerPx);
    std::cerr << "\nDone! Saved to output.bmp\n";
    return 0;
}
