#pragma once
#include "hittable_list.h"
#include "triangle.h"
#include "bvh.h"
#include "material.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

// Load a .obj file and return a BVHNode containing all its triangles.
// Supports: v (vertices), f (faces — triangles and quads).
// Ignores: normals, UVs, materials (uses the provided material instead).
inline std::shared_ptr<Hittable> loadOBJ(const std::string& filename,
                                          std::shared_ptr<Material> material)
{
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "ERROR: Cannot open OBJ file: " << filename << "\n";
        return nullptr;
    }

    std::vector<Point3>  vertices;
    HittableList         triangles;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "v") {
            // Vertex position
            double x, y, z;
            ss >> x >> y >> z;
            vertices.emplace_back(x, y, z);

        } else if (token == "f") {
            // Face — parse indices (OBJ is 1-based)
            // Supports: "f 1 2 3", "f 1/2/3 4/5/6 7/8/9", "f 1//3 2//4 3//5"
            std::vector<int> indices;
            std::string part;
            while (ss >> part) {
                // Take only the first number before '/'
                int idx = std::stoi(part.substr(0, part.find('/')));
                if (idx < 0)
                    idx = static_cast<int>(vertices.size()) + idx + 1; // negative = relative
                indices.push_back(idx - 1); // convert to 0-based
            }

            // Triangulate (fan from first vertex — works for convex faces)
            for (size_t i = 1; i + 1 < indices.size(); ++i) {
                triangles.add(std::make_shared<Triangle>(
                    vertices[indices[0]],
                    vertices[indices[i]],
                    vertices[indices[i + 1]],
                    material
                ));
            }
        }
    }

    std::cerr << "Loaded: " << filename
              << "  |  vertices: " << vertices.size()
              << "  |  triangles: " << triangles.objects.size() << "\n";

    // Wrap all triangles in a BVH for fast traversal
    return std::make_shared<BVHNode>(triangles);
}
