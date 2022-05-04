#include "render/app/render_shell.hpp"

#include "render/debug.hpp"
#include "render/utils.hpp"

namespace render
{
	render_shell::render_shell(window* window) : phase(window)
	{
	}

	void render_shell::preload()
	{
		FT_Library ft;
		FT_Error err = FT_Init_FreeType(&ft);
		font = std::make_unique<font_renderer>("/usr/share/fonts/liberation/LiberationSans-Regular.ttf", 128, device, allocator);
		
		pool = device.createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsFamily));

		vk::AttachmentDescription attachment({}, win->swapchainFormat.format, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
		vk::AttachmentReference ref(0, vk::ImageLayout::eColorAttachmentOptimal);
		vk::SubpassDependency dep(VK_SUBPASS_EXTERNAL, 0, 
			vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, 
			{}, vk::AccessFlagBits::eColorAttachmentWrite);
		vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref);
		vk::RenderPassCreateInfo renderpass_info({}, attachment, subpass, dep);
		renderPass = device.createRenderPassUnique(renderpass_info);
		debugName(device, renderPass.get(), "Shell Render Pass");

		font->preload(ft, loader, renderPass.get());
	}

	void render_shell::prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews)
	{
		commandBuffers = device.allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(pool.get(), vk::CommandBufferLevel::ePrimary, swapchainImages.size()));
		this->swapchainImages = swapchainImages;

		for(int i=0; i<swapchainViews.size(); i++)
		{
			vk::FramebufferCreateInfo framebuffer_info({}, renderPass.get(), swapchainViews[i],
				win->swapchainExtent.width, win->swapchainExtent.height, 1);
			framebuffers.push_back(device.createFramebufferUnique(framebuffer_info));
			debugName(device, framebuffers.back().get(), "Loading Screen Framebuffer #"+std::to_string(i));

			debugName(device, commandBuffers[i].get(), "Loading Screen Command Buffer #"+std::to_string(i));
		}
		font->prepare(swapchainViews.size());
	}

	void render_shell::render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence)
	{
		vk::UniqueCommandBuffer& commandBuffer = commandBuffers[frame];

		commandBuffer->begin(vk::CommandBufferBeginInfo());

		vk::ClearValue color(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});
		commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(renderPass.get(), framebuffers[frame].get(), 
			vk::Rect2D({0, 0}, win->swapchainExtent), color), vk::SubpassContents::eInline);

		vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
		vk::Rect2D scissor({0,0}, win->swapchainExtent);
		commandBuffer->setViewport(0, viewport);
		commandBuffer->setScissor(0, scissor);

		font->renderText(commandBuffer.get(), frame, "FPS: "+std::to_string(win->currentFPS), 0, 0, 0.05f, glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
		font->renderText(commandBuffer.get(), frame, "Hello Cynder :D", 1, 0, 0.1f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		font->finish(frame);
		
		commandBuffer->endRenderPass();
		commandBuffer->end();

		vk::PipelineStageFlags waitFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::SubmitInfo submit_info(imageAvailable, waitFlags, commandBuffer.get(), renderFinished);
		graphicsQueue.submit(submit_info, fence);
	}
}