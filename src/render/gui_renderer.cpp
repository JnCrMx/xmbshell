#include "render/gui_renderer.hpp"

namespace render
{
    gui_renderer::gui_renderer(vk::CommandBuffer commandBuffer, int frame, vk::Extent2D frameSize, font_renderer* fontRenderer, image_renderer* imageRenderer)
        : m_fontRenderer(fontRenderer), m_imageRenderer(imageRenderer), m_commandBuffer(commandBuffer), m_frame(frame),
        frame_size(frameSize), aspect_ratio(static_cast<double>(frameSize.width) / frameSize.height)
    {
    }

    void gui_renderer::set_color(glm::vec4 color)
    {
        m_color = color;
    }
    void gui_renderer::set_alpha(float alpha)
    {
        m_color.a = alpha;
    }
    void gui_renderer::reset_color()
    {
        m_color = glm::vec4(1.0, 1.0, 1.0, 1.0);
    }

    void gui_renderer::draw_text(std::string_view text, float x, float y, float scale, glm::vec4 color, bool centerH, bool centerV)
    {
        if(centerH || centerV) {
            glm::vec2 size = m_fontRenderer->measureText(text, scale);
            if(centerH)
                x -= size.x / 2.0f;
            if(centerV)
                y -= size.y / 2.0f;
        }
        m_fontRenderer->renderText(m_commandBuffer, m_frame, text, x, y, scale, color*m_color);
    }

    void gui_renderer::draw_image(const texture& texture, float x, float y, float scaleX, float scaleY, glm::vec4 color)
    {
        m_imageRenderer->renderImage(m_commandBuffer, m_frame, texture, x, y, scaleX, scaleY, color*m_color);
    }
    void gui_renderer::draw_image(vk::ImageView view, float x, float y, float scaleX, float scaleY, glm::vec4 color)
    {
        m_imageRenderer->renderImage(m_commandBuffer, m_frame, view, x, y, scaleX, scaleY, color*m_color);
    }

    void gui_renderer::draw_image_sized(const texture& texture, float x, float y, int width, int height, glm::vec4 color)
    {
        m_imageRenderer->renderImageSized(m_commandBuffer, m_frame, texture, x, y, width, height, color*m_color);
    }
    void gui_renderer::draw_image_sized(vk::ImageView view, float x, float y, int width, int height, glm::vec4 color)
    {
        m_imageRenderer->renderImageSized(m_commandBuffer, m_frame, view, x, y, width, height, color*m_color);
    }
}
