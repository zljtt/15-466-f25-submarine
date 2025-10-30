#pragma once
#include "Raycast.hpp"

#include <glm/glm.hpp>
#include <list>
struct PlayMode;

struct Radar
{
    static const int RADAR_RAY_COUNT = 40;
    static constexpr float RADAR_INTERVAL = 1.0f;
    static constexpr float RADAR_RANGE = 10.0f;

    std::vector<RaycastResult> last_radar_hits;
    PlayMode *client_game;

    Radar(PlayMode *playMode)
    {
        client_game = playMode;
    }

    void scan(GameObject const *origin, float range, int count);
    void raycast_surrounding(const glm::vec2 origin, float range, int count, std::vector<RaycastResult> &out);
    RaycastResult raycast_direction(const glm::vec2 origin, glm::vec2 direction, float range);
};
