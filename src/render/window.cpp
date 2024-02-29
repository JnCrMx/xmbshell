#include "render/window.hpp"
#include "constants.hpp"
#include "config.hpp"

#include "render/debug.hpp"

#include <spdlog/spdlog.h>

#include <cxxabi.h>

#include <iterator>
#include <map>
#include <set>
#include <stdexcept>

using namespace config;

namespace render
{
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		vk::DebugUtilsMessageSeverityFlagBitsEXT severity = (vk::DebugUtilsMessageSeverityFlagBitsEXT) messageSeverity;
		vk::DebugUtilsMessageTypeFlagsEXT type(messageType);

		if(type==vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral &&
			severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose &&
		 	pCallbackData->messageIdNumber == 0)
			return VK_FALSE;

		switch(severity)
		{
			case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
				spdlog::error("[Vulkan Validation] {}", pCallbackData->pMessage);
				break;
			case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
				spdlog::info("[Vulkan Validation] {}", pCallbackData->pMessage);
				break;
			case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
				spdlog::warn("[Vulkan Validation] {}", pCallbackData->pMessage);
				break;
			case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
				spdlog::debug("[Vulkan Validation] {}", pCallbackData->pMessage);
				break;
		}

		return VK_FALSE;
	}

	void window::init()
	{
		initWindow();
		initVulkan();
	}

	void window::initWindow()
	{
		glfwInit();

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();

		int videoModeCount;
		const GLFWvidmode* modes = glfwGetVideoModes(monitor, &videoModeCount);
		const GLFWvidmode* mode = std::max_element(modes, modes+videoModeCount, [](GLFWvidmode a, GLFWvidmode b){
			return a.width*a.height*a.refreshRate < b.width*b.height*b.refreshRate; // find maximal video mode
		});

		const char* name = glfwGetMonitorName(monitor);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		win = std::unique_ptr<GLFWwindow, glfw_window_deleter>(glfwCreateWindow(mode->width, mode->height, constants::displayname.c_str(), monitor, nullptr));
		spdlog::info("Created window ({}x{} @ {} Hz R{}G{}B{}) on monitor \"{}\"", mode->width, mode->height, mode->refreshRate,
			mode->redBits, mode->greenBits, mode->blueBits, name);

		window_width = mode->width;
		window_height = mode->height;
	}

	void window::initVulkan()
	{
		vk::DynamicLoader dl;
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>( "vkGetInstanceProcAddr" );
		VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );

		std::vector<const char*> layers = {
#ifndef NDEBUG
			"VK_LAYER_KHRONOS_validation"
#endif
		};
		std::vector<const char*> extensions = {
#ifndef NDEBUG
			"VK_EXT_debug_utils"
#endif
		};

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::copy(glfwExtensions, glfwExtensions+glfwExtensionCount, std::back_inserter(extensions));

		auto const app = vk::ApplicationInfo()
			.setPApplicationName(constants::name.c_str())
			.setApplicationVersion(constants::version)
			.setPEngineName(constants::name.c_str())
			.setEngineVersion(constants::version)
			.setApplicationVersion(VK_VERSION_1_0);
		auto const inst_info = vk::InstanceCreateInfo()
			.setPApplicationInfo(&app)
			.setPEnabledLayerNames(layers)
			.setPEnabledExtensionNames(extensions);
		instance = vk::createInstanceUnique(inst_info);
		VULKAN_HPP_DEFAULT_DISPATCHER.init(instance.get());

#ifndef NDEBUG
		debugMessenger = instance->createDebugUtilsMessengerEXTUnique(vk::DebugUtilsMessengerCreateInfoEXT({},
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
				debugCallback));
