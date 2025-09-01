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

export module xmbshell.app:blur_layer;

import dreamrender;
import vulkan_hpp;
import :component;

namespace app {

export class blur_layer : public component {
    public:
        blur_layer(class xmbshell* xmb);

        void render(dreamrender::gui_renderer& renderer, class xmbshell* xmb) override;
        [[nodiscard]] bool is_opaque() const override { return false; }
    private:
        dreamrender::texture srcTexture;
        dreamrender::texture dstTexture;

        vk::UniqueDescriptorSetLayout descriptorSetLayout;
        vk::UniqueDescriptorPool descriptorPool;
        vk::DescriptorSet descriptorSet;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline pipeline;
};

}
