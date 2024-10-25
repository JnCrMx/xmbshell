module;

#include <array>

module xmbshell.app;
import :choice_overlay;

import dreamrender;
import glm;
import vulkan_hpp;
import vma;

namespace app {

choice_overlay::choice_overlay() {
}

void choice_overlay::render(dreamrender::gui_renderer& renderer) {
    renderer.draw_quad(std::array{
        dreamrender::simple_renderer::vertex_data{{0.65f, 0.0f}, {0.1f, 0.1f, 0.1f, 1.0f}, {0.0f, 0.0f}},
        dreamrender::simple_renderer::vertex_data{{0.65f, 1.0f}, {0.1f, 0.1f, 0.1f, 1.0f}, {0.0f, 1.0f}},
        dreamrender::simple_renderer::vertex_data{{0.90f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        dreamrender::simple_renderer::vertex_data{{0.90f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    }, dreamrender::simple_renderer::params{
        std::array{
            glm::vec2{0.0f, 0.05f},
            glm::vec2{0.0f, 0.0f},
            glm::vec2{-0.1f, 0.1f},
            glm::vec2{-0.1f, 0.1f},
        }
    });
}

}
