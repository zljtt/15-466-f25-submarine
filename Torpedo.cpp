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

int Torpedo::can_collide(const NetworkObject *other) const
{
    if (other->id == this->id)
        return 0;
    // don't collide with owner
    if (other->id == this->owner)
        return 0;

    return 1;
}

void Torpedo::update(float elapsed, Game *game)
{
    auto hits = move_with_collision(game, velocity * elapsed);

    age += elapsed;
    if (age > TORPEDO_LIFETIME)
    {
        deleted = true;
    }
    auto player_hit = get_colliders<Player>(hits);
    if (player_hit)
    {
        player_hit->take_damage(TORPEDO_DAMAGE, this);
        deleted = true;
    }
}
