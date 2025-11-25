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
    data.i_frame -= elapsed;
    switch (data.status)
    {
    case NotReady:
    {
        if (controls.num1.downs)
        {
            select_ship(Default);
            select_ship(Explorer);
            data.status = Ready;
        }
        else if (controls.num2.downs)
        {
            select_ship(Default);
            select_ship(Fighter);
            data.status = Ready;
        }
        else if (controls.num3.downs)
        {
            select_ship(Default);
            select_ship(Quest);
            data.status = Ready;
        }
        break;
    }
    case Ready:
        break;
    case Play:
    {
        if (controls.radar.downs)
        {
            if (data.super_radar_exposure)
            {
                game->level.revealed_objects.emplace_back(id, 0.0f, static_cast<float>(SUPER_RADAR_EXPOSURE_TIME));
            }
        }
        if (controls.light.downs)
        {
            data.light_on = !data.light_on;
        }
        update_movement(elapsed, game);
        update_weapon(elapsed, game);
        update_win_lose(elapsed, game);

        break;
    }
    case RespawnSelect:
    {
        data.respawn_timer -= elapsed;
        if (data.respawn_timer <= 0)
        {
            data.hp = 100;
            data.status = Play;
            break;
        }
        if (controls.num1.downs)
        {
            select_ship(Default);
            select_ship(Explorer);
            data.status = RespawnWait;
        }
        else if (controls.num2.downs)
        {
            select_ship(Default);
            select_ship(Fighter);
            data.status = RespawnWait;
        }
        else if (controls.num3.downs)
        {
            select_ship(Default);
            select_ship(Quest);
            data.status = RespawnWait;
        }

        break;
    }
    case RespawnWait:
    {
        data.respawn_timer -= elapsed;
        if (data.respawn_timer <= 0)
        {
            data.status = Play;
        }
        break;
    }
    case GameOver:
    {
        break;
    }
    default:
        break;
    }
    // reset 'downs' since controls have been handled:
    update_control(elapsed, game);
}

void Player::update_movement(float elapsed, Game *game)
{
    glm::vec2 control = glm::vec2(0.0f, 0.0f);
    if (controls.left.pressed)
        control.x -= 1.0f;
    if (controls.right.pressed)
        control.x += 1.0f;
    if (controls.down.pressed)
        control.y -= 1.0f;
    if (controls.up.pressed)
        control.y += 1.0f;

    glm::vec2 v_desired = normalize(control) * MAX_SPEED;
    glm::vec2 dv = v_desired - velocity;
    float rate = 0;
    if (glm::dot(dv, v_desired) > 0.0f)
    {
        if (glm::length(control) >= 0.01f && !data.engineStarted) // and not playing engine start
        {
            data.engineStarted = true;
            // PLAY SOUND : engine start
            // PLAY SOUND : engine noise loop
            add_sound_cue(static_cast<uint8_t>(SoundCues::Start));
        }

        rate = ACCEL_RATE;
    }
    else
    {
        rate = DECEL_RATE;
    }
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

    if (glm::length(control) <= 0.01f && data.engineStarted) // and not playing engine stop
    {
        data.engineStarted = false;
        // PLAY SOUND : engine stop
        // PLAY SOUND : stop playing engine noise loop
        add_sound_cue(static_cast<uint8_t>(SoundCues::Stop));
    }

    auto hits = move_with_collision(game, delta);

    if (hits.size() > 0)
    {
        // if hit flag
        auto flag = get_colliders<Flag>(hits);
        if (flag && !data.has_flag)
        {
            data.has_flag = true;
            flag->deleted = true;
            // UI NOTIFY : flag captured by player
        }
        // if hit obstacle
        auto obstacle = get_colliders(hits, ObjectType::Obstacle);
        if (obstacle)
        {
            take_damage(game, data.collision_damage, obstacle);
            velocity = glm::vec2(0, 0);
        }
    }
}

