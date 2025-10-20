#include "PlayMode.hpp"
#include "Connection.hpp"
#include "Game.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "LitColorTextureProgram.hpp"

#include <glm/glm.hpp>

Load<Prefab> prefab_player(LoadTagLate, []() -> Prefab const *
                           { return new Prefab("Player"); });

Scene::Drawable *Prefab::create_drawable(Scene &scene, glm::vec3 pos, glm::vec3 scale, glm::quat rotation) const
{
    // create transform
    Scene::Transform &transform = scene.transforms.emplace_back();
    Scene::Transform *transform_p = &transform;
    transform_p->position = pos;
    transform_p->scale = scale;
    transform_p->rotation = rotation;

    // create drawables
    scene.drawables.emplace_back(transform_p);
    Scene::Drawable &drawable = scene.drawables.back();

    drawable.pipeline = lit_color_texture_program_pipeline;
    drawable.pipeline.vao = meshes_for_lit_color_texture_program;
    drawable.pipeline.type = mesh.type;
    drawable.pipeline.start = mesh.start;
    drawable.pipeline.count = mesh.count;
    return &drawable;
}

Prefab::Prefab(std::string n) : name(n)
{
    mesh = meshes->lookup(n);
};