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

#include <numbers>

// ── Area light descriptor ─────────────────────────────────────────────────────
// Mirrors the Quad light in the scene so rayColor can sample it directly.
struct AreaLight {
    Point3 Q;       // corner of the light rectangle
    Vec3   u, v;    // two edge vectors
    Color  emit;    // emitted radiance
    Vec3   normal;  // unit normal (points downward into the room)
    double area;    // precomputed |u × v|

    AreaLight(const Point3& Q, const Vec3& u, const Vec3& v, const Color& emit)
        : Q(Q), u(u), v(v), emit(emit)
    {
        Vec3 cross_uv = cross(u, v);
        area   = cross_uv.length();
        normal = normalize(cross_uv);
    }

    // Returns a uniformly random point on the light surface
    Point3 sample() const {
        return Q + randomDouble() * u + randomDouble() * v;
    }
};

// ── Ray color with Next Event Estimation (direct light sampling) ──────────────
// includeEmit: true for camera rays and specular bounces,
//              false after diffuse bounces (already counted via direct sampling)
Color rayColor(const Ray& ray, const Hittable& world,
               const AreaLight& light, int depth, bool includeEmit = true)
{
    if (depth <= 0)
        return {0, 0, 0};

    HitRecord rec;
    if (!world.hit(ray, 1e-4, infinity, rec))
        return {0, 0, 0};  // Black background — the room has no windows

    // Emission (only shown to camera/specular rays to avoid double-counting)
    Color emitted = includeEmit ? rec.material->emitted() : Color(0, 0, 0);

    Ray   scattered;
    Color attenuation;
    if (!rec.material->scatter(ray, rec, attenuation, scattered))
        return emitted;  // Hit the lamp itself — just glow

    // ── Direct lighting: shadow ray straight to the area light ───────────────
    Color direct{0, 0, 0};
    {
        Point3 lightPt  = light.sample();
        Vec3   toLight  = lightPt - rec.point;
        double dist     = toLight.length();
        Vec3   toLightN = toLight / dist;

        double cosI = dot(rec.normal,    toLightN);   // angle at surface
        double cosO = dot(light.normal, -toLightN);   // angle at light face

        if (cosI > 0 && cosO > 0) {
            // Shadow ray — if nothing blocks the path, we're lit
            HitRecord shadow;
            if (!world.hit(Ray(rec.point, toLightN), 1e-4, dist - 1e-3, shadow)) {
                // Solid-angle pdf: p = d² / (cosO * A)
                double pdf = (dist * dist) / (cosO * light.area);
                direct = attenuation * light.emit * cosI / (pdf * std::numbers::pi);
            }
        }
    }

    // ── Indirect lighting: random bounce (don't re-count light emission) ──────
    Color indirect = attenuation * rayColor(scattered, world, light, depth - 1, false);

    return emitted + direct + indirect;
}

// ── Render one horizontal band of rows [rowStart, rowEnd) ────────────────────
// Each thread calls this with its own slice — no shared writes, no data races.
void renderBand(int rowStart, int rowEnd,
                int imageWidth, int imageHeight,
                int samplesPerPx, int maxDepth,
                const Camera& camera, const Hittable& world,
                const AreaLight& light,
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
                pixelColor += rayColor(camera.getRay(u, v), world, light, maxDepth);
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
    auto lightMat = std::make_shared<DiffuseLight>(Color(15, 15, 15));  // bright white light

    // Walls (555 x 555 x 555 room)
    world.add(std::make_shared<Quad>(Point3(555,0,0),   Vec3(0,555,0),  Vec3(0,0,555),  green)); // right
    world.add(std::make_shared<Quad>(Point3(0,0,0),     Vec3(0,555,0),  Vec3(0,0,555),  red));   // left
    world.add(std::make_shared<Quad>(Point3(0,0,0),     Vec3(555,0,0),  Vec3(0,0,555),  white)); // floor
    world.add(std::make_shared<Quad>(Point3(555,555,555),Vec3(-555,0,0),Vec3(0,0,-555), white)); // ceiling
    world.add(std::make_shared<Quad>(Point3(0,0,555),   Vec3(555,0,0),  Vec3(0,555,0),  white)); // back wall

    // Area light — a glowing rectangle cut into the ceiling
    world.add(std::make_shared<Quad>(Point3(213,554,227), Vec3(130,0,0), Vec3(0,0,105), lightMat));

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

    // Area light — must match the Quad we added to the scene exactly
    AreaLight light(
        Point3(213, 554, 227),
        Vec3(130, 0, 0),
        Vec3(0, 0, 105),
        Color(15, 15, 15)
    );

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
            std::cref(light),
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

