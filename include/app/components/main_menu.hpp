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
        main_menu(class xmbshell* shell);
        void preload(vk::Device device, vma::Allocator allocator, render::resource_loader& loader);
        void tick();
        void render(render::gui_renderer& renderer);

        void key_down(SDL_Keysym key) override;
        void key_up(SDL_Keysym key) override;

        void button_down(SDL_GameController* controller, SDL_GameControllerButton button) override;
        void button_up(SDL_GameController* controller, SDL_GameControllerButton button) override;
        void axis_motion(SDL_GameController* controller, SDL_GameControllerAxis axis, Sint16 value) override;
    private:
        using time_point = std::chrono::time_point<std::chrono::system_clock>;

        class xmbshell* shell;

        void render_crossbar(render::gui_renderer& renderer, time_point now);
        void render_submenu(render::gui_renderer& renderer, time_point now);

        enum class direction {
            left,
            right,
            up,
            down
        };

        void select(int index);
        void select_menu_item(int index);
        void select_submenu_item(int index);
        bool select_relative(direction dir);
        bool activate_current();
        bool back();
        void error_rumble(SDL_GameController* controller);

        SoundChunk ok_sound;

        std::vector<std::unique_ptr<menu::menu>> menus;
        int selected = 0;

        int last_selected = 0;
        time_point last_selected_transition;
        constexpr static auto transition_duration = std::chrono::milliseconds(200);

        int last_selected_menu_item = 0;
        time_point last_selected_menu_item_transition;
        constexpr static auto transition_menu_item_duration = std::chrono::milliseconds(200);

        bool in_submenu = false;
        menu::menu* current_submenu = nullptr;
        time_point last_submenu_transition;
        constexpr static auto transition_submenu_activate_duration = std::chrono::milliseconds(100);

        int last_selected_submenu_item;
        time_point last_selected_submenu_item_transition;
        constexpr static auto transition_submenu_item_duration = std::chrono::milliseconds(100);

        constexpr static int controller_axis_input_threshold = 10000;
        time_point last_controller_axis_input_time[2];
        std::optional<std::tuple<SDL_GameController*, direction>> last_controller_axis_input[2];
        constexpr static auto controller_axis_input_duration = std::chrono::milliseconds(200);

        time_point last_controller_button_input_time;
        std::optional<std::tuple<SDL_GameController*, SDL_GameControllerButton>> last_controller_button_input;
        constexpr static auto controller_button_input_duration = std::chrono::milliseconds(200);
};

}
