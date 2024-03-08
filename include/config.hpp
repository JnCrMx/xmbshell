#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <chrono>
#include <limits>

namespace config
{
	class config
	{
		public:
			vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifoRelaxed; //aka VSync
			vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e4; // aka Anti-aliasing

			double maxFPS = 100;
			std::chrono::duration<double> frameTime = std::chrono::duration<double>(std::chrono::seconds(1))/maxFPS;
			void setMaxFPS(double fps) {
				if(fps <= 0) {
					maxFPS = std::numeric_limits<double>::max();
					frameTime = std::chrono::duration<double>(0);
					return;
				}
				maxFPS = fps;
				frameTime = std::chrono::duration<double>(std::chrono::seconds(1))/maxFPS;
			}
	};
	inline class config CONFIG;
}
