module;

#include <array>
#include <cstdint>

module xmbshell.render;

import dreamrender;
import vulkan_hpp;

namespace render::shaders {

namespace blur {
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc23-extensions"
    constexpr char vert_array[] = {
    #embed "shaders/blur.vert.spv"
    };
    constexpr char frag_array[] = {
    #embed "shaders/blur.frag.spv"
    };
    #pragma clang diagnostic pop

    constexpr std::array vert_shader = dreamrender::convert<std::to_array(vert_array), uint32_t>();
    constexpr std::array frag_shader = dreamrender::convert<std::to_array(frag_array), uint32_t>();

    vk::UniqueShaderModule vert(vk::Device device) {
        return dreamrender::createShader(device, vert_shader);
    }
    vk::UniqueShaderModule frag(vk::Device device) {
        return dreamrender::createShader(device, frag_shader);
    }
}

namespace wave_renderer {
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc23-extensions"
    constexpr char vert_array[] = {
    #embed "shaders/wave.vert.spv"
    };
    constexpr char frag_array[] = {
    #embed "shaders/wave.frag.spv"
    };
    #pragma clang diagnostic pop

    constexpr std::array vert_shader = dreamrender::convert<std::to_array(vert_array), uint32_t>();
    constexpr std::array frag_shader = dreamrender::convert<std::to_array(frag_array), uint32_t>();

    vk::UniqueShaderModule vert(vk::Device device) {
        return dreamrender::createShader(device, vert_shader);
    }
    vk::UniqueShaderModule frag(vk::Device device) {
        return dreamrender::createShader(device, frag_shader);
    }
}

}
