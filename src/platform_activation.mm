#import <Cocoa/Cocoa.h>
#import <AppKit/NSRunningApplication.h>
#include "platform_activation.h"

void activateGameApplication() {
    NSApplication* application = [NSApplication sharedApplication];
    [application setActivationPolicy:NSApplicationActivationPolicyRegular];
    NSRunningApplication* process = [NSRunningApplication currentApplication];
    [process activateWithOptions:NSApplicationActivateAllWindows];
    [application activateIgnoringOtherApps:YES];
}
