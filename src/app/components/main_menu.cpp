#include "app/components/main_menu.hpp"
#include <spdlog/spdlog.h>

namespace app {

main_menu::main_menu() {

}

static std::unique_ptr<simple_menu> create_simple_menu(const std::string& name, const std::string& icon_path,
    vk::Device device, vma::Allocator allocator, render::resource_loader& loader)
{
    render::texture icon(device, allocator);
    auto menu = std::make_unique<simple_menu>(name, std::move(icon));
    loader.loadTexture(&menu->get_icon(), icon_path);
    return menu;
}

void main_menu::preload(vk::Device device, vma::Allocator allocator, render::resource_loader& loader) {
    ok_sound = SoundChunk(Mix_LoadWAV("sounds/ok.wav"));
    if(!ok_sound) {
        spdlog::error("Mix_LoadWAV: {}", Mix_GetError());
    }

    menus.push_back(create_simple_menu("Users", "icons/icon_category_users.png", device, allocator, loader));
    menus.push_back(create_simple_menu("Settings", "icons/icon_category_settings.png", device, allocator, loader));
    menus.push_back(create_simple_menu("Photo", "icons/icon_category_photo.png", device, allocator, loader));
    menus.push_back(create_simple_menu("Music", "icons/icon_category_music.png", device, allocator, loader));
    menus.push_back(create_simple_menu("Video", "icons/icon_category_video.png", device, allocator, loader));
    menus.push_back(create_simple_menu("TV", "icons/icon_category_tv.png", device, allocator, loader));
    menus.push_back(create_simple_menu("Game", "icons/icon_category_game.png", device, allocator, loader));
    menus.push_back(create_simple_menu("Network", "icons/icon_category_network.png", device, allocator, loader));
    menus.push_back(create_simple_menu("Friends", "icons/icon_category_friends.png", device, allocator, loader));
}

void main_menu::key_down(SDL_Keysym key) {
    if(key.sym == SDLK_LEFT) {
        select_relative(direction::left);
    } else if(key.sym == SDLK_RIGHT) {
        select_relative(direction::right);
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
    }
}
void main_menu::button_up(SDL_GameController* controller, SDL_GameControllerButton button) {
    last_controller_button_input = std::nullopt;
}
void main_menu::axis_motion(SDL_GameController* controller, SDL_GameControllerAxis axis, Sint16 value) {
    if(axis == SDL_CONTROLLER_AXIS_LEFTX) {
        if(std::abs(value) < controller_axis_input_threshold) {
            last_controller_axis_input = std::nullopt;
            last_controller_axis_input_time = std::chrono::system_clock::now();
            return;
        }
        direction dir = value < 0 ? direction::left : direction::right;
        if(last_controller_axis_input && std::get<1>(*last_controller_axis_input) == dir) {
            return;
        }
        if(!select_relative(dir)) {
            error_rumble(controller);
        }
        last_controller_axis_input = std::make_tuple(controller, dir);
        last_controller_axis_input_time = std::chrono::system_clock::now();
    }
}

void main_menu::error_rumble(SDL_GameController* controller) {
    SDL_GameControllerRumble(controller, 1000, 10000, 100);
}

bool main_menu::select_relative(direction dir) {
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
}

void main_menu::tick() {
    if(last_controller_axis_input) {
        auto time_since_input = std::chrono::duration<double>(std::chrono::system_clock::now() - last_controller_axis_input_time);
        if(time_since_input > controller_axis_input_duration) {
            auto [controller, dir] = *last_controller_axis_input;
            if(!select_relative(dir)) {
                error_rumble(controller);
            }
            last_controller_axis_input_time = std::chrono::system_clock::now();
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
    float x = 0.35f/renderer.aspect_ratio - 0.15f/renderer.aspect_ratio*real_selection;

    for (int i = 0; i < menus.size(); i++) {
        auto& menu = menus[i];
        renderer.draw_image(menu->get_icon(), x, 0.25f, 0.1f, 0.1f);
        if(i == selected) {
            renderer.draw_text(menu->get_name(), x+0.05f/renderer.aspect_ratio, 0.35f, 0.04f, glm::vec4(1, 1, 1, 1), true);
        }
        x += 0.15f/renderer.aspect_ratio;
    }
}

}
