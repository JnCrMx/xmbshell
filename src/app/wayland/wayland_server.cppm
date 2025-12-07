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

#if __linux__
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <sys/mman.h>
#include <unistd.h>
#endif

module xmbshell.app:wayland_server;

#if __linux__

import dreamrender;
import glm;
import spdlog;
import vma;
import vulkan_hpp;
import waylandpp;
import xmbshell.render;
import :component;

namespace wl {

struct shm_pool {
    int fd;
    uint32_t size;
    void* ptr;

    shm_pool(int fd, uint32_t size)
        : fd(fd), size(size)
    {
        ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // NOLINT(cppcoreguidelines-prefer-member-initializer)
        if(ptr == MAP_FAILED) {
            throw std::runtime_error("Failed to mmap SHM pool");
        }
        close(fd);
    }
    shm_pool(const shm_pool&) = delete;
    shm_pool(shm_pool&&) = delete;

    shm_pool& operator=(const shm_pool&) = delete;
    shm_pool& operator=(shm_pool&&) = delete;

    ~shm_pool() {
        if(ptr != MAP_FAILED) {
            munmap(ptr, size);
        }
    }

    void resize(uint32_t new_size) {
        ptr = mremap(ptr, size, new_size, MREMAP_MAYMOVE); // NOLINT(cppcoreguidelines-pro-type-vararg)
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
struct viewport {
    double sx, sy, sw, sh; // source rectangle
    int32_t dw, dh; // destination size
};
struct surface {
    std::shared_ptr<shm_buffer> buffer;
    std::shared_ptr<dreamrender::texture> texture;
    bool texture_undefined = true; // indicates that the texture is in VK_IMAGE_LAYOUT_UNDEFINED rather than VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

    // TODO: prevent early destruction of descriptor set while it's still in use by the GPU
    std::shared_ptr<vk::UniqueDescriptorSet> descriptor_set;

    std::optional<wl::viewport> viewport;

    // pending state
    struct {
        std::shared_ptr<shm_buffer> buffer;
        std::vector<vk::Rect2D> damages;
        std::vector<wayland::server::callback_t> frame_callbacks;
        std::optional<wl::viewport> viewport;
    } pending;
};
struct surface_damages {
    wayland::server::surface_t surface;
    std::vector<vk::Rect2D> damages;
};
struct xdg_surface {
    wayland::server::surface_t surface;
};
struct xdg_toplevel {
    wayland::server::xdg_surface_t xdg_surface;

    std::string title;
};

}

namespace app {

export class wayland_server : public component, public action_receiver {
    struct SurfaceParams {
        glm::mat4 matrix;
        glm::mat4 texture_matrix;
        glm::vec4 color;
        bool is_opaque;
    };

public:
    wayland_server(const std::string socket_name = "xmbshell") {
        server_display.add_socket(socket_name);
    }

    void preload(dreamrender::resource_loader* loader, const std::vector<vk::RenderPass>& renderPasses, vk::SampleCountFlagBits sampleCount, vk::PipelineCache pipelineCache, app::xmbshell* xmb) override {
        device = loader->getDevice();
        allocator = loader->getAllocator();

        using dreamrender::debugName;
        {
            std::tie(upload_buffer, upload_buffer_allocation) = allocator.createBufferUnique(
                vk::BufferCreateInfo{{}, upload_buffer_size, vk::BufferUsageFlagBits::eTransferSrc},
                vma::AllocationCreateInfo{vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, vma::MemoryUsage::eCpuToGpu,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent}
            );
            debugName(device, upload_buffer.get(), "Wayland Server Upload Buffer");
        }
        {
            vk::SamplerCreateInfo sampler_info({}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
                0.0f, false, 0.0f, false, vk::CompareOp::eNever, 0.0f, 0.0f, vk::BorderColor::eFloatTransparentBlack, false);
            sampler = device.createSamplerUnique(sampler_info);
            debugName(device, sampler.get(), "Wayland Server Image Sampler");
        }
        {
            std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {
                vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment)
            };
            vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingInfo({});
            vk::DescriptorSetLayoutCreateInfo layout_info({}, bindings, &bindingInfo);
            descriptor_set_layout = device.createDescriptorSetLayoutUnique(layout_info);
            debugName(device, descriptor_set_layout.get(), "Wayland Server Descriptor Set Layout");
        }
        {
            std::array<vk::PushConstantRange, 1> push_constant_ranges = {
                vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(SurfaceParams)),
            };
            vk::PipelineLayoutCreateInfo layout_info({}, descriptor_set_layout.get(), push_constant_ranges);
            pipeline_layout = device.createPipelineLayoutUnique(layout_info);
            debugName(device, pipeline_layout.get(), "Wayland Server Pipeline Layout");
        }
        {
            vk::UniqueShaderModule vertexShader = render::shaders::wayland_surface::vert(device);
            vk::UniqueShaderModule fragmentShader = render::shaders::wayland_surface::frag(device);
            std::array<vk::PipelineShaderStageCreateInfo, 2> shaders = {
                vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShader.get(), "main"),
                vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShader.get(), "main")
            };

