module;

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_gamecontroller.h>

export module xmbshell.input;

export namespace input {
    class keyboard_handler {
        public:
            virtual void key_down(SDL_Keysym key) {};
            virtual void key_up(SDL_Keysym key) {};
    };

    class controller_handler {
        public:
            virtual void add_controller(SDL_GameController* controller) {};
            virtual void remove_controller(SDL_GameController* controller) {};

            virtual void button_down(SDL_GameController* controller, SDL_GameControllerButton button) {};
            virtual void button_up(SDL_GameController* controller, SDL_GameControllerButton button) {};
            virtual void axis_motion(SDL_GameController* controller, SDL_GameControllerAxis axis, Sint16 value) {};
    };
}
