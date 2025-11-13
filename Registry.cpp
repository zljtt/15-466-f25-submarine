#include "Registry.hpp"
#include "UIRenderer.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include "LitColorTextureProgram.hpp"
#include "ColorTextureProgram.hpp"

GLuint meshes_for_lit_color_texture_program = 0;

static Sprite *load_texture_from_png(const std::string &path)
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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
    return new Sprite(tex, size.x, size.y);
}

// ============= TEXTURE =============
Load<Sprite> tex_obstacle(LoadTagDefault, []() -> Sprite const *
                          { return load_texture_from_png(data_path("rock_material_basecolor_2.png")); });

Load<Sprite> tex_radar_blurred(LoadTagDefault, []() -> Sprite const *
                               { return load_texture_from_png(data_path("radar_spot.png")); });

Load<Sprite> tex_radar_submarine(LoadTagDefault, []() -> Sprite const *
                                 { return load_texture_from_png(data_path("submarine_icon.png")); });

Load<Sprite> tex_radar_flag(LoadTagDefault, []() -> Sprite const *
                            { return load_texture_from_png(data_path("flag_icon.png")); });

Load<Sprite> tex_radar_radar(LoadTagDefault, []() -> Sprite const *
                             { return load_texture_from_png(data_path("radar_icon.png")); });

// ============= SCENE AND MESH =============
Load<MeshBuffer> prototype_scene_meshes(LoadTagDefault, []() -> MeshBuffer const *
                                        {
	MeshBuffer const *ret = new MeshBuffer(data_path("prototype_scene.pnct"));
	meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

Load<Scene> prototype_scene(LoadTagDefault, []() -> Scene const *
                            { 
    auto on_drawable = [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name) {
        if (mesh_name == "Player") return;
        if (mesh_name == "Torpedo") return;
        Mesh const &mesh = prototype_scene_meshes->lookup(mesh_name);
        scene.drawables.emplace_back(transform);
        Scene::Drawable &drawable = scene.drawables.back();

        drawable.pipeline = lit_color_texture_program_pipeline;
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

// ============= SOUND =============
Load<Sound::Sample> Submarine_Moving(LoadTagEarly, []() -> Sound::Sample const *
                                     { return new Sound::Sample(data_path("sub_moving.wav")); });
