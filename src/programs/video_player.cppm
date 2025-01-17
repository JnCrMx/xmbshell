module;

#include <cassert>
#include <filesystem>
#include <future>
#include <vector>
#include <libavutil/pixfmt.h>

export module xmbshell.app:video_player;

import dreamrender;
import glm;
import spdlog;
import avcpp;
import vulkan_hpp;
import vma;
import i18n;
import xmbshell.utils;
import :component;
import :programs;
import :message_overlay;
import xmbshell.app;

namespace programs {

using namespace app;
using namespace mfk::i18n::literals;

export class video_player : public component, public action_receiver, public joystick_receiver, public mouse_receiver {
    constexpr static auto preferred_format = AV_PIX_FMT_RGBA;
    public:
        video_player(std::filesystem::path path, dreamrender::resource_loader& loader) :
            path(std::move(path)), device(loader.getDevice()), allocator(loader.getAllocator())
        {
            load_future = std::async(std::launch::async, [this, &loader] -> std::unique_ptr<video_decoding_context> {
                auto ctx = std::make_unique<video_decoding_context>();
                try {
                    ctx->ictx.openInput(this->path.string());
                    ctx->ictx.findStreamInfo();

                    for(size_t i = 0; i < ctx->ictx.streamsCount(); ++i) {
                        auto st = ctx->ictx.stream(i);
                        if (st.mediaType() == AVMEDIA_TYPE_VIDEO) {
                            ctx->vst = st;
                            break;
                        }
                    }
                    if(!ctx->vst.isValid()) {
                        throw std::runtime_error("No video stream found");
                    }

                    ctx->vdec = av::VideoDecoderContext{ctx->vst};
                    if(!ctx->vdec.isValid()) {
                        throw std::runtime_error("Cannot create video decoder context found");
                    }
                    ctx->codec = av::findDecodingCodec(ctx->vdec.raw()->codec_id);
                    ctx->vdec.setCodec(ctx->codec);
                    ctx->vdec.setRefCountedFrames(true);
                    ctx->vdec.open({{"threads", "1"}}); // TODO: change to auto, once we resolved "Resource temporarily unavailable"
                    if(!ctx->vdec.isValid()) {
                        throw std::runtime_error("Cannot open video decoder context found");
                    }

                    ctx->rescaler = av::VideoRescaler{
                        /* dst */ ctx->vdec.width(), ctx->vdec.height(), preferred_format,
                        /* src */ ctx->vdec.width(), ctx->vdec.height(), ctx->vdec.pixelFormat()
                    };
                } catch(const std::exception& e) {
                    spdlog::error("Failed to load video: {}", e.what());
                    return nullptr;
                }
                return ctx;
            });
        }
        ~video_player() {
            if(loaded) {
                device.waitIdle();
            }
        }

        result tick(xmbshell* xmb) override {
            if(!loaded && !utils::is_ready(load_future)) {
                return result::success;
            }
            if(!loaded) {
                ctx = load_future.get();
                if(!ctx->vdec.isValid()) {
                    spdlog::error("Failed to load video");
                    xmb->emplace_overlay<message_overlay>("Failed to open video"_(),
                        ctx->error_message.empty() ? "Unknown error"_() : ctx->error_message,
                        std::vector<std::string>{"OK"_()});
                    return result::close;
                }
                spdlog::info("Video of size {}x{} loaded in pixel format {}",
                    ctx->vdec.width(), ctx->vdec.height(), ctx->vdec.pixelFormat().name());
                if(ctx->vdec.pixelFormat() != preferred_format) {
                    spdlog::warn("Video is not in preferred format, converting it to {}",
                        av::PixelFormat{preferred_format}.name());
                }

                texture = std::make_unique<dreamrender::texture>(device, allocator, ctx->vdec.width(), ctx->vdec.height());
                unsigned int staging_count = xmb->get_window()->swapchainImageCount;
                staging_buffers.resize(staging_count);
                staging_buffer_allocations.resize(staging_count);
                vk::DeviceSize staging_size = ctx->vdec.width() * ctx->vdec.height() * 4;
                spdlog::debug("Allocating {} staging buffers of size {}", staging_count, staging_size);

                vk::BufferCreateInfo buffer_info({}, staging_size, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
                vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eCpuToGpu);
                for(unsigned int i = 0; i < staging_count; ++i) {
                    std::tie(staging_buffers[i], staging_buffer_allocations[i]) = allocator.createBufferUnique(buffer_info, alloc_info);
                }
                loaded = true;
            }

            float pic_aspect = texture->width / (float)texture->height;
            glm::vec2 limit(pic_aspect, 1.0f);
            limit *= zoom;
            limit /= 2.0f;

            offset = glm::clamp(offset + move_delta_pos, -limit, limit);
            return result::success;
        }

