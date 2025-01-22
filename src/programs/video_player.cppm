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
import :base_viewer;
import xmbshell.app;
import xmbshell.render;

namespace programs {

using namespace app;
using namespace mfk::i18n::literals;

export class video_player : private base_viewer, public component, public action_receiver, public joystick_receiver, public mouse_receiver {
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
                image_width = ctx->vdec.width();
                image_height = ctx->vdec.height();
                spdlog::info("Video of size {}x{} loaded in pixel format {}",
                    image_width, image_height, ctx->vdec.pixelFormat().name());
                yuv_conversion = ctx->vdec.pixelFormat() == AV_PIX_FMT_YUV420P;
                if(yuv_conversion) {
                    spdlog::info("Video is in YUV420P format, converting it to RGBA on GPU");
                } else if(ctx->vdec.pixelFormat() != preferred_format) {
                    spdlog::warn("Video is not in preferred format, converting it to {}",
                        av::PixelFormat{preferred_format}.name());
                }

                {
                    vk::ImageCreateInfo image_info(vk::ImageCreateFlagBits::eMutableFormat | vk::ImageCreateFlagBits::eExtendedUsage,
                        vk::ImageType::e2D, vk::Format::eR8G8B8A8Srgb,
                        vk::Extent3D(image_width, image_height, 1), 1, 1, vk::SampleCountFlagBits::e1,
                        vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst,
                        vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined);
                    vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eGpuOnly);
                    std::tie(decoded_image, decoded_allocation) = allocator.createImageUnique(image_info, alloc_info);
                }
                {
                    vk::ImageViewUsageCreateInfo view_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
                    vk::ImageViewCreateInfo view_info({}, decoded_image.get(), vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Srgb,
                        vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
                    vk::StructureChain view_chain{view_info, view_usage};
                    decoded_view = device.createImageViewUnique(view_chain.get<vk::ImageViewCreateInfo>());
                }

                unsigned int staging_count = xmb->get_window()->swapchainImageCount;
                staging_buffers.resize(staging_count);
                staging_buffer_allocations.resize(staging_count);
                vk::DeviceSize staging_size = image_width * image_height * 4;
                spdlog::debug("Allocating {} staging buffers of size {}", staging_count, staging_size);

                vk::BufferCreateInfo buffer_info({}, staging_size, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
                vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eCpuToGpu);
                for(unsigned int i = 0; i < staging_count; ++i) {
                    std::tie(staging_buffers[i], staging_buffer_allocations[i]) = allocator.createBufferUnique(buffer_info, alloc_info);
                }

                if(yuv_conversion) {
                    unormView = device.createImageViewUnique(vk::ImageViewCreateInfo({}, decoded_image.get(), vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm,
                        vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));

                    std::array<vk::DescriptorSetLayoutBinding, 4> bindings = {
			            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute),
		            	vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute),
		            	vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute),
		            	vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute)
		            };
		            descriptorSetLayout = device.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({}, bindings));

                    vk::DescriptorPoolSize pool_size(vk::DescriptorType::eStorageImage, 4);
                    descriptorPool = device.createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({}, 1, pool_size));
                    descriptorSet = device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(descriptorPool.get(), descriptorSetLayout.get())).front();

                    pipelineLayout = device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, descriptorSetLayout.get()));
                    vk::UniqueShaderModule shaderModule = render::shaders::yuv420p::decode_comp(device);
                    vk::PipelineShaderStageCreateInfo shaderInfo({}, vk::ShaderStageFlagBits::eCompute, shaderModule.get(), "main");
                    vk::Result r{};
                    std::tie(r, pipeline) = device.createComputePipelineUnique({}, vk::ComputePipelineCreateInfo({}, shaderInfo, pipelineLayout.get())).asTuple();
                    if(r != vk::Result::eSuccess) {
                        throw std::runtime_error("Failed to create compute pipeline");
                    }

                    plane_textures[0] = std::make_unique<dreamrender::texture>(device, allocator, image_width, image_height, vk::ImageUsageFlagBits::eStorage, vk::Format::eR8Unorm);
                    plane_textures[1] = std::make_unique<dreamrender::texture>(device, allocator, image_width/2, image_height/2, vk::ImageUsageFlagBits::eStorage, vk::Format::eR8Unorm);
                    plane_textures[2] = std::make_unique<dreamrender::texture>(device, allocator, image_width/2, image_height/2, vk::ImageUsageFlagBits::eStorage, vk::Format::eR8Unorm);

                    vk::DescriptorImageInfo output_info({}, unormView.get(), vk::ImageLayout::eGeneral);
                    vk::DescriptorImageInfo input_y_info({}, plane_textures[0]->imageView.get(), vk::ImageLayout::eGeneral);
                    vk::DescriptorImageInfo input_cb_info({}, plane_textures[1]->imageView.get(), vk::ImageLayout::eGeneral);
                    vk::DescriptorImageInfo input_cr_info({}, plane_textures[2]->imageView.get(), vk::ImageLayout::eGeneral);
                    std::array<vk::WriteDescriptorSet, 4> writes = {
                        vk::WriteDescriptorSet(descriptorSet, 0, 0, 1, vk::DescriptorType::eStorageImage, &output_info),
                        vk::WriteDescriptorSet(descriptorSet, 1, 0, 1, vk::DescriptorType::eStorageImage, &input_y_info),
                        vk::WriteDescriptorSet(descriptorSet, 2, 0, 1, vk::DescriptorType::eStorageImage, &input_cb_info),
                        vk::WriteDescriptorSet(descriptorSet, 3, 0, 1, vk::DescriptorType::eStorageImage, &input_cr_info)
                    };
                    device.updateDescriptorSets(writes, {});
                }

                loaded = true;
            }

            base_viewer::tick();
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
                spdlog::trace("Decoded frame @ {}s: {}x{} format={}, size={}", videoFrame.pts().seconds(),
                    videoFrame.width(), videoFrame.height(), videoFrame.pixelFormat().name(),
                    videoFrame.size());
                if(yuv_conversion) {
                    unsigned int offset = 0;
                    allocator.copyMemoryToAllocation(videoFrame.data(0), staging_buffer_allocations[frame].get(), offset, videoFrame.size(0));
                    offset += videoFrame.size(0);
                    allocator.copyMemoryToAllocation(videoFrame.data(1), staging_buffer_allocations[frame].get(), offset, videoFrame.size(1));
                    offset += videoFrame.size(1);
                    allocator.copyMemoryToAllocation(videoFrame.data(2), staging_buffer_allocations[frame].get(), offset, videoFrame.size(2));

                    cmd.pipelineBarrier(
                        vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer,
                        {}, {}, {}, {
                            vk::ImageMemoryBarrier(
                                {}, vk::AccessFlagBits::eTransferWrite,
                                vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                                plane_textures[0]->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                            ),
                            vk::ImageMemoryBarrier(
                                {}, vk::AccessFlagBits::eTransferWrite,
                                vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                                plane_textures[1]->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                            ),
                            vk::ImageMemoryBarrier(
                                {}, vk::AccessFlagBits::eTransferWrite,
                                vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                                plane_textures[2]->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                            ),
                        }
                    );
                    std::array<vk::BufferImageCopy, 3> copies = {
                        vk::BufferImageCopy(0, videoFrame.raw()->linesize[0], 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                            vk::Offset3D(0, 0, 0), vk::Extent3D(videoFrame.width(), videoFrame.height(), 1)),
                        vk::BufferImageCopy(videoFrame.size(0), videoFrame.raw()->linesize[1], 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                            vk::Offset3D(0, 0, 0), vk::Extent3D(videoFrame.width()/2, videoFrame.height()/2, 1)),
                        vk::BufferImageCopy(videoFrame.size(0)+videoFrame.size(1), videoFrame.raw()->linesize[2], 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                            vk::Offset3D(0, 0, 0), vk::Extent3D(videoFrame.width()/2, videoFrame.height()/2, 1))
                    };
                    cmd.copyBufferToImage(staging_buffers[frame].get(), plane_textures[0]->image, vk::ImageLayout::eTransferDstOptimal, copies[0]);
                    cmd.copyBufferToImage(staging_buffers[frame].get(), plane_textures[1]->image, vk::ImageLayout::eTransferDstOptimal, copies[1]);
                    cmd.copyBufferToImage(staging_buffers[frame].get(), plane_textures[2]->image, vk::ImageLayout::eTransferDstOptimal, copies[2]);

                    cmd.pipelineBarrier(
                        vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eComputeShader,
                        {}, {}, {}, {
                            vk::ImageMemoryBarrier(
                                vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                                vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral,
                                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                                plane_textures[0]->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                            ),
                            vk::ImageMemoryBarrier(
                                vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                                vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral,
                                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                                plane_textures[1]->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                            ),
                            vk::ImageMemoryBarrier(
                                vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                                vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral,
                                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                                plane_textures[2]->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                            ),
                            vk::ImageMemoryBarrier(
                                vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eShaderWrite,
                                vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                                vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                                decoded_image.get(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                            )
                        }
                    );
                    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline.get());
                    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout.get(), 0, descriptorSet, {});
                    cmd.dispatch((image_width+31)/16/2, (image_height+31)/16/2, 1);
                    cmd.pipelineBarrier(
                        vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eFragmentShader,
                        {}, {}, {},
                        vk::ImageMemoryBarrier(
                            vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead,
                            vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
                            vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                            decoded_image.get(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                        )
                    );
                } else {
                    if(videoFrame.pixelFormat() != preferred_format) {
                        videoFrame.setStreamIndex(0);
                        videoFrame.setPictureType();
                        videoFrame = ctx->rescaler.rescale(videoFrame);
                        spdlog::trace("Rescaled frame @ {}s: {}x{} format={}", videoFrame.pts().seconds(),
                            videoFrame.width(), videoFrame.height(), videoFrame.pixelFormat().name());
                    }
                    allocator.copyMemoryToAllocation(videoFrame.data(), staging_buffer_allocations[frame].get(), 0,
                        std::min(videoFrame.size(), static_cast<size_t>(image_width*image_height*4)) /* videoFrame.size() is too big sometimes */);
                    cmd.pipelineBarrier(
                        vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eTransfer,
                        {}, {}, {},
                        vk::ImageMemoryBarrier(
                            {}, vk::AccessFlagBits::eTransferWrite,
                            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                            vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                            decoded_image.get(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                        )
                    );
                    cmd.copyBufferToImage(staging_buffers[frame].get(), decoded_image.get(), vk::ImageLayout::eTransferDstOptimal,
                        vk::BufferImageCopy(0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                            vk::Offset3D(0, 0, 0), vk::Extent3D(image_width, image_height, 1)));
                    cmd.pipelineBarrier(
                        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                        {}, {}, {},
                        vk::ImageMemoryBarrier(
                            vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                            vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                            vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
                            decoded_image.get(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                        )
                    );
                }
                break;
            }
        }

        void render(dreamrender::gui_renderer& renderer, class xmbshell* xmb) override {
            if(!loaded) {
                return;
            }

            constexpr float size = 0.9;
            base_viewer::render(decoded_view.get(), size, renderer);
        }
        result on_action(action action) override {
            if(action == action::cancel) {
                return result::close;
            } else {
                result r = base_viewer::on_action(action);
                if(r != result::unsupported) {
                    return r;
                }
            }
            return result::failure;
        }

        result on_joystick(unsigned int index, float x, float y) override {
            return base_viewer::on_joystick(index, x, y);
        }

        result on_mouse_move(float x, float y) override {
            return base_viewer::on_mouse_move(x, y);
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
        vma::UniqueImage decoded_image;
        vma::UniqueAllocation decoded_allocation;
        vk::UniqueImageView decoded_view;
        unsigned int image_width, image_height;

        bool yuv_conversion = false;
        std::array<std::unique_ptr<dreamrender::texture>, 3> plane_textures;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline pipeline;
        vk::UniqueDescriptorSetLayout descriptorSetLayout;
        vk::UniqueDescriptorPool descriptorPool;
        vk::DescriptorSet descriptorSet;
        vk::UniqueImageView unormView;

        std::future<std::unique_ptr<video_decoding_context>> load_future;

        std::vector<vma::UniqueBuffer> staging_buffers;
        std::vector<vma::UniqueAllocation> staging_buffer_allocations;

        enum class play_state {
            playing,
            paused,
            stopped,
        };
        play_state state = play_state::playing;
        std::chrono::steady_clock::time_point start_time;
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
