module;

#include <wayland-server.hpp>
#include <wayland-server-protocol.hpp>

export module waylandpp;

export namespace wayland::server {
    using wayland::server::display_t;
    using wayland::server::client_t;
}
