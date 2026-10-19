/* Tiny 4-channel chip-style synth on the default CoreAudio output unit. */
#include <AudioUnit/AudioUnit.h>
#include <CoreServices/CoreServices.h>
#include <math.h>
#include "sound.h"

#define SR 44100.0f

enum { W_SQ, W_SQ25, W_TRI, W_NOISE };

typedef struct { float f0, f1, ms, vol; int wave; } Note;

#define END {0, 0, 0, 0, 0}

static const Note s_paddle[]  = { {330, 330, 45, .30f, W_SQ}, END };
static const Note s_brick[]   = { {660, 660, 45, .25f, W_SQ}, END };
static const Note s_metal[]   = { {1760, 1600, 50, .22f, W_SQ25}, {2093, 2093, 70, .15f, W_SQ25}, END };
static const Note s_laser[]   = { {1500, 300, 80, .18f, W_SQ25}, END };
static const Note s_capsule[] = { {523, 523, 50, .25f, W_SQ25}, {659, 659, 50, .25f, W_SQ25},
                                  {784, 784, 50, .25f, W_SQ25}, {1047, 1047, 90, .25f, W_SQ25}, END };
static const Note s_enlarge[] = { {150, 900, 280, .25f, W_SQ}, END };
static const Note s_die[]     = { {900, 700, 80, .30f, W_SQ}, {700, 60, 600, .30f, W_SQ},
                                  {3000, 300, 400, .25f, W_NOISE}, END };
static const Note s_enemy[]   = { {6000, 400, 220, .30f, W_NOISE}, END };
static const Note s_start[]   = { {392, 392, 110, .25f, W_SQ25}, {523, 523, 110, .25f, W_SQ25},
                                  {659, 659, 110, .25f, W_SQ25}, {784, 784, 220, .25f, W_SQ25},
                                  {0, 0, 50, 0, W_SQ}, {659, 659, 110, .25f, W_SQ25},
                                  {784, 784, 110, .25f, W_SQ25}, {1047, 1047, 400, .25f, W_SQ25}, END };
static const Note s_life[]    = { {1047, 1047, 60, .22f, W_SQ}, {1319, 1319, 60, .22f, W_SQ},
                                  {1568, 1568, 60, .22f, W_SQ}, {2093, 2093, 160, .22f, W_SQ}, END };
static const Note s_dohhit[]  = { {300, 150, 140, .35f, W_SQ}, END };
static const Note s_dohshot[] = { {900, 300, 180, .20f, W_SQ25}, END };
static const Note s_gameover[]= { {784, 784, 220, .30f, W_TRI}, {659, 659, 220, .30f, W_TRI},
                                  {523, 523, 220, .30f, W_TRI}, {392, 392, 220, .30f, W_TRI},
                                  {330, 330, 220, .30f, W_TRI}, {262, 262, 600, .30f, W_TRI}, END };
static const Note s_warp[]    = { {200, 2400, 700, .22f, W_SQ25}, END };
static const Note s_catch[]   = { {220, 220, 40, .25f, W_SQ}, END };
static const Note s_ending[]  = { {523, 523, 150, .25f, W_SQ25}, {659, 659, 150, .25f, W_SQ25},
                                  {784, 784, 150, .25f, W_SQ25}, {1047, 1047, 300, .25f, W_SQ25},
                                  {880, 880, 150, .25f, W_SQ25}, {1047, 1047, 150, .25f, W_SQ25},
                                  {1175, 1175, 150, .25f, W_SQ25}, {1319, 1319, 600, .25f, W_SQ25}, END };

static const Note *tables[SND_COUNT] = {
    s_paddle, s_brick, s_metal, s_laser, s_capsule, s_enlarge, s_die, s_enemy,
    s_start, s_life, s_dohhit, s_dohshot, s_gameover, s_warp, s_catch, s_ending
};
/* Each effect owns a channel; a new effect cuts off the old one on that channel. */
static const int chan_of[SND_COUNT] = { 0, 1, 1, 2, 3, 3, 3, 2, 3, 3, 1, 2, 3, 3, 0, 3 };

typedef struct {
    const Note *seq;
    int idx, pos, len;
    float phase;
    unsigned lfsr;
    float nval;
    const Note * volatile req;
    volatile int reqn;
    int ackn;
} Chan;

