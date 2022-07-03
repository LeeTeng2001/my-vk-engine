#include "engine.hpp"
#include "creation_helper.hpp"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <filesystem>
#include <bitset>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace fs = std::filesystem;

void Engine::initialize() {
    setRequiredFeatures();
    initBase();
    initCommand();
    initRenderPass();
    initFramebuffer();
    initSync();
    initDescriptors();
    initPipeline();
    initData();
}

void Engine::setRequiredFeatures() {
    // Required device feature
    _requiredPhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
    _requiredPhysicalDeviceFeatures.sampleRateShading = VK_TRUE;
    _requiredPhysicalDeviceFeatures.wideLines = VK_TRUE;
}

void Engine::initBase() {
    // Create window
    SDL_Init(SDL_INIT_VIDEO);
    uint32_t window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    _window = SDL_CreateWindow(
            "My Vulkan Engine",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            _windowExtent.width,
            _windowExtent.height,
            window_flags
    );

    // Initialise vulkan through bootstrap  ---------------------------------------------
    vkb::InstanceBuilder instBuilder;
    instBuilder.set_debug_callback (
            [] (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void *pUserData) -> VkBool32 {
                auto severity = vkb::to_string_message_severity(messageSeverity);
                auto type = vkb::to_string_message_type(messageType);
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[%s: %s] %s\n", severity, type, pCallbackData->pMessage);
                return VK_FALSE;
            }
    );
    auto instBuildRes = instBuilder
            .set_app_name ("Awesome Vulkan Application")
            .set_engine_name("Excellent Game Engine")
            .enable_validation_layers()
            .require_api_version(1, 3)
            .build();

    if (!instBuildRes) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Vulkan instance. Error: %s", instBuildRes.error().message().c_str());
    }

    // Store initialised instances
    vkb::Instance vkbInst = instBuildRes.value();
    _instance = vkbInst.instance;
    _debug_messenger = vkbInst.debug_messenger;

    // Surface (window handle for different os)
    if (!SDL_Vulkan_CreateSurface(_window, _instance, &_surface))
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR when trying to create SDL surface");

    // Select physical device, same pattern  ---------------------------------------------
    vkb::PhysicalDeviceSelector physSelector(vkbInst);
    auto physSelectorBuildRes = physSelector
            .set_surface(_surface)
            .set_minimum_version(1, 1)
            .set_required_features(_requiredPhysicalDeviceFeatures)
            .select();
    if (!physSelectorBuildRes) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Vulkan physical device. Error: %s", physSelectorBuildRes.error().message().c_str());
    }
    auto physDevice = physSelectorBuildRes.value();

    // Select logical device, criteria in physical device will automatically propagate to logical device creation
    vkb::DeviceBuilder deviceBuilder{physDevice};
    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Store results of physical device and logical device
    _gpu = physDevice.physical_device;
    _device = vkbDevice.device;
    _gpuProperties = physDevice.properties;

    // Get queues (bootstrap will enable one queue for each family cuz in practice one is enough)
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    _presentsQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
    _presentsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::present).value();

    if (!_graphicsQueue || !_presentsQueue) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get queue from logical device");
    }
    printPhysDeviceProps();

    // Swapchains, remember you'll need to rebuild swapchain if your window is resized ---------------------------------------------
    vkb::SwapchainBuilder swapchainBuilder{_gpu, _device, _surface };
    auto vkbSwapchainRes = swapchainBuilder
            .use_default_format_selection()  // B8G8R8A8_SRGB + SRGB non linear
            .use_default_present_mode_selection()  // mailbox, falback to fifo
            .use_default_image_usage_flags()  // Color attachment
            .set_desired_extent(_windowExtent.width, _windowExtent.height)
            .build();

    if (!vkbSwapchainRes){
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create swapchain. Error: %s", vkbSwapchainRes.error().message().c_str());
    }

    // store swapchain and its related images
    auto vkbSwapchain = vkbSwapchainRes.value();
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapChainExtent = vkbSwapchain.extent;
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
    _swapchainImageFormat = vkbSwapchain.image_format;

    // VMA Memory ---------------------------------------------
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _gpu;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    vmaCreateAllocator(&allocatorInfo, &_allocator);

    // Cleanup ---------------------------------------------
    _globCleanup.emplace([this, vkbInst, vkbDevice, vkbSwapchain](){
        vmaDestroyAllocator(_allocator);
        for (auto &_swapchainImageView : _swapchainImageViews)
            vkDestroyImageView(_device, _swapchainImageView, nullptr);

        vkb::destroy_swapchain(vkbSwapchain);
        vkb::destroy_device(vkbDevice);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_instance(vkbInst);
        SDL_DestroyWindow(_window);
    });
}

