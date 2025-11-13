#include "PlayMode.hpp"
#include "Registry.hpp"

void PlayMode::update_ui(float elapsed)
{
    // Torpedo cooldown
    std::string torpedo_text = std::string("READY!!");
    if (player_data[local_player->id].torpedo_timer <= Player::TORPEDO_COOLDOWN)
    {
        torpedo_text = std::to_string(player_data[local_player->id].torpedo_timer);
    }
    text_overlays[GUI].update_text("torpedo_cooldown", torpedo_text, glm::vec2(-200, 10), UIOverlay::BottomRight);

    // HP Text
    text_overlays[GUI].update_text(HP, "HP: " + std::to_string((int)player_data[local_player->id].hp), glm::vec2(-200, -100), UIOverlay::TopRight);

    // Player flag text
    text_overlays[GUI].remove_texts([](std::string const &key)
                                    { return key.rfind("Flag_", 0) == 0; });
    auto players = get_objects(ObjectType::Player);
    for (auto &player : players)
    {
        auto data = player_data.find(player.id);
        if (data != player_data.end())
        {
            std::string pk = "Flag_" + std::to_string(player.id);
            glm::vec2 pos = world_to_screen(glm::vec3(player.position, 0), text_overlays[GUI].renderer);
            text_overlays[GUI].update_text(pk, data->second.has_flag ? "⚑" : "", pos + glm::vec2(-10.0f, 20.0f));
        }
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
    glm::vec3 spawn(player_data[local_player->id].spawn_pos, 0);
    hud.draw(spawn + glm::vec3(-5, 0, 0), spawn + glm::vec3(5, 0, 0), glm::u8vec4(0, 255, 0, 255));
    hud.draw(spawn + glm::vec3(0, -5, 0), spawn + glm::vec3(0, 5, 0), glm::u8vec4(0, 255, 0, 255));
    // draw radar
    radar.render(hud);

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
