#pragma once

#include "utils/common.hpp"
#include "def.hpp"

// think about what kind of abstraction to expose to upper user
// for vulkan renderer?
// this is not a general renderer!

// resources in a single flight
struct FlightResource {
    // MRT
    VkFramebuffer mrtFramebuffer{};
    VkDescriptorSet mrtDescSet{};
    VkSemaphore mrtSemaphore{};
    VkCommandBuffer mrtCmdBuffer{};

    // Resources
    vector<ImgResource*> imgResourceList;

    // Composition
    VkDescriptorSet compDescSet{};
    VkSemaphore compSemaphore{};
    VkCommandBuffer compCmdBuffer{};
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    // used during screen resize
    bool initialise(RenderConfig renderConfig = {});
    void rebuild();

    // render related
    void newFrame();
    void draw();

    // data related
    bool uploadAndPopulateModal(ModelData& modelData);

private:
    // internal creations
    void setRequiredFeatures();
    void printPhysDeviceProps();
    bool validateConfig();
    bool initBase();
    bool initCommand();
    bool initRenderResources();
    bool initDescriptors();
    bool initRenderPass();

    bool initImGUI();

    // Command Helper
    void execOneTimeCmd(const std::function<void(VkCommandBuffer)> &function);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImg(VkBuffer srcBuffer, VkImage dstImg, VkExtent2D extent);
    void transitionImgLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    // members
    RenderConfig _renderConf;
    stack<function<void ()>> _interCleanup{};
    stack<function<void ()>> _globCleanup;
    int _curFrameInFlight = 0;

    // core
    class SDL_Window* _window{};
    VkInstance _instance{};
    VkPhysicalDevice _gpu{};
    VkPhysicalDeviceProperties _gpuProperties{};
    VkDevice _device{};
    VkSurfaceKHR _surface{};
    VmaAllocator _allocator{};  // Memory allocator by gpuopen

    // props
    VkPhysicalDeviceFeatures _requiredPhysicalDeviceFeatures;
    VkFormat _depthFormat;

    // Queues
    VkQueue _graphicsQueue{};
    uint32_t _graphicsQueueFamily{};
    VkQueue _transferQueue{};
    uint32_t _transferQueueFamily{};
    VkQueue _presentsQueue{};
    uint32_t _presentsQueueFamily{};

    // Swapchain & Renderpass & framebuffer
    VkSwapchainKHR _swapchain{};
    VkFormat _swapchainImageFormat{};
    VkExtent2D _swapChainExtent{};
    vector<VkImage> _swapchainImages;
    vector<VkImageView> _swapchainImageViews;
    vector<VkFramebuffer> _swapChainFramebuffers;

    // Descriptions & layout
    VkRenderPass _mrtRenderPass{}; // render to multiple attachment output
    VkDescriptorSetLayout _mrtSetLayout;
    VkRenderPass _compositionRenderPass{};  // use multiple attachment as sampler, do some composition and present
    VkDescriptorSetLayout _compSetLayout;

    // Resources
    VkCommandPool _renderCmdPool{};
    VkCommandPool _oneTimeCmdPool{};
    VkDescriptorPool _globalDescPool;
    vector<FlightResource*> _flightResources;
    VkFence _renderFence{};
};
