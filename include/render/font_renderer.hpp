#pragma once

#include <memory>
#include <string>
#include <map>

#include <freetype2/ft2build.h>
#include <freetype/freetype.h>
#include <glm/glm.hpp>

#include "texture.hpp"
#include "resource_loader.hpp"

namespace render
{
	struct Glyph
	{
		int x;
		int y;
		int w;
		int h;
	};

	class font_renderer
	{
		public:
			font_renderer(std::string name, int size, vk::Device device, vma::Allocator allocator);
			~font_renderer();

			void preload(FT_Library ft, resource_loader* loader, vk::RenderPass renderPass);
			void prepare(int imageCount);
			void renderText(vk::CommandBuffer cmd, int frame, std::string text, float x, float y, float scale = 1.0f, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
			void finish(int frame);

			std::unique_ptr<texture> fontTexture;
			std::shared_future<void> textureReady;
			vk::UniqueSampler sampler;

			vk::UniquePipelineLayout pipelineLayout;
			vk::UniquePipeline pipeline;

			vk::UniqueDescriptorSetLayout descriptorLayout;
			vk::UniqueDescriptorPool descriptorPool;
			std::vector<vk::DescriptorSet> descriptorSets;
			
			std::vector<vk::Buffer> uniformBuffers;
			std::vector<vma::Allocation> uniformMemories;
			std::vector<vk::DeviceSize> uniformOffsets;

			std::vector<vk::Buffer> vertexBuffers;
			std::vector<vma::Allocation> vertexMemories;
			std::vector<uint32_t> vertexOffsets;
		private:
			vk::Device device;
			vma::Allocator allocator;

			std::string name;
			int size;
			FT_Face face = 0;
			std::map<char, Glyph> glyphs;

			float lineHeight;

			static constexpr char charStart = 32;
			static constexpr char charEnd = 127;

			static constexpr size_t maxCharacters = 256;
			static constexpr size_t maxTexts = 256;
			struct VertexCharacter {
				glm::vec2 position;
				glm::vec2 texCoord;
				glm::vec2 size;
				glm::vec4 color;
			};
			struct TextUniform {
				glm::vec2 position;
				glm::vec2 textureSize;
				float scale;
			};

			std::vector<VertexCharacter*> vertexPointers;
			std::vector<TextUniform*> uniformPointers;
	};
}
