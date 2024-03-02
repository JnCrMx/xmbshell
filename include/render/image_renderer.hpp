#pragma once

#include "texture.hpp"

namespace render
{
	class image_renderer
	{
		public:
			image_renderer(vk::Device device) : device(device) {}
			~image_renderer();

			void preload(vk::RenderPass renderPass);
			void prepare(int imageCount);
			void renderImage(vk::CommandBuffer cmd, int frame, const texture& texture, float x, float y, float w, float h);
			void finish(int frame);

			constexpr static unsigned int max_images = 512;
        private:
			vk::Device device;

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
