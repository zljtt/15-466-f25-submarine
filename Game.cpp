#include "Game.hpp"

#include "Connection.hpp"
#include "PlayMode.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include "BBox.hpp"

//-----------------------------------------

Game::Game()
{
}

Game::~Game()
{
    for (auto obj : game_objects)
    {
        free(obj);
    }
}

void Game::init_player_spawn_info(Player *player)
{
    player->data.spawn_pos = SpawnPos[next_player_number];
    player->position = SpawnPos[next_player_number];
    next_player_number++;
}

void Game::remove_object(uint32_t id)
{
    bool found = false;
    for (auto pi = game_objects.begin(); pi != game_objects.end(); ++pi)
    {
        if ((*pi)->id == id)
        {
            game_objects.erase(pi);
            free(*pi);
            found = true;
            break;
        }
    }
    assert(found);
}

void Game::update(float elapsed)
{
    // spawn flag
    auto flags = get_objects<Flag>();
    auto players = get_objects<Player>();
    bool has_flag = false;
    for (auto p : players)
        if (p->data.has_flag)
            has_flag = true;
    if (flags.size() == 0 && !has_flag)
    {
        flag_spawn_timer -= elapsed;
        if (flag_spawn_timer < 0)
        {
            auto flag = spawn_object<Flag>();
            // PLAY SOUND : new flag spawned
            flag->add_sound_cue(static_cast<uint8_t>(SoundCues::JustSpawned));
            // UI NOTIFY : new flag spawned

            std::uniform_real_distribution<float> randx(std::min(FlagSpawnMin.x, FlagSpawnMax.x), std::max(FlagSpawnMin.x, FlagSpawnMax.x));
            std::uniform_real_distribution<float> randy(std::min(FlagSpawnMin.y, FlagSpawnMax.y), std::max(FlagSpawnMin.y, FlagSpawnMax.y));
            flag->position = glm::vec2(randx(mt), randy(mt));

            std::cout << "flag spawn at " << flag->position.x << " " << flag->position.y << "\n";
            flag_spawn_timer = FlagSpawnCooldown;
        }
    }

    for (NetworkObject *obj : game_objects)
    {
        obj->update(elapsed, this);
    }
    level.update(elapsed);
}

void Game::send_state_message(Connection *connection_, Player *connection_player) const
{
    assert(connection_);
    auto &connection = *connection_;

    connection.send(Message::S2C_State);
    // will patch message size in later, for now placeholder bytes:
    connection.send(uint8_t(0));
    connection.send(uint8_t(0));
    connection.send(uint8_t(0));
    size_t mark = connection.send_buffer.size(); // keep track of this position in the buffer

    // send game objects
    connection.send(uint8_t(game_objects.size()));

    // send local players
    if (connection_player)
        connection_player->send(&connection);

    // send game objects
    for (auto const &obj : game_objects)
    {
        if (obj == connection_player)
            continue;

        obj->send(&connection);
    }
    // send player data
    auto players = get_objects<Player>();
    connection.send(uint8_t(players.size()));
    for (auto p : players)
    {
        connection.send(uint32_t(p->id));
        p->data.send(&connection);
    }

    // send level data
    level.send(&connection);

    // compute the message size and patch into the message header:
    uint32_t size = uint32_t(connection.send_buffer.size() - mark);
    connection.send_buffer[mark - 3] = uint8_t(size);
    connection.send_buffer[mark - 2] = uint8_t(size >> 8);
    connection.send_buffer[mark - 1] = uint8_t(size >> 16);
}
