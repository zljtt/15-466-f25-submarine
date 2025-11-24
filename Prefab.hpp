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
extern Load<Prefab> prefab_propeller;
extern Load<Prefab> prefab_ammo;
extern Load<Prefab> prefab_submit_point;

// extern Load<Prefab> prefab_spotlight_mesh;

static inline Scene::Drawable *create_drawable_at(Scene &scene, ObjectType type, glm::vec3 pos, glm::vec3 scale)
{
    switch (type)
    {
    case ObjectType::Player:
    {
        // glm::vec3 eulerAnglesDegrees = glm::vec3(-90.0f, 0.0f, 0.0f);
        // glm::vec3 eulerAnglesRadians = glm::radians(eulerAnglesDegrees);
        Scene::Drawable *propeller_drawable = prefab_propeller->create_drawable(scene, glm::vec3(0, 0, 0), glm::vec3(0.5f, 1, 1), glm::quat(1, 0, 0, 0));
        Scene::Drawable *player_drawable = prefab_player->create_drawable(scene, pos, scale, glm::quat(0, 0, 0, 1));
        propeller_drawable->transform->parent = player_drawable->transform;
        player_drawable->transform->child = propeller_drawable->transform;
        return player_drawable;
    }
    case ObjectType::Torpedo:
        return prefab_torpedo->create_drawable(scene, pos, scale, glm::quat(0, 0, 0, 1));
    case ObjectType::Flag:
        return prefab_torpedo->create_drawable(scene, pos, scale, glm::quat(0, 0, 0, 1));
    case ObjectType::SubmitPoint:
        return prefab_torpedo->create_drawable(scene, pos, scale, glm::quat(0, 0, 0, 1));
    case ObjectType::Ammo:
        return prefab_torpedo->create_drawable(scene, pos, scale, glm::quat(0, 0, 0, 1));
    default:
        return nullptr;
    }
}