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

struct AABB
{
    glm::vec2 min, max;
};

struct Networkable
{
    uint32_t id;
    Networkable() {};
    virtual ~Networkable() {};
    virtual void send(Connection *connection) const {};
    virtual void receive(uint32_t *at, std::vector<uint8_t> &recv_buffer) {};
};

struct GameObject
{
    glm::vec2 position = glm::vec2(0.0f, 0.0f);
    glm::vec2 scale = glm::vec2(1.0f, 1.0f);
    glm::vec2 velocity = glm::vec2(0.0f, 0.0f);
    uint8_t radar_color = 0;

    GameObject() {};
    GameObject(glm::vec2 pos, glm::vec2 box) : position(pos), scale(box) {};

    virtual void init() {};
    virtual void update() {};

    AABB get_aabb() const
    {
        return {position - scale, position + scale};
    }

    virtual ~GameObject() {};
};

struct NetworkObject : GameObject, Networkable
{

    NetworkObject() {};
    virtual ~NetworkObject() {};

    virtual void init() override;
    virtual void send(Connection *connection) const override;
    virtual void receive(uint32_t *at, std::vector<uint8_t> &recv_buffer) override;
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

    virtual void init() override;
    virtual void send(Connection *connection) const override;
    virtual void receive(uint32_t *at, std::vector<uint8_t> &recv_buffer) override;
};
