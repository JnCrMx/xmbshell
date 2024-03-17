#include "render/components/wave_renderer.hpp"

#include "config.hpp"
#include "render/utils.hpp"

#include "wave.vert.h"
#include "wave.frag.h"

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace render {
    namespace {
        struct push_constants {
			glm::vec4 color;
            float time;
        };
    }

    wave_renderer::~wave_renderer() {
		allocator.destroyBuffer(vertexBuffer, vertexAllocation);
		allocator.destroyBuffer(indexBuffer, indexAllocation);
    }

	void generate_grid(int N, std::vector<glm::vec3> &vertices, std::vector<uint16_t> &indices)
	{
		for(int j=0; j<=N; ++j)
		{
			for(int i=0; i<=N; ++i)
			{
				float x = 2.0*((float)i/(float)N)-1.0;
				float y = 2.0*((float)j/(float)N)-1.0;
				float z = 0.0f;
				vertices.push_back(glm::vec3(x, y, z));
			}
		}

		for(int j=0; j<N; ++j)
		{
			for(int i=0; i<N; ++i)
			{
				int row1 = j * (N+1);
				int row2 = (j+1) * (N+1);

				// triangle 1
				indices.push_back(row1+i);
				indices.push_back(row1+i+1);
				indices.push_back(row2+i+1);

				// triangle 2
				indices.push_back(row1+i);
				indices.push_back(row2+i+1);
				indices.push_back(row2+i);
			}
		}
	}

    void wave_renderer::preload(vk::RenderPass renderPass) {
		{
			std::vector<glm::vec3> vertices;
			std::vector<uint16_t> indices;
			generate_grid(grid_quality, vertices, indices);
			vertexCount = vertices.size();
			indexCount = indices.size();

			spdlog::debug("Grid made of {} vertices and {} indices", vertexCount, indexCount);

			{
				std::tie(vertexBuffer, vertexAllocation) = allocator.createBuffer(
					vk::BufferCreateInfo({}, vertices.size() * sizeof(vertices[0]), vk::BufferUsageFlagBits::eVertexBuffer),
					vma::AllocationCreateInfo({}, vma::MemoryUsage::eCpuToGpu));

				decltype(&vertices[0]) map = (decltype(&vertices[0]))allocator.mapMemory(vertexAllocation);
				std::copy(vertices.begin(), vertices.end(), map);
				allocator.unmapMemory(vertexAllocation);
			}
			{
				std::tie(indexBuffer, indexAllocation) = allocator.createBuffer(
					vk::BufferCreateInfo({}, indices.size() * sizeof(indices[0]), vk::BufferUsageFlagBits::eIndexBuffer),
					vma::AllocationCreateInfo({}, vma::MemoryUsage::eCpuToGpu));

				decltype(&indices[0]) map = (decltype(&indices[0]))allocator.mapMemory(indexAllocation);
				std::copy(indices.begin(), indices.end(), map);
				allocator.unmapMemory(indexAllocation);
			}
		}
        {
            vk::PushConstantRange range(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(push_constants));
			vk::PipelineLayoutCreateInfo layout_info({}, {}, range);
			pipelineLayout = device.createPipelineLayoutUnique(layout_info);
		}
		{
			vk::VertexInputBindingDescription input_binding(0, sizeof(glm::vec3), vk::VertexInputRate::eVertex);
			vk::VertexInputAttributeDescription input_attribute(0, 0, vk::Format::eR32G32B32Sfloat, 0);

			vk::PipelineVertexInputStateCreateInfo vertex_input({}, input_binding, input_attribute);
			vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::eTriangleList);
			vk::PipelineTessellationStateCreateInfo tesselation({}, {});

			vk::Viewport v{};
			vk::Rect2D s{};
			vk::PipelineViewportStateCreateInfo viewport({}, v, s);

			vk::PipelineRasterizationStateCreateInfo rasterization({}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);
			vk::PipelineMultisampleStateCreateInfo multisample({}, config::CONFIG.sampleCount);
			vk::PipelineDepthStencilStateCreateInfo depthStencil({}, false, false);

			vk::PipelineColorBlendAttachmentState attachment(true, vk::BlendFactor::eOne, vk::BlendFactor::eOne, vk::BlendOp::eAdd, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd);
			attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
			vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, vk::LogicOp::eClear, attachment);

			std::array<vk::DynamicState, 2> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
			vk::PipelineDynamicStateCreateInfo dynamic({}, dynamicStates);

			{
				vk::UniqueShaderModule vertexShader = createShader(device, shaders_wave_vert);
				vk::UniqueShaderModule fragmentShader = createShader(device, shaders_wave_frag);
				std::array<vk::PipelineShaderStageCreateInfo, 2> shaders = {
					vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShader.get(), "main"),
					vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShader.get(), "main")
				};

				vk::GraphicsPipelineCreateInfo pipeline_info({}, shaders, &vertex_input,
					&input_assembly, &tesselation, &viewport, &rasterization, &multisample, &depthStencil, &colorBlend, &dynamic, pipelineLayout.get(), renderPass);
				pipeline = device.createGraphicsPipelineUnique({}, pipeline_info).value;
			}
        }
    }

    void wave_renderer::prepare(int imageCount) {
    }

    void wave_renderer::finish(int frame) {
    }

	static auto startTime = std::chrono::high_resolution_clock::now();
    void wave_renderer::render(vk::CommandBuffer cmd, int frame) {
		auto time = std::chrono::high_resolution_clock::now() - startTime;
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time);
		auto partialSeconds = std::chrono::duration<float>(time-seconds);

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());

        push_constants push{
			glm::vec4(config::CONFIG.waveColor, 1.0),
			(seconds.count() + partialSeconds.count())*speed
		};
        cmd.pushConstants(pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(push_constants), &push);

        cmd.bindVertexBuffers(0, vertexBuffer, {0});
        cmd.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);
        cmd.drawIndexed(indexCount, 1, 0, 0, 0);
    }
}
