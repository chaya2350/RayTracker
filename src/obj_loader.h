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
    std::vector<Vec3>    normals;     // vertex normals from "vn" lines
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

        } else if (token == "vn") {
            // Vertex normal
            double x, y, z;
            ss >> x >> y >> z;
            normals.emplace_back(x, y, z);

        } else if (token == "f") {
            // Face — parse indices (OBJ is 1-based)
            // Format can be: "f v", "f v/vt", "f v/vt/vn", "f v//vn"
            struct FaceVert { int vi = 0, ni = -1; }; // vertex index, normal index
            std::vector<FaceVert> fverts;

            std::string part;
            while (ss >> part) {
                FaceVert fv;
                // Split by '/'
                std::istringstream ps(part);
                std::string tok;
                int slot = 0;
                while (std::getline(ps, tok, '/')) {
                    if (!tok.empty()) {
                        int idx = std::stoi(tok);
                        if (slot == 0) fv.vi = (idx < 0) ? (int)vertices.size() + idx : idx - 1;
                        if (slot == 2) fv.ni = (idx < 0) ? (int)normals.size()   + idx : idx - 1;
                    }
                    ++slot;
                }
                fverts.push_back(fv);
            }

            // Triangulate (fan from first vertex)
            bool hasNormals = !normals.empty() && fverts[0].ni >= 0;
            for (size_t i = 1; i + 1 < fverts.size(); ++i) {
                if (hasNormals) {
                    triangles.add(std::make_shared<Triangle>(
                        vertices[fverts[0].vi],
                        vertices[fverts[i].vi],
                        vertices[fverts[i+1].vi],
                        normals[fverts[0].ni],
                        normals[fverts[i].ni],
                        normals[fverts[i+1].ni],
                        material
                    ));
                } else {
                    triangles.add(std::make_shared<Triangle>(
                        vertices[fverts[0].vi],
                        vertices[fverts[i].vi],
                        vertices[fverts[i+1].vi],
                        material
                    ));
                }
            }
        }
    }

    std::cerr << "Loaded: " << filename
              << "  |  vertices: " << vertices.size()
              << "  |  triangles: " << triangles.objects.size() << "\n";

    // Wrap all triangles in a BVH for fast traversal
    return std::make_shared<BVHNode>(triangles);
}
