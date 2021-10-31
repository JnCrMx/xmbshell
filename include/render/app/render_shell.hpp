#pragma once

#include "render/phase.hpp"
#include "render/window.hpp"
#include "render/font_renderer.hpp"

namespace render
{
	class render_shell : public phase
	{
		public:
			render_shell(window* window);
			void preload() override;
			void prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews) override;
			void render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence) override;
		private:
			std::unique_ptr<font_renderer> font;

			vk::UniqueRenderPass renderPass;

			std::vector<vk::Image> swapchainImages;
			std::vector<vk::UniqueFramebuffer> framebuffers;
			
			vk::UniqueCommandPool pool;
			std::vector<vk::UniqueCommandBuffer> commandBuffers;
	};
}