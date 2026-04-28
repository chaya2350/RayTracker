#include "vec3.h"
#include "ray.h"
#include "utils.h"
#include "camera.h"
#include "hittable_list.h"
#include "sphere.h"
#include "material.h"
#include "bmp_writer.h"

#include <iostream>
#include <vector>

// ── Ray color (recursive path tracing) ───────────────────────────────────────
Color rayColor(const Ray& ray, const Hittable& world, int depth) {
    if (depth <= 0)
        return {0, 0, 0};

    HitRecord rec;
    if (world.hit(ray, 1e-4, infinity, rec)) {
        Ray   scattered;
        Color attenuation;
        if (rec.material->scatter(ray, rec, attenuation, scattered))
            return attenuation * rayColor(scattered, world, depth - 1);
        return {0, 0, 0};
    }

    // Background: sky gradient — blue at top, white at bottom
    Vec3   unit = normalize(ray.direction);
    double t    = 0.5 * (unit.y + 1.0);
    return (1.0 - t) * Color(1.0, 1.0, 1.0) + t * Color(0.5, 0.7, 1.0);
}

// ── Main ──────────────────────────────────────────────────────────────────────
int main() {
    // Image
    constexpr double aspectRatio   = 16.0 / 9.0;
    constexpr int    imageWidth    = 800;
    constexpr int    imageHeight   = static_cast<int>(imageWidth / aspectRatio);
    constexpr int    samplesPerPx  = 100;
    constexpr int    maxDepth      = 50;

    // Materials
    auto matGround  = std::make_shared<Lambertian>(Color(0.8, 0.8, 0.0));  // yellow-green
    auto matCenter  = std::make_shared<Dielectric>(1.5);                    // glass
    auto matLeft    = std::make_shared<Metal>(Color(0.8, 0.8, 0.8), 0.0);  // chrome
    auto matRight   = std::make_shared<Metal>(Color(0.8, 0.6, 0.2), 0.4);  // brushed gold

    // World
    HittableList world;
    world.add(std::make_shared<Sphere>(Point3( 0.0,    0.0, -1.0),   0.5,   matCenter));
    world.add(std::make_shared<Sphere>(Point3(-1.0,    0.0, -1.0),   0.5,   matLeft));
    world.add(std::make_shared<Sphere>(Point3( 1.0,    0.0, -1.0),   0.5,   matRight));
    world.add(std::make_shared<Sphere>(Point3( 0.0, -100.5, -1.0), 100.0,   matGround));

    // Camera
    Camera camera(
        {0, 0,  0},
        {0, 0, -1},
        {0, 1,  0},
        70.0,
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
