#include "engine.hpp"
#include "creation_helper.hpp"
#include "utils.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <filesystem>
#include <bitset>
#include <imgui.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_sdl3.h>
#include <vk_mem_alloc.h>
// TODO: Fix use Vulkan header instead of SDL header
#include <array>
#include "camera.hpp"

namespace fs = std::filesystem;

void Engine::initialize() {
    setRequiredFeatures();
    initBase();
    initCommand();
    initRenderResources();
//    initDebugAssets();
    initAssets();
    initData();
    initSampler();
    initDescriptors();
    initRenderPass();
    initFramebuffer();
    initSync();
    initPipeline();
    initImGUI();
    initCamera();
}

void Engine::setRequiredFeatures() {
    // Required device feature
    _requiredPhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
    _requiredPhysicalDeviceFeatures.sampleRateShading = VK_TRUE;
#ifndef __APPLE__
    _requiredPhysicalDeviceFeatures.wideLines = VK_TRUE;
#endif
    _requiredPhysicalDeviceFeatures.shaderInt64 = VK_TRUE;
}

void Engine::initAssets() {
    // Right now a hardcoded path for file
    fs::path modelPath("assets/models/teapot.obj");
    fs::path diffusePath("assets/textures/viking_room.png");

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loading model: %s", modelPath.c_str());
    tinyobj::ObjReader objReader;
    tinyobj::ObjReaderConfig reader_config;
    if (!objReader.ParseFromFile(modelPath.generic_string(), reader_config)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Load model return error");
        if (!objReader.Error().empty()) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, objReader.Error().c_str());
        }
        return;
    }
    if (!objReader.Warning().empty()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, objReader.Warning().c_str());
    }
    //attrib will contain the vertex arrays of the file
    tinyobj::attrib_t attrib = objReader.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = objReader.GetShapes();
    std::vector<tinyobj::material_t> materials = objReader.GetMaterials();

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "model shapes: %zu", shapes.size());
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "model materials: %zu", materials.size());

    // Usage Guide: https://github.com/tinyobjloader/tinyobjloader
    // Loop over shapes
    // By default obj is already in counter-clockwise order
    for (auto &shape : shapes) {
        // Loop over faces (polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            bool shouldGenerateNormal = false;
            // Loop over vertices in the face. (rn I hardcode 3 vertices for a face)
            for (size_t v = 0; v < 3; v++) {
                Vertex vertex{};

                // access to vertex
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
                tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
                tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];

                vertex.pos.x = vx;
                vertex.pos.y = vy;
                vertex.pos.z = vz;
                vertex.color.x = vx;
                vertex.color.y = vy;
                vertex.color.z = vz;

                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
                    tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
                    tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
                    vertex.normal.x = nx;
                    vertex.normal.y = ny;
                    vertex.normal.z = nz;
                } else {
                    shouldGenerateNormal = true;
                }
                // TODO: Normal view transformation https://www.google.com/search?q=Normal+Vector+Transformation&rlz=1C1CHBD_enMY1006MY1006&oq=normal+vec&gs_lcrp=EgZjaHJvbWUqBggBEEUYOzIGCAAQRRg5MgYIARBFGDsyBggCEEUYOzIGCAMQRRg70gEIMjA0OGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8
                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0) {
                    tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
                    tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
                    // need to flip sign becase vulkan use tex from topy -> btm, inverse to OBJ
                    vertex.texCoord.x = tx;
                    vertex.texCoord.y = 1-ty;
                }
                // Optional: vertex colors
                // tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
                // tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
                // tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];

                _mVertex.push_back(vertex);
                _mIdx.push_back(_mIdx.size());
            }

            if (shouldGenerateNormal) {
                // since our model is being drawn in counter-clockwise
                glm::vec3 v1 = _mVertex[index_offset+2].pos - _mVertex[index_offset+1].pos;
                glm::vec3 v2 = _mVertex[index_offset].pos - _mVertex[index_offset+1].pos;
                glm::vec3 norm = glm::normalize(glm::cross(v1, v2));
                for (int i = 0; i < 3; ++i) {
                    _mVertex[index_offset+i].normal = norm;
                }
            }

            // By default obj uses counter-clockwise face
            // since, our front face is counter clockwise, and the
            // viewport get flipped during projection transformation
            // we need to swap front faces to clockwise in world space
            std::swap(_mVertex[index_offset+1], _mVertex[index_offset+2]);

            index_offset += 3;
        }
    }

    // Read material images
    // TODO: Rn we hardcode diffuse texture
