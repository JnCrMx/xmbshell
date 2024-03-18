#include "render/resource_loader.hpp"
#include "render/debug.hpp"
#include "render/obj_loader.hpp"

#include <vk_mem_alloc.hpp>
#include <spdlog/spdlog.h>
#include <SDL_image.h>

#include <fstream>
#include <chrono>

namespace render
{
	resource_loader::resource_loader(vk::Device device, vma::Allocator allocator,
		uint32_t transferFamily, uint32_t graphicsFamily,
		std::vector<vk::Queue> queues) : device(device), allocator(allocator), transferFamily(transferFamily), graphicsFamily(graphicsFamily)
	{
		int index = 0;
		for(auto& queue : queues)
		{
			threads.emplace_back(&resource_loader::loadThread, this, index, queue);
			index++;
		}
	}

	resource_loader::~resource_loader()
	{
		quit = true;
		cv.notify_all();
		for(auto& t : threads)
		{
			if(t.joinable())
				t.join();
		}
	}

	std::future<void> resource_loader::loadTexture(texture* image, std::filesystem::path path)
	{
		std::future<void> f;
		{
			std::scoped_lock<std::mutex> l(lock);
			tasks.push(LoadTask{.type = LoadType::Texture, .src = path, .dst = image, .promise = std::promise<void>()});
			f = tasks.back().promise.get_future();
		}
		cv.notify_one();
		return f;
	}

	std::future<void> resource_loader::loadTexture(texture* image, LoaderFunction func)
	{
		std::future<void> f;
		{
			std::scoped_lock<std::mutex> l(lock);
			tasks.push(LoadTask{.type = LoadType::Texture, .src = func, .dst = image, .promise = std::promise<void>()});
			f = tasks.back().promise.get_future();
		}
		cv.notify_one();
		return f;
	}

	std::future<void> resource_loader::loadModel(model* model, std::filesystem::path path)
	{
		std::future<void> f;
		{
			std::scoped_lock<std::mutex> l(lock);
			tasks.push(LoadTask{.type = LoadType::Model, .src = path, .dst = model, .promise = std::promise<void>()});
			f = tasks.back().promise.get_future();
		}
		cv.notify_one();
		return f;
	}

	bool load_texture(
		int index, LoadTask& task, std::mutex& lock,
		vk::Device device, vma::Allocator allocator, vma::Allocation allocation,
		vk::CommandBuffer commandBuffer,
		uint8_t* decodeBuffer, size_t stagingSize, vk::Buffer stagingBuffer)
	{
		texture* tex = std::get<texture*>(task.dst);
		if(std::holds_alternative<std::filesystem::path>(task.src))
		{
			const auto& path = std::get<std::filesystem::path>(task.src);

			SDL_Surface* surface = IMG_Load(path.c_str());
			if(!surface)
			{
				spdlog::error("[Resource Loader {}] Failed to load image {}", index, path.string());
				std::scoped_lock<std::mutex> l(lock);
				tex->create_image(1, 1); // create fake image to avoid crash
				return false;
			}

			if(surface->format->format != SDL_PIXELFORMAT_RGBA32)
			{
				SDL_Surface* newSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
				SDL_FreeSurface(surface);
				surface = newSurface;
			}

			std::size_t size = surface->w * surface->h * surface->format->BytesPerPixel;
			if(size > stagingSize)
			{
				spdlog::error("[Resource Loader {}] Image {} is too large ({} bytes)", index, path.string(), size);
				SDL_FreeSurface(surface);
				std::scoped_lock<std::mutex> l(lock);
				tex->create_image(1, 1); // create fake image to avoid crash
				return false;
			}

			{
				std::scoped_lock<std::mutex> l(lock);
				tex->create_image(surface->w, surface->h);
			}

			SDL_LockSurface(surface);
			allocator.copyMemoryToAllocation(surface->pixels, allocation, 0, surface->w * surface->h * 4);
			SDL_UnlockSurface(surface);
			SDL_FreeSurface(surface);
		}
		else
		{
			std::fill(decodeBuffer, decodeBuffer+stagingSize, 0x00);
			std::get<LoaderFunction>(task.src)(decodeBuffer, stagingSize);
			allocator.copyMemoryToAllocation(decodeBuffer, allocation, 0, stagingSize);
		}

		commandBuffer.begin(vk::CommandBufferBeginInfo());

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
			vk::ImageMemoryBarrier(
				{}, vk::AccessFlagBits::eTransferWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
				VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				tex->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
		std::array<vk::BufferImageCopy, 1> copies = {
			vk::BufferImageCopy(0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
				{}, {static_cast<uint32_t>(tex->width), static_cast<uint32_t>(tex->height), 1})
		};
		commandBuffer.copyBufferToImage(stagingBuffer, tex->image, vk::ImageLayout::eTransferDstOptimal, copies);
		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
			vk::ImageMemoryBarrier(
				vk::AccessFlagBits::eTransferWrite, {},
				vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
				VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				tex->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));
		commandBuffer.end();

		if(std::holds_alternative<std::filesystem::path>(task.src))
		{
			const auto& path = std::get<std::filesystem::path>(task.src).string();
			debugTag(device, tex->image, debug_tag::TextureSrc, path);
			debugName(device, tex->image, "Texture \""+path+"\"");
			debugName(device, tex->imageView.get(), "Texture \""+path+"\" View");
		}
		else
		{
			debugTag(device, tex->image, debug_tag::TextureSrc, "dynamic");
			debugName(device, tex->image, "Dynamic Texture");
			debugName(device, tex->imageView.get(), "Dynamic Texture View");
		}
		return true;
	}

