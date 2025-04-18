#pragma once

#include "utils/common.hpp"
#include "def.hpp"

// think about what kind of abstraction to expose to upper user
// for vulkan renderer?
// this is not a general renderer!
namespace luna {

constexpr int MRT_SAMPLE_SIZE = 3;
constexpr int MRT_OUT_SIZE = 4;

// resources in a single flight
struct FlightResource {
        // MRT
        VkFramebuffer mrtFramebuffer{};
        VkSemaphore mrtSemaphore{};
        VkCommandBuffer mrtCmdBuffer{};

        // Img Resources
        std::vector<ImgResource> compImgResourceList;

        // Composition
        std::vector<VkDescriptorSet> compDescSetList{};
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
        void writeDebugUi(const std::string &msg);
        void endRecordCmd();
        void draw();

        // data related
        int createMaterial(MaterialCpu &materialCpu);  // return material id
        std::shared_ptr<ModalState> uploadModel(ModelDataCpu &modelData);
        void removeModal(const std::shared_ptr<ModalState> &modelData);

        // setter
        void setViewMatrix(const glm::mat4 &viewTransform) { _camViewTransform = viewTransform; };
        void setProjectionMatrix(const glm::mat4 &projectionTransform) {
            _camProjectionTransform = projectionTransform;
        };
        void setCamPos(const glm::vec3 &pos) { _nextCompUboData.camPos = glm::vec4{pos, 1}; };
        void setLightInfo(const glm::vec3 &pos, const glm::vec3 &color, float radius);
        void setDirLight(const glm::vec3 &dir, const glm::vec3 &color) {
            _nextCompUboData.dirLight = {glm::vec4{dir, 1}, glm::vec4{color, 1}};
        };
        void setClearColor(const glm::vec3 &color) { _clearVal = {color.x, color.y, color.z, 1}; };

        // getter
        [[nodiscard]] const RenderConfig &getRenderConfig() { return _renderConf; }

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
        bool initSync();
        bool initPipeline();
        bool initImGUI();
        bool initPreApp();

        // Command Helper
        void execOneTimeCmd(const std::function<void(VkCommandBuffer)> &function);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void copyBufferToImg(VkBuffer srcBuffer, VkImage dstImg, VkExtent2D extent);
        std::function<void(VkCommandBuffer)> transitionImgLayout(VkImage image,
                                                                 VkImageLayout oldLayout,
                                                                 VkImageLayout newLayout);
        void uploadImageForSampling(const TextureData &cpuTexData, ImgResource &outResourceInfo,
                                    VkFormat sampleFormat);

        // Current draw state
        int _curFrameInFlight = 0;
        uint32_t _curPresentImgIdx = 0;
        glm::mat4 _camViewTransform{};
        glm::mat4 _camProjectionTransform{};
        CompUboData _nextCompUboData{};
        int _nextLightPos{};
        std::vector<std::string> _debugUiText;
        int _nextMatId = 0;
        std::unordered_map<int, std::shared_ptr<MaterialGpu>> _materialMap;
        std::vector<std::shared_ptr<ModalState>> _modalStateList;

        // user settable basic config?
        VkClearValue _clearVal = {.color = {0, 0, 0}};

        // members
        RenderConfig _renderConf;
        std::stack<std::function<void()>> _interCleanup{};
        std::stack<std::function<void()>> _globCleanup;

        // core
        class SDL_Window *_window{};
        VkInstance _instance{};
        VkPhysicalDevice _gpu{};
        VkPhysicalDeviceProperties _gpuProperties{};
        VkDevice _device{};
        VkSurfaceKHR _surface{};
        VmaAllocator _allocator{};  // Memory allocator by gpuopen

        // defer info
        std::array<ImgResource, MRT_OUT_SIZE> _imgInfoList = {};

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
        std::vector<VkImage> _swapchainImages;
        std::vector<VkImageView> _swapchainImageViews;

        // Descriptions & layout
        VkDescriptorSetLayout _mrtSetLayout{};
        VkPipelineLayout _mrtPipelineLayout{};
        VkPipeline _mrtPipeline{};

        std::vector<VkDescriptorSetLayout> _compSetLayoutList{};
        VkPipelineLayout _compPipelineLayout{};
        VkPipeline _compPipeline{};

        // Resources
        VkCommandPool _renderCmdPool{};
        VkCommandPool _oneTimeCmdPool{};
        VkDescriptorPool _globalDescPool{};
        std::vector<FlightResource *> _flightResources;
};
}  // namespace luna