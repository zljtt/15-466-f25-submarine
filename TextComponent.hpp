
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
struct TextStorage
{
    std::vector<std::string> texts;

    void read_from_file(std::string path)
    {
        std::ifstream file(path);
        if (file)
        {
            std::string line;
            while (std::getline(file, line))
            {
                texts.push_back(line);
            }
        }
    };
};

struct TextRenderer
{
    // std::string filename;
    FT_Library ft_library;
    FT_Face ft_face;
    hb_font_t *hb_font;
    hb_face_t *hb_face;

    GLuint buffer_for_color_texture_program = 0;
    GLuint buffer = 0;
    GLuint tex = 0;
    GLuint program = 0;
    GLuint matrix4fv = 0;
    int font_size = 32;
    int width;
    int height;
    TextRenderer(const char *filename, int font_size = 32);

    void init(GLsizei tex_width, GLsizei tex_height, GLuint program, GLuint matrix4fv, GLuint pos, GLuint color, GLuint texcord);

    void upload_pixels(const std::vector<uint8_t> &data, int w, int h, GLint x = 0, GLint y = 0) const
    {
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RED, GL_UNSIGNED_BYTE, data.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        GLint swz[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swz);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void draw_screen_quad(glm::uvec2 const &drawable_size) const;

    void update_text_data(std::vector<uint8_t> &data, const std::string &text, int clip_w, int clip_h, int world_x, int world_y) const;

    void destroy()
    {
        if (buffer)
        {
            glDeleteBuffers(1, &buffer);
            buffer = 0;
        }
        if (buffer_for_color_texture_program)
        {
            glDeleteVertexArrays(1, &buffer_for_color_texture_program);
            buffer_for_color_texture_program = 0;
        }
        if (tex)
        {
            glDeleteTextures(1, &tex);
            tex = 0;
        }
        if (hb_font)
        {
            hb_font_destroy(hb_font);
        }
        if (hb_face)
        {
            hb_face_destroy(hb_face);
        }
        if (ft_face)
        {
            FT_Done_Face(ft_face);
            ft_face = nullptr;
        }
        if (ft_library)
        {
            FT_Done_FreeType(ft_library);
            ft_library = nullptr;
        }
    }
};

struct TextOverlay
{
    struct TextComponent
    {
        std::string text;
        int x;
        int y;
    };

    const TextRenderer *renderer;
    std::unordered_map<std::string, TextComponent> texts;
    std::vector<uint8_t> data;
    int clip_w;
    int clip_h;

    TextOverlay(const TextRenderer *r, int w, int h)
    {
        renderer = r;
        clip_w = w;
        clip_h = h;
    };

    std::string get_string(const std::string id) const
    {
        if (texts.find(id) == texts.end())
            return "";
        else
            return texts.at(id).text;
    }

    const std::vector<uint8_t> &get_data()
    {
        return data;
    };

    void set_text(const std::string id, const std::string text)
    {
        texts[id].text = text;

        data.clear();
        data.assign(int(clip_w * clip_h), 0);

        for (auto t : texts)
        {
            renderer->update_text_data(data, t.second.text, clip_w, clip_h, t.second.x, t.second.y);
        }
        renderer->upload_pixels(data, clip_w, clip_h);
    }

    void move_text(const std::string id, int x, int y)
    {
        if (texts.find(id) != texts.end())
        {
            texts[id].x = x;
            texts[id].y = y;
        }
        data.clear();
        data.assign(int(clip_w * clip_h), 0);

        for (auto t : texts)
        {
            renderer->update_text_data(data, t.second.text, clip_w, clip_h, t.second.x, t.second.y);
        }
        renderer->upload_pixels(data, clip_w, clip_h);
    }

    void add_text(const std::string id, const std::string text, int x, int y)
    {
        texts.emplace(id, TextComponent{text, x, y});
        data.clear();
        data.assign(int(clip_w * clip_h), 0);

        for (auto t : texts)
        {
            renderer->update_text_data(data, t.second.text, clip_w, clip_h, t.second.x, t.second.y);
        }
        renderer->upload_pixels(data, clip_w, clip_h);
    };

    void draw(glm::uvec2 const &drawable_size)
    {
        renderer->draw_screen_quad(drawable_size);
    };

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
        for (auto t : texts)
        {
            renderer->update_text_data(data, t.second.text, clip_w, clip_h, t.second.x, t.second.y);
        }
        renderer->upload_pixels(data, clip_w, clip_h);
    }
};