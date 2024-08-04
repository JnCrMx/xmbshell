#pragma once

#include "render/utils.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
import vulkan_hpp;
#include <vk_mem_alloc.hpp>

namespace render
{
    class simple_renderer {
        public:
            simple_renderer(vk::Device device, vk::Extent2D frameSize) : device(device), frameSize(frameSize),
				aspectRatio(static_cast<double>(frameSize.width)/frameSize.height) {}
			~simple_renderer();

            void preload(const std::vector<vk::RenderPass>& renderPasses, vk::PipelineCache pipelineCache = {});

            void drawPoints(vk::CommandBuffer cmd, int frame, vk::RenderPass renderPass, const std::vector<glm::vec2>& vertices, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
            void drawLines(vk::CommandBuffer cmd, int frame, vk::RenderPass renderPass, const std::vector<glm::vec2>& vertices, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
            void drawTriangles(vk::CommandBuffer cmd, int frame, vk::RenderPass renderPass, const std::vector<glm::vec2>& vertices, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));

            void drawLine(vk::CommandBuffer cmd, int frame, vk::RenderPass renderPass, float x1, float y1, float x2, float y2, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
            void drawTriangle(vk::CommandBuffer cmd, int frame, vk::RenderPass renderPass, float x1, float y1, float x2, float y2, float x3, float y3, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
            void drawRectangle(vk::CommandBuffer cmd, int frame, vk::RenderPass renderPass, float x, float y, float width, float height, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
        private:
			vk::Device device;

			vk::Extent2D frameSize;
			double aspectRatio;

			vk::UniquePipelineLayout pipelineLayout;

			UniquePipelineMap linePipelines;
            UniquePipelineMap trianglePipelines;
            UniquePipelineMap rectanglePipelines;
    };
}
