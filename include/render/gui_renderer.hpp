#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include "render/font_renderer.hpp"
#include "render/image_renderer.hpp"
#include "render/texture.hpp"

namespace render
{
    class gui_renderer
    {
        public:
            gui_renderer(vk::CommandBuffer commandBuffer, int frame, font_renderer* fontRenderer, image_renderer* imageRenderer);

            void draw_text(std::string_view text, float x, float y, float scale = 1.0f, glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));

            void draw_image(const texture& texture, float x, float y, float scaleX = 1.0f, float scaleY = 1.0f);
            void draw_image(vk::ImageView view, float x, float y, float scaleX = 1.0f, float scaleY = 1.0f);
            void draw_image_sized(const texture& texture, float x, float y, int width = -1, int height = -1);
            void draw_image_sized(vk::ImageView view, float x, float y, int width, int height);
        private:
            font_renderer* m_fontRenderer;
            image_renderer* m_imageRenderer;
            vk::CommandBuffer m_commandBuffer;
            int m_frame;
    };
}
