module;

#include <functional>
#include <sstream>
#include <string>
#include <unordered_set>

export module xmbshell.app:applications_menu;
import :menu_base;
import dreamrender;
import glibmm;
import giomm;
import spdlog;

namespace app {
    class xmbshell;
}

export namespace menu {
    using AppFilter = std::function<bool(const Gio::DesktopAppInfo&)>;

    constexpr auto andFilter(AppFilter a, AppFilter b) {
        return [a, b](const Gio::DesktopAppInfo& app) {
            return a(app) && b(app);
        };
    }
    constexpr auto noFilter() {
        return [](const Gio::DesktopAppInfo&) {
            return true;
        };
    }
    constexpr auto categoryFilter(const std::string& category) {
        return [category](const Gio::DesktopAppInfo& app) {
            std::istringstream iss(app.get_categories());
            std::string c;
            while(std::getline(iss, c, ';')) {
                if(c == category) {
                    return true;
                }
            }
            return false;
        };
    }
    constexpr auto excludeFilter(const std::unordered_set<std::string>& ids) {
        return [ids](const Gio::DesktopAppInfo& app) {
            return !ids.contains(app.get_id());
        };
    }

    class applications_menu : public simple_menu {
        public:
            applications_menu(const std::string& name, dreamrender::texture&& icon, app::xmbshell* xmb, dreamrender::resource_loader& loader, AppFilter filter = noFilter());
            ~applications_menu() override = default;
        private:
            std::vector<Glib::RefPtr<Gio::DesktopAppInfo>> apps;
    };

}
