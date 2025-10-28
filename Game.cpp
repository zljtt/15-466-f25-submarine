#include "Game.hpp"

#include "Connection.hpp"
#include "PlayMode.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include "AABB.hpp"

//-----------------------------------------

Game::Game()
{
}

Player *Game::spawn_player()
{
    players.emplace_back();
    Player &player = players.back();
    player.init();
    return &player;
}

Torpedo *Game::spawn_torpedo()
{
    torpedoes.emplace_back();
    Torpedo &torpedo = torpedoes.back();
    torpedo.init();
    return &torpedo;
}

NetworkObject *Game::spawn_object()
{
    game_objects.emplace_back();
    NetworkObject &obj = game_objects.back();
    obj.init();
    return &obj;
}

void Game::remove_player(Player *player)
{
    bool found = false;
    for (auto pi = players.begin(); pi != players.end(); ++pi)
    {
        if (&*pi == player)
        {
            players.erase(pi);
            found = true;
            break;
        }
    }
    assert(found);
}

void Game::remove_torpedo(Torpedo *torpedo)
{
    bool found = false;
    for (auto ti = torpedoes.begin(); ti != torpedoes.end(); ++ti)
    {
        if (&*ti == torpedo)
        {
            torpedoes.erase(ti);
            found = true;
            break;
        }
    }
    assert(found);
}

void Game::remove_object(NetworkObject *object)
{
    bool found = false;
    for (auto pi = game_objects.begin(); pi != game_objects.end(); ++pi)
    {
        if (&*pi == object)
        {
            game_objects.erase(pi);
            found = true;
            break;
        }
    }
    assert(found);
}

void Game::update(float elapsed)
{
    // position/velocity update:
    for (auto &p : players)
    {
        glm::vec2 dir = glm::vec2(0.0f, 0.0f);
        if (p.controls.left.pressed)
            dir.x -= 1.0f;
        if (p.controls.right.pressed)
            dir.x += 1.0f;
        if (p.controls.down.pressed)
            dir.y -= 1.0f;
        if (p.controls.up.pressed)
            dir.y += 1.0f;

        // spawn a torpedo
        if (p.controls.jump.pressed && p.torpWait > p.torpCD)
        {
            // std::cout << "from player: " << p.id << "wait time is" << p.torpWait << std::endl;
            std::cout << "Player Pos: " << p.position.x << " " << p.position.y << " \n";
            // Torpedo *torp = spawn_torpedo();
            // torp->position = p.position + glm::normalize(dir) * 5.0f;
            // torp->velocity = glm::normalize(dir) * 10.0f;
            // p.torpWait = 0.0f;
        }
        else
        {
            p.torpWait += elapsed;
        }

        // loop through the torpedoes to determine which ones has reached its time
        for (Torpedo &torp : torpedoes)
        {
            torp.age += elapsed;
            if (torp.age > torp.lifetime)
            {
                remove_torpedo(&torp);
            }
        }

        // combine static obstacle and players, vector is better for traversing
        std::vector<GameObject> targets;
        targets.reserve(static_obstacles.size() + players.size() + game_objects.size());
        targets.insert(targets.end(), static_obstacles.begin(), static_obstacles.end());
        for (auto &obj : players)
        {
            if (&obj != &p)
                targets.push_back(obj);
        }
        for (auto &obj : game_objects)
        {
            targets.push_back(obj);
        }

        // calculate collision
        if (dir != glm::vec2(0.0f))
            dir = glm::normalize(dir);
        glm::vec2 delta = dir * 10.0f * elapsed;
        p.position.x += delta.x;
        // if overlapping any obstacle, push out on X
        for (int pass = 0; pass < 2; ++pass)
        {
            for (const auto &o : targets)
            {
                p.position += resolve_axis(p, o, 0);
            }
        }

        p.position.y += delta.y;
        for (int pass = 0; pass < 2; ++pass)
        {
            for (const auto &o : targets)
            {
                p.position += resolve_axis(p, o, 1);
            }
        }

        // reset 'downs' since controls have been handled:
        p.controls.left.downs = 0;
        p.controls.right.downs = 0;
        p.controls.up.downs = 0;
        p.controls.down.downs = 0;
        p.controls.jump.downs = 0;
    }

    // collision resolution:
    // for (auto &p1 : players)
    // {
    //     // player/player collisions:
    //     for (auto &p2 : players)
    //     {
    //         if (&p1 == &p2)
    //             break;
    //         glm::vec2 p12 = p2.position - p1.position;
    //         float len2 = glm::length2(p12);
    //         if (len2 > (2.0f * PlayerRadius) * (2.0f * PlayerRadius))
    //             continue;
    //         if (len2 == 0.0f)
    //             continue;
    //         glm::vec2 dir = p12 / std::sqrt(len2);
    //         // mirror velocity to be in separating direction:
    //         glm::vec2 v12 = p2.velocity - p1.velocity;
    //         glm::vec2 delta_v12 = dir * glm::max(0.0f, -1.75f * glm::dot(dir, v12));
    //         p2.velocity += 0.5f * delta_v12;
    //         p1.velocity -= 0.5f * delta_v12;
    //     }
    //     // player/arena collisions:
    //     if (p1.position.x < ArenaMin.x + PlayerRadius)
    //     {
    //         p1.position.x = ArenaMin.x + PlayerRadius;
    //         p1.velocity.x = std::abs(p1.velocity.x);
    //     }
    //     if (p1.position.x > ArenaMax.x - PlayerRadius)
    //     {
    //         p1.position.x = ArenaMax.x - PlayerRadius;
    //         p1.velocity.x = -std::abs(p1.velocity.x);
    //     }
    //     if (p1.position.y < ArenaMin.y + PlayerRadius)
    //     {
    //         p1.position.y = ArenaMin.y + PlayerRadius;
    //         p1.velocity.y = std::abs(p1.velocity.y);
    //     }
    //     if (p1.position.y > ArenaMax.y - PlayerRadius)
    //     {
    //         p1.position.y = ArenaMax.y - PlayerRadius;
    //         p1.velocity.y = -std::abs(p1.velocity.y);
    //     }
    // }
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

    // send player info helper:

    // player count:
    connection.send(uint8_t(players.size()));
    if (connection_player)
        connection_player->send(&connection);
    for (auto const &player : players)
    {
        if (&player == connection_player)
            continue;
        player.send(&connection);
    }

    // send torpedoes
    connection.send(uint8_t(torpedoes.size()));
    for (auto const &torpedo : torpedoes)
    {
        torpedo.send(&connection);
    }

    // send game objects
    connection.send(uint8_t(game_objects.size()));
    for (auto const &obj : game_objects)
    {
        obj.send(&connection);
    }

    // compute the message size and patch into the message header:
    uint32_t size = uint32_t(connection.send_buffer.size() - mark);
    connection.send_buffer[mark - 3] = uint8_t(size);
    connection.send_buffer[mark - 2] = uint8_t(size >> 8);
    connection.send_buffer[mark - 1] = uint8_t(size >> 16);
}

