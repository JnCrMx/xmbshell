module;

#include <string>
#include <stdexcept>
#include <functional>
#include <memory>

export module xmbshell.app:menu_base;
import dreamrender;
import xmbshell.utils;

export namespace menu {

class menu_entry {
    public:
        virtual ~menu_entry() = default;

        virtual const std::string& get_name() const = 0;
        virtual const dreamrender::texture& get_icon() const = 0;
        virtual result activate(action action) {
            return result::unsupported;
        }
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
class simple : public T {
    public:
        simple(const std::string& name, dreamrender::texture&& icon) : name(name), icon(std::move(icon)) {}
        ~simple() override = default;

        const std::string& get_name() const override {
            return name;
        }
        const dreamrender::texture& get_icon() const override {
            return icon;
        }
        dreamrender::texture& get_icon() {
            return icon;
        }
    private:
        std::string name;
        dreamrender::texture icon;
};

using simple_menu_shallow = simple<menu>;
using simple_menu_entry = simple<menu_entry>;

class action_menu_entry : public simple_menu_entry {
    public:
        action_menu_entry(const std::string& name, dreamrender::texture&& icon,
            std::function<result()> on_activate, std::function<result(action)> on_action = {})
            : simple_menu_entry(name, std::move(icon)), on_activate(on_activate), on_action(on_action) {}
        ~action_menu_entry() override = default;

        result activate(action action) override {
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

class simple_menu : public simple_menu_shallow {
    public:
        simple_menu(const std::string& name, dreamrender::texture&& icon) : simple_menu_shallow(name, std::move(icon)) {}

        template<std::derived_from<menu_entry> T, std::size_t N>
        simple_menu(const std::string& name, dreamrender::texture&& icon, std::array<std::unique_ptr<T>, N>&& entries) : simple_menu_shallow(name, std::move(icon)) {
            for(auto& entry : entries) {
                this->entries.push_back(std::move(entry));
            }
        }
        ~simple_menu() override = default;

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
}
