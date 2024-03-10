#pragma once

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_gamecontroller.h>

namespace input {
    class keyboard_handler {
        public:
            virtual void key_down(SDL_Keysym key) = 0;
            virtual void key_up(SDL_Keysym key) = 0;
    };

    class controller_handler {
        public:
            virtual void add_controller(SDL_GameController* controller) = 0;
            virtual void remove_controller(SDL_GameController* controller) = 0;

            virtual void button_down(SDL_GameController* controller, SDL_GameControllerButton button) = 0;
            virtual void button_up(SDL_GameController* controller, SDL_GameControllerButton button) = 0;
            virtual void axis_motion(SDL_GameController* controller, SDL_GameControllerAxis axis, Sint16 value) = 0;
    };
}
