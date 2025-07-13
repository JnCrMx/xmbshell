#include <cstring>
#include <iostream>
#include <wayland-server-protocol.hpp>
#include <wayland-server-protocol-extra.hpp>
#include <format>

#include <sys/mman.h>

import waylandpp;

int main() {
    wayland::server::display_t server_display;
    wayland::server::global_compositor_t global_compositor{server_display};
    wayland::server::global_shm_t global_shm{server_display};
    wayland::server::global_data_device_manager_t global_data_device_manager{server_display};
    wayland::server::global_xdg_wm_base_t global_xdg_wm_base{server_display};
    server_display.add_socket("wayland-1");

    wl_display_init_shm(server_display.c_ptr());
    wl_display_add_shm_format(server_display.c_ptr(), static_cast<uint32_t>(wayland::server::shm_format::argb8888));

    wayland::server::global_output_t global_output{server_display};
    global_output.on_bind() = [](const wayland::server::client_t& client, wayland::server::output_t output) {
        std::cout << std::format("Output bound to client {}: {}\n", client.get_fd(), output.get_id());
        output.geometry(0, 0, 0, 0, wayland::server::output_subpixel::none, "Generic", "Generic Monitor", wayland::server::output_transform::normal);
        output.scale(1);
        output.mode(wayland::server::output_mode::current, 1920, 1080, 60);
        output.done();
    };

    global_compositor.on_bind() = [](const wayland::server::client_t& client, wayland::server::compositor_t compositor) {
        std::cout << std::format("Compositor bound to client {}: {}\n", client.get_fd(), compositor.get_id());
        compositor.on_create_region() = [](wayland::server::region_t region) {
            std::cout << std::format("Region created: {}\n", region.get_id());
        };
        compositor.on_create_surface() = [](wayland::server::surface_t surface) {
            std::cout << std::format("Surface created: {}\n", surface.get_id());
            surface.on_attach() = [](wayland::server::buffer_t buffer, int32_t x, int32_t y) {
                std::cout << std::format("Buffer attached to surface: {} at position ({}, {})\n", buffer.get_id(), x, y);
            };
            surface.on_commit() = [=]() {
                std::cout << "Surface committed\n";
                auto buffer = wl_shm_buffer_get(surface.c_ptr());
                std::cout << std::format("Buffer: {}\n", reinterpret_cast<void*>(buffer));
            };
            surface.on_damage_buffer() = [=](int32_t x, int32_t y, int32_t width, int32_t height) {
                std::cout << std::format("Surface damaged at ({}, {}) with size {}x{}\n", x, y, width, height);

                auto buffer = wl_shm_buffer_get(surface.c_ptr());
                std::cout << std::format("Buffer: {}\n", reinterpret_cast<void*>(buffer));
            };
        };
        compositor.on_destroy() = [=]() {
            std::cout << std::format("Compositor destroyed: {}\n", compositor.get_id());
        };
    };
    global_xdg_wm_base.on_bind() = [](const wayland::server::client_t& client, wayland::server::xdg_wm_base_t xdg_wm_base) {
        std::cout << std::format("XDG WM Base bound to client {}: {}\n", client.get_fd(), xdg_wm_base.get_id());
        xdg_wm_base.on_get_xdg_surface() = [](wayland::server::xdg_surface_t xdg_surface, wayland::server::surface_t surface) {
            std::cout << std::format("XDG surface created: {} for surface {}\n", xdg_surface.get_id(), surface.get_id());
            xdg_surface.on_destroy() = [=]() {
                std::cout << std::format("XDG surface destroyed: {}\n", xdg_surface.get_id());
            };
        };
        xdg_wm_base.on_destroy() = [=]() {
            std::cout << std::format("XDG WM Base destroyed: {}\n", xdg_wm_base.get_id());
        };
    };

    auto el = server_display.get_event_loop();
    while(true) {
        el.dispatch(1);
        server_display.flush_clients();
    }
}
