#pragma once

#include "stb_image.h"
#include "vk_mem_alloc.h"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "utils/common.hpp"

namespace luna {

struct RenderConfig {
        int maxFrameInFlight = 2;
        int windowWidth = 1700;
        int windowHeight = 900;
        VkDebugUtilsMessageSeverityFlagBitsEXT callbackSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
};

struct MrtUboData {
        glm::vec4 diffuse{};
        glm::vec4 emissive{};
        int textureToggle{};

        bool useColor() { return (textureToggle & 0b1) != 0; }
        bool useNormal() { return (textureToggle & 0b10) != 0; }
        bool useAo() { return (textureToggle & 0b100) != 0; }
        bool useRoughness() { return (textureToggle & 0b1000) != 0; }
        bool useHeight() { return (textureToggle & 0b10000) != 0; }

        void setColor() { textureToggle |= 0b1; }
        void setNormal() { textureToggle |= 0b10; }
        void setAo() { textureToggle |= 0b100; }
        void setRoughness() { textureToggle |= 0b1000; }
        void setHeight() { textureToggle |= 0b10000; }
};

struct MrtPushConstantData {
        glm::mat4 viewModalTransform;
        glm::mat4 perspectiveTransform;
};

struct DirectionalLight {
        glm::vec4 position;
        glm::vec4 color;
};

struct PointLight {
        glm::vec4 position;
        glm::vec4 colorAndRadius;
};

struct CompUboData {
        DirectionalLight dirLight;
        PointLight lights[6];
        glm::vec4 camPos;
};

struct CompPushConstantData {
        float sobelWidth;
        float sobelHeight;
};

struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec3 tangents;
        glm::vec3 bitangents;

        static std::vector<VkVertexInputBindingDescription> getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return {bindingDescription};
        };

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, normal);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Vertex, tangents);

            attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[4].offset = offsetof(Vertex, bitangents);

            return attributeDescriptions;
        }
};

struct TextureData {
        // populated by application
        const unsigned char* data;
        int texWidth;
        int texHeight;
        int texChannels;
};

struct ImgResource {
        bool inuse;
        // info
        VkFormat format;
        VkImageUsageFlags usage;
        VkExtent2D extent;
        VkImageAspectFlags aspect;
        VkClearValue clearValue;

        // alloc
        VkImage image;
        VkImageView imageView;
        VkSampler sampler;
        VmaAllocation allocation;
};

// partition single model into group of indices and materials
struct ModelDataPartition {
        int firstIndex{};
        int indexCount{};
        int materialId{};
};

// cpu submit
struct ModelDataCpu {
        std::vector<Vertex> vertex = {};
        std::vector<uint32_t> indices = {};
        std::vector<ModelDataPartition> modelDataPartition = {};
};

struct MaterialCpu {
        MrtUboData info{};

        TextureData albedoTexture = {};
        TextureData normalTexture = {};
        TextureData aoRoughnessHeightTexture = {};
};

struct MaterialGpu {
        // resource group
        VkDescriptorSet descriptorSet{};

        // uniform
        MrtUboData uboData{};
        VkBuffer uniformBuffer{};
        VmaAllocation uniformAlloc{};
        VmaAllocationInfo uniformAllocInfo{};

        // sampler resource
        ImgResource albedoTex{};             // rgb - albedo, a is unused because this is SNORM
        ImgResource normalTex{};             // rgb - normal, a -
        ImgResource aoRoughnessHeight = {};  // r - ao, g - roughness, b - height, a -
};

// shared state between model handler and renderer
struct ModalState {
        // update by application
        glm::mat4 worldTransform{};
        // populated by renderer
        VmaAllocation vAllocation{};
        VmaAllocation iAllocation{};
        VkBuffer vBuffer{};
        VkBuffer iBuffer{};
        uint32_t indicesSize{};

        std::vector<ModelDataPartition> modelDataPartition{};
};

}  // namespace luna