#include "render/model.hpp"

namespace render
{
	std::array<vk::VertexInputAttributeDescription, 3> vertex_data::attributes(uint32_t binding)
	{
		return {
			vk::VertexInputAttributeDescription(0, binding, vk::Format::eR32G32B32Sfloat, offsetof(vertex_data, position)),
			vk::VertexInputAttributeDescription(1, binding, vk::Format::eR32G32B32Sfloat, offsetof(vertex_data, normal)),
			vk::VertexInputAttributeDescription(2, binding, vk::Format::eR32G32Sfloat, offsetof(vertex_data, texCoord)),
		};
	}

	model::model(vk::Device device, vma::Allocator allocator) : device(device), allocator(allocator)
	{

	}

	model::~model()
	{
		allocator.destroyBuffer(vertexBuffer, vertexAllocation);
		allocator.destroyBuffer(indexBuffer, indexAllocation);
	}

	void model::create_buffers(int vc, int ic)
	{
		vertexCount = vc;
		indexCount = ic;

		vk::BufferCreateInfo vertex_info({}, sizeof(vertex_data)*vertexCount, 
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive);
		vk::BufferCreateInfo index_info({}, sizeof(uint32_t)*indexCount, 
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive);
		vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eGpuOnly);

		auto [vb, va] = allocator.createBuffer(vertex_info, alloc_info); vertexBuffer = vb; vertexAllocation = va;
		auto [ib, ia] = allocator.createBuffer(index_info, alloc_info); indexBuffer = ib; indexAllocation = ia;
	}
}