            vk::PipelineVertexInputStateCreateInfo vertex_input{};
            vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::eTriangleStrip);
            vk::PipelineTessellationStateCreateInfo tesselation({}, {});

            vk::Viewport v{};
            vk::Rect2D s{};
            vk::PipelineViewportStateCreateInfo viewport({}, v, s);

            vk::PipelineRasterizationStateCreateInfo rasterization({}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);
            vk::PipelineMultisampleStateCreateInfo multisample({}, sampleCount);
            vk::PipelineDepthStencilStateCreateInfo depthStencil({}, false, false);

            vk::PipelineColorBlendAttachmentState attachment(true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
                vk::BlendFactor::eOne, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
            vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, vk::LogicOp::eClear, attachment);

            std::array<vk::DynamicState, 2> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
            vk::PipelineDynamicStateCreateInfo dynamic({}, dynamicStates);

            vk::GraphicsPipelineCreateInfo info({},
                shaders, &vertex_input, &input_assembly, &tesselation, &viewport,
                &rasterization, &multisample, &depthStencil, &colorBlend, &dynamic,
                pipeline_layout.get(), renderPasses[0], 0, {}, {});
            pipelines = dreamrender::createPipelines(device, pipelineCache, info, renderPasses, "Wayland Server Pipeline");
        }
        {
            std::array<vk::DescriptorPoolSize, 1> sizes = {
                vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000)
            };
            vk::DescriptorPoolCreateInfo pool_info(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, sizes);
            descriptor_pool = device.createDescriptorPoolUnique(pool_info);
            debugName(device, descriptor_pool.get(), "Wayland Server Descriptor Pool");
        }
    }

    void init(app::xmbshell* xmb) override {
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
                            user_data.texture_undefined = true;

                            vk::DescriptorSetAllocateInfo alloc_info(descriptor_pool.get(), descriptor_set_layout.get());
                            user_data.descriptor_set = std::make_shared<vk::UniqueDescriptorSet>(std::move(device.allocateDescriptorSetsUnique(alloc_info).front()));

                            vk::DescriptorImageInfo image_info(sampler.get(), user_data.texture->imageView.get(), vk::ImageLayout::eShaderReadOnlyOptimal);
                            vk::WriteDescriptorSet write_descriptor_set(user_data.descriptor_set->get(), 0, 0, vk::DescriptorType::eCombinedImageSampler, image_info);
                            device.updateDescriptorSets(write_descriptor_set, {});
                        }
                    }
                    if(!user_data.pending.damages.empty()) {
                        damaged_surfaces.push_back(wl::surface_damages{
                            .surface = surface,
                            .damages = std::move(user_data.pending.damages)
                        });
                    }
                    std::ranges::copy(user_data.pending.frame_callbacks, std::back_inserter(frame_callbacks));
                    user_data.viewport = user_data.pending.viewport;

                    user_data.pending = {};
                    user_data.pending.viewport = user_data.viewport; // keep the current viewport as pending by default
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
                    if(x < 0 || y < 0) {
                        spdlog::warn("[Surface {}] damage called with negative coordinates ({}, {})", surface.get_id(), x, y);
                        width = std::max(0, width + x);
                        height = std::max(0, height + y);
                        x = std::max(0, x);
                        y = std::max(0, y);
                    }
                    auto& user_data = surface.user_data().get<wl::surface>();
                    user_data.pending.damages.emplace_back(vk::Offset2D{x, y}, vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
                };
                surface.on_damage_buffer() = [this, surface](int32_t x, int32_t y, int32_t width, int32_t height) mutable {
                    spdlog::trace("[Surface {}] damaged buffer at ({}, {}) with size {}x{}", surface.get_id(), x, y, width, height);
                    if(x < 0 || y < 0) {
                        spdlog::warn("[Surface {}] damage_buffer called with negative coordinates ({}, {})", surface.get_id(), x, y);
                        width = std::max(0, width + x);
                        height = std::max(0, height + y);
                        x = std::max(0, x);
                        y = std::max(0, y);
                    }
                    auto& user_data = surface.user_data().get<wl::surface>();
                    user_data.pending.damages.emplace_back(vk::Offset2D{x, y}, vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
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
        global_viewporter.on_bind() = [](const wayland::server::client_t& client, wayland::server::viewporter_t viewporter) {
            spdlog::trace("[Client {}] viewporter bound: {}", client.get_fd(), viewporter.get_id());

            viewporter.on_get_viewport() = [viewporter](wayland::server::viewport_t viewport, wayland::server::surface_t surface) {
                spdlog::trace("[Viewporter {}] viewport created: {} for surface {}", viewporter.get_id(), viewport.get_id(), surface.get_id());

                auto& surface_data = surface.user_data().get<wl::surface>();
                surface_data.pending.viewport = wl::viewport{
                    .sx=0, .sy=0,
                    .sw=static_cast<double>(surface_data.buffer ? surface_data.buffer->width : 0),
                    .sh=static_cast<double>(surface_data.buffer ? surface_data.buffer->height : 0),
                    .dw=surface_data.buffer ? surface_data.buffer->width : 0,
                    .dh=surface_data.buffer ? surface_data.buffer->height : 0
                };

                viewport.on_set_source() = [viewport, surface](double x, double y, double width, double height) mutable {
                    spdlog::trace("[Viewport {}] source set to ({}, {}) with size {}x{}", viewport.get_id(), x, y, width, height);
                    if(width <= 0 || height <= 0) {
                        viewport.post_bad_value("source width and height must be positive");
                    }
                    if(x < 0 || y < 0 /*check width and height*/) {
                        viewport.post_out_of_buffer("source x and y must be non-negative");
                    }

                    auto& surface_data = surface.user_data().get<wl::surface>();
                    surface_data.pending.viewport->sx = x;
                    surface_data.pending.viewport->sy = y;
                    surface_data.pending.viewport->sw = width;
                    surface_data.pending.viewport->sh = height;
                };
                viewport.on_set_destination() = [viewport, surface](int width, int height) mutable {
                    spdlog::trace("[Viewport {}] destination set to size {}x{}", viewport.get_id(), width, height);
                    if(width <= 0 || height <= 0) {
                        viewport.post_bad_value("destination width and height must be positive");
                    }

                    auto& surface_data = surface.user_data().get<wl::surface>();
                    surface_data.pending.viewport->dw = width;
                    surface_data.pending.viewport->dh = height;
                };
                viewport.on_destroy() = [viewport, surface]() mutable {
                    spdlog::trace("[Viewport {}] destroyed", viewport.get_id());

                    auto& surface_data = surface.user_data().get<wl::surface>();
                    surface_data.pending.viewport.reset();
                };
            };
            viewporter.on_destroy() = [viewporter]() {
                spdlog::trace("[Viewporter {}] destroyed", viewporter.get_id());
            };
        };
        global_seat.on_bind() = [](const wayland::server::client_t& client, wayland::server::seat_t seat) {
            spdlog::trace("[Client {}] seat bound: {}", client.get_fd(), seat.get_id());
            seat.name("default");
            seat.capabilities(wayland::server::seat_capability::pointer | wayland::server::seat_capability::keyboard);

            seat.on_get_pointer() = [seat](wayland::server::pointer_t pointer) {
                spdlog::trace("[Seat {}] pointer created: {}", seat.get_id(), pointer.get_id());
                pointer.on_destroy() = [pointer]() {
                    spdlog::trace("[Pointer {}] destroyed", pointer.get_id());
                };
            };
            seat.on_get_keyboard() = [seat](wayland::server::keyboard_t keyboard) {
                spdlog::trace("[Seat {}] keyboard created: {}", seat.get_id(), keyboard.get_id());
                keyboard.on_destroy() = [keyboard]() {
                    spdlog::trace("[Keyboard {}] destroyed", keyboard.get_id());
                };
            };
            seat.on_destroy() = [seat]() {
                spdlog::trace("[Seat {}] destroyed", seat.get_id());
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
        global_xwayland_shell_v1.on_bind() = [](const wayland::server::client_t& client, wayland::server::xwayland_shell_v1_t xwayland_shell) {
            spdlog::trace("[Client {}] xwayland_shell_v1 bound: {}", client.get_fd(), xwayland_shell.get_id());
            xwayland_shell.on_get_xwayland_surface() = [=](wayland::server::xwayland_surface_v1_t xwayland_surface, wayland::server::surface_t surface) {
                spdlog::trace("[XWayland Shell V1 {}] xwayland_surface created: {} for surface {}", xwayland_shell.get_id(), xwayland_surface.get_id(), surface.get_id());
                xwayland_surface.user_data() = surface;

                xwayland_surface.on_set_serial() = [xwayland_surface](uint32_t serial_lo, uint32_t serial_hi) mutable {
                    spdlog::trace("[XWayland Surface V1 {}] serial set to: {:x}:{:x}", xwayland_surface.get_id(), serial_lo, serial_hi);
                };

                xwayland_surface.on_destroy() = [=]() {
                    spdlog::trace("[XWayland Surface V1 {}] destroyed", xwayland_surface.get_id());
                };
            };
            xwayland_shell.on_destroy() = [=]() {
                spdlog::trace("[XWayland Shell V1 {}] destroyed", xwayland_shell.get_id());
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
                        [fd](const wl::surface_damages& e) {
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

        // TODO: batch all required image transitions into a single pipeline barrier
        vk::DeviceSize upload_buffer_offset = 0;
        int i{};
        for(i=0; i<damaged_surfaces.size(); ++i) {
            auto& damaged_surface = damaged_surfaces[i];
            auto& surface = damaged_surface.surface;

            auto& user_data = surface.user_data().get<wl::surface>();
            if(!user_data.buffer || !user_data.texture) {
                continue;
            }

            vk::ImageLayout current_layout = user_data.texture_undefined ? vk::ImageLayout::eUndefined : vk::ImageLayout::eShaderReadOnlyOptimal;
            cmd.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
                {}, {}, {}, vk::ImageMemoryBarrier{
                    {}, vk::AccessFlagBits::eTransferWrite,
                    current_layout, vk::ImageLayout::eTransferDstOptimal,
                    vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                    user_data.texture->image, vk::ImageSubresourceRange{
                        vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
                    }
                });
            user_data.texture_undefined = false;

            int j{};
            for(j=0; j<damaged_surface.damages.size(); j++) {
                auto& damage = damaged_surface.damages[j];
                size_t buffer_size = user_data.buffer->stride * (damage.extent.height-1) + damage.extent.width * 4;
                size_t buffer_offset = user_data.buffer->offset + damage.offset.y * user_data.buffer->stride + damage.offset.x * 4;

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
                region.imageExtent.width = damage.extent.width;
                region.imageExtent.height = damage.extent.height;
                region.imageExtent.depth = 1;
                region.imageOffset.x = damage.offset.x;
                region.imageOffset.y = damage.offset.y;
                region.imageOffset.z = 0;
                region.bufferOffset = upload_buffer_offset;
                region.bufferRowLength = user_data.buffer->stride / 4;

                cmd.copyBufferToImage(upload_buffer.get(), user_data.texture->image,
                    vk::ImageLayout::eTransferDstOptimal, region);

                allocator.copyMemoryToAllocation(
                    static_cast<char*>(user_data.buffer->pool->ptr) + buffer_offset,
                    upload_buffer_allocation.get(),
                    upload_buffer_offset,
                    buffer_size
                );
                upload_buffer_offset += buffer_size;
            }
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

            damaged_surface.damages.erase(damaged_surface.damages.begin(), damaged_surface.damages.begin() + j);
            if(!damaged_surface.damages.empty()) {
                spdlog::warn("Not all damages were processed for surface {}: {} remaining", surface.get_id(), damaged_surface.damages.size());
                break;
            }
        };
        // erase only the surfaces that we managed to fit into the upload buffer
        damaged_surfaces.erase(damaged_surfaces.begin(), damaged_surfaces.begin() + i);
        if(!damaged_surfaces.empty()) {
            spdlog::warn("Not all damaged surfaces were processed, {} remaining", damaged_surfaces.size());
        }
    }
    void prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews, app::xmbshell* xmb) override {
        used_resources.resize(swapchainImages.size());
    };
    void render(dreamrender::gui_renderer& renderer, class xmbshell* xmb) override {
        auto& resources = used_resources[renderer.get_frame()];
        resources.clear();

        if(toplevels.empty()) {
            return;
        }

        vk::CommandBuffer cmd = renderer.get_command_buffer();
        vk::RenderPass render_pass = renderer.get_render_pass();

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines[render_pass].get());

        for(auto& xdg_toplevel : toplevels) {
            auto& xdg_toplevel_data = xdg_toplevel.user_data().get<wl::xdg_toplevel>();

            auto& xdg_surface_data = xdg_toplevel_data.xdg_surface.user_data().get<wl::xdg_surface>();
            auto& surface_data = xdg_surface_data.surface.user_data().get<wl::surface>();

            if(!surface_data.buffer || !surface_data.texture || !surface_data.descriptor_set) {
                continue;
            }

            double x = 0;
            double y = 0;
            int src_width = surface_data.texture->width;
            int src_height = surface_data.texture->height;
            int dst_width = surface_data.viewport ? surface_data.viewport->dw : src_width;
            int dst_height = surface_data.viewport ? surface_data.viewport->dh : src_height;

            double scaleX = static_cast<double>(dst_width) / renderer.frame_size.width;
            double scaleY = static_cast<double>(dst_height) / renderer.frame_size.height;

            glm::vec2 pos = glm::vec2(x, y)*2.0f - glm::vec2(1.0f);

            SurfaceParams params{};
            params.matrix = glm::mat4(1.0f);
            params.matrix = glm::translate(params.matrix, glm::vec3(pos, 0.0f));
            params.matrix = glm::scale(params.matrix, glm::vec3(scaleX, scaleY, 1.0f));
            params.texture_matrix = glm::mat4(1.0f);
            params.color = glm::vec4(1.0f);
            params.is_opaque = surface_data.buffer->format == wayland::server::shm_format::xrgb8888;

            cmd.pushConstants(pipeline_layout.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(SurfaceParams), &params);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout.get(), 0, 1, &surface_data.descriptor_set->get(), 0, nullptr);
            cmd.draw(4, 1, 0, 0);

            resources.push_back(surface_data.buffer);
            resources.push_back(surface_data.texture);
            resources.push_back(surface_data.descriptor_set);
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

    vk::UniqueSampler sampler;
    vk::UniqueDescriptorSetLayout descriptor_set_layout;
    vk::UniqueDescriptorPool descriptor_pool;
    vk::UniquePipelineLayout pipeline_layout;
    dreamrender::UniquePipelineMap pipelines;

    std::vector<std::vector<std::shared_ptr<void>>> used_resources;

    wayland::server::display_t server_display;
    wayland::server::global_compositor_t global_compositor{server_display};
    wayland::server::global_viewporter_t global_viewporter{server_display};
    wayland::server::global_data_device_manager_t global_data_device_manager{server_display};
    wayland::server::global_output_t global_output{server_display};
    wayland::server::global_shm_t global_shm{server_display};
    wayland::server::global_seat_t global_seat{server_display};
    wayland::server::global_xdg_wm_base_t global_xdg_wm_base{server_display};
    wayland::server::global_xwayland_shell_v1_t global_xwayland_shell_v1{server_display};

    wayland::server::event_loop_t event_loop = server_display.get_event_loop();
    std::vector<wl::surface_damages> damaged_surfaces;
    std::vector<wayland::server::xdg_toplevel_t> toplevels;

    std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds> start_time =
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
    std::vector<wayland::server::callback_t> frame_callbacks;
};

}
#endif // __linux__