void Player::update_weapon(float elapsed, Game *game)
{
    // spawn a torpedo
    if (controls.jump.pressed && data.torpedo_timer > data.torpedo_cooldown)
    {
        // std::cout << "from player: " << id << "wait time is " << data.torpedo_timer << std::endl;
        // std::cout << "Player Pos: " << p.position.x << " " << p.position.y << " \n";
        Torpedo *torp = game->spawn_object<Torpedo>();
        torp->position = position;

        torp->velocity = glm::vec2(data.player_facing ? 1 : -1, 0) * data.torpedo_speed;
        torp->owner = id;
        data.torpedo_timer = 0.0f;
    }
    else
    {

        data.torpedo_timer += elapsed;
    }
}

void Player::update_control(float elapsed, Game *game)
{

    controls.left.downs = 0;
    controls.right.downs = 0;
    controls.up.downs = 0;
    controls.down.downs = 0;
    controls.jump.downs = 0;
    controls.radar.downs = 0;
    controls.light.downs = 0;
    controls.num1.downs = 0;
    controls.num2.downs = 0;
    controls.num3.downs = 0;
    controls.num4.downs = 0;
    controls.rotate_left.downs = 0;
    controls.rotate_right.downs = 0;
}

void Player::update_win_lose(float elapsed, Game *game)
{
    auto prev = data.submitting;
    auto sps = game->get_objects<SubmitPoint>();
    data.submitting = false;
    if (sps.size() > 0)
    {
        data.submitting = glm::distance(sps[0]->position, position) < 8 && data.has_flag;
    }
    if (!data.submitting)
        return;
    // UI NOTIFY : get point
    if (!prev)
    {
        auto players = game->get_objects<Player>();
        for (auto p : players)
        {
            if (p->id == this->id)
            {
                game->send_notification_message(p, "Submitting BlackBox...");
            }
            else
            {
                game->send_notification_message(p, "A Submarine is submitting BlackBox!");
            }
        }
        data.submit_progression = 0.0f;
        data.submitting = true;
    }
    data.submit_progression += elapsed;
    if (data.submit_progression >= 10.0f)
    {
        // PLAY SOUND : get point
        add_sound_cue(static_cast<uint8_t>(SoundCues::GetPoint));
        data.flag_count++;
        data.has_flag = false;
        auto players = game->get_objects<Player>();
        for (auto p : players)
        {
            if (p->id == this->id)
            {
                game->send_notification_message(p, "BlackBox submitted!");
            }
            else
            {
                game->send_notification_message(p, "A Submarine has submitted a BlackBox");
            }
        }
    }
}

void Player::take_damage(Game *game, float damage, GameObject *source)
{
    if (data.status != Play || data.i_frame >= 0.0f)
        return;
    // PLAY SOUND : take damage
    data.i_frame = 0.5f;
    add_sound_cue(static_cast<uint8_t>(SoundCues::Hit));
    std::cout << "Player " << id << " take " << damage << " damage from " << int(source->type) << "\n";
    data.hp -= damage;
    // game->send_notification_message(this, "Submarine Damaged.");

    Torpedo *other = dynamic_cast<Torpedo *>(source);
    if (other)
    {
        game->send_notification_message(this, "You are damaged by a Torpedo!");
    }

    if (data.hp < 0)
    {
        die(game, source);
        return;
    }
}

void Player::die(Game *game, GameObject *source)
{
    std::cout << "Player " << id << " die\n";
    // UI NOTIDY to killer : player killed
    auto players = game->get_objects<Player>();
    for (auto &player : players)
    {
        if (player->id == this->id)
        {
            game->send_notification_message(player, "Your submarine is destroyed.");
        }
        else
        {
            game->send_notification_message(player, "Another submarine is destroyed.");
        }
    }
    if (data.has_flag)
    {
        // UI NOTIFY to others : flag droped
        // drop flag
        auto flag = game->spawn_object<Flag>();
        flag->position = position;
        data.has_flag = false;
        for (auto &player : players)
        {
            game->send_notification_message(player, "BlackBox dropped!");
        }
    }
    data.respawn_timer = 10.0f;
    data.status = RespawnSelect;
    position = data.spawn_pos;
}

