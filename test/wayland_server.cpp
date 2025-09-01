/* XMBShell, a console-like desktop shell
 * Copyright (C) 2025 - JCM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <wayland-server-protocol-extra.hpp>
#include <wayland-server-protocol.hpp>

#include <sys/mman.h>

import waylandpp;

struct shm_pool {
    int fd;
    uint32_t size;
    void* ptr;

    shm_pool(int fd, uint32_t size)
        : fd(fd), size(size)
    {
        ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if(ptr == MAP_FAILED) {
            throw std::runtime_error("Failed to mmap SHM pool");
        }
    }

    ~shm_pool() {
        if(ptr != MAP_FAILED) {
            munmap(ptr, size);
        }
        close(fd);
    }
};
struct shm_buffer {
    std::shared_ptr<shm_pool> pool;
    uint32_t offset;
    int32_t width;
    int32_t height;
    int32_t stride;
    wayland::server::shm_format format;

    shm_buffer(std::shared_ptr<shm_pool> pool, uint32_t offset, int32_t width, int32_t height, int32_t stride, wayland::server::shm_format format)
        : pool(pool), offset(offset), width(width), height(height), stride(stride), format(format)
    {
        if(!pool) {
            throw std::runtime_error("SHM pool is null");
        }
        if(offset + (height * stride) > pool->size) {
            throw std::runtime_error("Buffer exceeds SHM pool size");
        }
    }

    void dump(const std::filesystem::path& file) {
        void* data = static_cast<char*>(pool->ptr) + offset;

        std::ofstream os(file);
        os << "P6\n" << width << " " << height << "\n255\n";
        for(int32_t j = 0; j < height; ++j) {
            for(int32_t i = 0; i < width; ++i) {
                uint32_t pixel = *reinterpret_cast<uint32_t*>(static_cast<char*>(data) + j * stride + i * 4);
                os.put((pixel >> 16) & 0xFF); // R
                os.put((pixel >> 8) & 0xFF);  // G
                os.put(pixel & 0xFF);         // B
            }
        }
    }
};

int main() {
    wayland::server::display_t server_display;
    wayland::server::global_compositor_t global_compositor{server_display};
    wayland::server::global_shm_t global_shm{server_display};
    wayland::server::global_data_device_manager_t global_data_device_manager{server_display};
    wayland::server::global_xdg_wm_base_t global_xdg_wm_base{server_display};
    server_display.add_socket("wayland-1");

    global_shm.on_bind() = [](const wayland::server::client_t& client, wayland::server::shm_t shm) {
        std::cout << std::format("[Client {}] SHM bound: {}\n", client.get_fd(), shm.get_id());

        shm.format(wayland::server::shm_format::argb8888);
        shm.format(wayland::server::shm_format::xrgb8888);

        shm.on_create_pool() = [=](wayland::server::shm_pool_t pool, int32_t fd, uint32_t size) mutable {
            std::cout << std::format("[SHM {}] SHM pool created: {} with fd {} and size {}\n", shm.get_id(), pool.get_id(), fd, size);

            pool.user_data() = std::make_shared<shm_pool>(fd, size);

            pool.on_create_buffer() = [=](wayland::server::buffer_t buffer, uint32_t offset, int32_t width, int32_t height, int32_t stride, wayland::server::shm_format format) mutable {
                std::cout << std::format("[SHM Pool {}] buffer created: {} with offset {}, size {}x{}, stride {}, format {}\n", pool.get_id(), buffer.get_id(), offset, width, height, stride, static_cast<int>(format));
                buffer.on_destroy() = [=]() {
                    std::cout << std::format("Buffer destroyed: {}\n", buffer.get_id());
                };
                buffer.user_data() = shm_buffer(pool.user_data().get<std::shared_ptr<shm_pool>>(), offset, width, height, stride, format);
            };
            pool.on_destroy() = [=]() {
                std::cout << std::format("[SHM Pool {}] destroyed\n", pool.get_id());
            };
            pool.on_resize() = [=](uint32_t new_size) {
                std::cout << std::format("[SHM Pool {}] resized to {}\n", pool.get_id(), new_size);
            };
        };
    };

    wayland::server::output_t the_output;

    wayland::server::global_output_t global_output{server_display};
    global_output.on_bind() = [&the_output](const wayland::server::client_t& client, wayland::server::output_t output) {
        std::cout << std::format("[Client {}] Output bound: {}\n", client.get_fd(), output.get_id());
        output.geometry(0, 0, 0, 0, wayland::server::output_subpixel::none, "Generic", "Generic Monitor", wayland::server::output_transform::normal);
        output.scale(1);
        output.mode(wayland::server::output_mode::current, 1920, 1080, 60);
        output.done();

        the_output = output;
    };

    std::vector<wayland::server::callback_t> frame_callbacks;

    global_compositor.on_bind() = [&the_output, &frame_callbacks](const wayland::server::client_t& client, wayland::server::compositor_t compositor) {
        std::cout << std::format("[Client {}] Compositor bound: {}\n", client.get_fd(), compositor.get_id());
        compositor.on_create_region() = [=](wayland::server::region_t region) {
            std::cout << std::format("[Compositor {}] region created: {}\n", compositor.get_id(), region.get_id());
        };
        compositor.on_create_surface() = [=, &the_output, &frame_callbacks](wayland::server::surface_t surface) mutable {
            std::cout << std::format("[Compositor {}] surface created: {}\n", compositor.get_id(), surface.get_id());
            surface.on_attach() = [=](wayland::server::buffer_t buffer, int32_t x, int32_t y) mutable {
                std::cout << std::format("[Surface {}] buffer attached: {} at offset ({}, {})\n", surface.get_id(), buffer.get_id(), x, y);
                surface.user_data() = buffer.user_data().get<shm_buffer>();
            };
            surface.on_commit() = [=, &the_output]() mutable {
                std::cout << std::format("[Surface {}] committed\n", surface.get_id());
                //surface.enter(the_output);
            };
            surface.on_damage_buffer() = [=](int32_t x, int32_t y, int32_t width, int32_t height) mutable {
                std::cout << std::format("[Surface {}] buffer damaged at ({}, {}) with size {}x{}\n", surface.get_id(), x, y, width, height);
                auto& buffer = surface.user_data().get<shm_buffer>();
                buffer.dump(std::format("surface_{}.ppm", surface.get_id()));
            };
            surface.on_damage() = [=](int32_t x, int32_t y, int32_t width, int32_t height) mutable {
                std::cout << std::format("[Surface {}] damage set to ({}, {}) with size {}x{}\n", surface.get_id(), x, y, width, height);
                auto& buffer = surface.user_data().get<shm_buffer>();
                buffer.dump(std::format("surface_{}.ppm", surface.get_id()));
            };
            surface.on_destroy() = [=]() {
                std::cout << std::format("[Surface {}] destroyed\n", surface.get_id());
            };
            surface.on_frame() = [=, &frame_callbacks](wayland::server::callback_t frame_callback) {
                std::cout << std::format("[Surface {}] frame callback set: {}\n", surface.get_id(), frame_callback.get_id());
                frame_callback.on_destroy() = [=]() {
                    std::cout << std::format("[Frame callback {}] destroyed\n", frame_callback.get_id());
                };
                frame_callbacks.push_back(frame_callback);
            };
            surface.on_offset() = [=](int32_t x, int32_t y) {
                std::cout << std::format("[Surface {}] offset set to ({}, {})\n", surface.get_id(), x, y);
            };
        };
        compositor.on_destroy() = [=]() {
            std::cout << std::format("[Compositor {}] destroyed\n", compositor.get_id());
        };
    };
    global_xdg_wm_base.on_bind() = [](const wayland::server::client_t& client, wayland::server::xdg_wm_base_t xdg_wm_base) {
        std::cout << std::format("[Client {}] XDG WM Base bound: {}\n", client.get_fd(), xdg_wm_base.get_id());
        xdg_wm_base.on_get_xdg_surface() = [=](wayland::server::xdg_surface_t xdg_surface, wayland::server::surface_t surface) {
            std::cout << std::format("[XDG WM Base {}] XDG surface created: {} for surface {}\n", xdg_wm_base.get_id(), xdg_surface.get_id(), surface.get_id());
            xdg_surface.on_destroy() = [=]() {
                std::cout << std::format("[XDG surface {}] destroyed\n", xdg_surface.get_id());
            };
            xdg_surface.on_ack_configure() = [=](uint32_t serial) {
                std::cout << std::format("[XDG surface {}] configure acknowledged with serial {}\n", xdg_surface.get_id(), serial);
            };
            xdg_surface.on_get_popup() = [=](wayland::server::xdg_popup_t xdg_popup, wayland::server::surface_t surface, wayland::server::xdg_positioner_t positioner) {
                std::cout << std::format("[XDG surface {}] Popup created: {} for surface {}\n", xdg_surface.get_id(), xdg_popup.get_id(), surface.get_id());
                xdg_popup.on_destroy() = [=]() {
                    std::cout << std::format("[XDG popup {}] destroyed\n", xdg_popup.get_id());
                };
            };
            xdg_surface.on_set_window_geometry() = [=](int32_t x, int32_t y, int32_t width, int32_t height) mutable {
                std::cout << std::format("[XDG surface {}] window geometry set to ({}, {}) with size {}x{}\n", xdg_surface.get_id(), x, y, width, height);
            };
            xdg_surface.on_get_toplevel() = [=](wayland::server::xdg_toplevel_t xdg_toplevel) {
                std::cout << std::format("[XDG surface {}] toplevel created: {}\n", xdg_surface.get_id(), xdg_toplevel.get_id());
                xdg_toplevel.on_set_title() = [=](const std::string& title) {
                    std::cout << std::format("[XDG toplevel {}] title set to '{}'\n", xdg_toplevel.get_id(), title);
                };
                xdg_toplevel.on_set_app_id() = [=](const std::string& app_id) {
                    std::cout << std::format("[XDG toplevel {}] app ID set to '{}'\n", xdg_toplevel.get_id(), app_id);
                };
                xdg_toplevel.on_set_maximized() = [=]() {
                    std::cout << std::format("[XDG toplevel {}] maximized\n", xdg_toplevel.get_id());
                };
                xdg_toplevel.on_unset_maximized() = [=]() {
                    std::cout << std::format("[XDG toplevel {}] unset maximized\n", xdg_toplevel.get_id());
                };
                xdg_toplevel.on_set_fullscreen() = [=](wayland::server::output_t output) {
                    std::cout << std::format("[XDG toplevel {}] fullscreen set on output {}\n", xdg_toplevel.get_id(), output.get_id());
                };
                xdg_toplevel.on_unset_fullscreen() = [=]() {
                    std::cout << std::format("[XDG toplevel {}] unset fullscreen\n", xdg_toplevel.get_id());
                };
                xdg_toplevel.on_set_minimized() = [=]() {
                    std::cout << std::format("[XDG toplevel {}] minimized\n", xdg_toplevel.get_id());
                };
                // xdg_toplevel.on_set_parent() = [=](const wayland::server::xdg_toplevel_t& parent) {
                //     std::cout << std::format("XDG toplevel parent set to {}\n", parent.proxy_has_object() ? parent.get_id() : 0);
                // };
                xdg_toplevel.on_set_max_size() = [=](int32_t width, int32_t height) mutable {
                    std::cout << std::format("[XDG toplevel {}] max size set to {}x{}\n", xdg_toplevel.get_id(), width, height);
                xdg_toplevel.configure(1920, 1080, std::vector{wayland::server::xdg_toplevel_state::fullscreen, wayland::server::xdg_toplevel_state::activated});
                };
                xdg_toplevel.on_set_min_size() = [=](int32_t width, int32_t height) mutable {
                    std::cout << std::format("[XDG toplevel {}] min size set to {}x{}\n", xdg_toplevel.get_id(), width, height);
                xdg_toplevel.configure(1920, 1080, std::vector{wayland::server::xdg_toplevel_state::fullscreen, wayland::server::xdg_toplevel_state::activated});
                };
                xdg_toplevel.on_show_window_menu() = [=](wayland::server::data_device_t data_device, int32_t x, int32_t y, uint32_t serial) {
                    std::cout << std::format("[XDG toplevel {}] show window menu requested at ({}, {}) with serial {}\n", xdg_toplevel.get_id(), x, y, serial);
                };
                xdg_toplevel.on_move() = [=](wayland::server::data_device_t data_device, uint32_t serial) {
                    std::cout << std::format("[XDG toplevel {}] move requested with serial {}\n", xdg_toplevel.get_id(), serial);
                };
                xdg_toplevel.on_resize() = [=](wayland::server::data_device_t data_device, uint32_t serial, wayland::server::xdg_toplevel_resize_edge edge) {
                    std::cout << std::format("[XDG toplevel {}] resize requested with serial {} and edge {}\n", xdg_toplevel.get_id(), serial, static_cast<int>(edge));
                };
                xdg_toplevel.configure(1920, 1080, std::vector{wayland::server::xdg_toplevel_state::fullscreen, wayland::server::xdg_toplevel_state::activated});
            };
            xdg_surface.configure(0);
        };
        xdg_wm_base.on_destroy() = [=]() {
            std::cout << std::format("[XDG WM Base {}] destroyed\n", xdg_wm_base.get_id());
        };
    };

    auto el = server_display.get_event_loop();
    while(true) {
        for(auto& cb : frame_callbacks) {
            std::cout << std::format("Processing frame callback: {}\n", cb.get_id());

            uint32_t time_in_millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            cb.done(time_in_millis);
        }
        frame_callbacks.clear();
        server_display.flush_clients();

        el.dispatch(1);
        server_display.flush_clients();
    }
}
