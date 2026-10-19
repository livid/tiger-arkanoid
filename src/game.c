/* Arkanoid - SNES-style remake. Game logic and software rendering. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <CoreFoundation/CoreFoundation.h>
#include "game.h"
#include "gfx.h"
#include "sound.h"

#define FIELD_L 24
#define FIELD_R 232
#define FIELD_T 24
#define BW 16
#define BH 8
#define COLS 13
#define ROWS 18
#define BRICK_Y0 FIELD_T
#define PAD_Y 204
#define BALL_S 5
#define SHADOW 4
#define MAXBALLS 8
#define MAXBULLETS 8
#define MAXENEMIES 3
#define MAXEXPL 32
#define MAXSHOTS 4
#define NROUNDS 32
#define ROUND_DOH (NROUNDS + 1)
#define MAX_SPEED 3.6f
#define STORY_END 1780

#define DOH_W 48
#define DOH_H 64
#define DOH_X (128 - DOH_W / 2)
#define DOH_Y 36
#define DOH_HP 16
#define DOH_EYE 1u

/* Guardian: mini-boss that patrols below the bricks every 5th round */
#define MB_W 28
#define MB_H 18
#define MB_MAXY 156

#define C_WHITE 0xFFFFFF
#define C_RED   0xF83838
#define C_YELL  0xF8D838
#define C_CYAN  0x58D8F8

enum { BR_NONE, BR_WHITE, BR_ORANGE, BR_CYAN, BR_GREEN, BR_RED, BR_BLUE,
       BR_PINK, BR_YELLOW, BR_SILVER, BR_GOLD };
enum { ST_TITLE, ST_INTRO, ST_PLAY, ST_DIE, ST_WARP, ST_CLEAR, ST_GAMEOVER,
       ST_CONTINUE, ST_DOHDEATH, ST_ENDING, ST_STORY };
enum { PW_NONE, PW_CATCH, PW_LASER, PW_ENLARGE };
enum { CAP_S, CAP_C, CAP_L, CAP_E, CAP_D, CAP_B, CAP_P, NCAPS };

/* ------------------------------------------------------------------ */
/* Round layouts. W O C G R B P Y = colours, S = silver, X = gold.    */

static const char *levels[NROUNDS][ROWS] = {
    { "", "", "",
      "SSSSSSSSSSSSS",
      "RRRRRRRRRRRRR",
      "YYYYYYYYYYYYY",
      "BBBBBBBBBBBBB",
      "PPPPPPPPPPPPP",
      "GGGGGGGGGGGGG" },
    { "", "",
      "W............",
      "WO...........",
      "WOC..........",
      "WOCG.........",
      "WOCGR........",
      "WOCGRB.......",
      "WOCGRBP......",
      "WOCGRBPY.....",
      "WOCGRBPYW....",
      "WOCGRBPYWO...",
      "WOCGRBPYWOC..",
      "WOCGRBPYWOCG.",
      "SSSSSSSSSSSSR" },
    { "", "",
      "GGGGGGGGGGGGG",
      "",
      "WWWXXXXXXXXXX",
      "",
      "RRRRRRRRRRRRR",
      "",
      "XXXXXXXXXXWWW",
      "",
      "PPPPPPPPPPPPP",
      "",
      "BBBXXXXXXXXXX",
      "",
      "CCCCCCCCCCCCC" },
    { "", "",
      "...C.....C...",
      "....C...C....",
      "...CCCCCCC...",
      "..CCRCCCRCC..",
      ".CCCCCCCCCCC.",
      ".C.CCCCCCC.C.",
      ".C.C.....C.C.",
      "....CC.CC....",
      "",
      "SSS.......SSS",
      "YYY.......YYY" },
    { "", "",
      "......Y......",
      ".....YOY.....",
      "....YOROY....",
      "...YORPROY...",
      "..YORPBPROY..",
      ".YORPBCBPROY.",
      "YORPBCWCBPROY",
      ".YORPBCBPROY.",
      "..YORPBPROY..",
      "...YORPROY...",
      "....YOROY....",
      ".....YOY.....",
      "......Y......" },
    { "", "",
      "XSSSSSSSSSSSX",
      "S...........S",
      "S.BBBBBBBBB.S",
      "S.B.......B.S",
      "S.B.RRRRR.B.S",
      "S.B.RYYYR.B.S",
      "S.B.RRRRR.B.S",
      "S.B.......B.S",
      "S.BBBBBBBBB.S",
      "S...........S",
      "XSSSS...SSSSX" },
    { "", "",
      "S.S.S.S.S.S.S",
      "W.O.C.G.R.B.P",
      "W.O.C.G.R.B.P",
      "W.O.C.G.R.B.P",
      "W.O.C.G.R.B.P",
      "Y.W.O.C.G.R.B",
      "Y.W.O.C.G.R.B",
      "Y.W.O.C.G.R.B",
      "Y.W.O.C.G.R.B" },
    { "", "", "",
      "C.C.C.C.C.C.C",
      ".P.P.P.P.P.P.",
      "C.C.C.C.C.C.C",
      ".P.P.P.P.P.P.",
      "S.S.S.S.S.S.S",
      ".Y.Y.Y.Y.Y.Y.",
      "G.G.G.G.G.G.G",
      ".R.R.R.R.R.R.",
      "B.B.B.B.B.B.B",
      ".X.X.X.X.X.X." },
    { "", "",
      "S.S.S...S.S.S",
      "SSSSS...SSSSS",
      "SRRRS...SBBBS",
      "SRYRS...SBCBS",
      "SRRRS...SBBBS",
      "SSSSS...SSSSS",
      "",
      "..XXXXXXXXX..",
      "..PPPPPPPPP..",
      "..WWWWWWWWW..",
      "OOOOOOOOOOOOO" },
    { "", "",
      "..RRR...RRR..",
      ".RPPPR.RPPPR.",
      "RPPWPPRPPPPPR",
      "RPWPPPPPPPPPR",
      "RPPPPPPPPPPPR",
      ".RPPPPPPPPPR.",
      "..RPPPPPPPR..",
      "...RPPPPPR...",
      "....RPPPR....",
      ".....RPR.....",
      "......R......",
      "",
      "XXX.......XXX" },
    { "", "",
      "...RRRRRRR...",
      "..ROOOOOOOR..",
      ".ROYYYYYYYOR.",
      "ROYGGGGGGGYOR",
      "ROYG.....GYOR",
      "ROYG.CCC.GYOR",
      "ROYG.BBB.GYOR",
      "ROYG.....GYOR",
      "SSSS.....SSSS" },
    { "", "",
      "SSSSSSSSSSSSS",
      "X.....X.....X",
      "X.CCC.X.PPP.X",
      "X.CBC.X.PYP.X",
      "X.CCC.X.PPP.X",
      "X.....X.....X",
      "XSSSSSXSSSSSX",
      "",
      "OOOOOOOOOOOOO",
      "WWWWWWWWWWWWW" },
    /* 13 pyramid */
    { "", "", "",
      "......W......",
      ".....WOW.....",
      "....WOCOW....",
      "...WOCGCOW...",
      "..WOCGRGCOW..",
      ".WOCGRBRGCOW.",
      "WOCGRBPBRGCOW",
      "SSSSSSSSSSSSS" },
    /* 14 mushroom */
    { "", "",
      "....RRRRR....",
      "..RRWWRRRWR..",
      ".RRWWWRRRWWR.",
      ".RRRWRRRRWWR.",
      "RRRRRRRRRRRRR",
      "RWRRRRRRRRRWR",
      "..YYYYYYYYY..",
      "....YXYXY....",
      "....YYYYY....",
      "....YYYYY....",
      "....OOOOO...." },
    /* 15 skull */
    { "", "",
      "...WWWWWWW...",
      "..WWWWWWWWW..",
      ".WWWWWWWWWWW.",
      ".WW...W...WW.",
      ".WW.R.W.R.WW.",
      ".WWWWW.WWWWW.",
      "..WWWW.WWWW..",
      "...WWWWWWW...",
      "...W.W.W.W...",
      "...SSSSSSS..." },
    /* 16 rocket */
    { "", "",
      "......R......",
      ".....RRR.....",
      ".....WWW.....",
      "....WWBWW....",
      "....WBCBW....",
      "....WWBWW....",
      "....WWWWW....",
      "....WWWWW....",
      "...RWWWWWR...",
      "..RRWWWWWRR..",
      "..RR.YYY.RR..",
      ".....OYO.....",
      "......O......" },
    /* 17 gold maze */
    { "", "",
      "CCCCCCCCCCCCC",
      "XXXXXX.XXXXXX",
      "PPPPPPPPPPPPP",
      "X.XXXXXXXXXXX",
      "GGGGGGGGGGGGG",
      "XXXXXXXXXXX.X",
      "YYYYYYYYYYYYY",
      "XXXXXX.XXXXXX",
      "BBBBBBBBBBBBB" },
    /* 18 silver checker */
    { "", "", "",
      "SWSWSWSWSWSWS",
      "WSWSWSWSWSWSW",
      "OCOCOCOCOCOCO",
      "COCOCOCOCOCOC",
      "SGSGSGSGSGSGS",
      "GSGSGSGSGSGSG",
      "RBRBRBRBRBRBR",
      "BRBRBRBRBRBRB" },
    /* 19 smiley */
    { "", "",
      "....YYYYY....",
      "..YYYYYYYYY..",
      ".YYYYYYYYYYY.",
      ".YYXYYYYYXYY.",
      "YYYXYYYYYXYYY",
      "YYYYYYYYYYYYY",
      "YYRYYYYYYYRYY",
      "YYYRYYYYYRYYY",
      ".YYYRRRRRYYY.",
      "..YYYYYYYYY..",
      "....YYYYY...." },
    /* 20 ghost */
    { "", "",
      "....PPPPP....",
      "..PPPPPPPPP..",
      ".PPPPPPPPPPP.",
      ".PWWPPPPWWPP.",
      "PPWBPPPPWBPPP",
      "PPWWPPPPWWPPP",
      "PPPPPPPPPPPPP",
      "PPPPPPPPPPPPP",
      "PPPPPPPPPPPPP",
      "PP.PPP.PPP.PP",
      "P...P...P...P" },
    /* 21 zigzag */
    { "", "",
      "R...R...R...R",
      ".O.O.O.O.O.O.",
      "..Y...Y...Y..",
      "S...S...S...S",
      ".G.G.G.G.G.G.",
      "..C...C...C..",
      "X...X...X...X",
      ".B.B.B.B.B.B.",
      "..P...P...P.." },
    /* 22 columns */
    { "", "",
      "BBBBBBBBBBBBB",
      "XX.XX.XX.XX.X",
      "RR.OO.YY.GG.C",
      "RR.OO.YY.GG.C",
      "RR.OO.YY.GG.C",
      "RR.OO.YY.GG.C",
      "RR.OO.YY.GG.C",
      "RR.OO.YY.GG.C",
      "SS.SS.SS.SS.S" },
    /* 23 target */
    { "", "",
      "RRRRRRRRRRRRR",
      "R...........R",
      "R.OOOOOOOOO.R",
      "R.O.......O.R",
      "R.O.YYYYY.O.R",
      "R.O.Y...Y.O.R",
      "R.O.Y.X.Y.O.R",
      "R.O.Y...Y.O.R",
      "R.O.YYYYY.O.R",
      "R.O.......O.R",
      "R.OOOOOOOOO.R",
      "R...........R",
      "RRRRRRRRRRRRR" },
    /* 24 hourglass */
    { "", "",
      "SSSSSSSSSSSSS",
      ".CCCCCCCCCCC.",
      "..CCCCCCCCC..",
      "...BBBBBBB...",
      "....BBBBB....",
      ".....PPP.....",
      "......X......",
      ".....PPP.....",
      "....GGGGG....",
      "...GGGGGGG...",
      "..YYYYYYYYY..",
      ".YYYYYYYYYYY.",
      "SSSSSSSSSSSSS" },
    /* 25 cat */
    { "", "",
      ".O.........O.",
      ".OO.......OO.",
      ".OOO.....OOO.",
      ".OOOOOOOOOOO.",
      "OOOOOOOOOOOOO",
      "OOGGOOOOOGGOO",
      "OOGXOOOOOGXOO",
      "OOOOOOPOOOOOO",
      "WOOOOPPPOOOOW",
      "OOOOOOOOOOOOO",
      ".OOOOOOOOOOO.",
      "..OOOOOOOOO.." },
    /* 26 tree */
    { "", "",
      "......G......",
      ".....GGG.....",
      "....GGRGG....",
      "...GGGGGGG...",
      "..GGGYGGGGG..",
      ".GGRGGGGGRGG.",
      "GGGGGGBGGGGGG",
      "......O......",
      "......O......",
      ".....SSS.....",
      "XXXX.....XXXX" },
    /* 27 twin stairs */
    { "", "",
      "B...........B",
      "BP.........PB",
      "BPC.......CPB",
      "BPCG.....GCPB",
      "BPCGY...YGCPB",
      "BPCGYR.RYGCPB",
      "BPCGYRSRYGCPB",
      "SSSSSS.SSSSSS" },
    /* 28 crossed blades */
    { "", "",
      "RR.........RR",
      "RRR.......RRR",
      ".RRR.....RRR.",
      "..RRR...RRR..",
      "...RRRXRRR...",
      "....BXXXB....",
      "...BBBXBBB...",
      "..BBB...BBB..",
      ".BBB.....BBB.",
      "BBB.......BBB",
      "BB.........BB" },
    /* 29 labyrinth */
    { "", "",
      "XXXXXXXXXXXXX",
      "XCCCCCCCCCCCX",
      "XCXXXXWXXXXCX",
      "XCXPPPPPPPXCX",
      "XCXPXXXXXPXCX",
      "XCXPXYYYXPXCX",
      "XCXPXYYYXPXCX",
      "XCXPXXWXXPXCX",
      "XCXPPPPPPPXCX",
      "XCXXXXXXXXXCX",
      "XCCCCCCCCCCCX",
      "XXXXXXWXXXXXX" },
    /* 30 trophy */
    { "", "",
      ".YYYYYYYYYYY.",
      "YY.YWYYYYY.YY",
      "Y..YWYYYYY..Y",
      "YY.YWYYYYY.YY",
      ".YYYWYYYYYYY.",
      "...YWYYYYY...",
      "....YYYYY....",
      ".....YYY.....",
      "......Y......",
      ".....OOO.....",
      "....OOOOO....",
      "...SSSSSSS..." },
    /* 31 rampart */
    { "", "",
      "X.X.X.X.X.X.X",
      "SSSSSSSSSSSSS",
      "BBBBBBBBBBBBB",
      "RRRRRRRRRRRRR",
      "SSSSSSSSSSSSS",
      "PPPPPPPPPPPPP",
      "YYYYYYYYYYYYY",
      ".X.X.X.X.X.X." },
    /* 32 gate of doh */
    { "", "",
      "XSXSXSXSXSXSX",
      "SRRRRRRRRRRRS",
      "XRWWWWWWWWWRX",
      "SRWBBBBBBBWRS",
      "XRWBPPPPPBWRX",
      "SRWBPYYYPBWRS",
      "XRWBPPPPPBWRX",
      "SRWBBBBBBBWRS",
      "XRWWWWWWWWWRX",
      "SRRRRRRRRRRRS",
      "XSXS.....SXSX" },
};

