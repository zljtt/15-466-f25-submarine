#pragma once
#include "Raycast.hpp"
#include "BBox.hpp"
#include "DrawLines.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>
#include <list>
#include <random>

struct PlayMode;

struct RadarPoint
{
    glm::vec2 direction;
    glm::vec4 color = glm::vec4(1.0f);
    bool touched = false;
    float bound = INFINITY; // max distance
    GLuint tex;
};

struct ScanPath
{
    std::vector<RadarPoint> points;
    glm::vec2 origin;
    glm::u8vec4 color;
    bool show_out_of_range = false;
    float distance;
    float max_distance;
    float speed;
};

struct ScanResult
{
    glm::vec2 point;
    glm::vec4 color;
    float size = 50.0f;
    float age = 0.0f;
    bool show_out_of_range = false;
    GLuint tex;
};

struct Radar
{
    static const int RADAR_RAY_COUNT = 40;
    static constexpr float RADAR_INTERVAL = 1.3f;
    static constexpr float RADAR_RANGE = 20.0f;
    static constexpr float RADAR_SPEED = 30.0f;
    static constexpr float RADAR_POINT_LIFE_TIME = 2.8f;
    static constexpr float RADAR_POINT_SIZE = 70.0f;

    glm::u8vec4 default_color = {0x6f, 0x6f, 0x6f, 0x6f};
    std::vector<ScanPath> paths;
    std::vector<ScanResult> results;
    float special_radar_timer;

    PlayMode *client_game;

    std::mt19937 gen;
    std::uniform_int_distribution<int> dist;
    Radar(PlayMode *playMode)
    {
        client_game = playMode;
        std::random_device rng;
        gen = std::mt19937(rng());
        dist = std::uniform_int_distribution<int>(-RADAR_POINT_SIZE * 0.1f, RADAR_POINT_SIZE * 0.1f);
    }

    glm::u8vec4 get_radar_color(GameObject const *target);
    void render(DrawLines &hud);
    void render_path(DrawLines &hud);
    void render_revealed_player();
    void render_results();

    void update(float elapse);
    void scan(GameObject const *origin, float range, int count);
    void scan_special(GameObject const *origin, float range);
    Trace raycast_direction(const glm::vec2 origin, glm::vec2 direction, float range);
};