void Player::select_ship(ShipType obj_type)
{
    switch (obj_type)
    {
    case Default:
        data.hp = MAX_HEALTH;
        data.collision_damage = 10.0f;

        data.torpedo_cooldown = TORPEDO_COOLDOWN;
        data.torpedo_speed = TORPEDO_SPEED;

        data.normal_radar_interval = 1.0f;
        data.normal_radar_malfunction_change = 0.0f;
        data.normal_radar_range = 30.0f;
        data.normal_radar_info_duration = 2.8f;
        data.super_radar_cooldown = 5.0f;
        data.super_radar_info_duration = 5.0f;
        data.super_radar_exposure = true;
        break;
    case Explorer:
        data.collision_damage = 5.0f;
        data.normal_radar_interval = 0.6f;
        data.normal_radar_malfunction_change = 0.4f;
        data.normal_radar_info_duration = 1.7f;
        break;
    case Fighter:
        data.hp = 120.0f;
        data.torpedo_speed = 20.0f;
        data.normal_radar_interval = 1.0f;
        data.normal_radar_malfunction_change = 0.2f;
        break;
    case Quest:
        data.super_radar_info_duration = 10.0f;
        data.super_radar_exposure = false;
        data.normal_radar_range = 20.0f;
        data.normal_radar_interval = 1.1f;
        break;
    default:
        break;
    }
}
void Player::Controls::send_controls_message(Connection *connection_) const
{
    assert(connection_);
    auto &connection = *connection_;

    uint32_t size = 13;
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
    send_button(radar);
    send_button(light);
    send_button(num1);
    send_button(num2);
    send_button(num3);
    send_button(num4);
    send_button(rotate_left);
    send_button(rotate_right);
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
    if (size != 13)
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
    recv_button(recv_buffer[4 + 5], &radar);
    recv_button(recv_buffer[4 + 6], &light);
    recv_button(recv_buffer[4 + 7], &num1);
    recv_button(recv_buffer[4 + 8], &num2);
    recv_button(recv_buffer[4 + 9], &num3);
    recv_button(recv_buffer[4 + 10], &num4);
    recv_button(recv_buffer[4 + 11], &rotate_left);
    recv_button(recv_buffer[4 + 12], &rotate_right);

    // delete message from buffer:
    recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

    return true;
}

void Player::PlayerData::send(Connection *connection) const
{
    // general
    connection->send(status);
    connection->send(hp);
    connection->send(has_flag);
    connection->send(flag_count);
    connection->send(spawn_pos);
    connection->send(collision_damage);
    connection->send(respawn_timer);
    connection->send(submit_progression);
    connection->send(submitting);

    // weapon
    connection->send(torpedo_timer);
    connection->send(torpedo_cooldown);
    connection->send(torpedo_speed);

    // radar
    connection->send(normal_radar_interval);
    connection->send(normal_radar_malfunction_change);
    connection->send(normal_radar_range);
    connection->send(normal_radar_info_duration);
    connection->send(super_radar_cooldown);
    connection->send(super_radar_info_duration);

    // light
    connection->send(light_on);
    connection->send(player_facing);
};

void Player::PlayerData::receive(uint32_t *at, std::vector<uint8_t> &recv_buffer)
{
    auto read = [&](auto *val)
    {
        std::memcpy(val, &recv_buffer[4 + *at], sizeof(*val));
        *at += sizeof(*val);
    };

    // general
    read(&status);
    read(&hp);
    read(&has_flag);
    read(&flag_count);
    read(&spawn_pos);
    read(&collision_damage);
    read(&respawn_timer);
    read(&submit_progression);
    read(&submitting);

    // weapon
    read(&torpedo_timer);
    read(&torpedo_cooldown);
    read(&torpedo_speed);

    // radar
    read(&normal_radar_interval);
    read(&normal_radar_malfunction_change);
    read(&normal_radar_range);
    read(&normal_radar_info_duration);
    read(&super_radar_cooldown);
    read(&super_radar_info_duration);

    // light
    read(&light_on);
    read(&player_facing);
};