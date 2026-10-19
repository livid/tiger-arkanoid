#import <Cocoa/Cocoa.h>

@interface GameView : NSOpenGLView
{
    NSTimer *timer;
    unsigned int tex;
    BOOL glReady;
    unsigned held, pressed;
    int mouseMoved;
    float mouseX;
    double lastTime, accum;
    BOOL frameDirty;
    NSCursor *blankCursor;
}
- (void)lostFocus;
@end
