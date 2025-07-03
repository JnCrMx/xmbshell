#include <iostream>
#include <thread>

#include <libintl.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

import sdl2;
import spdlog;
import glibmm;
import giomm;
import dreamrender;
import argparse;
import avcpp;
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

    argparse::ArgumentParser program("XMB Shell");
    program.add_argument("--width")
        .help("Width of the window")
        .metavar("WIDTH")
        .scan<'i', int>()
        .default_value(-1);
    program.add_argument("--height")
        .help("Height of the window")
        .metavar("HEIGHT")
        .scan<'i', int>()
        .default_value(-1);
    program.add_argument("--no-fullscreen").flag()
        .help("Do not start in fullscreen mode");
    program.add_argument("--background-only").flag()
        .help("Only render the background");
    program.add_argument("--headless").flag()
        .help("Run in headless mode (requires --width and --height)");
    program.add_argument("--headless-output-dir")
        .help("Output directory for headless mode")
        .metavar("DIRECTORY")
        .default_value("./output");
    program.add_argument("--headless-output-pattern")
        .help("Output pattern for headless mode")
        .metavar("PATTERN")
        .default_value("frame_{:06d}.png");
    program.add_argument("--terminal").flag()
        .help("Run in terminal mode (requires --width and --height)");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(1);
    }

    spdlog::info("Welcome to your XMB!");
    std::set_terminate([]() {
        spdlog::critical("Uncaught exception");

        try {
            std::rethrow_exception(std::current_exception());
        } catch(const std::exception& e) {
            spdlog::critical("Exception: {}", e.what());
        } catch(const Glib::Exception& e) {
            spdlog::critical("Glib::Exception: {}", static_cast<std::string>(e.what()));
        } catch(...) {
            spdlog::critical("Unknown exception");
        }

        std::abort();
    });

    // Initialize Glib and Gio
    Gio::init();
    setlocale(LC_ALL, "");
    bindtextdomain("xmbshell", config::CONFIG.locale_directory.c_str());
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

    // Initialize avcpp
    av::init();
#ifndef NDEBUG
    av::set_logging_level("debug");
#endif

    config::CONFIG.load();
    spdlog::debug("Config loaded");

    dreamrender::window_config window_config;
    window_config.name = "xmbshell";
    window_config.title = "XMB";
    window_config.preferredPresentMode = config::CONFIG.preferredPresentMode;
    window_config.sampleCount = config::CONFIG.sampleCount;
    window_config.fpsLimit = config::CONFIG.maxFPS;
    if(std::getenv("DISPLAY")) {
        window_config.sdl_hints[sdl::hints::video_x11_window_visualid] = find_visualid();
    }
    window_config.width = program.get<int>("--width");
    window_config.height = program.get<int>("--height");
    window_config.fullscreen = !program.get<bool>("--no-fullscreen");
    window_config.headless = program.get<bool>("--headless") || program.get<bool>("--terminal");
    window_config.headless_terminal = program.get<bool>("--terminal");
    if(window_config.headless && (window_config.width == -1 || window_config.height == -1)) {
        spdlog::error("Headless mode requires --width and --height");
        std::exit(1);
    }
    // This purposefully excludes the case, in which just "--terminal" is used
    if(program.is_used("--headless") || program.is_used("--headless-output-dir") || program.is_used("--headless-output-pattern")) {
        window_config.headless_output_dir = program.get<std::string>("--headless-output-dir");
        window_config.headless_output_format = program.get<std::string>("--headless-output-pattern");
    } else {
        window_config.headless_output_dir.clear();
    }

    dreamrender::window window{window_config};
    window.init();

    app::xmbshell* shell = new app::xmbshell(&window);
    if(program.get<bool>("--background-only")) {
        shell->set_background_only(true);
    }
    window.set_phase(shell, shell, shell);

    std::unique_ptr<dbus::dbus_server> server;
    try {
        server = std::make_unique<dbus::dbus_server>(&window, shell);
        server->run();
    } catch(const std::exception& e) {
        spdlog::error("Failed to start D-Bus server: {}", e.what());
    } catch(...) {
        spdlog::error("Failed to start D-Bus server: unknown exception");
    }

    window.loop();

    loop->quit();
#if __cpp_lib_jthread >= 201911L
#else
    main_loop_thread.join();
#endif

    return 0;
}
