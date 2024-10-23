module;

#include <format>
#include <memory>
#include <string>

export module xmbshell.app:settings_menu;

import spdlog;
import glibmm;
import giomm;
import i18n;
import dreamrender;
import xmbshell.config;
import :menu_base;
import :menu_utils;

namespace app {
    class xmbshell;
}

export namespace menu {

class settings_menu : public simple_menu {
    public:
        settings_menu(const std::string& name, dreamrender::texture&& icon, app::xmbshell* xmb, dreamrender::resource_loader& loader);
        ~settings_menu() override = default;
};

}

using namespace mfk::i18n::literals;

namespace menu {
    std::unique_ptr<action_menu_entry> make_settings_entry(dreamrender::resource_loader& loader,
        const std::string& name, const std::string& schema, const std::string& key)
    {
        std::string filename = std::format("icon_settings_{}.png", key);
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
}
