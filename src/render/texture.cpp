#include "render/texture.hpp"
#include "render/debug.hpp"

namespace render
{
	texture::texture(vk::Device device, vma::Allocator allocator, int width, int height,
		vk::ImageUsageFlags usage, vk::Format format, vk::SampleCountFlagBits sampleCount, bool transfer, vk::ImageAspectFlags aspects) 
		: device(device), allocator(allocator), width(width), height(height)
	{
		vk::ImageCreateInfo image_info({}, vk::ImageType::e2D, format, 
			{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1}, 1, 1, 
			sampleCount, vk::ImageTiling::eOptimal,
			usage | (transfer?vk::ImageUsageFlagBits::eTransferDst:vk::ImageUsageFlagBits{}), 
			vk::SharingMode::eExclusive);
		vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eGpuOnly);
		
		auto [i, a] = allocator.createImage(image_info, alloc_info);
		image = i;
		allocation = a;

		vk::ImageViewCreateInfo view_info({}, image, vk::ImageViewType::e2D, format, 
			vk::ComponentMapping(), vk::ImageSubresourceRange(aspects, 0, 1, 0, 1));
		imageView = device.createImageViewUnique(view_info);
	}

	texture::texture(vk::Device device, vma::Allocator allocator,
		vk::ImageUsageFlags usage, vk::Format format, vk::SampleCountFlagBits sampleCount, bool transfer, vk::ImageAspectFlags aspects) 
		: device(device), allocator(allocator)
	{
		image_info = vk::ImageCreateInfo({}, vk::ImageType::e2D, format, 
			{0, 0, 1}, 1, 1,
			sampleCount, vk::ImageTiling::eOptimal,
			usage | (transfer?vk::ImageUsageFlagBits::eTransferDst:vk::ImageUsageFlagBits{}), 
			vk::SharingMode::eExclusive);
		view_info = vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, format, 
			vk::ComponentMapping(), vk::ImageSubresourceRange(aspects, 0, 1, 0, 1));
	}

	void texture::create_image(int width, int height)
	{
		if(imageView)
			return;

		this->width = width;
		this->height = height;
		image_info.extent = vk::Extent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

		vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eGpuOnly);
		auto [i, a] = allocator.createImage(image_info, alloc_info);
		image = i;
		allocation = a;

		imageView = device.createImageViewUnique(view_info);
	}

	void texture::name(std::string name)
	{
		debugName(device, image, name+" Image");
		debugName(device, imageView.get(), name+" Image View");
	}

	texture::~texture()
	{
		imageView.reset();
		allocator.destroyImage(image, allocation);
	}
}