        void prerender(vk::CommandBuffer cmd, int frame, xmbshell* xmb) override {
            if(!loaded) {
                return;
            }

            while(true) {
                av::Packet pkt = ctx->ictx.readPacket();
                if(pkt.isNull()) {
                    break;
                }
                if(pkt.streamIndex() != ctx->vst.index()) {
                    continue;
                }
                av::VideoFrame videoFrame = ctx->vdec.decode(pkt);
                if(!videoFrame) {
                    continue;
                }
                spdlog::trace("Decoded frame @ {}s: {}x{} format={}", videoFrame.pts().seconds(),
                    videoFrame.width(), videoFrame.height(), videoFrame.pixelFormat().name());
                if(videoFrame.pixelFormat() != preferred_format) {
                    videoFrame.setStreamIndex(0);
                    videoFrame.setPictureType();
                    videoFrame = ctx->rescaler.rescale(videoFrame);
                    spdlog::trace("Rescaled frame @ {}s: {}x{} format={}", videoFrame.pts().seconds(),
                        videoFrame.width(), videoFrame.height(), videoFrame.pixelFormat().name());
                }
                allocator.copyMemoryToAllocation(videoFrame.data(), staging_buffer_allocations[frame].get(), 0,
                    std::min(videoFrame.size(), static_cast<size_t>(texture->width*texture->height*4)) /* videoFrame.size() is too big sometimes */);
                cmd.pipelineBarrier(
                    vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eTransfer,
                    {}, {}, {},
                    vk::ImageMemoryBarrier(
                        {}, vk::AccessFlagBits::eTransferWrite,
                        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                        vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                        texture->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                    )
                );
                cmd.copyBufferToImage(staging_buffers[frame].get(), texture->image, vk::ImageLayout::eTransferDstOptimal,
                    vk::BufferImageCopy(0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                        vk::Offset3D(0, 0, 0), vk::Extent3D(texture->width, texture->height, 1)));
                cmd.pipelineBarrier(
                    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                    {}, {}, {},
                    vk::ImageMemoryBarrier(
                        vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                        vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                        texture->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                    )
                );
                break;
            }
        }

        void render(dreamrender::gui_renderer& renderer, class xmbshell* xmb) override {
            if(!loaded) {
                return;
            }

            constexpr float size = 0.9;

            float bw = size*renderer.aspect_ratio;
            float bh = size;

            float pic_aspect = texture->width / (float)texture->height;
            float w{}, h{};
            if(pic_aspect > renderer.aspect_ratio) {
                w = bw;
                h = bw / pic_aspect;
            } else {
                h = bh;
                w = bh * pic_aspect;
            }
            glm::vec2 offset = {(1.0f-size)/2, (1.0f-size)/2};
            offset += glm::vec2((bw-w)/2/renderer.aspect_ratio, (bh-h)/2);
            offset -= (zoom-1.0f) * glm::vec2(w/renderer.aspect_ratio, h) / 2.0f;
            offset += this->offset / glm::vec2(renderer.aspect_ratio, 1.0f);

            renderer.set_clip((1.0f-size)/2, (1.0f-size)/2, size, size);
            renderer.draw_image(texture->imageView.get(), offset.x, offset.y, zoom*w, zoom*h);
            renderer.reset_clip();
        }
        result on_action(action action) override {
            if(action == action::cancel) {
                return result::close;
            }
            else if(action == action::up) {
                zoom *= 1.1f;
                return result::success;
            }
            else if(action == action::down) {
                zoom /= 1.1f;
                return result::success;
            }
            else if(action == action::extra) {
                offset = {0.0f, 0.0f};
                zoom = 1.0f;
                return result::success;
            }
            return result::failure;
        }

        result on_joystick(unsigned int index, float x, float y) override {
            if(index == 1) {
                move_delta_pos = -glm::vec2(x, y)/25.0f;
                if(std::abs(x) < 0.1f) {
                    move_delta_pos.x = 0.0f;
                }
                if(std::abs(y) < 0.1f) {
                    move_delta_pos.y = 0.0f;
                }
                return result::success;
            }
            return result::unsupported;
        }

        result on_mouse_move(float x, float y) override {
            offset = glm::vec2(x, y) - glm::vec2(0.5f, 0.5f);
            return result::success;
        }

        [[nodiscard]] bool is_opaque() const override {
            return loaded;
        }
    private:
        vk::Device device;
        vma::Allocator allocator;

        std::filesystem::path path;
        bool loaded = false;
        struct video_decoding_context {
            av::FormatContext ictx;
            av::Stream vst;
            av::VideoDecoderContext vdec;
            av::Codec codec;
            av::VideoRescaler rescaler;

            std::string error_message;
        };
        std::unique_ptr<video_decoding_context> ctx;
        std::unique_ptr<dreamrender::texture> texture;
        std::future<std::unique_ptr<video_decoding_context>> load_future;

        std::vector<vma::UniqueBuffer> staging_buffers;
        std::vector<vma::UniqueAllocation> staging_buffer_allocations;

        glm::vec2 move_delta_pos = {0.0f, 0.0f};

        float zoom = 1.0f;
        glm::vec2 offset = {0.0f, 0.0f};
};

namespace {
const inline register_program<video_player> video_player_program{
    "video_player",
    {
        "video/mp4", "video/webm", "video/ogg", "video/quicktime",
        "video/x-matroska",
    }
};
}

}
