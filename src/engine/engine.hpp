#pragma once

#include <VkBootstrap.h>
#include <functional>
#include <stack>
#include <vector>
#include "vk_mem_alloc.h"
#include "custom_type.hpp"

using std::function;
using std::stack;
using std::array;
using std::vector;

class Engine {
public:
    Engine() = default;
    ~Engine();

    void initialize();
    void run();

private:
    // Initialization helper
    void setRequiredFeatures();
    void initBase();
    void initCommand();
    void initRenderPass();
    void initFramebuffer();
    void initSync();
    void initDescriptors();
    void initPipeline();
    void initData();
    void initImGUI();

    // Initialisation info helper
    void printPhysDeviceProps();

    // Command Helper
    void execOneTimeCmd(const std::function<void(VkCommandBuffer)> &function);

    // Core functions
    void draw();
    void drawImGUI();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIdx);

private:
    // Core member
    int _frameNumber{0};
    VkExtent2D _windowExtent{1700, 900};
    class SDL_Window *_window{};
    stack<function<void ()>> _globCleanup;

    // Required feature & extensions
    VkPhysicalDeviceFeatures _requiredPhysicalDeviceFeatures{};

    // Core vulkan variables
    VkInstance _instance{};
    VkDebugUtilsMessengerEXT _debug_messenger{};
    VkPhysicalDevice _gpu{};
    VkPhysicalDeviceProperties _gpuProperties{};
    VkDevice _device{};
    VkSurfaceKHR _surface{};
    VmaAllocator _allocator{};  // Memory allocator by gpuopen

    // Queues
    VkQueue _graphicsQueue{};
    uint32_t _graphicsQueueFamily{};
    VkQueue _presentsQueue{};
    uint32_t _presentsQueueFamily{};

    // Swapchain & Renderpass & framebuffer
    VkSwapchainKHR _swapchain{};
    VkFormat _swapchainImageFormat{};
    VkExtent2D _swapChainExtent{};
    vector<VkImage> _swapchainImages;
    vector<VkImageView> _swapchainImageViews;
    vector<VkFramebuffer> _swapChainFramebuffers;
    VkRenderPass _renderPass{};

    // Sync structure
    VkSemaphore _presentSemaphore{}, _renderSemaphore{};
    VkFence _renderFence{};

    // Commands
    VkCommandPool _renderCmdPool{};
    VkCommandPool _oneTimeCmdPool{};
    VkCommandBuffer _renderCmdBuffer{};

    // Pipeline layout
    VkPipelineLayout _pipelineLayout{};
    VkPipeline _graphicPipeline{};

    // descriptor


    // Data
    VkBuffer _vertexBuffer{};
    VkBuffer _idxBuffer{};
    vector<Vertex> _mVertex = {
            {{0, -0.5, 0}, {1, 0, 0}},
            {{-0.5, 0.5, 0}, {0, 0, 1}},
            {{0.5, 0.5, 0}, {0, 1, 0}},
    };
    vector<uint32_t> _mIdx = {
            0, 1, 2
    };
};
