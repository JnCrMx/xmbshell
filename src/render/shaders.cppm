module;

export module xmbshell.render:shaders;

import vulkan_hpp;

namespace render::shaders {

namespace blur {
    vk::UniqueShaderModule vert(vk::Device device);
    vk::UniqueShaderModule frag(vk::Device device);
}

namespace wave_renderer {
    vk::UniqueShaderModule vert(vk::Device device);
    vk::UniqueShaderModule frag(vk::Device device);
}

}
