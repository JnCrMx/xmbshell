module;

#include <filesystem>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>

export module xmbshell.menu:applications_menu;
import :base;
import dreamrender;
import glibmm;
import giomm;
import spdlog;

export namespace menu {
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
            applications_menu(const std::string& name, dreamrender::texture&& icon, dreamrender::resource_loader& loader, AppFilter filter = noFilter());
            ~applications_menu() override = default;
        private:
            std::vector<Glib::RefPtr<Gio::DesktopAppInfo>> apps;
    };

}

namespace menu {

inline std::unordered_map<std::string, std::filesystem::path> themedIconPaths;
inline std::once_flag themedIconPathsFlag;

static void initialize_themed_icons(){
    std::unordered_map<std::string, std::filesystem::path> paths;
    auto process = [&paths](const std::string& dir) {
        auto path = std::filesystem::path(dir) / "icons" / "hicolor";
        if(!std::filesystem::exists(path)) {
            return;
        }
        for(const auto& icon : std::filesystem::recursive_directory_iterator(path)) {
            if(icon.is_regular_file()) {
                auto iconName = icon.path().stem().string();
                auto iconPath = icon.path();
                paths[iconName] = iconPath;
            }
        }
    };
    process("/usr/share");
    process("/usr/local/share");
    process(Glib::get_home_dir()+"/.local/share");

    auto xdg_data_dirs = std::getenv("XDG_DATA_DIRS");
    if(xdg_data_dirs) {
        std::istringstream iss(xdg_data_dirs);
        std::string dir;
        while(std::getline(iss, dir, ':')) {
            process(dir);
        }
    }
    spdlog::debug("Found {} themed icons", paths.size());

    themedIconPaths = paths;
};

applications_menu::applications_menu(const std::string& name, dreamrender::texture&& icon, dreamrender::resource_loader& loader, AppFilter filter) : simple_menu(name, std::move(icon)) {
    std::call_once(themedIconPathsFlag, initialize_themed_icons);

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

            dreamrender::texture icon_texture(loader.getDevice(), loader.getAllocator());
            auto entry = std::make_unique<action_menu_entry>(desktop_app->get_display_name(), std::move(icon_texture),
                [desktop_app](){
                    desktop_app->launch(std::vector<Glib::RefPtr<Gio::File>>());
                    return result::success;
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