module;

#include <filesystem>
#include <memory>

export module xmbshell.menu:utils;
import dreamrender;
import :base;

export namespace menu {

template<typename Menu, typename... Args>
std::unique_ptr<Menu> make_simple(const std::string& name, const std::filesystem::path& icon_path,
    dreamrender::resource_loader& loader,
    Args&&... args)
{
    auto menu = std::make_unique<Menu>(name, dreamrender::texture(loader.getDevice(), loader.getAllocator()), std::forward<Args>(args)...);
    loader.loadTexture(&menu->get_icon(), icon_path);
    return menu;
}

template<typename Menu, typename... Args>
std::unique_ptr<simple<Menu>> make_simple_of(const std::string& name, const std::filesystem::path& icon_path,
    dreamrender::resource_loader& loader,
    Args&&... args)
{
    return make_simple<simple<Menu>>(name, icon_path, loader, std::forward<Args>(args)...);
}

}
