#pragma once

#include "render/gui_renderer.hpp"

namespace app {

class news_display {
    public:
        news_display(class xmbshell* shell);
        void preload(vk::Device device, vma::Allocator allocator, render::resource_loader& loader);
        void tick();
        void render(render::gui_renderer& renderer);
    private:
        class xmbshell* shell;
};

}
