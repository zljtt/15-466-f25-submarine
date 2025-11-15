#pragma once
#include "Load.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>
#include "Mesh.hpp"
#include "Scene.hpp"

#include "Sound.hpp"

struct Sprite
{
    GLuint tex;
    uint32_t width, height;
    Sprite(GLuint t, uint32_t w, uint32_t h) : tex(t), width(w), height(h) {};
};

extern GLuint meshes_for_lit_color_texture_program;

extern Load<Sprite> tex_obstacle;
extern Load<Sprite> tex_radar_blurred;
extern Load<Sprite> tex_radar_submarine;
extern Load<Sprite> tex_radar_flag;
extern Load<Sprite> tex_radar_radar;

extern Load<MeshBuffer> prototype_scene_meshes;
extern Load<Scene> prototype_scene;

struct UIRenderer;
extern Load<UIRenderer> renderer_gui;
extern Load<UIRenderer> renderer_radar;

extern Load<Sound::Sample> Submarine_Moving;
