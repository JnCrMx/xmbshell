#pragma once

#include "constants.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <glm/vec3.hpp>
#include <spdlog/spdlog.h>

#include <chrono>

namespace config
{
	class config
	{
		public:
			config() = default;

			std::string asset_directory = [](){
				if(auto v = std::getenv("XMB_ASSET_DIR"); v != nullptr) {
					return std::string(v);
				}
				return std::string(constants::asset_directory);
			}();

			vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifoRelaxed; //aka VSync
			vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e4; // aka Anti-aliasing

			double maxFPS = 100;
			std::chrono::duration<double> frameTime = std::chrono::duration<double>(std::chrono::seconds(1))/maxFPS;

			bool showFPS;
			bool showMemory;

			std::string fontPath;
			glm::vec3 waveColor;
			std::string dateTimeFormat = constants::fallback_datetime_format;
			double dateTimeOffset = 0.0;

			bool controllerRumble;

			void load();

			void setSampleCount(vk::SampleCountFlagBits count);
			void setMaxFPS(double fps);
			void setFontPath(std::string path);
			void setWaveColor(glm::vec3 color);
			void setWaveColor(std::string_view hex);
			void setWaveColor(const std::string& hex);
			void setDateTimeFormat(const std::string& format);
	};
	inline class config CONFIG;
}