//    for (const auto &mat: materials) {
//        fs::path diffuseTexPath = modelPath.parent_path() / mat.name;
//    }
    RawAppImage imgInfo;
    imgInfo.path = diffusePath.generic_string();
    imgInfo.stbRef = stbi_load(imgInfo.path.c_str(), &imgInfo.texWidth, &imgInfo.texHeight, &imgInfo.texChannels, 4);
    if (!imgInfo.stbRef) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "failed to load diffuse texture");
    }
    _mInitTextures.push_back(imgInfo);

    _globCleanup.emplace([this]() {
        for (const auto &cpuTex: _mInitTextures) {
            stbi_image_free(cpuTex.stbRef);
        }
    });
}

void Engine::initDebugAssets() {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loading debug model cube");

    // a cube in vulkan axis space
    float scale = 2;
    _mVertex = {
            // normal cube in world space: up:+y, right:+x, forward: +z
            // draw in counter-clockwise order
            {{scale, scale, -scale}, {}, {0, 0, 0}},      //0 front, tr
            {{scale, -scale, -scale}, {}, {1, 0, 0}},     //1 front, br
            {{-scale, scale, -scale}, {}, {0, 1, 0}},     //2 front, tl
            {{-scale, -scale, -scale}, {}, {1, 1, 0}},    //3 front, bl
            {{scale, scale, scale}, {}, {0, 0, 1}},       //4 back, tr
            {{scale, -scale, scale}, {}, {1, 0, 1}},      //5 back, br
            {{-scale, scale, scale}, {}, {0, 1, 1}  },    //6 back, tl
            {{-scale, -scale, scale}, {}, {1, 1, 1} },    //7 back, bl
            // RAW TRIANGLE! directly in clip space: up:-y, right:+x, forward: +z
            // draw in counter-clockwise order
//            {{0, -0.5, 0}, {}, {1, 0, 0}},       //8 top
//            {{-0.5, 0.5, 0}, {}, {0, 0, 1}},     //9 bottom left
//            {{0.5, 0.5, 0}, {}, {0, 1, 0}},      //10 bottom right
    };
    _mIdx = {
            // debug raw triangle
//            8, 9, 10
            // front
            0, 2, 1,
            2, 3, 1,
            // top
            4, 6, 0,
            6, 2, 0,
            // back
//            2, 3, 1,
    };
}

void Engine::initBase() {
    // Create SDL window
    SDL_Init(SDL_INIT_VIDEO);
    uint32_t window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    _window = SDL_CreateWindow(
            "Luna's Vulkan Engine",
            _windowExtent.width,
            _windowExtent.height,
            window_flags
    );

    // Initialise vulkan through bootstrap  ---------------------------------------------
    vkb::InstanceBuilder instBuilder;
    instBuilder.set_debug_callback(
            [] (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void *pUserData) -> VkBool32 {
                auto severity = vkb::to_string_message_severity(messageSeverity);
                auto type = vkb::to_string_message_type(messageType);
                switch (messageSeverity) {
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s\n", pCallbackData->pMessage);
                    break;
                default:
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vk [%s: %s] %s\n", severity, type, pCallbackData->pMessage);
                }
                return VK_FALSE;
            }
    );
//    instBuilder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);
//    instBuilder.set_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT);  // To enable output from shader
    instBuilder.set_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT);  // Normal applicaiton usage
    auto instBuildRes = instBuilder
            .set_app_name("Luna Vulkan Engine")
            .set_engine_name("Luna Engine")
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
    if (!SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface))
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
            .build();

    if (!vkbSwapchainRes){
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create swapchain. Error: %s", vkbSwapchainRes.error().message().c_str());
    }

    // store swapchain and its related images
    auto vkbSwapchain = vkbSwapchainRes.value();
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapChainExtent = vkbSwapchain.extent;
    // TODO: Fix extent, should query SDL: https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Window extent: %d, %d", _windowExtent.width, _windowExtent.height);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Swapchain extent: %d, %d", vkbSwapchain.extent.width, vkbSwapchain.extent.height);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Swapchain image counts: %zu", vkbSwapchain.get_images()->size());
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
        // deltaS
        float delta  = 1 / static_cast<float>(SDL_GetTicks() - lastFrameTick) * 1000;

        // Handle external events
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);  // handle event in ImGUI
            cam->ProcessInputEvent(delta, &e);
            switch (e.type) {
                case SDL_EVENT_QUIT:
                    bQuit = true;
                    break;
            }
        }

        // imgui new frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();  // Put your imgui draw code after NewFrame
        drawImGUI();
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
    SDL_Log("\tMax push constant size: %d", _gpuProperties.limits.maxPushConstantsSize);
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
    if (vkAllocateCommandBuffers(_device, &allocInfo, &_mrtCmdBuffer) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate command buffers");
    }
    if (vkAllocateCommandBuffers(_device, &allocInfo, &_compCmdBuffer) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate command buffers");
    }
}

