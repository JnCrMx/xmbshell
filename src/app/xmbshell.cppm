module;

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

export module xmbshell.app:main;

import xmbshell.render;
import xmbshell.utils;
import dreamrender;
import glm;
import sdl2;
import vulkan_hpp;

import :choice_overlay;
import :main_menu;
import :message_overlay;
import :news_display;
import :progress_overlay;

namespace app
{
    export using clipboard = std::variant<
        std::string, std::function<std::string()>,
        std::function<bool(std::filesystem::path)>
    >;

    using namespace dreamrender;
    export class xmbshell : public phase, public input::keyboard_handler, public input::controller_handler
    {
        public:
            xmbshell(window* window);
            ~xmbshell();
            void preload() override;
            void prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews) override;
            void render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence) override;
            void tick();

            void key_up(sdl::Keysym key) override;
            void key_down(sdl::Keysym key) override;

            void add_controller(sdl::GameController* controller) override;
            void remove_controller(sdl::GameController* controller) override;
            void button_down(sdl::GameController* controller, sdl::GameControllerButton button) override;
            void button_up(sdl::GameController* controller, sdl::GameControllerButton button) override;
            void axis_motion(sdl::GameController* controller, sdl::GameControllerAxis axis, int16_t value) override;

            void reload_background();
            void reload_fonts();
            void reload_button_icons();

            void dispatch(action action);
            void handle(result result);

            std::string get_controller_type() const;
            void render_controller_buttons(gui_renderer& renderer, float x, float y, std::ranges::range auto buttons) {
                constexpr float min_width = 0.2f;
                constexpr float size = 0.05f;
                float size_x = static_cast<float>(size/renderer.aspect_ratio);
                float total_width = 0.0f;
                float last_width = 0.0f;
                for (const auto& [action, text] : buttons) {
                    last_width = size_x/1.25f+renderer.measure_text(text, size).x;
                    total_width += std::max(min_width, last_width);
                }
                if(last_width < min_width) {
                    total_width -= (min_width - last_width);
                }

                float current_x = x - total_width/2;
                for (const auto& [action, text] : buttons) {
                    auto icon = buttonTextures[std::to_underlying(action)].get();
                    float width = std::max(min_width, size_x/1.25f+renderer.measure_text(text, size).x);
                    if(action != action::none && icon) {
                        renderer.draw_image(*icon, current_x, y, size/2.0, size/2.0);
                        renderer.draw_text(text, current_x+size_x/1.25f, y+size*0.033f, size);
                    }
                    current_x += width;
                }
            }

            void set_ingame_mode(bool ingame_mode) { this->ingame_mode = ingame_mode; }
            bool get_ingame_mode() const { return ingame_mode; }

            void set_blur_background(bool blur) {
                if (blur == blur_background) return;
                blur_background = blur;
                last_blur_background_change = std::chrono::system_clock::now();
            }
            bool get_blur_background() const { return blur_background; }

            void set_choice_overlay(const std::optional<choice_overlay>& overlay) {
                old_choice_overlay = std::move(choice_overlay);
                choice_overlay = overlay;
                last_choice_overlay_change = std::chrono::system_clock::now();
            }
            const std::optional<choice_overlay>& get_choice_overlay() const { return choice_overlay; }

            void set_progress_overlay(std::optional<progress_overlay>&& overlay) {
                progress_overlay = std::move(overlay);
            }
            const std::optional<progress_overlay>& get_progress_overlay() const { return progress_overlay; }

            void set_message_overlay(std::optional<message_overlay>&& overlay) {
                message_overlay = std::move(overlay);
            }
            const std::optional<message_overlay>& get_message_overlay() const { return message_overlay; }

            void set_clipboard(clipboard&& clipboard) {
                this->clipboard = std::move(clipboard);
            }
            const std::optional<clipboard>& get_clipboard() const { return clipboard; }
        private:
            using time_point = std::chrono::time_point<std::chrono::system_clock>;

            std::unique_ptr<font_renderer> font_render;
            std::unique_ptr<image_renderer> image_render;
            std::unique_ptr<simple_renderer> simple_render;
            std::unique_ptr<render::wave_renderer> wave_render;

            vk::UniqueRenderPass backgroundRenderPass, blurRenderPass, shellRenderPass;

            std::vector<vk::UniqueFramebuffer> backgroundFramebuffers;

            vk::UniqueDescriptorSetLayout blurDescriptorSetLayout;
            vk::UniqueDescriptorPool blurDescriptorPool;
            std::vector<vk::DescriptorSet> blurDescriptorSets;
            vk::UniquePipelineLayout blurPipelineLayout;
            vk::UniquePipeline blurPipeline;

            std::unique_ptr<texture> renderImage;
            std::unique_ptr<texture> blurImageSrc;
            std::unique_ptr<texture> blurImageDst;

            std::vector<vk::Image> swapchainImages;
            std::vector<vk::UniqueFramebuffer> framebuffers;

            std::unique_ptr<texture> backgroundTexture;
            main_menu menu{this};
            news_display news{this};
            std::array<std::unique_ptr<texture>, std::to_underlying(action::_length)> buttonTextures;

            sdl::mix::unique_chunk ok_sound;

            void render_gui(gui_renderer& renderer);

            // input handling
            constexpr static int controller_axis_input_threshold = 10000;
            std::array<time_point, 2> last_controller_axis_input_time;
            std::array<std::optional<std::tuple<sdl::GameController*, action>>, 2> last_controller_axis_input;
            constexpr static auto controller_axis_input_duration = std::chrono::milliseconds(200);

            time_point last_controller_button_input_time;
            std::optional<std::tuple<sdl::GameController*, sdl::GameControllerButton>> last_controller_button_input;
            constexpr static auto controller_button_input_duration = std::chrono::milliseconds(200);

            // state
            bool ingame_mode = false;

            bool blur_background = false;
            time_point last_blur_background_change;

            std::optional<app::choice_overlay> choice_overlay = std::nullopt;
            std::optional<app::choice_overlay> old_choice_overlay = std::nullopt;
            time_point last_choice_overlay_change;

            std::optional<app::progress_overlay> progress_overlay = std::nullopt;
            std::optional<app::message_overlay> message_overlay = std::nullopt;

            std::optional<clipboard> clipboard;

            // transition duration constants
            constexpr static auto blur_background_transition_duration = std::chrono::milliseconds(500);
            constexpr static auto choice_overlay_transition_duration = std::chrono::milliseconds(100);
    };
}
