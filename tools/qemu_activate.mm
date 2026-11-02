#import <ApplicationServices/ApplicationServices.h>

#include <cerrno>
#include <cstdlib>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    char* end = nullptr;
    const long value = std::strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || value <= 0) return 2;

    AXUIElementRef application = AXUIElementCreateApplication(static_cast<pid_t>(value));
    if (!application) return 1;

    AXError frontmost = AXUIElementSetAttributeValue(
        application, kAXFrontmostAttribute, kCFBooleanTrue);

    CFTypeRef windows = nullptr;
    AXError copied = AXUIElementCopyAttributeValue(
        application, kAXWindowsAttribute, &windows);
    if (copied == kAXErrorSuccess && windows && CFGetTypeID(windows) == CFArrayGetTypeID()) {
        CFArrayRef windowArray = static_cast<CFArrayRef>(windows);
        for (CFIndex index = 0; index < CFArrayGetCount(windowArray); ++index) {
            auto window = static_cast<AXUIElementRef>(
                const_cast<void*>(CFArrayGetValueAtIndex(windowArray, index)));
            AXUIElementPerformAction(window, kAXRaiseAction);
        }
    }
    if (windows) CFRelease(windows);
    CFRelease(application);
    return frontmost == kAXErrorSuccess ? 0 : 1;
}
