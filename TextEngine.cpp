#include "TextEngine.hpp"
#include "LitColorTextureProgram.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

TextEngine::TextEngine() {}
TextEngine::~TextEngine()
{
    // destroy fonts
    for (auto &f : fonts)
    {
        if (f.hb_font)
            hb_font_destroy(f.hb_font);
        if (f.face)
            FT_Done_Face(f.face);
    }
    if (ft_lib)
        FT_Done_FreeType(ft_lib);
}

bool TextEngine::init()
{
    if (FT_Init_FreeType(&ft_lib))
    {
        std::cerr << "Freetype init failed\n";
        return false;
    }
    return true;
}

int TextEngine::load_font(std::string const &font_path, int pixel_size)
{
    FT_Face face = nullptr;
    if (FT_New_Face(ft_lib, font_path.c_str(), 0, &face))
    {
        std::cerr << "Failed to load font: " << font_path << std::endl;
        return -1;
    }
    FT_Set_Pixel_Sizes(face, 0, pixel_size);
    hb_font_t *hb = hb_ft_font_create(face, NULL);
    FontEntry e;
    e.face = face;
    e.hb_font = hb;
    e.size = pixel_size;
    e.path = font_path;
    fonts.push_back(e);
    return int(fonts.size() - 1);
}

void TextEngine::render_text(int font_id, std::string const &utf8, float x, float y)
{
    if (font_id < 0 || font_id >= (int)fonts.size())
        return;
    auto &f = fonts[font_id];

    // HarfBuzz shaping:
    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, utf8.c_str(), -1, 0, -1);
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, hb_script_from_string("Latn", -1));
    hb_buffer_set_language(buf, hb_language_from_string("en", -1));
    hb_shape(f.hb_font, buf, nullptr, 0);

    unsigned int len = hb_buffer_get_length(buf);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buf, nullptr);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buf, nullptr);

    // get current viewport size so we can build an ortho projection that matches window pixels:
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    float win_w = (float)vp[2];
    float win_h = (float)vp[3];

    // top-left origin ortho (Y down):
    glm::mat4 ortho = glm::ortho(0.0f, win_w, win_h, 0.0f);

    // Use lit shader program
    GLuint program = lit_color_texture_program->program;
    glUseProgram(program);

    // Override lights for text
    glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 3); // unused light type
    glm::vec3 full_brightness(100.0f, 100.0f, 100.0f);
    glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(full_brightness));

    // Set constant vertex color to white
    glVertexAttrib4f(lit_color_texture_program->Color_vec4, 1.0f, 1.0f, 1.0f, 1.0f);

    // set projection uniform
    glUniformMatrix4fv(lit_color_texture_program->CLIP_FROM_OBJECT_mat4, 1, GL_FALSE, glm::value_ptr(ortho));

    // Make sure blending is enabled for alpha mask text, and use typical alpha blending
    GLboolean depth_test_was = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blend_was = glIsEnabled(GL_BLEND);
    if (depth_test_was)
        glDisable(GL_DEPTH_TEST);
    if (!blend_was)
        glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set a constant vertex color (white) for the Color_vec4 attribute so glyphs are white:
    // Using glVertexAttrib4f sets a constant attribute value when no vertex array is bound for that attrib.
    glVertexAttrib4f(lit_color_texture_program->Color_vec4, 0.0f, 1.0f, 0.0f, 1.0f);

    // TEST CODE
    // Optionally also boost LIGHT_ENERGY_vec3
    glm::vec3 light_test(10.0f, 10.0f, 10.0f); // huge light to ignore actual scene lighting
    glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(light_test));

    float pen_x = x;
    float pen_y = y;

    // Ensure single-channel uploads are tightly packed:
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned int i = 0; i < len; ++i)
    {
        uint32_t gid = info[i].codepoint;

        // Load and render glyph to FreeType bitmap
        if (FT_Load_Glyph(f.face, gid, FT_LOAD_RENDER))
            continue;
        FT_GlyphSlot g = f.face->glyph;

        int bmp_w = g->bitmap.width;
        int bmp_h = g->bitmap.rows;
        if (bmp_w == 0 || bmp_h == 0)
        {
            // advance only (space or empty glyph)
            pen_x += pos[i].x_advance / 64.0f;
            pen_y += pos[i].y_advance / 64.0f;
            continue;
        }

        // Create GL texture for this glyph (debug / simple path)
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        // upload as single-channel and swizzle so sampled color = white, alpha = red-channel:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, bmp_w, bmp_h, 0, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // swizzle: R->A, RGB = 1.0 (white). So sampled RGBA = (1,1,1, R)
        GLint swizzleMask[4] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

        // compute quad position in window pixel coordinates:
        float gx = pen_x + g->bitmap_left;
        float gy = pen_y - g->bitmap_top + g->bitmap.rows;

        float w = (float)g->bitmap.width;
        float h = (float)g->bitmap.rows;

        float verts[6][4] = {
            {gx, gy - h, 0.0f, 0.0f},
            {gx + w, gy - h, 1.0f, 0.0f},
            {gx + w, gy, 1.0f, 1.0f},

            {gx, gy - h, 0.0f, 0.0f},
            {gx + w, gy, 1.0f, 1.0f},
            {gx, gy, 0.0f, 1.0f},
        };

        // create VAO/VBO, upload verts:
        GLuint vao = 0, vbo = 0;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

        // Position_vec4 attribute: provide as 2 floats (x,y); shader will receive vec4(x,y,0,1)
        GLuint pos_loc = lit_color_texture_program->Position_vec4;
        glEnableVertexAttribArray(pos_loc);
        glVertexAttribPointer(pos_loc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

        // TexCoord_vec2 attribute: offset by 2 floats:
        GLuint uv_loc = lit_color_texture_program->TexCoord_vec2;
        glEnableVertexAttribArray(uv_loc);
        glVertexAttribPointer(uv_loc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

        // Bind the glyph texture to texture unit 0 (shader will sample TEXTURE0)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        // draw
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // cleanup per-glyph temporary GL objects:
        glDisableVertexAttribArray(pos_loc);
        glDisableVertexAttribArray(uv_loc);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glDeleteTextures(1, &tex);

        // Advance pen using HarfBuzz advance (convert 1/64 to pixels)
        pen_x += pos[i].x_advance / 64.0f;
        pen_y += pos[i].y_advance / 64.0f;
    }

    // restore GL state
    if (!blend_was)
        glDisable(GL_BLEND);
    if (depth_test_was)
        glEnable(GL_DEPTH_TEST);

    hb_buffer_destroy(buf);
}