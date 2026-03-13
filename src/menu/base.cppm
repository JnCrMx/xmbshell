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

#include <string>
#include <stdexcept>
#include <functional>
#include <memory>

export module xmbshell.app:menu_base;
import dreamrender;
import glm;
import xmbshell.utils;

export namespace menu {

class menu_entry {
    public:
        virtual ~menu_entry() = default;

        virtual result activate(action action) {
            return result::unsupported;
        }
        virtual void draw_icon(dreamrender::gui_renderer& renderer, float x, float y, float w, float h) const = 0;
        virtual void draw_name(dreamrender::gui_renderer& renderer, float x, float y, float size,
            const glm::vec4& color = glm::vec4(1.0f), bool centerH = false, bool centerV = false) const = 0;
        virtual void draw_description(dreamrender::gui_renderer& renderer, float x, float y, float size,
            const glm::vec4& color = glm::vec4(1.0f), bool centerH = false, bool centerV = false) const = 0;
        virtual glm::vec2 measure_name(dreamrender::gui_renderer& renderer, float size) const = 0;
        virtual glm::vec2 measure_description(dreamrender::gui_renderer& renderer, float size) const = 0;
};

class menu : public menu_entry {
    public:
        virtual unsigned int get_submenus_count() const {
            return 0;
        }
        virtual unsigned int get_selected_submenu() const {
            return 0;
        }
        virtual void select_submenu(unsigned int index) {
        }
        virtual menu_entry& get_submenu(unsigned int index) const {
            throw std::out_of_range("Index out of range");
        }
        virtual void on_open() {
        }
        virtual void on_close() {
        }
        virtual void get_button_actions(std::vector<std::pair<action, std::string>>& v) {

        }
};

template<typename T>
class default_rendered : public T {
    public:
        virtual const dreamrender::texture& get_icon() const = 0;
        virtual std::string_view get_name() const = 0;
        virtual std::string_view get_description() const = 0;
        virtual bool is_enabled() const {
            return true;
        }

        void draw_icon(dreamrender::gui_renderer& renderer, float x, float y, float w, float h) const override {
            renderer.draw_image_a(get_icon(), x, y, w, h);
        }
        void draw_name(dreamrender::gui_renderer& renderer, float x, float y, float size,
            const glm::vec4& color = glm::vec4(1.0f), bool centerH = false, bool centerV = false) const override
        {
            renderer.draw_text(get_name(), x, y, size, is_enabled() ? color : (color * 0.5f), centerH, centerV);
        }
        void draw_description(dreamrender::gui_renderer& renderer, float x, float y, float size,
            const glm::vec4& color = glm::vec4(1.0f), bool centerH = false, bool centerV = false) const override
        {
            renderer.draw_text(get_description(), x, y, size, is_enabled() ? color : (color * 0.5f), centerH, centerV);
        }
        glm::vec2 measure_name(dreamrender::gui_renderer& renderer, float size) const override {
            return renderer.measure_text(get_name(), size);
        }
        glm::vec2 measure_description(dreamrender::gui_renderer& renderer, float size) const override {
            return renderer.measure_text(get_description(), size);
        }
};

template<typename T>
class simple : public default_rendered<T> {
    public:
        using icon_type = dreamrender::texture;

        simple(std::string name, icon_type&& icon, std::string description = "", bool enabled = true) :
            name(std::move(name)), icon(std::move(icon)), description(std::move(description)), enabled(enabled) {}
        ~simple() override = default;

        std::string_view get_name() const override {
            return name;
        }
        std::string_view get_description() const override {
            return description;
        }
        const dreamrender::texture& get_icon() const override {
            return icon;
        }
        dreamrender::texture& get_icon() {
            return icon;
        }
        bool is_enabled() const override {
            return enabled;
        }
    private:
        std::string name;
        std::string description;
        icon_type icon;
        bool enabled;
};

template<typename T>
class simple_shared : public default_rendered<T> {
    public:
        using icon_type = std::shared_ptr<dreamrender::texture>;

