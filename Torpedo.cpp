#include "Game.hpp"

#include "Connection.hpp"
#include "PlayMode.hpp"
#include "Scene.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#include "AABB.hpp"

void Torpedo::init()
{
    NetworkObject::init();
    type = ObjectType::Torpedo;
    scale = glm::vec2(0.5f, 0.5f);
    tracking = false;
    age = 0;
}

void Torpedo::update(float elapsed, Game *game)
{
    position += velocity * elapsed;
    age += elapsed;
    if (age > TORPEDO_LIFETIME)
    {
        deleted = true;
    }
}
