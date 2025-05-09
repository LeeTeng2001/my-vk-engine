#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"
#include <bitset>
#include "imgui.h"
#include "tiny_obj_loader.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_sdl3.h"
#include "vk_mem_alloc.h"
#include "VkBootstrap.h"

#include "renderer.hpp"
#include "utils/common.hpp"
#include "creation_helper.hpp"
#include "builder.hpp"

namespace luna {

bool Renderer::initialise(RenderConfig renderConfig) {
    auto l = SLog::get();
    _renderConf = renderConfig;
    setRequiredFeatures();

    if (!validate()) {
        l->error("failed to validate");
        return false;
    };
    if (!initBase()) {
        l->error("failed to initialise base");
        return false;
    };
    if (!initCommand()) {
        l->error("failed to initialise command");
        return false;
    };
    if (!initBuffer()) {
        l->error("failed to initialise buffer");
        return false;
    };
    if (!initRenderResources()) {
        l->error("failed to create render resources");
        return false;
    };
    if (!initDescriptors()) {
        l->error("failed to create descriptors");
        return false;
    };
    if (!initSync()) {
        l->error("failed to create sync structure");
        return false;
    };
    if (!initPipeline()) {
        l->error("failed to create pipeline");
        return false;
    };
    if (!initImGUI()) {
        l->error("failed to create imgui");
        return false;
    };
    if (!initPreApp()) {
        l->error("failed to init preapp");
        return false;
    };

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

bool Renderer::validate() {
    auto l = SLog::get();
    l->debug("validating render config");

    if (_renderConf.maxFrameInFlight < 1) {
        l->error("max frame in flight cannot be less than 1");
        return false;
    }
    if (_renderConf.maxFrameInFlight > 3) {
        l->error("max frame in flight cannot be > 3");
        return false;
    }

    return true;
}

bool Renderer::initBase() {
    auto l = SLog::get();
    l->debug("initialising base");

    // Create SDL window
    SDL_Init(SDL_INIT_VIDEO);
    uint32_t window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    _window = SDL_CreateWindow("Luna's Vulkan Engine", _renderConf.windowWidth,
                               _renderConf.windowHeight, window_flags);

    // Initialise vulkan through bootstrap  ---------------------------------------------
    vkb::InstanceBuilder instBuilder;
    instBuilder.set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                      VkDebugUtilsMessageTypeFlagsEXT messageType,
                                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                      void *pUserData) -> VkBool32 {
        auto l2 = SLog::get();
        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                l2->debug(fmt::format("vkCallback: {:s}\n", pCallbackData->pMessage));
                break;
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
                l2->warn(
                    fmt::format("vkCallback unrecognised level: {:s}\n", pCallbackData->pMessage));
        }
        return VK_FALSE;
    });

    auto enableValidation = false;
#ifdef DEBUG
    instBuilder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
    enableValidation = true;
#endif

    instBuilder.set_debug_messenger_severity(_renderConf.callbackSeverity);
    auto instBuildRes = instBuilder.set_app_name("Luna Vulkan Engine")
                            .set_engine_name("Luna Engine")
                            .enable_validation_layers(enableValidation)
                            .require_api_version(1, 3)
                            .build();

    if (!instBuildRes) {
        l->error(fmt::format("Failed to create Vulkan instance. Error: {:s}",
                             instBuildRes.error().message().c_str()));
        return false;
    }

    // Store initialised instances
    vkb::Instance vkbInst = instBuildRes.value();
    _instance = vkbInst.instance;
    l->info(fmt::format(
        "Vulkan instance created, instance version: {:d}.{:d}, api version: {:d}.{:d}",
        VK_API_VERSION_MAJOR(vkbInst.instance_version),
        VK_API_VERSION_MINOR(vkbInst.instance_version), VK_API_VERSION_MAJOR(vkbInst.api_version),
        VK_API_VERSION_MINOR(vkbInst.api_version)));

    // Surface (window handle for different os)
    if (!SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface)) {
        l->error("failed to create SDL surface");
        return false;
    }

    // Select physical device, same pattern  ---------------------------------------------
    vkb::PhysicalDeviceSelector physSelector(vkbInst);
    auto physSelectorBuildRes = physSelector.set_surface(_surface)
                                    .set_minimum_version(1, 3)
                                    .set_required_features(_requiredPhysicalDeviceFeatures)
                                    .add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
                                    .select();
    if (!physSelectorBuildRes) {
        l->error(fmt::format("Failed to create Vulkan physical device. Error: {:s}",
                             physSelectorBuildRes.error().message().c_str()));
        return false;
    }
    auto physDevice = physSelectorBuildRes.value();

    // dynamic rendering struct
    VkPhysicalDeviceDynamicRenderingFeatures dynRenderFeature{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .dynamicRendering = VK_TRUE,
    };
    VkPhysicalDeviceSynchronization2Features sync2Feature{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .synchronization2 = VK_TRUE,
    };

    // Select logical device, criteria in physical device will automatically propagate to logical
    // device creation
    vkb::DeviceBuilder deviceBuilder{physDevice};
    vkb::Device vkbDevice =
        deviceBuilder.add_pNext(&dynRenderFeature).add_pNext(&sync2Feature).build().value();

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
        l->error("Failed to get queue from logical device");
        return false;
    }

    // Check
    l->info(fmt::format("required pushc size ({:d}, {:d}), hardware limit ({:d})",
                        sizeof(MrtPushConstantData), sizeof(CompPushConstantData),
                        _gpuProperties.limits.maxPushConstantsSize));
    l->info(fmt::format("mrt ubo size ({:d}), composition ubo size ({:d})", sizeof(MrtUboData),
                        sizeof(CompUboData)));

    // Swapchains, remember you'll need to rebuild swapchain if your window is resized
    // ---------------------------------------------
    vkb::SwapchainBuilder swapchainBuilder{_gpu, _device, _surface};
    auto vkbSwapchainRes = swapchainBuilder
                               .use_default_format_selection()  // B8G8R8A8_SRGB + SRGB non linear
                               .use_default_present_mode_selection()  // mailbox, falback to fifo
                               .use_default_image_usage_flags()       // Color attachment
                               .build();

    if (!vkbSwapchainRes) {
        l->error(fmt::format("Failed to create swapchain. Error: {:s}",
                             vkbSwapchainRes.error().message().c_str()));
        return false;
    }

    // store swapchain and its related images
    auto vkbSwapchain = vkbSwapchainRes.value();
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapChainExtent = vkbSwapchain.extent;
    // TODO: Fix extent, should query SDL:
    // https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain
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
    _globCleanup.emplace([this, vkbInst, vkbDevice, vkbSwapchain]() {
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

void Renderer::shutdown() {
    // make sure the gpu has stopped doing its things
    // for (auto &item : _frames)
    for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
        vkWaitForFences(_device, 1, &_flightResources[i]->renderFence, true, 1000000000);
    }
    vkDeviceWaitIdle(_device);

    // Imgui
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

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
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &queueFamilyCount, queueFamilies.data());

    // Print info
    l->debug(fmt::format("Selected gpu: {:s}", _gpuProperties.deviceName));
    l->debug(fmt::format("\tTotal MSAA color samples bits: {:d}",
                         _gpuProperties.limits.framebufferColorSampleCounts));
    l->debug(fmt::format("\tTotal MSAA depth samples bits: {:d}",
                         _gpuProperties.limits.framebufferDepthSampleCounts));
    l->debug(
        fmt::format("\tMax color attachment: {:d}", _gpuProperties.limits.maxColorAttachments));
    l->debug(
        fmt::format("\tMax push constant size: {:d}", _gpuProperties.limits.maxPushConstantsSize));
    for (auto &q_family : queueFamilies) {
        l->debug(fmt::format("\t-> Queue Counts: {:d}, Flag: {:s}", q_family.queueCount,
                             std::bitset<4>(q_family.queueFlags).to_string().c_str()));
    }
    l->debug(fmt::format("\tSelected graphic queue family idx: {:d}", _graphicsQueueFamily));
    l->debug(fmt::format("\tSelected present queue family idx: {:d}", _presentsQueueFamily));
    l->debug(fmt::format("\tWindow extent: {:d}, {:d}", _renderConf.windowWidth,
                         _renderConf.windowWidth));
    l->debug(fmt::format("\tSwapchain extent: {:d}, {:d}", _swapChainExtent.width,
                         _swapChainExtent.height));
    l->debug(fmt::format("\tSwapchain image counts: {:d}", _swapchainImages.size()));
    // SDL_Log("\tMinimum buffer alignment of %lld",
    // _gpuProperties.limits.minUniformBufferOffsetAlignment);
}

