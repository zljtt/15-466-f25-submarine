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
