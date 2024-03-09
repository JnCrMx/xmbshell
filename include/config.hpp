#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <glm/vec3.hpp>
#include <spdlog/spdlog.h>

#include <chrono>
#include <limits>

namespace config
{
	class config
	{
		public:
			config() = default;

			void load();

			vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifoRelaxed; //aka VSync
			vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e4; // aka Anti-aliasing
			void setSampleCount(vk::SampleCountFlagBits count) {
				sampleCount = count;
			}

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

			bool showFPS;
			bool showMemory;

			std::string fontPath;
			void setFontPath(std::string path) {
				fontPath = path;
			}
			glm::vec3 waveColor;
			void setWaveColor(glm::vec3 color) {
				waveColor = color;
			}
			void setWaveColor(std::string_view hex) {
				if(hex.size() != 7 || hex[0] != '#') {
					spdlog::error("Ignoring invalid hex wave-color: {}", hex);
					return;
				}
				auto parseHex = [](std::string_view hex) -> std::optional<unsigned int> {
					unsigned int value;
					if(std::from_chars(hex.data(), hex.data() + hex.size(), value, 16).ec == std::errc()) {
						return value;
					}
					return std::nullopt;
				};
				waveColor = glm::vec3(
					parseHex(hex.substr(1, 2)).value_or(0) / 255.0f,
					parseHex(hex.substr(3, 2)).value_or(0) / 255.0f,
					parseHex(hex.substr(5, 2)).value_or(0) / 255.0f
				);
			}
			void setWaveColor(const std::string& hex) {
				setWaveColor(std::string_view(hex));
			}
	};
	inline class config CONFIG;
}
