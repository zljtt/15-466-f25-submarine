#include "Game.hpp"

#include "Connection.hpp"
#include "PlayMode.hpp"
#include "Scene.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#include "BBox.hpp"

void Torpedo::init()
{
    NetworkObject::init();
    type = ObjectType::Torpedo;
    scale = glm::vec2(0.5f, 0.5f);
    tracking = false;
    age = 0;
}

bool Torpedo::can_collide(const NetworkObject *other) const
{
    if (other->id == this->id)
        return false;
    // don't collide with owner
    if (other->id == this->owner)
        return false;

    return true;
}

void Torpedo::update(float elapsed, Game *game)
{
    auto hits = move_with_collision(game, velocity * elapsed);

    age += elapsed;
    if (age > TORPEDO_LIFETIME)
    {
        deleted = true;
    }
    if (hits.size() > 0)
    {
        for (auto hit : hits)
        {
            Player *opponent = dynamic_cast<Player *>(hit);
            if (opponent)
            {
                std::cout << "Hit opponent \n";
            }
        }
        deleted = true;
    }
}
