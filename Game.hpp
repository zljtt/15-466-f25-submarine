#pragma once

#include "Scene.hpp"
#include "GameObject.hpp"
#include "Raycast.hpp"
#include "Sound.hpp"
#include "Level.hpp"

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
    S2C_Notification = 'n',
    C2S_Data = 'd',
    //...
};

enum class SoundCues : uint8_t
{
    Start = 1 << 0,
    Stop = 1 << 1,
    Hit = 1 << 2,
    JustSpawned = 1 << 3,
    GetPoint = 1 << 4,
    Hit_TORP = 1 << 5,
    Capture = 1 << 6
};

struct Game
{
    static const int MaxBlackBox = 3;
    std::mt19937 mt{0x15466666}; // used for spawning players
    std::uniform_int_distribution<uint32_t> dist{1u, 0xFFFFFFFFu};

    uint32_t next_player_number = 1; // used for naming players
    std::unordered_map<Connection *, Player *> connection_to_player;
    std::list<GameObject> static_obstacles;  // the collision box should not be sync, instead generated from the scene on both server and client (if the client needs it)
    std::list<NetworkObject *> game_objects; // the dynamic game object sync to from server to client
    BVH bvh;

    Level level;

    float flag_spawn_timer = 0;
    template <typename O>
    O *spawn_object()
    {
        static_assert(std::is_base_of_v<NetworkObject, O>, "Can only spawn network game object");
        O *obj = new O();
        // NetworkObject *no = static_cast<NetworkObject *>(obj);
        game_objects.push_back(obj);
        obj->id = dist(mt);
        obj->init();
        return obj;
    }

    template <typename O>
    std::vector<O *> get_objects() const
    {
        static_assert(std::is_base_of_v<NetworkObject, O>, "Can only get network game object");
        std::vector<O *> ret;
        ret.clear();
        for (auto obj : game_objects)
        {
            O *no = dynamic_cast<O *>(obj);
            if (no)
            {
                ret.push_back(no);
            }
        }
        return ret;
    }
    /**
     * Don't call this to remove object, instead mark the object as deleted
     */
    void remove_object(uint32_t id);
    Game();
    ~Game();
    // state update function:
    void update(float elapsed);
    void init_player_spawn_info(Player *player);

    // constants:
    // the update rate on the server:

    inline static glm::vec2 SpawnPos[4] = {glm::vec2(30, 490),
                                           glm::vec2(112, 490),
                                           glm::vec2(212, 490),
                                           glm::vec2(262, 490)};

    inline static constexpr float Tick = 1.0f / 30.0f;

    inline static constexpr float FlagSpawnCooldown = 10;

    // inline static const glm::vec2 SubmitPointSpawnPositions[] = {
    //     glm::vec2(75, 410),
    //     glm::vec2(150, 410),
    //     glm::vec2(225, 410),
    // };

    // inline static const glm::vec2 FlagSpawnPositions[] = {
    //     glm::vec2(104, 341),
    //     glm::vec2(50, 341),
    //     glm::vec2(145, 300),
    //     glm::vec2(174, 341),
    //     glm::vec2(137, 361),
    // };

    // player constants:
    inline static constexpr float PlayerRadius = 0.03f;
    inline static constexpr float PlayerSpeed = 4.0f;
    inline static constexpr float PlayerAccelHalflife = 0.25f;

    // used by server:
    // send game state.
    //   Will move "connection_player" to the front of the front of the sent list.
    void send_state_message(Connection *connection, Player *connection_player = nullptr) const;

    void send_notification_message(Player *connection_player, std::string notification) const;

    bool recv_data_message(Connection *connection, Player *connection_player);
};
