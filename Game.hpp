#pragma once

#include "Scene.hpp"
#include "GameObject.hpp"

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

struct Game
{
    // for server
    std::list<GameObject> static_obstacles;  // the collision box should not be sync, instead generated from the scene on both server and client (if the client needs it)
    std::list<NetworkObject *> game_objects; // the dynamic game object sync to from server to client

    template <typename O>
    O *spawn_object()
    {
        static_assert(std::is_base_of_v<NetworkObject, O>, "Can only spawn network game object");
        O *obj = new O();
        // NetworkObject *no = static_cast<NetworkObject *>(obj);
        game_objects.push_back(obj);
        obj->init();
        return obj;
    }
    /**
     * Don't call this to remove object, instead mark the object as deleted
     */
    void remove_object(uint32_t id);

    Game();
    ~Game();
    // state update function:
    void update(float elapsed);

    // constants:
    // the update rate on the server:
    inline static constexpr float Tick = 1.0f / 30.0f;

    // player constants:
    inline static constexpr float PlayerRadius = 0.06f;
    inline static constexpr float PlayerSpeed = 2.0f;
    inline static constexpr float PlayerAccelHalflife = 0.25f;

    // used by server:
    // send game state.
    //   Will move "connection_player" to the front of the front of the sent list.
    void send_state_message(Connection *connection, Player *connection_player = nullptr) const;
};
