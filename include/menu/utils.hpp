#pragma once

#include "menu/menu.hpp"
#include "render/resource_loader.hpp"

#include <filesystem>
#include <memory>

namespace menu {

template<typename Menu, typename... Args>
static std::unique_ptr<Menu> make_simple(const std::string& name, const std::filesystem::path& icon_path,
    render::resource_loader& loader,
    Args&&... args)
{
    render::texture icon(loader.getDevice(), loader.getAllocator());
    auto menu = std::make_unique<Menu>(name, std::move(icon), std::forward<Args>(args)...);
    loader.loadTexture(&menu->get_icon(), icon_path);
    return menu;
}

template<typename Menu, typename... Args>
static std::unique_ptr<simple<Menu>> make_simple_of(const std::string& name, const std::filesystem::path& icon_path,
    render::resource_loader& loader,
    Args&&... args)
{
    return make_simple<simple<Menu>>(name, icon_path, loader, std::forward<Args>(args)...);
}

}
