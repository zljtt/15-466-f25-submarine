#include "MenuMode.hpp"
#include "Registry.hpp"
#include "PlayMode.hpp"
#include "UIOverlay.hpp"

#include <iostream>

MenuMode::MenuMode()
{
    text_overlays.emplace_back(renderer_gui.value);
    text_overlays.emplace_back(renderer_large.value);
    text_overlays[1].update_text("title", "Submarine", glm::vec2(-70, 500), UIOverlay::Bottom);
    text_overlays[0].update_text("prompt1", "Enter the server address (ip:port):", glm::vec2(-120, 420), UIOverlay::Bottom);
    text_overlays[0].update_text("prompt2", "Press ENTER to proceed", glm::vec2(-120, 360), UIOverlay::Bottom);

    text_overlays[0].update_text("textfield_ip", "ip: __________", glm::vec2(-120, 300), UIOverlay::Bottom);
    text_overlays[0].update_text("textfield_port", "port: __________", glm::vec2(-120, 240), UIOverlay::Bottom);
    text_overlays[0].update_text("textfield_name", "your name: __________", glm::vec2(-120, 180), UIOverlay::Bottom);

    SDL_StartTextInput(Mode::window);
}

MenuMode::~MenuMode()
{
}

bool MenuMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{

    if (evt.type == SDL_EVENT_TEXT_INPUT)
    {
        if (current_text == 0)
        {
            ip += evt.text.text;
            text_overlays[0].update_text("textfield_ip", "ip: " + ip, glm::vec2(-120, 300), UIOverlay::Bottom);
        }
        else if (current_text == 1)
        {
            port += evt.text.text;
            text_overlays[0].update_text("textfield_port", "port: " + port, glm::vec2(-120, 240), UIOverlay::Bottom);
        }
        else
        {
            name += evt.text.text;
            text_overlays[0].update_text("textfield_name", "your name: " + name, glm::vec2(-120, 180), UIOverlay::Bottom);
        }
    }
    else if (evt.type == SDL_EVENT_KEY_DOWN)
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
        std::string &current = current_text == 0 ? ip : (current_text == 1 ? port : name);
        if (evt.key.key == SDLK_BACKSPACE && !current.empty())
        {
            if (current_text == 0)
            {
                ip.pop_back();
                text_overlays[0].update_text("textfield_ip", "ip: " + ip, glm::vec2(-120, 300), UIOverlay::Bottom);
            }
            else if (current_text == 1)
            {
                port.pop_back();
                text_overlays[0].update_text("textfield_port", "port: " + port, glm::vec2(-120, 240), UIOverlay::Bottom);
            }
            else
            {
                name.pop_back();
                text_overlays[0].update_text("textfield_name", "your name: " + name, glm::vec2(-120, 180), UIOverlay::Bottom);
            }
        }
        else if (evt.key.key == SDLK_RETURN || evt.key.key == SDLK_KP_ENTER)
        {
            if (current_text == 0)
            {
                current_text = 1;
                text_overlays[0].update_text("textfield_port", "port: ", glm::vec2(-120, 240), UIOverlay::Bottom);
            }
            else if (current_text == 1)
            {
                current_text = 2;
                text_overlays[0].update_text("textfield_name", "your name: ", glm::vec2(-120, 180), UIOverlay::Bottom);
            }
            else if (current_text == 2)
            {
                if (!port.empty() && !ip.empty() && !name.empty())
                {
                    std::cout << ip << "\n";
                    std::cout << port << "\n";
                    std::cout << name << "\n";

                    text_overlays[0].clear();
                    text_overlays[1].clear();

                    SDL_StopTextInput(Mode::window);
                    Mode::set_current(std::make_shared<PlayMode>(ip, port, name));
                }
            }
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

void MenuMode::update(float elapsed)
{
}

void MenuMode::draw(glm::uvec2 const &drawable_size)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);

    for (auto text_overlay : text_overlays)
    {
        text_overlay.draw(drawable_size);
    }
}
