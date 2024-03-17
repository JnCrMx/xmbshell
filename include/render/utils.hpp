#pragma once

#include <filesystem>
#include <span>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace render
{
	vk::UniqueShaderModule createShader(vk::Device device, const std::filesystem::path& path);
	vk::UniqueShaderModule createShader(vk::Device device, std::span<const uint32_t> code);
}
