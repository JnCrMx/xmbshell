#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <chrono>

namespace config
{
	class config
	{
		public:
			vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifoRelaxed; //aka VSync
			vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e2; // aka Anti-aliasing

			int maxFPS = 100;
			std::chrono::duration<double> frameTime = std::chrono::duration<double>(std::chrono::seconds(1))/maxFPS;
	};
	inline class config CONFIG;
}
