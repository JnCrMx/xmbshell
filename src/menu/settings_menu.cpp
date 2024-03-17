#include "menu/settings_menu.hpp"

#include "config.hpp"
#include "menu/utils.hpp"

#include <spdlog/spdlog.h>

#include <giomm/settings.h>
#include <giomm/settingsschema.h>
#include <giomm/settingsschemasource.h>

namespace menu {
    std::unique_ptr<action_menu_entry> make_settings_entry(render::resource_loader& loader,
        const std::string& name, const std::string& schema, const std::string& key)
    {
        std::string filename = std::format("icon_settings_{}.png", key);
        return make_simple<action_menu_entry>(name, config::CONFIG.asset_directory/"icons"/filename, loader, [key, schema](){
            spdlog::info("Opening setting {} in schema {}", key, schema);
            auto source = Gio::SettingsSchemaSource::get_default();
            if(!source) {
                spdlog::error("Failed to get default settings schema source");
                return false;
            }
            auto schemaObj = source->lookup(schema, true);
            if(!schemaObj) {
                spdlog::error("Failed to find schema for {}", schema);
                return false;
            }
            auto keyObj = schemaObj->get_key(key);
            if(!keyObj) {
                spdlog::error("Failed to find key {} in schema {}", key, schema);
                return false;
            }
            auto type = keyObj->get_value_type();
            return true;
        });
    }

    settings_menu::settings_menu(const std::string& name, render::texture&& icon, render::resource_loader& loader) : simple_menu(name, std::move(icon)) {
        entries.push_back(make_simple<simple_menu>("Video Settings", config::CONFIG.asset_directory/"icons/icon_settings_video.png", loader,
            std::array{
                make_settings_entry(loader, "VSync", "re.jcm.xmbos.xmbshell.render", "vsync"),
                make_settings_entry(loader, "Sample Count", "re.jcm.xmbos.xmbshell.render", "sample-count"),
                make_settings_entry(loader, "Max FPS", "re.jcm.xmbos.xmbshell.render", "max-fps"),
            }
        ));
        entries.push_back(make_simple<simple_menu>("Debug Settings", config::CONFIG.asset_directory/"icons/icon_settings_debug.png", loader,
            std::array{
                make_settings_entry(loader, "Show FPS", "re.jcm.xmbos.xmbshell.render", "show-fps"),
                make_settings_entry(loader, "Show Memory Usage", "re.jcm.xmbos.xmbshell.render", "show-mem"),
            }
        ));
        entries.push_back(make_simple<action_menu_entry>("Reset", config::CONFIG.asset_directory/"icons/icon_settings_reset.png", loader, [](){
            spdlog::info("Settings reset request from XMB");
            Glib::RefPtr<Gio::Settings> shellSettings =
                Gio::Settings::create("re.jcm.xmbos.xmbshell");
            auto source = Gio::SettingsSchemaSource::get_default();
            if(!source) {
                spdlog::error("Failed to get default settings schema source");
                return false;
            }
            auto schema = source->lookup("re.jcm.xmbos.xmbshell", true);
            if(!schema) {
                spdlog::error("Failed to find schema for re.jcm.xmbos.xmbshell");
                return false;
            }
            for(auto key : schema->list_keys()) {
                shellSettings->reset(key);
            }
            config::CONFIG.load();
            return true;
        }));
    }
}
