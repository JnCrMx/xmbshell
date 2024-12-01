module;

#include <sdbus-c++/sdbus-c++.h>

export module xmbshell.dbus;
import dreamrender;
import xmbshell.app;

export namespace dbus
{
    class dbus_server
    {
        public:
            dbus_server(dreamrender::window* w, app::xmbshell* xmb);
            ~dbus_server();
            void run();
        private:
            std::unique_ptr<sdbus::IConnection> connection;
            std::unique_ptr<sdbus::IObject> object;
            dreamrender::window* win;
            app::xmbshell* xmb;
    };
}
