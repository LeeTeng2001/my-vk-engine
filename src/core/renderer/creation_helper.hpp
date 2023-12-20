#pragma once

#include "utils/common.hpp"
#include <fstream>

class CreationHelper {
public:
    static VkAttachmentDescription createVkAttDesc(VkFormat format, VkImageLayout finalLayout) {
        VkAttachmentDescription attachment{};
        attachment.format = format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;  // todo; customise
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // what to do with the data when load?
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // we're not using stencil
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = finalLayout; // msaa final image

        return attachment;
    }

    static VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent2D extent) {
        VkImageCreateInfo info {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = format;
        info.extent = {
                extent.width,
                extent.height,
                1,
        };

        info.mipLevels = 1;  // todo: customisable?
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT; // todo; customise for color attachment
        info.tiling = VK_IMAGE_TILING_OPTIMAL; // not possible to read the image without transforming, but optimal for access
        info.usage = usageFlags;

        return info;
    }

    static VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo info {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.image = image;
        info.format = format;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        info.subresourceRange.aspectMask = aspectFlags;

        return info;
    }

    static VkSamplerCreateInfo samplerCreateInfo(VkPhysicalDevice device, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = magFilter;
        samplerInfo.minFilter = minFilter;

        samplerInfo.addressModeU = addressMode;
        samplerInfo.addressModeV = addressMode;
        samplerInfo.addressModeW = addressMode;

        // get max anisotropy by querying physical device
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device, &properties);
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE; // coordinate? always use [0, 1] for our case

        // mainly for shadow map, skipping
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        return samplerInfo;
    }


    static VkRenderPassBeginInfo createRenderPassBeginInfo(VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D extent) {
        VkRenderPassBeginInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        createInfo.renderPass = renderPass;
        createInfo.framebuffer = framebuffer;
        createInfo.renderArea.offset = {0, 0};
        createInfo.renderArea.extent = extent;

        return createInfo;
    }

    static VkFenceCreateInfo createFenceInfo(bool initSignalOn = false) {
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        // we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
        if (initSignalOn) fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        return fenceCreateInfo;
    }

    static VkSemaphoreCreateInfo createSemaphoreInfo() {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.flags = 0;

        return semaphoreCreateInfo;
    }

    static VmaAllocationCreateInfo createStagingAllocInfo() {
        VmaAllocationCreateInfo stagingAllocInfo{};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;  // vma way texBufferInfo doing stuff, read doc!
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        stagingAllocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // to avoid flushing
        return stagingAllocInfo;
    }

    static VkResult createUniformBuffer(VmaAllocator allocator, VkDeviceSize bufSize,
                                              VkBuffer &outBuf, VmaAllocation &outAlloc, VmaAllocationInfo &outAllocInfo) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo createAllocInfo{};
        createAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        createAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                VMA_ALLOCATION_CREATE_MAPPED_BIT;
        createAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // to avoid flushing

        return vmaCreateBuffer(allocator, &bufferInfo, &createAllocInfo, &outBuf, &outAlloc, &outAllocInfo);
    }

    static vector<char> readFile(const string &filename) {
        std::ifstream f(filename, std::ios::ate | std::ios::binary);

        if (!f.is_open()) {
            string errMsg = "Failed to open f: " + filename;
            throw std::runtime_error(errMsg);
        }

        size_t fileSize = (size_t) f.tellg();
        std::vector<char> buffer(fileSize);
        f.seekg(0);
        f.read(buffer.data(), fileSize);

        return buffer;
    }

    static VkShaderModule createShaderModule(const vector<char> &code, VkDevice device) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }


    static void fillAndCreateGPipeline(VkGraphicsPipelineCreateInfo &pipelineCreateInfo, VkPipeline &graphicPipeline,
                                       VkDevice device, VkExtent2D viewportExtend, int colorAttachmentCount) {
        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;  // for indexed drawing

        // Viewport and scissors
        VkViewport viewport{};  // origin top left
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) viewportExtend.width;
        viewport.height = (float) viewportExtend.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = viewportExtend; // don't cut any

        // Combine viewport and scissor together
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;  // clamp point instead of discarding
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;  // how to rasterise?
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;  // define cull
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;  // define front face
        rasterizer.depthBiasEnable = VK_FALSE;  // disable depth bias
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;  // AA for texture interior
        multisampling.minSampleShading = 0; // min fraction for sample shading; closer to one is smooth
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;  // define multi sampling sample counts
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // Depth and stencil
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;  // pixel should be discarded?
        depthStencil.depthWriteEnable = VK_TRUE;  // write new depth?
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;  // lower depth nearer.
        depthStencil.depthBoundsTestEnable = VK_FALSE;  // only keep fragment in range
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        // Color blending (per attachment)
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        vector<VkPipelineColorBlendAttachmentState> cbAttachVec{};
        for (int i = 0; i < colorAttachmentCount; ++i) {
            cbAttachVec.push_back(colorBlendAttachment);
        }

        // Color blending (global)
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;  // enable bitwise combination
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = cbAttachVec.size();
        colorBlending.pAttachments = cbAttachVec.data();
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // Dynamic state to update without recreating the pipeline.
        vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_LINE_WIDTH
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = dynamicStates.size();
        dynamicState.pDynamicStates = dynamicStates.data();

        // Finally, build graphic pipeline
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pRasterizationState = &rasterizer;
        pipelineCreateInfo.pMultisampleState = &multisampling;
        pipelineCreateInfo.pDepthStencilState = &depthStencil; // Optional
        pipelineCreateInfo.pColorBlendState = &colorBlending;
        pipelineCreateInfo.pDynamicState = nullptr; // Optional

        // You can create new pipeline by deriving from existing pipeline.
        // Need to set VK_PIPELINE_CREATE_DERIVATIVE_BIT flag.
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineCreateInfo.basePipelineIndex = -1; // Optional

        // You can pass in multiple VkPipeline to create multiple pipeline in single call.
        // 2nd arg is pipeline cache (reuse relevant data across pipeline creation (multiple graphic pipeline call))
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicPipeline) != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Graphic pipeline fail to create!");
        }
    }
};
