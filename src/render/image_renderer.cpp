#include "render/image_renderer.hpp"
#include "render/debug.hpp"
#include "render/utils.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "image.vert.h"
#include "image.frag.h"

namespace render {

struct push_constants {
    glm::mat4 model;
    unsigned int index;
};

image_renderer::~image_renderer() = default;

void image_renderer::preload(vk::RenderPass renderPass) {
    {
        vk::SamplerCreateInfo sampler_info({}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
            vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
            0.0f, false, 0.0f, false, vk::CompareOp::eNever, 0.0f, 0.0f, vk::BorderColor::eFloatTransparentBlack, false);
        sampler = device.createSamplerUnique(sampler_info);
    }
    {
        std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, max_images, vk::ShaderStageFlagBits::eFragment)
        };
        vk::DescriptorBindingFlags flags = vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::ePartiallyBound;
        vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingInfo(flags);
        vk::DescriptorSetLayoutCreateInfo layout_info(
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
            bindings, &bindingInfo);
        descriptorLayout = device.createDescriptorSetLayoutUnique(layout_info);
        debugName(device, descriptorLayout.get(), "Image Renderer Descriptor Layout");
    }
    {
        std::array<vk::PushConstantRange, 1> push_constant_ranges = {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(push_constants)),
        };
        vk::PipelineLayoutCreateInfo layout_info({}, descriptorLayout.get(), push_constant_ranges);
        pipelineLayout = device.createPipelineLayoutUnique(layout_info);
        debugName(device, pipelineLayout.get(), "Image Renderer Pipeline Layout");
    }

    {
        vk::UniqueShaderModule vertexShader = createShader(device, shaders_image_vert_spv, shaders_image_vert_spv_len);
        vk::UniqueShaderModule fragmentShader = createShader(device, shaders_image_frag_spv, shaders_image_frag_spv_len);
        std::array<vk::PipelineShaderStageCreateInfo, 2> shaders = {
            vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertexShader.get(), "main"),
            vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragmentShader.get(), "main")
        };

        vk::PipelineVertexInputStateCreateInfo vertex_input{};
        vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::eTriangleList);
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
            &input_assembly, &tesselation, &viewport, &rasterization, &multisample, &depthStencil, &colorBlend, &dynamic, pipelineLayout.get(), renderPass);
        pipeline = device.createGraphicsPipelineUnique({}, pipeline_info).value;
        debugName(device, pipeline.get(), "Image Renderer Pipeline");
    }
}

void image_renderer::prepare(int frameCount) {
    std::array<vk::DescriptorPoolSize, 1> sizes = {
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1*frameCount*max_images)
    };
    vk::DescriptorPoolCreateInfo pool_info(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        frameCount, sizes);
    descriptorPool = device.createDescriptorPoolUnique(pool_info);

    std::vector<vk::DescriptorSetLayout> layouts(frameCount, descriptorLayout.get());
    vk::DescriptorSetAllocateInfo set_info(descriptorPool.get(), layouts);
    descriptorSets = device.allocateDescriptorSets(set_info);

    imageInfos.resize(frameCount);
}

void image_renderer::renderImage(vk::CommandBuffer cmd, int frame, vk::ImageView view, float x, float y, float scaleX, float scaleY) {
    vk::DescriptorImageInfo image_info(sampler.get(), view, vk::ImageLayout::eShaderReadOnlyOptimal);
    int index = imageInfos[frame].size();
    imageInfos[frame].push_back(image_info);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout.get(), 0, descriptorSets[frame], {});

    push_constants push;
    push.model = glm::mat4(1.0f);
    push.model = glm::translate(push.model, glm::vec3(x, y, 0.0f));
    push.model = glm::scale(push.model, glm::vec3(scaleX / aspectRatio, scaleY, 1.0f));
    push.index = index;

    cmd.pushConstants<push_constants>(pipelineLayout.get(),
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, push);
    cmd.draw(6, 1, 0, 0);
}

void image_renderer::finish(int frame) {
    if(imageInfos[frame].empty())
        return;
    vk::WriteDescriptorSet write(
        descriptorSets[frame], 0, 0,
        imageInfos[frame].size(), vk::DescriptorType::eCombinedImageSampler, imageInfos[frame].data());
    device.updateDescriptorSets(write, {});
    imageInfos[frame].clear();
}

}
