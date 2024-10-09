#include "app/components/main_menu.hpp"

#include "app/xmbshell.hpp"

import sdl2;
import spdlog;
import i18n;
import glm;
using namespace mfk::i18n::literals;

import xmbshell.config;
import xmbshell.menu;

namespace app {

main_menu::main_menu(xmbshell* shell) : shell(shell) {

}

void main_menu::preload(vk::Device device, vma::Allocator allocator, dreamrender::resource_loader& loader) {
    // ok_sound = SoundChunk(Mix_LoadWAV((config::CONFIG.asset_directory/"sounds/ok.wav").c_str()));
    // if(!ok_sound) {
    //     spdlog::error("Mix_LoadWAV: {}", Mix_GetError());
    // }
    using ::menu::make_simple;
    using ::menu::make_simple_of;

    const auto& asset_directory = config::CONFIG.asset_directory;
    menus.push_back(make_simple<menu::users_menu>("Users"_(), asset_directory/"icons/icon_category_users.png", loader, loader));
    menus.push_back(make_simple<menu::settings_menu>("Settings"_(), asset_directory/"icons/icon_category_settings.png", loader, loader));
    menus.push_back(make_simple_of<menu::menu>("Photo"_(), asset_directory/"icons/icon_category_photo.png", loader));
    menus.push_back(make_simple_of<menu::menu>("Music"_(), asset_directory/"icons/icon_category_music.png", loader));
    menus.push_back(make_simple_of<menu::menu>("Video"_(), asset_directory/"icons/icon_category_video.png", loader));
    menus.push_back(make_simple_of<menu::menu>("TV"_(), asset_directory/"icons/icon_category_tv.png", loader));
    menus.push_back(make_simple<menu::applications_menu>("Game"_(), asset_directory/"icons/icon_category_game.png", loader, loader, ::menu::categoryFilter("Game")));
    menus.push_back(make_simple_of<menu::menu>("Network"_(), asset_directory/"icons/icon_category_network.png", loader));
    menus.push_back(make_simple_of<menu::menu>("Friends"_(), asset_directory/"icons/icon_category_friends.png", loader));

    menus[selected]->on_open();
}

void main_menu::key_down(sdl::Keysym key) {
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
void main_menu::key_up(sdl::Keysym key) {

}
void main_menu::button_down(sdl::GameController* controller, sdl::GameControllerButton button) {
    last_controller_button_input = std::make_tuple(controller, button);
    last_controller_button_input_time = std::chrono::system_clock::now();

    if(button == sdl::GameControllerButtonValues::DPAD_LEFT) {
        if(!select_relative(direction::left)) {
            error_rumble(controller);
        }
    } else if(button == sdl::GameControllerButtonValues::DPAD_RIGHT) {
        if(!select_relative(direction::right)) {
            error_rumble(controller);
        }
    } else if(button == sdl::GameControllerButtonValues::DPAD_UP) {
        if(!select_relative(direction::up)) {
            error_rumble(controller);
        }
    } else if(button == sdl::GameControllerButtonValues::DPAD_DOWN) {
        if(!select_relative(direction::down)) {
            error_rumble(controller);
        }
    } else if(button == sdl::GameControllerButtonValues::A) {
        if(!activate_current()) {
            error_rumble(controller);
        }
    } else if(button == sdl::GameControllerButtonValues::B) {
        if(!back()) {
            error_rumble(controller);
        }
    }
}
void main_menu::button_up(sdl::GameController* controller, sdl::GameControllerButton button) {
    last_controller_button_input = std::nullopt;
}
void main_menu::axis_motion(sdl::GameController* controller, sdl::GameControllerAxis axis, int16_t value) {
    if(!config::CONFIG.controllerAnalogStick) {
        return;
    }

    if(axis == sdl::GameControllerAxisValues::LEFTX || axis == sdl::GameControllerAxisValues::LEFTY) {
        unsigned int index = axis == sdl::GameControllerAxisValues::LEFTX  ? 0 : 1;
        if(std::abs(value) < controller_axis_input_threshold) {
            last_controller_axis_input[index] = std::nullopt;
            last_controller_axis_input_time[index] = std::chrono::system_clock::now();
            return;
        }
        direction dir = axis == sdl::GameControllerAxisValues::LEFTX  ? (value > 0 ? direction::right : direction::left)
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

void main_menu::error_rumble(sdl::GameController* controller) {
    if(!config::CONFIG.controllerRumble) {
        return;
    }
    sdl::GameControllerRumble(controller, 1000, 10000, 100);
}

bool main_menu::select_relative(direction dir) {
    if(!in_submenu) {
        if(dir == direction::left) {
            if(selected > 0) {
                select(selected-1);
                // if(Mix_PlayChannel(-1, ok_sound.get(), 0) == -1) {
                //     spdlog::error("Mix_PlayChannel: {}", Mix_GetError());
                // }

                return true;
            }
        } else if(dir == direction::right) {
            if(selected < menus.size()-1) {
                select(selected+1);
                // if(Mix_PlayChannel(-1, ok_sound.get(), 0) == -1) {
                //     spdlog::error("Mix_PlayChannel: {}", Mix_GetError());
                // }

                return true;
            }
        } else if(dir == direction::up) {
            auto& menu = menus[selected];
            if(menu->get_selected_submenu() > 0) {
                select_menu_item(menu->get_selected_submenu()-1);
                // if(Mix_PlayChannel(-1, ok_sound.get(), 0) == -1) {
                //     spdlog::error("Mix_PlayChannel: {}", Mix_GetError());
                // }

                return true;
            }
        } else if(dir == direction::down) {
            auto& menu = menus[selected];
            if(menu->get_selected_submenu() < menu->get_submenus_count()-1) {
                select_menu_item(menu->get_selected_submenu()+1);
                // if(Mix_PlayChannel(-1, ok_sound.get(), 0) == -1) {
                //     spdlog::error("Mix_PlayChannel: {}", Mix_GetError());
                // }

                return true;
            }
        }
    } else {
        auto& menu = current_submenu;
        if(dir == direction::up) {
            if(menu->get_selected_submenu() > 0) {
                select_submenu_item(menu->get_selected_submenu()-1);
                // if(Mix_PlayChannel(-1, ok_sound.get(), 0) == -1) {
                //     spdlog::error("Mix_PlayChannel: {}", Mix_GetError());
                // }

                return true;
            }
        } else if(dir == direction::down) {
            if(menu->get_selected_submenu() < menu->get_submenus_count()-1) {
                select_submenu_item(menu->get_selected_submenu()+1);
                // if(Mix_PlayChannel(-1, ok_sound.get(), 0) == -1) {
                //     spdlog::error("Mix_PlayChannel: {}", Mix_GetError());
                // }

                return true;
            }
        }
    }
    return false;
}
bool main_menu::activate_current() {
    auto& menu = *menus[selected];
    auto res = menu.activate();
    if(res == menu::result::submenu) {
        if(auto submenu = dynamic_cast<menu::menu*>(&menu.get_submenu(menu.get_selected_submenu()))) {
            if(submenu->get_submenus_count() > 0 && !in_submenu) {
                current_submenu = submenu;
                current_submenu->on_open();

                in_submenu = true;
                last_submenu_transition = std::chrono::system_clock::now();
                return true;
            }
        }
        return false;
    }
    return menus[selected]->activate() == menu::result::success;
}
bool main_menu::back() {
    if(in_submenu) {
        current_submenu->on_close();
        current_submenu = nullptr;

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

    menus[last_selected]->on_close();
    menus[selected]->on_open();

    last_selected_menu_item = menus[selected]->get_selected_submenu();
}
void main_menu::select_menu_item(int index) {
    auto& menu = menus[selected];
    if(index == menu->get_selected_submenu()) {
        return;
    }
    if(index < 0 || index >= menu->get_submenus_count()) {
        return;
    }

    last_selected_menu_item = menu->get_selected_submenu();
    last_selected_menu_item_transition = std::chrono::system_clock::now();
    menu->select_submenu(index);
}
void main_menu::select_submenu_item(int index) {
    auto& menu = current_submenu;
    if(index == menu->get_selected_submenu()) {
        return;
    }
    if(index < 0 || index >= menu->get_submenus_count()) {
        return;
    }

    last_selected_submenu_item = menu->get_selected_submenu();
    last_selected_submenu_item_transition = std::chrono::system_clock::now();
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

void main_menu::render(dreamrender::gui_renderer& renderer) {
    tick(); // TODO: This should be called from the outside

    constexpr glm::vec4 active_color(1.0f, 1.0f, 1.0f, 1.0f);
    constexpr glm::vec4 inactive_color(0.25f, 0.25f, 0.25f, 0.25f);

    auto now = std::chrono::system_clock::now();

    auto time_since_transition = std::chrono::duration<double>(now - last_submenu_transition);
    double partial = std::clamp(time_since_transition / transition_submenu_activate_duration, 0.0, 1.0);
    partial = in_submenu ? partial : 1.0 - partial;
    bool in_submenu_now = in_submenu || partial > 0.0;

    renderer.set_color(glm::mix(active_color, inactive_color, partial));
    render_crossbar(renderer, now);
    renderer.set_color(active_color);
    if(in_submenu_now) {
        render_submenu(renderer, now);
    }
}

void main_menu::render_crossbar(dreamrender::gui_renderer& renderer, time_point now) {
    double submenu_transition = std::clamp(
        std::chrono::duration<double>(now - last_submenu_transition) / transition_submenu_activate_duration, 0.0, 1.0);
    submenu_transition = in_submenu ? submenu_transition : 1.0 - submenu_transition;
    bool in_submenu_now = in_submenu || submenu_transition > 0.0;

    const glm::vec2 base_pos = glm::mix(
        glm::vec2(0.35f/renderer.aspect_ratio, 0.25f),
        glm::vec2(0.30f/renderer.aspect_ratio, 0.25f),
        submenu_transition);
    const double base_size = glm::mix(0.1, 0.075, submenu_transition);

    float real_selection;
    int selected = this->selected;

    if(selected == last_selected) {
        real_selection = selected;
    } else {
        auto time_since_transition = std::chrono::duration<double>(now - last_selected_transition);
        real_selection = last_selected + (selected - last_selected) *
            time_since_transition / transition_duration;
        if(selected > last_selected && real_selection > selected) {
            real_selection = selected;
        } else if(selected < last_selected && real_selection < selected) {
            real_selection = selected;
        }
    }

    const auto selected_menu_x = base_pos.x;
    float x = selected_menu_x - (base_size*1.5f)/renderer.aspect_ratio*real_selection;

    for (int i = 0; i < menus.size(); i++) {
        if(i == selected && in_submenu_now) {
            x += (base_size*1.5f)/renderer.aspect_ratio;
            continue; // the selected menu is rendered as part of the submenu
        }

        auto& menu = menus[i];
        renderer.draw_image(menu->get_icon(), x, base_pos.y, base_size, base_size);
        if(i == selected) {
            renderer.draw_text(menu->get_name(), x+(base_size*0.5f)/renderer.aspect_ratio, base_pos.y+base_size, base_size*0.4f, glm::vec4(1, 1, 1, 1), true);
        }
        x += (base_size*1.5f)/renderer.aspect_ratio;
    }

    // TODO: transition / animation
    x = selected_menu_x - ((base_size*1.5f)/renderer.aspect_ratio)*(real_selection - selected);
    auto& menu = menus[selected];
    int selected_submenu = menu->get_selected_submenu();
    float partial_transition = 1.0f;
    float partial_y = 0.0f;
    if(selected_submenu != last_selected_menu_item) {
        auto time_since_transition = std::chrono::duration<double>(now - last_selected_menu_item_transition);
        if(time_since_transition > transition_menu_item_duration) {
            last_selected_menu_item = selected_submenu;
        } else {
            partial_transition = time_since_transition / transition_menu_item_duration;
            partial_y = (selected_submenu - last_selected_menu_item) * (1.0f - partial_transition);
        }
    }
    {
        float y = base_pos.y - (base_size*1.5f) - (base_size*0.65f) + partial_y*base_size*1.5f;
        if(last_selected_menu_item > selected_submenu) {
            y += base_size*glm::mix(0.65f, 1.5f, 1.0f-partial_transition);
        } else if(last_selected_menu_item < selected_submenu) {
            y += base_size*glm::mix(0.65f, 1.5f, 1.0f-partial_transition);
        } else {
            y += base_size*0.65f;
        }
        for(int i=selected_submenu-1; i >= 0 && y >= -base_size*0.65f; i--) {
            auto& submenu = menu->get_submenu(i);
            renderer.draw_image(submenu.get_icon(), x+(base_size*0.2f)/renderer.aspect_ratio, y, base_size*0.6f, base_size*0.6f);
            if(!in_submenu_now)
                renderer.draw_text(submenu.get_name(), x+(base_size*1.5f)/renderer.aspect_ratio, y+(base_size*0.3f), base_size*0.4f, glm::vec4(0.7, 0.7, 0.7, 1), false, true);
            y -= base_size*0.65f;
        }
    }
    {
        float y = base_pos.y + (base_size*1.5f) - (base_size*0.65f) + partial_y*base_size*1.5f;
        if(last_selected_menu_item > selected_submenu) {
            y += base_size*glm::mix(0.65f, 1.5f, 1.0f-partial_transition);
        } else {
            y += base_size*0.65f;
        }
        for(int i=selected_submenu, count = menu->get_submenus_count(); i<count && y < 1.0f; i++) {
            auto& submenu = menu->get_submenu(i);
            if(i == selected_submenu) {
                if(!in_submenu_now) {
                    double size = base_size*glm::mix(0.6, 1.2, partial_transition);
                    renderer.draw_image(submenu.get_icon(), x+(base_size*0.5f-size/2.0f)/renderer.aspect_ratio, y, size, size);
                    if(!in_submenu_now)
                        renderer.draw_text(submenu.get_name(), x+(base_size*1.5f)/renderer.aspect_ratio, y+size/2, size/2, glm::vec4(1, 1, 1, 1), false, true);
                }
                y += base_size*glm::mix(0.65f, 1.5f, partial_transition);
            }
            else if(i == last_selected_menu_item) {
                double size = base_size*glm::mix(0.6, 1.2, 1.0f-partial_transition);
                renderer.draw_image(submenu.get_icon(), x+(0.05f-size/2.0f)/renderer.aspect_ratio, y, size, size);
                if(!in_submenu_now)
                    renderer.draw_text(submenu.get_name(), x+(base_size*1.5f)/renderer.aspect_ratio, y+size/2, size/2, glm::vec4(1, 1, 1, 1), false, true);
                y += base_size*glm::mix(0.65f, 1.5f, 1.0f-partial_transition);
            }
            else {
                renderer.draw_image(submenu.get_icon(), x+(base_size*0.2f)/renderer.aspect_ratio, y, base_size*0.6f, base_size*0.6f);
                if(!in_submenu_now)
                    renderer.draw_text(submenu.get_name(), x+(base_size*1.5f)/renderer.aspect_ratio, y+base_size*0.3f, base_size*0.4f, glm::vec4(0.7, 0.7, 0.7, 1), false, true);
                y += base_size*0.65f;
            }
        }
    }
}

void main_menu::render_submenu(dreamrender::gui_renderer& renderer, time_point now) {
    double submenu_transition = std::clamp(
        std::chrono::duration<double>(now - last_submenu_transition) / transition_submenu_activate_duration, 0.0, 1.0);
    submenu_transition = in_submenu ? submenu_transition : 1.0 - submenu_transition;

    constexpr auto offset = (0.1f-0.075f)/2.0f;
    const glm::vec2 base_pos = glm::mix(
        glm::vec2(0.35f/renderer.aspect_ratio, 0.25f),
        glm::vec2((0.15f-offset)/renderer.aspect_ratio, 0.25f-2*offset),
        submenu_transition);
    const double base_size = 0.1;

    const auto& selected_menu = *menus[selected];
    const auto& selected_submenu = selected_menu.get_submenu(selected_menu.get_selected_submenu());

    renderer.draw_image(selected_menu.get_icon(), base_pos.x, base_pos.y, 0.1f, 0.1f);
    renderer.draw_image(selected_submenu.get_icon(), base_pos.x, base_pos.y+0.15f, 0.1f, 0.1f);

    if(!in_submenu)
        return;
    if(const auto* submenu = dynamic_cast<const menu::menu*>(&selected_submenu)) {
        double selected = submenu->get_selected_submenu();
        auto time_since_transition = std::chrono::duration<double>(now - last_selected_submenu_item_transition);
        if(time_since_transition < transition_submenu_item_duration) {
            selected = last_selected_submenu_item + (selected - last_selected_submenu_item) *
                time_since_transition / transition_submenu_item_duration;
        }

        double offsetY = 0.15f - selected*0.15f;

        for(int i=0; i<submenu->get_submenus_count(); i++) {
            double partial_selection = 0.0;
            if(i == selected) {
                partial_selection = std::clamp(time_since_transition / transition_submenu_item_duration, 0.0, 1.0);
            }

            double size = base_size*glm::mix(0.75, 1.0, partial_selection);
            double offset = (base_size - size) / 4.0;

            auto& entry = submenu->get_submenu(i);
            renderer.draw_image(entry.get_icon(), base_pos.x + 0.1 + offset, base_pos.y+offsetY+0.15f*i, size, size);
            renderer.draw_text(entry.get_name(), base_pos.x + 0.2, base_pos.y+offsetY+0.15f*i+size/2, size/2, glm::vec4(1, 1, 1, 1), false, true);
        }
    }
}

}
