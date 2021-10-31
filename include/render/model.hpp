#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include <glm/glm.hpp>

namespace render
{
	struct vertex_data
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;

		static std::array<vk::VertexInputAttributeDescription, 3> attributes(uint32_t binding);
	};

	struct model
	{
		model(vk::Device device, vma::Allocator allocator);
		~model();

		vk::Device device;
		vma::Allocator allocator;

		vk::Buffer vertexBuffer;
		vma::Allocation vertexAllocation;
		vk::Buffer indexBuffer;
		vma::Allocation indexAllocation;

		int vertexCount;
		int indexCount;

		void create_buffers(int vertexCount, int indexCount);

		glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
	};
}