void Engine::initRenderPass() {
    // Create MRT render pass ------------------------------------------------------------------d
    // Attachment in render pass
    VkAttachmentDescription colorAttachment = CreationHelper::createVkAttDesc(
            _swapchainImageFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkAttachmentDescription depthAttachment = CreationHelper::createVkAttDesc(
            _depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    vector<VkAttachmentDescription> allAttachments{
        colorAttachment, depthAttachment
    };

    // Subpass attachments
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Subpass & dependencies
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    // wait for swap-chain to finish reading before accessing it.
    // we should wait at color attachment stage or early fragment testing stage
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = VK_ACCESS_NONE;  // relates to memory access
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

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
            dependency, depthDependency, outDependency
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

void Engine::initRenderResources() {
    // This resource should probably be recreated if screen size change?
    _depthFormat = VK_FORMAT_D32_SFLOAT;  // TODO: Should query! not hardcode
    VkImageCreateInfo depthImgInfo = CreationHelper::imageCreateInfo(_depthFormat,
                                                                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                     _swapChainExtent);
    VkImageCreateInfo colorImgInfo = CreationHelper::imageCreateInfo(_swapchainImageFormat,
                                                                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                     _swapChainExtent);
    // allocate from GPU LOCAL memory
    VmaAllocationCreateInfo localAllocInfo {};
    localAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    localAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkResult result = vmaCreateImage(_allocator, &depthImgInfo, &localAllocInfo, &_depthImage._image, &_depthImage._allocation, nullptr);
    VK_CHECK_RESULT(result);
    result = vmaCreateImage(_allocator, &colorImgInfo, &localAllocInfo, &_colorImage._image, &_colorImage._allocation, nullptr);
    VK_CHECK_RESULT(result);

    VkImageViewCreateInfo ivInfo = CreationHelper::imageViewCreateInfo(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);
    result = vkCreateImageView(_device, &ivInfo, nullptr, &_depthImageView);
    VK_CHECK_RESULT(result);
    ivInfo = CreationHelper::imageViewCreateInfo(_swapchainImageFormat, _colorImage._image, VK_IMAGE_ASPECT_COLOR_BIT);
    result = vkCreateImageView(_device, &ivInfo, nullptr, &_colorImageView);
    VK_CHECK_RESULT(result);

    _globCleanup.emplace([this](){
        vkDestroyImageView(_device, _depthImageView, nullptr);
        vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);
        vkDestroyImageView(_device, _colorImageView, nullptr);
        vmaDestroyImage(_allocator, _colorImage._image, _colorImage._allocation);
    });
}

void Engine::initFramebuffer() {
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
                _swapchainImageViews[i] // , _colorImageView, _depthImageView
        };

        framebufferInfo.attachmentCount = std::size(attachments);
        framebufferInfo.pAttachments = attachments;

        VK_CHECK_RESULT(vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_swapChainFramebuffers[i]));
    }

    // mrt framebuffer
    framebufferInfo.renderPass = _mrtRenderPass;
    VkImageView attachments[] = {_colorImageView, _depthImageView};
    framebufferInfo.attachmentCount = std::size(attachments);
    framebufferInfo.pAttachments = attachments;
    VK_CHECK_RESULT(vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_mrtFramebuffer));

    _globCleanup.emplace([this](){
        for (const auto &item: _swapChainFramebuffers)
            vkDestroyFramebuffer(_device,  item, nullptr);
        vkDestroyFramebuffer(_device, _mrtFramebuffer, nullptr);
    });
}

