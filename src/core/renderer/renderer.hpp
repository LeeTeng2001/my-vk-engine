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
    VkSemaphore mrtSemaphore{};
    VkCommandBuffer mrtCmdBuffer{};

    // Img Resources
    vector<ImgResource*> compImgResourceList;

    // Composition
    vector<VkDescriptorSet> compDescSetList{};
    VkSemaphore compSemaphore{};
    VkCommandBuffer compCmdBuffer{};

    // Comp Uniform
    VkBuffer compUniformBuffer;
    VmaAllocationInfo compUniformAllocInfo{};

    VkSemaphore imageAvailableSem{};
    VkFence renderFence{};
};

class Renderer {
public:
    Renderer() = default;

    // used during screen resize
    bool initialise(RenderConfig renderConfig = {});
    void shutdown();
    // void rebuild();

    // render related, should invoke in order
    void newFrame();
    void beginRecordCmd();
    void drawAllModel();
    void writeDebugUi(const string &msg);
    void endRecordCmd();
    void draw();

    // data related
    shared_ptr<ModalState> uploadAndPopulateModal(ModelData& modelData);
    void removeModal(const shared_ptr<ModalState>& modelData);

    // setter
    void setViewMatrix(const glm::mat4 &viewTransform) { _camViewTransform = viewTransform; };
    void setProjectionMatrix(const glm::mat4 &projectionTransform) { _camProjectionTransform = projectionTransform; };
    void setCamPos(const glm::vec3 &pos) { _nextCompUboData.camPos = glm::vec4{pos, 1}; };
    void setLightInfo(const glm::vec3 &pos, const glm::vec3 &color, float radius);

    // getter
    [[nodiscard]] const RenderConfig& getRenderConfig() { return _renderConf; }

private:
    // internal creations
    void setRequiredFeatures();
    void printPhysDeviceProps();
    bool validate();
    bool initBase();
    bool initCommand();
    bool initBuffer();
    bool initRenderResources();
    bool initDescriptors();
    bool initRenderPass();
    bool initFramebuffer();
    bool initSync();
    bool initPipeline();
    bool initImGUI();

    // Command Helper
    void execOneTimeCmd(const std::function<void(VkCommandBuffer)> &function);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImg(VkBuffer srcBuffer, VkImage dstImg, VkExtent2D extent);
    void transitionImgLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void uploadImageForSampling(const TextureData &cpuTexData, ImgResource &outResourceInfo, VkFormat sampleFormat);

    // Current draw state
    int _curFrameInFlight = 0;
    uint32_t _curPresentImgIdx = 0;
    glm::mat4 _camViewTransform{};
    glm::mat4 _camProjectionTransform{};
    CompUboData _nextCompUboData{};
    int _nextLightPos{};
    vector<string> _debugUiText;
    vector<shared_ptr<ModalState>> _modalStateList;

    // members
    RenderConfig _renderConf;
    stack<function<void ()>> _interCleanup{};
    stack<function<void ()>> _globCleanup;

    // core
    class SDL_Window* _window{};
    VkInstance _instance{};
    VkPhysicalDevice _gpu{};
    VkPhysicalDeviceProperties _gpuProperties{};
    VkDevice _device{};
    VkSurfaceKHR _surface{};
    VmaAllocator _allocator{};  // Memory allocator by gpuopen

    // props
    VkPhysicalDeviceFeatures _requiredPhysicalDeviceFeatures{};
    VkFormat _depthFormat{};

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

    // Descriptions & layout
    VkRenderPass _mrtRenderPass{}; // render to multiple attachment output
    VkDescriptorSetLayout _mrtSetLayout{};
    VkPipelineLayout _mrtPipelineLayout{};
    VkPipeline _mrtPipeline{};
    VkRenderPass _compositionRenderPass{};  // use multiple attachment as sampler, do some composition and present
    vector<VkDescriptorSetLayout> _compSetLayoutList{};
    VkPipelineLayout _compPipelineLayout{};
    VkPipeline _compPipeline{};

    // Resources
    VkCommandPool _renderCmdPool{};
    VkCommandPool _oneTimeCmdPool{};
    VkDescriptorPool _globalDescPool{};
    vector<VkClearValue> _mrtClearColor;
    vector<FlightResource*> _flightResources;
};
