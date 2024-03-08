#include "render/components/font_renderer.hpp"

#include "config.hpp"
#include "render/utils.hpp"
#include "render/debug.hpp"
#include "utils.hpp"

#include <freetype2/ft2build.h>
#include <freetype/freetype.h>

#include <spdlog/spdlog.h>
#include <string_view>
#include <glm/ext/matrix_transform.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <cmath>

#include "text.vert.h"
#include "text.geom.h"
#include "text.frag.h"

namespace render
{
	font_renderer::font_renderer(std::string name, int size, vk::Device device, vma::Allocator allocator, vk::Extent2D frameSize) :
		name(name), size(size), device(device), allocator(allocator),
		aspectRatio(static_cast<double>(frameSize.width)/frameSize.height)
	{

	}

	font_renderer::~font_renderer()
	{
		if(face)
			FT_Done_Face(face);

		for(int i=0; i<uniformBuffers.size(); i++)
		{
			allocator.unmapMemory(uniformMemories[i]);
			allocator.destroyBuffer(uniformBuffers[i], uniformMemories[i]);
		}
		for(int i=0; i<vertexBuffers.size(); i++)
		{
			allocator.unmapMemory(vertexMemories[i]);
			allocator.destroyBuffer(vertexBuffers[i], vertexMemories[i]);
		}
	}