bool Renderer::initCommand() {
    auto l = SLog::get();
    l->debug("initialising command buffer");

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
    _globCleanup.emplace([this]() {
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
        if (vkAllocateCommandBuffers(_device, &allocInfo, &_flightResources[i]->mrtCmdBuffer) !=
            VK_SUCCESS) {
            l->error("Failed to allocate mrt command buffers");
            return false;
        }
        if (vkAllocateCommandBuffers(_device, &allocInfo, &_flightResources[i]->compCmdBuffer) !=
            VK_SUCCESS) {
            l->error("Failed to allocate composition command buffers");
            return false;
        }
    }

    return true;
}

bool Renderer::initBuffer() {
    auto l = SLog::get();
    l->debug("initialising uniform buffer");

    for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
        VkBuffer buf;
        VmaAllocation alloc;
        // comp
        l->vk_res(CreationHelper::createUniformBuffer(_allocator, sizeof(CompUboData), buf, alloc,
                                                      _flightResources[i]->compUniformAllocInfo));
        _flightResources[i]->compUniformBuffer = buf;
        _globCleanup.emplace([this, buf, alloc]() { vmaDestroyBuffer(_allocator, buf, alloc); });
    }

    return true;
}

bool Renderer::initRenderResources() {
    auto l = SLog::get();
    l->debug("initialising render resources");

    _depthFormat = VK_FORMAT_D32_SFLOAT;  // TODO: Should query! not hardcode
    // TODO: optiomise G buffer structure, for example RGB the A component is unused in normal map
    // https://computergraphics.stackexchange.com/questions/4969/how-much-precision-do-i-need-in-my-g-buffer
    // one of the unused A channel can be used to store specular/bump highlight
    // MRT: depth, albedo, normal, position, normal.a with position.a forms uv
    _imgInfoList[0].format = _depthFormat;
    _imgInfoList[0].usage =
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    _imgInfoList[0].extent = _swapChainExtent;
    _imgInfoList[0].aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    _imgInfoList[0].clearValue = {
        .depthStencil =
            {
                .depth = 1.0f,
            },
    };

    _imgInfoList[1].format = VK_FORMAT_R8G8B8A8_UNORM;
    _imgInfoList[1].usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    _imgInfoList[1].extent = _swapChainExtent;
    _imgInfoList[1].aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    _imgInfoList[1].clearValue = {.color = {{0.8f, 0.6f, 0.4f, 1.0f}}};

    _imgInfoList[2].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    _imgInfoList[2].usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    _imgInfoList[2].extent = _swapChainExtent;
    _imgInfoList[2].aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    _imgInfoList[2].clearValue = {.color = {{0, 0, 0, 1.0f}}};

    _imgInfoList[3].format =
        VK_FORMAT_R16G16B16A16_SFLOAT;  // TODO: position requires high precision
    _imgInfoList[3].usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    _imgInfoList[3].extent = _swapChainExtent;
    _imgInfoList[3].aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    _imgInfoList[3].clearValue = {.color = {{0, 0, 0, 1.0f}}};

    // allocate from GPU LOCAL memory
    VmaAllocationCreateInfo localAllocInfo{};
    localAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    localAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
        for (const auto &imgInfo : _imgInfoList) {
            auto newImgResource = imgInfo;

            // create image, image view, sampler for each of the resources
            VkImageCreateInfo createImgInfo = CreationHelper::imageCreateInfo(
                newImgResource.format, newImgResource.usage, newImgResource.extent);
            l->vk_res(vmaCreateImage(_allocator, &createImgInfo, &localAllocInfo,
                                     &newImgResource.image, &newImgResource.allocation, nullptr));
            VkImageViewCreateInfo createImgViewInfo = CreationHelper::imageViewCreateInfo(
                newImgResource.format, newImgResource.image, newImgResource.aspect);
            l->vk_res(
                vkCreateImageView(_device, &createImgViewInfo, nullptr, &newImgResource.imageView));

            // TODO: assume all resources uses clamp to edge in MRT
            VkSamplerCreateInfo createSampInfo = CreationHelper::samplerCreateInfo(
                _gpu, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
            l->vk_res(vkCreateSampler(_device, &createSampInfo, nullptr, &newImgResource.sampler));

            _flightResources[i]->compImgResourceList.push_back(newImgResource);
        }
    }

    _globCleanup.emplace([this]() {
        for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
            for (const auto &imgInfo : _flightResources[i]->compImgResourceList) {
                vkDestroySampler(_device, imgInfo.sampler, nullptr);
                vkDestroyImageView(_device, imgInfo.imageView, nullptr);
                vmaDestroyImage(_allocator, imgInfo.image, imgInfo.allocation);
            }
        }
    });

    return true;
}

