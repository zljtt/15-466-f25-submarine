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

Load<MeshBuffer> meshes(LoadTagDefault, []() -> MeshBuffer const *
                        {
	MeshBuffer const *ret = new MeshBuffer(data_path("prototype.pnct"));
	meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

Load<Scene> prototype_scene(LoadTagDefault, []() -> Scene const *
                            { 
    auto on_drawable = [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name) {
        if (mesh_name == "Player") return;

        Mesh const &mesh = meshes->lookup(mesh_name);
        scene.drawables.emplace_back(transform);
        Scene::Drawable &drawable = scene.drawables.back();

        drawable.pipeline = lit_color_texture_program_pipeline;
        drawable.pipeline.vao = meshes_for_lit_color_texture_program;
        drawable.pipeline.type = mesh.type;
        drawable.pipeline.start = mesh.start;
        drawable.pipeline.count = mesh.count; 
    };
    return new Scene(data_path("prototype.scene"), on_drawable); });

PlayMode::PlayMode(Client &client_) : scene(*prototype_scene), client(client_)
{
    // get pointer to camera for convenience:
    if (scene.cameras.size() != 1)
        throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
    camera = &scene.cameras.front();
}

PlayMode::~PlayMode()
{
}

void create_game_object()
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
					if (game.recv_state_message(c, drawables, scene)) handled_message = true;
				} while (handled_message);
			} catch (std::exception const &e) {
				std::cerr << "[" << c->socket << "] malformed message from server: " << e.what() << std::endl;
				//quit the game:
				throw e;
			}
		} }, 0.0);
    glm::vec2 local_pos = drawables[game.local_player]->transform->position;
    camera->transform->position = glm::vec3(local_pos.x, local_pos.y, 20);
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

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
    // update camera aspect ratio for drawable:
    camera->aspect = float(drawable_size.x) / float(drawable_size.y);

    // set up light type and position for lit_color_texture_program:
    glUseProgram(lit_color_texture_program->program);
    glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
    glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
    glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
    glUseProgram(0);

    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // this is the default depth comparison function, but FYI you can change it.

    scene.draw(*camera);
    GL_ERRORS();
}
