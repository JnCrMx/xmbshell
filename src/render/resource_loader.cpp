#include "render/resource_loader.hpp"
#include "render/debug.hpp"
#include "render/obj_loader.hpp"

#include <vk_mem_alloc.hpp>
#include <spdlog/spdlog.h>
#include <spng.h>

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

	std::future<void> resource_loader::loadTexture(texture* image, std::string filename)
	{
		std::future<void> f;
		{
			std::scoped_lock<std::mutex> l(lock);
			tasks.push(LoadTask{.type = LoadType::Texture, .src = filename, .dst = image, .promise = std::promise<void>()});
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

	std::future<void> resource_loader::loadModel(model* model, std::string filename)
	{
		std::future<void> f;
		{
			std::scoped_lock<std::mutex> l(lock);
			tasks.push(LoadTask{.type = LoadType::Model, .src = filename, .dst = model, .promise = std::promise<void>()});
			f = tasks.back().promise.get_future();
		}
		cv.notify_one();
		return f;
	}

	// Ugly hack to get PNG size BEFORE loading it, so we can create a vk::Image and a vk::ImageView in advance
	vk::Extent2D resource_loader::getImageSize(std::string filename)
	{
		std::ifstream in("assets/textures/"+filename, std::ios_base::binary);

		// 32 bytes is enough to capture the IHDR chunk (it's guaranteed to be the first chunk) which is all we need
		std::vector<char> data(32);
		in.read(data.data(), data.size());

		spng_ctx* sizeCtx = spng_ctx_new(0);
		spng_set_png_buffer(sizeCtx, data.data(), data.size());

		struct spng_ihdr ihdr;
    	spng_get_ihdr(sizeCtx, &ihdr);
		spng_ctx_free(sizeCtx);

		return vk::Extent2D{ihdr.width, ihdr.height};
	}

	void load_texture(
		int index, LoadTask& task,
		vk::Device device, vma::Allocator allocator, vma::Allocation allocation,
		vk::CommandBuffer commandBuffer, spng_ctx* ctx, 
		uint8_t* decodeBuffer, size_t stagingSize, vk::Buffer stagingBuffer)
	{
		texture* tex = std::get<texture*>(task.dst);
		if(std::holds_alternative<std::string>(task.src))
		{
			std::ifstream in("assets/textures/"+std::get<std::string>(task.src), std::ios_base::ate | std::ios_base::binary);
			size_t size = in.tellg();
			std::vector<char> data(size);
			in.seekg(0);
			in.read(data.data(), size);

			spng_set_png_buffer(ctx, data.data(), data.size());

			struct spng_ihdr ihdr;
    		spng_get_ihdr(ctx, &ihdr);
			tex->create_image(ihdr.width, ihdr.height);

			size_t decodedSize;
			spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &decodedSize);
			assert(decodedSize <= stagingSize);
			spng_decode_image(ctx, decodeBuffer, decodedSize, SPNG_FMT_RGBA8, 0);
		}
		else
		{
			std::fill(decodeBuffer, decodeBuffer+stagingSize, 0x00);
			std::get<LoaderFunction>(task.src)(decodeBuffer, stagingSize);
		}
		void* buf = allocator.mapMemory(allocation);
		std::copy(decodeBuffer, decodeBuffer+stagingSize, (char*)buf);
		allocator.unmapMemory(allocation);

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

		if(std::holds_alternative<std::string>(task.src))
		{
			debugTag(device, tex->image, debug_tag::TextureSrc, std::get<std::string>(task.src));
			debugName(device, tex->image, "Texture \""+std::get<std::string>(task.src)+"\"");
			debugName(device, tex->imageView.get(), "Texture \""+std::get<std::string>(task.src)+"\" View");
		}
		else
		{
			debugTag(device, tex->image, debug_tag::TextureSrc, "dynamic");
			debugName(device, tex->image, "Dynamic Texture");
			debugName(device, tex->imageView.get(), "Dynamic Texture View");
		}
	}

	void load_model(
		int index, LoadTask& task,
		vk::Device device, vma::Allocator allocator, vma::Allocation allocation,
		vk::CommandBuffer commandBuffer, 
		size_t stagingSize, vk::Buffer stagingBuffer)
	{
		std::ifstream obj("assets/models/"+std::get<std::string>(task.src));
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

		debugName(device, mesh->vertexBuffer, "Model \""+std::get<std::string>(task.src)+"\" Vertex Buffer");
		debugName(device, mesh->indexBuffer, "Model \""+std::get<std::string>(task.src)+"\" Index Buffer");
	}

	void resource_loader::loadThread(int index, vk::Queue queue)
	{
		vk::UniqueCommandPool pool = device.createCommandPoolUnique(
				vk::CommandPoolCreateInfo({}, transferFamily));
		vk::UniqueCommandBuffer commandBuffer = std::move(device.allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo(pool.get(), vk::CommandBufferLevel::ePrimary, 1)).back());
		vk::UniqueFence fence = device.createFenceUnique(vk::FenceCreateInfo());

		vk::BufferCreateInfo buffer_info({}, stagingSize, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
		vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eCpuToGpu);
		auto [stagingBuffer, allocation] = allocator.createBuffer(buffer_info, alloc_info);

		uint8_t* cpuBuffer = new uint8_t[stagingSize];

		spng_ctx *ctx = spng_ctx_new(0);

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
					std::holds_alternative<std::string>(task.src) ? std::get<std::string>(task.src) : "dynamic resource");
				auto t0 = std::chrono::high_resolution_clock::now();
				{
					if(task.type == Texture)
					{
						load_texture(index, task, device, allocator, allocation, commandBuffer.get(), ctx, cpuBuffer, stagingSize, stagingBuffer);
					}
					else if(task.type == Model)
					{
						load_model(index, task, device, allocator, allocation, commandBuffer.get(), stagingSize, stagingBuffer);
					}
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
					task.promise.set_value();
				}
				auto t1 = std::chrono::high_resolution_clock::now();
				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
				spdlog::debug("[Resource Loader {}] Loaded {} in {} ms", index,
					std::holds_alternative<std::string>(task.src) ? std::get<std::string>(task.src) : "dynamic resource", time);

				l.lock();
			}
		} while(!quit);

		spng_ctx_free(ctx);
		allocator.destroyBuffer(stagingBuffer, allocation);
		spdlog::info("[Resource Loader {}]: Quit", index);
	}
}