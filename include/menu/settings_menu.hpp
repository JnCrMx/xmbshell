#pragma once

#include "menu/menu.hpp"
#include "render/resource_loader.hpp"

namespace menu {

class settings_menu : public simple_menu {
    public:
        settings_menu(const std::string& name, render::texture&& icon, render::resource_loader& loader);
        ~settings_menu() override = default;
};

}
