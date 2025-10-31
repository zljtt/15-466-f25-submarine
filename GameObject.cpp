#include "Game.hpp"

#include "Connection.hpp"
#include "PlayMode.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#include "BBox.hpp"

inline bool overlap1D(float min1, float max1, float min2, float max2)
{
    return !(max1 <= min2 || max2 <= min1);
}

inline float penetration1D(float min1, float max1, float min2, float max2)
{
    float penl = max1 - min2;
    float penr = max2 - min1;
    return (penl < penr) ? -penl : penr;
}

static inline void check_collision_in_axis(GameObject *object, float move, int axis, std::vector<GameObject *> candidates, std::vector<GameObject *> &hits)
{
    if (move == 0.0f)
        return;

    if (axis == 0)
        object->position.x += move;
    else
        object->position.y += move;

    for (int iter = 0; iter < 8; ++iter)
    {
        BBox self = object->get_BBox();
        bool pushed = false;

        for (GameObject *o : candidates)
        {
            BBox other = o->get_BBox();

            if (axis == 0)
            {
                if (!overlap1D(self.min.y, self.max.y, other.min.y, other.max.y))
                    continue;
                if (!overlap1D(self.min.x, self.max.x, other.min.x, other.max.x))
                    continue;
                float push = penetration1D(self.min.x, self.max.x, other.min.x, other.max.x);
                if (push != 0.0f)
                {
                    object->position.x += push;
                    pushed = true;
                    hits.push_back(o);
                }
            }
            else
            {
                if (!overlap1D(self.min.x, self.max.x, other.min.x, other.max.x))
                    continue;
                if (!overlap1D(self.min.y, self.max.y, other.min.y, other.max.y))
                    continue;
                float push = penetration1D(self.min.y, self.max.y, other.min.y, other.max.y);
                if (push != 0.0f)
                {
                    object->position.y += push;
                    pushed = true;
                    hits.push_back(o);
                }
            }
        }
        if (!pushed)
            break;
    }
};

bool NetworkObject::can_collide(const NetworkObject *other) const
{
    // if (other->id == this->id)
    //     return false;
    return false;
}

void NetworkObject::gather_collision_candidates(Game *game, const BBox &sweepBox, std::vector<GameObject *> &out)
{
    out.clear();
    out.reserve(64);
    for (auto &s : game->static_obstacles)
    {
        if (s.get_BBox().overlaps(sweepBox))
            out.push_back(&s);
    }
    for (auto *o : game->game_objects)
    {
        if (can_collide(o))
        {
            if (o->get_BBox().overlaps(sweepBox))
                out.push_back(o);
        }
    }
}

std::vector<GameObject *> NetworkObject::move_with_collision(Game *game, glm::vec2 movement)
{
    BBox box = get_BBox();
    BBox sweep;
    sweep.min = glm::min(box.min, box.min + movement);
    sweep.max = glm::max(box.max, box.max + movement);

    std::vector<GameObject *> candidates;

    gather_collision_candidates(game, sweep, candidates);
    std::vector<GameObject *> hits;
    check_collision_in_axis(this, movement.x, 0, candidates, hits);
    check_collision_in_axis(this, movement.y, 1, candidates, hits);

    std::sort(hits.begin(), hits.end());
    hits.erase(std::unique(hits.begin(), hits.end()), hits.end());
    return hits;
    // float movement_length = glm::length(movement);
    // glm::vec2 movement_direction = glm::normalize(movement);
    // Ray2D collision_detection_ray(position, movement_direction);
    // Trace result = game->bvh.hit(collision_detection_ray);
    // for (const auto &o : game->game_objects)
    // {
    //     if (o->id == this->id)
    //         continue;
    //     Trace h = o->hit(collision_detection_ray);
    //     if (h.hit)
    //     {
    //         if (h.distance < result.distance)
    //             result = h;
    //     }
    // }
    // float bound = glm::length(get_BBox().max - get_BBox().min) / 2.0f;
    // if (bound > result.distance)
    // {
    //     position -= (bound - result.distance) * movement_direction;
    //     return result.obj;
    // }
    // else if ((bound + movement_length) > result.distance)
    // {
    //     position += (result.distance - bound) * movement_direction;
    //     return result.obj;
    // }
    // else
    // {
    //     position += movement;
    //     return nullptr;
    // }
}

Trace GameObject::hit(Ray2D ray) const
{
    Trace trace;
    trace.hit = false;
    glm::vec2 result(ray.dist_bounds.x, ray.dist_bounds.y);
    bool h = get_BBox().hit(ray, result);
    if (h)
    {
        trace.hit = true;
        trace.distance = result.x;
        trace.obj = this;
        trace.point = ray.point + ray.dir * trace.distance;
        return trace;
    };
    return trace;
}

void NetworkObject::init()
{
    id = dist(mt);
}

void NetworkObject::send(Connection *connection) const
{
    connection->send(id);
    connection->send(type);
    connection->send(position);
    connection->send(scale);
    connection->send(deleted);
};

void NetworkObject::receive(uint32_t *at, std::vector<uint8_t> &recv_buffer)
{
    auto read = [&](auto *val)
    {
        std::memcpy(val, &recv_buffer[4 + *at], sizeof(*val));
        *at += sizeof(*val);
    };
    read(&id);
    read(&type);
    read(&position);
    read(&scale);
    read(&deleted);
};
