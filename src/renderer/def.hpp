#pragma once

#include "utils/common.hpp"

struct RenderConfig {
    int maxFrameInFlight = 2;
    int windowWidth = 1700;
    int windowHeight = 900;
    VkDebugUtilsMessageSeverityFlagBitsEXT callbackSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
};

struct MrtPushConstantData {
    glm::mat4 viewTransform;
    glm::vec3 sunPos;
    unsigned long long time;
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

    static vector<VkVertexInputBindingDescription> getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return {bindingDescription};
    };

    static vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        vector<VkVertexInputAttributeDescription> attributeDescriptions(4);
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

        return attributeDescriptions;
    }
};

struct CpuImgResource {
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

struct ModelData {
    // populated by application
    vector<Vertex> vertex = {};
    vector<uint32_t> indices = {};
    CpuImgResource albedoTexture;
    // populated by renderer
    VkBuffer vBuffer{};
    VkBuffer iBuffer{};
    ImgResource gpuAlbedoTex;
};
