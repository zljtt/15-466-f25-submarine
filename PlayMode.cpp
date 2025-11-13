#include "PlayMode.hpp"
#include "Registry.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "Prefab.hpp"
#include "Sound.hpp"
#include "TextEngine.hpp"
#include "GameObject.hpp"

#include "LitColorTextureProgram.hpp"
#include "ColorTextureProgram.hpp"

#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <array>

#include "load_save_png.hpp"

const std::string PlayMode::HP = "HP";

PlayMode::PlayMode(Client &client_) : scene(*prototype_scene), radar(this), client(client_)
{
    // get pointer to camera for convenience:
    if (scene.cameras.size() != 1)
        throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
    camera = &scene.cameras.front();

    std::vector<GameObject> obstacles;
    auto on_drawable = [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name)
    {
        // create collision box
        // local_obstacles.emplace_back(transform->position, transform->scale);
        obstacles.emplace_back(transform->position, transform->scale);
    };
    Scene(data_path("prototype.scene"), on_drawable);
    bvh.build(std::move(obstacles));

    // create TextEngine and load a font
    // if (!text_engine)
    //     text_engine = std::make_unique<TextEngine>();
    // text_engine->init();
    // if (font_id == -1)
    // {
    //     auto path = "DejaVuSans.ttf";
    //     font_id = text_engine->load_font(data_path(path), 24);
    // }
    // int w, h;
    text_overlays.emplace_back(renderer_gui.value); // GUI
    // text_overlays[GUI].update_text("test", "test", glm::vec2(500, 400), UIOverlay::BottomLeft);
    text_overlays.emplace_back(renderer_radar.value); // RADAR
    // text_overlays[RADAR].update_image("test", *tex_radar_result, glm::vec2(100, 100), glm::vec2(100, 100));
}

PlayMode::~PlayMode()
{
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{

    if (evt.type == SDL_EVENT_KEY_DOWN)
    {
        if (evt.key.repeat)
        {
            // ignore repeats
        }
        else if (evt.key.key == SDLK_ESCAPE)
        {
            SDL_SetWindowRelativeMouseMode(Mode::window, false);
            return true;
        }
        else if (evt.key.key == SDLK_A)
        {
            controls.left.downs += 1;
            controls.left.pressed = true;
            return true;
        }
        else if (evt.key.key == SDLK_D)
        {
            controls.right.downs += 1;
            controls.right.pressed = true;
            return true;
        }
        else if (evt.key.key == SDLK_W)
        {
            controls.up.downs += 1;
            controls.up.pressed = true;
            return true;
        }
        else if (evt.key.key == SDLK_S)
        {
            controls.down.downs += 1;
            controls.down.pressed = true;
            return true;
        }
        else if (evt.key.key == SDLK_SPACE)
        {
            controls.jump.downs += 1;
            controls.jump.pressed = true;
            return true;
        }
        else if (evt.key.key == SDLK_R)
        {
            controls.radar.downs += 1;
            controls.radar.pressed = true;
            return true;
        }
    }
    else if (evt.type == SDL_EVENT_KEY_UP)
    {
        if (evt.key.key == SDLK_A)
        {
            controls.left.pressed = false;
            return true;
        }
        else if (evt.key.key == SDLK_D)
        {
            controls.right.pressed = false;
            return true;
        }
        else if (evt.key.key == SDLK_W)
        {
            controls.up.pressed = false;
            return true;
        }
        else if (evt.key.key == SDLK_S)
        {
            controls.down.pressed = false;
            return true;
        }
        else if (evt.key.key == SDLK_SPACE)
        {
            controls.jump.pressed = false;
            return true;
        }
        else if (evt.key.key == SDLK_R)
        {
            controls.radar.pressed = false;
            return true;
        }
    }
    else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (SDL_GetWindowRelativeMouseMode(Mode::window) == false)
        {
            SDL_SetWindowRelativeMouseMode(Mode::window, true);
            return true;
        }
    }

    return false;
}

void PlayMode::execute_network_soundcues(ObjectType type, uint8_t sc, glm::vec3 pos, uint32_t id)
{
    if (type == ObjectType::Player)
    {
        if (toPlay(sc, SoundCues::Start))
        {
            // std::cout << "start engine" << id << std::endl;
            if (sub_moving.find(id) == sub_moving.end())
                sub_moving[id] = Sound::loop_3D(*Submarine_Moving, 0.0f, pos, 10.0f);
            sub_moving[id]->set_volume(1.0f);
        }
        if (toPlay(sc, SoundCues::Stop))
        {
            // std::cout << "stop engine" << id << std::endl;
            sub_moving[id]->set_volume(0.0f);
        }
        if (toPlay(sc, SoundCues::Hit))
        {
        }
        if (toPlay(sc, SoundCues::GetPoint))
        {
        }
    }
}

