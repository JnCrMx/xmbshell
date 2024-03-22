#pragma once

#include "render/texture.hpp"
#include <glm/vec4.hpp>
#include <map>

namespace render
{
	class image_renderer
	{
		public:
			image_renderer(vk::Device device, vk::Extent2D frameSize) : device(device), frameSize(frameSize),
				aspectRatio(static_cast<double>(frameSize.width)/frameSize.height) {}
			~image_renderer();

			void preload(const std::vector<vk::RenderPass>& renderPasses);
			void prepare(int imageCount);

			void renderImage(vk::CommandBuffer cmd, int frame, vk::RenderPass renderPass, const texture& texture, float x, float y, float scaleX = 1.0, float scaleY = 1.0, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0)) {
				if(!texture.loaded) return;
				renderImage(cmd, frame, renderPass, texture.imageView.get(), x, y, scaleX, scaleY, color);
			}
			void renderImage(vk::CommandBuffer cmd, int frame, vk::RenderPass renderPass, vk::ImageView view, float x, float y, float scaleX = 1.0, float scaleY = 1.0, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));

			void renderImageSized(vk::CommandBuffer cmd, int frame, vk::RenderPass renderPass, const texture& texture, float x, float y, int width = -1, int height = -1, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0)) {
				if(!texture.loaded) return;
				renderImageSized(cmd, frame, renderPass, texture.imageView.get(), x, y, width == -1 ? texture.width : width, height == -1 ? texture.height : height, color);
			}
			void renderImageSized(vk::CommandBuffer cmd, int frame, vk::RenderPass renderPass, vk::ImageView view, float x, float y, int width, int height, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));

			void finish(int frame);

			constexpr static unsigned int max_images = 512;
        private:
			vk::Device device;

			vk::Extent2D frameSize;
			double aspectRatio;

			vk::UniqueSampler sampler;

			vk::UniqueDescriptorSetLayout descriptorLayout;
			vk::UniqueDescriptorPool descriptorPool;
			std::vector<vk::DescriptorSet> descriptorSets;

			vk::UniquePipelineLayout pipelineLayout;
			std::map<vk::RenderPass, vk::UniquePipeline> pipelines;

			std::vector<
				std::vector<vk::DescriptorImageInfo>
			> imageInfos;
    };
}
