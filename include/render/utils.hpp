#pragma once

#include <filesystem>
#include <map>
#include <span>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace render
{
	vk::UniqueShaderModule createShader(vk::Device device, const std::filesystem::path& path);
	vk::UniqueShaderModule createShader(vk::Device device, std::span<const uint32_t> code);

	using UniquePipelineMap = std::map<vk::RenderPass, vk::UniquePipeline>;

	UniquePipelineMap createPipelines(
		vk::Device device,
		vk::PipelineCache pipelineCache,
		const vk::GraphicsPipelineCreateInfo& createInfo,
		const std::vector<vk::RenderPass>& renderPasses,
		const std::string& debugName);
}
