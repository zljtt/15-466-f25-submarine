#include "Game.hpp"

#include "Connection.hpp"
#include "PlayMode.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#include "AABB.hpp"

void NetworkObject::init()
{
    id = dist(mt);
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

void Player::init()
{
    NetworkObject::init();
    position = glm::vec3{148, 70, 0};
    color = glm::normalize(color);
    radar_color = 1;
    name = "Player " + std::to_string(next_player_number++);
    do
    {
        color.r = mt() / float(mt.max());
        color.g = mt() / float(mt.max());
        color.b = mt() / float(mt.max());
    } while (color == glm::vec3(0.0f));
}

void Player::send(Connection *connection) const
{
    NetworkObject::send(connection);
    connection->send(color);
    connection->send(radar_color);
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
    read(&radar_color);
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

void Torpedo::init()
{
    NetworkObject::init();
    scale = glm::vec2(0.5f, 0.5f);
    tracking = false;
    age = 0;
}

void Torpedo::send(Connection *connection) const
{
    NetworkObject::send(connection);
    connection->send(tracking);
    connection->send(age);
};

void Torpedo::receive(uint32_t *at, std::vector<uint8_t> &recv_buffer)
{
    auto read = [&](auto *val)
    {
        std::memcpy(val, &recv_buffer[4 + *at], sizeof(*val));
        *at += sizeof(*val);
    };
    NetworkObject::receive(at, recv_buffer);
    read(&tracking);
    read(&age);
};

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