        simple_shared(std::string name, icon_type&& icon, std::string description = "", bool enabled = true) :
            name(std::move(name)), icon(std::move(icon)), description(std::move(description)), enabled(enabled) {}
        ~simple_shared() override = default;

        std::string_view get_name() const override {
            return name;
        }
        std::string_view get_description() const override {
            return description;
        }
        const dreamrender::texture& get_icon() const override {
            return *icon;
        }
        dreamrender::texture& get_icon() {
            return *icon;
        }
        bool is_enabled() const override {
            return enabled;
        }
    private:
        std::string name;
        std::string description;
        icon_type icon;
        bool enabled;
};

using simple_menu_shallow = simple<menu>;
using simple_menu_shallow_shared = simple_shared<menu>;
using simple_menu_entry = simple<menu_entry>;
using simple_menu_entry_shared = simple_shared<menu_entry>;

template<typename Base>
class action_menu_entry_generic : public Base {
    public:
        action_menu_entry_generic(
            std::string name, Base::icon_type&& icon,
            std::function<result()> on_activate, std::function<result(action)> on_action = {},
            std::string description = "",
            bool enabled = true
        ) :
            Base(std::move(name), std::move(icon), std::move(description), enabled), on_activate(std::move(on_activate)), on_action(std::move(on_action)) {}
        ~action_menu_entry_generic() override = default;

        result activate(action action) override {
            if(!this->is_enabled()) {
                return result::failure;
            }

            if(on_action) {
                return on_action(action);
            } else if(action != action::ok) {
                return result::unsupported;
            }
            return on_activate();
        }
    private:
        std::function<result()> on_activate;
        std::function<result(action)> on_action;
};
using action_menu_entry = action_menu_entry_generic<simple_menu_entry>;
using action_menu_entry_shared = action_menu_entry_generic<simple_menu_entry_shared>;

template<typename Base>
class simple_menu_generic : public Base {
    public:
        simple_menu_generic(std::string name, Base::icon_type&& icon, std::string description = "") :
            Base(std::move(name), std::move(icon), std::move(description)) {}

        template<std::derived_from<menu_entry> T, std::size_t N>
        simple_menu_generic(const std::string& name, Base::icon_type&& icon, std::array<std::unique_ptr<T>, N>&& entries, std::string description = "") :
            Base(name, std::move(icon), std::move(description))
        {
            for(auto& entry : std::move(entries)) {
                this->entries.push_back(std::move(entry));
            }
        }
        simple_menu_generic(const std::string& name, Base::icon_type&& icon, std::ranges::range auto&& entries, std::string description = "") :
            Base(name, std::move(icon), std::move(description))
        {
            for(auto& entry : std::move(entries)) {
                this->entries.push_back(std::move(entry));
            }
        }
        ~simple_menu_generic() override = default;

        unsigned int get_submenus_count() const override {
            return entries.size();
        }
        menu_entry& get_submenu(unsigned int index) const override {
            return *entries.at(index);
        }
        unsigned int get_selected_submenu() const override {
            return selected_submenu;
        }
        void select_submenu(unsigned int index) override {
            selected_submenu = index;
        }
        result activate(action action) override {
            if(!is_open) {
                return result::submenu;
            }
            if(selected_submenu < entries.size()) {
                return entries.at(selected_submenu)->activate(action);
            }
            return result::unsupported;
        }
        void on_open() override {
            is_open = true;
        }
        void on_close() override {
            is_open = false;
        }
    protected:
        bool is_open = false;
        std::vector<std::unique_ptr<menu_entry>> entries;
        unsigned int selected_submenu = 0;
};
using simple_menu = simple_menu_generic<simple_menu_shallow>;
using simple_menu_shared = simple_menu_generic<simple_menu_shallow_shared>;

}
