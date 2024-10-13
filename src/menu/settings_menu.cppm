module;

#include <memory>
#include <string>

export module xmbshell.menu:settings_menu;

import spdlog;
import glibmm;
import giomm;
import i18n;
import dreamrender;
import xmbshell.config;
import :base;
import :utils;

export namespace menu {

class settings_menu : public simple_menu {
    public:
        settings_menu(const std::string& name, dreamrender::texture&& icon, dreamrender::resource_loader& loader);
        ~settings_menu() override = default;
};

}

using namespace mfk::i18n::literals;

namespace menu {
    std::unique_ptr<action_menu_entry> make_settings_entry(dreamrender::resource_loader& loader,
        const std::string& name, const std::string& schema, const std::string& key)
    {
        std::string filename = "icon_settings_"+key+".png";
        return make_simple<action_menu_entry>(name, config::CONFIG.asset_directory/"icons"/filename, loader, [key, schema](){
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
            return result::success;
        });
    }

    settings_menu::settings_menu(const std::string& name, dreamrender::texture&& icon, dreamrender::resource_loader& loader) : simple_menu(name, std::move(icon)) {
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
