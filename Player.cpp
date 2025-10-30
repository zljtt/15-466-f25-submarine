#include "Game.hpp"

#include "Connection.hpp"
#include "PlayMode.hpp"
#include "Scene.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#include "AABB.hpp"

void Player::init()
{
    NetworkObject::init();
    type = ObjectType::Player;
    position = glm::vec3{148, 70, 0};
}

inline glm::vec2 normalize(glm::vec2 v)
{
    float m = glm::length(v);
    return (m > 1e-6f) ? (v / m) : glm::vec2(0.0f);
}

void Player::update(float elapsed, Game *game)
{
    glm::vec2 control_dir = glm::vec2(0.0f, 0.0f);
    if (controls.left.pressed)
        control_dir.x -= 1.0f;
    if (controls.right.pressed)
        control_dir.x += 1.0f;
    if (controls.down.pressed)
        control_dir.y -= 1.0f;
    if (controls.up.pressed)
        control_dir.y += 1.0f;

    update_weapon(elapsed, game);
    update_movement(elapsed, game, control_dir);

    // reset 'downs' since controls have been handled:
    controls.left.downs = 0;
    controls.right.downs = 0;
    controls.up.downs = 0;
    controls.down.downs = 0;
    controls.jump.downs = 0;
}

void Player::update_movement(float elapsed, Game *game, glm::vec2 control)
{

    glm::vec2 v_desired = normalize(control) * MAX_SPEED;
    glm::vec2 dv = v_desired - velocity;
    float rate = (glm::dot(dv, v_desired) > 0.0f) ? ACCEL_RATE : DECEL_RATE;
    // lerp velocity
    float dv_len = glm::length(dv);
    if (dv_len > 1e-6f)
    {
        velocity += dv * std::min(1.0f, rate * elapsed / dv_len);
    }
    else
    {
        velocity = v_desired;
    }
    // drag
    velocity *= (1.0f / (1.0f + DRAG_S * elapsed));
    float s = glm::length(velocity);
    if (s > MAX_SPEED)
        velocity *= (MAX_SPEED / s);
    // move at the end
    glm::vec2 delta = velocity * elapsed;
    // position += delta;

    // check collision
    // combine static obstacle and players, vector is better for traversing
    std::vector<GameObject> targets;
    targets.reserve(game->static_obstacles.size() + game->game_objects.size());
    targets.insert(targets.end(), game->static_obstacles.begin(), game->static_obstacles.end());
    for (auto &obj : game->game_objects)
    {
        if (obj->id == id)
            continue;
        targets.push_back(*obj);
    }

    position.x += delta.x;
    // if overlapping any obstacle, push out on X
    for (int pass = 0; pass < 2; ++pass)
    {
        for (const auto &o : targets)
        {
            position += resolve_axis(this, o, 0);
        }
    }

    position.y += delta.y;
    for (int pass = 0; pass < 2; ++pass)
    {
        for (const auto &o : targets)
        {
            position += resolve_axis(this, o, 1);
        }
    }
}
void Player::update_weapon(float elapsed, Game *game)
{
    // spawn a torpedo
    if (controls.jump.pressed && data.torpedo_timer > TORPEDO_COOLDOWN)
    {
        // std::cout << "from player: " << id << "wait time is " << data.torpedo_timer << std::endl;
        // std::cout << "Player Pos: " << p.position.x << " " << p.position.y << " \n";
        Torpedo *torp = game->spawn_object<Torpedo>();
        torp->position = position;
        torp->velocity = glm::normalize(velocity) * Torpedo::TORPEDO_SPEED;
        data.torpedo_timer = 0.0f;
    }
    else
    {
        data.torpedo_timer += elapsed;
    }
}

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
