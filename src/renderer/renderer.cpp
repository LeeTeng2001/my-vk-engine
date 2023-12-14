#include <SDL.h>
#include <SDL_vulkan.h>
#include <bitset>
#include <imgui.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_sdl3.h>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

#include "renderer.hpp"
#include "utils/common.hpp"
#include "creation_helper.hpp"
#include "builder.hpp"


bool Renderer::initialise(RenderConfig renderConfig) {
    auto l = SLog::get();
    _renderConf = renderConfig;
    setRequiredFeatures();

    if (!validateConfig()) { l->error("failed to validate config"); return false; };
    if (!initBase()) { l->error("failed to initialise base"); return false; };
    if (!initCommand()) { l->error("failed to initialise command"); return false; };
    if (!initRenderResources()) { l->error("failed to create render resources"); return false; };
    if (!initDescriptors()) { l->error("failed to create descriptors"); return false; };
    if (!initRenderPass()) { l->error("failed to create renderpass"); return false; };
    initFramebuffer();
    initSync();
    initPipeline();
    if (!initImGUI()) { l->error("failed to create imgui"); return false; };
    initCamera();

    return true;
}

void Renderer::setRequiredFeatures() {
    // Required device feature
    _requiredPhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
    _requiredPhysicalDeviceFeatures.sampleRateShading = VK_TRUE;
#ifndef __APPLE__
    _requiredPhysicalDeviceFeatures.wideLines = VK_TRUE;
#endif
    _requiredPhysicalDeviceFeatures.shaderInt64 = VK_TRUE;
}

bool Renderer::validateConfig() {
    // TODO: implement me
    return true;
}

bool Renderer::initBase() {
    auto l = SLog::get();
    // Create SDL window
    SDL_Init(SDL_INIT_VIDEO);
    uint32_t window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    _window = SDL_CreateWindow("Luna's Vulkan Engine", _renderConf.windowWidth, _renderConf.windowHeight, window_flags);

    // Initialise vulkan through bootstrap  ---------------------------------------------
    vkb::InstanceBuilder instBuilder;
    instBuilder.set_debug_callback(
            [] (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void *pUserData) -> VkBool32 {
//                auto severity = vkb::to_string_message_severity(messageSeverity);
//                auto type = vkb::to_string_message_type(messageType);
                auto l2 = SLog::get();
                switch (messageSeverity) {
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                    l2->info(fmt::format("vkCallback: {:s}\n", pCallbackData->pMessage));
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                    l2->warn(fmt::format("vkCallback: {:s}\n", pCallbackData->pMessage));
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                    l2->error(fmt::format("vkCallback: {:s}\n", pCallbackData->pMessage));
                    break;
                default:
                    l2->warn(fmt::format("vkCallback unrecognised level: {:s}\n", pCallbackData->pMessage));
                }
                return VK_FALSE;
            }
    );
//    instBuilder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
    instBuilder.set_debug_messenger_severity(_renderConf.callbackSeverity);
    auto instBuildRes = instBuilder
            .set_app_name("Luna Vulkan Engine")
            .set_engine_name("Luna Engine")
            .enable_validation_layers()
            .require_api_version(1, 3)
            .build();

    if (!instBuildRes) {
        l->error(fmt::format("Failed to create Vulkan instance. Error: {:s}", instBuildRes.error().message().c_str()));
        return false;
    }

    // Store initialised instances
    vkb::Instance vkbInst = instBuildRes.value();
    _instance = vkbInst.instance;
    // _debug_messenger = vkbInst.debug_messenger;

    // Surface (window handle for different os)
    if (!SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface)) {
        l->error("failed to create SDL surface");
        return false;
    }

    // Select physical device, same pattern  ---------------------------------------------
    vkb::PhysicalDeviceSelector physSelector(vkbInst);
    auto physSelectorBuildRes = physSelector
            .set_surface(_surface)
            .set_minimum_version(1, 1)
            .set_required_features(_requiredPhysicalDeviceFeatures)
            .select();
    if (!physSelectorBuildRes) {
        l->error(fmt::format("Failed to create Vulkan physical device. Error: {:s}", physSelectorBuildRes.error().message().c_str()));
        return false;
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
    _transferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
    _transferQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();
    _presentsQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
    _presentsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::present).value();

    if (!_graphicsQueue || !_presentsQueue || !_transferQueue) {
        l->error("Failed to get queue from logical device");
        return false;
    }

    // Swapchains, remember you'll need to rebuild swapchain if your window is resized ---------------------------------------------
    vkb::SwapchainBuilder swapchainBuilder{_gpu, _device, _surface };
    auto vkbSwapchainRes = swapchainBuilder
            .use_default_format_selection()  // B8G8R8A8_SRGB + SRGB non linear
            .use_default_present_mode_selection()  // mailbox, falback to fifo
            .use_default_image_usage_flags()  // Color attachment
            .build();

    if (!vkbSwapchainRes){
        l->error(fmt::format("Failed to create swapchain. Error: {:s}", vkbSwapchainRes.error().message().c_str()));
        return false;
    }

    // store swapchain and its related images
    auto vkbSwapchain = vkbSwapchainRes.value();
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapChainExtent = vkbSwapchain.extent;
    // TODO: Fix extent, should query SDL: https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
    _swapchainImageFormat = vkbSwapchain.image_format;

    // VMA Memory ---------------------------------------------
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _gpu;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    vmaCreateAllocator(&allocatorInfo, &_allocator);

    printPhysDeviceProps();

    for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
        _flightResources.push_back(new FlightResource);
    }

    // Cleanup ---------------------------------------------
    _globCleanup.emplace([this, vkbInst, vkbDevice, vkbSwapchain](){
        for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
            delete _flightResources[i];
        }
        _flightResources.clear();

        vmaDestroyAllocator(_allocator);
        for (auto &_swapchainImageView : _swapchainImageViews)
            vkDestroyImageView(_device, _swapchainImageView, nullptr);

        vkb::destroy_swapchain(vkbSwapchain);
        vkb::destroy_device(vkbDevice);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_instance(vkbInst);
        SDL_DestroyWindow(_window);
    });

    return true;
}