void Engine::run() {
    SDL_Event e;
    bool bQuit = false;
    while (!bQuit) {
        //Handle events on queue
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                bQuit = true;
            }
        }
        draw();
    }
}

Engine::~Engine() {
    // make sure the gpu has stopped doing its things
    // for (auto &item : _frames)
    vkWaitForFences(_device, 1, &_renderFence, true, 1000000000);
    vkDeviceWaitIdle(_device);
    while (!_globCleanup.empty()) {
        auto nextCleanup = _globCleanup.top();
        _globCleanup.pop();
        nextCleanup();
    }
}


void Engine::printPhysDeviceProps() {
    // Get info
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &queueFamilyCount, nullptr);
    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &queueFamilyCount, queueFamilies.data());

    // Print info
    SDL_Log("Selected gpu: %s", _gpuProperties.deviceName);
    SDL_Log("\tTotal MSAA color samples bits: %d", _gpuProperties.limits.framebufferColorSampleCounts);
    SDL_Log("\tTotal MSAA depth samples bits: %d", _gpuProperties.limits.framebufferDepthSampleCounts);
    SDL_Log("\tMax color attachment: %d", _gpuProperties.limits.maxColorAttachments);
    for (auto &q_family: queueFamilies)
        SDL_Log("\t-> Queue Counts: %d, Flag: %s", q_family.queueCount, std::bitset<4>(q_family.queueFlags).to_string().c_str());
    SDL_Log("\tSelected graphic queue family idx: %d", _graphicsQueueFamily);
    SDL_Log("\tSelected present queue family idx: %d", _presentsQueueFamily);
    // SDL_Log("\tMinimum buffer alignment of %lld", _gpuProperties.limits.minUniformBufferOffsetAlignment);
}

void Engine::initCommand() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = _graphicsQueueFamily;

    // create pool suitable for reusing buffer and another one for one time buffer
    if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_renderCmdPool) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create command pool");
    }
    poolInfo.flags = 0;
    if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_oneTimeCmdPool) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create command pool");
    }

    _globCleanup.emplace([this](){
        vkDestroyCommandPool(_device, _renderCmdPool, nullptr);
        vkDestroyCommandPool(_device, _oneTimeCmdPool, nullptr);
    });

    // Create command buffer for submitting rendering work
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _renderCmdPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    // they'll implicitly free up when command pool clean up
    if (vkAllocateCommandBuffers(_device, &allocInfo, &_renderCmdBuffer) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate command buffers");
    }
}

void Engine::initRenderPass() {
    // Attachment in render pass
    VkAttachmentDescription colorAttachment = CreationHelper::createVkAttDesc(
            _swapchainImageFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    vector<VkAttachmentDescription> allAttachments{
        colorAttachment
    };

    // Subpass attachments
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Subpass & dependencies
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    // wait for swap-chain to finish reading before accessing it.
    // we should wait at color attachment stage or early fragment testing stage
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = VK_ACCESS_NONE;  // relates to memory access
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // create render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = allAttachments.size();
    renderPassInfo.pAttachments = allAttachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create render pass");
    }

    _globCleanup.emplace([this](){
        vkDestroyRenderPass(_device,  _renderPass, nullptr);
    });
}

