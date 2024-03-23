#include "app/xmbshell.hpp"

#include "config.hpp"
#include "render/debug.hpp"
#include "render/utils.hpp"

#include "blur.vert.h"
#include "blur.frag.h"

#include <chrono>

namespace app
{
	xmbshell::xmbshell(window* window) : phase(window)
	{
	}

	xmbshell::~xmbshell()
	{
		allocator.destroyImage(renderImage, renderAllocation);
	}

	void xmbshell::preload()
	{
		font_render = std::make_unique<font_renderer>(config::CONFIG.fontPath, 32, device, allocator, win->swapchainExtent);
		image_render = std::make_unique<image_renderer>(device, win->swapchainExtent);
		wave_render = std::make_unique<wave_renderer>(device, allocator, win->swapchainExtent);

		pool = device.createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsFamily));

		{
			std::array<vk::AttachmentDescription, 2> attachments = {
				vk::AttachmentDescription({}, win->swapchainFormat.format, config::CONFIG.sampleCount,
					vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
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
			constexpr auto stages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			constexpr auto access = vk::AccessFlagBits::eColorAttachmentWrite;
			vk::SubpassDependency dep(VK_SUBPASS_EXTERNAL, 0,
				stages, stages,
				access, access);
			vk::RenderPassCreateInfo renderpass_info({}, attachments, subpass, dep);
			backgroundRenderPass = device.createRenderPassUnique(renderpass_info);
			debugName(device, backgroundRenderPass.get(), "Background Render Pass");
		}
		{
			std::array<vk::AttachmentDescription, 1> attachments = {
				vk::AttachmentDescription({}, win->swapchainFormat.format, config::CONFIG.sampleCount,
					vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal)
			};
			vk::AttachmentReference ref(0, vk::ImageLayout::eColorAttachmentOptimal);
			vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref);
			vk::RenderPassCreateInfo renderpass_info({}, attachments, subpass, {});
			blurRenderPass = device.createRenderPassUnique(renderpass_info);
			debugName(device, blurRenderPass.get(), "Blur Render Pass");
		}
		{
			std::array<vk::AttachmentDescription, 2> attachments = {
				vk::AttachmentDescription({}, win->swapchainFormat.format, config::CONFIG.sampleCount,
					vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal),
				vk::AttachmentDescription({}, win->swapchainFormat.format, vk::SampleCountFlagBits::e1,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral)
			};
			vk::AttachmentReference ref(0, vk::ImageLayout::eColorAttachmentOptimal);
			vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref);
			vk::RenderPassCreateInfo renderpass_info({}, attachments, subpass, {});
			shellRenderPass = device.createRenderPassUnique(renderpass_info);
			debugName(device, shellRenderPass.get(), "Shell Render Pass");
		}
		{
			std::array<vk::AttachmentDescription, 2> attachments = {
				vk::AttachmentDescription({}, win->swapchainFormat.format, config::CONFIG.sampleCount,
					vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal),
				vk::AttachmentDescription({}, win->swapchainFormat.format, vk::SampleCountFlagBits::e1,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR)
			};
			vk::AttachmentReference ref(0, vk::ImageLayout::eColorAttachmentOptimal);
			vk::AttachmentReference rref(1, vk::ImageLayout::eColorAttachmentOptimal);
			vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref, rref);
			vk::RenderPassCreateInfo renderpass_info({}, attachments, subpass, {});
			popupRenderPass = device.createRenderPassUnique(renderpass_info);
			debugName(device, popupRenderPass.get(), "Popup Render Pass");
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
			vk::UniqueShaderModule vertexShader = createShader(device, shaders_blur_vert);
			vk::UniqueShaderModule fragmentShader = createShader(device, shaders_blur_frag);
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
			vk::PipelineMultisampleStateCreateInfo multisample({}, config::CONFIG.sampleCount);
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
			vk::ImageCreateInfo image_info({}, vk::ImageType::e2D, win->swapchainFormat.format,
				vk::Extent3D(win->swapchainExtent, 1), 1, 1, config::CONFIG.sampleCount,
				vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment,
				vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined);
			vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eGpuOnly);
			std::tie(renderImage, renderAllocation) = allocator.createImage(image_info, alloc_info);
			debugName(device, renderImage, "XMB Shell Render Image");

			vk::ImageViewCreateInfo view_info({}, renderImage, vk::ImageViewType::e2D, win->swapchainFormat.format,
				vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
			renderView = device.createImageViewUnique(view_info);
			debugName(device, renderView.get(), "XMB Shell Render Image View");
		}

		FT_Library ft;
		FT_Error err = FT_Init_FreeType(&ft);

		font_render->preload(ft, loader, {shellRenderPass.get()});
		image_render->preload({backgroundRenderPass.get(), shellRenderPass.get()});
		wave_render->preload({backgroundRenderPass.get()});
		menu.preload(device, allocator, *loader);

		if(config::CONFIG.backgroundType == config::config::background_type::image) {
			backgroundTexture = std::make_unique<texture>(device, allocator);
			loader->loadTexture(backgroundTexture.get(), config::CONFIG.backgroundImage);
		}
	}

	void xmbshell::prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews)
	{
		const int imageCount = swapchainImages.size();
		commandBuffers = device.allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(pool.get(), vk::CommandBufferLevel::ePrimary, imageCount));
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
				std::array<vk::ImageView, 2> attachments = {renderView.get(), swapchainViews[i]};
				vk::FramebufferCreateInfo framebuffer_info({}, shellRenderPass.get(), attachments,
					win->swapchainExtent.width, win->swapchainExtent.height, 1);
				framebuffers.push_back(device.createFramebufferUnique(framebuffer_info));
				debugName(device, framebuffers.back().get(), "XMB Shell Framebuffer #"+std::to_string(i));
			}
			{
				std::array<vk::ImageView, 2> attachments = {renderView.get(), swapchainViews[i]};
				vk::FramebufferCreateInfo framebuffer_info({}, backgroundRenderPass.get(), attachments,
					win->swapchainExtent.width, win->swapchainExtent.height, 1);
				backgroundFramebuffers.push_back(device.createFramebufferUnique(framebuffer_info));
				debugName(device, backgroundFramebuffers.back().get(), "XMB Shell Background Framebuffer #"+std::to_string(i));
			}
			{
				vk::FramebufferCreateInfo blur_framebuffer_info({}, blurRenderPass.get(), renderView.get(),
					win->swapchainExtent.width, win->swapchainExtent.height, 1);
				blurFramebuffers.push_back(device.createFramebufferUnique(blur_framebuffer_info));
				debugName(device, blurFramebuffers.back().get(), "XMB Shell Blur Framebuffer #"+std::to_string(i));
			}

			debugName(device, commandBuffers[i].get(), "XMB Shell Command Buffer #"+std::to_string(i));

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
		vk::UniqueCommandBuffer& commandBuffer = commandBuffers[frame];

		commandBuffer->begin(vk::CommandBufferBeginInfo());
		{
			vk::ClearValue color(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});
			if(config::CONFIG.backgroundType == config::config::background_type::color) {
				color = vk::ClearColorValue(std::array<float, 4>{
					config::CONFIG.backgroundColor.r,
					config::CONFIG.backgroundColor.g,
					config::CONFIG.backgroundColor.b,
					1.0f
				});
			}
			commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(backgroundRenderPass.get(), backgroundFramebuffers[frame].get(),
				vk::Rect2D({0, 0}, win->swapchainExtent), color), vk::SubpassContents::eInline);
			vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
			vk::Rect2D scissor({0,0}, win->swapchainExtent);
			commandBuffer->setViewport(0, viewport);
			commandBuffer->setScissor(0, scissor);

			if(config::CONFIG.backgroundType == config::config::background_type::wave) {
				wave_render->render(commandBuffer.get(), frame, backgroundRenderPass.get());
			}
			else if(config::CONFIG.backgroundType == config::config::background_type::image) {
				image_render->renderImageSized(commandBuffer.get(), frame, backgroundRenderPass.get(), *backgroundTexture,
					0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height);
			}

			commandBuffer->endRenderPass();
		}
		{
			commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(blurRenderPass.get(), blurFramebuffers[frame].get(),
				vk::Rect2D({0, 0}, win->swapchainExtent)), vk::SubpassContents::eInline);
			vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
			vk::Rect2D scissor({0,0}, win->swapchainExtent);
			commandBuffer->setViewport(0, viewport);
			commandBuffer->setScissor(0, scissor);

			if(blur_background) {
				commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, blurPipeline.get());
				commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, blurPipelineLayout.get(), 0, blurDescriptorSets[frame], {});
				commandBuffer->draw(4, 1, 0, 0);
			}

			commandBuffer->endRenderPass();
		}
		{
			commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(shellRenderPass.get(), framebuffers[frame].get(),
				vk::Rect2D({0, 0}, win->swapchainExtent)), vk::SubpassContents::eInline);
			vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
			vk::Rect2D scissor({0,0}, win->swapchainExtent);
			commandBuffer->setViewport(0, viewport);
			commandBuffer->setScissor(0, scissor);

			gui_renderer ctx(commandBuffer.get(), frame, shellRenderPass.get(), win->swapchainExtent, font_render.get(), image_render.get());
			render_gui(ctx);

			commandBuffer->endRenderPass();
		}
		{
			commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(popupRenderPass.get(), framebuffers[frame].get(),
				vk::Rect2D({0, 0}, win->swapchainExtent)), vk::SubpassContents::eInline);
			vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
			vk::Rect2D scissor({0,0}, win->swapchainExtent);
			commandBuffer->setViewport(0, viewport);
			commandBuffer->setScissor(0, scissor);

			// Use shellRenderPass for now, because it is compatible with the popupRenderPass
			gui_renderer ctx(commandBuffer.get(), frame, shellRenderPass.get(), win->swapchainExtent, font_render.get(), image_render.get());
			// TODO

			commandBuffer->endRenderPass();
		}
		font_render->finish(frame);
		image_render->finish(frame);
		commandBuffer->end();

		vk::PipelineStageFlags waitFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::SubmitInfo submit_info(imageAvailable, waitFlags, commandBuffer.get(), renderFinished);
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

		double debug_y = 0.0;
		if(config::CONFIG.showFPS) {
			renderer.draw_text(std::format("FPS: {:.2f}", win->currentFPS), 0, debug_y, 0.05f, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
			debug_y += 0.025f;
		}
		if(config::CONFIG.showMemory) {
			vk::DeviceSize budget{}, usage{};
			for(const auto& b : allocator.getHeapBudgets()) {
				budget += b.budget;
				usage += b.usage;
			}
			constexpr double mb = 1024.0*1024.0;
			renderer.draw_text(std::format("Memory: {:.2f}/{:.2f} MB", usage/mb, budget/mb), 0, debug_y, 0.05f, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
			debug_y += 0.025f;
		}
	}

	void xmbshell::key_up(SDL_Keysym key)
	{
		spdlog::trace("Key up: {}", key.sym);
		menu.key_up(key);
	}
	void xmbshell::key_down(SDL_Keysym key)
	{
		spdlog::trace("Key down: {}", key.sym);
		menu.key_down(key);
	}

	void xmbshell::add_controller(SDL_GameController* controller)
	{
	}
	void xmbshell::remove_controller(SDL_GameController* controller)
	{
	}
	void xmbshell::button_down(SDL_GameController* controller, SDL_GameControllerButton button)
	{
		spdlog::trace("Button down: {}", fmt::underlying(button));
		menu.button_down(controller, button);
	}
	void xmbshell::button_up(SDL_GameController* controller, SDL_GameControllerButton button)
	{
		spdlog::trace("Button up: {}", fmt::underlying(button));
		menu.button_up(controller, button);
	}
	void xmbshell::axis_motion(SDL_GameController* controller, SDL_GameControllerAxis axis, Sint16 value)
	{
		spdlog::trace("Axis motion: {} {}", fmt::underlying(axis), value);
		menu.axis_motion(controller, axis, value);
	}
}
