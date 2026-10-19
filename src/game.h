#ifndef GAME_H
#define GAME_H

enum {
    BTN_LEFT  = 1 << 0,
    BTN_RIGHT = 1 << 1,
    BTN_UP    = 1 << 2,
    BTN_DOWN  = 1 << 3,
    BTN_FIRE  = 1 << 4,
    BTN_START = 1 << 5,
    BTN_PAUSE = 1 << 6,
    BTN_ESC   = 1 << 7
};

typedef struct {
    unsigned held;      /* buttons currently down */
    unsigned pressed;   /* buttons that went down since last update */
    int mouse_moved;    /* nonzero if mouse_x is fresh */
    float mouse_x;      /* mouse x in game pixels */
} GameInput;

void game_init(void);
void game_update(const GameInput *in);
void game_render(void);
void game_focus_lost(void);

#endif