#endif
		spdlog::info("Initialized Vulkan with {} layer(s) and {} extension(s)", layers.size(), extensions.size());

		{
			vk::SurfaceKHR surface_;
			glfwCreateWindowSurface(instance.get(), win.get(), nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface_));
			vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderDynamic> deleter(instance.get(), nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
			surface = vk::UniqueSurfaceKHR(surface_, deleter);
		}

		auto devices = instance->enumeratePhysicalDevices();
		if(devices.empty())
		{
			spdlog::error("Found no video device with Vulkan support!");
			throw std::runtime_error("no vulkan device");
		}
		spdlog::debug("Found {} video devices", devices.size());

		std::multimap<int, vk::PhysicalDevice> candidates;
		for(auto& dev : devices)
		{
			int score = rateDeviceSuitability(dev);
			candidates.insert({score, dev});
		}
		if(candidates.rbegin()->first > 0)
		{
			physicalDevice = candidates.rbegin()->second;
		}
		else
		{
			spdlog::error("Found suitable video device!");
			throw std::runtime_error("no suitable device");
		}

		deviceProperties = physicalDevice.getProperties();
		queueFamilyIndices = findQueueFamilies(physicalDevice);
		swapchainSupport = querySwapChainSupport(physicalDevice);
		spdlog::info("Using video device {} of type {}", deviceProperties.deviceName, vk::to_string(deviceProperties.deviceType));

		std::vector<vk::DeviceQueueCreateInfo> queueInfos;
		std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value(),
			queueFamilyIndices.transferFamily.value_or(UINT32_MAX)};
		auto families = physicalDevice.getQueueFamilyProperties();

		std::vector<float> priorities(std::max_element(families.begin(), families.end(), [](auto a, auto b){
			return a.queueCount < b.queueCount;
		})->queueCount);
		std::fill(priorities.begin(), priorities.end(), 1.0f);
		for(uint32_t queueFamily : uniqueQueueFamilies)
		{
			if(queueFamily == UINT32_MAX)
				continue;

			queueInfos.emplace_back()
				.setQueueFamilyIndex(queueFamily)
				.setQueueCount(families[queueFamily].queueCount)
				.setPQueuePriorities(priorities.data());
		}

		vk::PhysicalDeviceFeatures features = vk::PhysicalDeviceFeatures()
			.setGeometryShader(true)
			.setSampleRateShading(true)
			.setFillModeNonSolid(true)
			.setWideLines(true);
		const std::vector<const char*> deviceExtensions = {
    		VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		vk::DeviceCreateInfo device_info = vk::DeviceCreateInfo()
			.setQueueCreateInfos(queueInfos)
			.setPEnabledFeatures(&features)
			.setPEnabledLayerNames(layers)
			.setPEnabledExtensionNames(deviceExtensions);

		device = physicalDevice.createDeviceUnique(device_info);
		VULKAN_HPP_DEFAULT_DISPATCHER.init(device.get());
		graphicsQueue = device->getQueue(queueFamilyIndices.graphicsFamily.value(), 0);
		presentQueue = device->getQueue(queueFamilyIndices.presentFamily.value(), 0);
		if(queueFamilyIndices.transferFamily.has_value())
		{
			int index = queueFamilyIndices.transferFamily.value();
			for(int i=0; i<families[index].queueCount; i++)
			{
				transferQueues.push_back(device->getQueue(index, i));
			}
		}
		else
		{
			transferQueues.push_back(graphicsQueue);
		}

		vma::AllocatorCreateInfo allocator_info({}, physicalDevice, device.get());
		allocator_info.setInstance(instance.get());
		vma::VulkanFunctions functs{vkGetInstanceProcAddr, vkGetDeviceProcAddr};
		allocator_info.setPVulkanFunctions(&functs);
		allocator = vma::createAllocator(allocator_info);

		loader = std::make_unique<resource_loader>(device.get(), allocator,
			queueFamilyIndices.transferFamily.value_or(queueFamilyIndices.graphicsFamily.value()),
			queueFamilyIndices.graphicsFamily.value(),
			transferQueues);

		auto formatIt = std::find_if(swapchainSupport.formats.begin(), swapchainSupport.formats.end(), [](auto f){
			return f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
		});
		swapchainFormat = formatIt == swapchainSupport.formats.end() ? swapchainSupport.formats[0] : *formatIt;

		auto presentModeIt = std::find_if(swapchainSupport.presentModes.begin(), swapchainSupport.presentModes.end(), [](auto p){
			return p == CONFIG.preferredPresentMode;
		});
		swapchainPresentMode = presentModeIt == swapchainSupport.presentModes.end() ? swapchainSupport.presentModes[0] : *presentModeIt;

		swapchainExtent = vk::Extent2D{
			std::clamp(window_width, swapchainSupport.capabilities.minImageExtent.width, swapchainSupport.capabilities.maxImageExtent.width),
			std::clamp(window_height, swapchainSupport.capabilities.minImageExtent.height, swapchainSupport.capabilities.maxImageExtent.height)
		};

		swapchainImageCount = swapchainSupport.capabilities.minImageCount + 1;
		if(swapchainSupport.capabilities.maxImageCount > 0)
			swapchainImageCount = std::min(swapchainImageCount, swapchainSupport.capabilities.maxImageCount);

		spdlog::debug("Swapchain of format {}, present mode {}, extent {}x{} and {} images",
			vk::to_string(swapchainFormat.format), vk::to_string(swapchainPresentMode),
			swapchainExtent.width, swapchainExtent.height, swapchainImageCount);

		vk::SwapchainCreateInfoKHR swapchain_info({}, surface.get(), swapchainImageCount,
			swapchainFormat.format, swapchainFormat.colorSpace,
			swapchainExtent, 1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst);
		swapchain_info.setPresentMode(swapchainPresentMode);
		if(swapchainSupport.capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
			swapchain_info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::ePreMultiplied);
		swapchain = device->createSwapchainKHRUnique(swapchain_info);
		swapchainImages = device->getSwapchainImagesKHR(swapchain.get());

		for(auto& image : swapchainImages)
		{
			vk::ImageViewCreateInfo view_info({}, image, vk::ImageViewType::e2D, swapchainFormat.format,
				vk::ComponentMapping{}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
			swapchainImageViews.push_back(device->createImageViewUnique(view_info));
			swapchainImageViewsRaw.push_back(swapchainImageViews.back().get());

			imageAvailableSemaphores.push_back(device->createSemaphoreUnique(vk::SemaphoreCreateInfo()));
			renderFinishedSemaphores.push_back(device->createSemaphoreUnique(vk::SemaphoreCreateInfo()));
			fences.push_back(device->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)));

			inFlightFences.push_back(fences.back().get());
			imagesInFlight.push_back(vk::Fence());
		}

