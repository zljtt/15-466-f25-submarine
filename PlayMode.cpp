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
#include "BasicMaterialForwardProgram.hpp"

#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <array>
#include <cstring>

#include "load_save_png.hpp"


std::mt19937 rng(std::random_device{}());

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
        if (mesh_name == "BackgroundPlane")
            return;

        // create collision box
        // local_obstacles.emplace_back(transform->position, transform->scale);
        obstacles.emplace_back(transform->position, transform->scale);
    };
    Scene(data_path("resource/scene/prototype.scene"), on_drawable);
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
    text_overlays.emplace_back(renderer_radar.value);        // RADAR
    text_overlays.emplace_back(renderer_notification.value); // NOTIFICATION
    text_overlays.emplace_back(renderer_large.value);        // NOTIFICATION


    //Background Music
    BGM_Shallow = Sound::loop(*Shallow_BGM,0.1f);
    BGM_Deep = Sound::loop(*Deep_BGM,0.0f);
    BGM_Urgent = Sound::loop(*Drums,0.0f);
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
        else if (evt.key.key == SDLK_Q)
        {
            controls.rotate_left.downs += 1;
            controls.rotate_left.pressed = true;
            return true;
        }
        else if (evt.key.key == SDLK_R)
        {
            controls.rotate_right.downs += 1;
            controls.rotate_right.pressed = true;
            return true;
        }
        else if (evt.key.key == SDLK_1)
        {
            controls.num1.downs += 1;
            controls.num1.pressed = true;
            return true;
        }
        else if (evt.key.key == SDLK_2)
        {
            controls.num2.downs += 1;
            controls.num2.pressed = true;
            return true;
        }
        else if (evt.key.key == SDLK_3)
        {
            controls.num3.downs += 1;
            controls.num3.pressed = true;
            return true;
        }
        else if (evt.key.key == SDLK_4)
        {
            controls.num4.downs += 1;
            controls.num4.pressed = true;
            return true;
        }
        else if (evt.key.key == SDLK_L)
        {
            controls.light.downs += 1;
            controls.light.pressed = true;
            return true;
        }
        //
        // else if (evt.key.key == SDLK_U)
        // {
        //     camera->transform->position.z -= 10.0f;
        //     return true;
        // }
        // else if (evt.key.key == SDLK_J)
        // {
        //     camera->transform->position.z += 10.0f;
        //     return true;
        // }
        // else if (evt.key.key == SDLK_I)
        // {
        //     spotlight_z -= 1.0f;
        //     return true;
        // }
        // else if (evt.key.key == SDLK_K)
        // {
        //     spotlight_z += 1.0f;
        //     return true;
        // }
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
        else if (evt.key.key == SDLK_Q)
        {
            controls.rotate_left.pressed = false;
            return true;
        }
        else if (evt.key.key == SDLK_E)
        {
            controls.rotate_right.pressed = false;
            return true;
        }
        else if (evt.key.key == SDLK_1)
        {
            controls.num1.pressed = false;
            return true;
        }
        else if (evt.key.key == SDLK_2)
        {
            controls.num2.pressed = false;
            return true;
        }
        else if (evt.key.key == SDLK_3)
        {
            controls.num3.pressed = false;
            return true;
        }
        else if (evt.key.key == SDLK_4)
        {
            controls.num4.pressed = false;
            return true;
        }
        else if (evt.key.key == SDLK_L)
        {
            controls.light.pressed = false;
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
        float atten = std::min(1.5f, 3.0f/glm::length(glm::vec2(pos.x,pos.y)- local_player->position));

        if (toPlay(sc, SoundCues::Start))
        {
            // std::cout << "start engine" << id << std::endl;
            if (sub_moving.find(id) == sub_moving.end())
                sub_moving[id] = Sound::loop_3D(*Submarine_Moving, 0.0f, pos , 0.2f);
            sub_moving[id]->set_volume(atten*66.0f, 1.0f);
            
            if (!sub_stop[id] || sub_stop[id]->stopped)
            {
                sub_start[id] = Sound::play_3D(*Submarine_Start, atten*16.0f, pos ,1.0f);
            }
        }
        if (toPlay(sc, SoundCues::Stop))
        {
            // std::cout << "stop engine" << id << std::endl;
            if (!sub_stop[id] || sub_stop[id]->stopped)
            {
                sub_stop[id] = Sound::play_3D(*Submarine_Stop, atten*16.0f, pos ,1.0f);
            }

            sub_moving[id]->set_volume(0.0f, 1.0f);
        }
        if(toPlay(sc,SoundCues::Hit_TORP)){
            //std::cout<<"torpedo_Hit"<<std::endl;
            if(id != local_player->id)Sound::play_3D(*Torpedo_Hit, 45.0f, pos, 0.5f);
        }
        if (toPlay(sc, SoundCues::Hit))
        {
            if (sub_hit && !sub_hit->stopped)
            {
                return;
            }
            if(id == local_player->id)
            sub_hit = Sound::play(*Submarine_Hit, 0.6f);
        }
        
        if (toPlay(sc, SoundCues::GetPoint))
        {
            Sound::play_3D(*Submarine_Submit,3.0f,pos,10.0f);
            //stop drms
            BGM_Urgent->set_volume(0.0f);
        }
        if(toPlay(sc,SoundCues::Capture)){
            std::cout<<"DRUMS "<<sc<<std::endl;
            BGM_Urgent->set_volume(0.8f,4.0f);
        }
    }
    else if(type == ObjectType::Torpedo){
        //std::cout<<"getting "<<sc<<std::endl;
        if(toPlay(sc,SoundCues::JustSpawned)){
            Sound::play_3D(*Submarine_Launch_Torpedo, 45.0f, pos, 0.5f);
        }
    }
    else if(type == ObjectType::Flag){
        
    }
}

