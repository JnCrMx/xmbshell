#pragma once

#include "app/components/main_menu.hpp"
#include "render/phase.hpp"
#include "render/window.hpp"
#include "render/components/font_renderer.hpp"
#include "render/components/image_renderer.hpp"
#include "render/components/wave_renderer.hpp"
#include "render/gui_renderer.hpp"

#include <memory>

namespace app
{
	using namespace render;
	class xmbshell : public phase, public input::keyboard_handler, public input::controller_handler
	{
		public:
			xmbshell(window* window);
			~xmbshell();
			void preload() override;
			void prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews) override;
			void render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence) override;

			void key_up(SDL_Keysym key) override;
			void key_down(SDL_Keysym key) override;

			void add_controller(SDL_GameController* controller) override;
			void remove_controller(SDL_GameController* controller) override;
			void button_down(SDL_GameController* controller, SDL_GameControllerButton button) override;
			void button_up(SDL_GameController* controller, SDL_GameControllerButton button) override;
			void axis_motion(SDL_GameController* controller, SDL_GameControllerAxis axis, Sint16 value) override;

			bool blur_background = false;
		private:
			std::unique_ptr<font_renderer> font_render;
			std::unique_ptr<image_renderer> image_render;
			std::unique_ptr<wave_renderer> wave_render;

			vk::UniqueRenderPass backgroundRenderPass, blurRenderPass, shellRenderPass, popupRenderPass;

			vk::UniqueDescriptorSetLayout blurDescriptorSetLayout;
			vk::UniqueDescriptorPool blurDescriptorPool;
			std::vector<vk::DescriptorSet> blurDescriptorSets;
			vk::UniquePipelineLayout blurPipelineLayout;
			vk::UniquePipeline blurPipeline;
			std::vector<vk::UniqueImageView> blurSrcViews;
			std::vector<vk::UniqueImageView> blurDstViews;

			std::vector<vk::Image> renderImages;
			std::vector<vma::Allocation> renderAllocations;
			std::vector<vk::UniqueImageView> renderViews;

			std::vector<vk::Image> swapchainImages;
			std::vector<vk::UniqueFramebuffer> framebuffers;

			vk::UniqueCommandPool pool;
			std::vector<vk::UniqueCommandBuffer> commandBuffers;

			std::unique_ptr<texture> backgroundTexture;
			main_menu menu{this};

			void render_gui(gui_renderer& renderer);
	};
}
