#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include "render/font_renderer.hpp"
#include "render/texture.hpp"

namespace render
{
    class gui_renderer
    {
        public:
            gui_renderer(vk::CommandBuffer commandBuffer, int frame, font_renderer* fontRenderer);

            void draw_text(std::string_view text, float x, float y, float scale = 1.0f, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
            void draw_image(const texture& texture, float x, float y, float w, float h);
        private:
            font_renderer* m_fontRenderer;
            vk::CommandBuffer m_commandBuffer;
            int m_frame;
    };
}