/* ------------------------------------------------------------------ */

typedef struct { unsigned char type, hits, flash; } Brick;
typedef struct { int active; float x, y, dx, dy; int stuck; float off; } Ball;
typedef struct { int active, type; float x, y; int anim; } Capsule;
typedef struct { int active; float x, y; } Bullet;
typedef struct { int active, type; float x, y, dx, dy; int tmr, anim; } Enemy;
typedef struct { int active; float x, y; int t; float seed; } Expl;
typedef struct { int active; float x, y, dx, dy; int anim; } Shot;

static Brick bricks[ROWS][COLS];
static int bricks_left;
static Ball balls[MAXBALLS];
static float ball_speed;
static int catch_timer, serve_timer;
static float pad_x, pad_w, pad_w_target;
static int power, laser_cool, pad_anim;
static Capsule capsule;
static int last_capsule = -1;
static Bullet bullets[MAXBULLETS];
static Enemy enemies[MAXENEMIES];
static int enemy_timer;
static int gate_open[2], gate_dir[2], gate_hold[2];
static const int gate_x[2] = { 56, 176 };
static Expl expls[MAXEXPL];
static int warp_open;
static int doh_hits, doh_flash, doh_mouth, doh_fire_timer;
static Shot shots[MAXSHOTS];
static uint32_t doh_img[2][DOH_H][DOH_W];
static int mb_hp, mb_hp_max, mb_flash, mb_fire_timer, mb_anim;
static float mb_x, mb_y, mb_y0, mb_dx;

static int state, st_timer, paused, intro_round, demo;
static int round_no, sel_round = 1, lives;
static long score, hiscore = 50000, next_extend;
static unsigned frame;
static uint32_t bgbuf[SCR_H][SCR_W];
static int g_shadow;
static int attract;

static void story_start(void);
static void story_update(void);

static unsigned rng_state = 0x2468ACE;
static unsigned rnd32(void) { rng_state = rng_state * 1103515245u + 12345u; return rng_state >> 8; }
static int rnd(int n) { return (int)(rnd32() % (unsigned)n); }
static float frnd(void) { return (float)(rnd32() & 0xFFFF) / 65535.0f; }

static const int brick_score[11] = { 0, 50, 60, 70, 80, 90, 100, 110, 120, 0, 0 };
/* base, highlight, shade */
static const uint32_t brick_cols[11][3] = {
    { 0, 0, 0 },
    { 0xE0E0E0, 0xFFFFFF, 0x9898A8 },
    { 0xF89030, 0xFFC878, 0xB05010 },
    { 0x30D0F0, 0xA0F8FF, 0x1080A8 },
    { 0x30C830, 0x98FF88, 0x108018 },
    { 0xE02828, 0xFF8878, 0x901010 },
    { 0x2850E8, 0x80A8FF, 0x1828A0 },
    { 0xE838C8, 0xFF98F0, 0x900880 },
    { 0xF0D828, 0xFFF8A0, 0xA88800 },
    { 0, 0, 0 }, { 0, 0, 0 }
};
static const uint32_t silver_rows[7] = { 0xF8F8FF, 0xD8D8E8, 0xC0C0D0, 0xB0B0C0, 0x9898A8, 0x808090, 0x585868 };
static const uint32_t gold_rows[7]   = { 0xFFF0A0, 0xF8D060, 0xE8B838, 0xD8A020, 0xB88810, 0x987008, 0x684800 };

/* ------------------------------------------------------------------ */

static void plot(int x, int y, uint32_t c)
{
    if (g_shadow) gfx_shadow_px(x + SHADOW, y + SHADOW);
    else gfx_pixel(x, y, c);
}

static int iround(float f) { return (int)floorf(f + 0.5f); }

static void hiscore_load(void)
{
    CFPropertyListRef v = CFPreferencesCopyAppValue(CFSTR("HighScore"), kCFPreferencesCurrentApplication);
    if (v) {
        if (CFGetTypeID(v) == CFNumberGetTypeID()) {
            long n = 0;
            CFNumberGetValue((CFNumberRef)v, kCFNumberLongType, &n);
            if (n > hiscore) hiscore = n;
        }
        CFRelease(v);
    }
}

static void hiscore_save(void)
{
    long n = hiscore;
    CFNumberRef num = CFNumberCreate(NULL, kCFNumberLongType, &n);
    CFPreferencesSetAppValue(CFSTR("HighScore"), num, kCFPreferencesCurrentApplication);
    CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
    CFRelease(num);
}

static void add_score(long pts)
{
    score += pts;
    if (demo) return;
    if (score >= next_extend) {
        lives++;
        next_extend += 60000;
        snd_play(SND_LIFE);
    }
    if (score > hiscore) hiscore = score;
}

/* ------------------------------------------------------------------ */
/* Background                                                          */

static const uint32_t bg_pals[12][4] = {
    { 0x060C28, 0x0C1A48, 0x18306C, 0x284890 },  /* 0 blue */
    { 0x041808, 0x0A3018, 0x144828, 0x22663C },  /* 1 green */
    { 0x200606, 0x380C0C, 0x541818, 0x742C2C },  /* 2 red */
    { 0x180620, 0x2C0E3A, 0x441C58, 0x602C78 },  /* 3 purple */
    { 0x061A20, 0x0C3038, 0x164856, 0x246676 },  /* 4 teal */
    { 0x100404, 0x200808, 0x381010, 0x501C14 },  /* 5 doh red */
    { 0x1C1004, 0x342008, 0x503410, 0x704C1C },  /* 6 gold */
    { 0x101014, 0x24242C, 0x383844, 0x50505C },  /* 7 steel */
    { 0x200C00, 0x3C1800, 0x5C2808, 0x803C10 },  /* 8 orange */
    { 0x200014, 0x380828, 0x501040, 0x701C5C },  /* 9 magenta */
    { 0x101404, 0x20280C, 0x343C14, 0x48541C },  /* 10 olive */
    { 0x000C18, 0x001C30, 0x08304C, 0x10486C },  /* 11 navy */
};

/* {pattern, palette} for rounds 1..ROUND_DOH */
static const unsigned char themes[ROUND_DOH][2] = {
    { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 6 }, { 6, 1 }, { 7, 11 },
    { 8, 7 }, { 9, 9 }, { 10, 8 }, { 11, 0 }, { 12, 10 }, { 13, 4 }, { 14, 7 }, { 15, 11 },
    { 16, 6 }, { 5, 0 }, { 7, 8 }, { 13, 3 }, { 10, 1 }, { 16, 7 }, { 14, 2 }, { 11, 4 },
    { 9, 8 }, { 8, 10 }, { 12, 9 }, { 6, 2 }, { 3, 11 }, { 14, 6 }, { 16, 9 }, { 5, 2 },
    { 15, 5 }
};

