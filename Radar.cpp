#include "Radar.hpp"
#include "PlayMode.hpp"

void Radar::scan(GameObject const *origin, float range, int count)
{
    raycast_surrounding(origin->position, range, count, last_radar_hits);
};

void Radar::raycast_surrounding(const glm::vec2 origin, float range, int count, std::vector<RaycastResult> &out)
{
    out.clear();
    out.reserve(count);

    for (int i = 0; i < count; ++i)
    {
        float angle = (6.28318530718f * i) / count;
        RaycastResult hit = raycast_direction(origin, {std::cos(angle), std::sin(angle)}, range);
        if (hit.hit)
        {
            out.push_back(hit);
        }
    }
}

RaycastResult Radar::raycast_direction(const glm::vec2 origin, glm::vec2 direction, float range)
{
    RaycastResult closest;

    for (const auto &o : client_game->local_obstacles)
    {
        RaycastResult h;
        if (raycastAABB(origin, direction, o, range, h))
        {
            if (h.t < closest.t)
                closest = h;
        }
    }

    for (const auto &o : client_game->network_objects)
    {
        if (o.id == client_game->local_player->id)
            continue;
        RaycastResult h;
        if (raycastAABB(origin, direction, o, range, h))
        {
            if (h.t < closest.t)
                closest = h;
        }
    }
    if (closest.hit)
        return closest;

    closest.hit = false;
    return closest;
}