bool Renderer::initSync() {
    auto l = SLog::get();
    l->debug("initialising sync structures");

    // first fence non-blocking
    VkFenceCreateInfo fenceInfo = CreationHelper::createFenceInfo(true);
    VkSemaphoreCreateInfo semInfo = CreationHelper::createSemaphoreInfo();

    for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
        if (vkCreateFence(_device, &fenceInfo, nullptr, &_flightResources[i]->renderFence) !=
            VK_SUCCESS) {
            l->error("failed to create fence");
            return false;
        }
        if ((vkCreateSemaphore(_device, &semInfo, nullptr, &_flightResources[i]->compSemaphore) !=
             VK_SUCCESS) ||
            (vkCreateSemaphore(_device, &semInfo, nullptr, &_flightResources[i]->mrtSemaphore) !=
             VK_SUCCESS) ||
            (vkCreateSemaphore(_device, &semInfo, nullptr,
                               &_flightResources[i]->imageAvailableSem) != VK_SUCCESS)) {
            l->error("failed to create semaphore");
            return false;
        }
    }

    _globCleanup.emplace([this]() {
        for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
            vkDestroyFence(_device, _flightResources[i]->renderFence, nullptr);
            vkDestroySemaphore(_device, _flightResources[i]->mrtSemaphore, nullptr);
            vkDestroySemaphore(_device, _flightResources[i]->compSemaphore, nullptr);
            vkDestroySemaphore(_device, _flightResources[i]->imageAvailableSem, nullptr);
        }
    });

    return true;
}

bool Renderer::initDescriptors() {
    auto l = SLog::get();
    l->debug("initialising descriptors");

    // Creat pool for binding resources
    std::vector<VkDescriptorPoolSize> sizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 200},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 200},
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = 200;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes = sizes.data();
    vkCreateDescriptorPool(_device, &pool_info, nullptr, &_globalDescPool);

    // MRT description layout and set
    // ------------------------------------------------------------------------ describe binding in
    // single set
    l->debug("init mrt set resource");
    DescriptorBuilder mrtSetBuilder(_device, _globalDescPool);
    mrtSetBuilder.setTotalSet(1);
    mrtSetBuilder.pushDefaultUniform(0, VK_SHADER_STAGE_FRAGMENT_BIT);
    for (int i = 0; i < MRT_SAMPLE_SIZE; ++i) {
        mrtSetBuilder.pushDefaultFragmentSamplerBinding(0);
    }
    _mrtSetLayout = mrtSetBuilder.buildSetLayout(0);
    // resources are handled by application

    // Composition description layout and set
    // ------------------------------------------------------------------------
    l->debug("init comp set resource");
    DescriptorBuilder compSetBuilder(_device, _globalDescPool);
    compSetBuilder.setTotalSet(2);
    for (int i = 0; i < MRT_OUT_SIZE; ++i) {
        compSetBuilder.pushDefaultFragmentSamplerBinding(0);
    }
    compSetBuilder.pushDefaultUniform(1, VK_SHADER_STAGE_FRAGMENT_BIT);
    _compSetLayoutList.push_back(compSetBuilder.buildSetLayout(0));
    _compSetLayoutList.push_back(compSetBuilder.buildSetLayout(1));

    // create descriptor set
    for (int i = 0; i < _renderConf.maxFrameInFlight; ++i) {
        compSetBuilder.clearSetWrite();

        // bind set 0 MRT sampler
        for (const auto &imgResouce : _flightResources[i]->compImgResourceList) {
            compSetBuilder.pushSetWriteImgSampler(0, imgResouce.imageView, imgResouce.sampler);
        }
        // bind set 1 uniform
        compSetBuilder.pushSetWriteUniform(1, _flightResources[i]->compUniformBuffer,
                                           sizeof(CompUboData));
        _flightResources[i]->compDescSetList.push_back(compSetBuilder.buildSet(0));
        _flightResources[i]->compDescSetList.push_back(compSetBuilder.buildSet(1));
    }

    _globCleanup.emplace([this]() {
        vkDestroyDescriptorPool(_device, _globalDescPool, nullptr);
        vkDestroyDescriptorSetLayout(_device, _mrtSetLayout, nullptr);
        for (const auto &item : _compSetLayoutList) {
            vkDestroyDescriptorSetLayout(_device, item, nullptr);
        }
    });

    return true;
}

