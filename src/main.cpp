#include <libintl.h>
#include <thread>

import spdlog;
import glibmm;
import giomm;
import dreamrender;
import xmbshell.app;
import xmbshell.dbus;
import xmbshell.config;

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

	dreamrender::window_config window_config;
	window_config.name = "xmbshell";
	window_config.title = "XMB";
	window_config.preferredPresentMode = config::CONFIG.preferredPresentMode;
	window_config.sampleCount = config::CONFIG.sampleCount;
	window_config.fpsLimit = config::CONFIG.maxFPS;
	dreamrender::window window{window_config};
	window.init();

	dbus::dbus_server server(&window);
	server.run();

	app::xmbshell* shell = new app::xmbshell(&window);
	window.set_phase(shell, shell, shell);

	window.loop();

	loop->quit();

	return 0;
}
