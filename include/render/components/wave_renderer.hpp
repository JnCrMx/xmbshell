#pragma once

#include "render/utils.hpp"
import vulkan_hpp;
#include <vk_mem_alloc.hpp>

namespace render
{
    class wave_renderer {
        public:
			static constexpr int grid_quality = 128;

            wave_renderer(vk::Device device, vma::Allocator allocator, vk::Extent2D frameSize) : device(device), allocator(allocator), frameSize(frameSize),
                aspectRatio(static_cast<double>(frameSize.width)/frameSize.height) {}
            ~wave_renderer();

            void preload(const std::vector<vk::RenderPass>& renderPass, vk::PipelineCache pipelineCache = {});
            void prepare(int imageCount);
            void finish(int frame);

            void render(vk::CommandBuffer cmd, int frame, vk::RenderPass renderPass);

            float speed = 1.0;
        private:
			vk::Device device;
            vma::Allocator allocator;

			vk::Extent2D frameSize;
			double aspectRatio;

            unsigned int vertexCount;
            vk::Buffer vertexBuffer;
            vma::Allocation vertexAllocation;

            unsigned int indexCount;
            vk::Buffer indexBuffer;
            vma::Allocation indexAllocation;

            vk::UniquePipelineLayout pipelineLayout;
            UniquePipelineMap pipelines;
    };
}
