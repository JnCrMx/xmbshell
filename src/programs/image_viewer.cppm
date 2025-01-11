module;

#include <cassert>
#include <filesystem>
#include <future>

export module xmbshell.app:image_viewer;

import dreamrender;
import glm;
import xmbshell.utils;
import :component;

namespace programs {

using namespace app;

export class image_viewer : public component, public action_receiver {
    public:
        image_viewer(std::filesystem::path path, dreamrender::resource_loader& loader) : path(std::move(path)) {
            texture = std::make_shared<dreamrender::texture>(loader.getDevice(), loader.getAllocator());
            load_future = loader.loadTexture(texture.get(), this->path);
        }
        image_viewer(std::shared_ptr<dreamrender::texture> texture) : texture(std::move(texture)) {
        }

        void render(dreamrender::gui_renderer& renderer, class xmbshell* xmb) override {
            if(load_future.valid()) {
                load_future.get();
            }

            constexpr float size = 0.9;

            float bw = size*renderer.aspect_ratio;
            float bh = size;

            float pic_aspect = texture->width / (float)texture->height;
            float w{}, h{};
            if(pic_aspect > renderer.aspect_ratio) {
                w = bw;
                h = bw / pic_aspect;
            } else {
                h = bh;
                w = bh * pic_aspect;
            }
            glm::vec2 offset = {(1.0f-size)/2, (1.0f-size)/2};
            offset += glm::vec2((bw-w)/2/renderer.aspect_ratio, (bh-h)/2);

            renderer.set_clip((1.0f-size)/2, (1.0f-size)/2, size, size);
            renderer.draw_image(*texture, offset.x, offset.y, w, h);
            renderer.reset_clip();
        }
        result on_action(action action) override {
            if(action == action::cancel) {
                return result::close;
            }
            return result::failure;
        }

        [[nodiscard]] bool is_opaque() const override {
            return true;
        }
    private:
        std::filesystem::path path;
        std::shared_ptr<dreamrender::texture> texture;
        std::shared_future<void> load_future;

        float zoom = 1.0f;
        glm::vec2 offset = {0.0f, 0.0f};
};

}
