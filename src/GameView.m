#import "GameView.h"
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#include "game.h"
#include "gfx.h"

#define STEP (1.0 / 60.0)

@implementation GameView

- (id)initWithFrame:(NSRect)frame
{
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFADoubleBuffer, NSOpenGLPFAAccelerated, NSOpenGLPFAColorSize, 24, 0
    };
    NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (!pf) {
        attrs[1] = NSOpenGLPFAColorSize;  /* fall back to a software renderer */
        attrs[2] = 24;
        attrs[3] = 0;
        pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    }
    self = [super initWithFrame:frame pixelFormat:[pf autorelease]];
    if (self) {
        NSImage *img = [[NSImage alloc] initWithSize:NSMakeSize(16, 16)];
        [img lockFocus];
        [[NSColor clearColor] set];
        NSRectFillUsingOperation(NSMakeRect(0, 0, 16, 16), NSCompositeClear);
        [img unlockFocus];
        blankCursor = [[NSCursor alloc] initWithImage:img hotSpot:NSMakePoint(0, 0)];
        [img release];

        game_init();
        lastTime = [NSDate timeIntervalSinceReferenceDate];
        timer = [[NSTimer timerWithTimeInterval:1.0 / 120.0 target:self
                                       selector:@selector(tick:) userInfo:nil repeats:YES] retain];
        [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
        [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSEventTrackingRunLoopMode];
    }
    return self;
}

- (void)dealloc
{
    [timer invalidate];
    [timer release];
    [blankCursor release];
    [super dealloc];
}

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)isOpaque { return YES; }

- (void)resetCursorRects
{
    [self addCursorRect:[self bounds] cursor:blankCursor];
}

- (void)setupGL
{
    long one = 1;
    [[self openGLContext] makeCurrentContext];
    [[self openGLContext] setValues:&one forParameter:NSOpenGLCPSwapInterval];
    glGenTextures(1, (GLuint *)&tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 256, 256, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DITHER);
    glReady = YES;
}

/* Largest 256:224 rectangle centred in the view. */
- (NSRect)viewport
{
    NSRect b = [self bounds];
    float s = b.size.width / SCR_W;
    float w, h;
    if (b.size.height / SCR_H < s) s = b.size.height / SCR_H;
    w = SCR_W * s;
    h = SCR_H * s;
    return NSMakeRect(floorf((b.size.width - w) / 2), floorf((b.size.height - h) / 2), w, h);
}

- (void)drawRect:(NSRect)rect
{
    NSRect b = [self bounds], vp = [self viewport];
    if (!glReady) [self setupGL];
    [[self openGLContext] makeCurrentContext];

    glViewport(0, 0, (GLsizei)b.size.width, (GLsizei)b.size.height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport((GLint)vp.origin.x, (GLint)vp.origin.y, (GLsizei)vp.size.width, (GLsizei)vp.size.height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_W, SCR_H, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, fb);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);                         glVertex2f(-1, 1);
    glTexCoord2f(1, 0);                         glVertex2f(1, 1);
    glTexCoord2f(1, SCR_H / 256.0f);            glVertex2f(1, -1);
    glTexCoord2f(0, SCR_H / 256.0f);            glVertex2f(-1, -1);
    glEnd();
    [[self openGLContext] flushBuffer];
}

- (void)tick:(NSTimer *)t
{
    double now = [NSDate timeIntervalSinceReferenceDate];
    int steps = 0;
    accum += now - lastTime;
    lastTime = now;
    if (accum > 0.1) accum = 0.1;
    while (accum >= STEP) {
        GameInput in;
        in.held = held;
        in.pressed = pressed;
        in.mouse_moved = mouseMoved;
        in.mouse_x = mouseX;
        game_update(&in);
        pressed = 0;
        mouseMoved = 0;
        accum -= STEP;
        steps++;
    }
    if (steps) {
        game_render();
        [self setNeedsDisplay:YES];
    }
}

static unsigned keyBits(NSEvent *e)
{
    NSString *s = [e charactersIgnoringModifiers];
    unichar c;
    if ([s length] == 0) return 0;
    c = [s characterAtIndex:0];
    switch (c) {
    case NSLeftArrowFunctionKey:  return BTN_LEFT;
    case NSRightArrowFunctionKey: return BTN_RIGHT;
    case NSUpArrowFunctionKey:    return BTN_UP;
    case NSDownArrowFunctionKey:  return BTN_DOWN;
    case ' ': case 'z': case 'Z': case 'x': case 'X': return BTN_FIRE;
    case '\r': case 3:            return BTN_START;
    case 'p': case 'P':           return BTN_PAUSE;
    case 27:                      return BTN_ESC;
    }
    return 0;
}

- (void)keyDown:(NSEvent *)e
{
    unsigned b = keyBits(e);
    if (!b) { [super keyDown:e]; return; }
    if (![e isARepeat]) pressed |= b;
    held |= b;
}

- (void)keyUp:(NSEvent *)e
{
    held &= ~keyBits(e);
}

- (void)updateMouse:(NSEvent *)e
{
    NSPoint p = [self convertPoint:[e locationInWindow] fromView:nil];
    NSRect vp = [self viewport];
    mouseX = (p.x - vp.origin.x) / vp.size.width * SCR_W;
    mouseMoved = 1;
}

- (void)mouseMoved:(NSEvent *)e   { [self updateMouse:e]; }
- (void)mouseDragged:(NSEvent *)e { [self updateMouse:e]; }

- (void)mouseDown:(NSEvent *)e
{
    pressed |= BTN_FIRE;
    held |= BTN_FIRE;
}

- (void)mouseUp:(NSEvent *)e
{
    held &= ~BTN_FIRE;
}

- (void)lostFocus
{
    held = 0;
    game_focus_lost();
}

@end