void Engine::initSync() {
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

void Engine::initDescriptors() {
    // Describe individual binding in a set
    // holds information about the layout of a set
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].pImmutableSamplers = nullptr;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // must indicate stage

    bindings[1] = bindings[0];
    bindings[1].binding = 1;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    // create the set layout
    VkResult result = vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_compSetLayout);
    VK_CHECK_RESULT(result);

    // Creat pool for binding resources (including imgui
    std::vector<VkDescriptorPoolSize> sizes = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = 10;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes = sizes.data();
    vkCreateDescriptorPool(_device, &pool_info, nullptr, &_globalDescPool);

    // Now that we have info for the pipeline, we need to actually bind them to resources
    // Allocate desc set from?
    VkDescriptorSetAllocateInfo allocInfo ={};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _globalDescPool;
    allocInfo.descriptorSetCount = 1; // should change
    allocInfo.pSetLayouts = &_compSetLayout;
    vkAllocateDescriptorSets(_device, &allocInfo, &_compDescSet);

    // TODO: Rn we hardcode the first texture resource only
    // The actual resource
    for (int i = 0; i < bindings.size(); ++i) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (i == 0) {
            imageInfo.imageView = _colorImageView;
            imageInfo.sampler = _colorImageSampler;
        } else if (i == 1) {
            imageInfo.imageView = _depthImageView;
            imageInfo.sampler = _depthImageSampler;
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "out of range sampler");
        }

        // update set description, could be in batch
        VkWriteDescriptorSet setWrite = {};
        setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        setWrite.dstBinding = i;  // destination binding
        setWrite.dstSet = _compDescSet;
        setWrite.descriptorCount = 1;
        setWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        setWrite.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(_device, 1, &setWrite, 0, nullptr);
    }

    _globCleanup.emplace([this]() {
        vkDestroyDescriptorPool(_device, _globalDescPool, nullptr);
        vkDestroyDescriptorSetLayout(_device, _compSetLayout, nullptr);
    });
}

void Engine::initPipeline() {
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
    CreationHelper::fillAndCreateGPipeline(pipelineCreateInfo, _mrtPipeline, _device, _swapChainExtent);

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
    CreationHelper::fillAndCreateGPipeline(pipelineCreateInfo, _compPipeline, _device, _swapChainExtent);

    _globCleanup.emplace([this](){
        vkDestroyPipelineLayout(_device, _mrtPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _mrtPipeline, nullptr);
        vkDestroyPipelineLayout(_device, _compPipelineLayout, nullptr);
        vkDestroyPipeline(_device, _compPipeline, nullptr);
    });
}