bool Renderer::initPipeline() {
    auto l = SLog::get();
    l->debug("initialising pipeline");

    if (!fs::is_directory("assets/shaders")) {
        l->error("Cannot find shader folder! Please check your working directory");
        return false;
    }

    // MRT Pipeline ------------------------------------------------------------

    // Create programmable shaders
    std::vector<char> mrtVertShaderCode = CreationHelper::readFile("assets/shaders/mrt.vert.spv");
    std::vector<char> mrtFragShaderCode = CreationHelper::readFile("assets/shaders/mrt.frag.spv");
    VkShaderModule mrtVertShaderModule =
        CreationHelper::createShaderModule(mrtVertShaderCode, _device);
    VkShaderModule mrtFragShaderModule =
        CreationHelper::createShaderModule(mrtFragShaderCode, _device);

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
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

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
    pushConstantRange.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;  // best to share directly

    // pipeline layout, specify which descriptor set to use (like uniform)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_mrtSetLayout;

    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_mrtPipelineLayout) !=
        VK_SUCCESS) {
        l->error("failed to create mrt pipeline layout");
        return false;
    }

    // replaced render pass with dynamic rendering
    // thus we need to provide this structure
    // TODO: we're getting the first one fromt the list, plus this iteration is dirty
    std::vector<VkFormat> colorFormats;
    for (const auto &imgRes : _flightResources[0]->compImgResourceList) {
        if (imgRes.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            continue;
        }
        colorFormats.push_back(imgRes.format);
    }
    VkPipelineRenderingCreateInfo pipelineRenderCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = static_cast<uint32_t>(colorFormats.size()),
        .pColorAttachmentFormats = colorFormats.data(),
        .depthAttachmentFormat = _depthFormat,
    };

    // Fill in incomplete stages and create pipeline
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.stageCount = std::size(shaderStages);
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.layout = _mrtPipelineLayout;
    pipelineCreateInfo.pNext = &pipelineRenderCreateInfo;
    pipelineCreateInfo.renderPass = VK_NULL_HANDLE;  // _mrtRenderPass
    pipelineCreateInfo.subpass = 0;  // index of subpass where this pipeline will be used
    CreationHelper::fillAndCreateGPipeline(pipelineCreateInfo, _mrtPipeline, _device,
                                           _swapChainExtent,
                                           MRT_OUT_SIZE - 1);  // total of color attachments

    // Free up immediate resources and delegate cleanup
    vkDestroyShaderModule(_device, mrtVertShaderModule, nullptr);
    vkDestroyShaderModule(_device, mrtFragShaderModule, nullptr);

    // Composition pipeline --------------------------------------------------------

    std::vector<char> compVertShaderCode =
        CreationHelper::readFile("assets/shaders/composition.vert.spv");
    std::vector<char> compFragShaderCode =
        CreationHelper::readFile("assets/shaders/composition.frag.spv");
    VkShaderModule compVertShaderModule =
        CreationHelper::createShaderModule(compVertShaderCode, _device);
    VkShaderModule compFragShaderModule =
        CreationHelper::createShaderModule(compFragShaderCode, _device);

    vertShaderStageInfo.module = compVertShaderModule;
    fragShaderStageInfo.module = compFragShaderModule;
    shaderStages[0] = vertShaderStageInfo;
    shaderStages[1] = fragShaderStageInfo;

    // pipeline layout info
    pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = _compSetLayoutList.size();
    pipelineLayoutInfo.pSetLayouts = _compSetLayoutList.data();

    // Push constant
    pushConstantRange.size = sizeof(CompPushConstantData);
    pushConstantRange.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;  // best to share directly
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_compPipelineLayout) !=
        VK_SUCCESS) {
        l->error("failed to create composition pipeline layout");
        return false;
    }

    // empty vertex info
    vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // replaced render pass with dynamic rendering
    // TODO: we're hardcoding the amount
    pipelineRenderCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &_swapchainImageFormat,
    };

    // fill in pipeline create info
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.stageCount = std::size(shaderStages);
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.layout = _compPipelineLayout;
    pipelineCreateInfo.pNext = &pipelineRenderCreateInfo;
    pipelineCreateInfo.renderPass = VK_NULL_HANDLE;  // _compositionRenderPass
    pipelineCreateInfo.subpass = 0;  // index of subpass where this pipeline will be used
    CreationHelper::fillAndCreateGPipeline(pipelineCreateInfo, _compPipeline, _device,
                                           _swapChainExtent, 1);

    _globCleanup.emplace([this]() {
        vkDestroyPipelineLayout(_device, _mrtPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _mrtPipeline, nullptr);
        vkDestroyPipelineLayout(_device, _compPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _compPipeline, nullptr);
    });

    return true;
}

