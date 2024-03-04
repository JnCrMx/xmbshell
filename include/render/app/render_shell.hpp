#pragma once

#include "render/phase.hpp"
#include "render/texture.hpp"
#include "render/window.hpp"
#include "render/font_renderer.hpp"
#include "render/image_renderer.hpp"
#include "render/gui_renderer.hpp"

#include <memory>

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
			std::unique_ptr<image_renderer> image_render;

			vk::UniqueRenderPass backgroundRenderPass, shellRenderPass, popupRenderPass;

			std::vector<vk::Image> swapchainImages;
			std::vector<vk::UniqueFramebuffer> framebuffers;

			vk::UniqueCommandPool pool;
			std::vector<vk::UniqueCommandBuffer> commandBuffers;

			std::unique_ptr<texture> test_texture;

			void render_gui(gui_renderer& renderer);
	};
}
