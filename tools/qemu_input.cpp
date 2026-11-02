#include <SDL3/SDL.h>

#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
constexpr uint8_t Sync = 0xA5;
constexpr int Port = 5556;

struct __attribute__((packed)) Packet {
    uint8_t sync;
    int16_t mouseDx;
    int16_t mouseDy;
    uint16_t buttons;
    uint8_t select;
    uint8_t reserved;
};

int connectToQemu() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(Port);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    if (connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}
}

int main() {
    SDL_SetHint(SDL_HINT_MAC_BACKGROUND_APP, "0");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "QEMU input SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow(
        "ESP32-S3 QEMU controls | KEEP THIS WINDOW FOCUSED | WASD move, mouse look, Space jump, LMB break, RMB place, 1-4 blocks",
        620, 82, 0);
    if (!window) {
        std::fprintf(stderr, "QEMU input window failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    // QEMU opens its game window centered; keep the input panel in a separate
    // corner so it is always visible and clickable.
    SDL_SetWindowPosition(window, 60, 60);
    SDL_ShowWindow(window);
    SDL_RaiseWindow(window);
    SDL_SetWindowAlwaysOnTop(window, true);
    // The QEMU display is a separate application. Once the player clicks
    // this panel, keep both devices routed here even while the pointer is
    // captured for look input.
    SDL_SetWindowKeyboardGrab(window, true);
    SDL_CaptureMouse(true);
    SDL_SetWindowRelativeMouseMode(window, true);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer) {
        SDL_SetRenderDrawColor(renderer, 24, 31, 43, 255);
        SDL_RenderClear(renderer);
        SDL_FRect accent{18.0f, 22.0f, 584.0f, 38.0f};
        SDL_SetRenderDrawColor(renderer, 56, 189, 138, 255);
        SDL_RenderFillRect(renderer, &accent);
        SDL_SetRenderDrawColor(renderer, 220, 240, 232, 255);
        SDL_FRect status{30.0f, 32.0f, 12.0f, 18.0f};
        SDL_RenderFillRect(renderer, &status);
        SDL_RenderPresent(renderer);
    }
    int fd = -1;
    bool running = true;
    bool keyW = false;
    bool keyS = false;
    bool keyA = false;
    bool keyD = false;
    bool keySpace = false;
    bool keySlots[4] = {};
    while (running) {
        SDL_Event event;
        int mouseDx = 0;
        int mouseDy = 0;
        bool jump = false;
        bool breakBlock = false;
        bool placeBlock = false;
        uint8_t select = 0;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
                SDL_SetWindowKeyboardGrab(window, true);
                SDL_CaptureMouse(true);
                SDL_SetWindowRelativeMouseMode(window, true);
            }
            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                if (event.key.key == SDLK_ESCAPE) running = false;
                if (event.key.key == SDLK_W) keyW = true;
                if (event.key.key == SDLK_S) keyS = true;
                if (event.key.key == SDLK_A) keyA = true;
                if (event.key.key == SDLK_D) keyD = true;
                if (event.key.key == SDLK_SPACE) {
                    keySpace = true;
                    jump = true;
                }
                for (int slot = 0; slot < 4; ++slot)
                    if (event.key.key == SDLK_1 + slot) {
                        keySlots[slot] = true;
                        select |= static_cast<uint8_t>(1u << slot);
                    }
            }
            if (event.type == SDL_EVENT_KEY_UP) {
                if (event.key.key == SDLK_W) keyW = false;
                if (event.key.key == SDLK_S) keyS = false;
                if (event.key.key == SDLK_A) keyA = false;
                if (event.key.key == SDLK_D) keyD = false;
                if (event.key.key == SDLK_SPACE) keySpace = false;
                for (int slot = 0; slot < 4; ++slot)
                    if (event.key.key == SDLK_1 + slot) keySlots[slot] = false;
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                mouseDx += static_cast<int>(event.motion.xrel);
                mouseDy += static_cast<int>(event.motion.yrel);
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                SDL_RaiseWindow(window);
                SDL_SetWindowKeyboardGrab(window, true);
                SDL_CaptureMouse(true);
                SDL_SetWindowRelativeMouseMode(window, true);
                if (event.button.button == SDL_BUTTON_LEFT) breakBlock = true;
                if (event.button.button == SDL_BUTTON_RIGHT) placeBlock = true;
            }
        }
        uint16_t buttons = 0;
        if (keyW) buttons |= 1u << 0;
        if (keyS) buttons |= 1u << 1;
        if (keyA) buttons |= 1u << 2;
        if (keyD) buttons |= 1u << 3;
        if (keySpace) buttons |= 1u << 4;
        uint32_t mouseButtons = SDL_GetMouseState(nullptr, nullptr);
        if (mouseButtons & SDL_BUTTON_LMASK) buttons |= 1u << 5;
        if (mouseButtons & SDL_BUTTON_RMASK) buttons |= 1u << 6;
        if (jump) buttons |= 1u << 4;
        if (breakBlock) buttons |= 1u << 5;
        if (placeBlock) buttons |= 1u << 6;
        for (int slot = 0; slot < 4; ++slot)
            if (keySlots[slot]) select |= static_cast<uint8_t>(1u << slot);

        if (fd < 0) {
            fd = connectToQemu();
            if (fd >= 0) std::fprintf(stderr, "QEMU input bridge connected on TCP %d\n", Port);
        }
        if (fd >= 0) {
            Packet packet{Sync, static_cast<int16_t>(mouseDx), static_cast<int16_t>(mouseDy), buttons, select, 0};
            ssize_t sent = send(fd, &packet, sizeof(packet), MSG_NOSIGNAL);
            if (sent != static_cast<ssize_t>(sizeof(packet))) {
                std::fprintf(stderr, "QEMU input bridge disconnected; retrying\n");
                close(fd);
                fd = -1;
            }
            if (fd >= 0) {
                char discard[256];
                recv(fd, discard, sizeof(discard), MSG_DONTWAIT);
            }
        }
        SDL_Delay(16);
    }
    if (fd >= 0) close(fd);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
