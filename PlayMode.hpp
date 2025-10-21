#pragma once

#include "Mode.hpp"

#include "Connection.hpp"
#include "Game.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Prefab.hpp"
#include "Load.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <array>

extern Load<Prefab> prefab_player;

struct PlayMode : Mode
{
    PlayMode(Client &client);
    virtual ~PlayMode();

    // functions called by main loop:
    virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
    virtual void update(float elapsed) override;
    virtual void update_control(float elapsed);
    virtual void draw(glm::uvec2 const &drawable_size) override;

    //----- game state -----
    Scene::Camera *camera = nullptr;
    Scene scene;
    std::unordered_map<uint32_t, Scene::Drawable *> drawables;

    // input tracking for local player:
    Player::Controls controls;

    // latest game state (from server):
    Game game;

    // last message from server:
    std::string server_message;

    // connection to server:
    Client &client;
};

extern GLuint meshes_for_lit_color_texture_program;
extern Load<MeshBuffer> prototype_scene_meshes;
extern Load<Scene> prototype_scene;
