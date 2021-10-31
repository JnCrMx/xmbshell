#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace render
{
	vk::UniqueShaderModule createShader(vk::Device device, std::string file);
	vk::UniqueShaderModule createShader(vk::Device device, unsigned char* p, unsigned int size);
}
