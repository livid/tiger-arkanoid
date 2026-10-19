#include <string.h>
#include "gfx.h"

uint32_t fb[SCR_H][SCR_W];
static unsigned char shmask[SCR_H][SCR_W];
static int cx0 = 0, cy0 = 0, cx1 = SCR_W, cy1 = SCR_H;
static unsigned char glyphs[128][7];

/* 5x7 font: a character followed by its seven rows. */
static const char *font_src[] = {
"0",".###.","#...#","#..##","#.#.#","##..#","#...#",".###.",
"1","..#..",".##..","..#..","..#..","..#..","..#..",".###.",
"2",".###.","#...#","....#","..##.",".#...","#....","#####",
"3",".###.","#...#","....#","..##.","....#","#...#",".###.",
"4","...#.","..##.",".#.#.","#..#.","#####","...#.","...#.",
"5","#####","#....","####.","....#","....#","#...#",".###.",
"6","..##.",".#...","#....","####.","#...#","#...#",".###.",
"7","#####","....#","...#.","..#..",".#...",".#...",".#...",
"8",".###.","#...#","#...#",".###.","#...#","#...#",".###.",
"9",".###.","#...#","#...#",".####","....#","...#.",".##..",
"A",".###.","#...#","#...#","#####","#...#","#...#","#...#",
"B","####.","#...#","#...#","####.","#...#","#...#","####.",
"C",".###.","#...#","#....","#....","#....","#...#",".###.",
"D","####.","#...#","#...#","#...#","#...#","#...#","####.",
"E","#####","#....","#....","####.","#....","#....","#####",
"F","#####","#....","#....","####.","#....","#....","#....",
"G",".###.","#...#","#....","#.###","#...#","#...#",".####",
"H","#...#","#...#","#...#","#####","#...#","#...#","#...#",
"I",".###.","..#..","..#..","..#..","..#..","..#..",".###.",
"J","..###","...#.","...#.","...#.","...#.","#..#.",".##..",
"K","#...#","#..#.","#.#..","##...","#.#..","#..#.","#...#",
"L","#....","#....","#....","#....","#....","#....","#####",
"M","#...#","##.##","#.#.#","#.#.#","#...#","#...#","#...#",
"N","#...#","#...#","##..#","#.#.#","#..##","#...#","#...#",
"O",".###.","#...#","#...#","#...#","#...#","#...#",".###.",
"P","####.","#...#","#...#","####.","#....","#....","#....",
"Q",".###.","#...#","#...#","#...#","#.#.#","#..#.",".##.#",
"R","####.","#...#","#...#","####.","#.#..","#..#.","#...#",
"S",".####","#....","#....",".###.","....#","....#","####.",
"T","#####","..#..","..#..","..#..","..#..","..#..","..#..",
"U","#...#","#...#","#...#","#...#","#...#","#...#",".###.",
"V","#...#","#...#","#...#","#...#","#...#",".#.#.","..#..",
"W","#...#","#...#","#...#","#.#.#","#.#.#","#.#.#",".#.#.",
"X","#...#","#...#",".#.#.","..#..",".#.#.","#...#","#...#",
"Y","#...#","#...#",".#.#.","..#..","..#..","..#..","..#..",
"Z","#####","....#","...#.","..#..",".#...","#....","#####",
"!","..#..","..#..","..#..","..#..","..#..",".....","..#..",
".",".....",".....",".....",".....",".....",".....","..#..",
",",".....",".....",".....",".....",".....","..#..",".#...",
":",".....","..#..",".....",".....",".....","..#..",".....",
"-",".....",".....",".....",".###.",".....",".....",".....",
"?",".###.","#...#","....#","...#.","..#..",".....","..#..",
"'","..#..","..#..",".#...",".....",".....",".....",".....",
"/","....#","....#","...#.","..#..",".#...","#....","#....",
"<","...#.","..#..",".#...","#....",".#...","..#..","...#.",
">",".#...","..#..","...#.","....#","...#.","..#..",".#...",
"=",".....",".....","#####",".....","#####",".....",".....",
"(","...#.","..#..",".#...",".#...",".#...","..#..","...#.",
")",".#...","..#..","...#.","...#.","...#.","..#..",".#...",
0
};

void gfx_init(void)
{
    int i, r, c;
    memset(glyphs, 0, sizeof glyphs);
    for (i = 0; font_src[i]; i += 8) {
        unsigned char ch = (unsigned char)font_src[i][0];
        for (r = 0; r < 7; r++) {
            int bits = 0;
            for (c = 0; c < 5; c++)
                if (font_src[i + 1 + r][c] == '#') bits |= 16 >> c;
            glyphs[ch][r] = (unsigned char)bits;
        }
    }
}

void gfx_clip(int x0, int y0, int x1, int y1)
{
    cx0 = x0 < 0 ? 0 : x0;
    cy0 = y0 < 0 ? 0 : y0;
    cx1 = x1 > SCR_W ? SCR_W : x1;
    cy1 = y1 > SCR_H ? SCR_H : y1;
}

void gfx_noclip(void) { gfx_clip(0, 0, SCR_W, SCR_H); }

