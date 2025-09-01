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

export module xmbshell.app:users_menu;

import :menu_base;

import dreamrender;
import glibmm;
import giomm;

namespace app {
    class xmbshell;
}

export namespace menu {

class users_menu : public simple_menu {
    public:
        users_menu(std::string name, dreamrender::texture&& icon, app::xmbshell* xmb, dreamrender::resource_loader& loader);
        ~users_menu() override = default;
    private:
        Glib::RefPtr<Gio::DBus::Proxy> login1, accounts;
};

}
