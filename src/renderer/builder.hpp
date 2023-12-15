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

    // create the actual set with new set write
    VkDescriptorSet buildSet(int targetSet);

    DescriptorBuilder& setTotalSet(int total);
    DescriptorBuilder& pushDefaultUniformVertex(int targetSet);
    DescriptorBuilder& pushDefaultFragmentSamplerBinding(int targetSet);
    DescriptorBuilder& clearSetWrite(int targetSet);
    DescriptorBuilder& pushSetWriteImgSampler(int targetSet, VkImageView imgView, VkSampler sampler);

private:
    VkDescriptorPool _descPool;
    VkDevice _device;
    // setL -> bindingsL
    vector<SetInfo> _setInfoList;
    vector<VkDescriptorImageInfo*> _dynamicImageInfo;
};