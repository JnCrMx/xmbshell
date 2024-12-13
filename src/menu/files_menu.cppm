module;

#include <filesystem>
#include <string>

export module xmbshell.app:files_menu;

import spdlog;
import glibmm;
import giomm;
import dreamrender;
import xmbshell.config;
import :menu_base;
import :menu_utils;

namespace app {
    class xmbshell;
}

export namespace menu {

class files_menu : public simple_menu {
    public:
        files_menu(const std::string& name, dreamrender::texture&& icon, app::xmbshell* xmb, const std::filesystem::path& path, dreamrender::resource_loader& loader);
        ~files_menu() override = default;

        void on_open() override;
        void on_close() override {
            simple_menu::on_close();
            entries.clear();
        }

        unsigned int get_submenus_count() const override {
            return is_open ? entries.size() : 1;
        }
    private:
        app::xmbshell* xmb;
        std::filesystem::path path;
        dreamrender::resource_loader& loader;
};

}
