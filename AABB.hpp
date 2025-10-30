
#pragma once
#include <glm/glm.hpp>
#include "GameObject.hpp"

static inline bool overlaps(const AABB &a, const AABB &b)
{
    return (a.min.x < b.max.x && a.max.x > b.min.x &&
            a.min.y < b.max.y && a.max.y > b.min.y);
};

static inline glm::vec2 resolve_axis(GameObject *object,
                                     const GameObject &obs,
                                     int axis)
{
    AABB p = object->get_aabb();
    AABB o = obs.get_aabb();
    if (!overlaps(p, o))
        return {0, 0};

    float pen_l = p.max.x - o.min.x;
    float pen_r = o.max.x - p.min.x;
    float pen_d = p.max.y - o.min.y;
    float pen_u = o.max.y - p.min.y;
    glm::vec2 push = {0, 0};
    if (axis == 0)
    {
        push.x += (pen_l < pen_r) ? -pen_l : pen_r;
    }
    else
    {
        push.y += (pen_d < pen_u) ? -pen_d : pen_u;
    }
    return push;
};