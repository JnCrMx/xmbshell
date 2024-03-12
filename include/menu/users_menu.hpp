#pragma once

#include "menu/menu.hpp"
#include "render/resource_loader.hpp"

namespace menu {

class users_menu : public simple_menu {
    public:
        users_menu(const std::string& name, render::texture&& icon, render::resource_loader& loader);
        ~users_menu() override = default;
};

}
