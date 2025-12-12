/* XMBShell, a console-like desktop shell
 * Copyright (C) 2025 - JCM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
module;

#include <chrono>
#include <filesystem>
#include <future>
#include <source_location>
#include <utility>
#include <variant>

#include <glm/glm.hpp>

export module xmbshell.utils;

import giomm;
import sdl2;

export enum class result {
    unsupported  = (1<<0),
    success      = (1<<1),
    failure      = (1<<2),

    submenu	     = (1<<3),
    close        = (1<<4),

    ok_sound     = (1<<5),
    error_rumble = (1<<6),
};
export inline result operator|(result a, result b) {
    return static_cast<result>(static_cast<int>(a) | static_cast<int>(b));
}
export inline bool operator&(result a, result b) {
    return static_cast<bool>(static_cast<int>(a) & static_cast<int>(b));
}

export enum class action {
    none = 0,
    left = 1,
    right = 2,
    up = 3,
    down = 4,
    ok = 5,
    cancel = 6,
    options = 7,
    extra = 8,

    _length,
};

export namespace events {
    enum class logical_controller_button {
        invalid = sdl::GameControllerButtonValues::INVALID,
        a = sdl::GameControllerButtonValues::A,
        b = sdl::GameControllerButtonValues::B,
        x = sdl::GameControllerButtonValues::X,
        y = sdl::GameControllerButtonValues::Y,
        back = sdl::GameControllerButtonValues::BACK,
        guide = sdl::GameControllerButtonValues::GUIDE,
        start = sdl::GameControllerButtonValues::START,
        leftstick = sdl::GameControllerButtonValues::LEFTSTICK,
        rightstick = sdl::GameControllerButtonValues::RIGHTSTICK,
        leftshoulder = sdl::GameControllerButtonValues::LEFTSHOULDER,
        rightshoulder = sdl::GameControllerButtonValues::RIGHTSHOULDER,
        dpad_up = sdl::GameControllerButtonValues::DPAD_UP,
        dpad_down = sdl::GameControllerButtonValues::DPAD_DOWN,
        dpad_left = sdl::GameControllerButtonValues::DPAD_LEFT,
        dpad_right = sdl::GameControllerButtonValues::DPAD_RIGHT,
        misc1 = sdl::GameControllerButtonValues::MISC1,
        paddle1 = sdl::GameControllerButtonValues::PADDLE1,
        paddle2 = sdl::GameControllerButtonValues::PADDLE2,
        paddle3 = sdl::GameControllerButtonValues::PADDLE3,
        paddle4 = sdl::GameControllerButtonValues::PADDLE4,
        touchpad = sdl::GameControllerButtonValues::TOUCHPAD,
    };
    struct controller_button_down {
        logical_controller_button button;

        controller_button_down(sdl::GameControllerButton b) : button(static_cast<logical_controller_button>(std::to_underlying(b))) {}
    };
    struct controller_button_up {
        logical_controller_button button;

        controller_button_up(sdl::GameControllerButton b) : button(static_cast<logical_controller_button>(std::to_underlying(b))) {}
    };

    enum class logical_joystick_index {
        left = 0,
        right = 1
    };
    struct joystick_axis {
        logical_joystick_index index;
        float x;
        float y;

        joystick_axis(unsigned int index, float x, float y) : index(static_cast<logical_joystick_index>(index)), x(x), y(y) {}
    };
    struct mouse_move {
        float x;
        float y;
        float xrel;
        float yrel;
    };
    struct mouse_scroll {
        float x;
    };

    enum class logical_mouse_button {
        left,
        middle,
        right,
        x1,
        x2
    };
    struct mouse_button_down {
        logical_mouse_button button;
    };
    struct mouse_button_up {
        logical_mouse_button button;
    };

    struct key_down {
        unsigned int keycode;

        key_down(const sdl::Keysym& sym) : keycode(std::to_underlying(sym.scancode)) {}
    };
    struct key_up {
        unsigned int keycode;

        key_up(const sdl::Keysym& sym) : keycode(std::to_underlying(sym.scancode)) {}
    };

    struct cursor_move {
        float x;
        float y;
    };
}

export struct event {
    action action;

    std::variant<std::monostate,
                 events::controller_button_down, events::controller_button_up,
                 events::joystick_axis,
                 events::mouse_move, events::mouse_scroll, events::mouse_button_down, events::mouse_button_up,
                 events::key_down, events::key_up,
                 events::cursor_move
                > data;

    template<typename T>
    bool is() const {
        return std::holds_alternative<T>(data);
    }

    template<typename T>
    T* get() {
        return std::get_if<T>(&data);
    }

    template<typename T>
    const T* get() const {
        return std::get_if<T>(&data);
    }

    template<typename T, typename Pred>
    bool test(Pred pred, bool default_value = false) const {
        if(auto* d = std::get_if<T>(&data)) {
            return pred(*d);
        }
        return default_value;
    }
};

export class action_receiver {
    public:
        virtual ~action_receiver() = default;
        virtual result on_action(action action) {
            return result::unsupported;
        }
        virtual result on_event(const event& event) {
            return on_action(event.action);
        }
};

export namespace utils
{
    std::optional<std::filesystem::path> resolve_icon(const Gio::Icon* icon);

    template<typename R>
    bool is_ready(std::future<R> const& f)
    { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

    template<typename R>
    bool is_ready(std::shared_future<R> const& f)
    { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

    std::string to_fixed_string(double d, int n);

    template<int n, typename T>
    std::string to_fixed_string(T d)
    {
        return to_fixed_string(d, n);
    }

    using time_point = std::chrono::time_point<std::chrono::system_clock>;
    constexpr double progress(time_point now, time_point start, std::chrono::duration<double> duration) {
        double d = std::chrono::duration<double>(now - start).count() / duration.count();
        return std::clamp(d, 0.0, 1.0);
    }

    template<typename T>
    class aligned_wrapper {
        public:
            aligned_wrapper(void* data, std::size_t alignment) :
                data(static_cast<std::byte*>(data)), alignment(alignment) {}

            T& operator[](std::size_t i) {
                return *reinterpret_cast<T*>(data + i * aligned(sizeof(T), alignment)); // NOLINT
            }

            const T& operator[](std::size_t i) const {
                return *reinterpret_cast<const T*>(data + i * aligned(sizeof(T), alignment)); // NOLINT
            }

            T* operator+(std::size_t i) {
                return reinterpret_cast<T*>(data + i * aligned(sizeof(T), alignment)); // NOLINT
            }

            std::size_t offset(std::size_t i) const {
                return i * aligned(sizeof(T), alignment);
            }
        private:
            constexpr static std::size_t aligned(std::size_t size, std::size_t alignment) {
                return (size + alignment - 1) & ~(alignment - 1);
            }

            std::byte* data;
            std::size_t alignment;
    };

    std::string demangle(const char* name);

    template <class T>
    std::string type_name(const T& t) {
        return demangle(typeid(t).name());
    }
}

namespace utils {
    template<typename T>
    constexpr std::string_view get_typename() {
        std::string_view name = std::source_location::current().function_name();
        name.remove_prefix(std::string_view::traits_type::length("std::string_view utils::get_typename()"));
        name.remove_prefix(std::string_view::traits_type::length(" [T = "));
        name.remove_suffix(std::string_view::traits_type::length("]"));
        return name;
    }

    // based on https://stackoverflow.com/questions/28828957/how-to-convert-an-enum-to-a-string-in-modern-c#comment115999268_55312360
    template<auto... values>
    constexpr std::string_view get_enum_values_impl() {
        return std::source_location::current().function_name();
    }

    template<size_t N>
    constexpr std::array<std::string_view, N> parse_enum_list(std::string_view sv, size_t prefix = 0) {
        std::array<std::string_view, N> array;
        size_t p = prefix + 2;
        for(size_t i = 0; i<N; i++) {
            size_t np = sv.find(",", p);
            auto name = sv.substr(p, np-p);
            array[i] = name;
            p = np + prefix + 2 + 2;
        }
        return array;
    }

    // based on https://stackoverflow.com/questions/28828957/how-to-convert-an-enum-to-a-string-in-modern-c#comment115999268_55312360
    template<typename Enum, auto... values>
    constexpr std::string_view get_enum_values_str(std::index_sequence<values...>) {
        auto sv = get_enum_values_impl<static_cast<Enum>(values)...>();
        sv.remove_prefix(std::string_view::traits_type::length("std::string_view utils::get_enum_values_impl() [values = <"));
        sv.remove_suffix(std::string_view::traits_type::length(">]"));

        return sv;
    }

    template<typename Enum, size_t N = 256>
    constexpr std::array<std::string_view, N> get_enum_values() {
        constexpr auto sv = get_enum_values_str<Enum>(std::make_index_sequence<N>());
        return parse_enum_list<N>(sv, get_typename<Enum>().size());
    }

    export template<typename Enum, size_t N = 256>
    constexpr std::string_view enum_name(Enum e) {
        constexpr auto list = get_enum_values<Enum, N>();
        return list[std::to_underlying(e)];
    }

}
