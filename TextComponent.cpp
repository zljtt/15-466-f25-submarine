#include "TextComponent.hpp"

#include <memory>
#include <vector>
#include <string>
#include <cmath>
#include <unordered_map>
#include <set>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <iostream>

TextRenderer::TextRenderer(const char *filename, int fsize)
{
    font_size = fsize;
    // copied and modified base on
    // https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-opentype.c
    FT_Error ft_error;
    ft_error = FT_Init_FreeType(&ft_library);
    if ((ft_error))
    {
        std::cout << "free type lib error" << "\n";
        abort();
    }
    ft_error = FT_New_Face(ft_library, filename, 0, &ft_face);
    if ((ft_error))
    {
        std::cout << "create new face error " << "\n";
        abort();
    }
    ft_error = FT_Set_Char_Size(ft_face, fsize * 64, fsize * 64, 0, 0);
    if ((ft_error))
    {
        std::cout << "set char size error " << "\n";
        abort();
    }
    FT_Select_Charmap(ft_face, FT_ENCODING_UNICODE);
    hb_face = hb_ft_face_create(ft_face, NULL);
    hb_font = hb_ft_font_create(ft_face, NULL);
    hb_ft_font_set_load_flags(hb_font, FT_LOAD_DEFAULT | FT_LOAD_TARGET_LIGHT);
}

// copied/modified from code provided by Prof. Jim McCann in Discord
void TextRenderer::init(GLsizei tex_width, GLsizei tex_height, GLuint p, GLuint m, GLuint pos, GLuint color, GLuint texcord)
{
    program = p;
    matrix4fv = m;
    width = tex_width;
    height = tex_height;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, tex_width, tex_height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // use linear interpolation to magnify:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // use trilinear interpolation (linear interpolation of linearly-interpolated mipmaps) to minify:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    GLint swizzle_mask[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    struct Vertex
    {
        glm::vec2 Position;
        glm::u8vec4 Color;
        glm::vec2 TexCoord;
    };

    Vertex quad[4] = {
        {{0.0f, 0.0f}, {0xff, 0xff, 0xff, 0xff}, {0.0f, 0.0f}},
        {{0.0f, 1.0f}, {0xff, 0xff, 0xff, 0xff}, {0.0f, 1.0f}},
        {{1.0f, 0.0f}, {0xff, 0xff, 0xff, 0xff}, {1.0f, 0.0f}},
        {{1.0f, 1.0f}, {0xff, 0xff, 0xff, 0xff}, {1.0f, 1.0f}},
    };
    // create a buffer object to store mesh data in:
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // create a vertex array object that references the buffer:
    glGenVertexArrays(1, &buffer_for_color_texture_program);
    glBindVertexArray(buffer_for_color_texture_program);

    // configure the vertex array object:
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    // set up Position to read from the buffer:
    // see https://registry.khronos.org/OpenGL-Refpages/gl4/html/glVertexAttribPointer.xhtml
    glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, Position));
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer(color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, Color));
    glEnableVertexAttribArray(color);
    glVertexAttribPointer(texcord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, TexCoord));
    glEnableVertexAttribArray(texcord);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // upload attribs to buffer:
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
}

// copied/modified from code provided by Prof. Jim McCann in Discord
void TextRenderer::draw_screen_quad(glm::uvec2 const &drawable_size) const
{
    // draw using the color_texture_program:
    glUseProgram(program);
    // draw with attributes from our buffer, as referenced by the vertex array:
    glBindVertexArray(buffer_for_color_texture_program);
    // draw using texture stored in tex:
    glBindTexture(GL_TEXTURE_2D, tex);
    // this particular shader program multiplies all positions by this matrix: (hmm, old naming style; I should have fixed that)
    //  (just setting it to the identity, so Positions are directly in clip space)

    //  quried ChatGPT for OBJECT_TO_CLIP calcuation
    glm::mat4 proj = glm::ortho(
        0.0f, float(drawable_size.x),
        0.0f, float(drawable_size.y),
        -1.0f, 1.0f);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(12.0f, 12.0f, 0.0f)) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(float(width), float(height), 1.0f));

    glm::mat4 OBJECT_TO_CLIP = proj * model;
    glUniformMatrix4fv(matrix4fv, 1, GL_FALSE, glm::value_ptr(OBJECT_TO_CLIP));

    // draw without depth testing (so will draw atop everything else):
    glDisable(GL_DEPTH_TEST);
    // draw with alpha blending (so transparent parts of the texture look transparent):
    glEnable(GL_BLEND);
    // standard 'over' blending:
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // actually draw:
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // unbind texture, vertex array, program:
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

static void blit(const FT_Bitmap &bm, std::vector<uint8_t> &data, int clip_w, int clip_h, int data_x, int data_y)
{
    int bw = int(bm.width);
    int bh = int(bm.rows);
    const float scale = (bm.pixel_mode == FT_PIXEL_MODE_GRAY && bm.num_grays > 1)
                            ? 255.0f / float(bm.num_grays - 1)
                            : 1.0f;

    // loop bitmap
    for (int row = 0; row < bh; ++row)
    {
        // y0x0 = low left
        int y = data_y + (bh - 1 - row);
        if (y < 0 || y >= clip_h)
        {
            continue;
        }
        const uint8_t *font_row = (bm.pitch >= 0) ? (bm.buffer + row * bm.pitch) : (bm.buffer + (bh - 1 - row) * (-bm.pitch));
        for (int col = 0; col < bw; ++col)
        {
            int x = data_x + col;
            if (x < 0 || x >= clip_w)
            {
                continue;
            }
            uint8_t c = uint8_t(std::lround(font_row[col] * scale));
            if (c > data[y * clip_w + x])
            {
                data[y * clip_w + x] = c;
            }
        }
    }
}

void TextRenderer::update_text_data(std::vector<uint8_t> &data, const std::string &text, int clip_w, int clip_h, int world_x, int world_y) const
{

    hb_buffer_t *hb_buffer = hb_buffer_create();
    // bytes -> codepoints
    hb_buffer_add_utf8(hb_buffer, text.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties(hb_buffer);
    // codepoints -> glyphs (pictures) + positions
    hb_shape(hb_font, hb_buffer, NULL, 0);

    unsigned int glyph_count = 0;
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);
    float pen_x = 0;
    float pen_y = 0;
    for (unsigned int i = 0; i < glyph_count; ++i)
    {
        hb_codepoint_t glyph_index = info[i].codepoint;
        FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT);
        FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);

        FT_GlyphSlot slot = ft_face->glyph;
        FT_Bitmap &bm = slot->bitmap;

        int data_x = world_x + int(std::floor(pen_x + pos[i].x_offset / 64.0f)) + slot->bitmap_left;
        int data_y = world_y + slot->bitmap_top - bm.rows + int(floor(pen_y + pos[i].y_offset / 64.0f));

        blit(bm, data, clip_w, clip_h, data_x, data_y);

        // std::cerr << "gid=" << i
        //           << " bm=" << int(bm.rows) << "x" << int(bm.width)
        //           << " data pos=(" << data_x << "," << data_y << ") -> ("
        //           << (data_x + int(bm.rows) - 1) << "," << (data_y + int(bm.width) - 1) << ")\n";

        pen_x += pos[i].x_advance / 64.0f;
        pen_y += pos[i].y_advance / 64.0f;
    }
    hb_buffer_destroy(hb_buffer);
}