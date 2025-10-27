#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "LitColorTextureProgram.hpp"

#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <array>

GLuint meshes_for_lit_color_texture_program = 0;

Load<MeshBuffer> prototype_scene_meshes(LoadTagDefault, []() -> MeshBuffer const *
                                        {
	MeshBuffer const *ret = new MeshBuffer(data_path("prototype_scene.pnct"));
	meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

Load<Scene> prototype_scene(LoadTagDefault, []() -> Scene const *
                            { 
    auto on_drawable = [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name) {
        if (mesh_name == "Player") return;
        if (mesh_name == "Torpedo") return;
        Mesh const &mesh = prototype_scene_meshes->lookup(mesh_name);
        scene.drawables.emplace_back(transform);
        Scene::Drawable &drawable = scene.drawables.back();

        drawable.pipeline = lit_color_texture_program_pipeline;
        drawable.pipeline.vao = meshes_for_lit_color_texture_program;
        drawable.pipeline.type = mesh.type;
        drawable.pipeline.start = mesh.start;
        drawable.pipeline.count = mesh.count;
        
        scene.static_obstacles.emplace_back(transform->position, transform->scale);
    };
    return new Scene(data_path("prototype.scene"), on_drawable); });

PlayMode::PlayMode(Client &client_) : scene(*prototype_scene), client(client_)
{
    // get pointer to camera for convenience:
    if (scene.cameras.size() != 1)
        throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
    camera = &scene.cameras.front();
    // put static obstacles
    radar.put_obstacles(scene.static_obstacles);
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

void PlayMode::update(float elapsed)
{
    update_control(elapsed);
    update_connection(elapsed);
    update_radar(elapsed);
    update_camera(elapsed);
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
}

void PlayMode::update_connection(float elapsed)
{
    auto on_player = [&](Player &player)
    {
        auto drawable = network_drawables.find(player.id);
        // create drawable if not exit on client
        if (drawable == network_drawables.end())
        {
            network_drawables[player.id] = prefab_player->create_drawable(scene, glm::vec3(player.position, 0));
        }
        // update drawable position
        else
        {
            drawable->second->transform->position = glm::vec3(player.position, 0);
            drawable->second->transform->scale = glm::vec3(player.scale, 1);
        }
    };
    

    auto on_game_object = [&](NetworkObject &obj)
    {
        auto drawable = network_drawables.find(obj.id);
        // create drawable if not exit on client
        if (drawable == network_drawables.end())
        {
            // TODO: create drawable according to the object type
        } 
        // update drawable position
        else
        {
            drawable->second->transform->position = glm::vec3(obj.position, 0);
            drawable->second->transform->scale = glm::vec3(obj.scale, 1);
        }
    };

    auto on_torpedo = [&](Torpedo &torpedo)
    {
        std::cout<<"read a torpedo"<<std::endl;
        auto drawable = network_drawables.find(torpedo.id);
        // create drawable if not exit on client
        if (drawable == network_drawables.end())
        {
            network_drawables[torpedo.id] = prefab_torpedo->create_drawable(scene, glm::vec3(torpedo.position, 0));
        }
        // update drawable position
        else
        {
            drawable->second->transform->position = glm::vec3(torpedo.position, 0);
            drawable->second->transform->scale = glm::vec3(torpedo.scale, 1);
        }
    };
    // send/receive data:
    client.poll([this, on_player, on_game_object, on_torpedo](Connection *c, Connection::Event event)
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
					if (game.recv_state_message(c, on_player, on_game_object, on_torpedo)) handled_message = true;
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
        // copy player and dynamic object to additional
        std::list<GameObject> additional;
        for (auto &p : game.players)
        {
            if (&p != game.local_player)
                additional.push_back(p);
        }
        additional.insert(additional.end(), game.game_objects.begin(), game.game_objects.end());
        radar.additional = additional;
        radar.scan(game.local_player, Radar::RADAR_RANGE, Radar::RADAR_RAY_COUNT);
    }
}

void PlayMode::update_camera(float elapsed)
{
    glm::vec2 local_pos = network_drawables[game.local_player->id]->transform->position;
    camera->transform->position = glm::vec3(local_pos.x, local_pos.y, camera->transform->position.z);
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

    //environment light
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

    //player point light
    {
        glEnable(GL_BLEND);
	    glBlendFunc(GL_ONE, GL_ONE);
	    glDepthFunc(GL_EQUAL);

        glm::vec3 point_light_pos(player_pos.x, player_pos.y, 2.0f);
        glm::vec3 point_light_energy(0.1f, 0.1f, 0.1f);
        // glm::vec3 point_light_energy(0.0f, 0.0f, 0.0f);

        glUseProgram(lit_color_texture_program->program);
        glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 0);
        glUniform3fv(lit_color_texture_program->LIGHT_LOCATION_vec3, 1, glm::value_ptr(point_light_pos));
        glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(point_light_energy));
        glUseProgram(0);

        scene.draw(*camera);

        glDisable(GL_BLEND);
    }
    
    GL_ERRORS();

    draw_overlay(drawable_size);
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

    auto draw_dot = [&](glm::vec3 p, RaycastResult hit)
    {
        float s = 0.1f;
        glm::u8vec4 color = {255, 255, 255, 255};
        if (hit.obj->radar_color == 1)
        {
            color = {255, 0, 0, 255};
        }
        hud.draw(p + glm::vec3(-s, 0, 0), p + glm::vec3(s, 0, 0), color);
        hud.draw(p + glm::vec3(0, -s, 0), p + glm::vec3(0, s, 0), color);
    };

    // glm::vec2 player_pos = local_player_pos();
    std::vector<RaycastResult> hits = radar.last_radar_hits;
    // radar.raycast_surrounding(player_pos, Radar::RADAR_RANGE, Radar::RADAR_RAY_COUNT, hits);

    for (const auto &h : hits)
    {
        glm::vec3 hitP = {h.point.x, h.point.y, 0};
        draw_dot(hitP, h);
        // hud.draw(hitP, {player_pos, 0});
    }
    // hud.draw(player_pos, glm::vec3(0, 0, 0));
}

glm::vec2 PlayMode::local_player_pos()
{
    glm::vec3 p = network_drawables[game.local_player->id]->transform->position;
    return {p.x, p.y};
}
