#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "Prefab.hpp"

#include "LitColorTextureProgram.hpp"

#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <array>

#include "load_save_png.hpp"

static GLuint load_texture_from_png(const std::string &path) {
    glm::uvec2 size;
    std::vector<glm::u8vec4> data;
    load_png(path, &size, &data, UpperLeftOrigin);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 size.x, size.y, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data.data());

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}



Load<GLuint> tex_obstacle(LoadTagDefault, []() -> GLuint const * {
    // GLuint t = load_texture_from_png(data_path("obstacle_basecolor.png"));
    GLuint t = load_texture_from_png(data_path("rock_basecolor.png"));

    return new GLuint(t);
});

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

        drawable.pipeline.textures[0].target = GL_TEXTURE_2D;
        drawable.pipeline.textures[0].texture = *tex_obstacle;
    };
    return new Scene(data_path("prototype.scene"), on_drawable); });

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
    update_spotlight(elapsed);
    radar.update(elapsed);
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
}

void PlayMode::update_camera(float elapsed)
{
    glm::vec2 local_pos = network_drawables[local_player->id]->transform->position;
    camera->transform->position = glm::vec3(local_pos.x, local_pos.y, camera->transform->position.z);
}

void PlayMode::update_spotlight(float elapsed)
{
    // calculate spotlight direction
    glm::vec2 cur_player_pos = local_player_pos();
    glm::vec2 velocity = cur_player_pos - prev_player_pos;
    prev_player_pos = cur_player_pos;

    if (velocity.x > 1e-3f)
        spot_light_dir_x = 1.0f;
    else if (velocity.x < -1e-3f)
        spot_light_dir_x = -1.0f;
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
        glm::vec3 spot_light_dir(spot_light_dir_x, 0.0f, 0.0f);
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

    radar.render(hud);

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
    if (at != size)
    {
        throw std::runtime_error("Trailing data in state message.");
    }

    // delete message from buffer:
    recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 4 + size);

    return true;
}
