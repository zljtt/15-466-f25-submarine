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
extern Load<MeshBuffer> prototype_prefab_meshes;

extern Load<Prefab> prefab_player;
extern Load<Prefab> prefab_torpedo;
extern Load<Prefab> prefab_flag;

static inline Scene::Drawable *create_drawable_at(Scene &scene, ObjectType type, glm::vec3 pos, glm::vec3 scale)
{
    switch (type)
    {
    case ObjectType::Player:
        return prefab_player->create_drawable(scene, pos, scale, glm::quat(0, 0, 0, 1));
    case ObjectType::Torpedo:
        return prefab_torpedo->create_drawable(scene, pos, scale, glm::quat(0, 0, 0, 1));
    case ObjectType::Flag:
        return prefab_torpedo->create_drawable(scene, pos, scale, glm::quat(0, 0, 0, 1));
    default:
        return nullptr;
    }
}