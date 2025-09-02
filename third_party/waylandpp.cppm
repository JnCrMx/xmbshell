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
module;

#include <wayland-server.hpp>
#include <wayland-server-protocol.hpp>
#include <wayland-server-protocol-extra.hpp>

export module waylandpp;

export namespace wayland::server {
    using wayland::server::buffer_t;
    using wayland::server::callback_t;
    using wayland::server::client_t;
    using wayland::server::compositor_t;
    using wayland::server::data_device_manager_t;
    using wayland::server::data_device_t;
    using wayland::server::data_offer_t;
    using wayland::server::data_source_t;
    using wayland::server::display_t;
    using wayland::server::event_loop_t;
    using wayland::server::global_compositor_t;
    using wayland::server::global_data_device_manager_t;
    using wayland::server::global_output_t;
    using wayland::server::global_shm_t;
    using wayland::server::global_viewporter_t;
    using wayland::server::global_xdg_wm_base_t;
    using wayland::server::output_mode;
    using wayland::server::output_subpixel;
    using wayland::server::output_t;
    using wayland::server::output_transform;
    using wayland::server::region_t;
    using wayland::server::resource_t;
    using wayland::server::shm_format;
    using wayland::server::shm_pool_t;
    using wayland::server::shm_t;
    using wayland::server::surface_t;
    using wayland::server::viewport_t;
    using wayland::server::viewporter_t;
    using wayland::server::xdg_surface_t;
    using wayland::server::xdg_toplevel_t;
    using wayland::server::xdg_wm_base_t;
}
