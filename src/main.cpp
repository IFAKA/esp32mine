#include "game.h"
#include "platform_activation.h"
#include <SDL3/SDL.h>
#include <cstdio>

int main(int, char**) {
    // SDL3 otherwise permits a terminal-launched process to remain the
    // background macOS application even when its window is above other apps.
    SDL_SetHint(SDL_HINT_MAC_BACKGROUND_APP, "0");
    if (!SDL_Init(SDL_INIT_VIDEO)) { std::fprintf(stderr, "SDL init failed: %s\n", SDL_GetError()); return 1; }
    SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, "1");
    SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED, "1");
    SDL_Window* window = SDL_CreateWindow("ESP32 Mine | WASD move, mouse look, Space jump, LMB break, RMB place, 1-4 blocks", 960, 540, SDL_WINDOW_ALWAYS_ON_TOP);
    if (!window) { std::fprintf(stderr, "SDL window setup failed: %s\n", SDL_GetError()); SDL_Quit(); return 1; }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, mine::ScreenWidth, mine::ScreenHeight);
    if (!renderer || !texture) { std::fprintf(stderr, "SDL renderer setup failed: %s\n", SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return 1; }
    SDL_ShowWindow(window);
    SDL_SetWindowFocusable(window, true);
    SDL_SetWindowAlwaysOnTop(window, true);
    SDL_RaiseWindow(window);
    activateGameApplication();
    SDL_SetWindowKeyboardGrab(window, true);
    SDL_SetWindowRelativeMouseMode(window, true);
    mine::Game game; uint16_t framebuffer[mine::ScreenWidth * mine::ScreenHeight]{}; bool running=true; Uint64 previous=SDL_GetTicks();
    Uint64 fpsClock = previous; int fpsFrames = 0; int fps = 0;
    int activationFrames = 120;
    while(running) {
        if (activationFrames > 0) {
            activateGameApplication();
            SDL_RaiseWindow(window);
            --activationFrames;
        }
        mine::Input input; SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type==SDL_EVENT_QUIT) running=false;
            if(event.type==SDL_EVENT_KEY_DOWN && !event.key.repeat) { if(event.key.key==SDLK_ESCAPE) running=false; if(event.key.key==SDLK_SPACE) input.jumpPressed=true; for(int i=0;i<4;++i) if(event.key.key==SDLK_1+i) input.selectPressed[i]=true; }
            if(event.type==SDL_EVENT_MOUSE_MOTION) { input.mouseDx += event.motion.xrel; input.mouseDy += event.motion.yrel; }
            if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN) { SDL_RaiseWindow(window); SDL_SetWindowRelativeMouseMode(window, true); if(event.button.button==SDL_BUTTON_LEFT) input.breakPressed=true; if(event.button.button==SDL_BUTTON_RIGHT) input.placePressed=true; }
        }
        const bool* keys=SDL_GetKeyboardState(nullptr); input.forward=keys[SDL_SCANCODE_W]; input.backward=keys[SDL_SCANCODE_S]; input.left=keys[SDL_SCANCODE_A]; input.right=keys[SDL_SCANCODE_D];
        // Use current device state as well as edge events. This keeps gameplay
        // controls reliable when macOS/tmux drops an individual key event.
        input.jumpPressed = input.jumpPressed || keys[SDL_SCANCODE_SPACE];
        input.selectPressed[0] = input.selectPressed[0] || keys[SDL_SCANCODE_1];
        input.selectPressed[1] = input.selectPressed[1] || keys[SDL_SCANCODE_2];
        input.selectPressed[2] = input.selectPressed[2] || keys[SDL_SCANCODE_3];
        input.selectPressed[3] = input.selectPressed[3] || keys[SDL_SCANCODE_4];
        Uint32 mouseButtons = SDL_GetMouseState(nullptr, nullptr);
        input.breakPressed = input.breakPressed || (mouseButtons & SDL_BUTTON_LMASK) != 0;
        input.placePressed = input.placePressed || (mouseButtons & SDL_BUTTON_RMASK) != 0;
        Uint64 now=SDL_GetTicks(); float dt=static_cast<float>(now-previous)/1000.0f; previous=now;
        game.update(input,dt); game.render(framebuffer);
        ++fpsFrames; now=SDL_GetTicks();
        if (now - fpsClock >= 500) { fps = static_cast<int>(fpsFrames * 1000 / (now - fpsClock)); fpsFrames=0; fpsClock=now; }
        mine::drawFps(framebuffer, fps);
        SDL_UpdateTexture(texture,nullptr,framebuffer,mine::ScreenWidth*sizeof(uint16_t)); SDL_RenderClear(renderer); SDL_RenderTexture(renderer,texture,nullptr,nullptr); SDL_RenderPresent(renderer);
    }
    SDL_DestroyTexture(texture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 0;
}
