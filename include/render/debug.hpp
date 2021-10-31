#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include "render/texture.hpp"

namespace render
{
	template<class T>
	void debugName(vk::Device device, T object, std::string name)
	{
#ifndef NDEBUG
		using CType = typename T::CType;
		vk::DebugUtilsObjectNameInfoEXT name_info(object.objectType, (uint64_t)((CType)object), name.c_str());
		vk::Result result = device.setDebugUtilsObjectNameEXT(&name_info);
		if(result != vk::Result::eSuccess)
			throw std::runtime_error("naming failed: "+vk::to_string(result));
#endif
	}

	enum debug_tag : uint64_t
	{
		TextureSrc
	};

	template<class T>
	void debugTag(vk::Device device, T object, debug_tag tag, std::string value)
	{
#ifndef NDEBUG
		using CType = typename T::CType;
		vk::DebugUtilsObjectTagInfoEXT tag_info(object.objectType, (uint64_t)((CType)object), (uint64_t)tag, value.size()+1, value.c_str());
		vk::Result result = device.setDebugUtilsObjectTagEXT(&tag_info);
		if(result != vk::Result::eSuccess)
			throw std::runtime_error("tagging failed: "+vk::to_string(result));
#endif
	}
}