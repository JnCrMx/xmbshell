module;

#include <chrono>
#include <memory>
#include <optional>

export module xmbshell.app:main_menu;

import xmbshell.menu;
import dreamrender;
import sdl2;
import vulkan_hpp;
import vma;

namespace app {

class main_menu : input::keyboard_handler, input::controller_handler {
    public:
        main_menu(class xmbshell* shell);
        void preload(vk::Device device, vma::Allocator allocator, dreamrender::resource_loader& loader);
        void tick();
        void render(dreamrender::gui_renderer& renderer);

        void key_down(sdl::Keysym key) override;
        void key_up(sdl::Keysym key) override;

        void button_down(sdl::GameController* controller, sdl::GameControllerButton button) override;
        void button_up(sdl::GameController* controller, sdl::GameControllerButton button) override;
        void axis_motion(sdl::GameController* controller, sdl::GameControllerAxis axis, int16_t value) override;
    private:
        using time_point = std::chrono::time_point<std::chrono::system_clock>;

        class xmbshell* shell;

        void render_crossbar(dreamrender::gui_renderer& renderer, time_point now);
        void render_submenu(dreamrender::gui_renderer& renderer, time_point now);

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
        void error_rumble(sdl::GameController* controller);

        sdl::mix::unique_chunk ok_sound;

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
        std::optional<std::tuple<sdl::GameController*, direction>> last_controller_axis_input[2];
        constexpr static auto controller_axis_input_duration = std::chrono::milliseconds(200);

        time_point last_controller_button_input_time;
        std::optional<std::tuple<sdl::GameController*, sdl::GameControllerButton>> last_controller_button_input;
        constexpr static auto controller_button_input_duration = std::chrono::milliseconds(200);
};

}
