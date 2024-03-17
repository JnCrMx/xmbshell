#include "menu/applications_menu.hpp"

#include <filesystem>
#include <giomm/appinfo.h>
#include <giomm/desktopappinfo.h>
#include <giomm/themedicon.h>
#include <giomm/fileicon.h>
#include <spdlog/spdlog.h>

namespace menu {

inline std::unordered_map<std::string, std::filesystem::path> themedIconPaths = [](){
    std::unordered_map<std::string, std::filesystem::path> paths;
    auto xdg_data_dirs = std::getenv("XDG_DATA_DIRS");
    if(xdg_data_dirs) {
        std::istringstream iss(xdg_data_dirs);
        std::string dir;
        while(std::getline(iss, dir, ':')) {
            auto path = std::filesystem::path(dir) / "icons";
            if(!std::filesystem::exists(path)) {
                continue;
            }
            for(const auto& icon : std::filesystem::recursive_directory_iterator(path)) {
                if(icon.path().extension() == ".png") {
                    auto iconName = icon.path().stem().string();
                    auto iconPath = icon.path();
                    paths[iconName] = iconPath;
                }
            }
        }
    }
    return paths;
}();

applications_menu::applications_menu(const std::string& name, render::texture&& icon, render::resource_loader& loader, AppFilter filter) : simple_menu(name, std::move(icon)) {
    auto appInfos = Gio::AppInfo::get_all();
    for (const auto& app : appInfos) {
        if(auto desktop_app = std::dynamic_pointer_cast<Gio::DesktopAppInfo>(app)) {
            if(!filter(*desktop_app))
                continue;

            spdlog::trace("Found application: {}", desktop_app->get_display_name());

            std::string icon_path;
            auto icon = desktop_app->get_icon();
            if(auto* themed_icon = dynamic_cast<Gio::ThemedIcon*>(icon.get())) {
                for(const auto& name : themed_icon->get_names()) {
                    if(auto it = themedIconPaths.find(name); it != themedIconPaths.end()) {
                        icon_path = it->second;
                        break;
                    }
                }
                if(icon_path.empty()) {
                    spdlog::warn("Themed icon \"{}\" not found for app \"{}\"",
                        themed_icon->get_names().front().c_str(), desktop_app->get_display_name());
                }
            } else if(auto* file_icon = dynamic_cast<Gio::FileIcon*>(icon.get())) {
                icon_path = file_icon->get_file()->get_path();
            } else {
                if(icon) {
                    auto& r = *icon;
                    spdlog::warn("Unsupported icon type for app \"{}\": {}", desktop_app->get_display_name(),
                        typeid(r).name());
                } else {
                    spdlog::warn("No icon for app \"{}\"", desktop_app->get_display_name());
                }
            }

            render::texture icon_texture(loader.getDevice(), loader.getAllocator());
            auto entry = std::make_unique<action_menu_entry>(desktop_app->get_display_name(), std::move(icon_texture),
                [desktop_app](){
                    desktop_app->launch(std::vector<Glib::RefPtr<Gio::File>>());
                    return true;
                });
            if(!icon_path.empty()) {
                loader.loadTexture(&entry->get_icon(), icon_path);
            }

            apps.push_back(desktop_app);
            entries.push_back(std::move(entry));
        } else {
            spdlog::warn("AppInfo is not a DesktopAppInfo: {}", app->get_display_name());
        }
    }
}

}
