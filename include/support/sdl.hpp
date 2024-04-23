#pragma once

#include <SDL.h>
#include <memory>

namespace sdl {

struct initializer {
    initializer() {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS |
            SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_JOYSTICK |
            SDL_INIT_AUDIO);
    }
    ~initializer() {
        SDL_Quit();
    }
};


struct window_deleter {
    void operator()(SDL_Window* ptr) const {
        SDL_DestroyWindow(ptr);
    }
};
using unique_window = std::unique_ptr<SDL_Window, window_deleter>;

struct surface_deleter {
    void operator()(SDL_Surface* ptr) const {
        SDL_FreeSurface(ptr);
    }
};
using unique_surface = std::unique_ptr<SDL_Surface, surface_deleter>;

struct surface_lock {
    SDL_Surface* surface;
    surface_lock(SDL_Surface* surface) : surface(surface) {
        SDL_LockSurface(surface);
    }
    ~surface_lock() {
        SDL_UnlockSurface(surface);
    }
    void* pixels() {
        return surface->pixels;
    }
};

}
