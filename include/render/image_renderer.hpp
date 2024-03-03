#pragma once

#include "texture.hpp"

namespace render
{
	class image_renderer
	{
		public:
			image_renderer(vk::Device device, vk::Extent2D frameSize) : device(device), frameSize(frameSize),
				aspectRatio(static_cast<double>(frameSize.width)/frameSize.height) {}
			~image_renderer();

			void preload(vk::RenderPass renderPass);
			void prepare(int imageCount);

			void renderImage(vk::CommandBuffer cmd, int frame, const texture& texture, float x, float y, float scaleX = 1.0, float scaleY = 1.0) {
				renderImage(cmd, frame, texture.imageView.get(), x, y, scaleX, scaleY);
			}
			void renderImage(vk::CommandBuffer cmd, int frame, vk::ImageView view, float x, float y, float scaleX = 1.0, float scaleY = 1.0);

			void renderImageSized(vk::CommandBuffer cmd, int frame, const texture& texture, float x, float y, int width = -1, int height = -1) {
				renderImageSized(cmd, frame, texture.imageView.get(), x, y, width == -1 ? texture.width : width, height == -1 ? texture.height : height);
			}
			void renderImageSized(vk::CommandBuffer cmd, int frame, vk::ImageView view, float x, float y, int width, int height);

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
			vk::UniquePipeline pipeline;

			std::vector<
				std::vector<vk::DescriptorImageInfo>
			> imageInfos;
    };
}
