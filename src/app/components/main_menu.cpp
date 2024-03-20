#include "app/components/main_menu.hpp"

#include "app/xmbshell.hpp"
#include "config.hpp"
#include "menu/applications_menu.hpp"
#include "menu/users_menu.hpp"
#include "menu/settings_menu.hpp"
#include "menu/utils.hpp"
#include <spdlog/spdlog.h>

namespace app {

main_menu::main_menu(xmbshell* shell) : shell(shell) {

}

void main_menu::preload(vk::Device device, vma::Allocator allocator, render::resource_loader& loader) {
    ok_sound = SoundChunk(Mix_LoadWAV((config::CONFIG.asset_directory/"sounds/ok.wav").c_str()));
    if(!ok_sound) {
        spdlog::error("Mix_LoadWAV: {}", Mix_GetError());
    }
    using ::menu::make_simple;
    using ::menu::make_simple_of;

    const auto& asset_directory = config::CONFIG.asset_directory;
    menus.push_back(make_simple<menu::users_menu>("Users", asset_directory/"icons/icon_category_users.png", loader, loader));
    menus.push_back(make_simple<menu::settings_menu>("Settings", asset_directory/"icons/icon_category_settings.png", loader, loader));
    menus.push_back(make_simple_of<menu::menu>("Photo", asset_directory/"icons/icon_category_photo.png", loader));
    menus.push_back(make_simple_of<menu::menu>("Music", asset_directory/"icons/icon_category_music.png", loader));
    menus.push_back(make_simple_of<menu::menu>("Video", asset_directory/"icons/icon_category_video.png", loader));
    menus.push_back(make_simple_of<menu::menu>("TV", asset_directory/"icons/icon_category_tv.png", loader));
    menus.push_back(make_simple<menu::applications_menu>("Game", asset_directory/"icons/icon_category_game.png", loader, loader, ::menu::categoryFilter("Game")));
    menus.push_back(make_simple_of<menu::menu>("Network", asset_directory/"icons/icon_category_network.png", loader));
    menus.push_back(make_simple_of<menu::menu>("Friends", asset_directory/"icons/icon_category_friends.png", loader));
}

void main_menu::key_down(SDL_Keysym key) {
    switch(key.sym) {
        case SDLK_LEFT:
            select_relative(direction::left);
            break;
        case SDLK_RIGHT:
            select_relative(direction::right);
            break;
        case SDLK_UP:
            select_relative(direction::up);
            break;
        case SDLK_DOWN:
            select_relative(direction::down);
            break;
        case SDLK_RETURN:
            activate_current();
            break;
        case SDLK_ESCAPE:
            back();
            break;
    }
}
void main_menu::key_up(SDL_Keysym key) {

}
void main_menu::button_down(SDL_GameController* controller, SDL_GameControllerButton button) {
    last_controller_button_input = std::make_tuple(controller, button);
    last_controller_button_input_time = std::chrono::system_clock::now();

    if(button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
        if(!select_relative(direction::left)) {
            error_rumble(controller);
        }
    } else if(button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
        if(!select_relative(direction::right)) {
            error_rumble(controller);
        }
    } else if(button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
        if(!select_relative(direction::up)) {
            error_rumble(controller);
        }
    } else if(button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
        if(!select_relative(direction::down)) {
            error_rumble(controller);
        }
    } else if(button == SDL_CONTROLLER_BUTTON_A) {
        if(!activate_current()) {
            error_rumble(controller);
        }
    } else if(button == SDL_CONTROLLER_BUTTON_B) {
        if(!back()) {
            error_rumble(controller);
        }
    }
}
void main_menu::button_up(SDL_GameController* controller, SDL_GameControllerButton button) {
    last_controller_button_input = std::nullopt;
}
void main_menu::axis_motion(SDL_GameController* controller, SDL_GameControllerAxis axis, Sint16 value) {
    if(!config::CONFIG.controllerAnalogStick) {
        return;
    }

    if(axis == SDL_CONTROLLER_AXIS_LEFTX || axis == SDL_CONTROLLER_AXIS_LEFTY) {
        unsigned int index = axis == SDL_CONTROLLER_AXIS_LEFTX ? 0 : 1;
        if(std::abs(value) < controller_axis_input_threshold) {
            last_controller_axis_input[index] = std::nullopt;
            last_controller_axis_input_time[index] = std::chrono::system_clock::now();
            return;
        }
        direction dir = axis == SDL_CONTROLLER_AXIS_LEFTX ? (value > 0 ? direction::right : direction::left)
            : (value > 0 ? direction::down : direction::up);
        if(last_controller_axis_input[index] && std::get<1>(*last_controller_axis_input[index]) == dir) {
            return;
        }
        if(!select_relative(dir)) {
            error_rumble(controller);
        }
        last_controller_axis_input[index] = std::make_tuple(controller, dir);
        last_controller_axis_input_time[index] = std::chrono::system_clock::now();
    }
}

void main_menu::error_rumble(SDL_GameController* controller) {
    if(!config::CONFIG.controllerRumble) {
        return;
    }
    SDL_GameControllerRumble(controller, 1000, 10000, 100);
}

bool main_menu::select_relative(direction dir) {
    if(!in_submenu) {
        if(dir == direction::left) {
            if(selected > 0) {
                select(selected-1);
                if(Mix_PlayChannel(-1, ok_sound.get(), 0) == -1) {
                    spdlog::error("Mix_PlayChannel: {}", Mix_GetError());
                }

                return true;
            }
        } else if(dir == direction::right) {
            if(selected < menus.size()-1) {
                select(selected+1);
                if(Mix_PlayChannel(-1, ok_sound.get(), 0) == -1) {
                    spdlog::error("Mix_PlayChannel: {}", Mix_GetError());
                }

                return true;
            }
        } else if(dir == direction::up) {
            auto& menu = menus[selected];
            if(menu->get_selected_submenu() > 0) {
                select_submenu(menu->get_selected_submenu()-1);
                if(Mix_PlayChannel(-1, ok_sound.get(), 0) == -1) {
                    spdlog::error("Mix_PlayChannel: {}", Mix_GetError());
                }

                return true;
            }
        } else if(dir == direction::down) {
            auto& menu = menus[selected];
            if(menu->get_selected_submenu() < menu->get_submenus_count()-1) {
                select_submenu(menu->get_selected_submenu()+1);
                if(Mix_PlayChannel(-1, ok_sound.get(), 0) == -1) {
                    spdlog::error("Mix_PlayChannel: {}", Mix_GetError());
                }

                return true;
            }
        }
    }
    return false;
}
bool main_menu::activate_current() {
    auto& menu = *menus[selected];
    if(auto submenu = dynamic_cast<menu::menu*>(&menu.get_submenu(menu.get_selected_submenu()))) {
        if(submenu->get_submenus_count() > 0 && !in_submenu) {
            in_submenu = true;
            last_submenu_transition = std::chrono::system_clock::now();
            return true;
        }
    }
    return menus[selected]->activate();
}
bool main_menu::back() {
    if(in_submenu) {
        in_submenu = false;
        last_submenu_transition = std::chrono::system_clock::now();
        return true;
    }
    return false;
}

void main_menu::select(int index) {
    if(index == selected) {
        return;
    }
    if(index < 0 || index >= menus.size()) {
        return;
    }

    last_selected = selected;
    last_selected_transition = std::chrono::system_clock::now();
    selected = index;

    last_selected_submenu = menus[selected]->get_selected_submenu();
}
void main_menu::select_submenu(int index) {
    auto& menu = menus[selected];
    if(index == menu->get_selected_submenu()) {
        return;
    }
    if(index < 0 || index >= menu->get_submenus_count()) {
        return;
    }

    last_selected_submenu = menu->get_selected_submenu();
    last_selected_submenu_transition = std::chrono::system_clock::now();
    menu->select_submenu(index);
}

void main_menu::tick() {
    for(unsigned int i=0; i<2; i++) {
        if(last_controller_axis_input[i]) {
            auto time_since_input = std::chrono::duration<double>(std::chrono::system_clock::now() - last_controller_axis_input_time[i]);
            if(time_since_input > controller_axis_input_duration) {
                auto [controller, dir] = *last_controller_axis_input[i];
                if(!select_relative(dir)) {
                    error_rumble(controller);
                }
                last_controller_axis_input_time[i] = std::chrono::system_clock::now();
            }
        }
    }
    if(last_controller_button_input) {
        auto time_since_input = std::chrono::duration<double>(std::chrono::system_clock::now() - last_controller_button_input_time);
        if(time_since_input > controller_button_input_duration) {
            auto [controller, button] = *last_controller_button_input;
            button_down(controller, button);
        }
    }
}

void main_menu::render(render::gui_renderer& renderer) {
    tick(); // TODO: This should be called from the outside

    constexpr glm::vec4 active_color(1.0f, 1.0f, 1.0f, 1.0f);
    constexpr glm::vec4 inactive_color(0.25f, 0.25f, 0.25f, 0.25f);

    auto time_since_transition = std::chrono::duration<double>(std::chrono::system_clock::now() - last_submenu_transition);
    double partial = std::clamp(time_since_transition / transition_submenu_activate_duration, 0.0, 1.0);
    partial = in_submenu ? partial : 1.0 - partial;

    renderer.set_color(glm::mix(active_color, inactive_color, partial));
    render_crossbar(renderer);
    renderer.set_color(active_color);
    if(in_submenu) {
        render_submenu(renderer);
    }
}

void main_menu::render_crossbar(render::gui_renderer& renderer) {
    float real_selection;
    int selected = this->selected;

    if(selected == last_selected) {
        real_selection = selected;
    } else {
        auto time_since_transition = std::chrono::duration<double>(std::chrono::system_clock::now() - last_selected_transition);
        real_selection = last_selected + (selected - last_selected) *
            time_since_transition / transition_duration;
        if(selected > last_selected && real_selection > selected) {
            real_selection = selected;
        } else if(selected < last_selected && real_selection < selected) {
            real_selection = selected;
        }
    }

    const auto selected_menu_x = 0.35f/renderer.aspect_ratio;
    float x = selected_menu_x - 0.15f/renderer.aspect_ratio*real_selection;

    for (int i = 0; i < menus.size(); i++) {
        if(i == selected && in_submenu) {
            x += 0.15f/renderer.aspect_ratio;
            continue; // the selected menu is rendered as part of the submenu
        }

        auto& menu = menus[i];
        renderer.draw_image(menu->get_icon(), x, 0.25f, 0.1f, 0.1f);
        if(i == selected) {
            renderer.draw_text(menu->get_name(), x+0.05f/renderer.aspect_ratio, 0.35f, 0.04f, glm::vec4(1, 1, 1, 1), true);
        }
        x += 0.15f/renderer.aspect_ratio;
    }

    // TODO: transition / animation
    x = selected_menu_x - 0.15f/renderer.aspect_ratio*(real_selection - selected);
    auto& menu = menus[selected];
    int selected_submenu = menu->get_selected_submenu();
    float partial_transition = 1.0f;
    float partial_y = 0.0f;
    if(selected_submenu != last_selected_submenu) {
        auto time_since_transition = std::chrono::duration<double>(std::chrono::system_clock::now() - last_selected_submenu_transition);
        if(time_since_transition > transition_duration) {
            last_selected_submenu = selected_submenu;
        } else {
            partial_transition = time_since_transition / transition_duration;
            partial_y = (selected_submenu - last_selected_submenu) * (1.0f - partial_transition);
        }
    }
    {
        float y = 0.25f - 0.15f + partial_y*0.15f;
        for(int i=selected_submenu-1; i >= 0 && y >= -0.065; i--) {
            auto& submenu = menu->get_submenu(i);
            renderer.draw_image(submenu.get_icon(), x+0.02f/renderer.aspect_ratio, y, 0.06f, 0.06f);
            if(!in_submenu)
                renderer.draw_text(submenu.get_name(), x+0.15f/renderer.aspect_ratio, y+0.03f, 0.04f, glm::vec4(0.7, 0.7, 0.7, 1), false, true);
            y -= 0.065f;
        }
    }
    {
        float y = 0.25f + 0.15f - 0.065f + partial_y*0.15f;
        if(last_selected_submenu > selected_submenu) {
            y += utils::mix(0.065f, 0.15f, 1.0f-partial_transition);
        } else {
            y += 0.065f;
        }
        for(int i=selected_submenu, count = menu->get_submenus_count(); i<count && y < 1.0f; i++) {
            auto& submenu = menu->get_submenu(i);
            if(i == selected_submenu) {
                if(!in_submenu) {
                    double size = utils::mix(0.06, 0.12, partial_transition);
                    renderer.draw_image(submenu.get_icon(), x+(0.05f-size/2.0f)/renderer.aspect_ratio, y, size, size);
                    if(!in_submenu)
                        renderer.draw_text(submenu.get_name(), x+0.15f/renderer.aspect_ratio, y+size/2, size/2, glm::vec4(1, 1, 1, 1), false, true);
                }
                y += utils::mix(0.065f, 0.15f, partial_transition);
            }
            else if(i == last_selected_submenu) {
                double size = utils::mix(0.06, 0.12, 1.0f-partial_transition);
                renderer.draw_image(submenu.get_icon(), x+(0.05f-size/2.0f)/renderer.aspect_ratio, y, size, size);
                if(!in_submenu)
                    renderer.draw_text(submenu.get_name(), x+0.15f/renderer.aspect_ratio, y+size/2, size/2, glm::vec4(1, 1, 1, 1), false, true);
                y += utils::mix(0.065f, 0.15f, 1.0f-partial_transition);
            }
            else {
                renderer.draw_image(submenu.get_icon(), x+0.02f/renderer.aspect_ratio, y, 0.06f, 0.06f);
                if(!in_submenu)
                    renderer.draw_text(submenu.get_name(), x+0.15f/renderer.aspect_ratio, y+0.03f, 0.04f, glm::vec4(0.7, 0.7, 0.7, 1), false, true);
                y += 0.065f;
            }
        }
    }
}

void main_menu::render_submenu(render::gui_renderer& renderer) {
    // TODO
}

}
