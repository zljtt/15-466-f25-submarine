#include "PlayMode.hpp"
#include "Connection.hpp"
#include "Game.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "LitColorTextureProgram.hpp"
#include "BasicMaterialForwardProgram.hpp"
#include "data_path.hpp"

#include <glm/glm.hpp>
#include "Registry.hpp"

GLuint prefab_meshes_for_lit_color_texture_program = 0;

Load<MeshBuffer> prototype_prefab_meshes(LoadTagDefault, []() -> MeshBuffer const *
                                         {
	MeshBuffer const *ret = new MeshBuffer(data_path("prototype_prefab.pnct"));
	// meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
    prefab_meshes_for_lit_color_texture_program = ret->make_vao_for_program(basic_material_forward_program->program);
	return ret; });

Load<Prefab> prefab_player(LoadTagLate, []() -> Prefab const *
                           { return new Prefab("Player"); });

Load<Prefab> prefab_torpedo(LoadTagLate, []() -> Prefab const *
                            { return new Prefab("Torpedo"); });

Load<Prefab> prefab_flag(LoadTagLate, []() -> Prefab const *
                         { return new Prefab("Flag"); });

Load<Prefab> prefab_ammo(LoadTagLate, []() -> Prefab const *
                         { return new Prefab("Flag"); });

Load<Prefab> prefab_submit_point(LoadTagLate, []() -> Prefab const *
                                 { return new Prefab("Flag"); });

Load<Prefab> prefab_propeller(LoadTagLate, []() -> Prefab const *
                              { return new Prefab("Propeller"); });

Load<Prefab> prefab_torpedo_head(LoadTagLate, []() -> Prefab const *
                              { return new Prefab("Torpedo_head"); });

// Load<Prefab> prefab_spotlight_mesh(LoadTagLate, []() -> Prefab const *
//                          { return new Prefab("SpotlightMesh"); });
Load<Prefab> prefab_spotlight_mesh(LoadTagLate, []() -> Prefab const *
                         { return new Prefab("SpotlightMesh"); });

Scene::Drawable *Prefab::create_drawable(Scene &scene, glm::vec3 pos, glm::vec3 scale, glm::quat rotation) const
{

    // create transform
    Scene::Transform &transform = scene.transforms.emplace_back();
    Scene::Transform *transform_p = &transform;
    transform_p->position = pos;
    transform_p->scale = scale;
    transform_p->rotation = rotation;

    if (name == "Player")
    {
        // std::cout<<"creating drawable with count "<<mesh.count<<" and lower bbox"<<mesh.min.x<<" "<<mesh.min.y<<" "<<mesh.min.z<<" "<<std::endl;
        // std::cout<<"mesh start: "<<mesh.start<<std::endl;
    }

    // create drawables
    scene.drawables.emplace_back(transform_p);
    Scene::Drawable &drawable = scene.drawables.back();

    // drawable.pipeline = lit_color_texture_program_pipeline;
    drawable.pipeline = basic_material_forward_program_pipeline;
    drawable.pipeline.vao = prefab_meshes_for_lit_color_texture_program;
    drawable.pipeline.vao = prefab_meshes_for_lit_color_texture_program;
    drawable.pipeline.type = mesh.type;
    drawable.pipeline.start = mesh.start;
    drawable.pipeline.count = mesh.count;
    // std::cout << mesh.count << "\n";
    return &drawable;
}

Prefab::Prefab(std::string n) : name(n)
{
    mesh = prototype_prefab_meshes->lookup(n);
    // std::cout<<"loading "<<n<<" whose count is "<<mesh.count<<" mesh lower bouding box: "<<mesh.min.x<<" "<<mesh.min.y<<" "<<mesh.min.z<<std::endl;
};
