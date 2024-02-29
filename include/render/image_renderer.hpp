#pragma once

#include "texture.hpp"

namespace render
{
	class font_renderer
	{
		public:
			font_renderer(std::string name, int size, vk::Device device, vma::Allocator allocator);
			~font_renderer();

			void preload();
			void prepare(int imageCount);
			void renderImage(vk::CommandBuffer cmd, int frame, const texture& texture, float x, float y, float w, float h);
			void finish(int frame);
        private:
			vk::UniquePipelineLayout pipelineLayout;
			vk::UniquePipeline pipeline;
    };
}
