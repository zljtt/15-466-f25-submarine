
#pragma once
#include <glm/glm.hpp>
#include <algorithm>
#include <cfloat>
#include <cmath>

struct Ray2D;

struct BBox
{
    glm::vec2 min, max;
    BBox &operator=(const BBox &other)
    {
        if (this != &other)
        {
            min = other.min;
            max = other.max;
        }
        return *this;
    }

    static inline glm::vec2 hmin(glm::vec2 l, glm::vec2 r)
    {
        return glm::vec2(std::min(l.x, r.x), std::min(l.y, r.y));
    }

    static inline glm::vec2 hmax(glm::vec2 l, glm::vec2 r)
    {
        return glm::vec2(std::max(l.x, r.x), std::max(l.y, r.y));
    }

    bool overlaps(const BBox &b) const
    {
        return (min.x < b.max.x && max.x > b.min.x &&
                min.y < b.max.y && max.y > b.min.y);
    };

    void enclose(glm::vec2 point)
    {
        min = hmin(min, point);
        max = hmax(max, point);
    }

    void enclose(BBox box)
    {
        min = hmin(min, box.min);
        max = hmax(max, box.max);
    }

    bool empty() const
    {
        return min.x > max.x || min.y > max.y;
    }

    float surface_area() const
    {
        if (empty())
            return 0.0f;
        glm::vec2 extent = max - min;
        return extent.x * extent.y;
    }

    /// Get center point of box
    glm::vec2 center() const
    {
        return (min + max) * 0.5f;
    }

    // copied and modified from Scotty3D (from zhijianw)
    bool hit(const Ray2D &ray, glm::vec2 &times) const;
};

// static inline glm::vec2 resolve_axis(GameObject *object,
//                                      const GameObject &obs,
//                                      int axis)
// {
//     BBox p = object->get_BBox();
//     BBox o = obs.get_BBox();
//     if (!overlaps(p, o))
//         return {0, 0};

//     float pen_l = p.max.x - o.min.x;
//     float pen_r = o.max.x - p.min.x;
//     float pen_d = p.max.y - o.min.y;
//     float pen_u = o.max.y - p.min.y;
//     glm::vec2 push = {0, 0};
//     if (axis == 0)
//     {
//         push.x += (pen_l < pen_r) ? -pen_l : pen_r;
//     }
//     else
//     {
//         push.y += (pen_d < pen_u) ? -pen_d : pen_u;
//     }
//     return push;
// };