void PlayMode::update(float elapsed)
{
    update_control(elapsed);
    update_connection(elapsed);
    update_radar(elapsed);
    update_camera(elapsed);
    update_sound(elapsed);
    update_spotlight(elapsed);
    update_ui(elapsed);
}

void PlayMode::update_control(float elapsed)
{
    // queue data for sending to server:
    controls.send_controls_message(&client.connection);

    if (controls.radar.downs)
    {
        radar.scan_special(local_player, 999);
    }

    // reset button press counters:
    controls.left.downs = 0;
    controls.right.downs = 0;
    controls.up.downs = 0;
    controls.down.downs = 0;
    controls.jump.downs = 0;
    controls.radar.downs = 0;
}

void PlayMode::update_connection(float elapsed)
{
    // send/receive data:
    client.poll([this](Connection *c, Connection::Event event)
                {
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			//std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush(); //DEBUG
			bool handled_message;
			try {
				do {
					handled_message = false;
					if (recv_state_message(c)) handled_message = true;
				} while (handled_message);
			} catch (std::exception const &e) {
				std::cerr << "[" << c->socket << "] malformed message from server: " << e.what() << std::endl;
				//quit the game:
				throw e;
			}
		} }, 0.0);
}

void PlayMode::update_radar(float elapsed)
{
    radar_timer -= elapsed;
    if (radar_timer < 0)
    {
        radar_timer = Radar::RADAR_INTERVAL;
        radar.scan(local_player, Radar::RADAR_RANGE, Radar::RADAR_RAY_COUNT);
    }
    radar.update(elapsed);
}

void PlayMode::update_camera(float elapsed)
{
    glm::vec2 local_pos = network_drawables[local_player->id]->transform->position;
    camera->transform->position = glm::vec3(local_pos.x, local_pos.y, camera->transform->position.z);
}

void PlayMode::update_sound(float elapsed)
{
    // go through all the network objects to see which has a sound to play

    for (auto &netObj : network_objects)
    {
        if (netObj.sound_cues != 0)
        {
            execute_network_soundcues(netObj.type, netObj.sound_cues, glm::vec3(netObj.position, 0), netObj.id);
            // clear the sound_cues
            netObj.sound_cues = 0;
        }
    }
    // change the position of submarine moving sound
    for (auto it = sub_moving.begin(); it != sub_moving.end();)
    {
        uint32_t id = it->first;
        // if id doesn't exist in players, it's removed
        if (network_drawables.find(id) == network_drawables.end())
        {
            it = sub_moving.erase(it);
        }
        else
        {
            glm::vec3 pos = network_drawables[id]->transform->position;
            it->second->set_position(pos);
            // std::cout<<"here"<<std::endl;
            it++;
        }
    }
}

void PlayMode::update_spotlight(float elapsed)
{
    // calculate spotlight direction
    // glm::vec2 cur_player_pos = local_player_pos();
    // glm::vec2 velocity = cur_player_pos - prev_player_pos;
    // prev_player_pos = cur_player_pos;

    // if (velocity.x > 1e-3f)
    //     spot_light_dir_x = 1.0f;
    // else if (velocity.x < -1e-3f)
    //     spot_light_dir_x = -1.0f;
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
    // update camera aspect ratio for drawable:
    camera->aspect = float(drawable_size.x) / float(drawable_size.y);

    glm::vec2 player_pos = local_player_pos();
    float depth = glm::max(0.0f, water_surface_y - player_pos.y);
    float atten = glm::clamp(1.0f - atten_speed * depth, 0.0f, 1.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    glUseProgram(lit_color_texture_program->program);
    glUniform1f(lit_color_texture_program->TILES_PER_UNIT_float, 0.1f);
    glUseProgram(0);

    // environment light
    {
        glm::vec3 hemi_light_dir(0.0f, 0.0f, -1.0f);
        glm::vec3 surface_light_energy(1.0f, 1.0f, 0.95f);
        glm::vec3 underwater_light_energy = surface_light_energy * atten;

        glUseProgram(lit_color_texture_program->program);
        glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
        glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(hemi_light_dir));
        glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(underwater_light_energy));
        glUseProgram(0);

        scene.draw(*camera);
    }

    // player point light
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDepthFunc(GL_EQUAL);

        glm::vec3 point_light_pos(player_pos.x, player_pos.y, 1.5f);
        glm::vec3 point_light_energy(0.3f, 0.3f, 0.3f);
        // glm::vec3 point_light_energy(0.0f, 0.0f, 0.0f);

        glUseProgram(lit_color_texture_program->program);
        glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 0);
        glUniform3fv(lit_color_texture_program->LIGHT_LOCATION_vec3, 1, glm::value_ptr(point_light_pos));
        glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(point_light_energy));
        glUseProgram(0);

        scene.draw(*camera);

        glDisable(GL_BLEND);
    }

    // player spot light
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDepthFunc(GL_EQUAL);

        glm::vec3 spot_light_pos(player_pos.x, player_pos.y, 0.0f);
        glm::vec3 spot_light_energy(5.0f, 5.0f, 5.0f);

        // glm::vec3 spot_light_dir(1.0f, 0.0f, 0.0f);
        glm::vec3 spot_light_dir(player_data[local_player->id].player_facing ? 1.0f : -1.0f, 0.0f, 0.0f);
        spot_light_dir = glm::normalize(spot_light_dir);
        float cos_cutoff = std::cos(cutoff);

        glUseProgram(lit_color_texture_program->program);
        glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 2);
        glUniform3fv(lit_color_texture_program->LIGHT_LOCATION_vec3, 1, glm::value_ptr(spot_light_pos));
        glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(spot_light_dir));
        glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(spot_light_energy));
        glUniform1f(lit_color_texture_program->LIGHT_CUTOFF_float, cos_cutoff);
        glUseProgram(0);

        scene.draw(*camera);

        glDisable(GL_BLEND);
    }

    draw_overlay(drawable_size);

    GL_ERRORS();
}

