module;

#include <filesystem>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

module xmbshell.app;

import spdlog;
import glibmm;
import giomm;
import i18n;
import dreamrender;
import xmbshell.config;

import :applications_menu;
import :choice_overlay;
import :message_overlay;

namespace menu {

using namespace mfk::i18n::literals;

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

applications_menu::applications_menu(const std::string& name, dreamrender::texture&& icon, app::xmbshell* xmb, dreamrender::resource_loader& loader, AppFilter filter) : simple_menu(name, std::move(icon)) {
    std::call_once(themedIconPathsFlag, initialize_themed_icons);

    auto appInfos = Gio::AppInfo::get_all();
    for (const auto& app : appInfos) {
        if(auto desktop_app = Glib::RefPtr<Gio::DesktopAppInfo>::cast_dynamic(app)) {
            if(!filter(*desktop_app.get()))
                continue;

            spdlog::trace("Found application: {} ({})", desktop_app->get_display_name(), desktop_app->get_id());

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
                        (*themed_icon->get_names().begin()).c_str(), desktop_app->get_display_name());
                }
            } else if(auto* file_icon = dynamic_cast<Gio::FileIcon*>(icon.get())) {
                icon_path = file_icon->get_file()->get_path();
            } else {
                if(icon) {
                    auto& r = *icon.get();
                    spdlog::warn("Unsupported icon type for app \"{}\": {}", desktop_app->get_display_name(),
                        typeid(r).name());
                } else {
                    spdlog::warn("No icon for app \"{}\"", desktop_app->get_display_name());
                }
            }

            dreamrender::texture icon_texture(loader.getDevice(), loader.getAllocator());
            auto entry = std::make_unique<action_menu_entry>(desktop_app->get_display_name(), std::move(icon_texture),
                std::function<result()>{}, [desktop_app, xmb](action action) {
                    if(action == action::ok) {
                        return desktop_app->launch(std::vector<Glib::RefPtr<Gio::File>>()) ? result::success : result::failure;
                    } else if(action == action::options) {
                        xmb->set_choice_overlay(app::choice_overlay{std::vector{
                            "Launch application"_(), "View information"_(), "Remove from XMB"_()
                        }, 0, [desktop_app, xmb](unsigned int index){
                            switch(index) {
                                case 0:
                                    desktop_app->launch(std::vector<Glib::RefPtr<Gio::File>>());
                                    return;
                                case 1: {
                                    std::string info{};
                                    info += "ID: "_() + desktop_app->get_id() + "\n";
                                    info += "Name: "_() + desktop_app->get_name() + "\n";
                                    info += "Display Name: "_() + desktop_app->get_display_name() + "\n";
                                    info += "Description: "_() + desktop_app->get_description() + "\n";
                                    info += "Executable: "_() + desktop_app->get_executable() + "\n";
                                    info += "Command Line: "_() + desktop_app->get_commandline() + "\n";
                                    info += "Icon: "_() + desktop_app->get_icon()->to_string() + "\n";
                                    info += "Categories: "_() + desktop_app->get_categories() + "\n";
                                    xmb->set_message_overlay(app::message_overlay{"Application Information"_(), info, {"OK"_()}, [](unsigned int){}, false});
                                    return;
                                    }
                                case 2:
                                    xmb->set_message_overlay(app::message_overlay{"Remove Application"_(), "Are you sure you want to remove this application from XMB Shell?"_(), {"Yes"_(), "No"_()}, [desktop_app, xmb](unsigned int index){
                                        if(index == 0) {
                                            // TODO: Implement
                                        }
                                    }, true});
                                    return;
                            }
                        }});
                    }
                    return result::unsupported;
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
