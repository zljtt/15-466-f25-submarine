#include "Game.hpp"

#include "Connection.hpp"
#include "PlayMode.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include "BBox.hpp"
#include "data_path.hpp"

Game::Game()
{
    level.black_box_pos.clear();
    level.submit_spawn_pos.clear();
    auto on_drawable = [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name)
    {
        if (transform->name.rfind("BlackBox", 0) == 0)
        {
            level.black_box_pos.push_back(glm::vec2(transform->position));
        }
        else if (transform->name.rfind("SubmitPoint", 0) == 0)
        {
            level.submit_spawn_pos.push_back(glm::vec2(transform->position));
        }
    };

    Scene(data_path("resource/scene/prototype_marker.scene"), on_drawable);
    std::cout << "flag count " << std::to_string(level.black_box_pos.size()) << "\n";
    std::cout << "submit count " << std::to_string(level.submit_spawn_pos.size()) + "\n";
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
    player->scale.x = 2;
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

    switch (level.status)
    {
    case Level::Prepare:
    {
        auto players = get_objects<Player>();
        if (players.size() >= level.max_player)
        {
            int ready = 0;
            for (auto p : players)
            {
                if (p->data.status == Player::Ready)
                {
                    ready++;
                }
            }
            if (ready >= players.size())
            {
                level.status = Level::Play;
                for (auto p : players)
                {
                    p->data.status = Player::Play;
                }
            }
        }
        break;
    }
    case Level::Play:
    {
        // spawn flag
        auto flags = get_objects<Flag>();
        auto players_ = get_objects<Player>();
        bool has_flag = false;
        for (auto p : players_)
            if (p->data.has_flag)
                has_flag = true;
        if (flags.size() == 0 && !has_flag)
        {
            flag_spawn_timer -= elapsed;
            if (flag_spawn_timer < 0)
            {
                // remove all submit point
                auto sps = get_objects<SubmitPoint>();
                if (sps.size() > 0)
                {
                    remove_object(sps[0]->id);
                }
                // spawn flag
                auto flag = spawn_object<Flag>();
                auto submit = spawn_object<SubmitPoint>();
                // PLAY SOUND : new flag spawned
                flag->add_sound_cue(static_cast<uint8_t>(SoundCues::JustSpawned));
                // UI NOTIFY : new flag spawned
                auto players__ = get_objects<Player>();
                for (auto p : players__)
                {
                    send_notification_message(p, "A new BlackBox location is revealed.");
                }
                assert(level.black_box_pos.size() > 0);
                assert(level.black_box_pos.size() > 0);
                std::uniform_int_distribution<int> randflag(0, int(level.black_box_pos.size()) - 1);
                std::uniform_int_distribution<int> randsubmit(0, int(level.submit_spawn_pos.size()) - 1);
                // std::uniform_real_distribution<float> randx(std::min(FlagSpawnMin.x, FlagSpawnMax.x), std::max(FlagSpawnMin.x, FlagSpawnMax.x));
                // std::uniform_real_distribution<float> randy(std::min(FlagSpawnMin.y, FlagSpawnMax.y), std::max(FlagSpawnMin.y, FlagSpawnMax.y));
                flag->position = level.black_box_pos[randflag(mt)];
                submit->position = level.submit_spawn_pos[randsubmit(mt)];

                std::cout << "flag spawn at " << flag->position.x << " " << flag->position.y << "\n";
                flag_spawn_timer = FlagSpawnCooldown;
            }
        }
        auto players = get_objects<Player>();
        for (auto player : players)
        {
            if (player->data.flag_count >= Game::MaxBlackBox)
            {
                level.status = Level::End;
                break;
            }
        }
        break;
    }
    case Level::End:
    {
        auto players = get_objects<Player>();
        for (auto player : players)
        {
            player->data.status = Player::Status::GameOver;
        }
        break;
    }
    default:
        break;
    }

    for (NetworkObject *obj : game_objects)
    {
        obj->update(elapsed, this);
    }
    level.update(elapsed);
}

void Game::send_notification_message(Player *connection_player, std::string notification) const
{
    Connection *connection = nullptr;
    for (auto &[conn, player] : connection_to_player)
        if (player == connection_player)
            connection = conn;
    assert(connection);
    connection->send(Message::S2C_Notification);
    connection->send(uint8_t(0));
    connection->send(uint8_t(0));
    connection->send(uint8_t(0));
    size_t mark = connection->send_buffer.size(); // keep track of this position in the buffer
    uint32_t len = (uint32_t)notification.size();

    connection->send(len);
    connection->send_buffer.insert(
        connection->send_buffer.end(),
        notification.begin(),
        notification.end());
    // compute the message size and patch into the message header:
    uint32_t size = uint32_t(connection->send_buffer.size() - mark);
    connection->send_buffer[mark - 3] = uint8_t(size);
    connection->send_buffer[mark - 2] = uint8_t(size >> 8);
    connection->send_buffer[mark - 1] = uint8_t(size >> 16);
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

bool Game::recv_data_message(Connection *connection_, Player *connection_player)
{
    assert(connection_);
    auto &connection = *connection_;
    auto &recv_buffer = connection.recv_buffer;
    if (recv_buffer.size() < 4)
        return false;
    if (recv_buffer[0] != uint8_t(Message::C2S_Data))
        return false;
    uint32_t size = (uint32_t(recv_buffer[3]) << 16) | (uint32_t(recv_buffer[2]) << 8) | uint32_t(recv_buffer[1]);
    uint32_t at = 0;

    // expecting complete message:
    if (recv_buffer.size() < 4 + size)
        return false;

    auto read_string = [&]()
    {
        uint32_t len;
        std::memcpy(&len, &recv_buffer[4 + at], sizeof(uint32_t));
        at += sizeof(uint32_t);

        if (4 + at + len > recv_buffer.size())
        {
            throw std::runtime_error("Ran out of bytes reading data.");
        }

        std::string s((char *)&recv_buffer[4 + at], len);
        at += len;
        return s;
    };

    connection_player->data.pname = read_string();
    std::cout << "name: " << connection_player->data.pname << "\n";
    // delete message from buffer:
    recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

    return true;
}
