module;

#include <memory>
#include <string>

export module xmbshell.app:progress_overlay;

import dreamrender;
import sdl2;
import vulkan_hpp;
import vma;
import xmbshell.utils;

namespace app {

export class progress_item {
    public:
        enum class status {
            running,
            success,
            error
        };

        virtual ~progress_item() = default;
        virtual status init(std::string& message) = 0;
        virtual status progress(double& progress, std::string& message) = 0;
        virtual bool cancel(std::string& message) {
            return false;
        }
};

export class progress_overlay : public action_receiver {
    public:
        progress_overlay(std::string title, std::unique_ptr<progress_item>&& item);

        result tick(class xmbshell* xmb);
        void render(dreamrender::gui_renderer& renderer, class xmbshell* xmb);
        result on_action(action action) override;
    private:
        std::string title;
        std::unique_ptr<progress_item> item;
        double progress = 0.0;
        bool done = false;
        bool failed = false;
        std::string status_message;
};

}
