#include "Game.hpp"

#include "Connection.hpp"
#include "PlayMode.hpp"
#include "Scene.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#include "BBox.hpp"

void Player::init()
{
    NetworkObject::init();
    type = ObjectType::Player;
}

inline glm::vec2 normalize(glm::vec2 v)
{
    float m = glm::length(v);
    return (m > 1e-6f) ? (v / m) : glm::vec2(0.0f);
}

int Player::can_collide(const NetworkObject *other) const
{
    if (other->type == ObjectType::Player && other->id != id)
        return 2;
    if (other->type == ObjectType::Flag)
        return 1;
    return 0;
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
    update_win_lose(elapsed, game);
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
    // clamp
    float s = glm::length(velocity);
    if (s > MAX_SPEED)
        velocity *= (MAX_SPEED / s);

    // move at the end
    glm::vec2 delta = velocity * elapsed;
    if (delta.x > 1e-6f)
        data.player_facing = true;
    if (delta.x < -1e-6f)
        data.player_facing = false;

    auto hits = move_with_collision(game, delta);

    if (hits.size() > 0)
    {
        // if hit flag
        auto flag = get_colliders<Flag>(hits);
        if (flag)
        {
            data.has_flag = true;
            flag->deleted = true;
        }
        // if hit obstacle
        auto obstacle = get_colliders(hits, ObjectType::Obstacle);
        if (obstacle)
        {
            take_damage(COLLISION_DAMAGAE, obstacle);
            velocity = glm::vec2(0, 0);
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

        torp->velocity = glm::vec2(data.player_facing ? 1 : -1, 0) * Torpedo::TORPEDO_SPEED;
        torp->owner = id;
        data.torpedo_timer = 0.0f;
    }
    else
    {
        data.torpedo_timer += elapsed;
    }
}

void Player::update_win_lose(float elapsed, Game *game)
{
    if (data.hp < 0)
    {
        die(game);
        return;
    }

    if (glm::distance(position, data.spawn_pos) < 5)
    {
        // win
        if (data.has_flag)
        {
            std::cout << "Player " << id << " collect a flag\n";
            data.flag_count++;
            data.has_flag = false;
        }
        return;
    }
}

void Player::take_damage(float damage, GameObject *source)
{
    std::cout << "Player " << id << " take " << damage << " damage from " << int(source->type) << "\n";
    data.hp -= damage;
}

void Player::die(Game *game)
{
    std::cout << "Player " << id << " die\n";
    if (data.has_flag)
    {
        // drop flag
        auto flag = game->spawn_object<Flag>();
        flag->position = position;
        data.has_flag = false;
    }
    data.hp = MAX_HEALTH;
    position = data.spawn_pos;
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

void Player::PlayerData::send(Connection *connection) const
{
    connection->send(torpedo_timer);
    connection->send(player_facing);
    connection->send(hp);
    connection->send(has_flag);
    connection->send(flag_count);
    connection->send(spawn_pos);
};

void Player::PlayerData::receive(uint32_t *at, std::vector<uint8_t> &recv_buffer)
{
    auto read = [&](auto *val)
    {
        std::memcpy(val, &recv_buffer[4 + *at], sizeof(*val));
        *at += sizeof(*val);
    };
    read(&torpedo_timer);
    read(&player_facing);
    read(&hp);
    read(&has_flag);
    read(&flag_count);
    read(&spawn_pos);
};