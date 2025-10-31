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
    for (NetworkObject *obj : game_objects)
    {
        obj->update(elapsed, this);
    }
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

    if (connection_player)
        connection_player->send(&connection);

    for (auto const &obj : game_objects)
    {
        if (obj == connection_player)
            continue;
        obj->send(&connection);
    }

    // compute the message size and patch into the message header:
    uint32_t size = uint32_t(connection.send_buffer.size() - mark);
    connection.send_buffer[mark - 3] = uint8_t(size);
    connection.send_buffer[mark - 2] = uint8_t(size >> 8);
    connection.send_buffer[mark - 1] = uint8_t(size >> 16);
}
