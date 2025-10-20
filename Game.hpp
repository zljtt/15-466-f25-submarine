#pragma once

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <string>
#include <list>
#include <random>
#include <vector>

struct Connection;

// Game state, separate from rendering.

// Currently set up for a "client sends controls" / "server sends whole state" situation.

enum class Message : uint8_t
{
    C2S_Controls = 1, // Greg!
    S2C_State = 's',
    //...
};

// used to represent a control input:
struct Button
{
    uint8_t downs = 0;    // times the button has been pressed
    bool pressed = false; // is the button pressed now
};

struct NetworkObject
{
    uint32_t id;
    glm::vec2 position = glm::vec2(0.0f, 0.0f);
    glm::vec2 velocity = glm::vec2(0.0f, 0.0f);
    NetworkObject() {};
    virtual ~NetworkObject() {};
    virtual void send(Connection *connection) const;
    virtual void receive(uint32_t *at, std::vector<uint8_t> &recv_buffer);
};

// state of one player in the game:
struct Player : NetworkObject
{
    Player() {};
    virtual ~Player() {};
    // player inputs (sent from client):
    struct Controls
    {
        Button left, right, up, down, jump;

        void send_controls_message(Connection *connection) const;

        // returns 'false' if no message or not a controls message,
        // returns 'true' if read a controls message,
        // throws on malformed controls message
        bool recv_controls_message(Connection *connection);
    } controls;

    // player state (sent from server):
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    std::string name = "";

    void send(Connection *connection) const override;
    void receive(uint32_t *at, std::vector<uint8_t> &recv_buffer) override;
};

struct Game
{
    std::list<Player> players; //(using list so they can have stable addresses)
    uint32_t local_player;
    // for server
    std::list<NetworkObject> game_objects;
    // for client

    Player *spawn_player(); // add player the end of the players list (may also, e.g., play some spawn anim)
    NetworkObject *spawn_object();
    void remove_player(Player *); // remove player from game (may also, e.g., play some despawn anim)
    void remove_object(NetworkObject *);

    std::mt19937 mt; // used for spawning players
    std::uniform_int_distribution<uint32_t> dist;
    uint32_t next_player_number = 1; // used for naming players

    Game();

    // state update function:
    void update(float elapsed);

    // constants:
    // the update rate on the server:
    inline static constexpr float Tick = 1.0f / 30.0f;

    // arena size:
    inline static constexpr glm::vec2 ArenaMin = glm::vec2(-0.75f, -1.0f);
    inline static constexpr glm::vec2 ArenaMax = glm::vec2(0.75f, 1.0f);

    // player constants:
    inline static constexpr float PlayerRadius = 0.06f;
    inline static constexpr float PlayerSpeed = 2.0f;
    inline static constexpr float PlayerAccelHalflife = 0.25f;

    //---- communication helpers ----

    // used by client:
    // set game state from data in connection buffer
    //  (return true if data was read)
    bool recv_state_message(Connection *connection, std::unordered_map<uint32_t, Scene::Drawable *> &game_objects, Scene &scene);

    // used by server:
    // send game state.
    //   Will move "connection_player" to the front of the front of the sent list.
    void send_state_message(Connection *connection, Player *connection_player = nullptr) const;
};
