#pragma once
#include "Connection.hpp"
#include "Game.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Load.hpp"

#include <glm/glm.hpp>

struct Prefab
{
    Mesh mesh;
    std::string name;
    Prefab(std::string n);
    Prefab() {};

    Scene::Drawable *create_drawable(Scene &scene, glm::vec3 pos) const
    {
        return create_drawable(scene, pos, glm::vec3(1, 1, 1), glm::quat(0, 0, 0, 1));
    }

    Scene::Drawable *create_drawable(Scene &scene, glm::vec3 pos, glm::vec3 scale, glm::quat rotation) const;
};

extern Load<Prefab> prefab_player;
