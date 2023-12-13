#pragma once

#include <VkBootstrap.h>
#include <functional>
#include <stack>
#include <vector>
#include <SDL.h>
#include <vk_mem_alloc.h>
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
    void initAssets();
    void initDebugAssets();
    void initBase();
    void initCommand();
    void initRenderPass();
    void initRenderResources();
    void initFramebuffer();
    void initSync();
    void initDescriptors();
    void initPipeline();
    void initData();
    void initSampler();
    void initImGUI();
    void initCamera();

    // Initialisation info helper
    void printPhysDeviceProps();

    // Command Helper
    void execOneTimeCmd(const std::function<void(VkCommandBuffer)> &function);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImg(VkBuffer srcBuffer, VkImage dstImg, VkExtent2D extent);
    void transitionImgLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    // Core functions
    void draw();
    void drawImGUI();
    void recordMrtCommandBuffer(VkCommandBuffer commandBuffer);
    void recordCompCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIdx);

private:
    // Core member
    int _frameNumber{0};
    VkExtent2D _windowExtent{1700, 900};
    SDL_Window *_window{};
    stack<function<void ()>> _globCleanup;
    uint64_t lastFrameTick{0};

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
    VkFramebuffer _mrtFramebuffer;

    // deffered render multiple renderpass
    VkRenderPass _mrtRenderPass{}; // render to multiple attachment output
    VkRenderPass _compositionRenderPass{};  // use multiple attachment as input, do some composition and present

    // Pipeline layout
    VkPipelineLayout _mrtPipelineLayout{};
    VkPipelineLayout _compPipelineLayout{};
    VkPipeline _mrtPipeline{};
    VkPipeline _compPipeline{};

    // depth (swapchain doesn't come with depth image, we should create it ourselves)
    // and intermediate render target
    VkImageView _depthImageView;
    AllocatedImage _depthImage;
    VkFormat _depthFormat;
    VkSampler _depthImageSampler;
    VkImageView _colorImageView;
    AllocatedImage _colorImage;
    VkSampler _colorImageSampler;

    // gpu textures & samplers
    vector<AllocatedImage> _textureImages;
    vector<VkImageView> _textureImagesView;
    vector<VkSampler> _textureImagesSampler;

    // Sync structure
    VkSemaphore _presentSemaphore{}, _renderMrtSemaphore{}, _compSemaphore{};
    VkFence _renderFence{};

    // Commands pool and buffers
    VkCommandPool _renderCmdPool{};
    VkCommandPool _oneTimeCmdPool{};
    VkCommandBuffer _mrtCmdBuffer{};
    VkCommandBuffer _compCmdBuffer{};

    // descriptor
    VkDescriptorPool _globalDescPool;
    VkDescriptorSetLayout _compSetLayout;
    VkDescriptorSet _compDescSet;

    // Data
    VkBuffer _vertexBuffer{};
    VkBuffer _idxBuffer{};
    vector<Vertex> _mVertex = {};
    vector<uint32_t> _mIdx = {};
    vector<RawAppImage> _mInitTextures = {};

    // Helper
    class Camera *cam;
};
