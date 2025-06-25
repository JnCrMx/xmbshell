module;

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <variant>

export module xmbshell.app:text_viewer;

import dreamrender;
import glm;
import spdlog;
import vulkan_hpp;
import xmbshell.utils;
import :component;
import :programs;

namespace programs {

using namespace app;

export class text_viewer : public component, public action_receiver, public joystick_receiver, public mouse_receiver {
    public:
        text_viewer(std::filesystem::path path, dreamrender::resource_loader& loader) : title(path.string()), src(std::move(path)) {
            text = "Test!";
            lines = std::ranges::count(text, '\n') + 1;
            calculate_offsets();
        }
        text_viewer(std::string title, std::string data) : title(std::move(title)), src(std::move(data)) {
            text = std::get<std::string>(src);
            lines = std::ranges::count(text, '\n') + 1;
            calculate_offsets();
        }
        text_viewer(std::string title, std::string_view data) : title(std::move(title)), src(data) {
            text = std::get<std::string_view>(src);
            lines = std::ranges::count(text, '\n') + 1;
            calculate_offsets();
        }

        void render(dreamrender::gui_renderer& renderer, class xmbshell* xmb) override {
            constexpr float x = (1.0f - width) / 2;
            constexpr float y = (1.0f - height) / 2;
            const double offset_x = 0.01;
            const double offset_y = 0.01 * renderer.aspect_ratio;

            renderer.draw_rect(glm::vec2{x - offset_x, y - offset_y}, glm::vec2{width + 2*offset_x, height + 2*offset_y},
                glm::vec4{0.1f, 0.1f, 0.1f, 0.5f});

            renderer.set_clip(x, y, width, height);
            if(!text.empty()) {
                std::string_view part = text.substr(begin_offset, end_offset - begin_offset);
                renderer.draw_text(part, x, y, font_size);
            }
            renderer.reset_clip();
        }

        result on_action(action action) override {
            if(action == action::cancel) {
                return result::close;
            } else if(action == action::up) {
                if(current_line > 0) {
                    current_line -= 1;
                    calculate_offsets();
                }
                return result::success;
            } else if(action == action::down) {
                if(current_line < lines - 1) {
                    current_line += 1;
                    calculate_offsets();
                }
                return result::success;
            }
            return result::failure;
        };
    private:
        static constexpr float width = 0.6f;
        static constexpr float height = 0.6f;
        static constexpr float font_size = 0.05f;
        static constexpr int rendered_lines = 2*height / font_size;

        std::string title;
        std::variant<std::filesystem::path, std::string, std::string_view> src;
        std::string_view text;
        unsigned int current_line = 0;
        unsigned int lines = 0;

        unsigned int begin_offset = 0;
        unsigned int end_offset = 0;

        void calculate_offsets() {
            begin_offset = 0;
            end_offset = 0;

            unsigned int l = 0;
            for(unsigned int i=0; i<text.size(); ++i) {
                if(l == current_line) {
                    begin_offset = i;
                }
                if(l == current_line + rendered_lines) {
                    end_offset = i;
                    break;
                }
                if(text[i] == '\n') {
                    l += 1;
                }
            }
            if(end_offset <= begin_offset) {
                end_offset = text.size();
            }
        }
};

namespace {
const inline register_program<text_viewer> text_viewer_program{
    "text_viewer",
    {
        "text/plain", "application/json", "text/x-shellscript", "text/x-c++"
    }
};
}

}