static Chan chans[4];
static AudioUnit unit;
static int snd_ok;

static void start_note(Chan *c)
{
    const Note *n = &c->seq[c->idx];
    if (n->ms <= 0) { c->seq = 0; return; }
    c->len = (int)(n->ms * SR / 1000.0f);
    c->pos = 0;
}

static float chan_sample(Chan *c)
{
    const Note *n;
    float t, f, v, env;
    if (!c->seq) return 0;
    n = &c->seq[c->idx];
    t = (float)c->pos / (float)c->len;
    f = n->f0 + (n->f1 - n->f0) * t;
    c->phase += f / SR;
    if (n->wave == W_NOISE) {
        while (c->phase >= 1.0f) {
            unsigned bit = ((c->lfsr >> 0) ^ (c->lfsr >> 1)) & 1;
            c->lfsr = (c->lfsr >> 1) | (bit << 14);
            c->nval = (c->lfsr & 1) ? 1.0f : -1.0f;
            c->phase -= 1.0f;
        }
        v = c->nval;
    } else {
        c->phase -= floorf(c->phase);
        if (n->wave == W_SQ) v = c->phase < 0.5f ? 1.0f : -1.0f;
        else if (n->wave == W_SQ25) v = c->phase < 0.25f ? 1.0f : -1.0f;
        else v = 4.0f * fabsf(c->phase - 0.5f) - 1.0f;
    }
    env = n->vol * (1.0f - 0.35f * t);
    if (c->pos < 60) env *= c->pos / 60.0f;
    if (c->len - c->pos < 120) env *= (c->len - c->pos) / 120.0f;
    if (n->f0 <= 0) env = 0;
    if (++c->pos >= c->len) {
        c->idx++;
        start_note(c);
    }
    return v * env;
}

static OSStatus render(void *ref, AudioUnitRenderActionFlags *flags,
                       const AudioTimeStamp *ts, UInt32 bus, UInt32 frames,
                       AudioBufferList *io)
{
    UInt32 i, k;
    float *L = (float *)io->mBuffers[0].mData;
    float *R = io->mNumberBuffers > 1 ? (float *)io->mBuffers[1].mData : 0;
    (void)ref; (void)flags; (void)ts; (void)bus;

    for (k = 0; k < 4; k++) {
        Chan *c = &chans[k];
        if (c->reqn != c->ackn) {
            c->ackn = c->reqn;
            c->seq = c->req;
            c->idx = 0;
            start_note(c);
        }
    }
    for (i = 0; i < frames; i++) {
        float s = 0;
        for (k = 0; k < 4; k++) s += chan_sample(&chans[k]);
        s *= 0.35f;
        if (s > 1) s = 1; else if (s < -1) s = -1;
        L[i] = s;
        if (R) R[i] = s;
    }
    return noErr;
}

void snd_init(void)
{
    ComponentDescription desc;
    Component comp;
    AURenderCallbackStruct cb;
    AudioStreamBasicDescription fmt;
    int k;

    for (k = 0; k < 4; k++) chans[k].lfsr = 0x4001;

    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    comp = FindNextComponent(NULL, &desc);
    if (!comp || OpenAComponent(comp, &unit) != noErr) return;

    cb.inputProc = render;
    cb.inputProcRefCon = NULL;
    if (AudioUnitSetProperty(unit, kAudioUnitProperty_SetRenderCallback,
                             kAudioUnitScope_Input, 0, &cb, sizeof cb) != noErr) return;

    fmt.mSampleRate = SR;
    fmt.mFormatID = kAudioFormatLinearPCM;
    fmt.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
    fmt.mBytesPerPacket = 4;
    fmt.mFramesPerPacket = 1;
    fmt.mBytesPerFrame = 4;
    fmt.mChannelsPerFrame = 2;
    fmt.mBitsPerChannel = 32;
    fmt.mReserved = 0;
    if (AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat,
                             kAudioUnitScope_Input, 0, &fmt, sizeof fmt) != noErr) return;
    if (AudioUnitInitialize(unit) != noErr) return;
    if (AudioOutputUnitStart(unit) != noErr) return;
    snd_ok = 1;
}

void snd_play(int id)
{
    Chan *c;
    if (!snd_ok || id < 0 || id >= SND_COUNT) return;
    c = &chans[chan_of[id]];
    c->req = tables[id];
    c->reqn++;
}
