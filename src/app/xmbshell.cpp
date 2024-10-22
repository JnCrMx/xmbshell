module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

module xmbshell.app;

import i18n;
import spdlog;
import dreamrender;
import glm;
import vulkan_hpp;
import vma;
import sdl2;
import xmbshell.config;
import xmbshell.render;

using namespace mfk::i18n::literals;

namespace app
{
	xmbshell::xmbshell(window* window) : phase(window)
	{
	}

	xmbshell::~xmbshell()
	{
	}

	void xmbshell::preload()
	{
		phase::preload();

		font_render = std::make_unique<font_renderer>(config::CONFIG.fontPath, 32, device, allocator, win->swapchainExtent);
		image_render = std::make_unique<image_renderer>(device, win->swapchainExtent);
		wave_render = std::make_unique<render::wave_renderer>(device, allocator, win->swapchainExtent);

		{
			std::array<vk::AttachmentDescription, 2> attachments = {
				vk::AttachmentDescription({}, win->swapchainFormat.format, win->config.sampleCount,
					vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal),
				vk::AttachmentDescription({}, win->swapchainFormat.format, vk::SampleCountFlagBits::e1,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal)
			};
			vk::AttachmentReference ref(0, vk::ImageLayout::eColorAttachmentOptimal);
			vk::AttachmentReference rref(1, vk::ImageLayout::eColorAttachmentOptimal);
			vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref, rref);
			std::array<vk::SubpassDependency, 2> deps{
				vk::SubpassDependency(vk::SubpassExternal, 0,
					vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
					vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eColorAttachmentWrite),
				vk::SubpassDependency(0, vk::SubpassExternal,
					vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader,
					vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead)
			};
			vk::RenderPassCreateInfo renderpass_info({}, attachments, subpass, deps);
			backgroundRenderPass = device.createRenderPassUnique(renderpass_info);
			debugName(device, backgroundRenderPass.get(), "Background Render Pass");
		}
		{
			std::array<vk::AttachmentDescription, 1> attachments = {
				vk::AttachmentDescription({}, win->swapchainFormat.format, vk::SampleCountFlagBits::e1,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal)
			};
			vk::AttachmentReference ref(0, vk::ImageLayout::eColorAttachmentOptimal);
			vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref);
			vk::SubpassDependency dep(0, vk::SubpassExternal,
				vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader,
				vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead);
			vk::RenderPassCreateInfo renderpass_info({}, attachments, subpass, dep);
			blurRenderPass = device.createRenderPassUnique(renderpass_info);
			debugName(device, blurRenderPass.get(), "Blur Render Pass");
		}
		{
			std::array<vk::AttachmentDescription, 2> attachments = {
				vk::AttachmentDescription({}, win->swapchainFormat.format, win->config.sampleCount,
					vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal),
				vk::AttachmentDescription({}, win->swapchainFormat.format, vk::SampleCountFlagBits::e1,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR)
			};
			vk::AttachmentReference ref(0, vk::ImageLayout::eColorAttachmentOptimal);
			vk::AttachmentReference rref(1, vk::ImageLayout::eColorAttachmentOptimal);
			vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref, rref);
			vk::SubpassDependency dep(vk::SubpassExternal, 0,
				vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eColorAttachmentWrite);
			vk::RenderPassCreateInfo renderpass_info({}, attachments, subpass, dep);
			shellRenderPass = device.createRenderPassUnique(renderpass_info);
			debugName(device, shellRenderPass.get(), "Shell Render Pass");
		}
		{
			vk::SamplerCreateInfo sampler_info({}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
				vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
				0.0f, false, 0.0f, false, vk::CompareOp::eNever, 0.0f, 0.0f, vk::BorderColor::eFloatTransparentBlack, false);
			blurSampler = device.createSamplerUnique(sampler_info);
    	}
		{
			std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {
				vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),
			};
			vk::DescriptorSetLayoutCreateInfo info({}, bindings);
			blurDescriptorSetLayout = device.createDescriptorSetLayoutUnique(info);
		}
		{
			vk::PipelineLayoutCreateInfo info({}, blurDescriptorSetLayout.get(), {});
			blurPipelineLayout = device.createPipelineLayoutUnique(info);
		}
		{
			vk::UniqueShaderModule vertexShader = render::shaders::blur::vert(device);
			vk::UniqueShaderModule fragmentShader = render::shaders::blur::frag(device);
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
			vk::PipelineMultisampleStateCreateInfo multisample({}, vk::SampleCountFlagBits::e1);
			vk::PipelineDepthStencilStateCreateInfo depthStencil({}, false, false);

			vk::PipelineColorBlendAttachmentState attachment(true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
				vk::BlendFactor::eOne, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
			vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, vk::LogicOp::eClear, attachment);

			std::array<vk::DynamicState, 2> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
			vk::PipelineDynamicStateCreateInfo dynamic({}, dynamicStates);

			vk::GraphicsPipelineCreateInfo pipeline_info({}, shaders, &vertex_input,
				&input_assembly, &tesselation, &viewport, &rasterization, &multisample, &depthStencil, &colorBlend, &dynamic,
				blurPipelineLayout.get(), blurRenderPass.get());
			blurPipeline = device.createGraphicsPipelineUnique({}, pipeline_info).value;
			debugName(device, blurPipeline.get(), "Blur Pipeline");
		}

		{
			renderImage = std::make_unique<texture>(device, allocator,
				win->swapchainExtent, vk::ImageUsageFlagBits::eColorAttachment,
				win->swapchainFormat.format, win->config.sampleCount, false, vk::ImageAspectFlagBits::eColor);
			blurImage = std::make_unique<texture>(device, allocator,
				win->swapchainExtent, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
				win->swapchainFormat.format, vk::SampleCountFlagBits::e1, false, vk::ImageAspectFlagBits::eColor);
		}

		font_render->preload(loader, {shellRenderPass.get()}, win->config.sampleCount, win->pipelineCache.get());
		image_render->preload({backgroundRenderPass.get(), shellRenderPass.get()}, win->config.sampleCount, win->pipelineCache.get());
		wave_render->preload({backgroundRenderPass.get()}, win->config.sampleCount, win->pipelineCache.get());

		menu.preload(device, allocator, *loader);
		news.preload(device, allocator, *loader);

		if(config::CONFIG.backgroundType == config::config::background_type::image) {
			backgroundTexture = std::make_unique<texture>(device, allocator);
			loader->loadTexture(backgroundTexture.get(), config::CONFIG.backgroundImage);
		}
		config::CONFIG.addCallback("background-type", [this](const std::string&){
			if(config::CONFIG.backgroundType == config::config::background_type::image) {
				reload_background();
			} else {
				backgroundTexture.reset();
			}
		});
		config::CONFIG.addCallback("background-image", [this](const std::string&){
			if(config::CONFIG.backgroundType == config::config::background_type::image) {
				reload_background();
			} else {
				backgroundTexture.reset();
			}
		});
	}

	void xmbshell::prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews)
	{
		phase::prepare(swapchainImages, swapchainViews);

		const int imageCount = swapchainImages.size();
		{
			vk::DescriptorPoolSize size(vk::DescriptorType::eCombinedImageSampler, imageCount);
			vk::DescriptorPoolCreateInfo pool_info({}, imageCount, size);
			blurDescriptorPool = device.createDescriptorPoolUnique(pool_info);

			std::vector<vk::DescriptorSetLayout> layouts(imageCount, blurDescriptorSetLayout.get());
			vk::DescriptorSetAllocateInfo alloc_info(blurDescriptorPool.get(), layouts);
			blurDescriptorSets = device.allocateDescriptorSets(alloc_info);
		}
		this->swapchainImages = swapchainImages;

		std::vector<vk::DescriptorImageInfo> imageInfos(imageCount);
		std::vector<vk::WriteDescriptorSet> writes(imageCount);

		for(int i=0; i<imageCount; i++)
		{
			{
				std::array<vk::ImageView, 2> attachments = {renderImage->imageView.get(), swapchainViews[i]};
				vk::FramebufferCreateInfo framebuffer_info({}, shellRenderPass.get(), attachments,
					win->swapchainExtent.width, win->swapchainExtent.height, 1);
				framebuffers.push_back(device.createFramebufferUnique(framebuffer_info));
				debugName(device, framebuffers.back().get(), "XMB Shell Framebuffer #"+std::to_string(i));
			}
			{
				std::array<vk::ImageView, 2> attachments = {renderImage->imageView.get(), swapchainViews[i]};
				vk::FramebufferCreateInfo framebuffer_info({}, backgroundRenderPass.get(), attachments,
					win->swapchainExtent.width, win->swapchainExtent.height, 1);
				backgroundFramebuffers.push_back(device.createFramebufferUnique(framebuffer_info));
				debugName(device, backgroundFramebuffers.back().get(), "XMB Shell Background Framebuffer #"+std::to_string(i));
			}
			{
				vk::FramebufferCreateInfo blur_framebuffer_info({}, blurRenderPass.get(), blurImage->imageView.get(),
					win->swapchainExtent.width, win->swapchainExtent.height, 1);
				blurFramebuffers.push_back(device.createFramebufferUnique(blur_framebuffer_info));
				debugName(device, blurFramebuffers.back().get(), "XMB Shell Blur Framebuffer #"+std::to_string(i));
			}

			imageInfos[i] = vk::DescriptorImageInfo(blurSampler.get(), swapchainViews[i], vk::ImageLayout::eShaderReadOnlyOptimal);
			writes[i] = vk::WriteDescriptorSet(blurDescriptorSets[i], 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &imageInfos[i]);
		}
		device.updateDescriptorSets(writes, {});

		font_render->prepare(swapchainViews.size());
		image_render->prepare(swapchainViews.size());
		wave_render->prepare(swapchainViews.size());
	}

	void xmbshell::render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence)
	{
		vk::CommandBuffer commandBuffer = commandBuffers[frame];

		commandBuffer.begin(vk::CommandBufferBeginInfo());
		{
			vk::ClearValue color(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
			if(config::CONFIG.backgroundType == config::config::background_type::color) {
				color = vk::ClearColorValue(std::array<float, 4>{
					config::CONFIG.backgroundColor.r,
					config::CONFIG.backgroundColor.g,
					config::CONFIG.backgroundColor.b,
					1.0f
				});
			}
			if(is_ingame) {
				color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.5f});
			}
			commandBuffer.beginRenderPass(vk::RenderPassBeginInfo(backgroundRenderPass.get(), backgroundFramebuffers[frame].get(),
				vk::Rect2D({0, 0}, win->swapchainExtent), color), vk::SubpassContents::eInline);
			vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
			vk::Rect2D scissor({0,0}, win->swapchainExtent);
			commandBuffer.setViewport(0, viewport);
			commandBuffer.setScissor(0, scissor);

			if(!is_ingame) {
				if(config::CONFIG.backgroundType == config::config::background_type::wave) {
					wave_render->render(commandBuffer, frame, backgroundRenderPass.get());
				}
				else if(config::CONFIG.backgroundType == config::config::background_type::image) {
					if(backgroundTexture) {
						image_render->renderImageSized(commandBuffer, frame, backgroundRenderPass.get(), *backgroundTexture,
							0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height);
					}
				}
			}

			commandBuffer.endRenderPass();
		}
		if(blur_background) {
			commandBuffer.beginRenderPass(vk::RenderPassBeginInfo(blurRenderPass.get(), blurFramebuffers[frame].get(),
				vk::Rect2D({0, 0}, win->swapchainExtent)), vk::SubpassContents::eInline);
			vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
			vk::Rect2D scissor({0,0}, win->swapchainExtent);
			commandBuffer.setViewport(0, viewport);
			commandBuffer.setScissor(0, scissor);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, blurPipeline.get());
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, blurPipelineLayout.get(), 0, blurDescriptorSets[frame], {});
			commandBuffer.draw(4, 1, 0, 0);

			commandBuffer.endRenderPass();
		}
		else {
			commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer,
				{}, {}, {},
				{
					vk::ImageMemoryBarrier(
						{}, vk::AccessFlagBits::eTransferRead,
						vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal,
						vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
						swapchainImages[frame], vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
					),
					vk::ImageMemoryBarrier(
						{}, vk::AccessFlagBits::eTransferWrite,
						vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
						vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
						blurImage->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
					),
				}
			);

			vk::ImageCopy region(
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0),
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0),
				vk::Extent3D(win->swapchainExtent, 1));
			commandBuffer.copyImage(swapchainImages[frame], vk::ImageLayout::eTransferSrcOptimal,
				blurImage->image, vk::ImageLayout::eTransferDstOptimal, region);

			commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
				{}, {}, {},
				{
					vk::ImageMemoryBarrier(
						vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
						vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
						vk::QueueFamilyIgnored, vk::QueueFamilyIgnored,
						blurImage->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
					),
				}
			);
			/*commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				{}, {}, {},
				{
					vk::ImageMemoryBarrier(
						vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eColorAttachmentWrite,
						vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eColorAttachmentOptimal,
						VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
						swapchainImages[frame], vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
					),
				}
			);*/
		}
		{
			vk::ClearValue color(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});
			commandBuffer.beginRenderPass(vk::RenderPassBeginInfo(shellRenderPass.get(), framebuffers[frame].get(),
				vk::Rect2D({0, 0}, win->swapchainExtent), color), vk::SubpassContents::eInline);
			vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
			vk::Rect2D scissor({0,0}, win->swapchainExtent);
			commandBuffer.setViewport(0, viewport);
			commandBuffer.setScissor(0, scissor);

			image_render->renderImageSized(commandBuffer, frame, shellRenderPass.get(), blurImage->imageView.get(),
				0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height);

			gui_renderer ctx(commandBuffer, frame, shellRenderPass.get(), win->swapchainExtent, font_render.get(), image_render.get());
			render_gui(ctx);

			commandBuffer.endRenderPass();
		}
		font_render->finish(frame);
		image_render->finish(frame);
		commandBuffer.end();

		vk::PipelineStageFlags waitFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::SubmitInfo submit_info(imageAvailable, waitFlags, commandBuffer, renderFinished);
		graphicsQueue.submit(submit_info, fence);
	}

	void xmbshell::render_gui(gui_renderer& renderer) {
		menu.render(renderer);

		static const std::chrono::time_zone* timezone = [](){
			auto tz = std::chrono::current_zone();
			auto system = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
			auto local = std::chrono::zoned_time(tz, system);
			spdlog::debug("{}", std::format("Timezone: {}, System Time: {}, Local Time: {}", tz->name(), system, local));
			return tz;
		}();
    	auto now = std::chrono::zoned_time(timezone, std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
		renderer.draw_text(std::vformat("{:"+config::CONFIG.dateTimeFormat+"}", std::make_format_args(now)),
			0.831770833f+config::CONFIG.dateTimeOffset, 0.086111111f, 0.021296296f*2.5f);

		news.render(renderer);

		double debug_y = 0.0;
		if(config::CONFIG.showFPS) {
			renderer.draw_text("FPS: {:.2f}"_(win->currentFPS), 0, debug_y, 0.05f, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
			debug_y += 0.025f;
		}
		if(config::CONFIG.showMemory) {
			vk::DeviceSize budget{}, usage{};
			for(const auto& b : allocator.getHeapBudgets()) {
				budget += b.budget;
				usage += b.usage;
			}
			constexpr double mb = 1024.0*1024.0;
			auto u = usage/mb;
			auto b = budget/mb;
			renderer.draw_text("Memory: {:.2f}/{:.2f} MB"_(u, b), 0, debug_y, 0.05f, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
			debug_y += 0.025f;
		}
	}

	void xmbshell::reload_background() {
		if(config::CONFIG.backgroundType == config::config::background_type::image) {
			backgroundTexture = std::make_unique<texture>(device, allocator);
			loader->loadTexture(backgroundTexture.get(), config::CONFIG.backgroundImage);
		}
	}

	void xmbshell::key_up(sdl::Keysym key)
	{
		spdlog::trace("Key up: {}", key.sym);
		menu.key_up(key);
	}
	void xmbshell::key_down(sdl::Keysym key)
	{
		spdlog::trace("Key down: {}", key.sym);
		menu.key_down(key);
	}

	void xmbshell::add_controller(sdl::GameController* controller)
	{
	}
	void xmbshell::remove_controller(sdl::GameController* controller)
	{
	}
	void xmbshell::button_down(sdl::GameController* controller, sdl::GameControllerButton button)
	{
		spdlog::trace("Button down: {}", fmt::underlying(button));
		menu.button_down(controller, button);
	}
	void xmbshell::button_up(sdl::GameController* controller, sdl::GameControllerButton button)
	{
		spdlog::trace("Button up: {}", fmt::underlying(button));
		menu.button_up(controller, button);
	}
	void xmbshell::axis_motion(sdl::GameController* controller, sdl::GameControllerAxis axis, int16_t value)
	{
		spdlog::trace("Axis motion: {} {}", fmt::underlying(axis), value);
		menu.axis_motion(controller, axis, value);
	}
}
