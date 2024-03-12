#pragma once

#include "menu/menu.hpp"
#include "render/gui_renderer.hpp"
#include "render/resource_loader.hpp"
#include "input.hpp"

#include <SDL_mixer.h>

#include <optional>

namespace app {

struct Mix_Chunk_Freer {
    void operator()(Mix_Chunk* chunk) {
        Mix_FreeChunk(chunk);
    }
};
using SoundChunk = std::unique_ptr<Mix_Chunk, Mix_Chunk_Freer>;

class main_menu : input::keyboard_handler, input::controller_handler {
    public:
        main_menu();
        void preload(vk::Device device, vma::Allocator allocator, render::resource_loader& loader);
        void tick();
        void render(render::gui_renderer& renderer);

        void key_down(SDL_Keysym key) override;
        void key_up(SDL_Keysym key) override;

        void button_down(SDL_GameController* controller, SDL_GameControllerButton button) override;
        void button_up(SDL_GameController* controller, SDL_GameControllerButton button) override;
        void axis_motion(SDL_GameController* controller, SDL_GameControllerAxis axis, Sint16 value) override;
    private:
        enum class direction {
            left,
            right,
            up,
            down
        };

        void select(int index);
        void select_submenu(int index);
        bool select_relative(direction dir);
        bool activate_current();
        void error_rumble(SDL_GameController* controller);

        SoundChunk ok_sound;

        std::vector<std::unique_ptr<menu::menu>> menus;
        int selected = 0;

        int last_selected = 0;
        std::chrono::time_point<std::chrono::system_clock> last_selected_transition;
        constexpr static auto transition_duration = std::chrono::milliseconds(200);

        int last_selected_submenu = 0;
        std::chrono::time_point<std::chrono::system_clock> last_selected_submenu_transition;
        constexpr static auto transition_submenu_duration = std::chrono::milliseconds(200);

        constexpr static int controller_axis_input_threshold = 10000;
        std::chrono::time_point<std::chrono::system_clock> last_controller_axis_input_time[2];
        std::optional<std::tuple<SDL_GameController*, direction>> last_controller_axis_input[2];
        constexpr static auto controller_axis_input_duration = std::chrono::milliseconds(200);

        std::chrono::time_point<std::chrono::system_clock> last_controller_button_input_time;
        std::optional<std::tuple<SDL_GameController*, SDL_GameControllerButton>> last_controller_button_input;
        constexpr static auto controller_button_input_duration = std::chrono::milliseconds(200);
};

}