	void font_renderer::preload(FT_Library ft, resource_loader* loader, vk::RenderPass renderPass)
	{
		FT_Error err = FT_New_Face(ft, name.c_str(), 0, &face);
		if(err != 0) throw std::runtime_error("failed to load face");

		err = FT_Set_Pixel_Sizes(face, 0, size);
		if(err != 0) throw std::runtime_error("failed to set size");

		int columns = std::ceil(std::sqrt((float)charEnd-charStart));
		int rows = std::ceil(std::sqrt((float)charEnd-charStart));

		long maxWidth = 0;
		long maxHeight = 0;
		FT_Int baseline = 0;

		for(char c=charStart; c<charEnd; c++)
		{
			FT_UInt glyph_index = FT_Get_Char_Index(face, c);
			FT_Int32 load_flags = FT_LOAD_DEFAULT;
			err = FT_Load_Glyph(face, glyph_index, load_flags);
			if(err != 0) throw std::runtime_error("failed to load glyph");

			baseline = std::max(baseline, face->glyph->bitmap_top);
			maxWidth = std::max(maxWidth, face->glyph->advance.x >> 6);
			maxHeight = std::max(maxWidth, face->glyph->advance.y >> 6);
		}
		maxWidth += 4;
		maxHeight += 4;

		long width = maxWidth*columns;
		long height = maxHeight*rows;
		spdlog::debug("Font texture will be {}x{} ({}x{})", width, height, columns, rows);

		char ch = charStart;
		for(int r=0; r<rows; r++)
		{
			for(int c=0; c<columns; c++)
			{
				FT_UInt glyph_index = FT_Get_Char_Index(face, ch);
				FT_Int32 load_flags = FT_LOAD_DEFAULT;
				err = FT_Load_Glyph(face, glyph_index, load_flags);
				if(err != 0) throw std::runtime_error("failed to load glyph");

				glyphs[ch] = {.x = (int)(c*maxWidth), .y = (int)(r*maxHeight),
					.w = (int)(face->glyph->advance.x >> 6), .h = (int)(maxHeight) };

				ch++; if(ch>=charEnd) break;
			}
			if(ch>=charEnd) break;
		}
		lineHeight = maxHeight;

		fontTexture = std::make_unique<texture>(device, allocator, width, height);
		textureReady = loader->loadTexture(fontTexture.get(), [this, rows, columns, maxWidth, maxHeight, width, baseline](uint8_t* p, size_t size){
			assert(sizeof(uint32_t)*rows*maxHeight*columns*maxWidth <= size);
			uint32_t* image = (uint32_t*)p;

			FT_Error err;
			char ch = charStart;
			for(int r = 0; r<rows; r++)
			{
				for(int c=0; c<columns; c++)
				{
					FT_UInt glyph_index = FT_Get_Char_Index(face, ch);
					FT_Int32 load_flags = FT_LOAD_DEFAULT;
					err = FT_Load_Glyph(face, glyph_index, load_flags);
					if(err != 0) throw std::runtime_error("failed to load glyph");

					err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
					if(err != 0) throw std::runtime_error("failed to render glyph");

					FT_Bitmap bitmap = face->glyph->bitmap;
					for(int y=0; y<bitmap.rows; y++)
					{
						for(int x=0; x<bitmap.width; x++)
						{
							unsigned char pixel_brightness = bitmap.buffer[y * bitmap.pitch + x];
							unsigned int color = pixel_brightness | pixel_brightness << 8 | pixel_brightness << 16 | pixel_brightness << 24;

							int dx = x;
							int dy = y + baseline - face->glyph->bitmap_top;

							image[(r*maxHeight+dy)*width + (c*maxWidth+dx)] = color;
						}
					}

					ch++;
					if(ch >= charEnd)
						break;
				}
				if(ch >= charEnd)
					break;
			}
		});

		vk::SamplerCreateInfo sampler_info({}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
			0.0f, false, 0.0f, false, vk::CompareOp::eNever, 0.0f, 0.0f, vk::BorderColor::eFloatTransparentBlack, false);
		sampler = device.createSamplerUnique(sampler_info);

		{
			std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
				vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment),
				vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment)
			};
			vk::DescriptorSetLayoutCreateInfo layout_info({}, bindings);
			descriptorLayout = device.createDescriptorSetLayoutUnique(layout_info);
			debugName(device, descriptorLayout.get(), "Font Renderer Descriptor Layout");
		}
		{
			vk::PipelineLayoutCreateInfo layout_info({}, descriptorLayout.get());
			pipelineLayout = device.createPipelineLayoutUnique(layout_info);
			debugName(device, pipelineLayout.get(), "Font Renderer Pipeline Layout");
		}
		{
			vk::UniqueShaderModule vertexShader = createShader(device, shaders_text_vert_spv, shaders_text_vert_spv_len);
			vk::UniqueShaderModule geometryShader = createShader(device, shaders_text_geom_spv, shaders_text_geom_spv_len);
			vk::UniqueShaderModule fragmentShader = createShader(device, shaders_text_frag_spv, shaders_text_frag_spv_len);
			std::array<vk::PipelineShaderStageCreateInfo, 3> shaders = {
				vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShader.get(), "main"),
				vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eGeometry, geometryShader.get(), "main"),
				vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShader.get(), "main")
			};

			vk::VertexInputBindingDescription binding(0, sizeof(VertexCharacter), vk::VertexInputRate::eVertex);
			std::array<vk::VertexInputAttributeDescription, 4> attributes = {
				vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(VertexCharacter, position)),
				vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32Sfloat, offsetof(VertexCharacter, texCoord)),
				vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(VertexCharacter, size)),
				vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(VertexCharacter, color)),
			};

			vk::PipelineVertexInputStateCreateInfo vertex_input({}, binding, attributes);
			vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::ePointList);
			vk::PipelineTessellationStateCreateInfo tesselation({}, {});

			vk::Viewport v{};
			vk::Rect2D s{};
			vk::PipelineViewportStateCreateInfo viewport({}, v, s);

			vk::PipelineRasterizationStateCreateInfo rasterization({}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);
			vk::PipelineMultisampleStateCreateInfo multisample({}, config::CONFIG.sampleCount);
			vk::PipelineDepthStencilStateCreateInfo depthStencil({}, false, false);

			vk::PipelineColorBlendAttachmentState attachment(true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
				vk::BlendFactor::eOne, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
			vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, vk::LogicOp::eClear, attachment);

			std::array<vk::DynamicState, 2> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
			vk::PipelineDynamicStateCreateInfo dynamic({}, dynamicStates);

			vk::GraphicsPipelineCreateInfo pipeline_info({}, shaders, &vertex_input,
				&input_assembly, &tesselation, &viewport, &rasterization, &multisample, &depthStencil, &colorBlend, &dynamic, pipelineLayout.get(), renderPass);
			pipeline = device.createGraphicsPipelineUnique({}, pipeline_info).value;
			debugName(device, pipeline.get(), "Font Renderer Pipeline");
		}
	}

	void font_renderer::prepare(int imageCount)
	{
		std::array<vk::DescriptorPoolSize, 2> sizes = {
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1*imageCount),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1*imageCount)
		};
		vk::DescriptorPoolCreateInfo pool_info({}, imageCount, sizes);
		descriptorPool = device.createDescriptorPoolUnique(pool_info);

		std::vector<vk::DescriptorSetLayout> layouts(imageCount);
		std::fill(layouts.begin(), layouts.end(), descriptorLayout.get());
		vk::DescriptorSetAllocateInfo set_info(descriptorPool.get(), layouts);
		descriptorSets = device.allocateDescriptorSets(set_info);

		const auto alignment = allocator.getPhysicalDeviceProperties()->limits.minUniformBufferOffsetAlignment;

		std::vector<vk::WriteDescriptorSet> writes(imageCount*2);
		std::vector<vk::DescriptorBufferInfo> bufferInfos(imageCount);
		vk::DescriptorImageInfo imageInfo(sampler.get(), fontTexture->imageView.get(), vk::ImageLayout::eShaderReadOnlyOptimal);
		for(int i=0; i<imageCount; i++)
		{
			{
				vk::BufferCreateInfo vertex_info({}, maxTexts*maxCharacters*sizeof(VertexCharacter), vk::BufferUsageFlagBits::eVertexBuffer);
				vma::AllocationCreateInfo va_info({}, vma::MemoryUsage::eCpuToGpu);
				auto [vb, va] = allocator.createBuffer(vertex_info, va_info);
				vertexBuffers.push_back(vb);
				vertexMemories.push_back(va);
				vertexPointers.push_back((VertexCharacter*)allocator.mapMemory(va));
			}

			{
				vk::BufferCreateInfo uniform_info({}, maxTexts*sizeof(TextUniform), vk::BufferUsageFlagBits::eUniformBuffer);
				vma::AllocationCreateInfo ua_info({}, vma::MemoryUsage::eCpuToGpu);
				auto [ub, ua] = allocator.createBuffer(uniform_info, ua_info);
				uniformBuffers.push_back(ub);
				uniformMemories.push_back(ua);
				uniformPointers.push_back(utils::aligned_wrapper<TextUniform>(allocator.mapMemory(ua), alignment));

				bufferInfos[i].setBuffer(ub).setOffset(0).setRange(sizeof(TextUniform));
				writes[2*i+0].setDstSet(descriptorSets[i]).setDstBinding(0).setDstArrayElement(0)
					.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic).setDescriptorCount(1).setBufferInfo(bufferInfos[i]);
				writes[2*i+1].setDstSet(descriptorSets[i]).setDstBinding(1).setDstArrayElement(0)
					.setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setDescriptorCount(1).setImageInfo(imageInfo);
			}
		}
		device.updateDescriptorSets(writes, {});

		uniformOffsets.resize(imageCount);
		vertexOffsets.resize(imageCount);
	}

	void font_renderer::renderText(vk::CommandBuffer cmd, int frame, std::string_view text, float x, float y, float scale, glm::vec4 color)
	{
		if(!utils::is_ready(textureReady))
			return;

		{
			VertexCharacter* vc = vertexPointers[frame] + vertexOffsets[frame];
			float x = 0;
			for(int i=0; i<text.size(); i++)
			{
				Glyph g = glyphs[text[i]];
				vc[i] = {
					.position = {x, 0},
					.texCoord = {g.x/lineHeight, g.y/lineHeight},
					.size = {((float)g.w)/lineHeight, ((float)g.h)/lineHeight},
					.color = color};
				x+= ((float)g.w)/lineHeight;
			}
		}
		{
    		glm::vec2 pos = glm::vec2(x, y)*2.0f - glm::vec2(1.0f);

			TextUniform& uni = uniformPointers[frame][uniformOffsets[frame]];
			uni.matrix = glm::mat4(1.0f);
			uni.matrix = glm::translate(uni.matrix, glm::vec3(pos, 0.0f));
			uni.matrix = glm::scale(uni.matrix, glm::vec3(scale / aspectRatio, scale, 1.0f));
			uni.textureSize = {fontTexture->width/lineHeight, fontTexture->height/lineHeight};
		}
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
		cmd.bindVertexBuffers(0, vertexBuffers[frame], vertexOffsets[frame]*sizeof(VertexCharacter));
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout.get(), 0, descriptorSets[frame], uniformPointers[frame].offset(uniformOffsets[frame]));
		cmd.draw(text.size(), 1, 0, 0);

		vertexOffsets[frame] += text.size();
		uniformOffsets[frame]++;
	}

	glm::vec2 font_renderer::measureText(std::string_view text, float scale)
	{
		float width = 0;
		for(int i=0; i<text.size(); i++)
		{
			Glyph g = glyphs[text[i]];
			width += ((float)g.w)/lineHeight;
		}
		return glm::vec2{width*scale/aspectRatio, scale}/2.0f;
	}

	void font_renderer::finish(int frame)
	{
		vertexOffsets[frame] = 0;
		uniformOffsets[frame] = 0;
	}
}
