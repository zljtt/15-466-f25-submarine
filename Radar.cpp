#include "Radar.hpp"

void Radar::scan(Player const *local_player, float range, int count)
{
    raycast_surrounding(local_player->position, range, count, last_radar_hits);
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
    // generate a combined vector
    std::vector<GameObject *> targets;
    targets.reserve(obstacles.size() + additional.size());
    targets.insert(targets.end(), obstacles.begin(), obstacles.end());
    for (auto &obj : additional)
    {
        targets.push_back(&obj);
    }

    for (const auto &o : targets)
    {
        RaycastResult h;
        if (raycastAABB(origin, direction, *o, range, h))
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
