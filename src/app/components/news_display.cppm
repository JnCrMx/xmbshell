/* XMBShell, a console-like desktop shell
 * Copyright (C) 2025 - JCM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
module;

export module xmbshell.app:news_display;

import dreamrender;
import vulkan_hpp;
import vma;

namespace app {

class news_display {
    public:
        news_display(class xmbshell* shell);
        void preload(vk::Device device, vma::Allocator allocator, dreamrender::resource_loader& loader);
        void tick();
        void render(dreamrender::gui_renderer& renderer);
    private:
        class xmbshell* shell;
};

}