glm::vec2 PlayMode::local_player_pos()
{
    glm::vec3 p = network_drawables[local_player->id]->transform->position;
    return {p.x, p.y};
}

bool PlayMode::recv_state_message(Connection *connection_)
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

    network_objects.clear();
    uint8_t network_objects_count;
    read(&network_objects_count);
    for (uint8_t i = 0; i < network_objects_count; ++i)
    {

        network_objects.emplace_back();
        NetworkObject &obj = network_objects.back();
        if (obj.sound_cues != 0)
        {
            std::cout << "what the fuck" << std::endl;
        };
        obj.receive(&at, recv_buffer);
        // find local player
        if (i == 0)
        {
            local_player = &obj;
        }
        // find drawable
        auto drawable = network_drawables.find(obj.id);
        // delete if mark deleted
        if (obj.deleted)
        {
            if (drawable != network_drawables.end())
            {
                scene.drawables.remove_if([&](const Scene::Drawable &d)
                                          { return &d == drawable->second; });
                network_drawables.erase(obj.id);
            }
            network_objects.pop_back();
            continue;
        }
        // create drawable if not exit on client
        if (drawable == network_drawables.end())
        {
            network_drawables[obj.id] = create_drawable_at(scene, obj.type, glm::vec3(obj.position, 0), glm::vec3(obj.scale, 1));
        }
        // update drawable position
        else
        {
            // std::cout << "update torpedo\n";
            drawable->second->transform->position = glm::vec3(obj.position, 0);
            drawable->second->transform->scale = glm::vec3(obj.scale, 1);
        }
    }

    uint8_t player_data_count;
    read(&player_data_count);
    for (uint8_t i = 0; i < player_data_count; ++i)
    {
        uint32_t player_id;
        read(&player_id);
        Player::PlayerData data;
        data.receive(&at, recv_buffer);
        player_data[player_id] = data;
    }

    if (at != size)
    {
        throw std::runtime_error("Trailing data in state message.");
    }

    // delete message from buffer:
    recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

    return true;
}

glm::vec2 PlayMode::world_to_screen(glm::vec2 worldPos, const UIRenderer *renderer) const
{
    glm::vec4 clip = camera->make_projection() * camera->make_view() * glm::vec4(worldPos, 0.0f, 1.0f);

    if (clip.w == 0.0f)
        return glm::vec2(-9999.0f, -9999.0f);
    glm::vec3 ndc = glm::vec3(clip) / clip.w;

    glm::vec2 screen_size = renderer->get_size();
    float x = (ndc.x * 0.5f + 0.5f) * screen_size[0];
    // float y = (1.0f - (ndc.y * 0.5f + 0.5f)) * screen_size[1];
    float y = (ndc.y * 0.5f + 0.5f) * screen_size[1];

    return glm::vec2(x, y);
}

glm::vec2 PlayMode::get_screen_size() const
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    return glm::vec2(w, h);
}

UIOverlay &PlayMode::get_overlay(int id)
{
    return text_overlays[id];
}

std::vector<NetworkObject> PlayMode::get_objects(ObjectType type) const
{
    std::vector<NetworkObject> ret;
    ret.clear();
    for (auto &obj : network_objects)
    {
        if (obj.type == type)
        {
            ret.push_back(obj);
        }
    }
    return ret;
}