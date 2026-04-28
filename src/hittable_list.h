#pragma once
#include "hittable.h"
#include <vector>
#include <memory>

class HittableList : public Hittable {
public:
    std::vector<std::shared_ptr<Hittable>> objects;

    void add(std::shared_ptr<Hittable> obj) { objects.push_back(std::move(obj)); }
    void clear() { objects.clear(); }

    bool hit(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override {
        HitRecord tmp;
        bool hitAnything = false;
        double closest   = tMax;

        for (const auto& obj : objects) {
            if (obj->hit(ray, tMin, closest, tmp)) {
                hitAnything = true;
                closest     = tmp.t;
                rec         = tmp;
            }
        }
        return hitAnything;
    }
};
