module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module xmbshell.app:main;

import xmbshell.render;
import dreamrender;
import sdl2;
import vulkan_hpp;

import :choice_overlay;
import :main_menu;
import :news_display;

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

            void key_up(sdl::Keysym key) override;
            void key_down(sdl::Keysym key) override;

            void add_controller(sdl::GameController* controller) override;
            void remove_controller(sdl::GameController* controller) override;
            void button_down(sdl::GameController* controller, sdl::GameControllerButton button) override;
            void button_up(sdl::GameController* controller, sdl::GameControllerButton button) override;
            void axis_motion(sdl::GameController* controller, sdl::GameControllerAxis axis, int16_t value) override;

            void reload_background();
            void reload_fonts();

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

            void render_gui(gui_renderer& renderer);

            // state
            bool ingame_mode = false;

            bool blur_background = false;
            time_point last_blur_background_change;

            std::optional<app::choice_overlay> choice_overlay = std::nullopt;
            std::optional<app::choice_overlay> old_choice_overlay = std::nullopt;
            time_point last_choice_overlay_change;

            // transition duration constants
            constexpr static auto blur_background_transition_duration = std::chrono::milliseconds(500);
            constexpr static auto choice_overlay_transition_duration = std::chrono::milliseconds(100);
    };
}
