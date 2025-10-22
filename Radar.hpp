#pragma once
#include "Raycast.hpp"

#include <glm/glm.hpp>
#include <list>

struct Radar
{
    static const int RADAR_RAY_COUNT = 40;
    static constexpr float RADAR_INTERVAL = 1.0f;
    static constexpr float RADAR_RANGE = 10.0f;

    std::vector<RaycastResult> last_radar_hits;
    // all obstacles (but only serve as static right now)
    std::list<GameObject *> obstacles;

    // this is temp, since the game object of dynamic game objects are cleared and recreated each frame,
    // it is impossible to store their pointer, so just keep an additional list rn to keep track of the dynamic
    // will move to obstacles later when the game state sync is changed.
    std::list<GameObject> additional;

    Radar()
    {
    }

    void put_obstacles(std::list<GameObject> &objs)
    {
        for (GameObject &o : objs)
        {
            obstacles.push_back(&o);
        }
    }

    void scan(Player const *local_player, float range, int count);
    void raycast_surrounding(const glm::vec2 origin, float range, int count, std::vector<RaycastResult> &out);
    RaycastResult raycast_direction(const glm::vec2 origin, glm::vec2 direction, float range);
};
