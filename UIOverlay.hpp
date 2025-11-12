#pragma once

#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <fstream>
#include <string>
#include <hb.h>
#include <hb-ft.h>
#include <vector>
#include <deque>
#include <array>

#include "GL.hpp"

#include "ColorTextureProgram.hpp"

struct UIRenderer;

struct UIOverlay
{
    enum Alignment
    {
        Top,
        TopRight,
        TopLeft,
        Bottom,
        BottomRight,
        BottomLeft
    };

    struct TextComponent
    {
        std::string text;
        glm::vec2 pos;
    };

    struct ImageComponent
    {
        GLuint tex = 0;
        glm::vec2 pos = {0, 0};
        glm::vec2 size = {0, 0};
        glm::vec4 uv = {0, 0, 1, 1};
        glm::vec4 color = {1, 1, 1, 1};
        ImageComponent() {};
        ImageComponent(GLuint t, glm::vec2 p, glm::vec2 s) : tex(t), pos(p), size(s) {};
    };

    const UIRenderer *renderer;
    std::unordered_map<std::string, TextComponent> texts;
    std::unordered_map<std::string, ImageComponent> images;

    std::vector<uint8_t> data;

    UIOverlay(const UIRenderer *r)
    {
        renderer = r;
    };

    std::string get_string(const std::string id) const
    {
        if (texts.find(id) == texts.end())
            return "";
        else
            return texts.at(id).text;
    }

    void update_data();

    const std::vector<uint8_t> &get_data()
    {
        return data;
    };

    void update_text(const std::string id, const std::string text, glm::vec2 relative_pos, Alignment anchor = BottomLeft)
    {
        texts[id].text = text;
        texts[id].pos = get_anchored_pos(anchor, relative_pos);
        update_data();
    }

    void update_image(const std::string id, GLuint tex, glm::vec2 size, glm::vec2 relative_pos, Alignment anchor = BottomLeft)
    {
        images[id].tex = tex;
        images[id].size = size;
        images[id].pos = get_anchored_pos(anchor, relative_pos);
    }

    void update_image(const std::string id, ImageComponent image)
    {
        images[id] = image;
    }

    void draw(glm::uvec2 const &drawable_size);

    template <typename Pred>
    void remove_texts(Pred pred)
    {
        for (auto it = texts.begin(); it != texts.end();)
        {
            if (pred(it->first))
            {
                it = texts.erase(it);
            }
            else
            {
                ++it;
            }
        }
        update_data();
    }

    template <typename Pred>
    void remove_images(Pred pred)
    {
        for (auto it = images.begin(); it != images.end();)
        {
            if (pred(it->first))
            {
                it = images.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    glm::vec2 get_anchored_pos(Alignment alignment, glm::vec2 relative);
};