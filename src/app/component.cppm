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

#include <vector>

export module xmbshell.app:component;

import dreamrender;
import xmbshell.utils;
import vulkan_hpp;

namespace app {

class xmbshell;

export class component {
    public:
        virtual ~component() = default;
        virtual void preload(dreamrender::resource_loader* loader, const std::vector<vk::RenderPass>& renderPasses, vk::SampleCountFlagBits sampleCount, vk::PipelineCache pipelineCache, app::xmbshell* xmb) {};
        virtual void prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews, app::xmbshell* xmb) {};
        virtual void init(app::xmbshell* xmb) {};
        virtual void prerender(vk::CommandBuffer cmd, int frame, app::xmbshell* xmb) {};
        virtual void render(dreamrender::gui_renderer& renderer, app::xmbshell* xmb) = 0;
        virtual result tick(app::xmbshell* xmb) { return result::success; }
        [[nodiscard]] virtual bool is_opaque() const { return true; }
        [[nodiscard]] virtual bool is_transparent() const { return false; }
        [[nodiscard]] virtual bool do_fade_in() const { return false; }
        [[nodiscard]] virtual bool do_fade_out() const { return false; }
};

}
