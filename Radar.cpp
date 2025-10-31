#include "Radar.hpp"
#include "PlayMode.hpp"
#include "BBox.hpp"
#include "BBox.hpp"
// #include <algorithm>
// #include <cfloat>
// #include <cmath>
void Radar::render(DrawLines &hud)
{
    auto draw_dot = [&](glm::vec3 p, glm::u8vec4 color)
    {
        float s = 0.1f;
        hud.draw(p + glm::vec3(-s, 0, 0), p + glm::vec3(s, 0, 0), color);
        hud.draw(p + glm::vec3(0, -s, 0), p + glm::vec3(0, s, 0), color);
    };

    for (auto &result : radar_results)
    {
        for (auto &p : result.points)
        {
            if (!p.touched && result.age > RADAR_RANGE / RADAR_SPEED)
            {
                // don't draw if untouched
            }
            else
            {
                draw_dot(glm::vec3(result.origin + p.current * p.direction, 0), p.color);
            }
        }
    }
}
void Radar::update(float elapse)
{
    for (auto &result : radar_results)
    {
        for (auto &p : result.points)
        {
            p.current += elapse * RADAR_SPEED;
            if (p.current > p.bound)
            {
                p.touched = true;
                p.current = p.bound;
            }
        }
        result.age += elapse;
    }
    radar_results.erase(
        std::remove_if(radar_results.begin(), radar_results.end(),
                       [](const RadarScanResult &r)
                       { return r.age > RADAR_MAX_LIFE_TIME; }),
        radar_results.end());
}

void Radar::scan(GameObject const *origin, float range, int count)
{
    RadarScanResult scan_result;
    scan_result.points.clear();
    scan_result.origin = origin->position;
    scan_result.age = 0;
    for (int i = 0; i < count; ++i)
    {
        RadarPoint radar_point;
        float angle = (glm::pi<float>() * 2 * i) / count;
        // float angle = (6.28318530718f * i) / count;
        glm::vec2 dir = {std::cos(angle), std::sin(angle)};
        Trace hit = raycast_direction(origin->position, dir, range);
        if (hit.hit)
        {
            if (hit.distance > range)
                radar_point.bound = INFINITY;
            else if (hit.distance < 1e-6f)
                radar_point.bound = 0;
            else
                radar_point.bound = hit.distance;
        }
        else
        {
            radar_point.bound = INFINITY;
        }
        radar_point.current = 0.5;
        radar_point.color = get_radar_color(hit.obj);
        radar_point.direction = dir;
        scan_result.points.push_back(radar_point);
    }
    radar_results.push_back(scan_result);
};

glm::u8vec4 Radar::get_radar_color(GameObject const *target)
{
    if (!target)
        return {255, 255, 255, 255};

    switch (target->type)
    {
    case ObjectType::Player:
        return {255, 0, 0, 255};
    default:
        return {255, 255, 255, 255};
    }
}
Trace Radar::raycast_direction(const glm::vec2 origin, glm::vec2 direction, float range)
{
    Ray2D ray(origin, direction, glm::vec2(0, range));
    Trace closest;
    closest.hit = false;
    // bvh on all obstacles
    closest = client_game->bvh.hit(ray);
    // additional raycast on dynamic objects
    for (const auto &o : client_game->network_objects)
    {
        if (o.id == client_game->local_player->id)
            continue;
        Trace h = o.hit(ray);
        if (h.hit)
        {
            if (h.distance < closest.distance)
                closest = h;
        }
    }
    return closest;
}
