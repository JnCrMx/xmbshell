#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <giomm/init.h>
#include <glibmm/main.h>
#include <libintl.h>

#include "config.hpp"
#include "dbus.hpp"
#include "render/window.hpp"
#include "app/xmbshell.hpp"

int main(int argc, char *argv[])
{
	Gio::init();
	setlocale (LC_ALL, "");
	textdomain("xmbshell");

	Glib::RefPtr<Glib::MainLoop> loop;
	std::jthread main_loop_thread([&loop]() {
		loop = Glib::MainLoop::create();
		loop->run();
	});

	config::CONFIG.load();

#ifndef NDEBUG
	spdlog::set_level(spdlog::level::debug);
#endif
	spdlog::cfg::load_env_levels();

	spdlog::info("Welcome to your XMB!");

	render::window window;
	window.init();

	dbus::dbus_server server(&window);
	server.run();

	app::xmbshell* shell = new app::xmbshell(&window);
	window.set_phase(shell, shell, shell);

	window.loop();

	loop->quit();

	return 0;
}
