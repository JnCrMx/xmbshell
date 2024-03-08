#pragma once

#include <memory>
#include <string>
#include <map>

#include <freetype2/ft2build.h>
#include <freetype/freetype.h>
#include <glm/glm.hpp>
#include <string_view>

#include "render/texture.hpp"
#include "render/resource_loader.hpp"
#include "utils.hpp"

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
			font_renderer(std::string name, int size, vk::Device device, vma::Allocator allocator, vk::Extent2D frameSize);
			~font_renderer();

			void preload(FT_Library ft, resource_loader* loader, vk::RenderPass renderPass);
			void prepare(int imageCount);
			void finish(int frame);

			void renderText(vk::CommandBuffer cmd, int frame, std::string_view text, float x, float y, float scale = 1.0f, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
			glm::vec2 measureText(std::string_view text, float scale = 1.0f);

			double aspectRatio;

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

			static constexpr size_t maxCharacters = 1024;
			static constexpr size_t maxTexts = 128;
			struct VertexCharacter {
				glm::vec2 position;
				glm::vec2 texCoord;
				glm::vec2 size;
				glm::vec4 color;
			};
			struct TextUniform {
				glm::mat4 matrix;
				glm::vec2 textureSize;
			};

			std::vector<VertexCharacter*> vertexPointers;
			std::vector<utils::aligned_wrapper<TextUniform>> uniformPointers;
	};
}
