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

applications_menu::applications_menu(const std::string& name, dreamrender::texture&& icon, app::xmbshell* xmb, dreamrender::resource_loader& loader, AppFilter filter)
    : simple_menu(name, std::move(icon)), xmb(xmb), loader(loader), filter(filter) {
    std::call_once(themedIconPathsFlag, initialize_themed_icons);

    auto appInfos = Gio::AppInfo::get_all();
    for (const auto& app : appInfos) {
        if(auto desktop_app = Glib::RefPtr<Gio::DesktopAppInfo>::cast_dynamic(app)) {
            if(!filter(*desktop_app.get()))
                continue;
            if(!show_hidden && config::CONFIG.excludedApplications.contains(app->get_id()))
                continue;

            spdlog::trace("Found application: {} ({})", desktop_app->get_display_name(), desktop_app->get_id());
            auto entry = create_action_menu_entry(desktop_app);
            apps.push_back(desktop_app);
            entries.push_back(std::move(entry));
        } else {
            spdlog::warn("AppInfo is not a DesktopAppInfo: {}", app->get_display_name());
        }
    }
}

std::unique_ptr<action_menu_entry> applications_menu::create_action_menu_entry(Glib::RefPtr<Gio::DesktopAppInfo> app) {
    std::string icon_path;
    auto icon = app->get_icon();
    if(auto* themed_icon = dynamic_cast<Gio::ThemedIcon*>(icon.get())) {
        for(const auto& name : themed_icon->get_names()) {
            if(auto it = themedIconPaths.find(name); it != themedIconPaths.end()) {
                icon_path = it->second;
                break;
            }
        }
        if(icon_path.empty()) {
            spdlog::warn("Themed icon \"{}\" not found for app \"{}\"",
                (*themed_icon->get_names().begin()).c_str(), app->get_display_name());
        }
    } else if(auto* file_icon = dynamic_cast<Gio::FileIcon*>(icon.get())) {
        icon_path = file_icon->get_file()->get_path();
    } else {
        if(icon) {
            auto& r = *icon.get();
            spdlog::warn("Unsupported icon type for app \"{}\": {}", app->get_display_name(),
                typeid(r).name());
        } else {
            spdlog::warn("No icon for app \"{}\"", app->get_display_name());
        }
    }

    dreamrender::texture icon_texture(loader.getDevice(), loader.getAllocator());
    auto entry = std::make_unique<action_menu_entry>(app->get_display_name(), std::move(icon_texture),
        std::function<result()>{}, std::bind(&applications_menu::activate_app, this, app, std::placeholders::_1));
    if(!icon_path.empty()) {
        loader.loadTexture(&entry->get_icon(), icon_path);
    }
    return entry;
}

void applications_menu::reload() {
    auto appInfos = Gio::AppInfo::get_all();
    for (const auto& app : appInfos) {
        if(auto desktop_app = Glib::RefPtr<Gio::DesktopAppInfo>::cast_dynamic(app)) {
            bool should_include = filter(*desktop_app.get()) && (show_hidden || !config::CONFIG.excludedApplications.contains(desktop_app->get_id()));
            spdlog::trace("Found application: {} ({})", desktop_app->get_display_name(), desktop_app->get_id());
            auto it = std::find_if(apps.begin(), apps.end(), [&desktop_app](const Glib::RefPtr<Gio::DesktopAppInfo>& app){
                return app->get_id() == desktop_app->get_id();
            });
            if(it == apps.end()) {
                if(should_include) {
                    spdlog::trace("Found new application: {} ({})", desktop_app->get_display_name(), desktop_app->get_id());
                    auto entry = create_action_menu_entry(desktop_app);
                    apps.push_back(desktop_app);
                    entries.push_back(std::move(entry));
                }
            } else {
                if(!should_include) {
                    spdlog::trace("Removing application: {} ({})", desktop_app->get_display_name(), desktop_app->get_id());
                    auto index = std::distance(apps.begin(), it);
                    apps.erase(it);
                    entries.erase(entries.begin() + index);
                }
            }
        }
    }
}

result applications_menu::activate_app(Glib::RefPtr<Gio::DesktopAppInfo> app, action action) {
    if(action == action::ok) {
        return app->launch(std::vector<Glib::RefPtr<Gio::File>>()) ? result::success : result::failure;
    } else if(action == action::options) {
        bool hidden = config::CONFIG.excludedApplications.contains(app->get_id());
        xmb->set_choice_overlay(app::choice_overlay{std::vector{
            "Launch application"_(), "View information"_(), hidden ? "Show in XMB"_() : "Remove from XMB"_()
        }, 0, [this, app, hidden](unsigned int index){
            switch(index) {
                case 0:
                    app->launch(std::vector<Glib::RefPtr<Gio::File>>());
                    return;
                case 1: {
                    std::string info{};
                    info += "ID: "_() + app->get_id() + "\n";
                    info += "Name: "_() + app->get_name() + "\n";
                    info += "Display Name: "_() + app->get_display_name() + "\n";
                    info += "Description: "_() + app->get_description() + "\n";
                    info += "Executable: "_() + app->get_executable() + "\n";
                    info += "Command Line: "_() + app->get_commandline() + "\n";
                    info += "Icon: "_() + app->get_icon()->to_string() + "\n";
                    info += "Categories: "_() + app->get_categories() + "\n";
                    xmb->set_message_overlay(app::message_overlay{"Application Information"_(), info, {"OK"_()}, [](unsigned int){}, false});
                    return;
                    }
                case 2:
                    if(!hidden) {
                        xmb->set_message_overlay(app::message_overlay{"Remove Application"_(),
                            "Are you sure you want to remove this application from XMB Shell?"_(),
                            {"Yes"_(), "No"_()}, [this, app](unsigned int index){
                            if(index == 0) {
                                config::CONFIG.excludeApplication(app->get_id());
                                reload();
                            }
                        }, true});
                    } else {
                        config::CONFIG.excludeApplication(app->get_id(), false);
                        reload();
                    }
                    return;
            }
        }});
    }
    return result::unsupported;
}

result applications_menu::activate(action action) {
    if(action == action::extra) {
        show_hidden = !show_hidden;
        reload();
        return result::success;
    }
    return simple_menu::activate(action);
}

void applications_menu::get_button_actions(std::vector<std::pair<action, std::string>>& v) {
    v.push_back(std::make_pair(action::extra, show_hidden ? "Hide excluded apps"_() : "Show excluded apps"_()));
    v.push_back(std::make_pair(action::none, ""));
    v.push_back(std::make_pair(action::none, ""));
    v.push_back(std::make_pair(action::none, ""));
    v.push_back(std::make_pair(action::none, ""));
}

}
