module;

#include <cstdint>
#include <memory>
#include <vector>

export module xmbshell.app:main;

import xmbshell.render;
import dreamrender;
import sdl2;
import vulkan_hpp;

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

            bool is_ingame = false;
            bool blur_background = false;
        private:
            std::unique_ptr<font_renderer> font_render;
            std::unique_ptr<image_renderer> image_render;
            std::unique_ptr<render::wave_renderer> wave_render;

            vk::UniqueRenderPass backgroundRenderPass, blurRenderPass, shellRenderPass;

            std::vector<vk::UniqueFramebuffer> backgroundFramebuffers;

            vk::UniqueDescriptorSetLayout blurDescriptorSetLayout;
            vk::UniqueDescriptorPool blurDescriptorPool;
            std::vector<vk::DescriptorSet> blurDescriptorSets;
            vk::UniqueSampler blurSampler;
            vk::UniquePipelineLayout blurPipelineLayout;
            vk::UniquePipeline blurPipeline;
            std::vector<vk::UniqueFramebuffer> blurFramebuffers;

            std::unique_ptr<texture> renderImage;
            std::unique_ptr<texture> blurImage;

            std::vector<vk::Image> swapchainImages;
            std::vector<vk::UniqueFramebuffer> framebuffers;

            std::unique_ptr<texture> backgroundTexture;
            main_menu menu{this};
            news_display news{this};

            void render_gui(gui_renderer& renderer);
    };
}
