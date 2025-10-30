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
    connection->send(type);
    connection->send(position);
    connection->send(scale);
    connection->send(deleted);
};

void NetworkObject::receive(uint32_t *at, std::vector<uint8_t> &recv_buffer)
{
    auto read = [&](auto *val)
    {
        std::memcpy(val, &recv_buffer[4 + *at], sizeof(*val));
        *at += sizeof(*val);
    };
    read(&id);
    read(&type);
    read(&position);
    read(&scale);
    read(&deleted);
};
