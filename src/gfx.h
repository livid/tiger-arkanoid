#ifndef GFX_H
#define GFX_H

#include <stdint.h>

#define SCR_W 256
#define SCR_H 224

/* Framebuffer, one 0xAARRGGBB word per pixel (uploaded as BGRA/8888_REV). */
extern uint32_t fb[SCR_H][SCR_W];

void gfx_init(void);
void gfx_clip(int x0, int y0, int x1, int y1);
void gfx_noclip(void);
void gfx_clear(uint32_t c);
void gfx_pixel(int x, int y, uint32_t c);
void gfx_rect(int x, int y, int w, int h, uint32_t c);
void gfx_dim(int x, int y, int w, int h);
void gfx_fade(int level);
void gfx_flash(int amount);

/* Drop shadows: each pixel is darkened at most once between begin() calls. */
void gfx_shadow_begin(void);
void gfx_shadow_px(int x, int y);
void gfx_shadow_rect(int x, int y, int w, int h);

int  gfx_glyph_row(char ch, int row);
int  gfx_text_width(const char *s);
void gfx_text(int x, int y, const char *s, uint32_t c);
void gfx_text_center(int y, const char *s, uint32_t c);
void gfx_text_big(int x, int y, const char *s, int scale,
                  const uint32_t *grad, int ngrad, uint32_t outline);

#endif
