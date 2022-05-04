#pragma once

#include "render/window.hpp"

#include <sdbus-c++/sdbus-c++.h>

namespace dbus
{
	class dbus_server
	{
		public:
			dbus_server(render::window* w);
			~dbus_server();
			void run();
		private:
			std::unique_ptr<sdbus::IConnection> connection;
			std::unique_ptr<sdbus::IObject> object;
			render::window* win;
	};
}
