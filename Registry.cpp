#include "Registry.hpp"
#include "UIRenderer.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include "LitColorTextureProgram.hpp"
#include "ColorTextureProgram.hpp"
#include "BasicMaterialForwardProgram.hpp"

GLuint meshes_for_lit_color_texture_program = 0;

static Sprite *load_texture_from_png(const std::string &path, bool use_mipmap, bool repeat)
{
    glm::uvec2 size;
    std::vector<glm::u8vec4> data;
    load_png(path, &size, &data, UpperLeftOrigin);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 size.x, size.y, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, use_mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, use_mipmap ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    if (use_mipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
    return new Sprite(tex, size.x, size.y);
}

// ============= TEXTURE =============
Load<Sprite> tex_obstacle(LoadTagDefault, []() -> Sprite const *
                          { return load_texture_from_png(data_path("rock_material_basecolor_2.png"), true, true); });

Load<Sprite> tex_radar_blurred(LoadTagDefault, []() -> Sprite const *
                               { return load_texture_from_png(data_path("radar_spot.png"), true, false); });

Load<Sprite> tex_radar_submarine(LoadTagDefault, []() -> Sprite const *
                                 { return load_texture_from_png(data_path("submarine_icon.png"), false, false); });

Load<Sprite> tex_radar_flag(LoadTagDefault, []() -> Sprite const *
                            { return load_texture_from_png(data_path("flag_icon.png"), false, false); });

Load<Sprite> tex_radar_radar(LoadTagDefault, []() -> Sprite const *
                             { return load_texture_from_png(data_path("radar_icon.png"), false, false); });

Load<Sprite> tex_radar_submit_point(LoadTagDefault, []() -> Sprite const *
                                    { return load_texture_from_png(data_path("submit_point_icon.png"), false, false); });

Load<Sprite> ui_select_ship(LoadTagDefault, []() -> Sprite const *
                            { return load_texture_from_png(data_path("ui_select_ship.png"), false, false); });

// ============= SCENE AND MESH =============
Load<MeshBuffer> prototype_scene_meshes(LoadTagDefault, []() -> MeshBuffer const *
                                        {
	MeshBuffer const *ret = new MeshBuffer(data_path("prototype_scene.pnct"));
	// meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
    meshes_for_lit_color_texture_program = ret->make_vao_for_program(basic_material_forward_program->program);
	return ret; });

Load<Scene> prototype_scene(LoadTagDefault, []() -> Scene const *
                            { 
    auto on_drawable = [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name) {
        if (mesh_name == "Player") return;
        if (mesh_name == "Torpedo") return;
        if (mesh_name == "Propeller") return;
        Mesh const &mesh = prototype_scene_meshes->lookup(mesh_name);
        scene.drawables.emplace_back(transform);
        Scene::Drawable &drawable = scene.drawables.back();


        std::cout<<" scene loading "<<mesh_name<<std::endl; 
        // drawable.pipeline = lit_color_texture_program_pipeline;
        drawable.pipeline = basic_material_forward_program_pipeline;
        drawable.pipeline.vao = meshes_for_lit_color_texture_program;
        drawable.pipeline.type = mesh.type;
        drawable.pipeline.start = mesh.start;
        drawable.pipeline.count = mesh.count;

        drawable.pipeline.textures[0].target = GL_TEXTURE_2D;
        drawable.pipeline.textures[0].texture = tex_obstacle->tex;
    };
    return new Scene(data_path("prototype.scene"), on_drawable); });

// ============= IMAGE AND TEXT =============
Load<UIRenderer> renderer_gui(LoadTagDefault, []() -> UIRenderer const *
                              {
	UIRenderer *ret = new UIRenderer(data_path("DejaVuSans.ttf").c_str(), 16);
    ret->init(1280, 720,
        color_texture_program->program,
        color_texture_program->CLIP_FROM_OBJECT_mat4,
        color_texture_program->Position_vec4,
        color_texture_program->Color_vec4,
        color_texture_program->TexCoord_vec2);
    return ret; });

Load<UIRenderer> renderer_radar(LoadTagDefault, []() -> UIRenderer const *
                                {
	UIRenderer *ret = new UIRenderer(data_path("DejaVuSans.ttf").c_str(), 24);
    ret->init(1280, 720,
        color_texture_program->program,
        color_texture_program->CLIP_FROM_OBJECT_mat4,
        color_texture_program->Position_vec4,
        color_texture_program->Color_vec4,
        color_texture_program->TexCoord_vec2);
    return ret; });

Load<UIRenderer> renderer_notification(LoadTagDefault, []() -> UIRenderer const *
                                       {
	UIRenderer *ret = new UIRenderer(data_path("DejaVuSans.ttf").c_str(), 16);
    ret->init(1280, 720,
        color_texture_program->program,
        color_texture_program->CLIP_FROM_OBJECT_mat4,
        color_texture_program->Position_vec4,
        color_texture_program->Color_vec4,
        color_texture_program->TexCoord_vec2);
    return ret; });

Load<UIRenderer> renderer_large(LoadTagDefault, []() -> UIRenderer const *
                                {
	UIRenderer *ret = new UIRenderer(data_path("DejaVuSans.ttf").c_str(), 24);
    ret->init(1280, 720,
        color_texture_program->program,
        color_texture_program->CLIP_FROM_OBJECT_mat4,
        color_texture_program->Position_vec4,
        color_texture_program->Color_vec4,
        color_texture_program->TexCoord_vec2);
    return ret; });

// ============= SOUND =============
Load<Sound::Sample> Submarine_Moving(LoadTagEarly, []() -> Sound::Sample const *
                                     { return new Sound::Sample(data_path("sub_moving.wav")); });
Load<Sound::Sample> Submarine_Start(LoadTagEarly, []() -> Sound::Sample const *
                                    { return new Sound::Sample(data_path("sub_start.wav")); });
Load<Sound::Sample> Submarine_Stop(LoadTagEarly, []() -> Sound::Sample const *
                                   { return new Sound::Sample(data_path("sub_stop.wav")); });
Load<Sound::Sample> Submarine_Hit(LoadTagEarly, []() -> Sound::Sample const *
                                  { return new Sound::Sample(data_path("sub_hit.wav")); });
Load<Sound::Sample> Submarine_Scan1(LoadTagEarly, []() -> Sound::Sample const *
                                    { return new Sound::Sample(data_path("sub_scan1.wav")); });
Load<Sound::Sample> Submarine_Scan2(LoadTagEarly, []() -> Sound::Sample const *
                                    { return new Sound::Sample(data_path("sub_scan2.wav")); });
Load<Sound::Sample> Submarine_Scan3(LoadTagEarly, []() -> Sound::Sample const *
                                    { return new Sound::Sample(data_path("sub_scan3.wav")); });
Load<Sound::Sample> Submarine_Scan_Enemy(LoadTagEarly, []() -> Sound::Sample const *
                                         { return new Sound::Sample(data_path("sub_scan_enemy.wav")); });