void Renderer::newFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

Renderer::~Renderer() {
    // make sure the gpu has stopped doing its things
    // for (auto &item : _frames)
    vkWaitForFences(_device, 1, &_renderFence, true, 1000000000);
    vkDeviceWaitIdle(_device);
    while (!_interCleanup.empty()) {
        auto nextCleanup = _interCleanup.top();
        _interCleanup.pop();
        nextCleanup();
    }
    while (!_globCleanup.empty()) {
        auto nextCleanup = _globCleanup.top();
        _globCleanup.pop();
        nextCleanup();
    }
}

void Renderer::printPhysDeviceProps() {
    auto l = SLog::get();
    // Get info
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &queueFamilyCount, nullptr);
    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &queueFamilyCount, queueFamilies.data());

    // Print info
    l->debug(fmt::format("Selected gpu: {:s}", _gpuProperties.deviceName));
    l->debug(fmt::format("\tTotal MSAA color samples bits: {:d}", _gpuProperties.limits.framebufferColorSampleCounts));
    l->debug(fmt::format("\tTotal MSAA depth samples bits: {:d}", _gpuProperties.limits.framebufferDepthSampleCounts));
    l->debug(fmt::format("\tMax color attachment: {:d}", _gpuProperties.limits.maxColorAttachments));
    l->debug(fmt::format("\tMax push constant size: {:d}", _gpuProperties.limits.maxPushConstantsSize));
    for (auto &q_family: queueFamilies) {
        l->debug(fmt::format("\t-> Queue Counts: {:d}, Flag: {:s}", q_family.queueCount,
                             std::bitset<4>(q_family.queueFlags).to_string().c_str()));
    }
    l->debug(fmt::format("\tSelected graphic queue family idx: {:d}", _graphicsQueueFamily));
    l->debug(fmt::format("\tSelected present queue family idx: {:d}", _presentsQueueFamily));
    l->debug(fmt::format("\tWindow extent: {:d}, {:d}", _renderConf.windowWidth, _renderConf.windowWidth));
    l->debug(fmt::format("\tSwapchain extent: {:d}, {:d}", _swapChainExtent.width, _swapChainExtent.height));
    l->debug(fmt::format("\tSwapchain image counts: {:d}", _swapchainImages.size()));
    // SDL_Log("\tMinimum buffer alignment of %lld", _gpuProperties.limits.minUniformBufferOffsetAlignment);
}

bool Renderer::initCommand() {
    auto l = SLog::get();

    // Create one time pool and long time pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = _graphicsQueueFamily;

    // create pool suitable for reusing buffer and another one for one time buffer
    if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_renderCmdPool) != VK_SUCCESS) {
        l->error("Failed to create command pool");
        return false;
    }
    poolInfo.flags = 0;
    if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_oneTimeCmdPool) != VK_SUCCESS) {
        l->error("Failed to create one time command pool");
        return false;
    }

    // Command buffer is implicitly deleted when pool is destroyed
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
    for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
        if (vkAllocateCommandBuffers(_device, &allocInfo, &_flightResources[i]->mrtCmdBuffer) != VK_SUCCESS) {
            l->error("Failed to allocate mrt command buffers");
            return false;
        }
        if (vkAllocateCommandBuffers(_device, &allocInfo, &_flightResources[i]->compCmdBuffer) != VK_SUCCESS) {
            l->error("Failed to allocate composition command buffers");
            return false;
        }
    }

    return true;
}

