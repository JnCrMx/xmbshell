#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include "render/window.hpp"
#include "render/app/render_shell.hpp"

#include "config.hpp"

#include <future>
#include <thread>

int main(int argc, char *argv[])
{
	spdlog::cfg::load_env_levels();
#ifndef NDEBUG
	spdlog::set_level(spdlog::level::debug);
#endif
	spdlog::info("Welcome to your XMB!");

	render::window window;
	window.init();

	render::render_shell* renderer = new render::render_shell(&window);
	window.set_phase(renderer);

	window.loop();

	return 0;
}