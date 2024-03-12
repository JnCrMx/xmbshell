#pragma once

#include "giomm/desktopappinfo.h"
#include "menu/menu.hpp"
#include "render/resource_loader.hpp"

#include <functional>

namespace menu {
    using AppFilter = std::function<bool(const Gio::DesktopAppInfo&)>;

    constexpr auto noFilter() {
        return [](const Gio::DesktopAppInfo&) {
            return true;
        };
    }
    constexpr auto categoryFilter(const std::string& category) {
        return [category](const Gio::DesktopAppInfo& app) {
            std::istringstream iss(app.get_categories());
            std::string c;
            while(std::getline(iss, c, ';')) {
                if(c == category) {
                    return true;
                }
            }
            return false;
        };
    }

    class applications_menu : public simple_menu {
        public:
            applications_menu(const std::string& name, render::texture&& icon, render::resource_loader& loader, AppFilter filter = noFilter());
            ~applications_menu() override = default;
        private:
            std::vector<Glib::RefPtr<Gio::DesktopAppInfo>> apps;
    };

}
