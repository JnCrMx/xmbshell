#include "render/gui_renderer.hpp"

namespace render
{
    gui_renderer::gui_renderer(vk::CommandBuffer commandBuffer, int frame, font_renderer* fontRenderer, image_renderer* imageRenderer)
        : m_fontRenderer(fontRenderer), m_imageRenderer(imageRenderer), m_commandBuffer(commandBuffer), m_frame(frame)
    {
    }

    void gui_renderer::draw_text(std::string_view text, float x, float y, float scale, glm::vec4 color)
    {
        m_fontRenderer->renderText(m_commandBuffer, m_frame, text, x, y, scale, color);
    }

    void gui_renderer::draw_image(const texture& texture, float x, float y, float scaleX, float scaleY, glm::vec4 color)
    {
        m_imageRenderer->renderImage(m_commandBuffer, m_frame, texture, x, y, scaleX, scaleY, color);
    }
    void gui_renderer::draw_image(vk::ImageView view, float x, float y, float scaleX, float scaleY, glm::vec4 color)
    {
        m_imageRenderer->renderImage(m_commandBuffer, m_frame, view, x, y, scaleX, scaleY, color);
    }

    void gui_renderer::draw_image_sized(const texture& texture, float x, float y, int width, int height, glm::vec4 color)
    {
        m_imageRenderer->renderImageSized(m_commandBuffer, m_frame, texture, x, y, width, height, color);
    }
    void gui_renderer::draw_image_sized(vk::ImageView view, float x, float y, int width, int height, glm::vec4 color)
    {
        m_imageRenderer->renderImageSized(m_commandBuffer, m_frame, view, x, y, width, height, color);
    }
}
