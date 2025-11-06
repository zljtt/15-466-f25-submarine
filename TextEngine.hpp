#pragma once
#include <string>
#include <vector>
#include <memory>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>
#include <glm/glm.hpp>
#include "GL.hpp"

struct TextEngine
{
public:
    TextEngine();
    ~TextEngine();

    // initialize the underlying libraries (call once)
    bool init();

    // load a font file at a pixel size and return a font id (simple int)
    int load_font(std::string const &font_path, int pixel_size);

    // very small API to draw text to screen in overlay coordinates (implement later)
    // for now this can be a stub that logs or returns success
    void render_text(int font_id, std::string const &utf8, float x, float y);

private:
    // FT + HB state:
    FT_Library ft_lib = nullptr;
    // keep faces / hb_font wrappers in simple arrays/maps in implementation
    struct FontEntry
    {
        FT_Face face;
        hb_font_t *hb_font;
        int size;
        std::string path;
    };
    std::vector<FontEntry> fonts;
};

struct Glyph
{
    GLuint tex; // texture ID for this glyph
    float ax;   // advance (x direction)
    float ay;   // advance (y direction)
    float bw;   // bitmap width
    float bh;   // bitmap height
    float bl;   // bitmap left bearing
    float bt;   // bitmap top bearing
};