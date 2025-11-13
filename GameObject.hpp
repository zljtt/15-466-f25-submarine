#pragma once
#include "Connection.hpp"
#include "BBox.hpp"
#include "Raycast.hpp"

#include <iostream>
#include <glm/glm.hpp>
#include <string>
#include <list>
#include <vector>

struct Game;

enum class ObjectType : uint8_t
{
    Obstacle = 0,
    Player = 1,
    Torpedo = 2,
    Flag = 3
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
    glm::vec2 velocity = glm::vec2(0.0f, 0.0f);

    GameObject() {};
    GameObject(glm::vec2 pos, glm::vec2 box) : position(pos), scale(box)
    {
        if (box.x < 0)
            box.x = -box.x;
        if (box.y < 0)
            box.y = -box.y;
        velocity = glm::vec2(0.0f, 0.0f);
    };

    virtual void init() {};
    virtual void update(float elapsed, Game *game) {};

    BBox get_BBox() const
    {
        return {position - scale, position + scale};
    }
    Trace hit(Ray2D ray) const;
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

    uint8_t sound_cues = 0;
    // server side function to add a cue to send
    void add_sound_cue(uint8_t s)
    {
        std::cout << "adding sound cue" << static_cast<int>(s) << "to" << static_cast<int>(sound_cues) << std::endl;
        sound_cues |= s;
        std::cout << "now " << id << " has " << static_cast<int>(sound_cues) << std::endl;
    }

    // server only, set to true to mark this object as deleted
    // it will be deleted at the end of this frame
    bool deleted = false;

    NetworkObject() {};
    virtual ~NetworkObject() {};
    virtual void init() override;
    void gather_collision_candidates(Game *game, const BBox &sweepBox, std::vector<GameObject *> &out);
    /**
     * 0: collide check fail, don't collide with the other
     * 1: can collide, can't move into the other
     * 2: can collide, and can move into the other
     */
    virtual int can_collide(const NetworkObject *other) const;
    std::vector<GameObject *> move_with_collision(Game *game, glm::vec2 movement);
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

    // movement
    static constexpr float MAX_SPEED = 10.0f;
    static constexpr float ACCEL_RATE = 20.0f;
    static constexpr float DECEL_RATE = 5.0f;
    static constexpr float DRAG_S = 0.5f;
    // combat
    static constexpr float TORPEDO_COOLDOWN = 1.0f;
    static constexpr float MAX_HEALTH = 100.0f;
    static constexpr float COLLISION_DAMAGAE = 10.0f;
    static constexpr float SUPER_RADAR_EXPOSURE_TIME = 5.0f;

    // player inputs (sent from client):
    struct Controls
    {
        Button left, right, up, down, jump, radar;

        void send_controls_message(Connection *connection) const;

        // returns 'false' if no message or not a controls message,
        // returns 'true' if read a controls message,
        // throws on malformed controls message
        bool recv_controls_message(Connection *connection);
    } controls;

    // additional player data
    struct PlayerData
    {
        float torpedo_timer = 0.0f;
        bool player_facing = false; // false is left, true is right
        float hp = MAX_HEALTH;
        bool has_flag = false;
        int flag_count = 0;
        glm::vec2 spawn_pos;

        bool engineStarted = false;

        void send(Connection *connection) const;
        void receive(uint32_t *at, std::vector<uint8_t> &recv_buffer);
    } data;

    virtual void init() override;
    virtual int can_collide(const NetworkObject *other) const override;
    virtual void update(float elapsed, Game *game) override;
    void update_weapon(float elapsed, Game *game);
    void update_movement(float elapsed, Game *game);
    void update_control(float elapsed, Game *game);
    void update_win_lose(float elapsed, Game *game);
    void take_damage(Game *game, float damage, GameObject *source);
    void die(Game *game, GameObject *source);
};

struct Torpedo : NetworkObject
{
    Torpedo() {};
    virtual ~Torpedo() {};
    // time till torpedo explode on its own
    static constexpr float TORPEDO_LIFETIME = 3.0f;
    static constexpr float TORPEDO_SPEED = 15.0f;
    static constexpr float TORPEDO_DAMAGE = 30.0f;

    // Torpedo states
    uint32_t owner;
    bool tracking; // if the torpedo could detect other submarines, switch to true
    float age;

    virtual int can_collide(const NetworkObject *other) const override;
    virtual void update(float elapsed, Game *game) override;
    virtual void init() override;
};

struct Flag : NetworkObject
{
    Flag() {};
    virtual ~Flag() {};

    // virtual bool can_collide(const NetworkObject *other) const override;
    virtual void update(float elapsed, Game *game) override;
    virtual void init() override;
};

/**
 * @brief Find the colliders according to the given object type
 */
template <typename O>
static inline O *get_colliders(std::vector<GameObject *> &all_hits)
{
    static_assert(std::is_base_of_v<GameObject, O>, "Can only spawn game object");
    if (all_hits.size() > 0)
    {
        for (auto hit : all_hits)
        {
            O *other = dynamic_cast<O *>(hit);
            if (other)
            {
                return other;
            }
        }
    }
    return nullptr;
}

static inline GameObject *get_colliders(std::vector<GameObject *> &all_hits, ObjectType type)
{
    if (all_hits.size() > 0)
    {
        for (auto hit : all_hits)
        {
            if (hit->type == type)
            {
                return hit;
            }
        }
    }
    return nullptr;
}