bool Renderer::initRenderPass() {
    // Create MRT render pass ------------------------------------------------------------------d
    // Attachment in render pass
    VkAttachmentDescription colorAttachment = CreationHelper::createVkAttDesc(
            _swapchainImageFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkAttachmentDescription normalAttachment = CreationHelper::createVkAttDesc(
            _swapchainImageFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkAttachmentDescription depthAttachment = CreationHelper::createVkAttDesc(
            _depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    vector<VkAttachmentDescription> allAttachments{
        colorAttachment, normalAttachment, depthAttachment
    };

    // Subpass attachments
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference normalAttachmentRef{};
    normalAttachmentRef.attachment = 1;
    normalAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 2;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Subpass & dependencies
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    VkAttachmentReference attachmentsRef[] = {colorAttachmentRef, normalAttachmentRef};
    subpass.colorAttachmentCount = std::size(attachmentsRef);
    subpass.pColorAttachments = attachmentsRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency inDependency{};
    inDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    inDependency.dstSubpass = 0;
    // wait for swap-chain to finish reading before accessing it.
    // we should wait at color attachment stage or early fragment testing stage
    inDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    inDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    inDependency.srcAccessMask = VK_ACCESS_NONE;  // relates to memory access
    inDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency depthDependency{};
    depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDependency.dstSubpass = 0;
    depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.srcAccessMask = VK_ACCESS_NONE;  // relates to memory access
    depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // transition to read only for composition render pass
    VkSubpassDependency outDependency{};
    outDependency.srcSubpass = 0;
    outDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
    outDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    outDependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    outDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    outDependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    vector<VkSubpassDependency > allDependencies{
            inDependency, depthDependency, outDependency
    };

    // create render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = allAttachments.size();
    renderPassInfo.pAttachments = allAttachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = allDependencies.size();
    renderPassInfo.pDependencies = allDependencies.data();

    if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_mrtRenderPass) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create render pass");
    }

    _globCleanup.emplace([this](){
        vkDestroyRenderPass(_device, _mrtRenderPass, nullptr);
    });

    // Create Composition render pass ------------------------------------------------------------------
    // Attachments
    VkAttachmentDescription presentAttachment = CreationHelper::createVkAttDesc(
            _swapchainImageFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    allAttachments = {presentAttachment};

    // Subpass attachments
    std::array<VkAttachmentReference, 1> colorAtt = {};
    colorAtt[0] = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    // Subpass & dependencies
    subpass.colorAttachmentCount = std::size(colorAtt);
    subpass.pColorAttachments = colorAtt.data();
    subpass.pDepthStencilAttachment = nullptr;

    // Create render pass
    renderPassInfo.attachmentCount = allAttachments.size();
    renderPassInfo.pAttachments = allAttachments.data();
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = nullptr;

    if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_compositionRenderPass) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create composition render pass");
    }

    _globCleanup.emplace([this](){
        vkDestroyRenderPass(_device,  _compositionRenderPass, nullptr);
    });
}

bool Renderer::initRenderResources() {
    auto l = SLog::get();

    _depthFormat = VK_FORMAT_D32_SFLOAT;  // TODO: Should query! not hardcode
    // TODO: optiomise G buffer structure, for example RGB the A component is unused in normal map
    // https://computergraphics.stackexchange.com/questions/4969/how-much-precision-do-i-need-in-my-g-buffer
    // MRT: depth, albedo, normal
    std::array<ImgResource, 3> imgInfoList = {};
    imgInfoList[0].format = _depthFormat;
    imgInfoList[0].usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imgInfoList[0].extent = _swapChainExtent;
    imgInfoList[0].aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    imgInfoList[1].format = _swapchainImageFormat;
    imgInfoList[1].usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imgInfoList[1].extent = _swapChainExtent;
    imgInfoList[1].aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    imgInfoList[2].format = _swapchainImageFormat;
    imgInfoList[2].usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imgInfoList[2].extent = _swapChainExtent;
    imgInfoList[2].aspect = VK_IMAGE_ASPECT_COLOR_BIT;

    // allocate from GPU LOCAL memory
    VmaAllocationCreateInfo localAllocInfo {};
    localAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    localAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
        for (const auto &imgInfo: imgInfoList) {
            auto newImgResource = new ImgResource(imgInfo);

            // create image, image view, sampler for each of the resources
            VkImageCreateInfo createImgInfo = CreationHelper::imageCreateInfo(newImgResource->format,
                                                                              newImgResource->usage,
                                                                              newImgResource->extent);
            l->vk_res(vmaCreateImage(_allocator, &createImgInfo, &localAllocInfo,
                                     &newImgResource->image, &newImgResource->allocation, nullptr));
            VkImageViewCreateInfo createImgViewInfo = CreationHelper::imageViewCreateInfo(newImgResource->format,
                                                                                          newImgResource->image,
                                                                                          newImgResource->aspect);
            l->vk_res(vkCreateImageView(_device, &createImgViewInfo, nullptr, &newImgResource->imageView));

            // TODO: assume all resources uses clamp to edge in MRT
            VkSamplerCreateInfo createSampInfo = CreationHelper::samplerCreateInfo(_gpu,
                                                                                   VK_FILTER_LINEAR,
                                                                                   VK_FILTER_LINEAR,
                                                                                   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
            l->vk_res(vkCreateSampler(_device, &createSampInfo, nullptr, &newImgResource->sampler));

            _flightResources[i]->imgResourceList.push_back(newImgResource);
        }
    }

    _globCleanup.emplace([this](){
        for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
            for (const auto &imgInfo: _flightResources[i]->imgResourceList) {
                vkDestroySampler(_device, imgInfo->sampler, nullptr);
                vkDestroyImageView(_device, imgInfo->imageView, nullptr);
                vmaDestroyImage(_allocator, imgInfo->image, imgInfo->allocation);
            }
        }
    });

    return true;
}

