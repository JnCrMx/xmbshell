#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>

namespace render
{
	struct texture
	{
		texture(vk::Device device, vma::Allocator allocator, int width, int height, 
			vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled,
			vk::Format format = vk::Format::eR8G8B8A8Srgb,
			vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1,
			bool transfer = true, vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eColor);
		texture(vk::Device device, vma::Allocator allocator, vk::Extent2D extent, 
			vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled,
			vk::Format format = vk::Format::eR8G8B8A8Srgb,
			vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1,
			bool transfer = true, vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eColor) 
			: texture(device, allocator, extent.width, extent.height, usage, format, sampleCount, transfer, aspects) {}
		
		texture(vk::Device device, vma::Allocator allocator, 
			vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled,
			vk::Format format = vk::Format::eR8G8B8A8Srgb,
			vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1,
			bool transfer = true, vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eColor);
		~texture();
		void create_image(int width, int height);
		void name(std::string name);

		vk::Device device;
		vma::Allocator allocator;

		vk::Image image;
		vma::Allocation allocation;

		int width;
		int height;

		vk::UniqueImageView imageView;

		private:
		vk::ImageCreateInfo image_info;
		vk::ImageViewCreateInfo view_info;
	};
}