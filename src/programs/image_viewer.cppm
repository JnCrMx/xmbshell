module;

#include <cassert>
#include <filesystem>
#include <future>

export module xmbshell.app:image_viewer;

import dreamrender;
import glm;
import spdlog;
import xmbshell.utils;
import :component;

namespace programs {

using namespace app;

export class image_viewer : public component, public action_receiver, public joystick_receiver, public mouse_receiver {
    public:
        image_viewer(std::filesystem::path path, dreamrender::resource_loader& loader) : path(std::move(path)) {
            texture = std::make_shared<dreamrender::texture>(loader.getDevice(), loader.getAllocator());
            load_future = loader.loadTexture(texture.get(), this->path);
        }
        image_viewer(std::shared_ptr<dreamrender::texture> texture) : texture(std::move(texture)) {
        }

        result tick(xmbshell*) override {
            float pic_aspect = texture->width / (float)texture->height;
            glm::vec2 limit(pic_aspect, 1.0f);
            limit *= zoom;
            limit /= 2.0f;

            offset = glm::clamp(offset + move_delta_pos, -limit, limit);
            return result::success;
        }

        void render(dreamrender::gui_renderer& renderer, class xmbshell* xmb) override {
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
            offset -= (zoom-1.0f) * glm::vec2(w/renderer.aspect_ratio, h) / 2.0f;
            offset += this->offset / glm::vec2(renderer.aspect_ratio, 1.0f);

            renderer.set_clip((1.0f-size)/2, (1.0f-size)/2, size, size);
            renderer.draw_image(*texture, offset.x, offset.y, zoom*w, zoom*h);
            renderer.reset_clip();
        }
        result on_action(action action) override {
            if(action == action::cancel) {
                return result::close;
            }
            else if(action == action::up) {
                zoom *= 1.1f;
                return result::success;
            }
            else if(action == action::down) {
                zoom /= 1.1f;
                return result::success;
            }
            else if(action == action::extra) {
                offset = {0.0f, 0.0f};
                zoom = 1.0f;
                return result::success;
            }
            return result::failure;
        }

        result on_joystick(unsigned int index, float x, float y) override {
            if(index == 1) {
                move_delta_pos = -glm::vec2(x, y)/25.0f;
                if(std::abs(x) < 0.1f) {
                    move_delta_pos.x = 0.0f;
                }
                if(std::abs(y) < 0.1f) {
                    move_delta_pos.y = 0.0f;
                }
                return result::success;
            }
            return result::unsupported;
        }

        result on_mouse_move(float x, float y) override {
            offset = glm::vec2(x, y) - glm::vec2(0.5f, 0.5f);
            return result::success;
        }

        [[nodiscard]] bool is_opaque() const override {
            // This is probably undefined behavior/a race condition, but it works well enough.
            // Maybe one day the entire program will explode due to this, oh well!
            return texture->loaded;
        }
    private:
        std::filesystem::path path;
        std::shared_ptr<dreamrender::texture> texture;
        std::shared_future<void> load_future;

        glm::vec2 move_delta_pos = {0.0f, 0.0f};

        float zoom = 1.0f;
        glm::vec2 offset = {0.0f, 0.0f};
};

}
