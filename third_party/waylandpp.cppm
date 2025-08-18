module;

#include <wayland-server.hpp>
#include <wayland-server-protocol.hpp>
#include <wayland-server-protocol-extra.hpp>

export module waylandpp;

export namespace wayland::server {
    using wayland::server::buffer_t;
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
    using wayland::server::global_xdg_wm_base_t;
    using wayland::server::output_mode;
    using wayland::server::output_subpixel;
    using wayland::server::output_t;
    using wayland::server::output_transform;
    using wayland::server::shm_format;
    using wayland::server::shm_pool_t;
    using wayland::server::shm_t;
    using wayland::server::surface_t;
    using wayland::server::xdg_surface_t;
    using wayland::server::xdg_toplevel_t;
    using wayland::server::xdg_wm_base_t;
    using wayland::server::region_t;
    using wayland::server::callback_t;
    using wayland::server::resource_t;
}
