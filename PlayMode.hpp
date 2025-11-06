#pragma once

#include "Mode.hpp"

#include "Connection.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Prefab.hpp"
#include "Load.hpp"
#include "Raycast.hpp"
#include "Radar.hpp"
#include "TextEngine.hpp"
#include "TextComponent.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <array>

extern Load<Prefab> prefab_player;
extern Load<Prefab> prefab_torpedo;

struct PlayMode : Mode
{
    // UI keys
    static const std::string HP;

    static const int Flag = 1;

    PlayMode(Client &client);
    virtual ~PlayMode();

    // functions called by main loop:
    virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
    virtual void update(float elapsed) override;
    void update_control(float elapsed);
    void update_connection(float elapsed);
    void update_radar(float elapsed);
    void update_camera(float elapsed);
    void update_spotlight(float elapsed);
    void update_ui(float elapsed);
    virtual void draw(glm::uvec2 const &drawable_size) override;
    glm::vec2 world_to_screen(const glm::vec3 &worldPos) const;
    glm::vec2 get_screen_size() const;
    //----- client game state -----
    Scene::Camera *camera = nullptr;
    std::unordered_map<uint32_t, Scene::Drawable *> network_drawables;
    Scene scene;

    NetworkObject *local_player;
    // std::list<GameObject> local_obstacles;
    BVH bvh;

    std::unique_ptr<TextEngine> text_engine = nullptr;
    std::vector<TextOverlay> text_overlays;

    std::list<NetworkObject> network_objects;

    Radar radar;
    float radar_timer;

    // input tracking for local player:
    Player::Controls controls;
    // data for local player;
    std::unordered_map<uint32_t, Player::PlayerData> player_data;

    // last message from server:
    std::string server_message;

    // connection to server:
    Client &client;

    // helper functions
    void draw_overlay(glm::uvec2 const &drawable_size);
    glm::vec2 local_player_pos();

    float water_surface_y = 220.0f;
    float atten_speed = 0.02f;
    float cutoff = glm::radians(20.0f);
    glm::vec2 prev_player_pos = glm::vec2(0.0f);
    float spot_light_dir_x = 0.0f;

    // used by client:
    // set game state from data in connection buffer
    //  (return true if data was read)
    bool recv_state_message(Connection *connection);

    std::vector<NetworkObject> get_objects(ObjectType type) const;
};

extern GLuint meshes_for_lit_color_texture_program;
extern Load<MeshBuffer> prototype_scene_meshes;
extern Load<Scene> prototype_scene;
