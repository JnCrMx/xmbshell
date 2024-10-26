module;

#include <memory>
#include <string>

export module xmbshell.app:settings_menu;

import spdlog;
import glibmm;
import giomm;
import dreamrender;
import xmbshell.config;
import :menu_base;
import :menu_utils;

namespace app {
    class xmbshell;
}

export namespace menu {

class settings_menu : public simple_menu {
    public:
        settings_menu(const std::string& name, dreamrender::texture&& icon, app::xmbshell* xmb, dreamrender::resource_loader& loader);
        ~settings_menu() override = default;
};

}

namespace menu {
    std::unique_ptr<action_menu_entry> make_settings_entry(dreamrender::resource_loader& loader, app::xmbshell* xmb,
        const std::string& name, const std::string& schema, const std::string& key);
}
