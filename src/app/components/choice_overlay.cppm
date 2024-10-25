module;

#include <chrono>

export module xmbshell.app:choice_overlay;

import dreamrender;
import sdl2;
import vulkan_hpp;
import vma;

namespace app {

export class choice_overlay {
    public:
        choice_overlay();

        void render(dreamrender::gui_renderer& renderer);
    private:
};

}
