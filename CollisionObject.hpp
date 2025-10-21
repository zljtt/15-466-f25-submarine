#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct AABB
{
    glm::vec2 min, max;
};

struct CollisionObject
{
    glm::vec2 position = glm::vec2(0.0f, 0.0f);
    glm::vec2 scale;

    CollisionObject() {};
    CollisionObject(glm::vec2 pos, glm::vec2 box) : position(pos), scale(box) {};

    AABB get_aabb() const
    {
        return {position - scale, position + scale};
    }

    virtual ~CollisionObject() {};
};

static inline bool overlaps(const AABB &a, const AABB &b)
{
    return (a.min.x < b.max.x && a.max.x > b.min.x &&
            a.min.y < b.max.y && a.max.y > b.min.y);
};

static inline void resolveAxis(CollisionObject &player,
                               const CollisionObject &obs,
                               int axis)
{
    AABB p = player.get_aabb();
    AABB o = obs.get_aabb();
    if (!overlaps(p, o))
        return;

    float penLeft = p.max.x - o.min.x;
    float penRight = o.max.x - p.min.x;
    float penDown = p.max.y - o.min.y;
    float penUp = o.max.y - p.min.y;

    if (axis == 0)
    {
        float push = (penLeft < penRight) ? -penLeft : penRight;
        player.position.x += push;
    }
    else
    {
        float push = (penDown < penUp) ? -penDown : penUp;
        player.position.y += push;
    }
};