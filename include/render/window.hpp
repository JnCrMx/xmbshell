#pragma once

import vulkan_hpp;
#include <vk_mem_alloc.hpp>

#include <SDL.h>

#include <chrono>
#include <map>
#include <memory>
#include <optional>

#include "phase.hpp"
#include "resource_loader.hpp"
#include "support/sdl.hpp"

import xmbshell.input;

namespace render
{
	struct QueueFamilyIndices {
    	std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		std::optional<uint32_t> transferFamily;

		bool isComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	class window
	{
		public:
			window() {}
			~window();

			void init();
			void loop();

			void set_phase(phase* renderer, input::keyboard_handler* keyboard_handler = nullptr, input::controller_handler* controller_handler = nullptr);

			std::unique_ptr<resource_loader> loader;

			std::unique_ptr<phase> current_renderer;
			input::keyboard_handler* keyboard_handler;
			input::controller_handler* controller_handler;

            static sdl::initializer sdl_init;
            sdl::unique_window win;
			uint32_t window_width, window_height;

			vk::UniqueInstance instance;
			vk::UniqueSurfaceKHR surface;

			vk::PhysicalDevice physicalDevice;

			vk::PhysicalDeviceProperties deviceProperties;
			QueueFamilyIndices queueFamilyIndices;
			SwapChainSupportDetails swapchainSupport;

			vk::UniqueDevice device;
			vma::Allocator allocator;

			vk::Queue graphicsQueue;
			vk::Queue presentQueue;
			std::vector<vk::Queue> transferQueues;

			vk::SurfaceFormatKHR swapchainFormat;
			vk::PresentModeKHR swapchainPresentMode;
			vk::Extent2D swapchainExtent;
			uint32_t swapchainImageCount;
			vk::UniqueSwapchainKHR swapchain;

			std::vector<vk::Image> swapchainImages;
			std::vector<vk::UniqueImageView> swapchainImageViews;
			std::vector<vk::ImageView> swapchainImageViewsRaw;

			const int MAX_FRAMES_IN_FLIGHT = 2;
			std::vector<vk::UniqueSemaphore> imageAvailableSemaphores;
			std::vector<vk::UniqueSemaphore> renderFinishedSemaphores;
			std::vector<vk::UniqueFence> fences;

			std::vector<vk::Fence> imagesInFlight;
			std::vector<vk::Fence> inFlightFences;

			vk::UniquePipelineCache pipelineCache;

			decltype(std::chrono::high_resolution_clock::now()) startTime;
			decltype(std::chrono::high_resolution_clock::now()) lastFrame;

			static constexpr int fpsSampleRate = 10;
			uint64_t framesInSecond{};
			decltype(std::chrono::high_resolution_clock::now()) lastFPS;
			int fpsCount{};
			double currentFPS{};
			int refreshRate{};

			struct sdl_controller_closer {
				void operator()(SDL_GameController* ptr) const {
					SDL_GameControllerClose(ptr);
				}
			};
			std::map<SDL_JoystickID, std::unique_ptr<SDL_GameController, sdl_controller_closer>> controllers;

#ifndef NDEBUG
			vk::UniqueDebugUtilsMessengerEXT debugMessenger;
#endif
		private:
			void initWindow();
			void initInput();
			void initAudio();
			void initVulkan();

			int rateDeviceSuitability(vk::PhysicalDevice phyDev);
			QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice phyDev);
			SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice phyDev);
	};
}
