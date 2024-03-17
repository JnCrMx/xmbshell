#pragma once

#include "constants.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <glm/vec3.hpp>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>

namespace config
{
    class config
    {
        public:
            config() = default;

            enum class background_type {
                wave, color, image
            };

            std::filesystem::path exe_directory = std::filesystem::canonical("/proc/self/exe").parent_path();
            std::filesystem::path asset_directory = [this](){
                if(auto v = std::getenv("XMB_ASSET_DIR"); v != nullptr) {
                    return std::filesystem::path(v);
                }
                return exe_directory / std::string(constants::asset_directory);
            }();
            std::filesystem::path fallback_font = exe_directory / std::string(constants::fallback_font);

            vk::PresentModeKHR      preferredPresentMode = vk::PresentModeKHR::eFifoRelaxed; //aka VSync
            vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e4; // aka Anti-aliasing

            double                          maxFPS = 100;
            std::chrono::duration<double>   frameTime = std::chrono::duration<double>(std::chrono::seconds(1))/maxFPS;

            bool showFPS;
            bool showMemory;

            std::filesystem::path   fontPath;
            background_type			backgroundType;
            glm::vec3               backgroundColor;
            std::filesystem::path   backgroundImage;
            glm::vec3               waveColor;
            std::string             dateTimeFormat = constants::fallback_datetime_format;
            double                  dateTimeOffset = 0.0;

            bool controllerRumble;
            bool controllerAnalogStick;

            void load();

            void setSampleCount(vk::SampleCountFlagBits count);
            void setMaxFPS(double fps);
            void setFontPath(std::string path);
            void setBackgroundType(background_type type);
            void setBackgroundType(std::string_view type);
            void setBackgroundType(const std::string& type);
            void setBackgroundColor(glm::vec3 color);
            void setBackgroundColor(std::string_view hex);
            void setBackgroundColor(const std::string& hex);
            void setWaveColor(glm::vec3 color);
            void setWaveColor(std::string_view hex);
            void setWaveColor(const std::string& hex);
            void setDateTimeFormat(const std::string& format);
    };
    inline class config CONFIG;
}