int Renderer::createMaterial(MaterialCpu &materialCpu) {
    auto l = SLog::get();
    std::shared_ptr<MaterialGpu> gpuMaterial = std::make_shared<MaterialGpu>();

    // uniform
    l->vk_res(CreationHelper::createUniformBuffer(
        _allocator, sizeof(MrtUboData), gpuMaterial->uniformBuffer, gpuMaterial->uniformAlloc,
        gpuMaterial->uniformAllocInfo));

    // Create descriptor set resource
    DescriptorBuilder mrtSetBuilder(_device, _globalDescPool);
    mrtSetBuilder.setTotalSet(1).setSetLayout(0, _mrtSetLayout);
    mrtSetBuilder.pushDefaultUniform(0, VK_SHADER_STAGE_FRAGMENT_BIT);
    mrtSetBuilder.pushDefaultFragmentSamplerBinding(0);
    mrtSetBuilder.pushDefaultFragmentSamplerBinding(0);
    mrtSetBuilder.pushDefaultFragmentSamplerBinding(0);

    mrtSetBuilder.pushSetWriteUniform(0, gpuMaterial->uniformBuffer, sizeof(MrtUboData));

    // upload textures for sample
    if (materialCpu.info.useColor()) {
        gpuMaterial->albedoTex.inuse = true;
        uploadImageForSampling(materialCpu.albedoTexture, gpuMaterial->albedoTex,
                               VK_FORMAT_R8G8B8A8_SRGB);
        mrtSetBuilder.pushSetWriteImgSampler(0, gpuMaterial->albedoTex.imageView,
                                             gpuMaterial->albedoTex.sampler, 1);
    }
    if (materialCpu.info.useNormal()) {
        gpuMaterial->normalTex.inuse = true;
        uploadImageForSampling(materialCpu.normalTexture, gpuMaterial->normalTex,
                               VK_FORMAT_R8G8B8A8_UNORM);
        mrtSetBuilder.pushSetWriteImgSampler(0, gpuMaterial->normalTex.imageView,
                                             gpuMaterial->normalTex.sampler, 2);
    }
    if (materialCpu.info.useAo() || materialCpu.info.useHeight() ||
        materialCpu.info.useRoughness()) {
        gpuMaterial->aoRoughnessHeight.inuse = true;
        uploadImageForSampling(materialCpu.aoRoughnessHeightTexture, gpuMaterial->aoRoughnessHeight,
                               VK_FORMAT_R8G8B8A8_UNORM);
        mrtSetBuilder.pushSetWriteImgSampler(0, gpuMaterial->aoRoughnessHeight.imageView,
                                             gpuMaterial->aoRoughnessHeight.sampler, 3);
    }

    gpuMaterial->uboData = materialCpu.info;
    gpuMaterial->descriptorSet = mrtSetBuilder.buildSet(0);

    _materialMap[_nextMatId] = gpuMaterial;
    return _nextMatId++;
}

std::shared_ptr<ModalState> Renderer::uploadModel(ModelDataCpu &modelData) {
    std::shared_ptr<ModalState> newModalState = std::make_shared<ModalState>();

    auto l = SLog::get();
    // VMA best usage info:
    // https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
    // Buffer info
    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = sizeof(Vertex) * modelData.vertex.size();
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBufferCreateInfo gpuBufferInfo = stagingBufferInfo;
    gpuBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    l->debug(fmt::format("copy vertex buffer to gpu (size: {:d}, total: {:d}, indices: {:d})",
                         sizeof(Vertex), modelData.vertex.size(), modelData.indices.size()));

    // Memory info, must have VMA_ALLOCATION_CREATE_MAPPED_BIT to create persistent map area so we
    // can avoid map / unmap memory
    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;  // vma way texBufferInfo doing stuff, read doc!
    stagingAllocInfo.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    stagingAllocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;  // to avoid flushing

    VmaAllocationCreateInfo dstAllocInfo{};
    dstAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    dstAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    dstAllocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    VmaAllocation stagingAllocation;  // represent underlying memory
    VmaAllocationInfo
        stagingAllocationInfo;  // for persistent mapping:
                                // https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html
    VkBuffer stagingBuffer;
    l->vk_res(vmaCreateBuffer(_allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer,
                              &stagingAllocation, &stagingAllocationInfo));

    // Copy vertex data to staging buffer physical memory
    memcpy(stagingAllocationInfo.pMappedData, modelData.vertex.data(), stagingBufferInfo.size);
    // TODO: Check stagingAllocationInfo.memoryType for VK_MEMORY_PROPERTY_HOST_COHERENT_BIT to
    // avoid flushing all
    // https://stackoverflow.com/questions/44366839/meaning-of-memorytypes-in-vulkan
    vmaFlushAllocation(_allocator, stagingAllocation, 0,
                       stagingBufferInfo.size);  // TODO: Transfer to destination buffer
    // Flush for VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:
    // https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html

    // transfer and free
    l->vk_res(vmaCreateBuffer(_allocator, &gpuBufferInfo, &dstAllocInfo, &newModalState->vBuffer,
                              &newModalState->vAllocation, &stagingAllocationInfo));
    copyBuffer(stagingBuffer, newModalState->vBuffer, sizeof(Vertex) * modelData.vertex.size());
    vmaDestroyBuffer(_allocator, stagingBuffer, stagingAllocation);

    // Buffer info
    stagingBufferInfo.size = sizeof(modelData.indices[0]) * modelData.indices.size();
    gpuBufferInfo.size = sizeof(modelData.indices[0]) * modelData.indices.size();
    gpuBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    l->vk_res(vmaCreateBuffer(_allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer,
                              &stagingAllocation, &stagingAllocationInfo));
    memcpy(stagingAllocationInfo.pMappedData, modelData.indices.data(), stagingBufferInfo.size);
    vmaFlushAllocation(_allocator, stagingAllocation, 0,
                       stagingBufferInfo.size);  // TODO: Transfer to destination buffer

    // transfer and free
    l->vk_res(vmaCreateBuffer(_allocator, &gpuBufferInfo, &dstAllocInfo, &newModalState->iBuffer,
                              &newModalState->iAllocation, &stagingAllocationInfo));
    copyBuffer(stagingBuffer, newModalState->iBuffer,
               sizeof(modelData.indices[0]) * modelData.indices.size());
    vmaDestroyBuffer(_allocator, stagingBuffer, stagingAllocation);

    // update modal state
    newModalState->indicesSize = modelData.indices.size();
    newModalState->modelDataPartition = modelData.modelDataPartition;

    _modalStateList.push_back(newModalState);

    return newModalState;
}

