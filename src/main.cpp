#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <glibmm/init.h>

#include "config.hpp"
#include "dbus.hpp"
#include "render/window.hpp"
#include "app/xmbshell.hpp"

int main(int argc, char *argv[])
{
	Glib::init();
	config::CONFIG.load();

	spdlog::cfg::load_env_levels();
#ifndef NDEBUG
	spdlog::set_level(spdlog::level::debug);
#endif
	spdlog::info("Welcome to your XMB!");

	render::window window;
	window.init();

	dbus::dbus_server server(&window);
	server.run();

	app::xmbshell* shell = new app::xmbshell(&window);
	window.set_phase(shell);

	window.loop();

	return 0;
}
