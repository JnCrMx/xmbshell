module;

#include <array>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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
    using namespace mfk::i18n::literals;

    std::unique_ptr<action_menu_entry> make_settings_entry(dreamrender::resource_loader& loader, app::xmbshell* xmb,
        const std::string& name, const std::string& schema, const std::string& key)
    {
        std::string filename = std::format("icon_settings_{}.png", key);
        return make_simple<action_menu_entry>(name, config::CONFIG.asset_directory/"icons"/filename, loader, [xmb, key, schema](){
            spdlog::debug("Opening setting {} in schema {}", key, schema);
            auto source = Gio::SettingsSchemaSource::get_default();
            if(!source) {
                spdlog::error("Failed to get default settings schema source");
                return result::failure;
            }
            auto schemaObj = source->lookup(schema, true);
            if(!schemaObj) {
                spdlog::error("Failed to find schema for {}", schema);
                return result::failure;
            }
            auto keyObj = schemaObj->get_key(key);
            if(!keyObj) {
                spdlog::error("Failed to find key {} in schema {}", key, schema);
                return result::failure;
            }
            auto type = keyObj->get_value_type();
            spdlog::trace("Type of key {} in schema {} is {}", key, schema, type.get_string());

            if(type.equal(Glib::VARIANT_TYPE_BOOL)) {
                auto settings = Gio::Settings::create(schema);
                bool value = settings->get_boolean(key);

                xmb->set_choice_overlay(app::choice_overlay{
                    std::vector<std::string>{"Off", "On"}, value ? 1u : 0u,
                    [settings, schema, key](unsigned int choice) {
                        settings->set_boolean(key, choice == 1);
                        settings->apply();
                    }
                });
            }

            return result::success;
        });
    }

    settings_menu::settings_menu(const std::string& name, dreamrender::texture&& icon, app::xmbshell* xmb, dreamrender::resource_loader& loader) : simple_menu(name, std::move(icon)) {
        entries.push_back(make_simple<simple_menu>("Video Settings"_(), config::CONFIG.asset_directory/"icons/icon_settings_video.png", loader,
            std::array{
                make_settings_entry(loader, xmb, "VSync"_(), "re.jcm.xmbos.xmbshell.render", "vsync"),
                make_settings_entry(loader, xmb, "Sample Count"_(), "re.jcm.xmbos.xmbshell.render", "sample-count"),
                make_settings_entry(loader, xmb, "Max FPS"_(), "re.jcm.xmbos.xmbshell.render", "max-fps"),
            }
        ));
        entries.push_back(make_simple<simple_menu>("Debug Settings"_(), config::CONFIG.asset_directory/"icons/icon_settings_debug.png", loader,
            std::array{
                make_settings_entry(loader, xmb, "Show FPS"_(), "re.jcm.xmbos.xmbshell.render", "show-fps"),
                make_settings_entry(loader, xmb, "Show Memory Usage"_(), "re.jcm.xmbos.xmbshell.render", "show-mem"),
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
                        xmb->set_choice_overlay(app::choice_overlay{std::vector<std::string>{{"Hello"}, {"World"}}});
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
