#include "render/utils.hpp"
#include "render/debug.hpp"

#include <fstream>

namespace render
{
	vk::UniqueShaderModule createShader(vk::Device device, const std::filesystem::path& path)
	{
		std::ifstream in(path, std::ios_base::binary | std::ios_base::ate);

		size_t size = in.tellg();
		std::vector<uint32_t> buf(size/sizeof(uint32_t));
		in.seekg(0);
		in.read(reinterpret_cast<char*>(buf.data()), size);

		vk::ShaderModuleCreateInfo shader_info({}, size, buf.data());
		vk::UniqueShaderModule shader = device.createShaderModuleUnique(shader_info);
		debugName(device, shader.get(), "Shader Module \""+path.filename().string()+"\"");
		return std::move(shader);
	}

	vk::UniqueShaderModule createShader(vk::Device device, std::span<const uint32_t> code)
	{
		vk::ShaderModuleCreateInfo shader_info({}, code.size()*sizeof(decltype(code)::element_type), code.data());
		vk::UniqueShaderModule shader = device.createShaderModuleUnique(shader_info);
		return std::move(shader);
	}

	UniquePipelineMap createPipelines(
		vk::Device device,
		vk::PipelineCache pipelineCache,
		const vk::GraphicsPipelineCreateInfo& createInfo,
		const std::vector<vk::RenderPass>& renderPasses,
		const std::string& debugName)
	{
		std::vector<vk::GraphicsPipelineCreateInfo> createInfos(renderPasses.size(), createInfo);
		createInfos[0].flags |= vk::PipelineCreateFlagBits::eAllowDerivatives;
		createInfos[0].renderPass = renderPasses[0];
		createInfos[0].basePipelineIndex = -1;

		for (size_t i = 1; i < renderPasses.size(); ++i)
		{
			createInfos[i].flags |= vk::PipelineCreateFlagBits::eDerivative;
			createInfos[i].renderPass = renderPasses[i];
			createInfos[i].basePipelineHandle = nullptr;
			createInfos[i].basePipelineIndex = 0;
		}
		auto result = device.createGraphicsPipelinesUnique(
			pipelineCache, createInfos, nullptr);
		if(result.result != vk::Result::eSuccess)
			throw std::runtime_error("Failed to create graphics pipeline(s) (error code: "+to_string(result.result)+")");

		UniquePipelineMap pipelines;
		for (size_t i = 0; i < renderPasses.size(); ++i)
		{
			render::debugName(device, result.value[i].get(), debugName);
			pipelines[renderPasses[i]] = std::move(result.value[i]);
		}
		return pipelines;
	}
}
