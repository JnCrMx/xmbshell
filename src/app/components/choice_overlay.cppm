module;

#include <chrono>
#include <functional>
#include <string>
#include <vector>

export module xmbshell.app:choice_overlay;

import dreamrender;
import sdl2;
import vulkan_hpp;
import vma;
import xmbshell.utils;

namespace app {

export class choice_overlay : public action_receiver {
    public:
        choice_overlay(std::vector<std::string> choices, unsigned int selection_index = 0,
            std::function<void(unsigned int)> confirm_callback = [](unsigned int){},
            std::function<void()> cancel_callback = [](){}
        );

        void render(dreamrender::gui_renderer& renderer);
        result on_action(action action) override;
    private:
        using time_point = std::chrono::time_point<std::chrono::system_clock>;

        std::vector<std::string> choices;
        std::function<void(unsigned int)> confirm_callback;
        std::function<void()> cancel_callback;

        bool select_relative(action dir);

        unsigned int selection_index = 0;
        unsigned int last_selection_index = 0;
        time_point last_selection_time = std::chrono::system_clock::now();

        constexpr static auto transition_duration = std::chrono::milliseconds(100);
};

}
