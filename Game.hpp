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
    // common
    std::list<Player> players;             // (using list so they can have stable addresses)
    std::list<Torpedo> torpedoes; 
    uint8_t torpedoes_count;               // used for tracking destruction of torpedoes
    std::list<NetworkObject> game_objects; // the dynamic game object other than player, they also have collision box, so it need to be checked during collision
    // for local
    Player *local_player;
    // for server
    std::list<GameObject> static_obstacles; // the collision box should not be sync, instead generated from the scene on both server and client (if the client needs it)

    Player *spawn_player(); // add player the end of the players list (may also, e.g., play some spawn anim)
    Torpedo *spawn_torpedo();
    NetworkObject *spawn_object();
    void remove_player(Player *); // remove player from game (may also, e.g., play some despawn anim)
    void remove_torpedo(Torpedo *);
    void remove_object(NetworkObject *);

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
    bool recv_state_message(Connection *connection, std::function<void(Player &)> on_player, std::function<void(NetworkObject &)> on_game_object, 
    std::function<void(Torpedo &)> on_torpedo,
    std::function<void()> on_torpedo_destroy);

    // used by server:
    // send game state.
    //   Will move "connection_player" to the front of the front of the sent list.
    void send_state_message(Connection *connection, Player *connection_player = nullptr) const;
};
