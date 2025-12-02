#include "PlayMode.hpp"
#include "Registry.hpp"
#include <iomanip>
#include <sstream>

static Player::Status prev;

void PlayMode::add_notification(std::string message, float time)
{
    notifications.emplace_back(message, time);
}

void PlayMode::update_notification(float elapsed)
{
    // update text ui
    text_overlays[NOTIFICATION].clear();
    for (size_t i = 0; i < notifications.size(); i++)
    {
        text_overlays[NOTIFICATION].update_text("notification_" + std::to_string(i),
                                                notifications[i].text, glm::vec2(10.0f, -50.0f - i * 50.0f),
                                                UIOverlay::TopLeft);
        notifications[i].time -= elapsed;
    }
    notifications.erase(
        std::remove_if(notifications.begin(), notifications.end(),
                       [](const Notification &n)
                       { return n.time < 0.0f; }),
        notifications.end());
}

void PlayMode::update_ui(float elapsed)
{
    // on status change
    if (local_player_data().status != prev)
    {
        text_overlays[GUI].clear();
        text_overlays[LARGE_TEXT].clear();
    }
    prev = local_player_data().status;
    // loop status
    switch (local_player_data().status)
    {

    case Player::NotReady:
    {
        text_overlays[GUI].update_image("select_ship", ui_select_ship->tex, glm::vec2(640, 410), glm::vec2(640, 360) - glm::vec2(640, 410) / 2.0f);
        text_overlays[GUI].update_text("ship1", "Explorer", glm::vec2(-150, 510), UIOverlay::Bottom, false);
        text_overlays[GUI].update_text("ship1_explain", "A strong radar, but sometimes malfunction", glm::vec2(-150, 460), UIOverlay::Bottom, false);
        text_overlays[GUI].update_text("ship2", "Fighter", glm::vec2(-150, 380), UIOverlay::Bottom, false);
        text_overlays[GUI].update_text("ship2_explain", "Higher HP, but weaker radar.", glm::vec2(-150, 330), UIOverlay::Bottom, false);
        text_overlays[GUI].update_text("ship3", "Seeker", glm::vec2(-150, 250), UIOverlay::Bottom, false);
        text_overlays[GUI].update_text("ship3_explain", "Strong and undetectable super scan, but weaker radar.", glm::vec2(-150, 200), UIOverlay::Bottom, false);
        text_overlays[GUI].update_text("prompt", "Select Ship using key [1,2,3]", glm::vec2(-100, 100), UIOverlay::Bottom, false);

        text_overlays[GUI].update_text("key0", "[Keybind]", glm::vec2(10, -20), UIOverlay::TopLeft, false);
        text_overlays[GUI].update_text("key1", "Move: WASD", glm::vec2(10, -40), UIOverlay::TopLeft, false);
        text_overlays[GUI].update_text("key2", "Toggle Flashlight: L", glm::vec2(10, -60), UIOverlay::TopLeft, false);
        text_overlays[GUI].update_text("key3", "Super Scan: R", glm::vec2(10, -80), UIOverlay::TopLeft, false);
        text_overlays[GUI].update_text("key4", "Launch Torpedo: Space", glm::vec2(10, -100), UIOverlay::TopLeft, false);

        text_overlays[GUI].update_text("hint0", "[Icons] (Position of the Object)", glm::vec2(10, -150), UIOverlay::TopLeft, false);
        text_overlays[GUI].update_image("hint1", tex_radar_flag->tex, glm::vec2(50, 50), glm::vec2(10, -200), UIOverlay::TopLeft);
        text_overlays[GUI].update_text("hint1", "Black Box", glm::vec2(60, -180), UIOverlay::TopLeft, false);
        text_overlays[GUI].update_image("hint2", tex_radar_submit_point->tex, glm::vec2(50, 50), glm::vec2(10, -250), UIOverlay::TopLeft);
        text_overlays[GUI].update_text("hint2", "Black Box Submit Point", glm::vec2(60, -230), UIOverlay::TopLeft, false);
        text_overlays[GUI].update_image("hint3", tex_radar_submarine->tex, glm::vec2(50, 50), glm::vec2(10, -300), UIOverlay::TopLeft);
        text_overlays[GUI].update_text("hint3", "Other submarine", glm::vec2(60, -280), UIOverlay::TopLeft, false);
        text_overlays[GUI].update_image("hint4", tex_radar_radar->tex, glm::vec2(50, 50), glm::vec2(10, -350), UIOverlay::TopLeft);
        text_overlays[GUI].update_text("hint4", "Submarine using Super Scan", glm::vec2(60, -330), UIOverlay::TopLeft, true);

        break;
    }
    case Player::Ready:
    {
        int i = 0;
        for (auto player : player_data)
        {
            i++;
            auto ready = (player.second.status == Player::Ready) ? "Ready" : "Not Ready";
            // auto local = (player.first == local_player->id) ? " (YOU)" : "";
            auto text = player.second.pname + ": " + ready;
            text_overlays[GUI].update_text("p" + std::to_string(player.first), text, glm::vec2(-100, 440 - i * 50), UIOverlay::Bottom);
        }
        auto pc = "(" + std::to_string(i) + "/" + std::to_string(level_data.max_player) + ")";
        text_overlays[GUI].update_text("ready", "Waiting for Player " + pc, glm::vec2(-100, 440), UIOverlay::Bottom);
        break;
    }
    case Player::Play:
    {
        // points
        int i = 0;
        text_overlays[GUI].update_text("p0", "Black Box Submitted", glm::vec2(-200, -20), UIOverlay::TopRight);
        for (auto player : player_data)
        {
            i++;
            // ⚑
            auto count = std::to_string(player.second.flag_count) + "/" + std::to_string(Game::MaxBlackBox);
            text_overlays[GUI].update_text("p1_" + std::to_string(player.first), player.second.pname, glm::vec2(-200, -20 - i * 40), UIOverlay::TopRight);
            text_overlays[GUI].update_text("p2_" + std::to_string(player.first), count, glm::vec2(-100, -20 - i * 40), UIOverlay::TopRight);
        }

        // Torpedo cooldown
        std::string torpedo_text = std::string("Torpedo : READY!!");
        if (local_player_data().torpedo_timer <= Player::TORPEDO_COOLDOWN)
        {
            float t = int((Player::TORPEDO_COOLDOWN - local_player_data().torpedo_timer) * 100.0f) / 100.0f;
            torpedo_text = "Torpedo : " + std::to_string(t);
        }
        text_overlays[GUI].update_text("torpedo_cooldown", torpedo_text, glm::vec2(-200, 20), UIOverlay::BottomRight);
        std::string radar_text = std::string("Super Scan : READY!!");
        if (radar.special_radar_timer > 0)
        {
            float t = int(radar.special_radar_timer * 100.0f) / 100.0f;
            radar_text = "Super Scan : " + std::to_string(t);
        }
        text_overlays[GUI].update_text("super_radar_cooldown", radar_text, glm::vec2(-200, 40), UIOverlay::BottomRight);
        // HP Text
        text_overlays[LARGE_TEXT].update_text(HP, "HP: " + std::to_string((int)local_player_data().hp), glm::vec2(-50, 30), UIOverlay::Bottom);

        // Submit flag
        text_overlays[LARGE_TEXT].remove_texts([](std::string const &key)
                                               { return key == "submitting"; });
        if (local_player_data().submitting)
        {
            int progress = int((local_player_data().submit_progression / 10.0f) * 100.0f);
            text_overlays[LARGE_TEXT].update_text("submitting", "Submitting BlackBox - " + std::to_string(progress) + "%", glm::vec2(-160, -200), UIOverlay::Top);
        }
        else if (local_player_data().has_flag)
        {
            text_overlays[LARGE_TEXT].update_text("submitting", "Carry BlackBox to Submit Point!", glm::vec2(-200, -50), UIOverlay::Top);
        }
        else
        {
            text_overlays[LARGE_TEXT].update_text("submitting", "Find BlackBox", glm::vec2(-75, -50), UIOverlay::Top);
        }

        // Player flag text
        text_overlays[GUI].remove_images([](std::string const &key)
                                         { return key.rfind("Flag_", 0) == 0; });
        auto players = get_objects(ObjectType::Player);
        for (auto &player : players)
        {
            auto data = player_data.find(player.id);
            if (data != player_data.end() && data->second.has_flag)
            {
                std::string pk = "Flag_" + std::to_string(player.id);
                auto renderer = text_overlays[GUI].renderer;
                glm::vec2 pos = world_to_screen(glm::vec3(player.position, 0), renderer);
                // glm::vec2 pos = client_game->world_to_screen(glm::vec3(p.position, 0), renderer);

                pos.x = std::clamp(pos.x - 50 / 2.0f, 0.0f, (float)renderer->width - 50);
                pos.y = std::clamp(pos.y - 50 / 2.0f, 0.0f, (float)renderer->height - 50);
                text_overlays[GUI].update_image(pk, tex_radar_flag->tex, glm::vec2(50, 50), pos);
                // text_overlays[GUI].update_text(pk, data->second.has_flag ? "⚑" : "", pos + glm::vec2(-10.0f, 20.0f));
            }
        }
        break;
    }
    case Player::RespawnSelect:
    {
        std::stringstream spawntime;
        spawntime << std::fixed << std::setprecision(2) << local_player_data().respawn_timer;
        text_overlays[GUI].update_text("respawn_timer", "Respawn in " + spawntime.str() + " seconds", glm::vec2(-100, 600), UIOverlay::Bottom);
        text_overlays[GUI].update_image("select_ship", ui_select_ship->tex, glm::vec2(640, 410), glm::vec2(640, 360) - glm::vec2(640, 410) / 2.0f);
        text_overlays[GUI].update_text("ship1", "Explorer", glm::vec2(-150, 510), UIOverlay::Bottom);
        text_overlays[GUI].update_text("ship1_explain", "A strong radar, but sometimes malfunction", glm::vec2(-150, 460), UIOverlay::Bottom);
        text_overlays[GUI].update_text("ship2", "Fighter", glm::vec2(-150, 380), UIOverlay::Bottom);
        text_overlays[GUI].update_text("ship2_explain", "Higher HP, but weaker radar.", glm::vec2(-150, 330), UIOverlay::Bottom);
        text_overlays[GUI].update_text("ship3", "Seeker", glm::vec2(-150, 250), UIOverlay::Bottom);
        text_overlays[GUI].update_text("ship3_explain", "Strong and undetectable super scan, but weaker radar.", glm::vec2(-150, 200), UIOverlay::Bottom);
        text_overlays[GUI].update_text("prompt", "Select Ship using [1,2,3]", glm::vec2(-100, 100), UIOverlay::Bottom);

        break;
    }
    case Player::RespawnWait:
    {
        std::stringstream spawntime;
        spawntime << std::fixed << std::setprecision(2) << local_player_data().respawn_timer;
        text_overlays[GUI].update_text("respawn_timer", "Respawn in " + spawntime.str() + " seconds", glm::vec2(-100, 600), UIOverlay::Bottom);
        break;
    }
    case Player::GameOver:
    {
        int i = 0;
        for (auto player : player_data)
        {
            i++;
            if (player.second.flag_count >= Game::MaxBlackBox)
            {
                auto name = (player.first == local_player->id) ? "You win the game!" : "Player " + std::to_string(i) + " wins the game!";
                text_overlays[LARGE_TEXT].update_text("win/lose", name, glm::vec2(-100, 300), UIOverlay::Bottom);
                break;
            }
        }
        break;
    }
    default:
        break;
    }
}