#ifndef NDEBUG
		debugName(device.get(), graphicsQueue, "Graphics Queue");
		debugName(device.get(), presentQueue, "Present Queue");
		for(int i=0; i<swapchainImageCount; i++)
		{
			debugName(device.get(), imageAvailableSemaphores[i].get(), "Image available Semaphore #"+std::to_string(i));
			debugName(device.get(), renderFinishedSemaphores[i].get(), "Render finished Semaphore #"+std::to_string(i));
			debugName(device.get(), fences[i].get(), "Render Fence #"+std::to_string(i));
		}
#endif
	}

	void window::set_phase(phase* renderer)
	{
		current_renderer.reset(renderer);

		auto t0 = std::chrono::high_resolution_clock::now();
		current_renderer->preload();
		auto tPreload = std::chrono::high_resolution_clock::now();
		current_renderer->prepare(swapchainImages, swapchainImageViewsRaw);
		auto tPrepare = std::chrono::high_resolution_clock::now();
		current_renderer->waitLoad(); // replace with loading screen
		auto tWaitLoad = std::chrono::high_resolution_clock::now();
		current_renderer->init();
		auto tInit = std::chrono::high_resolution_clock::now();

		auto dPreload = std::chrono::duration_cast<std::chrono::milliseconds>(tPreload - t0).count();
		auto dPrepare = std::chrono::duration_cast<std::chrono::milliseconds>(tPrepare - tPreload).count();
		auto dWaitLoad = std::chrono::duration_cast<std::chrono::milliseconds>(tWaitLoad - tPrepare).count();
		auto dInit = std::chrono::duration_cast<std::chrono::milliseconds>(tInit - tWaitLoad).count();
		auto dTotal = std::chrono::duration_cast<std::chrono::milliseconds>(tInit - t0).count();

		auto& type = typeid(*renderer);
		char* name = abi::__cxa_demangle(type.name(), 0, 0, 0);

		spdlog::debug("Timing for phase \"{}\": preload/prepare/load/init/total: {}/{}/{}/{}/{} ms", name, dPreload, dPrepare, dWaitLoad, dInit, dTotal);
	}

	int window::rateDeviceSuitability(vk::PhysicalDevice phyDev)
	{
		int score = 0;

		auto props = phyDev.getProperties();
		if(props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			score += 1000;
		if(props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
			score += 500;
		score += props.limits.maxImageDimension2D;

		if(!findQueueFamilies(phyDev).isComplete())
			score = 0;

		return score;
	}

	QueueFamilyIndices window::findQueueFamilies(vk::PhysicalDevice phyDev)
	{
		QueueFamilyIndices indices{};

		auto families = phyDev.getQueueFamilyProperties();

		unsigned index = 0;
		for(auto& family : families)
		{
			if(family.queueFlags & vk::QueueFlagBits::eGraphics)
				indices.graphicsFamily = index;
			else if(family.queueFlags & vk::QueueFlagBits::eTransfer)
				indices.transferFamily = index;
			if(phyDev.getSurfaceSupportKHR(index, surface.get()))
				indices.presentFamily = index;
			index++;
		}

		return indices;
	}

	SwapChainSupportDetails window::querySwapChainSupport(vk::PhysicalDevice phyDev)
	{
		SwapChainSupportDetails details;
		details.capabilities = phyDev.getSurfaceCapabilitiesKHR(surface.get());
		details.formats = phyDev.getSurfaceFormatsKHR(surface.get());
		details.presentModes = phyDev.getSurfacePresentModesKHR(surface.get());

		return details;
	}

	void window::loop()
	{
		lastFrame = std::chrono::high_resolution_clock::now();
		lastFPS = std::chrono::high_resolution_clock::now();
		framesInSecond = 0;

		int currentFrame = 0;
		vk::Result r;
		while(!glfwWindowShouldClose(win.get()))
		{
			glfwPollEvents();

			auto now = std::chrono::high_resolution_clock::now();
			auto dt = std::chrono::duration<double>(now - lastFrame).count();
			lastFrame = now;

			auto sleep = CONFIG.frameTime - std::chrono::duration<double>(now - lastFrame);
			std::this_thread::sleep_for(sleep);

			r = device->waitForFences(inFlightFences[currentFrame], true, UINT64_MAX);
			if(r != vk::Result::eSuccess)
				spdlog::error("Waiting for inFlightFences[{}] failed with result {}", currentFrame, vk::to_string(r));

			auto [result, imageIndex] = device->acquireNextImageKHR(swapchain.get(), UINT64_MAX, imageAvailableSemaphores[currentFrame].get());
			if(imagesInFlight[imageIndex])
			{
				r = device->waitForFences(imagesInFlight[imageIndex], true, UINT64_MAX);
				if(r != vk::Result::eSuccess)
					spdlog::error("Waiting for imagesInFlight[{}] failed with result {}", imageIndex, vk::to_string(r));
			}
			imagesInFlight[imageIndex] = inFlightFences[currentFrame];

			device->resetFences(fences[currentFrame].get());
			current_renderer->render(imageIndex, imageAvailableSemaphores[currentFrame].get(), renderFinishedSemaphores[currentFrame].get(), inFlightFences[currentFrame]);

			vk::PresentInfoKHR present_info(renderFinishedSemaphores[currentFrame].get(), swapchain.get(), imageIndex);
			r = presentQueue.presentKHR(present_info);
			if(r != vk::Result::eSuccess)
				spdlog::error("Present failed with result {}", vk::to_string(r));

			currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

			{
				framesInSecond++;
				auto t = std::chrono::high_resolution_clock::now();
				using namespace std::chrono_literals;
				if((t-lastFPS) > (1000ms/fpsSampleRate))
				{
					currentFPS = framesInSecond / std::chrono::duration<double>(t-lastFPS).count();
					lastFPS = t;
					framesInSecond = 0;

					if(fpsCount%fpsSampleRate == 0)
					{
						spdlog::debug("{} FPS", currentFPS);
						fpsCount = 0;
					}
					fpsCount++;
				}
			}
		}
		graphicsQueue.waitIdle();
		presentQueue.waitIdle();
	}

	window::~window()
	{
		device->waitIdle();

		current_renderer.reset();
		loader.reset();

		allocator.destroy();
	}
}
