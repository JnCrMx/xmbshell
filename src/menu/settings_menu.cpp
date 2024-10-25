module;
#include <array>
#include <string>
#include <optional>

module xmbshell.app;

import spdlog;
import glibmm;
import giomm;
import i18n;
import dreamrender;
import xmbshell.config;

import :settings_menu;
import :choice_overlay;

namespace menu {
    settings_menu::settings_menu(const std::string& name, dreamrender::texture&& icon, app::xmbshell* xmb, dreamrender::resource_loader& loader) : simple_menu(name, std::move(icon)) {
        entries.push_back(make_simple<simple_menu>("Video Settings"_(), config::CONFIG.asset_directory/"icons/icon_settings_video.png", loader,
            std::array{
                make_settings_entry(loader, "VSync"_(), "re.jcm.xmbos.xmbshell.render", "vsync"),
                make_settings_entry(loader, "Sample Count"_(), "re.jcm.xmbos.xmbshell.render", "sample-count"),
                make_settings_entry(loader, "Max FPS"_(), "re.jcm.xmbos.xmbshell.render", "max-fps"),
            }
        ));
        entries.push_back(make_simple<simple_menu>("Debug Settings"_(), config::CONFIG.asset_directory/"icons/icon_settings_debug.png", loader,
            std::array{
                make_settings_entry(loader, "Show FPS"_(), "re.jcm.xmbos.xmbshell.render", "show-fps"),
                make_settings_entry(loader, "Show Memory Usage"_(), "re.jcm.xmbos.xmbshell.render", "show-mem"),
                make_simple<action_menu_entry>("Toggle Background Blur"_(), config::CONFIG.asset_directory/"icons/icon_settings_toggle-background-blur.png", loader, [xmb](){
                    spdlog::info("Toggling background blur");
                    xmb->set_blur_background(!xmb->get_blur_background());
                    return result::success;
                }),
                make_simple<action_menu_entry>("Toggle Ingame Mode"_(), config::CONFIG.asset_directory/"icons/icon_settings-toggle-ingame-mode.png", loader, [xmb](){
                    spdlog::info("Toggling ingame mode");
                    xmb->set_ingame_mode(!xmb->get_ingame_mode());
                    return result::success;
                }),
                make_simple<action_menu_entry>("Test Choice Overlay"_(), config::CONFIG.asset_directory/"icons/icon_settings-toggle-ingame-mode.png", loader, [xmb](){
                    spdlog::info("Testing choice overlay");
                    if(xmb->get_choice_overlay()) {
                        xmb->set_choice_overlay(std::nullopt);
                    } else {
                        xmb->set_choice_overlay(app::choice_overlay{});
                    }
                    return result::success;
                }),
            }
        ));
        entries.push_back(make_simple<action_menu_entry>("Check for Updates"_(), config::CONFIG.asset_directory/"icons/icon_settings_update.png", loader, [](){
            spdlog::info("Update request from XMB");
            return result::unsupported;
        }));
        entries.push_back(make_simple<action_menu_entry>("Report bug"_(), config::CONFIG.asset_directory/"icons/icon_bug.png", loader, [](){
            spdlog::info("Bug report request from XMB");
            return result::unsupported;
        }));
        entries.push_back(make_simple<action_menu_entry>("Reset"_(), config::CONFIG.asset_directory/"icons/icon_settings_reset.png", loader, [](){
            spdlog::info("Settings reset request from XMB");
            Glib::RefPtr<Gio::Settings> shellSettings =
                Gio::Settings::create("re.jcm.xmbos.xmbshell");
            auto source = Gio::SettingsSchemaSource::get_default();
            if(!source) {
                spdlog::error("Failed to get default settings schema source");
                return result::failure;
            }
            auto schema = source->lookup("re.jcm.xmbos.xmbshell", true);
            if(!schema) {
                spdlog::error("Failed to find schema for re.jcm.xmbos.xmbshell");
                return result::failure;
            }
            for(auto key : schema->list_keys()) {
                shellSettings->reset(key);
            }
            config::CONFIG.load();
            return result::success;
        }));
    }
}
