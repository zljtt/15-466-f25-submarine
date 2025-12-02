#pragma once

#include "Mode.hpp"

#include "Connection.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Sound.hpp"
#include "Prefab.hpp"
#include "Load.hpp"
#include "Raycast.hpp"
#include "Radar.hpp"
#include "TextEngine.hpp"
#include "UIRenderer.hpp"
#include "Level.hpp"

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <deque>
#include <array>

struct MenuMode : Mode
{
    MenuMode();
    virtual ~MenuMode();

    virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
    virtual void update(float elapsed) override;
    virtual void draw(glm::uvec2 const &drawable_size) override;

    std::string port;
    std::string ip;
    std::string name;

    int current_text = 0;
    std::vector<UIOverlay> text_overlays;
};