void Renderer::initFramebuffer() {
    auto l = SLog::get();
    // swapchain frame buffer
    _swapChainFramebuffers.resize(_swapchainImageViews.size());

    // Base info
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = _compositionRenderPass;
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

        l->vk_res(vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_swapChainFramebuffers[i]));
    }

    // mrt framebuffer
    framebufferInfo.renderPass = _mrtRenderPass;
    VkImageView attachments[] = {_colorImageView, _normalImageView, _depthImageView};
    framebufferInfo.attachmentCount = std::size(attachments);
    framebufferInfo.pAttachments = attachments;
    l->vk_res(vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_mrtFramebuffer));

    _globCleanup.emplace([this](){
        for (const auto &item: _swapChainFramebuffers)
            vkDestroyFramebuffer(_device,  item, nullptr);
        vkDestroyFramebuffer(_device, _mrtFramebuffer, nullptr);
    });
}

void Renderer::initSync() {
    // first fence non-blocking
    VkSemaphoreCreateInfo semInfo = CreationHelper::createSemaphoreInfo();
    VkFenceCreateInfo fenceInfo = CreationHelper::createFenceInfo(true);

    if ((vkCreateSemaphore(_device, &semInfo, nullptr, &_renderMrtSemaphore) != VK_SUCCESS) ||
        (vkCreateSemaphore(_device, &semInfo, nullptr, &_presentSemaphore) != VK_SUCCESS) ||
        (vkCreateSemaphore(_device, &semInfo, nullptr, &_compSemaphore) != VK_SUCCESS) ||
        (vkCreateFence(_device, &fenceInfo, nullptr, &_renderFence) != VK_SUCCESS)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create semaphore or fence");
    }

    _globCleanup.emplace([this](){
        vkDestroySemaphore(_device, _renderMrtSemaphore, nullptr);
        vkDestroySemaphore(_device, _presentSemaphore, nullptr);
        vkDestroyFence(_device, _renderFence, nullptr);
    });
}

bool Renderer::initDescriptors() {
    // Creat pool for binding resources
    std::vector<VkDescriptorPoolSize> sizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 20 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20 },
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = 10;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes = sizes.data();
    vkCreateDescriptorPool(_device, &pool_info, nullptr, &_globalDescPool);

    // MRT description layout and set ------------------------------------------------------------------------
    // describe binding in single set
    DescriptorBuilder mrtSetBuilder(_device, _globalDescPool);
    mrtSetBuilder.setTotalSet(1).pushDefaultFragmentSamplerBinding(0);
    _mrtSetLayout = mrtSetBuilder.buildSetLayout(0);

    // TODO: Rn we skip MRT resource
//    for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
//        mrtSetBuilder.clearSetWrite(0);
//
//        _flightResources[i].
//    }

    // Composition description layout and set ------------------------------------------------------------------------
    DescriptorBuilder compSetBuilder(_device, _globalDescPool);
    compSetBuilder.setTotalSet(1);
    for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
        compSetBuilder.pushDefaultFragmentSamplerBinding(0);
    }
    _compSetLayout = compSetBuilder.buildSetLayout(0);

    // create descriptor set
    for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
        compSetBuilder.clearSetWrite(0);

        for (const auto &imgResouce: _flightResources[i]->imgResourceList) {
            compSetBuilder.pushSetWriteImgSampler(0, imgResouce->imageView, imgResouce->sampler);
        }
        _flightResources[i]->mrtDescSet = compSetBuilder.buildSet(0);
    }

    _globCleanup.emplace([this]() {
        vkDestroyDescriptorPool(_device, _globalDescPool, nullptr);
        vkDestroyDescriptorSetLayout(_device, _mrtSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(_device, _compSetLayout, nullptr);
    });
}