static unsigned hash2(int x, int y)
{
    unsigned h = (unsigned)x * 374761393u + (unsigned)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static float vnoise(int x, int y, int cell)
{
    int gx = x / cell, gy = y / cell;
    float fx = (float)(x % cell) / cell, fy = (float)(y % cell) / cell;
    float a = (hash2(gx, gy) & 255) / 255.0f, b = (hash2(gx + 1, gy) & 255) / 255.0f;
    float c = (hash2(gx, gy + 1) & 255) / 255.0f, d = (hash2(gx + 1, gy + 1) & 255) / 255.0f;
    fx = fx * fx * (3 - 2 * fx);
    fy = fy * fy * (3 - 2 * fy);
    return (a + (b - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy;
}

static int plus_in(int tx, int ty)
{
    int ax = abs(tx - 7), ay = abs(ty - 7);
    return tx >= 0 && ty >= 0 && ((ax <= 1 && ay <= 5) || (ay <= 1 && ax <= 5));
}

/* Returns a palette index 0..3 for pixel (x,y) of pattern k. */
static int bg_pattern(int k, int x, int y)
{
    int tx = x & 15, ty = y & 15;
    switch (k) {
    case 0: /* bevelled tiles */
        if (tx == 0 || ty == 0) return 0;
        if (tx == 1 || ty == 1) return 3;
        if (tx == 15 || ty == 15) return 1;
        return 2;
    case 1: { /* diamonds */
        int d = abs(tx - 8) + abs(ty - 8);
        if (d <= 2) return 3;
        if (d == 3) return 1;
        if (d == 7) return 3;
        if (d == 8) return 0;
        return d > 8 ? 1 : 2;
    }
    case 2: { /* rings */
        float dx, dy, r;
        if ((y >> 4) & 1) tx = (x + 8) & 15;
        dx = tx - 7.5f; dy = ty - 7.5f;
        r = sqrtf(dx * dx + dy * dy);
        if (r < 2.5f) return (dx < 0 && dy < 0) ? 3 : 2;
        if (r < 5.0f) return 1;
        if (r < 6.0f) return (dx + dy < 0) ? 3 : 0;
        return 1;
    }
    case 3: { /* lattice */
        int a = (x + y) & 15, b = (x - y) & 15;
        if (a == 0 || b == 0) return 3;
        if (a == 1 || b == 1) return 0;
        return (((x >> 3) ^ (y >> 3)) & 1) ? 1 : 2;
    }
    case 4: { /* brickwork */
        int by = y & 7, bx = (x + (((y >> 3) & 1) ? 8 : 0)) & 15;
        if (by == 7 || bx == 15) return 0;
        if (by == 0 || bx == 0) return 3;
        return ((bx * 5 + by * 3) % 11 == 0) ? 1 : 2;
    }
    case 5: { /* honeycomb */
        float best = 1e9f, second = 1e9f, bdx = 0, bdy = 0, edge;
        int i, j, j0 = y / 14;
        for (j = j0 - 1; j <= j0 + 2; j++) {
            int off = (j & 1) * 8, i0 = (x - off) >> 4;
            for (i = i0 - 1; i <= i0 + 2; i++) {
                float dx = x - (float)(i * 16 + off), dy = y - (float)(j * 14);
                float d = sqrtf(dx * dx + dy * dy);
                if (d < best) { second = best; best = d; bdx = dx; bdy = dy; }
                else if (d < second) second = d;
            }
        }
        edge = second - best;
        if (edge < 1.0f) return 0;
        if (edge < 2.4f) return (bdy < 0 || (bdx < 0 && bdy < 2)) ? 3 : 1;
        return best < 2.5f ? 3 : 2;
    }
    case 6: { /* circuit board */
        int lx = x & 7, ly = y & 7;
        unsigned h = hash2(x >> 3, y >> 3);
        if ((h & 12) == 12 && lx >= 2 && lx <= 5 && ly >= 2 && ly <= 5)
            return ((lx == 3 || lx == 4) && (ly == 3 || ly == 4)) ? 0 : 3;
        if ((h & 1) && (ly == 3 || ly == 4)) return ly == 3 ? 3 : 2;
        if ((h & 2) && (lx == 3 || lx == 4)) return lx == 3 ? 3 : 2;
        if ((h & 0x70) == 0x70 && lx == 6 && ly == 1) return 2;
        return 1;
    }
    case 7: { /* fish scales */
        int i, j, j0 = y / 6 + 2;
        for (j = j0; j >= j0 - 4; j--) {
            int off = (j & 1) * 8, i0 = (x - off) >> 4;
            for (i = i0 - 1; i <= i0 + 1; i++) {
                float dx = x - (float)(i * 16 + off + 8), dy = y - (float)(j * 6);
                float d = sqrtf(dx * dx + dy * dy);
                if (d < 9.0f) {
                    if (d > 7.8f) return 0;
                    if (d > 6.0f) return 1;
                    return (dx < 0 && dy < -2) ? 3 : 2;
                }
            }
        }
        return 1;
    }
    case 8: { /* basket weave */
        int horiz = ((x >> 3) ^ (y >> 3)) & 1;
        int s = (horiz ? (y & 7) : (x & 7)) & 3;
        int along = horiz ? (x & 7) : (y & 7);
        if (s == 0) return 3;
        if (s == 3) return 0;
        return (along == 0 || along == 7) ? 1 : 2;
    }
    case 9: { /* riveted studs */
        float dx = tx - 7.0f, dy = ty - 7.0f;
        if (tx == 15 || ty == 15) return 0;
        if (tx == 0 || ty == 0) return 2;
        if (dx * dx + dy * dy <= 16.0f) {
            if (dx + dy < -2) return 3;
            if (dx + dy > 2) return 0;
            return 2;
        }
        return 1;
    }
    case 10: { /* chevrons */
        static const int m[4] = { 0, 1, 3, 2 };
        return m[((y + abs(tx - 8)) >> 2) & 3];
    }
    case 11: { /* waves */
        static const int m[8] = { 0, 1, 2, 3, 3, 2, 1, 1 };
        float v = y + sinf(x * 0.19635f) * 3.0f;
        return m[((int)floorf(v)) & 7];
    }
    case 12: /* raised crosses */
        if (!plus_in(tx, ty)) return ((tx ^ ty) & 15) == 0 ? 0 : 1;
        if (!plus_in(tx - 1, ty) || !plus_in(tx, ty - 1)) return 3;
        if (!plus_in(tx + 1, ty) || !plus_in(tx, ty + 1)) return 0;
        return 2;
    case 13: { /* bubbles */
        unsigned h = hash2(x >> 4, y >> 4);
        int r = 3 + (int)(h % 3);
        float dx = (float)(tx - 7 - (int)((h >> 4) % 3) + 1);
        float dy = (float)(ty - 7 - (int)((h >> 8) % 3) + 1);
        float d = sqrtf(dx * dx + dy * dy);
        if (d < r) {
            if (d > r - 1.2f) return 2;
            if ((dx + 1.5f) * (dx + 1.5f) + (dy + 1.5f) * (dy + 1.5f) < 2.0f) return 3;
            return 1;
        }
        return (hash2(x, y) % 53 == 0) ? 2 : 0;
    }
    case 14: { /* pyramids */
        float dx = tx - 7.5f, dy = ty - 7.5f;
        if (fabsf(dx) > fabsf(dy)) return dx < 0 ? 2 : 1;
        return dy < 0 ? 3 : 0;
    }
    case 15: { /* deep space */
        float n = vnoise(x, y, 32) * 0.65f + vnoise(x, y, 8) * 0.35f;
        unsigned h = hash2(x, y) % 1000;
        if (h == 0) return 3;
        if (h < 4) return 2;
        return n > 0.66f ? 2 : n > 0.48f ? 1 : 0;
    }
    default: { /* grille */
        int vy = y & 31, vx = x & 7;
        if (vy < 3) return vy == 0 ? 3 : vy == 1 ? 2 : 0;
        return vx == 0 ? 0 : vx == 1 ? 3 : vx < 4 ? 2 : 1;
    }
    }
}

static void bg_build(int r)
{
    int x, y, pat, pal;
    if (r < 1) r = 1;
    if (r > ROUND_DOH) r = ROUND_DOH;
    pat = themes[r - 1][0];
    pal = themes[r - 1][1];
    for (y = 0; y < SCR_H; y++)
        for (x = 0; x < SCR_W; x++)
            bgbuf[y][x] = bg_pals[pal][bg_pattern(pat, x, y)] | 0xFF000000u;
}

/* ------------------------------------------------------------------ */
/* DOH                                                                 */

static void doh_build(void)
{
    static const uint32_t s[7] = { 0x101828, 0x283850, 0x405878, 0x5878A0,
                                   0x7898C0, 0xA0C0E0, 0xD0E8F8 };
    int m, x, y;
    for (m = 0; m < 2; m++)
        for (y = 0; y < DOH_H; y++)
            for (x = 0; x < DOH_W; x++) {
                int d = x < DOH_W / 2 ? DOH_W / 2 - 1 - x : x - DOH_W / 2;
                float hw, hw2;
                uint32_t c = 0;
                if (y < 10) hw = 12.0f + 9.0f * sqrtf(1.0f - (10 - y) * (10 - y) / 100.0f);
                else if (y < 52) hw = 21.0f;
                else hw = 21.0f - (y - 52) * 0.6f;
                hw2 = (y >= 20 && y <= 44) ? 23.0f : hw;
                if (d > hw2) c = 0;
                else if (d > hw) {
                    /* ears */
                    if (d >= 23 || y == 20 || y == 44 || (y >= 24 && y <= 40 && d == 22)) c = s[0];
                    else c = s[2];
                } else if (d > hw - 1) c = s[0];
                else {
                    float t = d / hw;
                    int idx = 5 - (int)(t * t * 4.0f);
                    if (y < 5) idx++;
                    if (idx > 6) idx = 6;
                    if (idx < 1) idx = 1;
                    c = s[idx];
                    if (y == 16 && d < 19) c = s[6];
                    else if ((y == 17 || y == 18) && d < 19) c = s[5];
                    else if ((y == 19 || y == 20) && d < 18) c = s[1];
                    if (y >= 21 && y <= 29 && d >= 3 && d <= 16)
                        c = (y == 29 || d == 16) ? s[2] : s[1];
                    if (y >= 24 && y <= 26 && d >= 6 && d <= 13 && !(y != 25 && (d == 6 || d == 13)))
                        c = DOH_EYE;
                    if (y >= 17 && y <= 37 && d <= 3)
                        c = d <= 1 ? s[6] : d == 2 ? s[5] : s[3];
                    if (y >= 38 && y <= 43) {
                        int nw = 3 + (y - 38);
                        if (d <= nw) c = d <= nw - 2 ? s[5] : s[3];
                        if (y >= 42 && d >= 2 && d <= 4) c = s[0];
                    }
                    if (y == 44 && d <= 8) c = s[1];
                    if (y >= 40 && y <= 50 && d == 9 + (y - 40) / 4) c = s[1];
                    if (!m) {
                        if (y == 47 && d <= 11) c = s[1];
                        if (y == 48 && d <= 11) c = s[0];
                        if (y == 49 && d <= 10) c = s[5];
                    } else if (y >= 46 && y <= 54 && d <= 11) {
                        if (y == 46 || y == 54 || d == 11) c = s[0];
                        else c = y <= 48 ? 0x700018 : 0x300008;
                    }
                    if (y >= 55 && y <= 59 && d <= 7) c = s[5];
                }
                doh_img[m][y][x] = (c == 0 || c == DOH_EYE) ? c : (c | 0xFF000000u);
            }
}

static int doh_alive(void) { return round_no == ROUND_DOH && doh_hits < DOH_HP; }

/* ------------------------------------------------------------------ */

static int brick_from_char(char ch)
{
    switch (ch) {
    case 'W': return BR_WHITE;  case 'O': return BR_ORANGE;
    case 'C': return BR_CYAN;   case 'G': return BR_GREEN;
    case 'R': return BR_RED;    case 'B': return BR_BLUE;
    case 'P': return BR_PINK;   case 'Y': return BR_YELLOW;
    case 'S': return BR_SILVER; case 'X': return BR_GOLD;
    }
    return BR_NONE;
}

static void level_load(int r)
{
    int row, c, bottom = FIELD_T;
    memset(bricks, 0, sizeof bricks);
    bricks_left = 0;
    if (r <= NROUNDS) {
        for (row = 0; row < ROWS && levels[r - 1][row]; row++) {
            const char *s = levels[r - 1][row];
            for (c = 0; c < COLS && s[c]; c++) {
                int t = brick_from_char(s[c]);
                if (!t) continue;
                bricks[row][c].type = (unsigned char)t;
                bricks[row][c].hits = (unsigned char)(t == BR_SILVER ? 2 + (r - 1) / 8 : 1);
                if (t != BR_GOLD) bricks_left++;
                bottom = BRICK_Y0 + (row + 1) * BH;
            }
        }
    }
    bg_build(r);
    memset(expls, 0, sizeof expls);
    doh_hits = 0; doh_flash = 0; doh_mouth = 0; doh_fire_timer = 240;

    mb_hp = mb_hp_max = 0;
    if (r <= NROUNDS && r % 5 == 0) {
        mb_hp = mb_hp_max = 5 + r / 6;
        mb_x = (FIELD_L + FIELD_R - MB_W) / 2;
        mb_y0 = (float)(bottom + 14 > MB_MAXY ? MB_MAXY : bottom + 14);
        mb_y = mb_y0;
        mb_dx = rnd(2) ? 0.6f : -0.6f;
        mb_flash = 0;
        mb_anim = 0;
    }
}

static void life_start(void)
{
    memset(balls, 0, sizeof balls);
    memset(bullets, 0, sizeof bullets);
    memset(enemies, 0, sizeof enemies);
    memset(shots, 0, sizeof shots);
    memset(gate_open, 0, sizeof gate_open);
    memset(gate_dir, 0, sizeof gate_dir);
    capsule.active = 0;
    pad_w = pad_w_target = 32;
    pad_x = (FIELD_L + FIELD_R) / 2 - 16;
    power = PW_NONE;
    laser_cool = 0;
    pad_anim = 40;
    warp_open = 0;
    balls[0].active = 1;
    balls[0].stuck = 2;
    balls[0].off = 16 - BALL_S / 2 + 4;
    balls[0].x = pad_x + balls[0].off;
    balls[0].y = PAD_Y - BALL_S;
    ball_speed = 1.7f + (round_no > 12 ? 12 : round_no) * 0.03f;
    serve_timer = 180;
    enemy_timer = 240 + rnd(240);
    mb_fire_timer = 300;
}

static void spawn_expl(float x, float y, int delay)
{
    int i;
    for (i = 0; i < MAXEXPL; i++)
        if (!expls[i].active) {
            expls[i].active = 1;
            expls[i].x = x; expls[i].y = y;
            expls[i].t = -delay;
            expls[i].seed = frnd() * 6.28f;
            return;
        }
}

static int ball_count(void)
{
    int i, n = 0;
    for (i = 0; i < MAXBALLS; i++) n += balls[i].active;
    return n;
}

static void ball_set_angle(Ball *b, float deg)
{
    float a = deg * (float)M_PI / 180.0f;
    b->dx = sinf(a);
    b->dy = -cosf(a);
}

static void paddle_bounce(Ball *b)
{
    static const float ang[8] = { -60, -45, -30, -18, 18, 30, 45, 60 };
    float rel = ((b->x + BALL_S * 0.5f) - (pad_x + pad_w * 0.5f)) / (pad_w * 0.5f);
    int seg = (int)((rel + 1.0f) * 4.0f);
    if (seg < 0) seg = 0;
    if (seg > 7) seg = 7;
    ball_set_angle(b, ang[seg]);
}

static void release_balls(void)
{
    int i;
    for (i = 0; i < MAXBALLS; i++) {
        Ball *b = &balls[i];
        if (!b->active || !b->stuck) continue;
        if (b->stuck == 2) ball_set_angle(b, b->off + BALL_S * 0.5f >= pad_w * 0.5f ? 30 : -30);
        else paddle_bounce(b);
        b->stuck = 0;
        b->y = PAD_Y - BALL_S - 0.5f;
    }
}

static void kill_enemy(Enemy *e)
{
    e->active = 0;
    spawn_expl(e->x + 8, e->y + 8, 0);
    snd_play(SND_ENEMY);
    add_score(100);
}

static void maybe_drop_capsule(float x, float y)
{
    static const int weight[NCAPS] = { 4, 4, 4, 4, 4, 1, 1 };
    int t, tries, sum = 0, i;
    if (capsule.active || ball_count() > 1 || round_no == ROUND_DOH || state != ST_PLAY) return;
    if (rnd(100) >= 18) return;
    for (i = 0; i < NCAPS; i++) sum += weight[i];
    for (tries = 0; tries < 4; tries++) {
        int r = rnd(sum);
        for (t = 0; t < NCAPS - 1 && r >= weight[t]; t++) r -= weight[t];
        if (t != last_capsule && !(t == CAP_B && warp_open)) break;
    }
    capsule.active = 1;
    capsule.type = t;
    capsule.x = x;
    capsule.y = y;
    capsule.anim = 0;
}

static void brick_hit(int r, int c)
{
    Brick *b = &bricks[r][c];
    if (b->type == BR_GOLD) { b->flash = 12; snd_play(SND_METAL); return; }
    if (b->type == BR_SILVER) {
        if (--b->hits > 0) { b->flash = 12; snd_play(SND_METAL); return; }
        add_score(50 * round_no);
    } else add_score(brick_score[b->type]);
    snd_play(SND_BRICK);
    b->type = BR_NONE;
    bricks_left--;
    if (ball_speed < MAX_SPEED) ball_speed += 0.004f;
    maybe_drop_capsule((float)(FIELD_L + c * BW), (float)(BRICK_Y0 + r * BH));
}

/* Does box overlap any brick? If hit, damage every overlapped brick. */
static int box_bricks(float x, float y, int w, int h, int hit)
{
    int r, c, any = 0;
    int c0 = (int)floorf((x - FIELD_L) / BW), c1 = (int)floorf((x + w - 0.01f - FIELD_L) / BW);
    int r0 = (int)floorf((y - BRICK_Y0) / BH), r1 = (int)floorf((y + h - 0.01f - BRICK_Y0) / BH);
    if (r0 < 0) r0 = 0;
    if (c0 < 0) c0 = 0;
    if (r1 >= ROWS) r1 = ROWS - 1;
    if (c1 >= COLS) c1 = COLS - 1;
    for (r = r0; r <= r1; r++)
        for (c = c0; c <= c1; c++)
            if (bricks[r][c].type) {
                any = 1;
                if (hit) brick_hit(r, c);
            }
    return any;
}

static int overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static void doh_hit(void)
{
    doh_hits++;
    doh_flash = 10;
    snd_play(SND_DOH_HIT);
    add_score(1000);
    if (doh_hits >= DOH_HP) {
        int i;
        for (i = 0; i < MAXBALLS; i++) balls[i].active = 0;
        memset(shots, 0, sizeof shots);
        state = ST_DOHDEATH;
        st_timer = 200;
    }
}

static void mboss_hit(void)
{
    int i;
    if (mb_flash) return;   /* brief invulnerability so one contact = one hit */
    mb_flash = 10;
    add_score(200);
    if (--mb_hp > 0) { snd_play(SND_DOH_HIT); return; }
    for (i = 0; i < 6; i++)
        spawn_expl(mb_x + 4 + frnd() * (MB_W - 8), mb_y + 2 + frnd() * (MB_H - 4), i * 4);
    snd_play(SND_ENEMY);
    add_score(5000);
}

static int mboss_overlap(float x, float y, float w, float h)
{
    return mb_hp > 0 && overlap(x, y, w, h, mb_x + 2, mb_y + 2, MB_W - 4, MB_H - 4);
}

static int obstacle(float x, float y)
{
    if (box_bricks(x, y, BALL_S, BALL_S, 1)) return 1;
    if (doh_alive() && overlap(x, y, BALL_S, BALL_S, DOH_X + 3, DOH_Y + 2, DOH_W - 6, DOH_H - 4)) {
        doh_hit();
        return 1;
    }
    return 0;
}

static void kill_paddle(void)
{
    int i;
    for (i = 0; i < MAXBALLS; i++) balls[i].active = 0;
    memset(bullets, 0, sizeof bullets);
    memset(shots, 0, sizeof shots);
    capsule.active = 0;
    for (i = 0; i < 5; i++)
        spawn_expl(pad_x + i * pad_w / 4, PAD_Y + 4, i * 5);
    snd_play(SND_DIE);
    state = ST_DIE;
    st_timer = 130;
}

static void split_balls(void)
{
    int i, j, n;
    Ball *src = NULL;
    for (i = 0; i < MAXBALLS; i++)
        if (balls[i].active && !balls[i].stuck) { src = &balls[i]; break; }
    if (!src) {
        release_balls();
        for (i = 0; i < MAXBALLS; i++)
            if (balls[i].active) { src = &balls[i]; break; }
    }
    if (!src) return;
    for (n = 0, j = 0; j < MAXBALLS && n < 2; j++) {
        Ball *b = &balls[j];
        float th, ca, sa, dx, dy, len;
        if (b->active) continue;
        *b = *src;
        th = (n == 0 ? 0.6f : -0.6f);
        ca = cosf(th); sa = sinf(th);
        dx = src->dx * ca - src->dy * sa;
        dy = src->dx * sa + src->dy * ca;
        if (fabsf(dy) < 0.45f) {
            dy = dy < 0 ? -0.45f : 0.45f;
            len = sqrtf(dx * dx + dy * dy);
            dx /= len; dy /= len;
        }
        b->dx = dx; b->dy = dy;
        n++;
    }
}

static void apply_capsule(int t)
{
    add_score(1000);
    snd_play(SND_CAPSULE);
    last_capsule = t;
    if (power == PW_CATCH && t != CAP_C && t != CAP_S && t != CAP_P && t != CAP_B) release_balls();
    switch (t) {
    case CAP_S:
        ball_speed = ball_speed * 0.6f;
        if (ball_speed < 1.5f) ball_speed = 1.5f;
        break;
    case CAP_C: power = PW_CATCH; break;
    case CAP_L: power = PW_LASER; break;
    case CAP_E: power = PW_ENLARGE; snd_play(SND_ENLARGE); break;
    case CAP_D: power = PW_NONE; split_balls(); break;
    case CAP_B: warp_open = 1; break;
    case CAP_P: lives++; snd_play(SND_LIFE); break;
    }
    pad_w_target = power == PW_ENLARGE ? 48 : 32;
}

static void start_round(int r)
{
    round_no = r;
    level_load(r);
    life_start();
    state = ST_INTRO;
    st_timer = 150;
    intro_round = 1;
    paused = 0;
    snd_play(SND_START);
}

static void start_game(int r)
{
    score = 0;
    lives = 2;
    next_extend = 20000;
    last_capsule = -1;
    start_round(r);
}

static void to_title(void)
{
    if (!demo) hiscore_save();
    demo = 0;
    paused = 0;
    state = ST_TITLE;
    st_timer = 0;
    round_no = sel_round;
    bg_build(1);
}

/* ------------------------------------------------------------------ */
/* Update                                                              */

static void update_paddle(const GameInput *in)
{
    float maxx;
    if (in->held & BTN_LEFT) pad_x -= 3.5f;
    if (in->held & BTN_RIGHT) pad_x += 3.5f;
    if (in->mouse_moved) pad_x = in->mouse_x - pad_w * 0.5f;
    if (pad_w < pad_w_target) { pad_w += 1; pad_x -= 0.5f; }
    else if (pad_w > pad_w_target) { pad_w -= 1; pad_x += 0.5f; }
    maxx = FIELD_R - pad_w;
    if (pad_x < FIELD_L) pad_x = FIELD_L;
    if (pad_x >= maxx) {
        pad_x = maxx;
        if (warp_open) {
            int i;
            for (i = 0; i < MAXBALLS; i++) balls[i].active = 0;
            capsule.active = 0;
            state = ST_WARP;
            snd_play(SND_WARP);
        }
    }
}

static void update_stuck_balls(void)
{
    int i;
    for (i = 0; i < MAXBALLS; i++) {
        Ball *b = &balls[i];
        if (!b->active || !b->stuck) continue;
        if (b->off > pad_w - BALL_S) b->off = pad_w - BALL_S;
        if (b->off < 0) b->off = 0;
        b->x = pad_x + b->off;
        b->y = PAD_Y - BALL_S;
    }
}

static void update_balls(void)
{
    const int steps = 4;
    int s, i, j;
    update_stuck_balls();
    for (s = 0; s < steps; s++)
        for (i = 0; i < MAXBALLS; i++) {
            Ball *b = &balls[i];
            float vx, vy;
            if (!b->active || b->stuck) continue;
            vx = b->dx * ball_speed / steps;
            vy = b->dy * ball_speed / steps;

            b->x += vx;
            if (b->x < FIELD_L) { b->x = FIELD_L; b->dx = fabsf(b->dx); }
            else if (b->x + BALL_S > FIELD_R) { b->x = FIELD_R - BALL_S; b->dx = -fabsf(b->dx); }
            else if (obstacle(b->x, b->y)) { b->x -= vx; b->dx = -b->dx; }
            if (!b->active) continue;

            b->y += vy;
            if (b->y < FIELD_T) {
                b->y = FIELD_T;
                b->dy = fabsf(b->dy);
                if (ball_speed < MAX_SPEED) ball_speed += 0.02f;
            } else if (obstacle(b->x, b->y)) { b->y -= vy; b->dy = -b->dy; }
            if (!b->active) continue;

            if (b->dy > 0 && b->y + BALL_S >= PAD_Y && b->y + BALL_S - vy <= PAD_Y + 1.5f &&
                b->x + BALL_S > pad_x && b->x < pad_x + pad_w) {
                b->y = PAD_Y - BALL_S;
                if (power == PW_CATCH) {
                    b->stuck = 1;
                    b->off = b->x - pad_x;
                    catch_timer = 180;
                    snd_play(SND_CATCH);
                } else {
                    paddle_bounce(b);
                    snd_play(SND_PADDLE);
                }
            }

            /* The guardian moves, so push the ball out along the shallower axis
               instead of undoing the step (which could leave it embedded). */
            if (mboss_overlap(b->x, b->y, BALL_S, BALL_S)) {
                float rx = b->x + BALL_S * 0.5f - (mb_x + MB_W * 0.5f);
                float ry = b->y + BALL_S * 0.5f - (mb_y + MB_H * 0.5f);
                float px = (MB_W - 4 + BALL_S) * 0.5f - fabsf(rx);
                float py = (MB_H - 4 + BALL_S) * 0.5f - fabsf(ry);
                if (px < py) {
                    b->x += rx < 0 ? -px : px;
                    b->dx = rx < 0 ? -fabsf(b->dx) : fabsf(b->dx);
                } else {
                    b->y += ry < 0 ? -py : py;
                    b->dy = ry < 0 ? -fabsf(b->dy) : fabsf(b->dy);
                }
                mboss_hit();
            }

            for (j = 0; j < MAXENEMIES; j++) {
                Enemy *e = &enemies[j];
                if (e->active && overlap(b->x, b->y, BALL_S, BALL_S, e->x + 2, e->y + 2, 12, 12)) {
                    kill_enemy(e);
                    b->dy = -b->dy;
                }
            }
            if (b->y > SCR_H) b->active = 0;
        }
}

static void update_bullets(void)
{
    int i, j, s;
    for (i = 0; i < MAXBULLETS; i++) {
        Bullet *u = &bullets[i];
        if (!u->active) continue;
        for (s = 0; s < 2 && u->active; s++) {
            u->y -= 2.5f;
            if (u->y < FIELD_T) { u->active = 0; break; }
            if (box_bricks(u->x, u->y, 2, 6, 1)) { u->active = 0; break; }
            if (mboss_overlap(u->x, u->y, 2, 6)) { mboss_hit(); u->active = 0; break; }
            for (j = 0; j < MAXENEMIES; j++)
                if (enemies[j].active && overlap(u->x, u->y, 2, 6, enemies[j].x + 2, enemies[j].y + 2, 12, 12)) {
                    kill_enemy(&enemies[j]);
                    u->active = 0;
                    break;
                }
        }
    }
}

static void fire_laser(void)
{
    int i, n = 0;
    for (i = 0; i < MAXBULLETS && n < 2; i++)
        if (!bullets[i].active) {
            bullets[i].active = 1;
            bullets[i].x = n == 0 ? pad_x + 2 : pad_x + pad_w - 4;
            bullets[i].y = PAD_Y - 8;
            n++;
        }
    if (n) { laser_cool = 14; snd_play(SND_LASER); }
}

static int enemy_blocked(float x, float y)
{
    if (x < FIELD_L || x + 16 > FIELD_R || y < FIELD_T) return 1;
    return box_bricks(x + 1, y + 1, 14, 14, 0);
}

static void update_enemies(void)
{
    static const float dirs[4][2] = { { 0, 0.5f }, { -0.5f, 0 }, { 0.5f, 0 }, { 0, -0.5f } };
    int i, g;

    if (round_no != ROUND_DOH && --enemy_timer <= 0) {
        int n = 0;
        for (i = 0; i < MAXENEMIES; i++) n += enemies[i].active;
        g = rnd(2);
        if (n < MAXENEMIES && gate_dir[g] == 0 && gate_open[g] == 0) gate_dir[g] = 1;
        enemy_timer = 360 + rnd(360);
    }

    for (g = 0; g < 2; g++) {
        if (gate_dir[g] > 0) {
            if (++gate_open[g] >= 12) {
                gate_open[g] = 12;
                gate_dir[g] = 0;
                gate_hold[g] = 50;
                for (i = 0; i < MAXENEMIES; i++)
                    if (!enemies[i].active) {
                        Enemy *e = &enemies[i];
                        memset(e, 0, sizeof *e);
                        e->active = 1;
                        e->type = (round_no - 1) % 4;
                        e->x = (float)(gate_x[g] + 4);
                        e->y = FIELD_T - 16;
                        e->dy = 0.5f;
                        e->tmr = 60;
                        break;
                    }
            }
        } else if (gate_dir[g] < 0) {
            if (--gate_open[g] <= 0) { gate_open[g] = 0; gate_dir[g] = 0; }
        } else if (gate_open[g] > 0 && --gate_hold[g] <= 0) gate_dir[g] = -1;
    }

    for (i = 0; i < MAXENEMIES; i++) {
        Enemy *e = &enemies[i];
        if (!e->active) continue;
        e->anim++;
        if (e->y < FIELD_T) {
            e->y += 0.5f;
        } else if (e->y > 140) {
            e->x += sinf(e->anim * 0.04f) * 0.9f;
            e->y += 0.3f;
            if (e->x < FIELD_L) e->x = FIELD_L;
            if (e->x > FIELD_R - 16) e->x = FIELD_R - 16;
        } else {
            float nx = e->x + e->dx, ny = e->y + e->dy;
            if (enemy_blocked(nx, ny) || --e->tmr <= 0) {
                int order[4], k, a;
                order[0] = rnd(100) < 60 ? 0 : 1 + rnd(2);
                order[1] = 1 + rnd(2);
                order[2] = order[1] == 1 ? 2 : 1;
                order[3] = 3;
                if (order[0] != 0) { order[1] = 0; order[2] = order[0] == 1 ? 2 : 1; }
                for (k = 0; k < 4; k++) {
                    a = order[k];
                    if (!enemy_blocked(e->x + dirs[a][0], e->y + dirs[a][1])) {
                        e->dx = dirs[a][0];
                        e->dy = dirs[a][1];
                        break;
                    }
                }
                e->tmr = 30 + rnd(90);
            } else {
                e->x = nx;
                e->y = ny;
            }
        }
        if (overlap(e->x + 2, e->y + 2, 12, 12, pad_x, PAD_Y, pad_w, 8)) kill_enemy(e);
        else if (e->y > SCR_H) e->active = 0;
    }
}

static void update_capsule(void)
{
    if (!capsule.active) return;
    capsule.anim++;
    capsule.y += 1.0f;
    if (overlap(capsule.x, capsule.y, 16, 8, pad_x, PAD_Y, pad_w, 8)) {
        capsule.active = 0;
        apply_capsule(capsule.type);
    } else if (capsule.y > SCR_H) capsule.active = 0;
}

/* Fire a shot from (x, y) aimed at the paddle. */
static void fire_shot(float x, float y, float sp)
{
    int i;
    for (i = 0; i < MAXSHOTS; i++)
        if (!shots[i].active) {
            Shot *s = &shots[i];
            float tx = pad_x + pad_w * 0.5f, ty = PAD_Y + 4;
            float dx, dy, len;
            s->active = 1;
            s->x = x; s->y = y;
            dx = tx - s->x; dy = ty - s->y;
            len = sqrtf(dx * dx + dy * dy);
            s->dx = dx / len * sp; s->dy = dy / len * sp;
            s->anim = 0;
            snd_play(SND_DOH_SHOT);
            return;
        }
}

static void update_doh(void)
{
    if (!doh_alive()) return;
    if (doh_flash) doh_flash--;
    if (doh_mouth > 0) {
        if (--doh_mouth == 25) fire_shot(128, DOH_Y + 50, 1.3f + doh_hits * 0.05f);
    } else if (--doh_fire_timer <= 0) {
        doh_mouth = 50;
        doh_fire_timer = 90 + rnd(90) - doh_hits * 3;
    }
}

static void update_mboss(void)
{
    float sp;
    if (mb_flash) mb_flash--;
    if (mb_hp <= 0) return;
    mb_anim++;
    /* speeds up as it takes damage */
    sp = 0.5f + round_no * 0.01f + (mb_hp_max - mb_hp) * 0.08f;
    mb_x += mb_dx < 0 ? -sp : sp;
    if (mb_x < FIELD_L + 2) { mb_x = FIELD_L + 2; mb_dx = 1; }
    if (mb_x > FIELD_R - MB_W - 2) { mb_x = FIELD_R - MB_W - 2; mb_dx = -1; }
    mb_y = mb_y0 + sinf(mb_anim * 0.05f) * 4.0f;
    if (--mb_fire_timer <= 0) {
        fire_shot(mb_x + MB_W * 0.5f, mb_y + MB_H, 1.1f + round_no * 0.01f);
        mb_fire_timer = 150 + rnd(120) - (mb_hp_max - mb_hp) * 8;
    }
}

static int update_shots(void)
{
    int i;
    for (i = 0; i < MAXSHOTS; i++) {
        Shot *s = &shots[i];
        if (!s->active) continue;
        s->anim++;
        s->x += s->dx; s->y += s->dy;
        if (overlap(s->x - 3, s->y - 3, 6, 6, pad_x, PAD_Y, pad_w, 8)) return 1;
        if (s->y > SCR_H || s->x < FIELD_L - 8 || s->x > FIELD_R + 8) s->active = 0;
    }
    return 0;
}

static void update_fx(void)
{
    int i, r, c;
    for (i = 0; i < MAXEXPL; i++)
        if (expls[i].active && ++expls[i].t > 24) expls[i].active = 0;
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            if (bricks[r][c].flash) bricks[r][c].flash--;
}

static void ai_input(GameInput *in)
{
    int i;
    Ball *best = NULL;
    float tx = (FIELD_L + FIELD_R) * 0.5f, c;
    in->held = BTN_FIRE;
    in->pressed = (frame % 30 == 0) ? BTN_FIRE : 0;
    in->mouse_moved = 0;
    for (i = 0; i < MAXBALLS; i++) {
        Ball *b = &balls[i];
        if (!b->active || b->stuck) continue;
        if (!best || (b->dy > 0 && (best->dy <= 0 || b->y > best->y))) best = b;
    }
    if (best && best->dy > 0) {
        float w = FIELD_R - BALL_S - FIELD_L;
        float t = (PAD_Y - BALL_S - best->y) / (best->dy * ball_speed);
        float x = best->x - FIELD_L + best->dx * ball_speed * t;
        x = fmodf(x, 2 * w);
        if (x < 0) x += 2 * w;
        if (x > w) x = 2 * w - x;
        tx = x + FIELD_L + BALL_S * 0.5f + (float)(((frame / 97) % 5) - 2) * 5.0f;
    } else if (capsule.active && capsule.type != CAP_S) tx = capsule.x + 8;
    else if (best) tx = best->x;
    c = pad_x + pad_w * 0.5f;
    if (tx < c - 2) in->held |= BTN_LEFT;
    else if (tx > c + 2) in->held |= BTN_RIGHT;
}

static void next_round(void)
{
    start_round(round_no >= ROUND_DOH ? ROUND_DOH : round_no + 1);
}

void game_update(const GameInput *inp)
{
    GameInput in = *inp;
    int i;
    frame++;

    if (demo) {
        if (in.pressed) { to_title(); return; }
        ai_input(&in);
    }

    switch (state) {
    case ST_TITLE:
        st_timer++;
        if (in.pressed & (BTN_UP | BTN_RIGHT)) { sel_round = sel_round % ROUND_DOH + 1; st_timer = 0; }
        if (in.pressed & (BTN_DOWN | BTN_LEFT)) { sel_round = (sel_round + ROUND_DOH - 2) % ROUND_DOH + 1; st_timer = 0; }
        round_no = sel_round;
        if (in.pressed & (BTN_START | BTN_FIRE)) start_game(sel_round);
        else if (st_timer > 60 * 15) {
            if (attract++ & 1) story_start();
            else {
                demo = 1;
                start_game(1 + rnd(NROUNDS));
            }
        }
        break;

    case ST_STORY:
        if (in.pressed) { to_title(); break; }
        story_update();
        if (++st_timer >= STORY_END) to_title();
        break;

    case ST_INTRO:
        if (in.pressed & BTN_ESC) { to_title(); return; }
        if (pad_anim > 0) pad_anim--;
        update_stuck_balls();
        update_fx();
        if (--st_timer <= 0) state = ST_PLAY;
        break;

    case ST_PLAY:
        if (in.pressed & BTN_ESC) { to_title(); return; }
        if (in.pressed & BTN_PAUSE) paused = !paused;
        if (paused) break;
        update_paddle(&in);
        if (state != ST_PLAY) break;
        if (in.pressed & BTN_FIRE) release_balls();
        if (laser_cool > 0) laser_cool--;
        if (power == PW_LASER && (in.held & BTN_FIRE) && laser_cool == 0) {
            int stuck = 0;
            for (i = 0; i < MAXBALLS; i++) stuck |= balls[i].active && balls[i].stuck;
            if (!stuck) fire_laser();
        }
        for (i = 0; i < MAXBALLS; i++)
            if (balls[i].active && balls[i].stuck) {
                if (balls[i].stuck == 2 ? --serve_timer <= 0 : --catch_timer <= 0) release_balls();
                break;
            }
        if (power != PW_CATCH) {
            for (i = 0; i < MAXBALLS; i++)
                if (balls[i].active && balls[i].stuck == 1) { release_balls(); break; }
        }
        update_balls();
        if (state != ST_PLAY) break;
        update_bullets();
        update_capsule();
        update_enemies();
        update_doh();
        update_mboss();
        update_fx();
        if (update_shots()) { kill_paddle(); break; }
        if (ball_count() == 0) { kill_paddle(); break; }
        if (round_no != ROUND_DOH && bricks_left <= 0 && mb_hp <= 0) {
            state = ST_CLEAR;
            st_timer = 100;
            for (i = 0; i < MAXBALLS; i++) balls[i].active = 0;
            capsule.active = 0;
            memset(enemies, 0, sizeof enemies);
            memset(bullets, 0, sizeof bullets);
            memset(shots, 0, sizeof shots);
            break;
        }
        if (ball_speed < MAX_SPEED) ball_speed += 0.0003f;
        break;

    case ST_DIE:
        update_fx();
        if (--st_timer <= 0) {
            if (lives > 0) {
                lives--;
                life_start();
                state = ST_INTRO;
                st_timer = 90;
                intro_round = 0;
            } else if (demo) {
                to_title();
            } else {
                state = ST_GAMEOVER;
                st_timer = 200;
                snd_play(SND_GAMEOVER);
                hiscore_save();
            }
        }
        break;

    case ST_WARP:
        pad_x += 0.6f;
        update_fx();
        if (pad_x > FIELD_R + 8) {
            add_score(10000);
            next_round();
        }
        break;

    case ST_CLEAR:
        update_fx();
        if (--st_timer <= 0) next_round();
        break;

    case ST_GAMEOVER:
        if (--st_timer <= 0) { state = ST_CONTINUE; st_timer = 600; }
        break;

    case ST_CONTINUE:
        if (in.pressed & (BTN_START | BTN_FIRE)) {
            score = 0;
            lives = 2;
            next_extend = 20000;
            start_round(round_no);
        } else if ((in.pressed & BTN_ESC) || --st_timer <= 0) to_title();
        break;

    case ST_DOHDEATH:
        update_fx();
        if (st_timer % 5 == 0 && st_timer > 30) {
            spawn_expl(DOH_X + 4 + frnd() * (DOH_W - 8), DOH_Y + 4 + frnd() * (DOH_H - 8), 0);
            if (st_timer % 15 == 0) snd_play(SND_ENEMY);
        }
        if (--st_timer <= 0) {
            state = ST_ENDING;
            st_timer = 0;
            snd_play(SND_ENDING);
            if (!demo) hiscore_save();
        }
        break;

    case ST_ENDING:
        st_timer++;
        if (demo || (st_timer > 120 && (in.pressed & (BTN_START | BTN_FIRE | BTN_ESC)))) to_title();
        break;
    }
}

void game_focus_lost(void)
{
    if (state == ST_PLAY && !demo) paused = 1;
}

/* ------------------------------------------------------------------ */
/* Rendering                                                           */

static const uint32_t pipe_c[8]  = { 0x202838, 0x485878, 0x8898B8, 0xD8E0F0, 0xA8B8D8, 0x687898, 0x384058, 0x181C28 };
static const uint32_t brass_c[8] = { 0x302008, 0x705020, 0xB89040, 0xF8E080, 0xD0B058, 0x907030, 0x504018, 0x201808 };
static const uint32_t door_c[8]  = { 0x182030, 0x405878, 0x7090B8, 0x405878, 0x283850, 0x405878, 0x7090B8, 0x182030 };

static uint32_t wall_color(int along, int i)
{
    int seg = ((along - 24) % 48 + 48) % 48;
    if (seg < 6) {
        if (seg == 0 || seg == 5) return brass_c[i] >> 1 & 0x7F7F7F;
        return brass_c[i];
    }
    return pipe_c[i];
}

static void draw_walls(void)
{
    int x, y, i, g;
    for (y = 16; y < SCR_H; y++)
        for (i = 0; i < 8; i++) {
            uint32_t c = wall_color(y, i);
            gfx_pixel(16 + i, y, c);
            gfx_pixel(FIELD_R + i, y, c);
        }
    for (x = 16; x < FIELD_R + 8; x++)
        for (i = 0; i < 8; i++) gfx_pixel(x, 16 + i, wall_color(x, i));
    /* corner blocks */
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++) {
            int d = x > y ? x : y;
            uint32_t c = brass_c[d < 7 ? 7 - d : 0];
            if ((x == 2 || x == 5) && (y == 2 || y == 5)) c = 0x201808;
            gfx_pixel(16 + x, 16 + y, c);
            gfx_pixel(FIELD_R + 7 - x, 16 + y, c);
        }
    /* enemy gates */
    for (g = 0; g < 2; g++) {
        int gx = gate_x[g], o = gate_open[g];
        for (y = 0; y < 8; y++) {
            gfx_pixel(gx - 2, 16 + y, brass_c[1]);
            gfx_pixel(gx - 1, 16 + y, brass_c[3]);
            gfx_pixel(gx + 24, 16 + y, brass_c[3]);
            gfx_pixel(gx + 25, 16 + y, brass_c[1]);
            for (x = 0; x < 24; x++) {
                uint32_t c;
                if (x < 12 - o || x >= 12 + o) c = (x == 11 || x == 12) ? 0x101820 : door_c[y];
                else c = 0x000000;
                gfx_pixel(gx + x, 16 + y, c);
            }
        }
    }
    /* warp gate on the right wall */
    for (i = 0; i < 8; i++) {
        gfx_pixel(FIELD_R + i, 186, brass_c[3]);
        gfx_pixel(FIELD_R + i, 187, brass_c[1]);
        gfx_pixel(FIELD_R + i, 220, brass_c[3]);
        gfx_pixel(FIELD_R + i, 221, brass_c[1]);
    }
    if (warp_open)
        for (y = 188; y < 220; y++)
            for (x = 0; x < 8; x++) {
                int v = (x * 3 - (int)frame + (y & 4)) & 15;
                uint32_t c = v < 2 ? 0xF0F0FF : v < 4 ? 0x60A0F8 : v < 6 ? 0x2040A0 : 0x000010;
                gfx_pixel(FIELD_R + x, y, c);
            }
}

static void draw_brick(int r, int c)
{
    const Brick *b = &bricks[r][c];
    int x = FIELD_L + c * BW, y = BRICK_Y0 + r * BH, xx, yy;
    int metal = b->type == BR_SILVER || b->type == BR_GOLD;
    const uint32_t *rows = b->type == BR_SILVER ? silver_rows : gold_rows;
    const uint32_t *pc = brick_cols[b->type];
    for (yy = 0; yy < BH; yy++)
        for (xx = 0; xx < BW; xx++) {
            uint32_t col;
            if (xx == BW - 1 || yy == BH - 1) col = 0x000000;
            else if (metal) {
                if (xx == 0) col = rows[0];
                else if (xx == BW - 2) col = rows[6];
                else col = rows[yy];
            } else if (yy == 0 || xx == 0) col = pc[1];
            else if (yy == BH - 2 || xx == BW - 2) col = pc[2];
            else col = pc[0];
            if (b->flash && xx < BW - 1 && yy < BH - 1) {
                int p = (12 - b->flash) * 2;
                int v = xx + (BH - 1 - yy) / 2;
                if (v >= p - 4 && v < p - 1) col = 0xFFFFFF;
            }
            gfx_pixel(x + xx, y + yy, col);
        }
}

static const uint32_t vb_body[8] = { 0x505868, 0xB8C0D0, 0xF0F4FF, 0xC8D0E0, 0xA0A8B8, 0x888C9C, 0x646878, 0x383C48 };
static const uint32_t vb_cap[8]  = { 0x701018, 0xE84848, 0xFF9C9C, 0xF05858, 0xD03030, 0xB02020, 0x801418, 0x480810 };
static const uint32_t vb_lcap[8] = { 0x202868, 0x4060D0, 0x90B0FF, 0x5878E0, 0x3858C0, 0x2840A0, 0x203078, 0x101848 };
static const uint32_t vb_band[8] = { 0x105060, 0x40C0E0, 0xB0F8FF, 0x70D8F0, 0x40B0D0, 0x2890B0, 0x186878, 0x0C3840 };

static uint32_t vaus_px(int xx, int yy, int w, int laser)
{
    int ex = xx < w / 2 ? xx : w - 1 - xx;
    if (yy < 0) {
        if (!laser || ex < 2 || ex > 4) return 0;
        return ex == 3 ? 0xE8E8F0 : 0x707888;
    }
    if (ex == 0 && (yy < 2 || yy > 5)) return 0;
    if (ex == 1 && (yy < 1 || yy > 6)) return 0;
    if (ex < 7) return (laser ? vb_lcap : vb_cap)[yy];
    if (ex < 9) return vb_band[yy];
    if (laser && (xx - w / 2 == 0 || xx - w / 2 == -1) && yy >= 2 && yy <= 5) return 0x40F0FF;
    return vb_body[yy];
}

static void draw_vaus(int x, int y, int w, int laser)
{
    int xx, yy;
    for (yy = -3; yy < 8; yy++)
        for (xx = 0; xx < w; xx++) {
            uint32_t c = vaus_px(xx, yy, w, laser);
            if (!c) continue;
            if (pad_anim > 0 && (int)((xx * 7 + yy * 13 + frame * 3) % 40) < pad_anim) continue;
            plot(x + xx, y + yy, c);
        }
}

static void draw_ball(int x, int y)
{
    static const char *spr[5] = { ".122.", "12223", "22233", "22334", ".334." };
    static const uint32_t pal[5] = { 0, 0xFFFFFF, 0xE8E8F0, 0xB0B0C0, 0x707080 };
    int r, c;
    for (r = 0; r < 5; r++)
        for (c = 0; c < 5; c++)
            if (spr[r][c] != '.') plot(x + c, y + r, pal[spr[r][c] - '0']);
}

static const uint32_t cap_cols[NCAPS][3] = {
    { 0xF08020, 0xFFC070, 0x904000 },
    { 0x30C030, 0x90FF90, 0x106010 },
    { 0xE02020, 0xFF8080, 0x801010 },
    { 0x2050E0, 0x80A0FF, 0x102080 },
    { 0x20C0E0, 0x90F0FF, 0x106080 },
    { 0xE040C0, 0xFF98F0, 0x801070 },
    { 0xA0A0B0, 0xF0F0F8, 0x505060 },
};
static const char cap_letters[] = "SCLEDBP";

static void draw_capsule(int x, int y, int type, int anim)
{
    int xx, yy;
    int band = (anim / 3) % 10;
    for (yy = 0; yy < 8; yy++) {
        int x0 = (yy == 0 || yy == 7) ? 3 : (yy == 1 || yy == 6) ? 1 : 0;
        for (xx = x0; xx < 16 - x0; xx++) {
            uint32_t c;
            if (yy <= 1) c = cap_cols[type][1];
            else if (yy >= 6) c = cap_cols[type][2];
            else c = cap_cols[type][0];
            if (xx == x0 || xx == 15 - x0) c = cap_cols[type][2];
            if (yy == band - 1) c = cap_cols[type][1];
            plot(x + xx, y + yy, c);
        }
    }
    if (!g_shadow)
        for (yy = 0; yy < 7; yy++) {
            int bits = gfx_glyph_row(cap_letters[type], yy);
            for (xx = 0; xx < 5; xx++)
                if (bits & (16 >> xx)) {
                    gfx_pixel(x + 6 + xx, y + 1 + yy, 0x000000);
                    gfx_pixel(x + 5 + xx, y + yy, 0xFFFFE0);
                }
        }
}

static uint32_t dim2(uint32_t c) { return (c >> 1) & 0x7F7F7F; }

static void draw_sphere(int cx, int cy, int r, uint32_t base)
{
    int dx, dy;
    for (dy = -r; dy <= r; dy++)
        for (dx = -r; dx <= r; dx++) {
            int d2 = dx * dx + dy * dy;
            int hx = dx + r / 2, hy = dy + r / 2;
            uint32_t c;
            if (d2 > r * r + r) continue;
            if (hx * hx + hy * hy <= r / 2 + 1) c = 0xFFFFFF;
            else if (d2 < (r - 1) * (r - 1)) c = base;
            else c = dim2(base);
            plot(cx + dx, cy + dy, c);
        }
}

static void draw_enemy(const Enemy *e)
{
    static const uint32_t cone[2][4] = {
        { 0xF8F060, 0xA09820, 0x60E0F8, 0x2080A0 },
        { 0xFF80C0, 0xA03070, 0xF0F0F0, 0x9090A0 },
    };
    int x = iround(e->x), y = iround(e->y), r, xx, k;
    float ph = e->anim * 0.12f;
    if (e->type == 0 || e->type == 3) {
        const uint32_t *pc = cone[e->type == 3];
        for (r = 0; r < 14; r++) {
            float hw = 1.5f + r * 0.45f;
            for (xx = -7; xx <= 7; xx++) {
                float u = (xx + 0.5f) / hw;
                int stripe;
                if (fabsf(u) > 1.0f) continue;
                stripe = ((int)floorf((asinf(u) + ph) * 3.0f / (float)M_PI)) & 1;
                plot(x + 8 + xx, y + 1 + r, pc[stripe * 2 + (fabsf(u) > 0.6f || r == 13)]);
            }
        }
    } else if (e->type == 1) {
        static const uint32_t cols[3] = { 0xF04040, 0x40D040, 0x4080FF };
        for (k = 0; k < 3; k++) {
            float a = ph * 0.7f + k * 2.0944f;
            draw_sphere(x + 8 + iround(cosf(a) * 4.0f), y + 8 + iround(sinf(a) * 4.0f), 3, cols[k]);
        }
    } else {
        float ry = fabsf(cosf(ph * 0.5f)) * 6.5f + 0.5f;
        for (k = 0; k < 48; k++) {
            float a = k * 6.2832f / 48.0f;
            int px = x + 8 + iround(cosf(a) * 6.5f), py = y + 8 + iround(sinf(a) * ry);
            uint32_t c = sinf(a) > 0 ? 0xFFA040 : 0x904010;
            plot(px, py, c);
            plot(px, py + 1, dim2(c));
        }
        draw_sphere(x + 8, y + 8, 2, 0x40E0F0);
    }
}

static void draw_expl(const Expl *e)
{
    static const uint32_t ec[7] = { 0xFFFFFF, 0xFFFF90, 0xFFD040, 0xFF8020, 0xD04010, 0x802008, 0x401004 };
    int t = e->t, dx, dy, k, ci;
    float r;
    if (t < 0) return;
    r = 1.5f + t * 0.45f;
    ci = t / 4;
    if (ci > 5) ci = 5;
    for (dy = -(int)r - 1; dy <= (int)r + 1; dy++)
        for (dx = -(int)r - 1; dx <= (int)r + 1; dx++) {
            float d = sqrtf((float)(dx * dx + dy * dy));
            if (d <= r && d >= r - 1.8f) plot(iround(e->x) + dx, iround(e->y) + dy, ec[ci]);
            else if (t < 8 && d < r - 1.8f) plot(iround(e->x) + dx, iround(e->y) + dy, ec[t < 4 ? 0 : 1]);
        }
    for (k = 0; k < 8; k++) {
        float a = k * 0.785f + e->seed, dist = t * 0.9f;
        int px = iround(e->x + cosf(a) * dist), py = iround(e->y + sinf(a) * dist);
        uint32_t c = ec[ci + 1];
        plot(px, py, c); plot(px + 1, py, c); plot(px, py + 1, c); plot(px + 1, py + 1, c);
    }
}

static void draw_doh(void)
{
    static const uint32_t eye[6] = { 0xFF2010, 0xFF6010, 0xFFB020, 0xFFF080, 0xFFB020, 0xFF6010 };
    int x, y, m = doh_mouth > 0;
    uint32_t ec = eye[(frame / 4) % 6];
    for (y = 0; y < DOH_H; y++)
        for (x = 0; x < DOH_W; x++) {
            uint32_t c = doh_img[m][y][x];
            if (!c) continue;
            if (c == DOH_EYE) c = ec;
            if (doh_flash && (doh_flash & 2)) c = (c == ec) ? c : 0xFFFFFF;
            plot(DOH_X + x, DOH_Y + y, c);
        }
}

static void draw_mboss(void)
{
    static const uint32_t eye[6] = { 0xFF2010, 0xFF6010, 0xFFB020, 0xFFF080, 0xFFB020, 0xFF6010 };
    static const uint32_t dome[5] = { 0x302858, 0x4838A0, 0x6858C8, 0x9080E8, 0xC8B8FF };
    int x0 = iround(mb_x), y0 = iround(mb_y), x, y;
    int flash = mb_flash & 2;
    /* pupil follows the paddle */
    float look = (pad_x + pad_w * 0.5f - (mb_x + MB_W * 0.5f)) / 60.0f;
    int px = 14 + (look > 1 ? 2 : look < -1 ? -2 : iround(look * 2));
    for (y = 0; y < MB_H; y++)
        for (x = 0; x < MB_W; x++) {
            float fx = x + 0.5f - 14, rim, dm;
            uint32_t c = 0;
            rim = (fx * fx) / (13.5f * 13.5f) + (y + 0.5f - 11) * (y + 0.5f - 11) / (4.5f * 4.5f);
            dm = (fx * fx) / 64.0f + (y + 0.5f - 9) * (y + 0.5f - 9) / 49.0f;
            if (y <= 10 && dm <= 1.0f) {
                int k = (int)((1.0f - dm) * 3.0f) + (y < 5);
                c = dome[k > 4 ? 4 : k];
                if ((x - px) * (x - px) + (y - 6) * (y - 6) <= 8) {
                    c = eye[(mb_anim / 4) % 6];
                    if (x == px && y >= 4 && y <= 8) c = 0x200000;
                }
            } else if (rim <= 1.0f) {
                int k = y - 7;
                c = silver_rows[k < 0 ? 0 : k > 6 ? 6 : k];
                if (rim > 0.8f) c = silver_rows[6];
                if (y == 11 && x % 4 == 2)
                    c = ((x / 4 + mb_anim / 6) % 7 == 0) ? 0x40FFFF : 0x206080;
            } else if (y >= 16 && x >= 12 && x <= 15) {
                c = ((mb_anim >> 1) + x) & 1 ? 0xFFC040 : 0xFF6020;
            } else if (y == 11 && (x == 0 || x == MB_W - 1)) {
                c = silver_rows[5];
            }
            if (!c) continue;
            if (flash) c = 0xFFFFFF;
            plot(x0 + x, y0 + y, c);
        }
    if (!g_shadow && mb_hp < mb_hp_max) {
        int w = (MB_W - 4) * mb_hp / mb_hp_max;
        gfx_rect(x0 + 2, y0 - 3, MB_W - 4, 1, 0x401010);
        gfx_rect(x0 + 2, y0 - 3, w, 1, mb_hp * 3 <= mb_hp_max ? C_RED : C_YELL);
    }
}

static void draw_shot(const Shot *s)
{
    static const uint32_t sc[4] = { 0xFFFFFF, 0xFF4040, 0xFFC040, 0xFF4040 };
    int dx, dy;
    for (dy = -3; dy <= 3; dy++)
        for (dx = -3; dx <= 3; dx++) {
            int d2 = dx * dx + dy * dy;
            if (d2 > 10) continue;
            if (d2 <= 2) plot(iround(s->x) + dx, iround(s->y) + dy, 0xFFFFC0);
            else {
                float a = atan2f((float)dy, (float)dx) + s->anim * 0.3f;
                int q = ((int)floorf(a * 2.0f / (float)M_PI) % 4 + 4) % 4;
                plot(iround(s->x) + dx, iround(s->y) + dy, sc[q]);
            }
        }
}

static void draw_lives(void)
{
    static const uint32_t body[4] = { 0xB8C0D0, 0xF0F4FF, 0x888C9C, 0x383C48 };
    static const uint32_t cap[4]  = { 0xE84848, 0xFF9C9C, 0xB02020, 0x480810 };
    int i, xx, yy, n = lives > 7 ? 7 : lives;
    for (i = 0; i < n; i++)
        for (yy = 0; yy < 4; yy++)
            for (xx = 0; xx < 12; xx++) {
                int ex = xx < 6 ? xx : 11 - xx;
                if (ex == 0 && (yy == 0 || yy == 3)) continue;
                gfx_pixel(FIELD_L + 3 + i * 14 + xx, 217 + yy, ex < 3 ? cap[yy] : body[yy]);
            }
}

static void draw_hud(void)
{
    char buf[32];
    if (!(state == ST_PLAY && !paused && (frame / 16) % 2)) gfx_text(24, 1, demo ? "DEMO" : "1UP", C_RED);
    sprintf(buf, "%06ld", score);
    gfx_text(24, 9, buf, C_WHITE);
    gfx_text(98, 1, "HIGH SCORE", C_RED);
    sprintf(buf, "%06ld", hiscore);
    gfx_text(110, 9, buf, C_WHITE);
    gfx_text(196, 1, "ROUND", C_RED);
    sprintf(buf, "%02d", round_no);
    gfx_text(208, 9, buf, C_WHITE);
}

static const uint32_t logo_grad[7] = { 0xFFFFFF, 0xD8E4FF, 0xA8BCF0, 0x6C88D8, 0xF0F4FF, 0xB8C8E8, 0x7890C8 };

static void render_title(void)
{
    char buf[40];
    int x, y;
    for (y = 0; y < SCR_H; y++)
        for (x = 0; x < SCR_W; x++) {
            int sy = (y + (int)(frame / 2)) % SCR_H;
            fb[y][x] = (bgbuf[sy][x] >> 1 & 0x7F7F7F) | 0xFF000000u;
        }
    gfx_text_big((SCR_W - (8 * 18 - 3)) / 2, 30, "ARKANOID", 3, logo_grad, 7, 0x701020);
    gfx_text_center(60 + 0, "", 0);
    gfx_text_center(64, "DOH IT AGAIN", C_YELL);

    g_shadow = 0;
    pad_anim = 0;
    draw_vaus(112, 92, 32, 0);
    {
        float t = frame * 0.05f;
        draw_ball(125 + iround(sinf(t) * 40.0f), 86 - iround(fabsf(cosf(t * 2.0f)) * 14.0f));
    }

    if ((frame / 24) % 2 == 0) gfx_text_center(116, "PUSH START", C_WHITE);
    sprintf(buf, "ROUND < %02d >", sel_round);
    gfx_text_center(134, sel_round == ROUND_DOH ? "ROUND < DOH >" : buf, C_CYAN);
    sprintf(buf, "HIGH SCORE %06ld", hiscore);
    gfx_text_center(152, buf, C_RED);
    gfx_text_center(176, "ARROWS / MOUSE  MOVE", 0xC0C0C0);
    gfx_text_center(186, "SPACE / CLICK  FIRE", 0xC0C0C0);
    gfx_text_center(196, "P  PAUSE    ESC  TITLE", 0xC0C0C0);
    gfx_text_center(212, "A FAN REMAKE FOR POWER MAC", 0x6070A0);
}

static void render_ending(void)
{
    static const char *lines[] = {
        "CONGRATULATIONS!", "", "DOH HAS BEEN DESTROYED.", "",
        "THE VAUS ESCAPES THE", "DIMENSION-CONTROLLING FORT", "AND SAILS HOME THROUGH",
        "THE VOID OF SPACE...", 0
    };
    char buf[40];
    int i, x, y, n = st_timer / 6;
    for (y = 0; y < SCR_H; y++)
        for (x = 0; x < SCR_W; x++) {
            unsigned h = (unsigned)(x * 7919 + y * 104729);
            fb[y][x] = ((h % 211) == 0 && ((h >> 3) + frame / 8) % 3) ? 0xFFC0C8FF : 0xFF000008;
        }
    for (i = 0; lines[i]; i++) {
        char part[40];
        int len = (int)strlen(lines[i]);
        if (n <= 0) break;
        strncpy(part, lines[i], sizeof part);
        part[sizeof part - 1] = 0;
        if (n < len) part[n] = 0;
        n -= len + 4;
        gfx_text((SCR_W - gfx_text_width(lines[i])) / 2, 40 + i * 12, part, i == 0 ? C_YELL : C_WHITE);
    }
    if (st_timer > 120) {
        sprintf(buf, "FINAL SCORE %06ld", score);
        gfx_text_center(160, buf, C_CYAN);
        gfx_text_big((SCR_W - (7 * 12 - 2)) / 2, 180, "THE END", 2, logo_grad, 7, 0x701020);
    }
}

/* ------------------------------------------------------------------ */
/* Intro story                                                         */

#define MS_W 128
#define MS_H 40
#define EN_W 48
#define EN_H 24
#define MARK_WIN 2u
#define MARK_EYE 3u

static uint32_t ms_img[MS_H][MS_W];
static uint32_t en_img[EN_H][EN_W];

static const struct { int t0; const char *text; } story_text[] = {
    {   40, "THE ERA AND TIME OF THIS\nSTORY IS UNKNOWN." },
    {  330, "THE MOTHERSHIP ARKANOID IS\nATTACKED BY AN UNKNOWN FOE..." },
    {  850, "THE SPACECRAFT VAUS SCRAMBLES\nAWAY FROM THE DOOMED SHIP," },
    { 1110, "ONLY TO BE TRAPPED IN A\nSPACE WARPED BY SOMEONE..." },
    { 1420, "THE DIMENSION-CONTROLLING\nFORT DOH. ESCAPE, VAUS!" },
};

static void ms_build(void)
{
    static const uint32_t m[6] = { 0x141A28, 0x34445C, 0x506890, 0x7890B8, 0xA8BCDC, 0xE0ECFF };
    const char *name = "ARKANOID";
    int x, y, i, r, b;
    memset(ms_img, 0, sizeof ms_img);
    for (x = 0; x < MS_W; x++) {
        int top, bot;
        if (x < 8) { top = 15; bot = 29; }
        else if (x < 14) { top = 13; bot = 30; }
        else if (x < 100) { top = 11; bot = 31; }
        else {
            float k = (x - 100) / 27.0f;
            top = 11 + (int)(k * k * 11);
            bot = 31 - (int)(k * 9);
        }
        if (x >= 56 && x <= 84) {
            int tt = 4 + (x > 76 ? x - 76 : 0);
            if (tt < top) top = tt;
        }
        if (x >= 20 && x <= 44) {
            int bb = 31 + (x < 32 ? (x - 20) / 2 : (44 - x) / 2);
            if (bb > bot) bot = bb;
        }
        for (y = top; y <= bot; y++) {
            uint32_t c;
            if (x < 8) {
                int k = (y - 15) % 5;
                c = m[k == 4 ? 0 : k == 0 ? 3 : 1];
            } else {
                float t = (float)(y - top) / (float)(bot - top + 1);
                int idx = t < 0.12f ? 5 : t < 0.3f ? 4 : t < 0.55f ? 3 : t < 0.8f ? 2 : 1;
                if (y == top || y == bot || x == 8 || x == MS_W - 1) idx = 0;
                else if (x % 16 == 0 && idx > 1) idx--;
                c = m[idx];
                if (y >= 25 && y <= 26 && x >= 14 && x <= 116 && y < bot) c = y == 25 ? 0xD03030 : 0x801818;
                if (y == 14 && x >= 18 && x <= 98 && x % 5 == 0) c = MARK_WIN;
                if (y == 8 && x >= 60 && x <= 78 && x % 3 == 0) c = MARK_WIN;
            }
            ms_img[y][x] = c;
        }
    }
    for (i = 0; name[i]; i++)
        for (r = 0; r < 7; r++) {
            int bits = gfx_glyph_row(name[i], r);
            for (b = 0; b < 5; b++)
                if (bits & (16 >> b)) ms_img[16 + r][34 + i * 6 + b] = 0x202838;
        }
    for (y = 0; y < MS_H; y++)
        for (x = 0; x < MS_W; x++)
            if (ms_img[y][x] > MARK_EYE) ms_img[y][x] |= 0xFF000000u;
}

static void en_build(void)
{
    static const uint32_t p[5] = { 0x180818, 0x401848, 0x682C78, 0x9048A8, 0xC078E0 };
    int x, y;
    for (y = 0; y < EN_H; y++)
        for (x = 0; x < EN_W; x++) {
            float dy = fabsf(y - 11.5f), hh, ex, ey;
            uint32_t c = 0;
            if (x < 30) hh = x * 0.38f;
            else hh = 11.5f - ((x / 3) % 2) * 4.0f;
            if (x > 44) hh -= (x - 44) * 2.0f;
            if (dy <= hh) {
                float t = dy / (hh + 0.01f);
                int idx = t > 0.85f ? 0 : t > 0.6f ? 1 : t > 0.3f ? 2 : 3;
                if (y < 11 && idx >= 2 && idx < 4) idx++;
                c = p[idx] | 0xFF000000u;
                ex = x - 26.0f; ey = y - 11.5f;
                if (ex * ex + ey * ey < 9.0f) c = MARK_EYE;
            }
            en_img[y][x] = c;
        }
}

static uint32_t burn(uint32_t c)
{
    uint32_t r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
    r = r + 70 > 255 ? 255 : r + 70;
    return (r << 16) | ((g * 5 / 8) << 8) | (b / 2);
}

/* Draw columns [xa,xb) of an image; small values are animated markers. */
static void draw_img(const uint32_t *img, int w, int h, int x0, int y0, int xa, int xb,
                     uint32_t win, uint32_t eye, int burning)
{
    int x, y;
    for (y = 0; y < h; y++)
        for (x = xa; x < xb; x++) {
            uint32_t c = img[y * w + x];
            if (!c) continue;
            if (c == MARK_WIN) c = win;
            else if (c == MARK_EYE) c = eye;
            else if (burning) c = burn(c);
            gfx_pixel(x0 + x, y0 + y, c);
        }
}

static void draw_line(int x0, int y0, int x1, int y1, uint32_t c)
{
    int dx = abs(x1 - x0), dy = -abs(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        int e2 = 2 * err;
        gfx_pixel(x0, y0, c);
        gfx_pixel(x0, y0 + 1, c);
        if (x0 == x1 && y0 == y1) break;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void draw_vaus_scaled(int cx, int cy, float s)
{
    int W = (int)(32 * s), H = (int)(11 * s), i, j;
    if (W < 1 || H < 1) return;
    for (j = 0; j < H; j++)
        for (i = 0; i < W; i++) {
            uint32_t c = vaus_px((int)(i / s), (int)(j / s) - 3, 32, 0);
            if (c) gfx_pixel(cx - W / 2 + i, cy - H / 2 + j, c);
        }
}

static void story_ms_pos(int t, float *x, float *y)
{
    *x = t < 300 ? -140.0f + t * (200.0f / 300.0f) : 60.0f + (t - 300) * 0.02f;
    *y = 50.0f + sinf(t * 0.03f) * 2.0f;
}

static void story_en_pos(int t, float *x, float *y)
{
    *x = t < 360 ? 300.0f : t < 540 ? 300.0f - (t - 360) * (110.0f / 180.0f) : 190.0f;
    *y = 22.0f + sinf(t * 0.05f) * 3.0f;
    if (t > 880) { *x += (t - 880) * 1.5f; *y -= (t - 880) * 0.5f; }
}

/* Vaus path: launch from the wreck, drift, then spiral into the warp. */
static int story_vaus_pos(int t, float *x, float *y, float *s)
{
    const float vx = 200, vy = 52;
    float mx, my, ax, ay, r0, a0, p;
    if (t < 850 || t >= 1350) return 0;
    story_ms_pos(850, &mx, &my);
    ax = mx + 64 + (t < 1100 ? t - 850 : 250) * 0.22f;
    ay = my + 30 - (t < 1100 ? t - 850 : 250) * 0.06f;
    *s = 1.0f;
    if (t >= 1100) {
        p = (t - 1100) / 250.0f;
        r0 = sqrtf((ax - vx) * (ax - vx) + (ay - vy) * (ay - vy));
        a0 = atan2f(ay - vy, ax - vx);
        ax = vx + cosf(a0 + p * 7.0f) * r0 * (1 - p);
        ay = vy + sinf(a0 + p * 7.0f) * r0 * (1 - p);
        *s = 1.0f - p * 0.9f;
    }
    *x = ax; *y = ay;
    return 1;
}

static void story_update(void)
{
    int t = st_timer;
    float mx, my;
    story_ms_pos(t, &mx, &my);
    if (t >= 610 && t < 830 && t % 7 == 0)
        spawn_expl(mx + 12 + frnd() * 108, my + 8 + frnd() * 24, 0);
    if (t >= 600 && t < 820 && t % 30 == 0) snd_play(SND_LASER);
    if (t >= 610 && t < 840 && t % 21 == 0) snd_play(SND_ENEMY);
    if (t == 830) snd_play(SND_DIE);
    if (t >= 840 && t < 1000 && t % 12 == 0)
        spawn_expl(mx + 64 + (t - 840) * 0.15f * (frnd() * 2 - 1), my + 10 + frnd() * 20, 0);
    if (t == 860) snd_play(SND_ENLARGE);
    if (t == 1340) snd_play(SND_WARP);
    if (t == 1400) snd_play(SND_DOH_HIT);
    if (t >= 1560 && t < 1680 && t % 24 == 0) snd_play(SND_DOH_SHOT);
    update_fx();
}

static void draw_typed(int x, int y, const char *s, int n, uint32_t c)
{
    char line[48];
    int len = 0;
    for (; n > 0; s++, n--) {
        if (*s == '\n' || !*s) {
            line[len] = 0;
            gfx_text(x, y, line, c);
            len = 0;
            y += 12;
            if (!*s) return;
        } else if (len < 47) line[len++] = *s;
    }
    line[len] = 0;
    gfx_text(x, y, line, c);
}

static void render_story(void)
{
    int t = st_timer, i, x, y, cur = -1;
    float mx, my, ex, ey, vx, vy, vs;
    int shake = 0;

    gfx_noclip();
    gfx_clear(0x000000);

    if (t < 1390) {
        /* parallax starfield */
        static const uint32_t sc[3] = { 0x383850, 0x7878A0, 0xFFFFFF };
        static const float sp[3] = { 0.15f, 0.4f, 1.0f };
        int l;
        for (l = 0; l < 3; l++)
            for (i = 0; i < 45; i++) {
                unsigned h = hash2(l * 1000 + i, 77);
                int sx = (int)(h % 256), sy = (int)((h >> 8) % 172);
                int px = ((sx - (int)(t * sp[l])) % 256 + 256) % 256;
                gfx_pixel(px, sy, sc[l]);
                if (l == 2 && (h & 16)) gfx_pixel(px + 1, sy, sc[1]);
            }
        /* ringed planet */
        for (y = 100; y < 172; y++)
            for (x = 0; x < 120; x++) {
                float dx = x - 34.0f, dy = y - 150.0f, d = sqrtf(dx * dx + dy * dy);
                float ry = (y - 150.0f) / 0.22f, e = sqrtf(dx * dx + ry * ry);
                if (e > 54.0f && e < 70.0f && (dy > 0 || d >= 46.0f)) {
                    int k = (int)e;
                    gfx_pixel(x, y, (k & 3) == 0 ? 0x605038 : (k > 60 ? 0xC0A070 : 0x907850));
                } else if (d < 46.0f) {
                    float lit = (-dx * 0.6f - dy * 0.8f) / 46.0f;
                    int band = ((int)(y + dx * 0.15f) >> 2) & 3;
                    static const uint32_t pc[4][4] = {
                        { 0x101838, 0x203060, 0x304890, 0x5070C0 },
                        { 0x0C1430, 0x182850, 0x284080, 0x4060B0 },
                        { 0x141C40, 0x243468, 0x384C98, 0x5878C8 },
                        { 0x0C1430, 0x182850, 0x284080, 0x4060B0 },
                    };
                    int li = lit > 0.45f ? 3 : lit > 0.1f ? 2 : lit > -0.3f ? 1 : 0;
                    if (((x + y) & 1) && lit > 0.43f && lit < 0.5f) li = 2;
                    gfx_pixel(x, y, pc[band][li]);
                }
            }

        if (t >= 700 && t < 840) shake = (int)(frnd() * 3) - 1;

        /* the mothership: whole, then broken in two */
        story_ms_pos(t, &mx, &my);
        {
            uint32_t win = ((t / 20) & 1) ? 0xFFF080 : 0xE0B040;
            int burning = t >= 640 && t < 840 && (t & 4);
            if (t < 840)
                draw_img(&ms_img[0][0], MS_W, MS_H, (int)mx + shake, (int)my + shake, 0, MS_W, win, 0, burning);
            else if (t < 1150) {
                float d = (t - 840) * 0.15f;
                draw_img(&ms_img[0][0], MS_W, MS_H, (int)(mx - d), (int)(my + d * 0.4f), 0, 64, 0x402010, 0, 1);
                draw_img(&ms_img[0][0], MS_W, MS_H, (int)(mx + d), (int)(my - d * 0.3f), 64, MS_W, 0x402010, 0, 1);
            }
            /* engine flames */
            if (t < 840)
                for (i = 0; i < 3; i++) {
                    int fy = (int)my + 16 + i * 5, len = 6 + (int)(frnd() * 6);
                    gfx_rect((int)mx - len + shake, fy + shake, len, 2, (frame & 2) ? 0xFFC040 : 0xFF8020);
                    gfx_rect((int)mx - len / 2 + shake, fy + shake, len / 2, 2, 0xFFFFC0);
                }
        }

        /* enemy cruiser and its beams */
        story_en_pos(t, &ex, &ey);
        if (t >= 360 && t < 1100) {
            uint32_t eye = ((t / 6) & 1) ? 0xFF3020 : 0xFFA040;
            draw_img(&en_img[0][0], EN_W, EN_H, (int)ex, (int)ey, 0, EN_W, 0, eye, 0);
            if (t >= 600 && t < 820 && t % 30 < 14) {
                unsigned h = hash2(t / 30, 5);
                int tx = (int)mx + 20 + (int)(h % 90), ty = (int)my + 10 + (int)((h >> 8) % 20);
                draw_line((int)ex + 2, (int)ey + 12, tx, ty, (t & 2) ? 0xFF80FF : 0xFFFFFF);
            }
        }

        /* warp vortex */
        if (t >= 1040 && t < 1390) {
            float R = t < 1340 ? (t - 1040) * 0.8f : (1390 - t) * 0.9f;
            static const uint32_t vc[5] = { 0x000000, 0x401060, 0x8030B0, 0xD070F8, 0xFFFFFF };
            if (R > 46) R = 46;
            for (y = (int)-R; y <= (int)R; y++)
                for (x = (int)-R; x <= (int)R; x++) {
                    float d = sqrtf((float)(x * x + y * y)), a, sw, b;
                    int idx;
                    if (d > R) continue;
                    a = atan2f((float)y, (float)x);
                    sw = a * 3.0f + d * 0.35f - t * 0.15f;
                    b = 1.0f - d / R;
                    idx = (int)(b * 3.2f) + (sinf(sw) > 0.2f);
                    if (idx > 4) idx = 4;
                    if (idx > 0) gfx_pixel(200 + x, 52 + y, vc[idx]);
                }
        }

        /* the Vaus */
        if (story_vaus_pos(t, &vx, &vy, &vs)) {
            if (t < 1100) {
                int len = 4 + (int)(frnd() * 4);
                gfx_rect((int)vx - 16 - len, (int)vy - 1, len, 2, (frame & 2) ? 0xFFE060 : 0x60D0FF);
            }
            draw_vaus_scaled((int)vx, (int)vy, vs);
        }

        gfx_clip(0, 0, SCR_W, 172);
        for (i = 0; i < MAXEXPL; i++)
            if (expls[i].active) draw_expl(&expls[i]);
        gfx_noclip();

        if (t >= 820 && t < 850) gfx_flash((850 - t) * 8);
        if (t >= 1340 && t < 1390) gfx_flash((t - 1340) * 5);
    } else {
        /* DOH rises out of the warped space */
        int fade = (t - 1390) * 2, m, ox = 0, oy = 0;
        uint32_t eye = t > 1500 ? (((t / 4) & 1) ? 0xFFF080 : 0xFF4020) : 0xFF2010;
        for (y = 0; y < 172; y++)
            for (x = 0; x < SCR_W; x++)
                fb[y][x] = bgbuf[(y + t / 3) % SCR_H][x];
        if (t >= 1560 && t < 1680) { ox = (int)(frnd() * 3) - 1; oy = (int)(frnd() * 3) - 1; }
        m = t > 1560 && t < 1680 && ((t / 12) & 1);
        if (fade > 256) fade = 256;
        for (y = 0; y < DOH_H * 2; y++)
            for (x = 0; x < DOH_W * 2; x++) {
                uint32_t c = doh_img[m][y / 2][x / 2];
                if (!c) continue;
                if (c == DOH_EYE) c = eye;
                c = ((((c >> 16) & 255) * fade >> 8) << 16) | ((((c >> 8) & 255) * fade >> 8) << 8) |
                    ((c & 255) * fade >> 8);
                gfx_pixel(128 - DOH_W + x + ox, 30 + y + oy, c);
            }
        if (t < 1420) gfx_flash((1420 - t) * 8);
    }

    /* story text box */
    for (i = 0; i < (int)(sizeof story_text / sizeof story_text[0]); i++)
        if (t >= story_text[i].t0) cur = i;
    gfx_rect(12, 174, 232, 46, 0x000000);
    gfx_rect(12, 174, 232, 1, brass_c[3]);
    gfx_rect(12, 219, 232, 1, brass_c[1]);
    gfx_rect(12, 174, 1, 46, brass_c[3]);
    gfx_rect(243, 174, 1, 46, brass_c[1]);
    gfx_rect(14, 176, 228, 1, brass_c[6]);
    if (cur >= 0) draw_typed(22, 184, story_text[cur].text, (t - story_text[cur].t0) / 2, C_WHITE);

    if (t < 40) gfx_fade(t * 256 / 40);
    if (t > STORY_END - 60) gfx_fade((STORY_END - t) * 256 / 60);
}

static void story_start(void)
{
    state = ST_STORY;
    st_timer = 0;
    memset(expls, 0, sizeof expls);
    bg_build(ROUND_DOH);
}

void game_render(void)
{
    int x, y, r, c, i;
    int show_pad = state == ST_INTRO || state == ST_PLAY || state == ST_WARP ||
                   state == ST_CLEAR || state == ST_DOHDEATH;
    int show_doh = round_no == ROUND_DOH &&
                   (doh_hits < DOH_HP || (state == ST_DOHDEATH && st_timer > 30 && (st_timer > 120 || (frame & 2))));
    char buf[40];

    gfx_noclip();
    gfx_clear(0x000000);
    if (state == ST_TITLE) { render_title(); return; }
    if (state == ST_ENDING) { render_ending(); return; }
    if (state == ST_STORY) { render_story(); return; }

    for (y = FIELD_T; y < SCR_H; y++)
        for (x = FIELD_L; x < FIELD_R; x++) fb[y][x] = bgbuf[y][x];

    /* shadows */
    gfx_clip(FIELD_L, FIELD_T, FIELD_R, SCR_H);
    gfx_shadow_begin();
    g_shadow = 1;
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            if (bricks[r][c].type)
                gfx_shadow_rect(FIELD_L + c * BW + SHADOW, BRICK_Y0 + r * BH + SHADOW, BW, BH);
    if (show_doh) draw_doh();
    if (mb_hp > 0) draw_mboss();
    for (i = 0; i < MAXENEMIES; i++)
        if (enemies[i].active && enemies[i].y >= FIELD_T) draw_enemy(&enemies[i]);
    if (capsule.active) draw_capsule(iround(capsule.x), iround(capsule.y), capsule.type, capsule.anim);
    if (show_pad) draw_vaus(iround(pad_x), PAD_Y, iround(pad_w), power == PW_LASER);
    for (i = 0; i < MAXBALLS; i++)
        if (balls[i].active) draw_ball(iround(balls[i].x), iround(balls[i].y));
    g_shadow = 0;

    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            if (bricks[r][c].type) draw_brick(r, c);
    if (show_doh) draw_doh();
    if (mb_hp > 0) draw_mboss();

    gfx_noclip();
    draw_walls();

    gfx_clip(FIELD_L, 16, FIELD_R, SCR_H);
    for (i = 0; i < MAXENEMIES; i++)
        if (enemies[i].active) draw_enemy(&enemies[i]);
    gfx_clip(FIELD_L, FIELD_T, FIELD_R, SCR_H);
    if (capsule.active) draw_capsule(iround(capsule.x), iround(capsule.y), capsule.type, capsule.anim);
    for (i = 0; i < MAXBULLETS; i++)
        if (bullets[i].active) {
            int bx = iround(bullets[i].x), by = iround(bullets[i].y);
            gfx_rect(bx, by, 2, 6, 0xFFF080);
            gfx_rect(bx, by + 4, 2, 2, 0xFF6020);
        }
    for (i = 0; i < MAXSHOTS; i++)
        if (shots[i].active) draw_shot(&shots[i]);
    gfx_clip(FIELD_L, FIELD_T, FIELD_R + 8, SCR_H);
    if (show_pad) draw_vaus(iround(pad_x), PAD_Y, iround(pad_w), power == PW_LASER);
    gfx_clip(FIELD_L, FIELD_T, FIELD_R, SCR_H);
    for (i = 0; i < MAXBALLS; i++)
        if (balls[i].active) draw_ball(iround(balls[i].x), iround(balls[i].y));
    for (i = 0; i < MAXEXPL; i++)
        if (expls[i].active) draw_expl(&expls[i]);
    gfx_noclip();

    draw_lives();
    draw_hud();

    if (state == ST_INTRO) {
        if (intro_round) {
            if (round_no == ROUND_DOH) strcpy(buf, "FINAL ROUND");
            else sprintf(buf, "ROUND %d", round_no);
            gfx_text_center(130, buf, C_WHITE);
            if (mb_hp > 0 && (frame / 16) % 2) gfx_text_center(186, "WARNING: GUARDIAN", C_RED);
        }
        if (st_timer < 120 || !intro_round) gfx_text_center(144, "READY", C_WHITE);
    } else if (state == ST_PLAY && paused) {
        gfx_dim(FIELD_L, FIELD_T, FIELD_R - FIELD_L, SCR_H - FIELD_T);
        gfx_text_center(130, "PAUSE", C_WHITE);
    } else if (state == ST_GAMEOVER) {
        gfx_text_center(140, "GAME OVER", C_WHITE);
    } else if (state == ST_CONTINUE) {
        gfx_dim(FIELD_L, FIELD_T, FIELD_R - FIELD_L, SCR_H - FIELD_T);
        gfx_text_center(110, "CONTINUE?", C_WHITE);
        sprintf(buf, "%d", st_timer / 60);
        gfx_text_big(128 - 5, 126, buf, 2, logo_grad, 7, 0x701020);
        gfx_text_center(160, "PUSH START", C_YELL);
    }
    if (demo && (frame / 32) % 2) gfx_text_center(170, "DEMO PLAY", C_YELL);
}

void game_init(void)
{
    gfx_init();
    doh_build();
    ms_build();
    en_build();
    hiscore_load();
    rng_state ^= (unsigned)time(NULL);
    to_title();
    story_start();
}
