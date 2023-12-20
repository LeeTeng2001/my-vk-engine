#pragma once

#include "utils/common.hpp"

struct SetInfo {
    VkDescriptorSetLayout layout;
    VkDescriptorSet set;

    vector<VkDescriptorSetLayoutBinding> setBinding;
    vector<VkWriteDescriptorSet> setWrite;
};

class DescriptorBuilder {
public:
    DescriptorBuilder(VkDevice device, VkDescriptorPool pool): _device(device), _descPool(pool) {};
    ~DescriptorBuilder();

    // Create the descriptor layout
    // will cache internally as set target
    VkDescriptorSetLayout buildSetLayout(int targetSet);

    // Use this if you're updating existing layout
    void setSetLayout(int targetSet, VkDescriptorSetLayout layout);

    // create the actual set with new set write
    VkDescriptorSet buildSet(int targetSet);

    DescriptorBuilder& setTotalSet(int total);
    DescriptorBuilder& pushDefaultUniform(int targetSet, VkShaderStageFlags stageFlag = VK_SHADER_STAGE_VERTEX_BIT);
    DescriptorBuilder& pushDefaultFragmentSamplerBinding(int targetSet);
    DescriptorBuilder& clearSetWrite(int targetSet = -1);
    DescriptorBuilder& pushSetWriteImgSampler(int targetSet, VkImageView imgView, VkSampler sampler, int targetBinding = -1);
    DescriptorBuilder& pushSetWriteUniform(int targetSet, VkBuffer buffer, int bufferSize, int targetBinding = -1);

private:
    VkDescriptorPool _descPool;
    VkDevice _device;
    // setL -> bindingsL
    vector<SetInfo> _setInfoList;
    // TODO: refactor to shared pointer
    vector<VkDescriptorImageInfo*> _dynamicImageInfo;
    vector<VkDescriptorBufferInfo*> _dynamicBufferInfo;

    bool inConstrain(int targetSet);
};