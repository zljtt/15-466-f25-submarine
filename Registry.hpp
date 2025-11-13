#pragma once
#include "Load.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>
#include "Mesh.hpp"
#include "Scene.hpp"

#include "Sound.hpp"

extern GLuint meshes_for_lit_color_texture_program;

extern Load<GLuint> tex_obstacle;
extern Load<GLuint> tex_radar_blurred;
extern Load<GLuint> tex_radar_submarine;
extern Load<GLuint> tex_radar_flag;

extern Load<MeshBuffer> prototype_scene_meshes;
extern Load<Scene> prototype_scene;

struct UIRenderer;
extern Load<UIRenderer> renderer_gui;
extern Load<UIRenderer> renderer_radar;

extern Load<Sound::Sample> Submarine_Moving;
