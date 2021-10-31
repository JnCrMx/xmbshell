#include "dbus.hpp"
#include "render/window.hpp"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

MyControl::MyControl(simppl::dbus::Dispatcher& disp) : simppl::dbus::Skeleton<backgroundwave::Control>(disp, "myControl")
{
	color = std::tuple<int, int, int>{0, 0, 0};
	speed = 1.0;

	shutdown >> [this, &disp](){
		spdlog::info("Received shutdown from D-Bus");
		glfwSetWindowShouldClose(win->win, GLFW_TRUE);
		disp.stop();
	};
}
