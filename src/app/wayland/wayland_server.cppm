module;

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <sys/mman.h>
#include <unistd.h>

module xmbshell.app:wayland_server;

import dreamrender;
import spdlog;
import vma;
import vulkan_hpp;
import waylandpp;
import :component;

namespace wl {

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
        close(fd);
    }

    ~shm_pool() {
        if(ptr != MAP_FAILED) {
            munmap(ptr, size);
        }
    }

    void resize(uint32_t new_size) {
        ptr = mremap(ptr, size, new_size, MREMAP_MAYMOVE);
        if(ptr == MAP_FAILED) {
            throw std::runtime_error("Failed to remap SHM pool");
        }
        size = new_size;
    }
};
struct shm_buffer {
    wayland::server::buffer_t wl_object;
    std::shared_ptr<shm_pool> pool;
    uint32_t offset;
    int32_t width;
    int32_t height;
    int32_t stride;
    wayland::server::shm_format format;

    shm_buffer(wayland::server::buffer_t wl_object, std::shared_ptr<shm_pool> pool, uint32_t offset, int32_t width, int32_t height, int32_t stride, wayland::server::shm_format format)
        : wl_object(std::move(wl_object)), pool(pool), offset(offset), width(width), height(height), stride(stride), format(format)
    {
        if(!pool) {
            throw std::runtime_error("SHM pool is null");
        }
        if(offset + (height * stride) > pool->size) {
            throw std::runtime_error("Buffer exceeds SHM pool size");
        }
    }
};
struct surface {
    std::shared_ptr<shm_buffer> buffer;
    std::shared_ptr<dreamrender::texture> texture;

    // pending state
    struct {
        std::shared_ptr<shm_buffer> buffer;
        std::vector<vk::Rect2D> damages;
        std::vector<wayland::server::callback_t> frame_callbacks;
    } pending;
};
struct xdg_surface {
    wayland::server::surface_t surface;
};
struct xdg_toplevel {
    wayland::server::xdg_surface_t xdg_surface;

    std::string title;
};
struct buffer_damage {
    wayland::server::surface_t surface;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

}

