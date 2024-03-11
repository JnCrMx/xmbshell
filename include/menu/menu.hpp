#pragma once

#include <string>
#include "render/texture.hpp"

class menu {
    public:
        virtual ~menu() = default;

        virtual const std::string& get_name() const = 0;
        virtual const render::texture& get_icon() const = 0;
};

class simple_menu : public menu {
    public:
        simple_menu(const std::string& name, render::texture&& icon) : name(name), icon(std::move(icon)) {}
        ~simple_menu() override = default;

        const std::string& get_name() const override {
            return name;
        }
        const render::texture& get_icon() const override {
            return icon;
        }
        render::texture& get_icon() {
            return icon;
        }
    private:
        std::string name;
        render::texture icon;
};
