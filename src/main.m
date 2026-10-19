#import <Cocoa/Cocoa.h>
#import "GameView.h"
#include "sound.h"

@interface AppDelegate : NSObject
{
    NSWindow *window;
    GameView *view;
}
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)n
{
    NSRect vis = [[NSScreen mainScreen] visibleFrame];
    int scale = 3;
    NSRect frame;
    while (scale > 1 && (256 * scale > vis.size.width - 16 || 224 * scale + 24 > vis.size.height - 16))
        scale--;
    frame = NSMakeRect(0, 0, 256 * scale, 224 * scale);

    snd_init();
    window = [[NSWindow alloc] initWithContentRect:frame
                                         styleMask:NSTitledWindowMask | NSClosableWindowMask |
                                                   NSMiniaturizableWindowMask | NSResizableWindowMask
                                           backing:NSBackingStoreBuffered
                                             defer:NO];
    [window setTitle:@"Arkanoid"];
    [window setContentMinSize:NSMakeSize(256, 224)];
    [window setContentAspectRatio:NSMakeSize(256, 224)];
    [window setAcceptsMouseMovedEvents:YES];
    [window setDelegate:self];
    view = [[GameView alloc] initWithFrame:frame];
    [window setContentView:view];
    [window center];
    [window makeKeyAndOrderFront:nil];
    [window makeFirstResponder:view];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app
{
    return YES;
}

- (void)windowDidResignKey:(NSNotification *)n
{
    [view lostFocus];
}

@end

static void buildMenus(void)
{
    NSMenu *bar = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem *appItem = [[NSMenuItem alloc] initWithTitle:@"" action:NULL keyEquivalent:@""];
    NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"Arkanoid"];
    NSMenuItem *winItem = [[NSMenuItem alloc] initWithTitle:@"" action:NULL keyEquivalent:@""];
    NSMenu *winMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    NSMenuItem *it;

    [appMenu addItemWithTitle:@"About Arkanoid" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Hide Arkanoid" action:@selector(hide:) keyEquivalent:@"h"];
    it = [appMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
    [it setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
    [appMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit Arkanoid" action:@selector(terminate:) keyEquivalent:@"q"];
    [appItem setSubmenu:appMenu];
    [bar addItem:appItem];

    [winMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
    [winMenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];
    [winItem setSubmenu:winMenu];
    [bar addItem:winItem];

    [NSApp setMainMenu:bar];
    /* -setAppleMenu: is not in the public 10.4 headers but is required for a nib-less app menu. */
    [NSApp performSelector:@selector(setAppleMenu:) withObject:appMenu];
    [NSApp setWindowsMenu:winMenu];
}

int main(int argc, char *argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [NSApplication sharedApplication];
    buildMenus();
    [NSApp setDelegate:[[AppDelegate alloc] init]];
    [NSApp run];
    [pool release];
    return 0;
}