namespace app {

export class wayland_server : public component {
public:
    wayland_server(vk::Device d, vma::Allocator a, const std::string socket_name = "xmbshell") : device(d), allocator(a) {
        std::tie(upload_buffer, upload_buffer_allocation) = allocator.createBufferUnique(
            vk::BufferCreateInfo{{}, upload_buffer_size, vk::BufferUsageFlagBits::eTransferSrc},
            vma::AllocationCreateInfo{vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, vma::MemoryUsage::eCpuToGpu,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent}
        );

        server_display.add_socket(socket_name);

        global_shm.on_bind() = [](const wayland::server::client_t& client, wayland::server::shm_t shm) {
            spdlog::trace("[Client {}] SHM bound: {}", client.get_fd(), shm.get_id());
            shm.format(wayland::server::shm_format::argb8888);
            shm.format(wayland::server::shm_format::xrgb8888);

            shm.on_create_pool() = [=](wayland::server::shm_pool_t pool, int32_t fd, uint32_t size) mutable {
                spdlog::trace("[SHM {}] SHM pool created: SHM pool created: {} with fd {} and size {}", client.get_fd(), shm.get_id(), pool.get_id(), fd, size);
                pool.user_data() = std::make_shared<wl::shm_pool>(fd, size);

                pool.on_create_buffer() = [=](wayland::server::buffer_t buffer, uint32_t offset, int32_t width, int32_t height, int32_t stride, wayland::server::shm_format format) mutable {
                    spdlog::trace("[SHM Pool {}] buffer created: {} with offset {}, size {}x{}, stride {}, format {}", pool.get_id(), buffer.get_id(), offset, width, height, stride, static_cast<int>(format));
                    buffer.user_data() = std::make_shared<wl::shm_buffer>(buffer,
                        pool.user_data().get<std::shared_ptr<wl::shm_pool>>(), offset, width, height, stride, format);

                    buffer.on_destroy() = [=]() {
                        spdlog::trace("[SHM Buffer {}] destroyed", buffer.get_id());
                    };
                };
                pool.on_resize() = [=](uint32_t new_size) mutable {
                    spdlog::trace("[SHM Pool {}] resized to {}", pool.get_id(), new_size);
                    pool.user_data().get<std::shared_ptr<wl::shm_pool>>()->resize(new_size);
                };
                pool.on_destroy() = [=]() {
                    spdlog::trace("[SHM Pool {}] destroyed", pool.get_id());
                };
            };
            shm.on_destroy() = [=]() {
                spdlog::trace("[SHM {}] destroyed", shm.get_id());
            };
        };
        global_output.on_bind() = [](const wayland::server::client_t& client, wayland::server::output_t output) {
            spdlog::trace("[Client {}] output bound: {}", client.get_fd(), output.get_id());
            output.geometry(0, 0, 0, 0, wayland::server::output_subpixel::none, "Generic", "Generic Monitor", wayland::server::output_transform::normal);
            output.scale(1);
            output.mode(wayland::server::output_mode::current, 1920, 1080, 60);
            output.done();
        };
        global_compositor.on_bind() = [this](const wayland::server::client_t& client, wayland::server::compositor_t compositor) {
            spdlog::trace("[Client {}] compositor bound: {}", client.get_fd(), compositor.get_id());
            compositor.on_create_region() = [=](wayland::server::region_t region) {
                spdlog::trace("[Compositor {}] region created: {}", compositor.get_id(), region.get_id());
                region.on_destroy() = [region]() {
                    spdlog::trace("[Region {}] destroyed", region.get_id());
                };
            };
            compositor.on_create_surface() = [this, compositor](wayland::server::surface_t surface) mutable {
                spdlog::trace("[Compositor {}] surface created: {}", compositor.get_id(), surface.get_id());
                surface.user_data() = wl::surface{};

                surface.on_commit() = [this, surface]() mutable {
                    spdlog::trace("[Surface {}] committed", surface.get_id());
                    auto& user_data = surface.user_data().get<wl::surface>();

                    if(user_data.pending.buffer && user_data.pending.buffer != user_data.buffer) {
                        if(user_data.buffer) {
                            user_data.buffer->wl_object.release();
                        }
                        user_data.buffer = user_data.pending.buffer;
                        if(!user_data.texture ||
                            user_data.texture->width != user_data.buffer->width ||
                            user_data.texture->height != user_data.buffer->height)
                        {
                            spdlog::debug("[Surface {}] creating new texture for buffer: {} with size {}x{}",
                                surface.get_id(), user_data.buffer->wl_object.get_id(),
                                user_data.buffer->width, user_data.buffer->height);
                            user_data.texture = std::make_shared<dreamrender::texture>(device, allocator,
                                user_data.buffer->width, user_data.buffer->height, vk::ImageUsageFlagBits::eSampled, vk::Format::eR8G8B8A8Unorm);
                        }
                    }

                    std::ranges::transform(user_data.pending.damages, std::back_inserter(damaged_surfaces),
                        [surface](const vk::Rect2D& damage) {
                            return wl::buffer_damage{
                                .surface=surface,
                                .x=damage.offset.x, .y=damage.offset.y,
                                .width=static_cast<int32_t>(damage.extent.width), .height=static_cast<int32_t>(damage.extent.height)
                            };
                        });
                    std::ranges::copy(user_data.pending.frame_callbacks, std::back_inserter(frame_callbacks));

                    user_data.pending = {};
                };
                surface.on_attach() = [this, surface](wayland::server::buffer_t buffer, int32_t x, int32_t y) mutable {
                    spdlog::trace("[Surface {}] attached buffer: {} at {}, {}", surface.get_id(), buffer.get_id(), x, y);
                    if(x != 0 || y != 0) {
                        surface.post_invalid_offset("attaching buffer with non-zero offset is not supported");
                        return;
                    }

                    auto& user_data = surface.user_data().get<wl::surface>();
                    user_data.pending.buffer = buffer.user_data().get<std::shared_ptr<wl::shm_buffer>>();
                };
                surface.on_damage() = [this, surface](int32_t x, int32_t y, int32_t width, int32_t height) mutable {
                    spdlog::trace("[Surface {}] damaged at ({}, {}) with size {}x{}", surface.get_id(), x, y, width, height);
                    auto& user_data = surface.user_data().get<wl::surface>();
                    user_data.pending.damages.push_back(vk::Rect2D{vk::Offset2D{x, y}, vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)}});
                };
                surface.on_damage_buffer() = [this, surface](int32_t x, int32_t y, int32_t width, int32_t height) mutable {
                    spdlog::trace("[Surface {}] damaged buffer at ({}, {}) with size {}x{}", surface.get_id(), x, y, width, height);
                    auto& user_data = surface.user_data().get<wl::surface>();
                    user_data.pending.damages.push_back(vk::Rect2D{vk::Offset2D{x, y}, vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)}});
                };
                surface.on_frame() = [this, surface](wayland::server::callback_t callback) mutable {
                    spdlog::trace("[Surface {}] frame callback set: {}", surface.get_id(), callback.get_id());
                    auto& user_data = surface.user_data().get<wl::surface>();
                    user_data.pending.frame_callbacks.push_back(callback);
                };
                surface.on_destroy() = [surface]() {
                    spdlog::trace("[Surface {}] destroyed", surface.get_id());
                };
            };
            compositor.on_destroy() = [compositor]() mutable {
                spdlog::trace("[Compositor {}] destroyed", compositor.get_id());
            };
        };
        global_xdg_wm_base.on_bind() = [this](const wayland::server::client_t& client, wayland::server::xdg_wm_base_t xdg_wm_base) {
            spdlog::trace("[Client {}] xdg_wm_base bound: {}", client.get_fd(), xdg_wm_base.get_id());
            xdg_wm_base.on_get_xdg_surface() = [this, xdg_wm_base](wayland::server::xdg_surface_t xdg_surface, wayland::server::surface_t surface) {
                spdlog::trace("[XDG WM Base {}] xdg_surface created: {} for surface {}", xdg_wm_base.get_id(), xdg_surface.get_id(), surface.get_id());
                xdg_surface.user_data() = wl::xdg_surface{surface};

                xdg_surface.on_get_toplevel() = [this, xdg_surface](wayland::server::xdg_toplevel_t xdg_toplevel) {
                    spdlog::trace("[XDG Surface {}] toplevel created: {}", xdg_surface.get_id(), xdg_toplevel.get_id());
                    xdg_toplevel.user_data() = wl::xdg_toplevel{.xdg_surface = xdg_surface};
                    toplevels.push_back(xdg_toplevel);

                    xdg_toplevel.on_set_title() = [xdg_toplevel](const std::string& title) mutable {
                        spdlog::trace("[XDG Toplevel {}] title set to: {}", xdg_toplevel.get_id(), title);
                        xdg_toplevel.user_data().get<wl::xdg_toplevel>().title = title;
                    };
                    xdg_toplevel.on_destroy() = [this, xdg_toplevel]() {
                        spdlog::trace("[XDG Toplevel {}] destroyed", xdg_toplevel.get_id());
                        auto it = std::ranges::remove_if(toplevels,
                            [&xdg_toplevel](const wayland::server::xdg_toplevel_t& e) {
                                return e.get_id() == xdg_toplevel.get_id();
                            });
                        toplevels.erase(it.begin(), it.end());
                    };
                };
                xdg_surface.on_destroy() = [=]() {
                    spdlog::trace("[XDG Surface {}] destroyed", xdg_surface.get_id());
                };
                xdg_surface.configure(server_display.next_serial());
            };
            xdg_wm_base.on_destroy() = [=]() {
                spdlog::trace("[XDG WM Base {}] destroyed", xdg_wm_base.get_id());
            };
        };
        server_display.on_client_created() = [this](wayland::server::client_t& client) {
            spdlog::debug("[New Client] Client connected: {}", client.get_fd());
            client.on_destroy() = [this, fd = client.get_fd()]() {
                if(server_display.get_client_list().empty()) {
                    spdlog::debug("[Client {}] disconnected during server destruction, skipping cleanup", fd);
                    return;
                }
                spdlog::debug("[Client {}] disconnected", fd);
                {
                    auto it = std::ranges::remove_if(damaged_surfaces,
                        [fd](const wl::buffer_damage& e) {
                            return !e.surface.proxy_has_object() || e.surface.get_client().get_fd() == fd;
                        });
                    damaged_surfaces.erase(it.begin(), it.end());
                }
                {
                    auto it = std::ranges::remove_if(toplevels,
                        [fd](const wayland::server::xdg_toplevel_t& toplevel) {
                            return !toplevel.proxy_has_object() || toplevel.get_client().get_fd() == fd;
                        });
                    toplevels.erase(it.begin(), it.end());
                }
            };
        };
        server_display.on_destroy() = [this]() {
            spdlog::debug("[Wayland Server] Display destroyed");
            damaged_surfaces.clear();
            toplevels.clear();
        };
    }

    void prerender(vk::CommandBuffer cmd, int frame, class xmbshell* xmb) override {
        if(damaged_surfaces.empty()) {
            return;
        }

        vk::DeviceSize upload_buffer_offset = 0;
        int i{};
        for(i=0; i<damaged_surfaces.size(); ++i) {
            auto& damage = damaged_surfaces[i];
            auto& surface = damage.surface;
            if(damage.x < 0 || damage.y < 0) {
                spdlog::warn("Invalid damage coordinates: ({}, {}) for surface {}", damage.x, damage.y, surface.get_id());
                continue;
            }
            auto& user_data = surface.user_data().get<wl::surface>();
            if(!user_data.buffer || !user_data.texture) {
                continue;
            }
            size_t buffer_size = user_data.buffer->stride * (damage.height-1) + damage.width * 4;
            size_t buffer_offset = user_data.buffer->offset + damage.y * user_data.buffer->stride + damage.x * 4;

            if(upload_buffer_offset + buffer_size > upload_buffer_size) {
                break;
            }
            if(buffer_offset + buffer_size > user_data.buffer->pool->size) {
                spdlog::warn("Damaged buffer size {} and total offset {} exceeds SHM pool size {} ({}x{} stride {})",
                    buffer_size, buffer_offset, user_data.buffer->pool->size,
                    user_data.buffer->width, user_data.buffer->height, user_data.buffer->stride);
                buffer_size = user_data.buffer->pool->size - buffer_offset;
            }

            // Update the texture with the buffer data
            vk::BufferImageCopy region{};
            region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = damage.width;
            region.imageExtent.height = damage.height;
            region.imageExtent.depth = 1;
            region.imageOffset.x = damage.x;
            region.imageOffset.y = damage.y;
            region.imageOffset.z = 0;
            region.bufferOffset = upload_buffer_offset;
            region.bufferRowLength = user_data.buffer->stride / 4;

            cmd.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
                {}, {}, {}, vk::ImageMemoryBarrier{
                    {}, vk::AccessFlagBits::eTransferWrite,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                    vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                    user_data.texture->image, vk::ImageSubresourceRange{
                        vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
                    }
                });
            cmd.copyBufferToImage(upload_buffer.get(), user_data.texture->image,
                vk::ImageLayout::eTransferDstOptimal, region);
            cmd.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                {}, {}, {}, vk::ImageMemoryBarrier{
                    vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                    vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                    vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                    user_data.texture->image, vk::ImageSubresourceRange{
                        vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
                    }
                });

            allocator.copyMemoryToAllocation(
                static_cast<char*>(user_data.buffer->pool->ptr) + buffer_offset,
                upload_buffer_allocation.get(),
                upload_buffer_offset,
                buffer_size
            );
            upload_buffer_offset += buffer_size;
        };
        // erase only the surfaces that we managed to fit into the upload buffer
        damaged_surfaces.erase(damaged_surfaces.begin(), damaged_surfaces.begin() + i);
    }
    void render(dreamrender::gui_renderer& renderer, class xmbshell* xmb) override {
        for(auto& xdg_toplevel : toplevels) {
            auto& xdg_toplevel_data = xdg_toplevel.user_data().get<wl::xdg_toplevel>();

            auto& xdg_surface_data = xdg_toplevel_data.xdg_surface.user_data().get<wl::xdg_surface>();
            auto& surface_data = xdg_surface_data.surface.user_data().get<wl::surface>();

            if(surface_data.texture) {
                renderer.draw_image_sized(*surface_data.texture->imageView, 0, 0, surface_data.texture->width, surface_data.texture->height);
            } else {
                spdlog::warn("No texture available for surface associated with toplevel: {}", xdg_toplevel.get_id());
            }
        }
    }
    result tick(app::xmbshell* xmb) override {
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds> current_time =
            std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
        for(auto& frame_callback : frame_callbacks) {
            frame_callback.done(static_cast<uint32_t>(elapsed_time));
        }
        frame_callbacks.clear();

        event_loop.dispatch(1);
        server_display.flush_clients();

        return result::success;
    }

    bool is_opaque() const override {
        return false;
    }
    bool is_transparent() const override {
        return true;
    }
private:
    vk::Device device;
    vma::Allocator allocator;

    constexpr static vk::DeviceSize upload_buffer_size = 16 * 1024 * 1024; // 16 MiB
    vma::UniqueAllocation upload_buffer_allocation;
    vma::UniqueBuffer upload_buffer;

    wayland::server::display_t server_display;
    wayland::server::global_compositor_t global_compositor{server_display};
    wayland::server::global_data_device_manager_t global_data_device_manager{server_display};
    wayland::server::global_output_t global_output{server_display};
    wayland::server::global_shm_t global_shm{server_display};
    wayland::server::global_xdg_wm_base_t global_xdg_wm_base{server_display};

    wayland::server::event_loop_t event_loop = server_display.get_event_loop();
    std::vector<wl::buffer_damage> damaged_surfaces;
    std::vector<wayland::server::xdg_toplevel_t> toplevels;

    std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds> start_time =
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
    std::vector<wayland::server::callback_t> frame_callbacks;
};

}
