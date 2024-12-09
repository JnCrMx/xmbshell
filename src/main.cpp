#include <libintl.h>
#include <thread>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

import sdl2;
import spdlog;
import glibmm;
import giomm;
import dreamrender;
import xmbshell.app;
import xmbshell.dbus;
import xmbshell.config;

std::string find_visualid() {
    std::unique_ptr<Display, decltype(&XCloseDisplay)> display(XOpenDisplay(nullptr), XCloseDisplay);
    if (!display) {
        throw std::runtime_error("Failed to open X display");
    }
    XVisualInfo visualInfo;
    if (!XMatchVisualInfo(display.get(), DefaultScreen(display.get()), 32, TrueColor, &visualInfo)) {
        throw std::runtime_error("Failed to find visual info");
    }
    return std::to_string(visualInfo.visualid);
}

int main(int argc, char *argv[])
{
#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
    spdlog::cfg::load_env_levels();

    spdlog::info("Welcome to your XMB!");

    Gio::init();
    setlocale(LC_ALL, "");
    textdomain("xmbshell");

    Glib::RefPtr<Glib::MainLoop> loop;
#if __cpp_lib_jthread >= 201911L
    std::jthread main_loop_thread([&loop]() {
#else
    std::thread main_loop_thread([&loop]() {
#endif
        loop = Glib::MainLoop::create();
        loop->run();
    });

    config::CONFIG.load();
    spdlog::debug("Config loaded");

    dreamrender::window_config window_config;
    window_config.name = "xmbshell";
    window_config.title = "XMB";
    window_config.preferredPresentMode = config::CONFIG.preferredPresentMode;
    window_config.sampleCount = config::CONFIG.sampleCount;
    window_config.fpsLimit = config::CONFIG.maxFPS;
    window_config.sdl_hints[sdl::hints::video_x11_window_visualid] = find_visualid();
    dreamrender::window window{window_config};
    window.init();

    app::xmbshell* shell = new app::xmbshell(&window);
    window.set_phase(shell, shell, shell);

    dbus::dbus_server server(&window, shell);
    server.run();

    window.loop();

    loop->quit();
#if __cpp_lib_jthread >= 201911L
#else
    main_loop_thread.join();
#endif

    return 0;
}
