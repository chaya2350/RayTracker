#include "vec3.h"
#include "ray.h"
#include "utils.h"
#include "camera.h"
#include "hittable_list.h"
#include "sphere.h"
#include "material.h"
#include "bvh.h"
#include "triangle.h"
#include "obj_loader.h"
#include "quad.h"
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
    if (!world.hit(ray, 1e-4, infinity, rec))
        return {0, 0, 0};  // Black background — the room has no windows

    // Ask the material: do you emit light?
    Color emitted = rec.material->emitted();

    // Ask the material: does the ray scatter?
    Ray   scattered;
    Color attenuation;
    if (!rec.material->scatter(ray, rec, attenuation, scattered))
        return emitted;  // Light source — just return its glow, don't recurse

    // Normal surface: emitted (usually 0) + reflected recursive color
    return emitted + attenuation * rayColor(scattered, world, depth - 1);
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
    // Image — Cornell Box is traditionally square (1:1)
    constexpr double aspectRatio  = 1.0;
    constexpr int    imageWidth   = 600;
    constexpr int    imageHeight  = 600;
    constexpr int    samplesPerPx = 200;
    constexpr int    maxDepth     = 50;

    // ── Cornell Box scene ─────────────────────────────────────────────────────
    // Classic setup: red left wall, green right wall, white everywhere else,
    // one bright area light in the ceiling, two boxes on the floor.
    HittableList world;

    auto red   = std::make_shared<Lambertian>(Color(0.65, 0.05, 0.05));
    auto green = std::make_shared<Lambertian>(Color(0.12, 0.45, 0.15));
    auto white = std::make_shared<Lambertian>(Color(0.73, 0.73, 0.73));
    auto light = std::make_shared<DiffuseLight>(Color(15, 15, 15));  // bright white light

    // Walls (555 x 555 x 555 room)
    world.add(std::make_shared<Quad>(Point3(555,0,0),   Vec3(0,555,0),  Vec3(0,0,555),  green)); // right
    world.add(std::make_shared<Quad>(Point3(0,0,0),     Vec3(0,555,0),  Vec3(0,0,555),  red));   // left
    world.add(std::make_shared<Quad>(Point3(0,0,0),     Vec3(555,0,0),  Vec3(0,0,555),  white)); // floor
    world.add(std::make_shared<Quad>(Point3(555,555,555),Vec3(-555,0,0),Vec3(0,0,-555), white)); // ceiling
    world.add(std::make_shared<Quad>(Point3(0,0,555),   Vec3(555,0,0),  Vec3(0,555,0),  white)); // back wall

    // Area light — a glowing rectangle cut into the ceiling
    world.add(std::make_shared<Quad>(Point3(213,554,227), Vec3(130,0,0), Vec3(0,0,105), light));

    // Two boxes (made of 6 quads each)
    // Tall box (rotated slightly — approximated with axis-aligned for now)
    auto addBox = [&](Point3 pmin, Point3 pmax, std::shared_ptr<Material> mat) {
        Vec3 dx(pmax.x - pmin.x, 0, 0);
        Vec3 dy(0, pmax.y - pmin.y, 0);
        Vec3 dz(0, 0, pmax.z - pmin.z);
        world.add(std::make_shared<Quad>(pmin,              dx, dy, mat)); // front
        world.add(std::make_shared<Quad>(pmin + dz,         dx, dy, mat)); // back
        world.add(std::make_shared<Quad>(pmin,              dz, dy, mat)); // left
        world.add(std::make_shared<Quad>(pmin + dx,         dz, dy, mat)); // right
        world.add(std::make_shared<Quad>(pmin,              dx, dz, mat)); // bottom
        world.add(std::make_shared<Quad>(pmin + dy,         dx, dz, mat)); // top
    };

    addBox(Point3(130, 0, 65),  Point3(295, 165, 230), white); // short box
    addBox(Point3(265, 0, 295), Point3(430, 330, 460), white); // tall box

    // ── Camera ───────────────────────────────────────────────────────────────
    // Looking straight into the room from the front opening
    Point3 lookFrom(278, 278, -800);
    Point3 lookAt  (278, 278,    0);
    double focusDist = 10.0;
    double aperture  = 0.0;  // no DOF — sharp throughout

    Camera camera(
        lookFrom, lookAt,
        {0, 1, 0},
        40.0,
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

