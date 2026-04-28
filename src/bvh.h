#pragma once
#include "hittable.h"
#include "hittable_list.h"
#include "utils.h"
#include <algorithm>
#include <vector>
#include <memory>

class BVHNode : public Hittable {
public:
    // Build a BVH tree from a list of objects.
    // Recursively splits them in half along a random axis each level.
    BVHNode(const HittableList& list)
        : BVHNode(list.objects, 0, list.objects.size()) {}

    BVHNode(const std::vector<std::shared_ptr<Hittable>>& src,
            size_t start, size_t end)
    {
        auto objects = src;  // local copy to sort

        // Pick a random axis to split on: 0=X, 1=Y, 2=Z
        int axis = static_cast<int>(randomDouble(0, 3));

        auto comparator = (axis == 0) ? boxXCompare
                        : (axis == 1) ? boxYCompare
                                      : boxZCompare;

        size_t span = end - start;

        if (span == 1) {
            // Only one object — put it in both children (leaf node)
            left_ = right_ = objects[start];
        } else if (span == 2) {
            // Two objects — one each side, sorted by chosen axis
            if (comparator(objects[start], objects[start + 1])) {
                left_  = objects[start];
                right_ = objects[start + 1];
            } else {
                left_  = objects[start + 1];
                right_ = objects[start];
            }
        } else {
            // More objects — sort and recurse on each half
            std::sort(objects.begin() + start, objects.begin() + end, comparator);
            size_t mid = start + span / 2;
            left_  = std::make_shared<BVHNode>(objects, start, mid);
            right_ = std::make_shared<BVHNode>(objects, mid,   end);
        }

        // This node's box = the box that contains both children
        AABB boxLeft, boxRight;
        left_->boundingBox(boxLeft);
        right_->boundingBox(boxRight);
        box_ = surroundingBox(boxLeft, boxRight);
    }

    bool hit(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override {
        // If the ray misses our bounding box — skip everything inside
        if (!box_.hit(ray, tMin, tMax)) return false;

        // Otherwise check both children
        bool hitLeft  = left_->hit(ray, tMin, tMax, rec);
        bool hitRight = right_->hit(ray, tMin, hitLeft ? rec.t : tMax, rec);

        return hitLeft || hitRight;
    }

    bool boundingBox(AABB& outputBox) const override {
        outputBox = box_;
        return true;
    }

private:
    std::shared_ptr<Hittable> left_;
    std::shared_ptr<Hittable> right_;
    AABB box_;

    // Comparators — sort objects by their box min on a given axis
    static bool boxCompare(const std::shared_ptr<Hittable>& a,
                           const std::shared_ptr<Hittable>& b, int axis)
    {
        AABB boxA, boxB;
        a->boundingBox(boxA);
        b->boundingBox(boxB);
        return (&boxA.minimum.x)[axis] < (&boxB.minimum.x)[axis];
    }

    static bool boxXCompare(const std::shared_ptr<Hittable>& a,
                             const std::shared_ptr<Hittable>& b)
    { return boxCompare(a, b, 0); }

    static bool boxYCompare(const std::shared_ptr<Hittable>& a,
                             const std::shared_ptr<Hittable>& b)
    { return boxCompare(a, b, 1); }

    static bool boxZCompare(const std::shared_ptr<Hittable>& a,
                             const std::shared_ptr<Hittable>& b)
    { return boxCompare(a, b, 2); }
};
