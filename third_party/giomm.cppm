module;

#include <giomm.h>

export module giomm;

export namespace Gio {
    using Gio::init;
    using Gio::DesktopAppInfo;
    using Gio::AppInfo;
    using Gio::FileIcon;
    using Gio::ThemedIcon;
    using Gio::File;
    using Gio::Settings;
    using Gio::SettingsSchema;
    using Gio::SettingsSchemaSource;
    using Gio::SettingsSchemaKey;

    namespace DBus {
        using Gio::DBus::Proxy;
        using Gio::DBus::Connection;
        using Gio::DBus::BusType;
        using Gio::DBus::Error;
    }
};
