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
#include "UIOverlay.hpp"

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

struct UIRenderer
{
    // std::string filename;
    FT_Library ft_library;
    FT_Face ft_face;
    hb_font_t *hb_font;
    hb_face_t *hb_face;

    GLuint buffer_for_color_texture_program = 0;
    GLuint buffer = 0;
    GLuint text_tex = 0;
    GLuint program = 0;
    GLuint matrix4fv = 0;
    GLuint color = 0;
    int font_size = 16;
    int width;
    int height;
    UIRenderer(const char *filename, int font_size = 16);

    void init(GLsizei tex_width, GLsizei tex_height, GLuint program, GLuint matrix4fv, GLuint pos, GLuint color, GLuint texcord);

    void upload_pixels(const std::vector<uint8_t> &data, int w, int h, GLint x = 0, GLint y = 0) const
    {
        glBindTexture(GL_TEXTURE_2D, text_tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RED, GL_UNSIGNED_BYTE, data.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        GLint swz[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swz);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glm::vec2 get_size() const
    {
        return glm::vec2(width, height);
    }

    void draw_screen_quad(glm::uvec2 const &drawable_size) const;
    void draw_textured_quad(glm::uvec2 const &drawable_size, UIOverlay::ImageComponent image) const;

    void update_text_data(std::vector<uint8_t> &data, const std::string &text, int clip_w, int clip_h, int world_x, int world_y) const;

    void destroy();
};
