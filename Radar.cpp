#include "Radar.hpp"
#include "PlayMode.hpp"
#include "BBox.hpp"
#include "UIRenderer.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
// #include <algorithm>
// #include <cfloat>
// #include <cmath>

void Radar::render_path(DrawLines &hud)
{
    auto draw_dot = [&](glm::vec3 p, glm::u8vec4 color, float s)
    {
        hud.draw(p + glm::vec3(-s, 0, 0), p + glm::vec3(s, 0, 0), color);
        hud.draw(p + glm::vec3(0, -s, 0), p + glm::vec3(0, s, 0), color);
    };
    glm::u8vec4 default_color = {0x6f, 0x6f, 0x6f, 0x6f};

    for (auto &path : paths)
    {
        for (auto &point : path.points)
        {
            if (!point.touched)
            {
                draw_dot(glm::vec3(path.origin + path.distance * point.direction, 0), default_color, 0.05f);
                float angle = (glm::pi<float>() * 2) / RADAR_RAY_COUNT;

                glm::vec2 fake_dir1 = glm::rotate(point.direction, angle / (4 + 1));
                glm::vec2 fake_dir2 = glm::rotate(point.direction, 2 * angle / (4 + 1));
                glm::vec2 fake_dir3 = glm::rotate(point.direction, -angle / (4 + 1));
                glm::vec2 fake_dir4 = glm::rotate(point.direction, -2 * angle / (4 + 1));

                draw_dot(glm::vec3(path.origin + path.distance * fake_dir1, 0), default_color, 0.05f);
                draw_dot(glm::vec3(path.origin + path.distance * fake_dir2, 0), default_color, 0.05f);
                draw_dot(glm::vec3(path.origin + path.distance * fake_dir3, 0), default_color, 0.05f);
                draw_dot(glm::vec3(path.origin + path.distance * fake_dir4, 0), default_color, 0.05f);
            }
        }
    }
}

void Radar::render(DrawLines &hud)
{
    render_path(hud);

    client_game->text_overlays[PlayMode::RADAR].remove_images([](std::string const &key)
                                                              { return key.rfind("RADAR_", 0) == 0; });
    for (size_t i = 0; i < results.size(); i++)
    {
        std::string key = "RADAR_" + std::to_string(i);

        glm::vec2 raw = results[i].point;
        glm::vec2 size = glm::vec2(results[i].size, results[i].size);
        glm::vec2 pos = client_game->world_to_screen(glm::vec3(raw, 0), radar_text) - size / 2.0f;
        UIOverlay::ImageComponent img(*tex_radar_result, pos, size);
        float a = (RADAR_POINT_LIFE_TIME - results[i].age) / RADAR_POINT_LIFE_TIME;
        img.color = glm::vec4(glm::vec3(results[i].color), a);
        // img.color = glm::vec4(0.0f, 0.1f, 1.0f, a);

        // auto clamp_x = std::clamp(pos.x - w / 2, 10.0f, (float)radar_text->width - 10.0f);
        // auto clamp_y = std::clamp(pos.y - h / 2, 10.0f, (float)radar_text->height - 10.0f);
        client_game->text_overlays[PlayMode::RADAR].update_image(key, img);
        // client_game->text_overlays[PlayMode::RADAR].update_text(key, special_radar_results[i].type, glm::vec2(clamp_x, clamp_y));
    }
}

void Radar::update(float elapse)
{
    for (auto &result : results)
    {
        result.age += elapse;
    }
    results.erase(
        std::remove_if(results.begin(), results.end(),
                       [](const ScanResult &r)
                       { return r.age > RADAR_POINT_LIFE_TIME; }),
        results.end());

    for (auto &path : paths)
    {
        path.distance += elapse * RADAR_SPEED;
        for (auto &point : path.points)
        {
            // if touch obstacles
            if (path.distance > point.bound && !point.touched)
            {
                point.touched = true;
                auto at = path.origin + path.distance * point.direction;
                int offset = dist(gen);
                glm::vec4 color;
                glm::vec4 green(0.0f, 1.0f, 0.0f, 1.0f);
                if (point.velocity < 0.05f)
                {
                    color = green;
                }
                else
                {
                    glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
                    float t = std::clamp(point.velocity / 8.0f, 0.0f, 1.0f);
                    color = glm::mix(green, red, t);
                }
                results.emplace_back(at, color, RADAR_POINT_SIZE + offset, 0.0f);
            }
        }
    }
    paths.erase(
        std::remove_if(paths.begin(), paths.end(),
                       [](const ScanPath &r)
                       { return r.distance > RADAR_RANGE; }),
        paths.end());

    special_radar_timer -= elapse;
}

void Radar::scan(GameObject const *origin, float range, int count)
{
    ScanPath path;
    path.points.clear();
    path.origin = origin->position;
    path.distance = 0.5f;
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

            radar_point.velocity = std::sqrtf(hit.obj->velocity.x * hit.obj->velocity.x + hit.obj->velocity.y * hit.obj->velocity.y);
        }
        else
        {
            radar_point.bound = INFINITY;
            radar_point.velocity = 0;
        }
        radar_point.color = get_radar_color(hit.obj);
        radar_point.direction = dir;
        radar_point.touched = false;
        path.points.push_back(radar_point);
    }
    paths.push_back(path);
};

void Radar::scan_special(GameObject const *origin, float range)
{
    // for (auto &obj : client_game->network_objects)
    // {
    // if (obj.type == ObjectType::Player)
    // {
    //     special_radar_results.emplace_back(obj.position, "x");
    // }
    // else if (obj.type == ObjectType::Flag)
    // {
    //     special_radar_results.emplace_back(obj.position, "⚑");
    // }
    // }
    special_radar_timer = 5.0f;
};

glm::u8vec4 Radar::get_radar_color(GameObject const *target)
{
    if (!target)
        return default_color;

    switch (target->type)
    {
    case ObjectType::Player:
        return {0xff, 0x00, 0x00, 0xff};
    case ObjectType::Obstacle:
        return {0xff, 0xff, 0xff, 0xff};
    case ObjectType::Flag:
        return {0xff, 0xff, 0xff, 0xff};
    default:
        return default_color;
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
