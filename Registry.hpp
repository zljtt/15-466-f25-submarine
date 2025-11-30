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
extern Load<Sprite> tex_radar_submit_point;
extern Load<Sprite> ui_select_ship;

extern Load<MeshBuffer> prototype_scene_meshes;
extern Load<Scene> prototype_scene;

struct UIRenderer;
extern Load<UIRenderer> renderer_gui;
extern Load<UIRenderer> renderer_radar;
extern Load<UIRenderer> renderer_notification;
extern Load<UIRenderer> renderer_large;

extern Load<Sound::Sample> Submarine_Moving;
extern Load<Sound::Sample> Submarine_Start;
extern Load<Sound::Sample> Submarine_Stop;
extern Load<Sound::Sample> Submarine_Hit;
extern Load<Sound::Sample> Submarine_Scan1;
extern Load<Sound::Sample> Submarine_Scan2;
extern Load<Sound::Sample> Submarine_Scan3;
extern Load<Sound::Sample> Submarine_Scan_Enemy;
extern Load<Sound::Sample> Submarine_Launch_Torpedo;
extern Load<Sound::Sample> Submarine_Superscan;
extern Load<Sound::Sample> Submarine_Submit;
extern Load<Sound::Sample> Shallow_BGM;
extern Load<Sound::Sample> Deep_BGM;
extern Load<Sound::Sample> Torpedo_Hit;