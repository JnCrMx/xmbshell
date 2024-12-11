module;

#include <filesystem>
#include <string>
#include <vector>

#include <unistd.h>

export module xmbshell.app:users_menu;

import :menu_base;
import :menu_utils;
import xmbshell.config;

import dreamrender;
import glibmm;
import giomm;
import i18n;
import spdlog;
import sdl2;
using namespace mfk::i18n::literals;

export namespace menu {

class users_menu : public simple_menu {
    public:
        users_menu(const std::string& name, dreamrender::texture&& icon, dreamrender::resource_loader& loader);
        ~users_menu() override = default;
    private:
        Glib::RefPtr<Gio::DBus::Proxy> login1, accounts;
};

}

namespace menu {
    users_menu::users_menu(const std::string& name, dreamrender::texture&& icon, dreamrender::resource_loader& loader) : simple_menu(name, std::move(icon)) {
        login1 = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BusType::BUS_TYPE_SYSTEM, "org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager");
        accounts = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BusType::BUS_TYPE_SYSTEM, "org.freedesktop.Accounts", "/org/freedesktop/Accounts", "org.freedesktop.Accounts");

        {
            uint64_t my_uid = getuid();
            Glib::Variant<std::vector<Glib::DBusObjectPathString>> users;
            accounts->call_sync("ListCachedUsers", Glib::VariantContainerBase{}).get_child(users);
            for(const auto& user_path : users.get()) {
                auto user = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BusType::BUS_TYPE_SYSTEM, "org.freedesktop.Accounts", user_path, "org.freedesktop.Accounts.User");
                Glib::Variant<Glib::ustring> real_name, icon_file;
                Glib::Variant<uint64_t> uid;
                user->get_cached_property(real_name, "RealName");
                user->get_cached_property(icon_file, "IconFile");
                user->get_cached_property(uid, "Uid");

                std::filesystem::path icon_file_path = static_cast<std::string>(icon_file.get());
                if(!std::filesystem::exists(icon_file_path)) {
                    icon_file_path = config::CONFIG.asset_directory/"icons/icon_user.png";
                }
                auto entry = make_simple<simple_menu_entry>(real_name.get(), icon_file_path, loader);

                if(uid.get() == my_uid) { // the current user is always first
                    entries.insert(entries.begin(), std::move(entry));
                } else {
                    entries.push_back(std::move(entry));
                }
            }
        }

        entries.push_back(make_simple<action_menu_entry>("Quit"_(), config::CONFIG.asset_directory/"icons/icon_action_quit.png", loader, [](){
            spdlog::info("Exit request from XMB");
            sdl::Event event = {
                .quit = {
                    .type = sdl::EventType::SDL_QUIT,
                    .timestamp = sdl::GetTicks()
                }
            };
            sdl::PushEvent(&event);
            return result::success;
        }));

        if(Glib::Variant<Glib::ustring> v; login1->call_sync("CanPowerOff", Glib::VariantContainerBase{}).get_child(v), v.get() == "yes") {
            entries.push_back(make_simple<action_menu_entry>("Power off"_(), config::CONFIG.asset_directory/"icons/icon_action_poweroff.png", loader, [this](){
                login1->call_sync("PowerOff", Glib::VariantContainerBase::create_tuple(Glib::Variant<bool>::create(true)));
                return result::success;
            }));
        }
        if(Glib::Variant<Glib::ustring> v; login1->call_sync("CanReboot", Glib::VariantContainerBase{}).get_child(v), v.get() == "yes") {
            entries.push_back(make_simple<action_menu_entry>("Reboot"_(), config::CONFIG.asset_directory/"icons/icon_action_reboot.png", loader, [this](){
                login1->call_sync("Reboot", Glib::VariantContainerBase::create_tuple(Glib::Variant<bool>::create(true)));
                return result::success;
            }));
        }
        if(Glib::Variant<Glib::ustring> v; login1->call_sync("CanSuspend", Glib::VariantContainerBase{}).get_child(v), v.get() == "yes") {
            entries.push_back(make_simple<action_menu_entry>("Suspend"_(), config::CONFIG.asset_directory/"icons/icon_action_suspend.png", loader, [this](){
                login1->call_sync("Suspend", Glib::VariantContainerBase::create_tuple(Glib::Variant<bool>::create(true)));
                return result::success;
            }));
        }
    }
}