void gfx_clear(uint32_t c)
{
    int i;
    uint32_t *p = &fb[0][0];
    c |= 0xFF000000u;
    for (i = 0; i < SCR_W * SCR_H; i++) p[i] = c;
}

void gfx_pixel(int x, int y, uint32_t c)
{
    if (x >= cx0 && x < cx1 && y >= cy0 && y < cy1)
        fb[y][x] = c | 0xFF000000u;
}

void gfx_rect(int x, int y, int w, int h, uint32_t c)
{
    int i, j;
    int x0 = x < cx0 ? cx0 : x, x1 = x + w > cx1 ? cx1 : x + w;
    int y0 = y < cy0 ? cy0 : y, y1 = y + h > cy1 ? cy1 : y + h;
    c |= 0xFF000000u;
    for (j = y0; j < y1; j++)
        for (i = x0; i < x1; i++) fb[j][i] = c;
}

static uint32_t darken(uint32_t c) { return ((c >> 1) & 0x7F7F7Fu) | 0xFF000000u; }

void gfx_dim(int x, int y, int w, int h)
{
    int i, j;
    int x0 = x < cx0 ? cx0 : x, x1 = x + w > cx1 ? cx1 : x + w;
    int y0 = y < cy0 ? cy0 : y, y1 = y + h > cy1 ? cy1 : y + h;
    for (j = y0; j < y1; j++)
        for (i = x0; i < x1; i++) fb[j][i] = darken(fb[j][i]);
}

void gfx_shadow_begin(void) { memset(shmask, 0, sizeof shmask); }

void gfx_shadow_px(int x, int y)
{
    if (x >= cx0 && x < cx1 && y >= cy0 && y < cy1 && !shmask[y][x]) {
        shmask[y][x] = 1;
        fb[y][x] = darken(fb[y][x]);
    }
}

void gfx_shadow_rect(int x, int y, int w, int h)
{
    int i, j;
    for (j = y; j < y + h; j++)
        for (i = x; i < x + w; i++) gfx_shadow_px(i, j);
}

int gfx_glyph_row(char ch, int row)
{
    unsigned char u = (unsigned char)ch;
    if (u >= 'a' && u <= 'z') u -= 32;
    if (u >= 128 || row < 0 || row > 6) return 0;
    return glyphs[u][row];
}

int gfx_text_width(const char *s)
{
    int n = (int)strlen(s);
    return n ? n * 6 - 1 : 0;
}

static void draw_glyphs(int x, int y, const char *s, uint32_t c)
{
    int r, b;
    for (; *s; s++, x += 6)
        for (r = 0; r < 7; r++) {
            int bits = gfx_glyph_row(*s, r);
            for (b = 0; b < 5; b++)
                if (bits & (16 >> b)) gfx_pixel(x + b, y + r, c);
        }
}

void gfx_text(int x, int y, const char *s, uint32_t c)
{
    draw_glyphs(x + 1, y + 1, s, 0x000000);
    draw_glyphs(x, y, s, c);
}

void gfx_text_center(int y, const char *s, uint32_t c)
{
    gfx_text((SCR_W - gfx_text_width(s)) / 2, y, s, c);
}

void gfx_text_big(int x, int y, const char *s, int scale,
                  const uint32_t *grad, int ngrad, uint32_t outline)
{
    int pass, r, b, sy, sx;
    const char *p;
    for (pass = 0; pass < 3; pass++) {
        int cx = x;
        for (p = s; *p; p++, cx += 6 * scale) {
            for (r = 0; r < 7; r++) {
                int bits = gfx_glyph_row(*p, r);
                for (b = 0; b < 5; b++) {
                    int px, py;
                    if (!(bits & (16 >> b))) continue;
                    px = cx + b * scale;
                    py = y + r * scale;
                    if (pass == 0)
                        gfx_rect(px + 1, py + 2, scale + 2, scale + 2, 0x000000);
                    else if (pass == 1)
                        gfx_rect(px - 1, py - 1, scale + 2, scale + 2, outline);
                    else
                        for (sy = 0; sy < scale; sy++) {
                            uint32_t c = grad[((r * scale + sy) * ngrad) / (7 * scale)];
                            for (sx = 0; sx < scale; sx++) gfx_pixel(px + sx, py + sy, c);
                        }
                }
            }
        }
    }
}

/* Scale the whole screen toward black (level 0..256). */
void gfx_fade(int level)
{
    int i;
    uint32_t *p = &fb[0][0];
    if (level >= 256) return;
    if (level < 0) level = 0;
    for (i = 0; i < SCR_W * SCR_H; i++) {
        uint32_t c = p[i];
        uint32_t r = ((c >> 16) & 255) * level >> 8, g = ((c >> 8) & 255) * level >> 8, b = (c & 255) * level >> 8;
        p[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
    }
}

/* Blend the whole screen toward white (amount 0..256). */
void gfx_flash(int amount)
{
    int i;
    uint32_t *p = &fb[0][0];
    if (amount <= 0) return;
    if (amount > 256) amount = 256;
    for (i = 0; i < SCR_W * SCR_H; i++) {
        uint32_t c = p[i];
        uint32_t r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
        r += (255 - r) * amount >> 8;
        g += (255 - g) * amount >> 8;
        b += (255 - b) * amount >> 8;
        p[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
    }
}
