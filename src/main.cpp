#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <glibmm/init.h>

#include "config.hpp"
#include "dbus.hpp"
#include "render/window.hpp"
#include "render/app/render_shell.hpp"

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

	render::render_shell* renderer = new render::render_shell(&window);
	window.set_phase(renderer);

	window.loop();

	return 0;
}
