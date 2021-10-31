#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>

#include "render/resource_loader.hpp"

namespace render
{
	class window;

	class phase
	{
		public:
			phase(window*);
			virtual ~phase();

			virtual void preload();
			virtual void prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews);
			virtual void init();
			virtual void render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence);

			void waitLoad();
		protected:
			window* win;

			vk::Instance instance;
			vk::Device device;
			vma::Allocator allocator;
			resource_loader* loader;

			uint32_t graphicsFamily;
			vk::Queue graphicsQueue;

			std::vector<std::shared_future<void>> loadingFutures;
	};
}