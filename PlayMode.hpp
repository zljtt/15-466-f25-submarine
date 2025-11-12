#pragma once

#include "Mode.hpp"

#include "Connection.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Sound.hpp"
#include "Prefab.hpp"
#include "Load.hpp"
#include "Raycast.hpp"
#include "Radar.hpp"
#include "TextEngine.hpp"
#include "UIRenderer.hpp"

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

    // overlay index
    static const int GUI = 0;
    static const int RADAR = 1;

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
    void update_sound(float elapsed);
    void update_spotlight(float elapsed);
    void update_ui(float elapsed);
    virtual void draw(glm::uvec2 const &drawable_size) override;
    glm::vec2 world_to_screen(glm::vec2 worldPos, const UIRenderer *renderer) const;
    glm::vec2 get_screen_size() const;
    //----- client game state -----
    Scene::Camera *camera = nullptr;
    std::unordered_map<uint32_t, Scene::Drawable *> network_drawables;
    Scene scene;

    // sounds

    std::unordered_map<uint32_t, std::shared_ptr<Sound::PlayingSample>> sub_moving;

    std::shared_ptr<Sound::PlayingSample> sub_start;
    std::shared_ptr<Sound::PlayingSample> sub_stop;
    std::shared_ptr<Sound::PlayingSample> flag_spawned;
    std::shared_ptr<Sound::PlayingSample> sub_hit;
    std::shared_ptr<Sound::PlayingSample> rad_scan;
    std::shared_ptr<Sound::PlayingSample> rad_detect;

    // helper for
    bool toPlay(uint8_t sc1, SoundCues sc2)
    {
        return sc1 & (uint8_t)sc2;
    }

    // client side function to play sound based on sound_cues
    void execute_network_soundcues(ObjectType type, uint8_t sc, glm::vec3 pos, uint32_t id);

    NetworkObject *local_player;
    // std::list<GameObject> local_obstacles;
    BVH bvh;

    std::unique_ptr<TextEngine> text_engine = nullptr;
    std::vector<UIOverlay> text_overlays;

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
extern Load<UIRenderer> ui_texts;
extern Load<UIRenderer> radar_text;
extern Load<GLuint> tex_radar_result;