void Renderer::initPipeline() {
    if (!fs::is_directory("assets/shaders")) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot find shader folder! Please check your working directory");
    }

    // MRT Pipeline ------------------------------------------------------------

    // Create programmable shaders
    vector<char> mrtVertShaderCode = CreationHelper::readFile("assets/shaders/mrt.vert.spv");
    vector<char> mrtFragShaderCode = CreationHelper::readFile("assets/shaders/mrt.frag.spv");
    VkShaderModule mrtVertShaderModule = CreationHelper::createShaderModule(mrtVertShaderCode, _device);
    VkShaderModule mrtFragShaderModule = CreationHelper::createShaderModule(mrtFragShaderCode, _device);

    // shader stage info
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = mrtVertShaderModule;
    vertShaderStageInfo.pName = "main";  // entrypoint

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vertShaderStageInfo;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = mrtFragShaderModule;

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

    // Push constant
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.size = sizeof(MrtPushConstantData);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;  // best to share directly

    // pipeline layout, specify which descriptor set to use (like uniform)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_mrtSetLayout;

    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_mrtPipelineLayout) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Pipeline layout fail to create!");
    }

    // Fill in incomplete stages and create pipeline
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.stageCount = std::size(shaderStages);
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.layout = _mrtPipelineLayout;
    pipelineCreateInfo.renderPass = _mrtRenderPass;
    pipelineCreateInfo.subpass = 0;  // index of subpass where this pipeline will be used
    CreationHelper::fillAndCreateGPipeline(pipelineCreateInfo, _mrtPipeline, _device, _swapChainExtent, 2);

    // Free up immediate resources and delegate cleanup
    vkDestroyShaderModule(_device, mrtVertShaderModule, nullptr);
    vkDestroyShaderModule(_device, mrtFragShaderModule, nullptr);

    // Composition pipeline --------------------------------------------------------

    vector<char> compVertShaderCode = CreationHelper::readFile("assets/shaders/composition.vert.spv");
    vector<char> compFragShaderCode = CreationHelper::readFile("assets/shaders/composition.frag.spv");
    VkShaderModule compVertShaderModule = CreationHelper::createShaderModule(compVertShaderCode, _device);
    VkShaderModule compFragShaderModule = CreationHelper::createShaderModule(compFragShaderCode, _device);

    vertShaderStageInfo.module = compVertShaderModule;
    fragShaderStageInfo.module = compFragShaderModule;
    shaderStages[0] = vertShaderStageInfo;
    shaderStages[1] = fragShaderStageInfo;

    // pipeline layout info
    pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_compSetLayout;

    // Push constant
    pushConstantRange.size = sizeof(CompPushConstantData);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;  // best to share directly
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_compPipelineLayout) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Pipeline layout fail to create!");
    }

    // empty vertex info
    vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // fill in pipeline create info
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.stageCount = std::size(shaderStages);
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.layout = _compPipelineLayout;
    pipelineCreateInfo.renderPass = _compositionRenderPass;
    pipelineCreateInfo.subpass = 0;  // index of subpass where this pipeline will be used
    CreationHelper::fillAndCreateGPipeline(pipelineCreateInfo, _compPipeline, _device, _swapChainExtent, 1);

    _globCleanup.emplace([this](){
        vkDestroyPipelineLayout(_device, _mrtPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _mrtPipeline, nullptr);
        vkDestroyPipelineLayout(_device, _compPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _compPipeline, nullptr);
    });
}

