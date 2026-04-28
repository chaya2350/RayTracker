#include "vec3.h"
#include "ray.h"
#include "utils.h"
#include "camera.h"
#include "hittable_list.h"
#include "sphere.h"
#include "material.h"
#include "bvh.h"
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

    // ── Big random scene ──────────────────────────────────────────────────────
    HittableList world;

    // Ground
    world.add(std::make_shared<Sphere>(
        Point3(0, -1000, 0), 1000,
        std::make_shared<Lambertian>(Color(0.5, 0.5, 0.5))));

    // Dozens of small random spheres
    for (int a = -11; a < 11; ++a) {
        for (int b = -11; b < 11; ++b) {
            double chooseMat = randomDouble();
            Point3 center(a + 0.9*randomDouble(), 0.2, b + 0.9*randomDouble());

            // Skip spheres too close to the big three
            if ((center - Point3(4, 0.2, 0)).length() < 0.9) continue;

            std::shared_ptr<Material> mat;

            if (chooseMat < 0.75) {
                // 75% — matte with random color
                Color albedo = Color(randomDouble()*randomDouble(),
                                     randomDouble()*randomDouble(),
                                     randomDouble()*randomDouble());
                mat = std::make_shared<Lambertian>(albedo);
            } else if (chooseMat < 0.92) {
                // 17% — metal with random color and fuzz
                Color  albedo = Color(randomDouble(0.5,1), randomDouble(0.5,1), randomDouble(0.5,1));
                double fuzz   = randomDouble(0, 0.4);
                mat = std::make_shared<Metal>(albedo, fuzz);
            } else {
                // 8% — glass
                mat = std::make_shared<Dielectric>(1.5);
            }

            world.add(std::make_shared<Sphere>(center, 0.2, mat));
        }
    }

    // Three hero spheres in the center
    world.add(std::make_shared<Sphere>(Point3( 0, 1, 0), 1.0,
        std::make_shared<Dielectric>(1.5)));                                       // glass

    world.add(std::make_shared<Sphere>(Point3(-4, 1, 0), 1.0,
        std::make_shared<Lambertian>(Color(0.4, 0.2, 0.1))));                     // brown matte

    world.add(std::make_shared<Sphere>(Point3( 4, 1, 0), 1.0,
        std::make_shared<Metal>(Color(0.7, 0.6, 0.5), 0.0)));                     // chrome

    // ── Camera with Depth of Field ────────────────────────────────────────────
    Point3 lookFrom(13, 2, 3);
    Point3 lookAt  ( 0, 0, 0);
    double focusDist = 10.0;
    double aperture  = 0.1;

    Camera camera(
        lookFrom, lookAt,
        {0, 1, 0},
        20.0,
        aspectRatio,
        aperture,
        focusDist
    );

    // Wrap world in BVH — turns O(n) ray tests into O(log n)
    BVHNode bvhWorld(world);

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
            std::cref(camera), std::cref(bvhWorld),
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

