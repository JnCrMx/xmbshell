#include "menu/users_menu.hpp"
#include "config.hpp"
#include "menu/utils.hpp"

#include <SDL_events.h>
#include <SDL_timer.h>
#include <spdlog/spdlog.h>
#include <glibmm/miscutils.h>

namespace menu {
    users_menu::users_menu(const std::string& name, render::texture&& icon, render::resource_loader& loader) : simple_menu(name, std::move(icon)) {
        entries.push_back(make_simple<action_menu_entry>("Quit", config::CONFIG.asset_directory/"icons/icon_action_quit.png", loader, [](){
            spdlog::info("Exit request from XMB");
            SDL_Event event = {
                .quit = {
                    .type = SDL_QUIT,
                    .timestamp = SDL_GetTicks()
                }
            };
            SDL_PushEvent(&event);
            return true;
        }));
        {
            std::string real_name = Glib::get_real_name();
            std::string user_name = Glib::get_user_name();
            constexpr auto faces_dir = "/var/lib/AccountsService/icons/";
            entries.push_back(make_simple<simple_menu_entry>(real_name, faces_dir+user_name, loader));
            selected_submenu = entries.size()-1;
        }
    }
}
