#include "Radar.hpp"
#include "PlayMode.hpp"
#include "BBox.hpp"
#include "UIRenderer.hpp"
#include "Registry.hpp"

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

    for (auto &path : paths)
    {
        for (auto &point : path.points)
        {
            if (!point.touched)
            {

                draw_dot(glm::vec3(path.origin + path.distance * point.direction, 0), path.color, 0.05f);
                float angle = (glm::pi<float>() * 2) / RADAR_RAY_COUNT;

                glm::vec2 fake_dir1 = glm::rotate(point.direction, angle / (4 + 1));
                glm::vec2 fake_dir2 = glm::rotate(point.direction, 2 * angle / (4 + 1));
                glm::vec2 fake_dir3 = glm::rotate(point.direction, -angle / (4 + 1));
                glm::vec2 fake_dir4 = glm::rotate(point.direction, -2 * angle / (4 + 1));

                draw_dot(glm::vec3(path.origin + path.distance * fake_dir1, 0), path.color, 0.05f);
                draw_dot(glm::vec3(path.origin + path.distance * fake_dir2, 0), path.color, 0.05f);
                draw_dot(glm::vec3(path.origin + path.distance * fake_dir3, 0), path.color, 0.05f);
                draw_dot(glm::vec3(path.origin + path.distance * fake_dir4, 0), path.color, 0.05f);
            }
        }
    }
}

void Radar::render_results()
{
    client_game->get_overlay(PlayMode::RADAR).remove_images([](std::string const &key)
                                                            { return key.rfind("R_", 0) == 0; });
    for (size_t i = 0; i < results.size(); i++)
    {
        std::string key = "R_" + std::to_string(i);
        auto point = results[i];
        glm::vec2 size = glm::vec2(point.size, point.size);
        glm::vec2 pos = client_game->world_to_screen(glm::vec3(point.point, 0), renderer_radar);
        if (point.show_out_of_range)
        {
            auto renderer = client_game->get_overlay(PlayMode::RADAR).renderer;
            auto clamp_x = std::clamp(pos.x, size.x / 2.0f, (float)renderer->width - size.x / 2.0f);
            auto clamp_y = std::clamp(pos.y, size.y / 2.0f, (float)renderer->height - size.y / 2.0f);
            pos = glm::vec2(clamp_x, clamp_y);
        }
        UIOverlay::ImageComponent img(point.tex, pos - size / 2.0f, size);
        float a = (RADAR_POINT_LIFE_TIME - point.age) / RADAR_POINT_LIFE_TIME;
        img.color = glm::vec4(glm::vec3(point.color), a);
        // img.color = glm::vec4(0.0f, 0.1f, 1.0f, a);

        client_game->get_overlay(PlayMode::RADAR).update_image(key, img);
    }
}

void Radar::render(DrawLines &hud)
{
    render_path(hud);
    render_results();
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
        path.distance += elapse * path.speed;
        for (auto &point : path.points)
        {
            // if touch obstacles
            if (path.distance > point.bound && !point.touched)
            {
                point.touched = true;
                auto at = path.origin + path.distance * point.direction;
                int offset = dist(gen);

                results.emplace_back(at, point.color, RADAR_POINT_SIZE + offset, 0.0f, path.show_out_of_range, point.tex);
            }
        }
    }
    paths.erase(
        std::remove_if(paths.begin(), paths.end(),
                       [](const ScanPath &r)
                       { return r.distance > r.max_distance; }),
        paths.end());

    special_radar_timer -= elapse;
}

void Radar::scan(GameObject const *origin, float range, int count)
{
    ScanPath path;
    path.points.clear();
    path.origin = origin->position;
    path.distance = 0.5f;
    path.color = default_color;
    path.show_out_of_range = false;
    path.speed = RADAR_SPEED;
    path.max_distance = RADAR_RANGE;

    for (int i = 0; i < count; ++i)
    {
        RadarPoint radar_point;
        float angle = (glm::pi<float>() * 2 * i) / count;
        // float angle = (6.28318530718f * i) / count;
        glm::vec2 dir = {std::cos(angle), std::sin(angle)};
        Trace hit = raycast_direction(origin->position, dir, range);
        glm::vec4 green(0.0f, 1.0f, 0.0f, 1.0f);
        if (hit.hit)
        {
            if (hit.distance > range)
                radar_point.bound = INFINITY;
            else if (hit.distance < 1e-6f)
                radar_point.bound = 0;
            else
                radar_point.bound = hit.distance;
            float velocity = std::sqrtf(hit.obj->velocity.x * hit.obj->velocity.x + hit.obj->velocity.y * hit.obj->velocity.y);
            if (velocity < 0.05f)
            {
                radar_point.color = green;
            }
            else
            {
                glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
                float t = std::clamp(velocity / 8.0f, 0.0f, 1.0f);
                radar_point.color = glm::mix(green, red, t);
            }
        }
        else
        {
            radar_point.bound = INFINITY;
            radar_point.color = green;
        }
        radar_point.tex = *tex_radar_blurred;
        radar_point.direction = dir;
        radar_point.touched = false;
        path.points.push_back(radar_point);
    }
    paths.push_back(path);
};

void Radar::scan_special(GameObject const *origin, float range)
{
    ScanPath path;
    path.points.clear();
    path.origin = origin->position;
    path.distance = 0.5f;
    path.color = glm::vec4(0xff, 0xff, 0x00, 0xff);
    path.show_out_of_range = true;
    path.speed = RADAR_SPEED * 1.5f;
    path.max_distance = 55.0f;

    for (int i = 0; i < RADAR_RAY_COUNT; ++i)
    {
        RadarPoint radar_point;
        float angle = (glm::two_pi<float>() * i) / RADAR_RAY_COUNT;
        radar_point.direction = {std::cos(angle), std::sin(angle)};
        radar_point.touched = false;
        radar_point.bound = INFINITY;
        radar_point.color = glm::vec4(1.0f);
        path.points.push_back(radar_point);
    }

    auto closest_ray_index = [&](glm::vec2 dir) -> int
    {
        if (dir == glm::vec2(0.0f))
            return 0;
        float a = std::atan2(dir.y, dir.x);
        if (a < 0.0f)
            a += glm::two_pi<float>();
        float step = glm::two_pi<float>() / float(RADAR_RAY_COUNT);
        int idx = int(std::floor((a + 0.5f * step) / step)) % RADAR_RAY_COUNT;
        return idx;
    };

    for (auto &obj : client_game->network_objects)
    {
        GLuint texture;
        if (obj.type == ObjectType::Player && obj.id != client_game->local_player->id)
        {
            texture = *tex_radar_submarine;
        }
        else if (obj.type == ObjectType::Flag)
        {
            texture = *tex_radar_flag;
        }
        else
        {
            continue;
        }

        auto direction = glm::normalize(obj.position - origin->position);
        RadarPoint &rp = path.points[closest_ray_index(direction)];
        rp.tex = texture;
        float distance = glm::length(obj.position - origin->position);
        rp.bound = std::clamp(distance, 0.0f, 50.0f);
    }

    paths.push_back(path);
    special_radar_timer = 5.0f;
}
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