bool Game::recv_state_message(Connection *connection_,
                              std::function<void(Player &)> on_player,
                              std::function<void(NetworkObject &)> on_game_object,
                              std::function<void(Torpedo &)> on_torpedo,
                              std::function<void()> on_torpedo_destroy)
{
    assert(connection_);
    auto &connection = *connection_;
    auto &recv_buffer = connection.recv_buffer;

    if (recv_buffer.size() < 4)
        return false;
    if (recv_buffer[0] != uint8_t(Message::S2C_State))
        return false;
    uint32_t size = (uint32_t(recv_buffer[3]) << 16) | (uint32_t(recv_buffer[2]) << 8) | uint32_t(recv_buffer[1]);
    uint32_t at = 0;
    // expecting complete message:
    if (recv_buffer.size() < 4 + size)
        return false;

    // copy bytes from buffer and advance position:
    auto read = [&](auto *val)
    {
        if (at + sizeof(*val) > size)
        {
            throw std::runtime_error("Ran out of bytes reading state message.");
        }
        std::memcpy(val, &recv_buffer[4 + at], sizeof(*val));
        at += sizeof(*val);
    };

    players.clear();
    uint8_t player_count;
    read(&player_count);
    for (uint8_t i = 0; i < player_count; ++i)
    {
        players.emplace_back();
        Player &player = players.back();
        player.receive(&at, recv_buffer);
        // find local player
        if (i == 0)
        {
            local_player = &player;
        }
        on_player(player);
    }

    torpedoes.clear();
    uint8_t tempTorp_count = torpedoes_count;
    read(&torpedoes_count);
    for (uint8_t i = 0; i < torpedoes_count; ++i)
    {
        torpedoes.emplace_back();
        Torpedo &torpedo = torpedoes.back();
        torpedo.receive(&at, recv_buffer);
        on_torpedo(torpedo);
    }
    if (tempTorp_count > torpedoes_count)
    {
        std::cout << "torpedoes destroyed" << std::endl;
        on_torpedo_destroy();
    }

    game_objects.clear();
    uint8_t object_count;
    read(&object_count);
    for (uint8_t i = 0; i < object_count; ++i)
    {

        game_objects.emplace_back();
        NetworkObject &obj = game_objects.back();
        obj.receive(&at, recv_buffer);
        on_game_object(obj);
    }

    if (at != size)
    {
        std::cout << "at:" << at << " size:" << size << " torpedo count " << torpedoes_count << " huh" << std::endl;
        throw std::runtime_error("Trailing data in state message.");
    }

    // delete message from buffer:
    recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

    return true;
}