bool Renderer::uploadAndPopulateModal(ModelData& modelData) {
    auto l = SLog::get();
    // VMA best usage info: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
    // Buffer info
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(Vertex) * modelData.vertex.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    l->debug(fmt::format("copy vertex buffer to gpu (size: {:d}, total: {:d}, indices: {:d})",
                         sizeof(Vertex), modelData.vertex.size(), modelData.indices.size()));

    // Memory info, must have VMA_ALLOCATION_CREATE_MAPPED_BIT to create persistent map area so we can avoid map / unmap memory
    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;  // vma way texBufferInfo doing stuff, read doc!
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    stagingAllocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // to avoid flushing

    VmaAllocation stagingAllocation;  // represent underlying memory
    VmaAllocationInfo stagingAllocationInfo; // for persistent mapping: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html
    l->vk_res(vmaCreateBuffer(_allocator, &bufferInfo, &stagingAllocInfo, &modelData.vBuffer, &stagingAllocation, &stagingAllocationInfo));
    _globCleanup.emplace([this, stagingAllocation, &modelData](){
        vmaDestroyBuffer(_allocator, modelData.vBuffer, stagingAllocation);
    });

    // Copy vertex data to staging buffer physical memory
    memcpy(stagingAllocationInfo.pMappedData, modelData.vertex.data(), bufferInfo.size);
    // TODO: Check stagingAllocationInfo.memoryType for VK_MEMORY_PROPERTY_HOST_COHERENT_BIT to avoid flushing all
    // https://stackoverflow.com/questions/44366839/meaning-of-memorytypes-in-vulkan
    vmaFlushAllocation(_allocator, stagingAllocation, 0, bufferInfo.size); // TODO: Transfer to destination buffer
    // Flush for VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html

    // Buffer info
    bufferInfo.size = sizeof(modelData.indices[0]) * modelData.indices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    l->vk_res(vmaCreateBuffer(_allocator, &bufferInfo, &stagingAllocInfo, &modelData.iBuffer, &stagingAllocation, &stagingAllocationInfo));
    _globCleanup.emplace([this, stagingAllocation, &modelData](){
        vmaDestroyBuffer(_allocator, modelData.iBuffer, stagingAllocation);
    });

    memcpy(stagingAllocationInfo.pMappedData, modelData.indices.data(), bufferInfo.size);
    vmaFlushAllocation(_allocator, stagingAllocation, 0, bufferInfo.size); // TODO: Transfer to destination buffer

    // Use staging buffer to transfer texture data to local memory
    if (modelData.albedoTexture.stbRef != nullptr) {
        l->debug(fmt::format("upload albedo texture dim: ({:d}, {:d}, {:d})",
                             modelData.albedoTexture.texWidth, modelData.albedoTexture.texHeight,
                             modelData.albedoTexture.texChannels));

        // Create buffer for transfer source image
        VkBuffer texBuffer;
        VkBufferCreateInfo texBufferInfo = bufferInfo;
        texBufferInfo.size = modelData.albedoTexture.texWidth * modelData.albedoTexture.texHeight * modelData.albedoTexture.texChannels;
        texBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        l->vk_res(vmaCreateBuffer(_allocator, &texBufferInfo, &stagingAllocInfo, &texBuffer, &stagingAllocation, &stagingAllocationInfo));
        memcpy(stagingAllocationInfo.pMappedData, modelData.albedoTexture.stbRef, texBufferInfo.size);
        vmaFlushAllocation(_allocator, stagingAllocation, 0, texBufferInfo.size); // TODO: Transfer to destination buffer

        // Create GPU local sampled image
        // TODO: Query format?
        VkExtent2D ext{static_cast<uint32_t>(modelData.albedoTexture.texWidth), static_cast<uint32_t>(modelData.albedoTexture.texHeight)};
        VkImageCreateInfo textureImageInfo = CreationHelper::imageCreateInfo(VK_FORMAT_R8G8B8A8_SRGB,
                                                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                             ext);
        // allocate from GPU LOCAL memory
        VmaAllocationCreateInfo texAllocInfo {};
        texAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        texAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkResult result = vmaCreateImage(_allocator, &textureImageInfo, &texAllocInfo,
                                         &modelData.gpuAlbedoTex.image, &modelData.gpuAlbedoTex.allocation, nullptr);
        l->vk_res(result);

        // transition the layout image for layout destination
        transitionImgLayout(modelData.gpuAlbedoTex.image, VK_FORMAT_R8G8B8A8_SRGB,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // perform copy operation
        copyBufferToImg(texBuffer, modelData.gpuAlbedoTex.image, ext);

        // make it optimal for sampling
        transitionImgLayout(modelData.gpuAlbedoTex.image, VK_FORMAT_R8G8B8A8_SRGB,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // staging buffer can be destroyed as src textures are no longer used
        vmaDestroyBuffer(_allocator, texBuffer, stagingAllocation);

        // image view and sampler
        VkImageView imgView;
        VkImageViewCreateInfo createImgViewInfo = CreationHelper::imageViewCreateInfo(VK_FORMAT_R8G8B8A8_SRGB,
                                                                                      modelData.gpuAlbedoTex.image,
                                                                                      VK_IMAGE_ASPECT_COLOR_BIT);
        l->vk_res(vkCreateImageView(_device, &createImgViewInfo, nullptr, &modelData.gpuAlbedoTex.imageView));
        VkSamplerCreateInfo createSampInfo = CreationHelper::samplerCreateInfo(_gpu,
                                                                               VK_FILTER_LINEAR,
                                                                               VK_FILTER_LINEAR,
                                                                               VK_SAMPLER_ADDRESS_MODE_REPEAT);
        l->vk_res(vkCreateSampler(_device, &createSampInfo, nullptr, &modelData.gpuAlbedoTex.sampler));

        _globCleanup.emplace([this, &modelData]() {
            vkDestroySampler(_device, modelData.gpuAlbedoTex.sampler, nullptr);
            vkDestroyImageView(_device, modelData.gpuAlbedoTex.imageView, nullptr);
            vmaDestroyImage(_allocator, modelData.gpuAlbedoTex.image, modelData.gpuAlbedoTex.allocation);
        });
    }

}

bool Renderer::initImGUI() {
    auto l = SLog::get();

    // 1: Create Descriptor pool for ImGUI
    // the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize pool_sizes[] =
            {
                    {VK_DESCRIPTOR_TYPE_SAMPLER,                400},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 400},
                    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          400},
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          400},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   400},
                    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   400},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         400},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         400},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 400},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 400},
                    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       400}
            };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 400;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
    if (vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool) != VK_SUCCESS) {
        l->error("Failed to create imgui descriptor pool");
        return false;
    }

    // 2: initialize imgui library
    // initializes the core structures of imgui + SDL
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(_window);

    // initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = _instance;
    initInfo.PhysicalDevice = _gpu;
    initInfo.Device = _device;
    initInfo.Queue = _graphicsQueue;
    initInfo.DescriptorPool = imguiPool;
    initInfo.MinImageCount = _swapchainImageViews.size();
    initInfo.ImageCount = _swapchainImageViews.size();
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo, _compositionRenderPass);
    //execute a gpu command to upload imgui font textures
    ImGui_ImplVulkan_CreateFontsTexture();

    // add the destroy the imgui created structures
    _globCleanup.emplace([this, imguiPool]() {
        ImGui_ImplVulkan_DestroyFontsTexture();
        vkDestroyDescriptorPool(_device, imguiPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
    });

    return true;
}