void PlayMode::update(float elapsed)
{
    update_connection(elapsed);
    update_notification(elapsed);
    update_radar(elapsed);
    update_spotlight(elapsed);
    update_camera(elapsed);
    update_sound(elapsed);
    update_animation(elapsed);
    update_ui(elapsed);
    update_control(elapsed);
}

void PlayMode::update_control(float elapsed)
{
    // queue data for sending to server:
    controls.send_controls_message(&client.connection);
    // reset button press counters:
    controls.left.downs = 0;
    controls.right.downs = 0;
    controls.up.downs = 0;
    controls.down.downs = 0;
    controls.jump.downs = 0;
    controls.light.downs = 0;
    controls.radar.downs = 0;
    controls.num1.downs = 0;
    controls.num2.downs = 0;
    controls.num3.downs = 0;
    controls.num4.downs = 0;
    controls.rotate_left.downs = 0;
    controls.rotate_right.downs = 0;
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
                    if (recv_notification_message(c)) handled_message = true;
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
    if (controls.radar.downs && local_player_data().status == Player::Play && radar.special_radar_timer <= 0)
    {
        radar.scan_special(local_player, 999);
        std::cout<<"playing superscan"<<std::endl;
        Sound::play(*Submarine_Superscan,0.5f,0.0f);
    }
    radar.update(elapsed);
}

void PlayMode::update_camera(float elapsed)
{
    glm::vec2 local_pos = network_drawables[local_player->id]->transform->position;

    camera->transform->position = glm::vec3(local_pos.x, local_pos.y, camera->transform->position.z);
}