	bool load_model(
		int index, LoadTask& task,
		vk::Device device, vma::Allocator allocator, vma::Allocation allocation,
		vk::CommandBuffer commandBuffer,
		size_t stagingSize, vk::Buffer stagingBuffer)
	{
		std::ifstream obj(std::get<std::filesystem::path>(task.src));
		std::vector<vertex_data> vertices;
		std::vector<uint32_t> indices;
		load_obj(obj, vertices, indices);

		model* mesh = std::get<model*>(task.dst);
		mesh->create_buffers(vertices.size(), indices.size());
		for(auto& v : vertices)
		{
			mesh->min = glm::min(mesh->min, v.position);
			mesh->max = glm::max(mesh->max, v.position);
		}

		vk::DeviceSize vertexOffset = 0;
		vk::DeviceSize vertexSize = vertices.size() * sizeof(vertex_data);
		vk::DeviceSize indexOffset = vertexSize;
		vk::DeviceSize indexSize = indices.size() * sizeof(uint32_t);

		void* buf = allocator.mapMemory(allocation);
		std::copy(vertices.begin(), vertices.end(), (vertex_data*)((uint8_t*)buf+vertexOffset));
		std::copy(indices.begin(), indices.end(), (uint32_t*)((uint8_t*)buf+indexOffset));
		allocator.unmapMemory(allocation);

		commandBuffer.begin(vk::CommandBufferBeginInfo());
		vk::BufferCopy region(0, 0, 0);
		commandBuffer.copyBuffer(stagingBuffer, mesh->vertexBuffer, region.setSrcOffset(vertexOffset).setSize(vertexSize));
		commandBuffer.copyBuffer(stagingBuffer, mesh->indexBuffer, region.setSrcOffset(indexOffset).setSize(indexSize));
		commandBuffer.end();

		const auto& path = std::get<std::filesystem::path>(task.src).string();
		debugName(device, mesh->vertexBuffer, "Model \""+path+"\" Vertex Buffer");
		debugName(device, mesh->indexBuffer, "Model \""+path+"\" Index Buffer");
		return true;
	}

	void resource_loader::loadThread(int index, vk::Queue queue)
	{
		vk::UniqueCommandPool pool;
		vk::UniqueCommandBuffer commandBuffer;
		vk::UniqueFence fence;
		vk::Buffer stagingBuffer;
		vma::Allocation allocation;
		{
			std::scoped_lock<std::mutex> l(lock);

			pool = device.createCommandPoolUnique(
					vk::CommandPoolCreateInfo({}, transferFamily));
			commandBuffer = std::move(device.allocateCommandBuffersUnique(
					vk::CommandBufferAllocateInfo(pool.get(), vk::CommandBufferLevel::ePrimary, 1)).back());
			fence = device.createFenceUnique(vk::FenceCreateInfo());

			vk::BufferCreateInfo buffer_info({}, stagingSize, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
			vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eCpuToGpu);
			std::tie(stagingBuffer, allocation) = allocator.createBuffer(buffer_info, alloc_info);
		}

		uint8_t* cpuBuffer = new uint8_t[stagingSize];

		spdlog::info("[Resource Loader {}]: Started", index);
		std::unique_lock<std::mutex> l(lock);
		do
		{
			cv.wait(l, [this]{
				return (tasks.size() || quit);
			});

			if(!quit && tasks.size())
			{
				auto task = std::move(tasks.front());
				tasks.pop();
				l.unlock();

				spdlog::debug("[Resource Loader {}] Loading {}", index,
					std::holds_alternative<std::filesystem::path>(task.src) ? std::get<std::filesystem::path>(task.src).string() : "dynamic resource");
				auto t0 = std::chrono::high_resolution_clock::now();
				{
					bool okay;
					if(task.type == Texture)
					{
						okay = load_texture(index, task, lock, device, allocator, allocation, commandBuffer.get(), cpuBuffer, stagingSize, stagingBuffer);
					}
					else if(task.type == Model)
					{
						okay = load_model(index, task, device, allocator, allocation, commandBuffer.get(), stagingSize, stagingBuffer);
					}
					if(okay) {
						std::array<vk::SubmitInfo, 1> submits = {
							vk::SubmitInfo({}, {}, commandBuffer.get(), {})
						};
						queue.submit(submits, fence.get());
						vk::Result result = device.waitForFences(fence.get(), true, UINT64_MAX);
						if(result != vk::Result::eSuccess)
						{
							spdlog::error("[Resource Loader {}] Waiting for fence failed: {}", index, vk::to_string(result));
						}
						device.resetCommandPool(pool.get());
						device.resetFences(fence.get());

						if(std::holds_alternative<texture*>(task.dst))
							std::get<texture*>(task.dst)->loaded = true;
						else if(std::holds_alternative<model*>(task.dst))
							std::get<model*>(task.dst)->loaded = true;
					}
					task.promise.set_value();
				}
				auto t1 = std::chrono::high_resolution_clock::now();
				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
				spdlog::debug("[Resource Loader {}] Loaded {} into {} in {} ms", index,
					std::holds_alternative<std::filesystem::path>(task.src) ? std::get<std::filesystem::path>(task.src).string() : "dynamic resource",
					std::holds_alternative<texture*>(task.dst) ? static_cast<void*>(std::get<texture*>(task.dst)->image) :
						(std::holds_alternative<model*>(task.dst) ? std::get<model*>(task.dst)->vertexBuffer : VK_NULL_HANDLE),
					time);

				l.lock();
			}
		} while(!quit);

		allocator.destroyBuffer(stagingBuffer, allocation);
		spdlog::info("[Resource Loader {}]: Quit", index);
	}
}
