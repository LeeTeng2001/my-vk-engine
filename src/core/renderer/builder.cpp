
#include "builder.hpp"

namespace luna {

DescriptorBuilder::~DescriptorBuilder() {
    for (const auto& item : _dynamicImageInfo) {
        delete item;
    }
    for (const auto& item : _dynamicBufferInfo) {
        delete item;
    }
}

VkDescriptorSetLayout DescriptorBuilder::buildSetLayout(int targetSet) {
    if (!inConstrain(targetSet)) {
        return nullptr;
    }

    auto l = SLog::get();
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(_setInfoList[targetSet].setBinding.size());
    layoutInfo.pBindings = _setInfoList[targetSet].setBinding.data();

    l->vk_res(vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr,
                                          &_setInfoList[targetSet].layout));

    return _setInfoList[targetSet].layout;
}

void DescriptorBuilder::setSetLayout(int targetSet, VkDescriptorSetLayout layout) {
    if (!inConstrain(targetSet)) {
        return;
    }

    _setInfoList[targetSet].layout = layout;
}

VkDescriptorSet DescriptorBuilder::buildSet(int targetSet) {
    if (!inConstrain(targetSet)) {
        return nullptr;
    }

    auto l = SLog::get();
    // Create actual descriptor set
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &_setInfoList[targetSet].layout;

    l->vk_res(vkAllocateDescriptorSets(_device, &allocInfo, &_setInfoList[targetSet].set));

    // update set resource info
    for (auto& item : _setInfoList[targetSet].setWrite) {
        item.dstSet = _setInfoList[targetSet].set;
    }
    vkUpdateDescriptorSets(_device, _setInfoList[targetSet].setWrite.size(),
                           _setInfoList[targetSet].setWrite.data(), 0, nullptr);

    return _setInfoList[targetSet].set;
}

DescriptorBuilder& DescriptorBuilder::setTotalSet(int total) {
    if (total <= 0) {
        auto l = SLog::get();
        l->error(fmt::format("total set cannot be < 0 {:d}", total));
    } else {
        _setInfoList.resize(total);
    }
    return *this;
}

DescriptorBuilder& DescriptorBuilder::pushDefaultUniform(int targetSet,
                                                         VkShaderStageFlags stageFlag) {
    auto l = SLog::get();
    if (targetSet < 0) {
        l->error(fmt::format("target set cannot be < 0 {:d}", targetSet));
    } else if (targetSet >= _setInfoList.size()) {
        l->error(fmt::format("target set out of range {:d}", targetSet));
    } else {
        // binding desc
        VkDescriptorSetLayoutBinding newBinding{};
        newBinding.binding = _setInfoList[targetSet].setBinding.size();
        newBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        newBinding.descriptorCount = 1;
        newBinding.pImmutableSamplers = nullptr;
        newBinding.stageFlags = stageFlag;

        _setInfoList[targetSet].setBinding.push_back(newBinding);
    }

    return *this;
}

DescriptorBuilder& DescriptorBuilder::pushDefaultFragmentSamplerBinding(int targetSet) {
    auto l = SLog::get();
    if (targetSet < 0) {
        l->error(fmt::format("target set cannot be < 0 {:d}", targetSet));
    } else if (targetSet >= _setInfoList.size()) {
        l->error(fmt::format("target set out of range {:d}", targetSet));
    } else {
        // binding desc
        VkDescriptorSetLayoutBinding newBinding{};
        newBinding.binding = _setInfoList[targetSet].setBinding.size();
        newBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        newBinding.descriptorCount = 1;
        newBinding.pImmutableSamplers = nullptr;
        newBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        _setInfoList[targetSet].setBinding.push_back(newBinding);
    }

    return *this;
}

DescriptorBuilder& DescriptorBuilder::clearSetWrite(int targetSet) {
    auto l = SLog::get();
    if (targetSet < -1) {
        l->error(fmt::format("target set cannot be < -1 {:d}", targetSet));
    } else if (targetSet == -1) {
        for (auto& item : _setInfoList) {
            item.setWrite.clear();
        }
    } else if (targetSet >= _setInfoList.size()) {
        l->error(fmt::format("target set out of range {:d}", targetSet));
    } else {
        _setInfoList[targetSet].setWrite.clear();
    }
    return *this;
}

DescriptorBuilder& DescriptorBuilder::pushSetWriteImgSampler(int targetSet, VkImageView imgView,
                                                             VkSampler sampler, int targetBinding) {
    if (!inConstrain(targetSet)) {
        return *this;
    }

    auto imageInfo = new VkDescriptorImageInfo();
    _dynamicImageInfo.push_back(imageInfo);
    imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo->imageView = imgView;
    imageInfo->sampler = sampler;

    // resource
    VkWriteDescriptorSet setWrite = {};
    setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrite.dstBinding =
        targetBinding == -1 ? _setInfoList[targetSet].setWrite.size() : targetBinding;
    setWrite.dstSet = _setInfoList[targetSet].set;  // might not have set before building
    setWrite.descriptorCount = 1;
    setWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    setWrite.pImageInfo = imageInfo;

    _setInfoList[targetSet].setWrite.push_back(setWrite);

    return *this;
}

DescriptorBuilder& DescriptorBuilder::pushSetWriteUniform(int targetSet, VkBuffer buffer,
                                                          int bufferSize, int targetBinding) {
    if (!inConstrain(targetSet)) {
        return *this;
    }

    auto bufferInfo = new VkDescriptorBufferInfo();
    _dynamicBufferInfo.push_back(bufferInfo);
    bufferInfo->buffer = buffer;
    bufferInfo->offset = 0;
    bufferInfo->range = bufferSize;

    // resource
    VkWriteDescriptorSet setWrite = {};
    setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrite.dstBinding =
        targetBinding == -1 ? _setInfoList[targetSet].setWrite.size() : targetBinding;
    setWrite.dstSet = _setInfoList[targetSet].set;  // might not have set before building
    setWrite.descriptorCount = 1;
    setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    setWrite.pBufferInfo = bufferInfo;

    _setInfoList[targetSet].setWrite.push_back(setWrite);

    return *this;
}

bool DescriptorBuilder::inConstrain(int targetSet) {
    auto l = SLog::get();
    if (targetSet < 0) {
        l->error(fmt::format("target set cannot be < 0 {:d}", targetSet));
        return false;
    } else if (targetSet >= _setInfoList.size()) {
        l->error(fmt::format("target set out of range {:d}", targetSet));
        return false;
    }
    return true;
}

}  // namespace luna