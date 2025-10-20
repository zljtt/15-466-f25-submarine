#include "Game.hpp"

#include "Connection.hpp"
#include "PlayMode.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

void Player::Controls::send_controls_message(Connection *connection_) const
{
    assert(connection_);
    auto &connection = *connection_;

    uint32_t size = 5;
    connection.send(Message::C2S_Controls);
    connection.send(uint8_t(size));
    connection.send(uint8_t(size >> 8));
    connection.send(uint8_t(size >> 16));

    auto send_button = [&](Button const &b)
    {
        if (b.downs & 0x80)
        {
            std::cerr << "Wow, you are really good at pressing buttons!" << std::endl;
        }
        connection.send(uint8_t((b.pressed ? 0x80 : 0x00) | (b.downs & 0x7f)));
    };

    send_button(left);
    send_button(right);
    send_button(up);
    send_button(down);
    send_button(jump);
}

bool Player::Controls::recv_controls_message(Connection *connection_)
{
    assert(connection_);
    auto &connection = *connection_;

    auto &recv_buffer = connection.recv_buffer;

    // expecting [type, size_low0, size_mid8, size_high8]:
    if (recv_buffer.size() < 4)
        return false;
    if (recv_buffer[0] != uint8_t(Message::C2S_Controls))
        return false;
    uint32_t size = (uint32_t(recv_buffer[3]) << 16) | (uint32_t(recv_buffer[2]) << 8) | uint32_t(recv_buffer[1]);
    if (size != 5)
        throw std::runtime_error("Controls message with size " + std::to_string(size) + " != 5!");

    // expecting complete message:
    if (recv_buffer.size() < 4 + size)
        return false;

    auto recv_button = [](uint8_t byte, Button *button)
    {
        button->pressed = (byte & 0x80);
        uint32_t d = uint32_t(button->downs) + uint32_t(byte & 0x7f);
        if (d > 255)
        {
            std::cerr << "got a whole lot of downs" << std::endl;
            d = 255;
        }
        button->downs = uint8_t(d);
    };

    recv_button(recv_buffer[4 + 0], &left);
    recv_button(recv_buffer[4 + 1], &right);
    recv_button(recv_buffer[4 + 2], &up);
    recv_button(recv_buffer[4 + 3], &down);
    recv_button(recv_buffer[4 + 4], &jump);

    // delete message from buffer:
    recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

    return true;
}

//-----------------------------------------

Game::Game() : mt(0x15466666), dist(1u, 0xFFFFFFFFu)
{
}

Player *Game::spawn_player()
{
    players.emplace_back();
    Player &player = players.back();
    player.id = dist(mt);
    player.color = glm::normalize(player.color);
    player.name = "Player " + std::to_string(next_player_number++);

    do
    {
        player.color.r = mt() / float(mt.max());
        player.color.g = mt() / float(mt.max());
        player.color.b = mt() / float(mt.max());
    } while (player.color == glm::vec3(0.0f));

    return &player;
}

NetworkObject *Game::spawn_object()
{
    game_objects.emplace_back();
    NetworkObject &obj = game_objects.back();
    obj.position.x = 0;
    obj.position.y = 0;
    obj.id = dist(mt);
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

        p.position += dir * elapsed * 20.0f;

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

bool Game::recv_state_message(Connection *connection_, std::unordered_map<uint32_t, Scene::Drawable *> &drawables, Scene &scene)
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
            local_player = player.id;
        }
        // create drawable if not find
        auto obj = drawables.find(player.id);
        auto pos = glm::vec3(player.position.x, player.position.y, 0);
        if (obj == drawables.end())
        {
            drawables[player.id] = prefab_player->create_drawable(scene, pos);
        }
        else
        {
            obj->second->transform->position = pos;
        }
    }

    game_objects.clear();
    uint8_t object_count;
    read(&object_count);
    for (uint8_t i = 0; i < object_count; ++i)
    {
        game_objects.emplace_back();
        NetworkObject &obj = game_objects.back();
        obj.receive(&at, recv_buffer);
        // create drawable if not find
        if (drawables.find(obj.id) == drawables.end())
        {
            // mode_->create_game_object(prefab_player, obj.id, glm::vec3(obj.position.x, obj.position.y, 0));
        }
    }

    if (at != size)
        throw std::runtime_error("Trailing data in state message.");

    // delete message from buffer:
    recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

    return true;
}

void NetworkObject::send(Connection *connection) const
{
    connection->send(id);
    connection->send(position);
    connection->send(velocity);
};

void NetworkObject::receive(uint32_t *at, std::vector<uint8_t> &recv_buffer)
{
    auto read = [&](auto *val)
    {
        std::memcpy(val, &recv_buffer[4 + *at], sizeof(*val));
        *at += sizeof(*val);
    };
    read(&id);
    read(&position);
    read(&velocity);
};

void Player::send(Connection *connection) const
{
    NetworkObject::send(connection);
    connection->send(color);

    // NOTE: can't just 'send(name)' because player.name is not plain-old-data type.
    // effectively: truncates player name to 255 chars
    uint8_t len = uint8_t(std::min<size_t>(255, name.size()));
    connection->send(len);
    connection->send_buffer.insert(connection->send_buffer.end(), name.begin(), name.begin() + len);
};

void Player::receive(uint32_t *at, std::vector<uint8_t> &recv_buffer)
{
    auto read = [&](auto *val)
    {
        std::memcpy(val, &recv_buffer[4 + *at], sizeof(*val));
        *at += sizeof(*val);
    };
    NetworkObject::receive(at, recv_buffer);
    read(&color);
    uint8_t name_len;
    read(&name_len);
    name = "";
    for (uint8_t n = 0; n < name_len; ++n)
    {
        char c;
        read(&c);
        name += c;
    }
};