void PlayMode::draw_overlay(glm::uvec2 const &drawable_size)
{
    glDisable(GL_DEPTH_TEST);

    float aspect = float(drawable_size.x) / float(drawable_size.y);
    glm::vec3 eye = camera->transform->position;
    glm::vec3 fwd = {0, 0, -1};
    glm::vec3 up = {0, 1, 0};
    glm::mat4 V = glm::lookAt(eye, eye + fwd, up);
    glm::mat4 P = glm::perspective(camera->fovy, aspect, 0.1f, 1000.0f);
    DrawLines hud(P * V);

    // draw spawn point
    // glm::vec3 spawn(local_player_data().spawn_pos, 0);
    if (local_player_data().status == Player::Play)
    {
        // hud.draw(spawn + glm::vec3(-5, 0, 0), spawn + glm::vec3(5, 0, 0), glm::u8vec4(0, 255, 0, 255));
        // hud.draw(spawn + glm::vec3(0, -5, 0), spawn + glm::vec3(0, 5, 0), glm::u8vec4(0, 255, 0, 255));
        // draw radar
        radar.render(hud);
    }

    // render text
    for (auto text_overlay : text_overlays)
    {
        text_overlay.draw(drawable_size);
    }

    // draw health
    // if (font_id >= 0 && local_player)
    // {
    //     GLint vp[4];
    //     glGetIntegerv(GL_VIEWPORT, vp);
    //     float win_w = float(vp[2]);
    //     float win_h = float(vp[3]);

    //     std::string health_text = "Health: " + std::to_string(player_data.hp);

    //     // Center the text
    //     float text_x = win_w / 2.0f; // tweak offset to roughly center
    //     float text_y = win_h / 2.0f;

    //     // text_engine->render_text(font_id, health_text, text_x, text_y);
    // }

    // auto draw_point = [&](glm::vec3 p, Trace hit)
    // {
    //     GLuint vao, vbo;
    //     glGenVertexArrays(1, &vao);
    //     glBindVertexArray(vao);

    //     glGenBuffers(1, &vbo);
    //     glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //     glBufferData(GL_ARRAY_BUFFER, sizeof(p), &p, GL_STATIC_DRAW);

    //     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    //     glEnableVertexAttribArray(0);

    //     glPointSize(6.0f);
    //     glDrawArrays(GL_POINTS, 0, 1);
    // };
}
