#pragma once
#include "Connection.hpp"

#include <glm/glm.hpp>

#include <string>
#include <list>
#include <random>
#include <vector>

static std::mt19937 mt(0x15466666); // used for spawning players
static std::uniform_int_distribution<uint32_t> dist(1u, 0xFFFFFFFFu);
static uint32_t next_player_number = 1; // used for naming players

struct Game;

enum class ObjectType : uint8_t
{
    Player = 0,
    Obstacle = 1,
    Torpedo = 2
};

struct AABB
{
    glm::vec2 min, max;
};

/**
 * The base game object class for all objects on map
 */
struct GameObject
{
    // common
    ObjectType type = ObjectType::Obstacle;
    glm::vec2 position = glm::vec2(0.0f, 0.0f);
    glm::vec2 scale = glm::vec2(1.0f, 1.0f);

    GameObject() {};
    GameObject(glm::vec2 pos, glm::vec2 box) : position(pos), scale(box) {};

    virtual void init() {};
    virtual void update(float elapsed, Game *game) {};

    AABB get_aabb() const
    {
        return {position - scale, position + scale};
    }

    virtual ~GameObject() {};
};

/**
 * The objects that need to sync between client and server
 * all varaibles not in NetworkObject and GameObject will not sync
 */
struct NetworkObject : GameObject
{
    // common
    uint32_t id;
    glm::vec2 velocity = glm::vec2(0.0f, 0.0f);

    // server only
    bool deleted = false;

    NetworkObject() {};
    virtual ~NetworkObject() {};
    virtual void init() override;
    void send(Connection *connection) const;
    void receive(uint32_t *at, std::vector<uint8_t> &recv_buffer);
};

// used to represent a control input:
struct Button
{
    uint8_t downs = 0;    // times the button has been pressed
    bool pressed = false; // is the button pressed now
};

// state of one player in the game:
struct Player : NetworkObject
{

    Player() {};
    virtual ~Player() {};

    static constexpr float MAX_SPEED = 10.0f;
    static constexpr float ACCEL_RATE = 30.0f;
    static constexpr float DECEL_RATE = 20.0f;
    static constexpr float DRAG_S = 1.5f;
    static constexpr float TORPEDO_COOLDOWN = 2.0f;

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

    // additional player data
    struct PlayerData
    {
        float torpedo_timer;
    } data;

    virtual void init() override;
    virtual void update(float elapsed, Game *game) override;
    void update_weapon(float elapsed, Game *game);
    void update_movement(float elapsed, Game *game, glm::vec2 control);
};

struct Torpedo : NetworkObject
{
    Torpedo() {};
    virtual ~Torpedo() {};
    // time till torpedo explode on its own
    static constexpr float TORPEDO_LIFETIME = 1.0f;
    static constexpr float TORPEDO_SPEED = 15.0f;

    // Torpedo states
    bool tracking; // if the torpedo could detect other submarines, switch to true
    float age;

    virtual void update(float elapsed, Game *game) override;
    virtual void init() override;
};
