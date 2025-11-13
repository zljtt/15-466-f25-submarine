#pragma once
#include "GameObject.hpp"
#include "Connection.hpp"

struct Level
{
    struct RevealedObject
    {
        uint32_t obj_id;
        float age;
        float duration;
    };
    std::vector<RevealedObject> revealed_objects;

    Level() {
    };

    void update(float elapsed)
    {
        for (auto &ro : revealed_objects)
        {
            ro.age += elapsed;
        }

        revealed_objects.erase(
            std::remove_if(revealed_objects.begin(), revealed_objects.end(),
                           [](const RevealedObject &r)
                           { return r.age > r.duration; }),
            revealed_objects.end());
    }

    void send(Connection *connection) const
    {
        connection->send(uint8_t(revealed_objects.size()));
        for (auto ro : revealed_objects)
        {
            connection->send(ro.obj_id);
        }
    }

    void receive(uint32_t *at, std::vector<uint8_t> &recv_buffer)
    {
        auto read = [&](auto *val)
        {
            std::memcpy(val, &recv_buffer[4 + *at], sizeof(*val));
            *at += sizeof(*val);
        };

        revealed_objects.clear();
        uint8_t ro_count;
        read(&ro_count);
        for (int i = 0; i < ro_count; i++)
        {
            RevealedObject obj;
            read(&obj.obj_id);
            obj.age = 0;
            obj.duration = 0;
            revealed_objects.push_back(obj);
        }
    }
};
