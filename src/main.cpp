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
#include <thread>
#include <atomic>

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

// ── Render one horizontal band of rows [rowStart, rowEnd) ────────────────────
// Each thread calls this with its own slice — no shared writes, no data races.
void renderBand(int rowStart, int rowEnd,
                int imageWidth, int imageHeight,
                int samplesPerPx, int maxDepth,
                const Camera& camera, const Hittable& world,
                std::vector<Color>& pixels,
                std::atomic<int>& rowsDone)
{
    // Each thread gets its own RNG seeded differently — avoids lock contention
    std::mt19937 rng(rowStart * 1000 + 42);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    auto randD = [&]() { return dist(rng); };

    for (int j = rowEnd - 1; j >= rowStart; --j) {
        for (int i = 0; i < imageWidth; ++i) {
            Color pixelColor(0, 0, 0);

            for (int s = 0; s < samplesPerPx; ++s) {
                double u = (i + randD()) / (imageWidth  - 1);
                double v = (j + randD()) / (imageHeight - 1);
                pixelColor += rayColor(camera.getRay(u, v), world, maxDepth);
            }

            int row = imageHeight - 1 - j;
            pixels[row * imageWidth + i] = pixelColor;
        }
        ++rowsDone;  // atomic — thread-safe progress counter
    }
}

// ── Main ──────────────────────────────────────────────────────────────────────
int main() {
    // Image
    constexpr double aspectRatio  = 16.0 / 9.0;
    constexpr int    imageWidth   = 800;
    constexpr int    imageHeight  = static_cast<int>(imageWidth / aspectRatio);
    constexpr int    samplesPerPx = 100;
    constexpr int    maxDepth     = 50;

    // Materials
    auto matGround = std::make_shared<Lambertian>(Color(0.8, 0.8, 0.0));
    auto matCenter = std::make_shared<Dielectric>(1.5);
    auto matLeft   = std::make_shared<Metal>(Color(0.8, 0.8, 0.8), 0.0);
    auto matRight  = std::make_shared<Metal>(Color(0.8, 0.6, 0.2), 0.4);

    // World
    HittableList world;
    world.add(std::make_shared<Sphere>(Point3( 0.0,    0.0, -1.0),   0.5, matCenter));
    world.add(std::make_shared<Sphere>(Point3(-1.0,    0.0, -1.0),   0.5, matLeft));
    world.add(std::make_shared<Sphere>(Point3( 1.0,    0.0, -1.0),   0.5, matRight));
    world.add(std::make_shared<Sphere>(Point3( 0.0, -100.5, -1.0), 100.0, matGround));

    // Camera
    Camera camera({0,0,0}, {0,0,-1}, {0,1,0}, 70.0, aspectRatio);

    // Thread setup
    const int numThreads = std::thread::hardware_concurrency();
    const int rowsPerThread = imageHeight / numThreads;

    std::cerr << "Rendering with " << numThreads << " threads...\n";

    std::vector<Color>   pixels(imageWidth * imageHeight);
    std::atomic<int>     rowsDone{0};
    std::vector<std::thread> threads;

    // Launch one thread per CPU core, each handles a horizontal band
    for (int t = 0; t < numThreads; ++t) {
        int rowStart = t * rowsPerThread;
        int rowEnd   = (t == numThreads - 1) ? imageHeight : rowStart + rowsPerThread;

        threads.emplace_back(renderBand,
            rowStart, rowEnd,
            imageWidth, imageHeight, samplesPerPx, maxDepth,
            std::cref(camera), std::cref(world),
            std::ref(pixels), std::ref(rowsDone));
    }

    // Progress bar — runs on the main thread while workers render
    while (rowsDone < imageHeight) {
        int done    = rowsDone.load();
        int percent = done * 100 / imageHeight;
        int filled  = percent / 2;   // 50 chars wide
        std::cerr << "\r[" << std::string(filled, '#')
                           << std::string(50 - filled, ' ')
                  << "] " << percent << "%   " << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for all threads to finish
    for (auto& t : threads) t.join();

    std::cerr << "\r[##################################################] 100%\n";

    saveBMP("output.bmp", pixels, imageWidth, imageHeight, samplesPerPx);
    std::cerr << "Done! Saved to output.bmp\n";
    return 0;
}

