#include "Game.hpp"

#include "Connection.hpp"
#include "PlayMode.hpp"
#include "Scene.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#include "BBox.hpp"

void Flag::init()
{
    NetworkObject::init();
    type = ObjectType::Flag;
    scale = glm::vec2(0.5f, 0.5f);
}

void Flag::update(float elapsed, Game *game)
{
}

void SubmitPoint::init()
{
    NetworkObject::init();
    type = ObjectType::SubmitPoint;
    scale = glm::vec2(0.0f, 0.0f);
}

void SubmitPoint::update(float elapsed, Game *game)
{
}

void Ammo::init()
{
    NetworkObject::init();
    type = ObjectType::Ammo;
    scale = glm::vec2(1.0f, 1.0f);
}

void Ammo::update(float elapsed, Game *game)
{
}
