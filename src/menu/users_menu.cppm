module;

#include <SDL_events.h>
#include <SDL_timer.h>

#include "render/texture.hpp"
#include "render/resource_loader.hpp"

export module xmbshell.menu:users_menu;

import :utils;
import xmbshell.config;
import glibmm;
import i18n;
import spdlog;
using namespace mfk::i18n::literals;

export namespace menu {

class users_menu : public simple_menu {
    public:
        users_menu(const std::string& name, render::texture&& icon, render::resource_loader& loader);
        ~users_menu() override = default;
};

}

namespace menu {
    users_menu::users_menu(const std::string& name, render::texture&& icon, render::resource_loader& loader) : simple_menu(name, std::move(icon)) {
        entries.push_back(make_simple<action_menu_entry>("Quit"_(), config::CONFIG.asset_directory/"icons/icon_action_quit.png", loader, [](){
            spdlog::info("Exit request from XMB");
            SDL_Event event = {
                .quit = {
                    .type = SDL_QUIT,
                    .timestamp = SDL_GetTicks()
                }
            };
            SDL_PushEvent(&event);
            return result::success;
        }));
        {
            std::string real_name = Glib::get_real_name();
            std::string user_name = Glib::get_user_name();
            constexpr auto faces_dir = "/var/lib/AccountsService/icons/";
            auto face_path = faces_dir+user_name;
            if(!std::filesystem::exists(face_path)) {
                face_path = config::CONFIG.asset_directory/"icons/icon_user.png";
            }
            entries.push_back(make_simple<simple_menu_entry>(real_name, face_path, loader));
            selected_submenu = entries.size()-1;
        }
    }
}
