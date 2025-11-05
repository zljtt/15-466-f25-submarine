#pragma once
#include "Raycast.hpp"
#include "BBox.hpp"
#include "DrawLines.hpp"

#include <glm/glm.hpp>
#include <list>

struct PlayMode;

struct RadarPoint
{
    glm::vec2 direction;
    float current;
    float bound;
    glm::u8vec4 color;
    bool touched = false;
};

struct RadarScanResult
{
    std::vector<RadarPoint> points;
    glm::vec2 origin;
    float age;
};

struct Radar
{
    static const int RADAR_RAY_COUNT = 40;
    static constexpr float RADAR_INTERVAL = 1.3f;
    static constexpr float RADAR_RANGE = 20.0f;
    static constexpr float RADAR_SPEED = 30.0f;
    static constexpr float RADAR_MAX_LIFE_TIME = 1.8f;
    glm::u8vec4 default_color = {0x6f, 0x6f, 0x6f, 0x6f};
    std::vector<RadarScanResult> radar_results;

    PlayMode *client_game;
    Radar(PlayMode *playMode)
    {
        client_game = playMode;
    }
    glm::u8vec4 get_radar_color(GameObject const *target);
    void render(DrawLines &hud);
    void update(float elapse);
    void scan(GameObject const *origin, float range, int count);
    Trace raycast_direction(const glm::vec2 origin, glm::vec2 direction, float range);
};