void Engine::initFramebuffer() {
    _swapChainFramebuffers.resize(_swapchainImageViews.size());

    // Base info
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = _renderPass;
    framebufferInfo.width = _swapChainExtent.width;
    framebufferInfo.height = _swapChainExtent.height;
    framebufferInfo.layers = 1;  // number of layers in image array

    // Create frame buffer for each image view in swap-chain
    for (size_t i = 0; i < _swapchainImageViews.size(); i++) {
        // Can have multiple attachment
        VkImageView attachments[] = {
                _swapchainImageViews[i]
        };

        framebufferInfo.attachmentCount = std::size(attachments);
        framebufferInfo.pAttachments = attachments;

        if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create framebuffer if index %d", i);
        }
    }

    _globCleanup.emplace([this](){
        for (const auto &item: _swapChainFramebuffers)
            vkDestroyFramebuffer(_device,  item, nullptr);
    });
}

void Engine::initSync() {
    // first fence non-blocking
    VkSemaphoreCreateInfo semInfo = CreationHelper::createSemaphoreInfo();
    VkFenceCreateInfo fenceInfo = CreationHelper::createFenceInfo(true);

    if ((vkCreateSemaphore(_device, &semInfo, nullptr, &_renderSemaphore) != VK_SUCCESS) ||
        (vkCreateSemaphore(_device, &semInfo, nullptr, &_presentSemaphore) != VK_SUCCESS) ||
        (vkCreateFence(_device, &fenceInfo, nullptr, &_renderFence) != VK_SUCCESS)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create semaphore or fence");
    }

    _globCleanup.emplace([this](){
        vkDestroySemaphore(_device, _renderSemaphore, nullptr);
        vkDestroySemaphore(_device, _presentSemaphore, nullptr);
        vkDestroyFence(_device, _renderFence, nullptr);
    });
}

void Engine::initDescriptors() {

}

void Engine::initPipeline() {
    if (!fs::is_directory("assets/shaders"))
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot find shader folder! Please check your working directory");

    // Create programmable shaders
    vector<char> vertShaderCode = CreationHelper::readFile("assets/shaders/simple_triangle.vert.spv");
    vector<char> fragShaderCode = CreationHelper::readFile("assets/shaders/simple_triangle.frag.spv");
    VkShaderModule vertShaderModule = CreationHelper::createShaderModule(vertShaderCode, _device);
    VkShaderModule fragShaderModule = CreationHelper::createShaderModule(fragShaderCode, _device);

    // shader stage info
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";  // entrypoint

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vertShaderStageInfo;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;

    // Combine programmable stages
    VkPipelineShaderStageCreateInfo shaderStages[] = {
            vertShaderStageInfo, fragShaderStageInfo
    };

    // Pipeline data to be filled
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};

    // vertex input
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescription = Vertex::getAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = bindingDescription.size();
    vertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescription.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

    // pipeline layout, specify which descriptor set to use (like uniform)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Pipeline layout fail to create!");

    // Fill in incomplete stages and create pipeline
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.stageCount = std::size(shaderStages);
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.layout = _pipelineLayout;
    pipelineCreateInfo.renderPass = _renderPass;
    pipelineCreateInfo.subpass = 0;  // index of subpass where this pipeline will be used
    CreationHelper::fillAndCreateGPipeline(pipelineCreateInfo, _graphicPipeline, _device, _swapChainExtent);

    // Free up immediate resources and delegate cleanup
    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(_device, fragShaderModule, nullptr);

    _globCleanup.emplace([this](){
        vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
        vkDestroyPipeline(_device, _graphicPipeline, nullptr);
    });
}

