#include "app/xmbshell.hpp"

#include "config.hpp"
#include "render/debug.hpp"
#include "render/utils.hpp"

#include "blur.comp.h"

#include <chrono>

namespace app
{
	xmbshell::xmbshell(window* window) : phase(window)
	{
	}

	xmbshell::~xmbshell()
	{
		for(unsigned int i = 0; i<renderImages.size(); i++)
		{
			allocator.destroyImage(renderImages[i], renderAllocations[i]);
		}
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
					vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral),
				vk::AttachmentDescription({}, win->swapchainFormat.format, vk::SampleCountFlagBits::e1,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral)
			};
			vk::AttachmentReference ref(0, vk::ImageLayout::eColorAttachmentOptimal);
			vk::AttachmentReference rref(1, vk::ImageLayout::eColorAttachmentOptimal);
			vk::SubpassDependency dep(VK_SUBPASS_EXTERNAL, 0,
				vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				{}, vk::AccessFlagBits::eColorAttachmentWrite);
			vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref, rref);
			vk::RenderPassCreateInfo renderpass_info({}, attachments, subpass, dep);
			backgroundRenderPass = device.createRenderPassUnique(renderpass_info);
			debugName(device, backgroundRenderPass.get(), "Background Render Pass");
		}
		{
			std::array<vk::AttachmentDescription, 2> attachments = {
				vk::AttachmentDescription({}, win->swapchainFormat.format, config::CONFIG.sampleCount,
					vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eGeneral, vk::ImageLayout::eColorAttachmentOptimal),
				vk::AttachmentDescription({}, win->swapchainFormat.format, vk::SampleCountFlagBits::e1,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral)
			};
			vk::AttachmentReference ref(0, vk::ImageLayout::eColorAttachmentOptimal);
			vk::SubpassDependency dep(VK_SUBPASS_EXTERNAL, 0,
				vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				{}, vk::AccessFlagBits::eColorAttachmentWrite);
			vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref);
			vk::RenderPassCreateInfo renderpass_info({}, attachments, subpass, dep);
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
			vk::SubpassDependency dep(VK_SUBPASS_EXTERNAL, 0,
				vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				{}, vk::AccessFlagBits::eColorAttachmentWrite);
			vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref, rref);
			vk::RenderPassCreateInfo renderpass_info({}, attachments, subpass, dep);
			popupRenderPass = device.createRenderPassUnique(renderpass_info);
			debugName(device, popupRenderPass.get(), "Popup Render Pass");
		}
		{
			std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
				vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute),
				vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute),
			};
			vk::DescriptorSetLayoutCreateInfo info({}, bindings);
			blurDescriptorSetLayout = device.createDescriptorSetLayoutUnique(info);
		}
		{
			vk::PipelineLayoutCreateInfo info({}, blurDescriptorSetLayout.get(), {});
			blurPipelineLayout = device.createPipelineLayoutUnique(info);
		}
		{
			vk::UniqueShaderModule compShader = createShader(device, shaders_blur_comp);
			vk::PipelineShaderStageCreateInfo shader({}, vk::ShaderStageFlagBits::eCompute, compShader.get(), "main");
			vk::ComputePipelineCreateInfo info({}, shader, blurPipelineLayout.get());
			blurPipeline = device.createComputePipelineUnique({}, info).value;
			debugName(device, blurPipeline.get(), "Blur Pipeline");
		}

		FT_Library ft;
		FT_Error err = FT_Init_FreeType(&ft);

		font_render->preload(ft, loader, shellRenderPass.get());
		image_render->preload(shellRenderPass.get());
		wave_render->preload(backgroundRenderPass.get());
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
			vk::DescriptorPoolSize size(vk::DescriptorType::eStorageImage, 2*imageCount);
			vk::DescriptorPoolCreateInfo pool_info({}, imageCount, size);
			blurDescriptorPool = device.createDescriptorPoolUnique(pool_info);

			std::vector<vk::DescriptorSetLayout> layouts(imageCount, blurDescriptorSetLayout.get());
			vk::DescriptorSetAllocateInfo alloc_info(blurDescriptorPool.get(), layouts);
			blurDescriptorSets = device.allocateDescriptorSets(alloc_info);
		}
		this->swapchainImages = swapchainImages;

		renderImages.resize(imageCount);
		renderAllocations.resize(imageCount);
		renderViews.resize(imageCount);
		blurSrcViews.resize(imageCount);
		blurDstViews.resize(imageCount);

		std::vector<vk::DescriptorImageInfo> imageInfos(2*imageCount);
		std::vector<vk::WriteDescriptorSet> writes(imageCount);

		for(int i=0; i<imageCount; i++)
		{
			vk::ImageCreateInfo image_info(vk::ImageCreateFlagBits::eExtendedUsage | vk::ImageCreateFlagBits::eMutableFormat,
				vk::ImageType::e2D, win->swapchainFormat.format,
				vk::Extent3D(win->swapchainExtent, 1), 1, 1, config::CONFIG.sampleCount,
				vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage,
				vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined);
			vma::AllocationCreateInfo alloc_info({}, vma::MemoryUsage::eGpuOnly);
			std::tie(renderImages[i], renderAllocations[i]) = allocator.createImage(image_info, alloc_info);
			debugName(device, renderImages[i], "XMB Shell Render Image #"+std::to_string(i));

			vk::ImageViewUsageCreateInfoKHR usage_info(vk::ImageUsageFlagBits::eColorAttachment);
			vk::ImageViewCreateInfo view_info({}, renderImages[i], vk::ImageViewType::e2D, win->swapchainFormat.format,
				vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
				&usage_info);
			renderViews[i] = device.createImageViewUnique(view_info);
			debugName(device, renderViews[i].get(), "XMB Shell Render Image View #"+std::to_string(i));

			std::array<vk::ImageView, 2> attachments = {renderViews[i].get(), swapchainViews[i]};
			vk::FramebufferCreateInfo framebuffer_info({}, shellRenderPass.get(), attachments,
				win->swapchainExtent.width, win->swapchainExtent.height, 1);
			framebuffers.push_back(device.createFramebufferUnique(framebuffer_info));
			debugName(device, framebuffers.back().get(), "XMB Shell Framebuffer #"+std::to_string(i));

			debugName(device, commandBuffers[i].get(), "XMB Shell Command Buffer #"+std::to_string(i));

			{
				vk::ImageViewUsageCreateInfoKHR usage_info(vk::ImageUsageFlagBits::eStorage);
				vk::ImageViewCreateInfo blur_src_view_info({}, swapchainImages[i], vk::ImageViewType::e2D, win->swapchainComputeFormat.format,
					vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
					&usage_info);
				blurSrcViews[i] = device.createImageViewUnique(blur_src_view_info);
				debugName(device, renderViews[i].get(), "XMB Shell Blur Src View #"+std::to_string(i));
			}
			{
				vk::ImageViewUsageCreateInfoKHR usage_info(vk::ImageUsageFlagBits::eStorage);
				vk::ImageViewCreateInfo blur_dst_view_info({}, renderImages[i], vk::ImageViewType::e2D, win->swapchainComputeFormat.format,
					vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
					&usage_info);
				blurDstViews[i] = device.createImageViewUnique(blur_dst_view_info);
				debugName(device, renderViews[i].get(), "XMB Shell Blur Dst View #"+std::to_string(i));
			}

			imageInfos[2*i] = vk::DescriptorImageInfo({}, blurSrcViews[i].get(), vk::ImageLayout::eGeneral);
			imageInfos[2*i+1] = vk::DescriptorImageInfo({}, blurDstViews[i].get(), vk::ImageLayout::eGeneral);
			writes[i] = vk::WriteDescriptorSet(blurDescriptorSets[i], 0, 0, 2, vk::DescriptorType::eStorageImage,
				&imageInfos[2*i]);
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
			commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(backgroundRenderPass.get(), framebuffers[frame].get(),
				vk::Rect2D({0, 0}, win->swapchainExtent), color), vk::SubpassContents::eInline);
			vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
			vk::Rect2D scissor({0,0}, win->swapchainExtent);
			commandBuffer->setViewport(0, viewport);
			commandBuffer->setScissor(0, scissor);

			if(config::CONFIG.backgroundType == config::config::background_type::wave) {
				wave_render->render(commandBuffer.get(), frame);
			}
			else if(config::CONFIG.backgroundType == config::config::background_type::image) {
				image_render->renderImageSized(commandBuffer.get(), frame, *backgroundTexture,
					0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height);
			}

			commandBuffer->endRenderPass();
		}
		{
			if(blur_background) {
				commandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, blurPipeline.get());
				commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, blurPipelineLayout.get(), 0, blurDescriptorSets[frame], {});
				commandBuffer->dispatch(win->swapchainExtent.width/16, win->swapchainExtent.height/16, 1);
			}
		}
		{
			commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(shellRenderPass.get(), framebuffers[frame].get(),
				vk::Rect2D({0, 0}, win->swapchainExtent)), vk::SubpassContents::eInline);
			vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
			vk::Rect2D scissor({0,0}, win->swapchainExtent);
			commandBuffer->setViewport(0, viewport);
			commandBuffer->setScissor(0, scissor);

			gui_renderer ctx(commandBuffer.get(), frame, win->swapchainExtent, font_render.get(), image_render.get());
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

			gui_renderer ctx(commandBuffer.get(), frame, win->swapchainExtent, font_render.get(), image_render.get());
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