void Renderer::removeModal(const std::shared_ptr<ModalState> &modalState) {
    auto l = SLog::get();
    auto iter = std::find(_modalStateList.begin(), _modalStateList.end(), modalState);
    if (iter != _modalStateList.end()) {
        l->debug("removing modal & materials");
        _modalStateList.erase(iter);
        // remove model data
        vmaDestroyBuffer(_allocator, modalState->vBuffer, modalState->vAllocation);
        vmaDestroyBuffer(_allocator, modalState->iBuffer, modalState->iAllocation);
        // delete all materials data
        for (const auto &modalDataPart : modalState->modelDataPartition) {
            if (!_materialMap.contains(modalDataPart.materialId)) {
                continue;
            }
            const auto mat = _materialMap[modalDataPart.materialId];
            _materialMap.erase(modalDataPart.materialId);
            // remove image sampler resource
            std::function<void(ImgResource &)> delImgIfUsed = [&](ImgResource &img) {
                if (img.inuse) {
                    vkDestroySampler(_device, img.sampler, nullptr);
                    vkDestroyImageView(_device, img.imageView, nullptr);
                    vmaDestroyImage(_allocator, img.image, img.allocation);
                }
            };
            delImgIfUsed(mat->albedoTex);
            delImgIfUsed(mat->normalTex);
            delImgIfUsed(mat->aoRoughnessHeight);
            vkFreeDescriptorSets(_device, _globalDescPool, 1, &mat->descriptorSet);
            vmaDestroyBuffer(_allocator, mat->uniformBuffer, mat->uniformAlloc);
        }
    }
}

void Renderer::setLightInfo(const glm::vec3 &pos, const glm::vec3 &color, float radius) {
    if (_nextLightPos == std::size(_nextCompUboData.lights)) {
        auto l = SLog::get();
        l->error("add light info exceed maximum capacity, skipping");
        return;
    }
    _nextCompUboData.lights[_nextLightPos].position = glm::vec4{pos, 1};
    _nextCompUboData.lights[_nextLightPos].colorAndRadius = glm::vec4{color, radius};
    _nextLightPos++;
}

bool Renderer::initImGUI() {
    auto l = SLog::get();
    l->debug("initialising GUI");

    // 1: Create Descriptor pool for ImGUI
    // the size of the pool is very oversize, but it's copied from imgui demo itself.
    VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 400},
                                         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 400},
                                         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 400},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 400},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 400},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 400},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 400},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 400},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 400},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 400},
                                         {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 400}};
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
    // initInfo.RenderPass = _compositionRenderPass;
    initInfo.UseDynamicRendering = true;
    // TODO: refactor this to be the same place as initpipeline?
    initInfo.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &_swapchainImageFormat,
    };

    ImGui_ImplVulkan_Init(&initInfo);
    // execute a gpu command to upload imgui font textures
    ImGui_ImplVulkan_CreateFontsTexture();

    // add the destroy the imgui created structures
    _globCleanup.emplace([this, imguiPool]() {
        ImGui_ImplVulkan_DestroyFontsTexture();
        vkDestroyDescriptorPool(_device, imguiPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
    });

    return true;
}

void Renderer::newFrame() {
    auto l = SLog::get();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // command buffer
    vkResetCommandBuffer(_flightResources[_curFrameInFlight]->compCmdBuffer, 0);
    vkResetCommandBuffer(_flightResources[_curFrameInFlight]->mrtCmdBuffer, 0);

    VkResult result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX,
                                            _flightResources[_curFrameInFlight]->imageAvailableSem,
                                            VK_NULL_HANDLE, &_curPresentImgIdx);
    l->vk_res(result);
}

void Renderer::beginRecordCmd() {
    auto l = SLog::get();
    // render start ---------------------------------------------
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;  // one time / secondary / simultaneous submit
    beginInfo.pInheritanceInfo = nullptr;
    if (vkBeginCommandBuffer(_flightResources[_curFrameInFlight]->mrtCmdBuffer, &beginInfo) !=
        VK_SUCCESS) {
        l->error("failed to begin recording command buffer!");
    }
    if (vkBeginCommandBuffer(_flightResources[_curFrameInFlight]->compCmdBuffer, &beginInfo) !=
        VK_SUCCESS) {
        l->error("failed to begin recording command buffer!");
    }

    // transition image layout at the start of the stage
    // TODO: we're hardcoding depth position to be 0
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.image = _flightResources[_curFrameInFlight]->compImgResourceList[0].image;
    barrier.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_DEPTH_BIT;  // if sencil is enabled, add VK_IMAGE_ASPECT_STENCIL_BIT
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    vkCmdPipelineBarrier(
        _flightResources[_curFrameInFlight]->mrtCmdBuffer,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0,
        0, nullptr, 0, nullptr, 1, &barrier);

    // FIXME: some attachment is read only,
    std::vector<VkImageMemoryBarrier> barriers;
    for (int i = 1; i < MRT_OUT_SIZE; ++i) {
        barrier.image = _flightResources[_curFrameInFlight]->compImgResourceList[i].image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        barriers.push_back(barrier);
    }
    vkCmdPipelineBarrier(
        _flightResources[_curFrameInFlight]->mrtCmdBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  // or VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, barriers.size(),
        barriers.data());

    // create render info
    VkRenderingInfo mrtRenderInfo = {};
    mrtRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    mrtRenderInfo.renderArea.offset = {0, 0};
    mrtRenderInfo.renderArea.extent = _swapChainExtent;
    mrtRenderInfo.layerCount = 1;

    std::vector<VkRenderingAttachmentInfo> depthInfo;
    std::vector<VkRenderingAttachmentInfo> colorInfo;
    // TODO: remove hardcoded logic where first image is depth, should also check for multiple
    // depth images
    for (const auto &imgRes : _flightResources[_curFrameInFlight]->compImgResourceList) {
        if (imgRes.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            VkRenderingAttachmentInfo attachmentInfo =
                CreationHelper::convertImgResourceToAttachmentInfo(
                    imgRes, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);
            depthInfo.push_back(attachmentInfo);
        } else {
            VkRenderingAttachmentInfo attachmentInfo =
                CreationHelper::convertImgResourceToAttachmentInfo(
                    imgRes, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR,
                    VK_ATTACHMENT_STORE_OP_STORE);
            colorInfo.push_back(attachmentInfo);
        }
    }
    mrtRenderInfo.pDepthAttachment = depthInfo.data();
    mrtRenderInfo.pColorAttachments = colorInfo.data();
    mrtRenderInfo.colorAttachmentCount = colorInfo.size();

    VkRenderingAttachmentInfo compAttachmentInfo = {};
    compAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    compAttachmentInfo.imageView = _swapchainImageViews[_curPresentImgIdx];
    compAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    compAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    compAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    compAttachmentInfo.clearValue = VkClearValue{.color = {0, 0, 0}};

    VkRenderingInfo compRenderInfo = mrtRenderInfo;
    compRenderInfo.pDepthAttachment = nullptr;
    compRenderInfo.colorAttachmentCount = 1;
    compRenderInfo.pColorAttachments = &compAttachmentInfo;

    // start render pass
    vkCmdBeginRendering(_flightResources[_curFrameInFlight]->mrtCmdBuffer, &mrtRenderInfo);
    vkCmdBindPipeline(_flightResources[_curFrameInFlight]->mrtCmdBuffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS, _mrtPipeline);
    vkCmdBeginRendering(_flightResources[_curFrameInFlight]->compCmdBuffer, &compRenderInfo);
    vkCmdBindPipeline(_flightResources[_curFrameInFlight]->compCmdBuffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS, _compPipeline);

    // global set for both pipeline
    vkCmdBindDescriptorSets(
        _flightResources[_curFrameInFlight]->compCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        _compPipelineLayout, 0, _flightResources[_curFrameInFlight]->compDescSetList.size(),
        _flightResources[_curFrameInFlight]->compDescSetList.data(), 0, nullptr);

    // Draw imgui
    ImGui::Text("World coord: up +y, right +x, forward -z");
}

