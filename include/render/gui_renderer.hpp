#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include "render/components/font_renderer.hpp"
#include "render/components/image_renderer.hpp"
#include "render/texture.hpp"

namespace render
{
    class gui_renderer
    {
        public:
            gui_renderer(vk::CommandBuffer commandBuffer, int frame,
                vk::Extent2D frameSize,
                font_renderer* fontRenderer, image_renderer* imageRenderer);
            const vk::Extent2D frame_size;
            const double aspect_ratio;

            void set_color(glm::vec4 color);
            void set_alpha(float alpha);
            void reset_color();

            void draw_text(std::string_view text, float x, float y, float scale = 1.0f,
                glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0),
                bool centerH = false, bool centerV = false);

            void draw_image(const texture& texture, float x, float y, float scaleX = 1.0f, float scaleY = 1.0f,
                glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
            void draw_image(vk::ImageView view, float x, float y, float scaleX = 1.0f, float scaleY = 1.0f,
                glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
            void draw_image_sized(const texture& texture, float x, float y, int width = -1, int height = -1,
                glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
            void draw_image_sized(vk::ImageView view, float x, float y, int width, int height,
                glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0));
        private:
            font_renderer* m_fontRenderer;
            image_renderer* m_imageRenderer;
            vk::CommandBuffer m_commandBuffer;
            int m_frame;

            glm::vec4 m_color = glm::vec4(1.0, 1.0, 1.0, 1.0);
    };
}
