#pragma once

#include "stb_image.h"
#include "vk_mem_alloc.h"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "utils/common.hpp"

struct RenderConfig {
    int maxFrameInFlight = 2;
    int windowWidth = 1700;
    int windowHeight = 900;
    VkDebugUtilsMessageSeverityFlagBitsEXT callbackSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
};

struct MrtUboData {
    int textureToggle {};

    void useColor() { textureToggle |= 0b1;}
    void useNormal() { textureToggle |= 0b10;}
};

struct MrtPushConstantData {
    glm::mat4 viewModalTransform;
    glm::mat4 perspectiveTransform;
};

struct PointLight {
    glm::vec4 position;
    glm::vec4 colorAndRadius;
};

struct CompUboData {
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
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 tangents;
    glm::vec3 bitangents;

    static vector<VkVertexInputBindingDescription> getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return {bindingDescription};
    };

    static vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        vector<VkVertexInputAttributeDescription> attributeDescriptions(6);
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
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, color);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(Vertex, tangents);

        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[5].offset = offsetof(Vertex, bitangents);

        return attributeDescriptions;
    }
};

struct TextureData {
    // populated by application
    std::string path;
    stbi_uc* stbRef;
    int texWidth;
    int texHeight;
    int texChannels;
};

struct ImgResource {
    // info
    VkFormat format;
    VkImageUsageFlags usage;
    VkExtent2D extent;
    VkImageAspectFlags aspect;

    // alloc
    VkImage image;
    VkImageView imageView;
    VkSampler sampler;
    VmaAllocation allocation;
};

// cpu submit
struct ModelData {
    vector<Vertex> vertex = {};
    vector<uint32_t> indices = {};
    TextureData albedoTexture = {};
    TextureData normalTexture = {};
};

// shared state between model handler and renderer
struct ModalState {
    // update by application
    glm::mat4 worldTransform{};
    // populated by renderer
    VmaAllocation allocation{};
    VkBuffer vBuffer{};
    VkBuffer iBuffer{};
    uint32_t indicesSize{};

    // uniform
    MrtUboData uboData;
    VkBuffer uniformBuffer;
    VmaAllocation uniformAlloc{};
    VmaAllocationInfo uniformAllocInfo{};

    ImgResource albedoTex{};
    ImgResource normalTex{};

    VkDescriptorSet descriptorSet{};
};

