#include "render/phase.hpp"
#include "render/window.hpp"

namespace render
{
	phase::phase(window* window) : 
		win(window),
		instance(window->instance.get()), device(window->device.get()), 
		allocator(window->allocator), loader(window->loader.get()),
		graphicsQueue(window->graphicsQueue), graphicsFamily(window->queueFamilyIndices.graphicsFamily.value())
	{

	}

	void phase::preload()
	{

	}

	void phase::prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews)
	{
		
	}

	void phase::waitLoad()
	{
		for(auto& f : loadingFutures)
		{
			f.wait();
		}
	}

	void phase::init()
	{

	}

	void phase::render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence)
	{
		
	}

	phase::~phase()
	{

	}
}