void Renderer::initCamera() {
    cam = new Camera(1980, 1080, 1, 100, 60);
}

void Renderer::draw() {
    // Draw imgui
//    ImGui::ShowDemoWindow();
    ImGui::Text("World coord: up +y, right +x, forward +z");
    ImGui::Text("Cam Pos    : %s", glm::to_string(cam->GetCamPos()).c_str());

    auto l = SLog::get();
    // Wait for previous frame to finish (takes array of fences)
    vkWaitForFences(_device, 1, &_renderFence, VK_TRUE, UINT64_MAX);

    // Get next available image
    uint32_t imageIdx;
    VkResult result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX,
                                            _presentSemaphore, VK_NULL_HANDLE, &imageIdx);
    l->vk_res(result);

    // reset to unsignaled only if we're sure we have work to do.
    vkResetFences(_device, 1, &_renderFence);

    // Record command buffer
    vkResetCommandBuffer(_mrtCmdBuffer, 0);
    recordMrtCommandBuffer(_mrtCmdBuffer);
    vkResetCommandBuffer(_compCmdBuffer, 0);
    recordCompCommandBuffer(_compCmdBuffer, imageIdx);

    // // Update uniform buffer
    // updateUniformBuffer(currentFrame);

    // sync primitive
    VkSemaphore mrtWaitSem[] = {_presentSemaphore};
    VkSemaphore mrtSignalSem[] = {_renderMrtSemaphore};
    VkSemaphore compSignalSem[] = {_compSemaphore};

    // Submit MRT ---------------------------------------------------
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_mrtCmdBuffer;

    // wait at the writing color before the image is available
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = std::size(mrtWaitSem);
    submitInfo.pWaitSemaphores = mrtWaitSem;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = std::size(mrtSignalSem);
    submitInfo.pSignalSemaphores = mrtSignalSem;

    l->vk_res(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

    // Submit Composition ---------------------------------------------------
    submitInfo.pCommandBuffers = &_compCmdBuffer;
    submitInfo.waitSemaphoreCount = std::size(mrtSignalSem);
    submitInfo.pWaitSemaphores = mrtSignalSem;
    submitInfo.signalSemaphoreCount = std::size(compSignalSem);
    submitInfo.pSignalSemaphores = compSignalSem;
    l->vk_res(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _renderFence));

    // Submit result back to swap chain
    // What to signal when we're done
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = std::size(compSignalSem);
    presentInfo.pWaitSemaphores = compSignalSem;

    VkSwapchainKHR swapchains[] = {_swapchain};
    presentInfo.swapchainCount = std::size(swapchains);
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIdx;

    vkQueuePresentKHR(_presentsQueue, &presentInfo);

    // Update next frame resources indexes to use
    // currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHTS;
}