void Renderer::drawAllModel() {
    MrtPushConstantData mrtData{};

    for (const auto &modalState : _modalStateList) {
        // compute final transform
        mrtData.viewModalTransform = _camViewTransform * modalState->worldTransform;
        mrtData.perspectiveTransform = _camProjectionTransform;
        vkCmdPushConstants(_flightResources[_curFrameInFlight]->mrtCmdBuffer, _mrtPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(MrtPushConstantData), &mrtData);

        // bind group of vertex and indices
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(_flightResources[_curFrameInFlight]->mrtCmdBuffer, 0, 1,
                               &modalState->vBuffer, offsets);
        vkCmdBindIndexBuffer(_flightResources[_curFrameInFlight]->mrtCmdBuffer, modalState->iBuffer,
                             0, VK_INDEX_TYPE_UINT32);

        // modal partition based on material
        for (const auto &modalDataPart : modalState->modelDataPartition) {
            auto mat = _materialMap[modalDataPart.materialId];
            // bind resources
            vkCmdBindDescriptorSets(_flightResources[_curFrameInFlight]->mrtCmdBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS, _mrtPipelineLayout, 0, 1,
                                    &mat->descriptorSet, 0, nullptr);
            // uniform data
            memcpy(mat->uniformAllocInfo.pMappedData, &mat->uboData, sizeof(MrtUboData));
            // draw index partition
            vkCmdDrawIndexed(_flightResources[_curFrameInFlight]->mrtCmdBuffer,
                             modalDataPart.indexCount, 1, modalDataPart.firstIndex, 0, 0);
        }
    }
}

void Renderer::writeDebugUi(const std::string &msg) { _debugUiText.emplace_back(msg); }

void Renderer::endRecordCmd() {
    // debug ui text buffer
    for (const auto &t : _debugUiText) {
        ImGui::Text("%s", t.c_str());
    }
    _debugUiText.clear();

    // uniform data
    memcpy(_flightResources[_curFrameInFlight]->compUniformAllocInfo.pMappedData, &_nextCompUboData,
           sizeof(CompUboData));
    _nextLightPos = 0;

    // Push constant for composition
    CompPushConstantData pushConstantData{};
    pushConstantData.sobelWidth = 1;
    pushConstantData.sobelHeight = 1;
    vkCmdPushConstants(_flightResources[_curFrameInFlight]->compCmdBuffer, _compPipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(CompPushConstantData), &pushConstantData);
    // Issue draw a single triangle that covers full screen
    vkCmdDraw(_flightResources[_curFrameInFlight]->compCmdBuffer, 3, 1, 0, 0);

    // IMGUI draw last call
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    _flightResources[_curFrameInFlight]->compCmdBuffer);

    // transition image layout for presentation
    auto l = SLog::get();
    vkCmdEndRendering(_flightResources[_curFrameInFlight]->mrtCmdBuffer);
    if (vkEndCommandBuffer(_flightResources[_curFrameInFlight]->mrtCmdBuffer) != VK_SUCCESS) {
        l->error("failed to end record command buffer!");
    }

    vkCmdEndRendering(_flightResources[_curFrameInFlight]->compCmdBuffer);
    auto transFn = transitionImgLayout(_swapchainImages[_curFrameInFlight],
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                       VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    transFn(_flightResources[_curFrameInFlight]->compCmdBuffer);
    if (vkEndCommandBuffer(_flightResources[_curFrameInFlight]->compCmdBuffer) != VK_SUCCESS) {
        l->error("failed to end record command buffer!");
    }
}