void Engine::initData() {
    // Buffer info
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(Vertex) * _mVertex.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Memory info
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;  // vma way of doing stuff, read doc!
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VmaAllocation allocation;  // represent underlying memory
    if (vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &_vertexBuffer, &allocation, nullptr) != VK_SUCCESS)
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot create vertex buffer!");
    _globCleanup.emplace([this, allocation](){
        vmaDestroyBuffer(_allocator, _vertexBuffer, allocation);
    });

    // Copy vertex data to staging buffer physical memory
    void* mappedData;
    vmaMapMemory(_allocator, allocation, &mappedData);
    memcpy(mappedData, _mVertex.data(), bufferInfo.size);
    vmaUnmapMemory(_allocator, allocation);

    // Buffer info
    bufferInfo.size = sizeof(_mIdx[0]) * _mIdx.size();
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    if (vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &_idxBuffer, &allocation, nullptr) != VK_SUCCESS)
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot create index buffer!");
    _globCleanup.emplace([this, allocation](){
        vmaDestroyBuffer(_allocator, _idxBuffer, allocation);
    });

    vmaMapMemory(_allocator, allocation, &mappedData);
    memcpy(mappedData, _mIdx.data(), bufferInfo.size);
    vmaUnmapMemory(_allocator, allocation);
}

void Engine::draw() {
    // Wait for previous frame to finish (takes array of fences)
    vkWaitForFences(_device, 1, &_renderFence, VK_TRUE, UINT64_MAX);

    // Get next available image
    uint32_t imageIdx;
    VkResult result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX,
                                            _presentSemaphore, VK_NULL_HANDLE, &imageIdx);

    // reset to unsignaled only if we're sure we have work to do.
    vkResetFences(_device, 1, &_renderFence);

    // Record command buffer
    vkResetCommandBuffer(_renderCmdBuffer, 0);
    recordCommandBuffer(_renderCmdBuffer, imageIdx);

    // // Update uniform buffer
    // updateUniformBuffer(currentFrame);

    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_renderCmdBuffer;

    // wait at the writing color before the image is available
    VkSemaphore waitSemaphores[] = {_presentSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = std::size(waitSemaphores);
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    // What to signal when we're done
    VkSemaphore signalSemaphores[] = {_renderSemaphore};
    submitInfo.signalSemaphoreCount = std::size(signalSemaphores);
    submitInfo.pSignalSemaphores = signalSemaphores;

    // queue submit takes
    if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _renderFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Submit result back to swap chain
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = std::size(signalSemaphores);
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {_swapchain};
    presentInfo.swapchainCount = std::size(swapchains);
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIdx;

    vkQueuePresentKHR(_presentsQueue, &presentInfo);

    // Update next frame resources indexes to use
    // currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHTS;
}

void Engine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIdx) {
    // record command into command buffer and index of current swapchina image to write into
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;  // one time / secondary / simultaneous submit
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to begin recording command buffer!");
    }

    // drawing commands
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _renderPass;
    renderPassInfo.framebuffer = _swapChainFramebuffers[imageIdx];

    // Size of render area
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = _swapChainExtent;

    // Clear value for attachment load op clear
    // Should have the same order as attachments binding index
    VkClearValue clearColor[1] = {};
    clearColor[0].color = {{0.2f, 0.2f, 0.5f, 1.0f}};
    renderPassInfo.clearValueCount = std::size(clearColor);
    renderPassInfo.pClearValues = clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    // RENDER START ----------------------------------------------------
    // Bind components
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicPipeline);
    VkBuffer vertexBuffers[] = {_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, std::size(vertexBuffers), vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _idxBuffer, 0, VK_INDEX_TYPE_UINT32);
    // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
    //                         0, 1, &_descriptorSets[currentFrame], 0, nullptr);

    // Issue draw
    vkCmdDrawIndexed(commandBuffer, _mIdx.size(), 1, 0, 0, 0);

    // RENDER END --------------------------------------------------------
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to end record command buffer!");
    }
}
