module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module xmbshell.app:main;

import xmbshell.render;
import xmbshell.utils;
import dreamrender;
import sdl2;
import vulkan_hpp;

import :choice_overlay;
import :main_menu;
import :message_overlay;
import :news_display;
import :progress_overlay;

namespace app
{
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

            void dispatch(action action);
            void handle(result result);

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

            sdl::mix::unique_chunk ok_sound;

            void render_gui(gui_renderer& renderer);

            // input handling
            constexpr static int controller_axis_input_threshold = 10000;
            time_point last_controller_axis_input_time[2];
            std::optional<std::tuple<sdl::GameController*, action>> last_controller_axis_input[2];
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

            // transition duration constants
            constexpr static auto blur_background_transition_duration = std::chrono::milliseconds(500);
            constexpr static auto choice_overlay_transition_duration = std::chrono::milliseconds(100);
    };
}