void Renderer::recordMrtCommandBuffer(VkCommandBuffer commandBuffer) {
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
    renderPassInfo.renderPass = _mrtRenderPass;
    renderPassInfo.framebuffer = _mrtFramebuffer;

    // Size of render area
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = _swapChainExtent;

    // Clear value for attachment load op clear
    // Should have the same order as attachments binding index
    VkClearValue clearColor[3] = {};
    clearColor[0].color = {{0.2f, 0.2f, 0.5f, 1.0f}}; // bg for color frame attach
    clearColor[1].color = {{0.0f, 0.0f, 0.0f, 1.0f}}; // bg for color frame attach
    clearColor[2].depthStencil.depth = 1.0f; // depth attachment
    renderPassInfo.clearValueCount = std::size(clearColor);
    renderPassInfo.pClearValues = clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    // RENDER START ----------------------------------------------------
    // Bind components
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _mrtPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _mrtPipelineLayout,
                            0, 1, &_mrtDescSet, 0, nullptr);
    VkBuffer vertexBuffers[] = {_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, std::size(vertexBuffers), vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _idxBuffer, 0, VK_INDEX_TYPE_UINT32);

    // Push constant
    MrtPushConstantData pushConstantData{};
    pushConstantData.time = SDL_GetTicks();
//    pushConstantData.viewTransform = cam->GetOrthographicTransformMatrix();
    pushConstantData.viewTransform = cam->GetPerspectiveTransformMatrix();
    pushConstantData.sunPos = cam->GetCamPos();
    vkCmdPushConstants(commandBuffer, _mrtPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(MrtPushConstantData), &pushConstantData);

    // Issue draw index
    vkCmdDrawIndexed(commandBuffer, _mIdx.size(), 1, 0, 0, 0);

    // RENDER END --------------------------------------------------------
    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to end record command buffer!");
    }
}

void Renderer::recordCompCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIdx) {
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
    renderPassInfo.renderPass = _compositionRenderPass;
    renderPassInfo.framebuffer = _swapChainFramebuffers[imageIdx];

    // Size of render area
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = _swapChainExtent;

    // don't clear attachment cuz we're accessing them

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    // RENDER START ----------------------------------------------------
    // Bind components
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _compPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _compPipelineLayout,
                            0, 1, &_compDescSet, 0, nullptr);


    // Push constant
    CompPushConstantData pushConstantData{};
    pushConstantData.sobelWidth = 1;
    pushConstantData.sobelHeight = 1;
    vkCmdPushConstants(commandBuffer, _compPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(CompPushConstantData), &pushConstantData);

    // Issue draw a single triangle that covers full screen
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // ImGUI Draw as last call
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    // RENDER END --------------------------------------------------------
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to end record command buffer!");
    }
}

void Renderer::execOneTimeCmd(const std::function<void(VkCommandBuffer)> &function) {
    auto l = SLog::get();
    // One time sync fence
    VkFenceCreateInfo fenceInfo = CreationHelper::createFenceInfo(false);
    VkFence oneTimeFence;
    l->vk_res(vkCreateFence(_device, &fenceInfo, nullptr, &oneTimeFence));

    // Create new command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = _oneTimeCmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    // Immediately record command buffer (one time)
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // execute the function
    function(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    // Execute command buffer to transfer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, oneTimeFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit command buffer!");
    }

    // Wait operation to complete
    vkWaitForFences(_device, 1, &oneTimeFence, true, 1000000000);

    // Free one time resources
    vkFreeCommandBuffers(_device, _oneTimeCmdPool, 1, &commandBuffer);
    vkDestroyFence(_device, oneTimeFence, nullptr);
}

void Renderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    auto func = [=](VkCommandBuffer cmdBuf) {
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(cmdBuf, srcBuffer, dstBuffer, 1, &copyRegion);
    };
    execOneTimeCmd(func);
}

void Renderer::copyBufferToImg(VkBuffer srcBuffer, VkImage dstImg, VkExtent2D extent) {
    auto func = [=](VkCommandBuffer cmdBuf) {
        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;

        copyRegion.imageOffset = {0, 0, 0};
        copyRegion.imageExtent = {extent.width, extent.height, 1};
        vkCmdCopyBufferToImage(cmdBuf, srcBuffer, dstImg,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
    };
    execOneTimeCmd(func);
}

void Renderer::transitionImgLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    // We need to handle two kind of layout transformation:
    // 1. Undefined → transferDst: transfer writes that don't need to wait on anything
    // 2. transferDst → shaderRead: shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's where we're going to use the texture
    // TODO: should we transit depth image?

    auto func = [=](VkCommandBuffer cmdBuf) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        // We're not transferring queue ownership
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // this is not default value!
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        // determine access stage & mask based on source and destination
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destStage;
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            barrier.srcAccessMask = VK_ACCESS_NONE;
            destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(cmdBuf, sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    };
    execOneTimeCmd(func);
}



