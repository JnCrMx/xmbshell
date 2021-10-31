#pragma once

#include <string>
#include <variant>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <optional>
#include <future>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>

#include <spng.h>

#include "texture.hpp"
#include "model.hpp"

namespace render
{
	enum LoadType
	{
		Texture,
		Image,
		Buffer,
		Model
	};

	using LoaderFunction = std::function<void(uint8_t*, size_t)>;
	struct LoadTask
	{
		LoadType type;
		std::variant<std::string, LoaderFunction> src;
		std::variant<texture*, model*, vk::Image, vk::Buffer> dst;
		std::promise<void> promise;
	};

	class resource_loader
	{
		public:
			resource_loader(vk::Device device, vma::Allocator allocator,
				uint32_t transferFamily, uint32_t graphicsFamily,
				std::vector<vk::Queue> queues);
			~resource_loader();

			std::future<void> loadTexture(texture* texture, std::string filename);
			std::future<void> loadTexture(texture* texture, LoaderFunction loader);

			std::future<void> loadModel(model* model, std::string filename);

			static vk::Extent2D getImageSize(std::string filename);
		private:
			vk::Device device;
			vma::Allocator allocator;

			uint32_t transferFamily;
			uint32_t graphicsFamily;

			std::mutex lock;
			std::vector<std::thread> threads;
			std::queue<LoadTask> tasks;
			std::condition_variable cv;
			bool quit = false;

			void loadThread(int index, vk::Queue queue);

			constexpr static vk::DeviceSize stagingSize = 16*1024*1024;
	};
}
