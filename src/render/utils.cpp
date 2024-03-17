#include "render/utils.hpp"
#include "render/debug.hpp"

#include <fstream>

namespace render
{
	vk::UniqueShaderModule createShader(vk::Device device, const std::filesystem::path& path)
	{
		std::ifstream in(path, std::ios_base::binary | std::ios_base::ate);

		size_t size = in.tellg();
		std::vector<uint32_t> buf(size/sizeof(uint32_t));
		in.seekg(0);
		in.read(reinterpret_cast<char*>(buf.data()), size);

		vk::ShaderModuleCreateInfo shader_info({}, size, buf.data());
		vk::UniqueShaderModule shader = device.createShaderModuleUnique(shader_info);
		debugName(device, shader.get(), "Shader Module \""+path.filename().string()+"\"");
		return std::move(shader);
	}

	vk::UniqueShaderModule createShader(vk::Device device, std::span<const uint32_t> code)
	{
		vk::ShaderModuleCreateInfo shader_info({}, code.size()*sizeof(decltype(code)::element_type), code.data());
		vk::UniqueShaderModule shader = device.createShaderModuleUnique(shader_info);
		return std::move(shader);
	}
}
