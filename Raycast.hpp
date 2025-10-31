#pragma once
#include "BBox.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <vector>

struct GameObject;

struct Trace
{
    bool hit = false;
    float distance = INFINITY;
    glm::vec2 point{0, 0};
    glm::vec2 normal{0, 0};
    const GameObject *obj = nullptr;
};

struct Node
{
    BBox bbox;
    size_t start, size, l, r;

    // A node is a leaf if l == r, since all interior nodes must have distinct children
    bool is_leaf() const;
};

// copied and modified from Scotty3D (from zhijianw)
struct BVH
{
    std::vector<GameObject> obstacles;
    std::vector<Node> nodes;
    size_t root_idx = 0;

    void build(std::vector<GameObject> &&obstacles, size_t max_leaf_size = 1);
    Trace hit(const Ray2D &ray) const;

    size_t new_node(BBox box, size_t start, size_t size, size_t l, size_t r);
};

struct SAHBucketData
{
    BBox bb;          ///< bbox of all primitives
    size_t num_prims; ///< number of primitives in the bucket
};

// copied/modified from Scotty3D
struct Ray2D
{

    Ray2D() = default;

    /// Create Ray from point and direction
    explicit Ray2D(glm::vec2 point, glm::vec2 dir,
                   glm::vec2 dist_bounds = glm::vec2{0.0f, std::numeric_limits<float>::infinity()})
        : point(point), dir(glm::normalize(dir)), dist_bounds(dist_bounds)
    {
    }

    Ray2D(const Ray2D &) = default;
    Ray2D &operator=(const Ray2D &) = default;
    ~Ray2D() = default;

    /// Get point on Ray at time t
    glm::vec2 at(float t) const
    {
        return point + t * dir;
    }

    /// The origin or starting point of this ray
    glm::vec2 point;
    /// The direction the ray travels in
    glm::vec2 dir;
    /// The minimum and maximum distance at which this ray can encounter collisions
    glm::vec2 dist_bounds = glm::vec2(0.0f, std::numeric_limits<float>::infinity());
};

// assume dir is normalized
// static inline bool raycastBBox(const glm::vec2 &origin, const glm::vec2 &dir, const GameObject &box, float range, RaycastResult &out)
// {
//     BBox BBox = box.get_BBox();

//     auto inv = [](float d)
//     {
//         return (std::abs(d) < 1e-8f) ? std::numeric_limits<float>::infinity() : 1.0f / d;
//     };

//     float invx = inv(dir.x);
//     float invy = inv(dir.y);

//     // distance to min and max x
//     float tx1 = (BBox.min.x - origin.x) * invx;
//     float tx2 = (BBox.max.x - origin.x) * invx;
//     if (tx1 > tx2)
//         std::swap(tx1, tx2);

//     // distance to min and max y
//     float ty1 = (BBox.min.y - origin.y) * invy;
//     float ty2 = (BBox.max.y - origin.y) * invy;
//     if (ty1 > ty2)
//         std::swap(ty1, ty2);

//     float enter = std::max(tx1, ty1);
//     float exit = std::min(tx2, ty2);

//     if (exit < 0.0f || enter > exit || enter > range)
//         return false;

//     float hit = (enter >= 0.0f) ? enter : exit;
//     if (hit < 0.0f || hit > range)
//         return false;

//     glm::vec2 normal(0, 0);
//     if (std::abs(hit - tx1) < 1e-5f)
//         normal = glm::vec2(-1.0f, 0.0f);
//     else if (std::abs(hit - tx2) < 1e-5f)
//         normal = glm::vec2(1.0f, 0.0f);
//     else if (std::abs(hit - ty1) < 1e-5f)
//         normal = glm::vec2(0.0f, -1.0f);
//     else
//         normal = glm::vec2(0.0f, 1.0f);

//     out.hit = true;
//     out.t = hit;
//     out.point = origin + dir * hit;
//     out.normal = normal;
//     out.obj = &box;
//     return true;
// }