void PlayMode::update_animation(float elapsed)
{

    // trivial turning animation
    glm::vec3 eulerAnglesDegrees = glm::vec3(180.0f, 0.0f, 0.0f);
    glm::vec3 eulerAnglesRadians = glm::radians(eulerAnglesDegrees);
    network_drawables[local_player->id]->transform->rotation = local_player_data().player_facing ? glm::quat(eulerAnglesRadians) : glm::quat(0, 0, 0, 1);

    for (auto p : player_data)
    {
        // animating propeller, the faster the player is moving, the more it spins
        auto player = get_object(p.first);
        float player_speed = glm::length(player.velocity);
        if (player_speed > 0.01f)
        {
            size_t playerDrawable_count = network_drawables.count(p.first);
            if (playerDrawable_count)
            {
                auto playerDrawable = network_drawables[p.first];
                glm::quat rotation_x = glm::angleAxis(glm::radians(std::min(100.0f * player_speed * player_speed * elapsed, elapsed * 1000.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
                playerDrawable->transform->child->rotation = playerDrawable->transform->child->rotation * rotation_x;
            }
        }
    }

    // torpedo facing aligns with its velocity
    for (auto obj : network_objects)
    {
        if (obj.type == ObjectType::Torpedo)
        {
            float theta = std::atan2(obj.velocity.y, obj.velocity.x);
            glm::quat rotation = glm::angleAxis(theta, glm::vec3(0, 0, 1));
            network_drawables[obj.id]->transform->rotation = glm::quat(0, 0, 0, 1) * rotation;
        }
    }
}

void PlayMode::update_sound(float elapsed)
{
    //update background music

    //blend deep and shallow based on depth
    glm::vec2 player_pos = local_player_pos();
    float depth = glm::max(0.0f, water_surface_y - player_pos.y);
    float complete_fade_factor = 1.0f - glm::smoothstep(max_depth, max_depth + depth_transition, depth);
    BGM_Shallow->set_volume(0.2f*complete_fade_factor);
    BGM_Deep->set_volume(0.2f*(1.0f - complete_fade_factor));
    //play sparse melodies
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float r = dist(rng); 
    if (BGM_Shallow->looped.exchange(false, std::memory_order_acquire)) {
        if(r<0.2f && r>0.13){
            (complete_fade_factor > 0.5f) ? Sound::play(*Shallow_Melody1,0.5f) : Sound::play(*Deep_Melody1,0.5f);
        }
        if(r<0.13f && r>0.06){
            (complete_fade_factor > 0.5f) ? Sound::play(*Shallow_Melody2,0.5f) : Sound::play(*Deep_Melody2,0.5f);
        }
        if(r<0.06f){
            (complete_fade_factor > 0.5f) ? Sound::play(*Shallow_Melody3,0.5f) : Sound::play(*Deep_Melody3,0.5f);
        }
    }

    // go through all the network objects to see which has a sound to play
    Sound::listener.set_position_right(camera->transform->position, glm::vec3(1, 0, 0), 1.0f / 60.0f);

    for (auto &netObj : network_objects)
    {
        if (netObj.sound_cues != 0)
        {

            // Scene::Transform *loc = network_drawables[local_player->id]->transform;
            //std::cout << network_drawables[local_player->id]->pipeline.count << " " << loc->position.x << " " << loc->position.y << " " << loc->position.z << loc->scale.x << std::endl;
            execute_network_soundcues(netObj.type, netObj.sound_cues, glm::vec3(netObj.position, 1.5f), netObj.id);
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

    for (auto p : player_data)
    {
        light_mesh_data[p.first]->scale = p.second.light_on ? glm::vec3(3.0f, 3.0f, 10.0f) : glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 pos_offset = p.second.player_facing ? glm::vec3(11.0f, 0.0f, 0.0f) : glm::vec3(-11.0f, 0.0f, 0.0f);
        glm::quat rot = p.second.player_facing ? glm::angleAxis(glm::radians(-90.0f), glm::vec3(0, 1, 0)) : glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));
        light_mesh_data[p.first]->position = network_drawables[p.first]->transform->position + pos_offset;
        light_mesh_data[p.first]->rotation = rot;
    }
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
    // update camera aspect ratio for drawable:
    camera->aspect = float(drawable_size.x) / float(drawable_size.y);

    glm::vec2 player_pos = local_player_pos();
    float depth = glm::max(0.0f, water_surface_y - player_pos.y);

    // float t = glm::clamp(depth / max_depth, 0.0f, 1.0f);
    // float atten = (1.0f - t) * (1.0f - t);
    // float atten = pow((1.0f - t), 3);
    float atten = std::exp(-k * depth);
    float complete_fade_factor = 1.0f - glm::smoothstep(max_depth, max_depth + depth_transition, depth);
    atten *= complete_fade_factor;
    glm::vec3 water_color = base_water_color * atten;

    // std::cout << "player y: " << player_pos.y << std::endl;
    // std::cout << "depth: " << depth << std::endl;
    // std::cout << "atten: " << atten << std::endl;

    // std::cout << "camera transform: "
    // << camera->transform->position.x << " "
    // << camera->transform->position.y << " "
    // << camera->transform->position.z << " "
    // << std::endl;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    // multi-pass lighting:
    //  glUseProgram(lit_color_texture_program->program);
    //  glUniform1f(lit_color_texture_program->TILES_PER_UNIT_float, 0.1f);
    //  glUniform3fv(lit_color_texture_program->WATER_COLOR_vec3, 1, glm::value_ptr(water_color));
    //  glUniform3fv(lit_color_texture_program->CAMERA_POSITION_vec3, 1, glm::value_ptr(camera->transform->position + glm::vec3(0.0f, 0.0f, 0.0f)));
    //  glUseProgram(0);

    // forward rendering:
    glUseProgram(basic_material_forward_program->program);
    glUniform1f(basic_material_forward_program->TILES_PER_UNIT_float, 0.1f);
    glUniform3fv(basic_material_forward_program->WATER_COLOR_vec3, 1, glm::value_ptr(water_color));
    glUniform3fv(basic_material_forward_program->CAMERA_POSITION_vec3, 1, glm::value_ptr(camera->transform->position + glm::vec3(0.0f, 0.0f, 0.0f)));
    glUniform3fv(basic_material_forward_program->EYE_vec3, 1, glm::value_ptr(camera->transform->position));

    std::vector<GLint> light_types;
    std::vector<glm::vec3> light_poss;
    std::vector<glm::vec3> light_dirs;
    std::vector<glm::vec3> light_energies;
    std::vector<float> light_cutoffs;
    // environment light
    {
        glm::vec3 hemi_light_dir(0.0f, 0.0f, -1.0f);
        glm::vec3 surface_light_energy(1.0f, 1.0f, 0.95f);
        glm::vec3 underwater_light_energy = surface_light_energy * atten;

        light_types.push_back(1);
        light_poss.push_back(glm::vec3(0.0f));
        light_dirs.push_back(hemi_light_dir);
        light_energies.push_back(underwater_light_energy);
        light_cutoffs.push_back(0.0f);

        // glUseProgram(lit_color_texture_program->program);
        // glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
        // glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(hemi_light_dir));
        // glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(underwater_light_energy));
        // glUseProgram(0);

        // scene.draw(*camera);
    }

    //torpedo light
    for(auto torp: network_objects){
        if(torp.type != ObjectType::Torpedo){
            continue;
        }
        glm::vec3 point_light_pos(torp.position.x, torp.position.y, 2.5f);
        //determine the energy of the point light based on distance to local player
        float distance_diminish = std::min(1.0f, 3.0f/glm::length(torp.position - local_player->position));
        glm::vec3 point_light_energy(10.0f, 5.0f, 5.0f);
        // glm::vec3 point_light_energy(0.0f, 0.0f, 0.0f);

        light_types.push_back(0); // point
        light_poss.push_back(point_light_pos);
        light_dirs.push_back(glm::vec3(0.0f));
        light_energies.push_back(point_light_energy * distance_diminish);
        light_cutoffs.push_back(0.0f);

    }

    // player point light
    for(auto data : player_data)
    {
        auto player = get_object(data.first);
        glm::vec3 point_light_pos(player.position.x, player.position.y + 1.8f, 2.5f);
        //determine the energy of the point light based on distance to local player
        float distance_diminish = std::min(1.0f, 1.0f/glm::length(player.position - local_player->position));
        glm::vec3 point_light_energy(20.0f, 20.0f, 30.0f);
        // glm::vec3 point_light_energy(0.0f, 0.0f, 0.0f);

        light_types.push_back(0); // point
        light_poss.push_back(point_light_pos);
        light_dirs.push_back(glm::vec3(0.0f));
        light_energies.push_back(point_light_energy * distance_diminish);
        light_cutoffs.push_back(0.0f);

        // glEnable(GL_BLEND);
        // glBlendFunc(GL_ONE, GL_ONE);
        // glDepthFunc(GL_EQUAL);

        // glUseProgram(lit_color_texture_program->program);
        // glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 0);
        // glUniform3fv(lit_color_texture_program->LIGHT_LOCATION_vec3, 1, glm::value_ptr(point_light_pos));
        // glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(point_light_energy));
        // glUseProgram(0);

        // scene.draw(*camera);

        // glDisable(GL_BLEND);
    }

    // player spot light:
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_ONE, GL_ONE);
    // glDepthFunc(GL_EQUAL);
    for (auto data : player_data)
    {
        if (!data.second.light_on)
            continue;
        auto player = get_object(data.first);

        spotlight_z = 0.0f;
        // float spot_light_pos_x =    data.second.player_facing ? player.position.x - 21.0f : player.position.x + 21.0f;
        glm::vec3 spot_light_pos(player.position.x, player.position.y, spotlight_z);
        // glm::vec3 spot_light_energy(2500.0f, 2500.0f, 2500.0f);
        glm::vec3 spot_light_energy(250.0f, 250.0f, 250.0f);

        glm::vec3 spot_light_dir(data.second.player_facing ? 1.0f : -1.0f, 0.0f, 0.0f);
        spot_light_dir = glm::normalize(spot_light_dir);
        float cos_cutoff = std::cos(cutoff);

        // spot_light_dir = -spot_light_dir;

        // std::cout << "spotlight pos: "
        // << spot_light_pos.x << " "
        // << spot_light_pos.y << " "
        // << spot_light_pos.z << " "
        // << std::endl;

        // std::cout << "spotlight dir: "
        // << spot_light_dir.x << " "
        // << spot_light_dir.y << " "
        // << spot_light_dir.z << " "
        // << std::endl;

        light_types.push_back(2); // spot
        light_poss.push_back(spot_light_pos);
        light_dirs.push_back(spot_light_dir);
        light_energies.push_back(spot_light_energy);
        light_cutoffs.push_back(cos_cutoff);

        // glUseProgram(lit_color_texture_program->program);
        // glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 2);
        // glUniform3fv(lit_color_texture_program->LIGHT_LOCATION_vec3, 1, glm::value_ptr(spot_light_pos));
        // glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(spot_light_dir));
        // glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(spot_light_energy));
        // glUniform1f(lit_color_texture_program->LIGHT_CUTOFF_float, cos_cutoff);
        // glUseProgram(0);

        // scene.draw(*camera);
    }
    // glDisable(GL_BLEND);


    GLsizei light_count = static_cast<GLsizei>(light_types.size());

    if (int(light_count) > int(BasicMaterialForwardProgram::MaxLights))
    {
        light_count = BasicMaterialForwardProgram::MaxLights;
    }

    glUniform1ui(basic_material_forward_program->LIGHTS_uint, light_count);
    glUniform1iv(basic_material_forward_program->LIGHT_TYPE_int_array, light_count, light_types.data());
    glUniform3fv(basic_material_forward_program->LIGHT_LOCATION_vec3_array, light_count, glm::value_ptr(light_poss[0]));
    glUniform3fv(basic_material_forward_program->LIGHT_DIRECTION_vec3_array, light_count, glm::value_ptr(light_dirs[0]));
    glUniform3fv(basic_material_forward_program->LIGHT_ENERGY_vec3_array, light_count, glm::value_ptr(light_energies[0]));
    glUniform1fv(basic_material_forward_program->LIGHT_CUTOFF_float_array, light_count, light_cutoffs.data());
    glUseProgram(0);

    scene.draw(*camera);

    draw_overlay(drawable_size);

    GL_ERRORS();
}

glm::vec2 PlayMode::local_player_pos()
{
    glm::vec3 p = network_drawables[local_player->id]->transform->position;
    return {p.x, p.y};
}

bool PlayMode::recv_notification_message(Connection *connection_)
{
    assert(connection_);
    auto &connection = *connection_;
    auto &recv_buffer = connection.recv_buffer;
    if (recv_buffer.size() < 4)
        return false;
    if (recv_buffer[0] != uint8_t(Message::S2C_Notification))
        return false;
    uint32_t size = (uint32_t(recv_buffer[3]) << 16) | (uint32_t(recv_buffer[2]) << 8) | uint32_t(recv_buffer[1]);
    uint32_t at = 0;
    if (recv_buffer.size() < 4 + size)
        return false;

    auto read_string = [&]()
    {
        uint32_t len;
        std::memcpy(&len, &recv_buffer[4 + at], sizeof(uint32_t));
        at += sizeof(uint32_t);

        if (4 + at + len > recv_buffer.size())
        {
            throw std::runtime_error("Ran out of bytes reading notification.");
        }

        std::string s((char *)&recv_buffer[4 + at], len);
        at += len;
        return s;
    };

    std::string notification = read_string();
    add_notification(notification, 5.0f);

    if (at != size)
    {
        throw std::runtime_error("Trailing data in notification message.");
    }

    // delete message from buffer:
    recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

    return true;
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
            std::cout << "what the hec" << std::endl;
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
            std::cout << "scale is " << obj.scale.x << " " << obj.scale.y << std::endl;
            network_drawables[obj.id] = create_drawable_at(scene, obj.type, glm::vec3(obj.position, 0), glm::vec3(obj.scale, 1));

            if (obj.type == ObjectType::Player)
            {
                glm::quat rot = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0, 1, 0));

                Scene::Drawable *cone = prefab_spotlight_mesh->create_drawable(scene, glm::vec3(obj.position, 0), glm::vec3(3.0f, 3.0f, 10.0f), rot);

                cone->pipeline.is_transparent = true;
                cone->pipeline.set_uniforms = []()
                { glUniform1i(basic_material_forward_program->IS_LIGHT_CONE_int, 1); };

                light_mesh_data[obj.id] = cone->transform;
            }
        }
        // update drawable position
        else
        {
            drawable->second->transform->position = glm::vec3(obj.position, 0);
            drawable->second->transform->scale = glm::vec3(obj.scale, 1);
        }
    }
    // receive player data
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
    // receive level data
    level_data.receive(&at, recv_buffer);

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

NetworkObject PlayMode::get_object(uint32_t id) const
{
    NetworkObject o;
    for (auto &obj : network_objects)
    {
        if (obj.id == id)
        {
            return obj;
        }
    }
    return o;
}
