#pragma once

#include "render/phase.hpp"
#include "render/texture.hpp"
#include "render/window.hpp"
#include "render/components/font_renderer.hpp"
#include "render/components/image_renderer.hpp"
#include "render/components/wave_renderer.hpp"
#include "render/gui_renderer.hpp"

#include <memory>

namespace render
{
	class render_shell : public phase
	{
		public:
			render_shell(window* window);
			~render_shell();
			void preload() override;
			void prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews) override;
			void render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence) override;
		private:
			std::unique_ptr<font_renderer> font_render;
			std::unique_ptr<image_renderer> image_render;
			std::unique_ptr<wave_renderer> wave_render;

			vk::UniqueRenderPass backgroundRenderPass, shellRenderPass, popupRenderPass;

			std::vector<vk::Image> renderImages;
			std::vector<vma::Allocation> renderAllocations;
			std::vector<vk::UniqueImageView> renderViews;

			std::vector<vk::Image> swapchainImages;
			std::vector<vk::UniqueFramebuffer> framebuffers;

			vk::UniqueCommandPool pool;
			std::vector<vk::UniqueCommandBuffer> commandBuffers;

			std::unique_ptr<texture> test_texture;

			void render_gui(gui_renderer& renderer);
	};
}
