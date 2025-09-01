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

#include <giomm.h>

export module giomm;

export namespace Gio {
    using Gio::init;
#if __linux__
    using Gio::DesktopAppInfo;
#endif
    using Gio::AppInfo;
    using Gio::Icon;
    using Gio::FileIcon;
    using Gio::ThemedIcon;
    using Gio::File;
    using Gio::FileInfo;
    using Gio::FileType;
    using Gio::Settings;
    using Gio::SettingsSchema;
    using Gio::SettingsSchemaSource;
    using Gio::SettingsSchemaKey;
    using Gio::Error;
    using Gio::FileQueryInfoFlags;

    namespace DBus {
        using Gio::DBus::Proxy;
        using Gio::DBus::Connection;
        using Gio::DBus::BusType;
        using Gio::DBus::Error;
    }
};
