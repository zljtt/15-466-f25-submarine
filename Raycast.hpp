#pragma once

#include "GameObject.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>

struct RaycastResult
{
    bool hit = false;
    float t = INFINITY;
    glm::vec2 point{0, 0};
    glm::vec2 normal{0, 0};
    const GameObject *obj = nullptr;
};

// assume dir is normalized
static inline bool raycastAABB(const glm::vec2 &origin, const glm::vec2 &dir, const GameObject &box, float range, RaycastResult &out)
{
    AABB aabb = box.get_aabb();

    auto inv = [](float d)
    {
        return (std::abs(d) < 1e-8f) ? std::numeric_limits<float>::infinity() : 1.0f / d;
    };

    float invx = inv(dir.x);
    float invy = inv(dir.y);

    // distance to min and max x
    float tx1 = (aabb.min.x - origin.x) * invx;
    float tx2 = (aabb.max.x - origin.x) * invx;
    if (tx1 > tx2)
        std::swap(tx1, tx2);

    // distance to min and max y
    float ty1 = (aabb.min.y - origin.y) * invy;
    float ty2 = (aabb.max.y - origin.y) * invy;
    if (ty1 > ty2)
        std::swap(ty1, ty2);

    float enter = std::max(tx1, ty1);
    float exit = std::min(tx2, ty2);

    if (exit < 0.0f || enter > exit || enter > range)
        return false;

    float hit = (enter >= 0.0f) ? enter : exit;
    if (hit < 0.0f || hit > range)
        return false;

    glm::vec2 normal(0, 0);
    if (std::abs(hit - tx1) < 1e-5f)
        normal = glm::vec2(-1.0f, 0.0f);
    else if (std::abs(hit - tx2) < 1e-5f)
        normal = glm::vec2(1.0f, 0.0f);
    else if (std::abs(hit - ty1) < 1e-5f)
        normal = glm::vec2(0.0f, -1.0f);
    else
        normal = glm::vec2(0.0f, 1.0f);

    out.hit = true;
    out.t = hit;
    out.point = origin + dir * hit;
    out.normal = normal;
    out.obj = &box;
    return true;
}