void Engine::initData() {
    // VMA best usage info: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
    // Buffer info
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(Vertex) * _mVertex.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    SDL_Log("copy total of vertex buffer to gpu (size: %lu, total: %zu)", sizeof(Vertex), _mVertex.size());

    // Memory info, must have VMA_ALLOCATION_CREATE_MAPPED_BIT to create persistent map area so we can avoid map / unmap memory
    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;  // vma way texBufferInfo doing stuff, read doc!
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    stagingAllocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // to avoid flushing

    VmaAllocation stagingAllocation;  // represent underlying memory
    VmaAllocationInfo stagingAllocationInfo; // for persistent mapping: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html
    VK_CHECK_RESULT(vmaCreateBuffer(_allocator, &bufferInfo, &stagingAllocInfo, &_vertexBuffer, &stagingAllocation, &stagingAllocationInfo))
    _globCleanup.emplace([this, stagingAllocation](){
        vmaDestroyBuffer(_allocator, _vertexBuffer, stagingAllocation);
    });

    // Copy vertex data to staging buffer physical memory
    memcpy(stagingAllocationInfo.pMappedData, _mVertex.data(), bufferInfo.size);
    // TODO: Check stagingAllocationInfo.memoryType for VK_MEMORY_PROPERTY_HOST_COHERENT_BIT to avoid flushing all
    // https://stackoverflow.com/questions/44366839/meaning-of-memorytypes-in-vulkan
    vmaFlushAllocation(_allocator, stagingAllocation, 0, bufferInfo.size); // TODO: Transfer to destination buffer
    // Flush for VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html

    // Buffer info
    bufferInfo.size = sizeof(_mIdx[0]) * _mIdx.size();
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    VK_CHECK_RESULT(vmaCreateBuffer(_allocator, &bufferInfo, &stagingAllocInfo, &_idxBuffer, &stagingAllocation, &stagingAllocationInfo))
    _globCleanup.emplace([this, stagingAllocation](){
        vmaDestroyBuffer(_allocator, _idxBuffer, stagingAllocation);
    });

    memcpy(stagingAllocationInfo.pMappedData, _mIdx.data(), bufferInfo.size);
    vmaFlushAllocation(_allocator, stagingAllocation, 0, bufferInfo.size); // TODO: Transfer to destination buffer

    // Use staging buffer to transfer texture data to local memory
    for (const auto &cpuTex: _mInitTextures) {
        // Create buffer for transfer source image
        VkBuffer texBuffer;
        VkBufferCreateInfo texBufferInfo = bufferInfo;
        texBufferInfo.size = cpuTex.texWidth * cpuTex.texHeight * cpuTex.texChannels;
        texBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VK_CHECK_RESULT(vmaCreateBuffer(_allocator, &texBufferInfo, &stagingAllocInfo, &texBuffer, &stagingAllocation, &stagingAllocationInfo))
        memcpy(stagingAllocationInfo.pMappedData, cpuTex.stbRef, texBufferInfo.size);
        vmaFlushAllocation(_allocator, stagingAllocation, 0, texBufferInfo.size); // TODO: Transfer to destination buffer

        // Create GPU local sampled image
        // TODO: Query format?
        VkExtent2D ext{static_cast<uint32_t>(cpuTex.texWidth), static_cast<uint32_t>(cpuTex.texHeight)};
        VkImageCreateInfo textureImageInfo = CreationHelper::imageCreateInfo(VK_FORMAT_R8G8B8A8_SRGB,
                                                                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                             ext);
        // allocate from GPU LOCAL memory
        VmaAllocationCreateInfo texAllocInfo {};
        texAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        texAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        AllocatedImage allocatedTexImage{};
        VkResult result = vmaCreateImage(_allocator, &textureImageInfo, &texAllocInfo, &allocatedTexImage._image, &allocatedTexImage._allocation, nullptr);
        VK_CHECK_RESULT(result);

        // transition the layout image for layout destination
        transitionImgLayout(allocatedTexImage._image, VK_FORMAT_R8G8B8A8_SRGB,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        _textureImages.push_back(allocatedTexImage);

        // perform copy operation
        copyBufferToImg(texBuffer, allocatedTexImage._image, ext);

        // make it optimal for sampling
        transitionImgLayout(allocatedTexImage._image, VK_FORMAT_R8G8B8A8_SRGB,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // staging buffer can be destroyed as src textures are no longer used
        vmaDestroyBuffer(_allocator, texBuffer, stagingAllocation);
    }

    _globCleanup.emplace([this]() {
        for (const auto &img: _textureImages) {
            vmaDestroyImage(_allocator, img._image, img._allocation);
        }
    });
}

void Engine::initSampler() {
    for (const auto &texImg: _textureImages) {
        // Image view
        VkImageView imgView;
        VkImageViewCreateInfo ivInfo = CreationHelper::imageViewCreateInfo(
                VK_FORMAT_R8G8B8A8_SRGB, texImg._image, VK_IMAGE_ASPECT_COLOR_BIT);
        VkResult result = vkCreateImageView(_device, &ivInfo, nullptr, &imgView);
        VK_CHECK_RESULT(result);

        // sampler, note that it doesn't reference any image view!
        VkSampler sampler;
        VkSamplerCreateInfo sampInfo = CreationHelper::samplerCreateInfo(_gpu,
                                                                         VK_FILTER_LINEAR,
                                                                         VK_FILTER_LINEAR,
                                                                         VK_SAMPLER_ADDRESS_MODE_REPEAT);
        result = vkCreateSampler(_device, &sampInfo, nullptr, &sampler);
        VK_CHECK_RESULT(result);

        _textureImagesView.push_back(imgView);
        _textureImagesSampler.push_back(sampler);

        _globCleanup.emplace([this, imgView, sampler]() {
            vkDestroySampler(_device, sampler, nullptr);
            vkDestroyImageView(_device, imgView, nullptr);
        });
    }

    // Sampler for MRT
    VkSamplerCreateInfo sampInfo = CreationHelper::samplerCreateInfo(_gpu,
                                                                     VK_FILTER_LINEAR,
                                                                     VK_FILTER_LINEAR,
                                                                     VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    VkResult result = vkCreateSampler(_device, &sampInfo, nullptr, &_colorImageSampler);
    VK_CHECK_RESULT(result);
    result = vkCreateSampler(_device, &sampInfo, nullptr, &_depthImageSampler);
    VK_CHECK_RESULT(result);

    _globCleanup.emplace([this]() {
        vkDestroySampler(_device, _depthImageSampler, nullptr);
        vkDestroySampler(_device, _colorImageSampler, nullptr);
    });

}

void Engine::initImGUI() {
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
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create imgui descriptor pool");
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
}

void Engine::initCamera() {
    cam = new Camera(1980, 1080, 1, 100, 60);
}

void Engine::draw() {
    // Wait for previous frame to finish (takes array of fences)
    vkWaitForFences(_device, 1, &_renderFence, VK_TRUE, UINT64_MAX);

    // Get next available image
    uint32_t imageIdx;
    VkResult result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX,
                                            _presentSemaphore, VK_NULL_HANDLE, &imageIdx);
    VK_CHECK_RESULT(result);

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

    VK_CHECK_RESULT(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

    // Submit Composition ---------------------------------------------------
    submitInfo.pCommandBuffers = &_compCmdBuffer;
    submitInfo.waitSemaphoreCount = std::size(mrtSignalSem);
    submitInfo.pWaitSemaphores = mrtSignalSem;
    submitInfo.signalSemaphoreCount = std::size(compSignalSem);
    submitInfo.pSignalSemaphores = compSignalSem;
    VK_CHECK_RESULT(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _renderFence));

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

void Engine::recordMrtCommandBuffer(VkCommandBuffer commandBuffer) {
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
    VkClearValue clearColor[2] = {};
    clearColor[0].color = {{0.2f, 0.2f, 0.5f, 1.0f}}; // bg for color frame attach
    clearColor[1].depthStencil.depth = 1.0f; // depth attachment
    renderPassInfo.clearValueCount = std::size(clearColor);
    renderPassInfo.pClearValues = clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    // RENDER START ----------------------------------------------------
    // Bind components
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _mrtPipeline);
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

void Engine::recordCompCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIdx) {
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

//    // Clear value for attachment load op clear
//    // Should have the same order as attachments binding index
//    VkClearValue clearColor[2] = {};
//    clearColor[0].color = {{0.2f, 0.2f, 0.5f, 1.0f}}; // bg for color frame attach
//    clearColor[1].depthStencil.depth = 1.0f; // depth attachment
//    renderPassInfo.clearValueCount = std::size(clearColor);
//    renderPassInfo.pClearValues = clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    // RENDER START ----------------------------------------------------
    // Bind components
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _compPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _compPipelineLayout,
                            0, 1, &_compDescSet, 0, nullptr);


    // Push constant
    CompPushConstantData pushConstantData{};
    pushConstantData.sobelWidth = 3;
    pushConstantData.sobelHeight = 3;
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

void Engine::execOneTimeCmd(const std::function<void(VkCommandBuffer)> &function) {
    // One time sync fence
    VkFenceCreateInfo fenceInfo = CreationHelper::createFenceInfo(false);
    VkFence oneTimeFence;
    VK_CHECK_RESULT(vkCreateFence(_device, &fenceInfo, nullptr, &oneTimeFence));

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

void Engine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    auto func = [=](VkCommandBuffer cmdBuf) {
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(cmdBuf, srcBuffer, dstBuffer, 1, &copyRegion);
    };
    execOneTimeCmd(func);
}

void Engine::copyBufferToImg(VkBuffer srcBuffer, VkImage dstImg, VkExtent2D extent) {
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

void Engine::transitionImgLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
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

void Engine::drawImGUI() {
//    ImGui::ShowDemoWindow();
    ImGui::Text("World coord: up +y, right +x, forward +z");
    ImGui::Text("Cam Pos    : %s", glm::to_string(cam->GetCamPos()).c_str());
}