void Renderer::draw() {
    auto l = SLog::get();
    // Wait for previous frame to finish (takes array of fences)
    vkWaitForFences(_device, 1, &_flightResources[_curFrameInFlight]->renderFence, VK_TRUE,
                    UINT64_MAX);
    // reset to unsignaled only if we're sure we have work to do.
    vkResetFences(_device, 1, &_flightResources[_curFrameInFlight]->renderFence);

    // sync primitive
    VkSemaphore mrtWaitSem[] = {_flightResources[_curFrameInFlight]->imageAvailableSem};
    VkSemaphore mrtSignalSem[] = {_flightResources[_curFrameInFlight]->mrtSemaphore};
    VkSemaphore compSignalSem[] = {_flightResources[_curFrameInFlight]->compSemaphore};

    // Submit MRT ---------------------------------------------------
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_flightResources[_curFrameInFlight]->mrtCmdBuffer;

    // wait at the writing color before the image is available
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = std::size(mrtWaitSem);
    submitInfo.pWaitSemaphores = mrtWaitSem;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = std::size(mrtSignalSem);
    submitInfo.pSignalSemaphores = mrtSignalSem;

    l->vk_res(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

    // Submit Composition ---------------------------------------------------
    submitInfo.pCommandBuffers = &_flightResources[_curFrameInFlight]->compCmdBuffer;
    submitInfo.waitSemaphoreCount = std::size(mrtSignalSem);
    submitInfo.pWaitSemaphores = mrtSignalSem;
    submitInfo.signalSemaphoreCount = std::size(compSignalSem);
    submitInfo.pSignalSemaphores = compSignalSem;
    l->vk_res(vkQueueSubmit(_graphicsQueue, 1, &submitInfo,
                            _flightResources[_curFrameInFlight]->renderFence));

    // Submit result back to swap chain
    // What to signal when we're done
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = std::size(compSignalSem);
    presentInfo.pWaitSemaphores = compSignalSem;

    VkSwapchainKHR swapchains[] = {_swapchain};
    presentInfo.swapchainCount = std::size(swapchains);
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &_curPresentImgIdx;

    vkQueuePresentKHR(_presentsQueue, &presentInfo);

    // Update next frame resources indexes to use
    _curFrameInFlight = (_curFrameInFlight + 1) % _renderConf.maxFrameInFlight;
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
        vkCmdCopyBufferToImage(cmdBuf, srcBuffer, dstImg, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &copyRegion);
    };
    execOneTimeCmd(func);
}

std::function<void(VkCommandBuffer)> Renderer::transitionImgLayout(VkImage image,
                                                                   VkImageLayout oldLayout,
                                                                   VkImageLayout newLayout) {
    // We need to handle two kind of layout transformation:
    // 1. Undefined → transferDst: transfer writes that don't need to wait on anything
    // 2. transferDst → shaderRead: shader reads should wait on transfer writes, specifically the
    // shader reads in the fragment shader, because that's where we're going to use the texture
    // TODO: should we transit depth image?

    return [=](VkCommandBuffer cmdBuf) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        // We're not transferring queue ownership
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // this is not default value!
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
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            barrier.srcAccessMask = VK_ACCESS_NONE;
            destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
            // swapchain presentation
            sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            destStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            barrier.dstAccessMask = VK_ACCESS_NONE;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(cmdBuf, sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);
    };
}

void Renderer::uploadImageForSampling(const TextureData &cpuTexData, ImgResource &outResourceInfo,
                                      VkFormat sampleFormat) {
    auto l = SLog::get();
    l->debug(fmt::format("upload texture dim: ({:d}, {:d}, {:d})", cpuTexData.texWidth,
                         cpuTexData.texHeight, cpuTexData.texChannels));

    // Create buffer for transfer source image
    VkBuffer texBuffer;
    VmaAllocationCreateInfo stagingAllocInfo = CreationHelper::createStagingAllocInfo();
    VmaAllocation stagingAllocation;
    VmaAllocationInfo stagingAllocationInfo;

    VkBufferCreateInfo texBufferCreateInfo{};
    texBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    texBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    texBufferCreateInfo.size = cpuTexData.texWidth * cpuTexData.texHeight * cpuTexData.texChannels;
    texBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    l->vk_res(vmaCreateBuffer(_allocator, &texBufferCreateInfo, &stagingAllocInfo, &texBuffer,
                              &stagingAllocation, &stagingAllocationInfo));
    memcpy(stagingAllocationInfo.pMappedData, cpuTexData.data, texBufferCreateInfo.size);
    vmaFlushAllocation(_allocator, stagingAllocation, 0,
                       texBufferCreateInfo.size);  // TODO: Transfer to destination buffer

    // Create GPU local sampled image
    VkExtent2D ext{static_cast<uint32_t>(cpuTexData.texWidth),
                   static_cast<uint32_t>(cpuTexData.texHeight)};
    VkImageCreateInfo textureImageInfo = CreationHelper::imageCreateInfo(
        sampleFormat, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, ext);
    // allocate from GPU LOCAL memory
    VmaAllocationCreateInfo texAllocInfo{};
    texAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    texAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkResult result = vmaCreateImage(_allocator, &textureImageInfo, &texAllocInfo,
                                     &outResourceInfo.image, &outResourceInfo.allocation, nullptr);
    l->vk_res(result);

    // transition the layout image for layout destination
    auto transFn = transitionImgLayout(outResourceInfo.image, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    execOneTimeCmd(transFn);

    // perform copy operation
    copyBufferToImg(texBuffer, outResourceInfo.image, ext);

    // make it optimal for sampling
    transFn = transitionImgLayout(outResourceInfo.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    execOneTimeCmd(transFn);

    // staging buffer can be destroyed as src textures are no longer used
    vmaDestroyBuffer(_allocator, texBuffer, stagingAllocation);

    // image view and sampler
    VkImageViewCreateInfo createImgViewInfo = CreationHelper::imageViewCreateInfo(
        sampleFormat, outResourceInfo.image, VK_IMAGE_ASPECT_COLOR_BIT);
    l->vk_res(vkCreateImageView(_device, &createImgViewInfo, nullptr, &outResourceInfo.imageView));
    VkSamplerCreateInfo createSampInfo = CreationHelper::samplerCreateInfo(
        _gpu, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    l->vk_res(vkCreateSampler(_device, &createSampInfo, nullptr, &outResourceInfo.sampler));
}

bool Renderer::initPreApp() {
    // Initialise default material for all object
    auto mat = MaterialCpu{.info = {
                               .diffuse = glm::vec4{0, 0.2, 0.2, 1},
                           }};
    createMaterial(mat);

    return true;
}
}  // namespace luna
