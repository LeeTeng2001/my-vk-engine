
#include "builder.hpp"

DescriptorBuilder::~DescriptorBuilder() {
    for (const auto &item: _dynamicImageInfo) {
        delete item;
    }
}

VkDescriptorSetLayout DescriptorBuilder::buildSetLayout(int targetSet) {
    auto l = SLog::get();
    if (targetSet < 0) {
        l->error(fmt::format("target set cannot be < 0 {:d}", targetSet));
    } else if (targetSet >= _setInfoList.size()) {
        l->error(fmt::format("target set out of range {:d}", targetSet));
    } else {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(_setInfoList[targetSet].setBinding.size());
        layoutInfo.pBindings = _setInfoList[targetSet].setBinding.data();

        l->vk_res(vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_setInfoList[targetSet].layout));
        return _setInfoList[targetSet].layout;
    }

    return nullptr;
}

VkDescriptorSet DescriptorBuilder::buildSet(int targetSet) {
    auto l = SLog::get();
    if (targetSet < 0) {
        l->error(fmt::format("target set cannot be < 0 {:d}", targetSet));
    } else if (targetSet >= _setInfoList.size()) {
        l->error(fmt::format("target set out of range {:d}", targetSet));
    } else {
        // Create actual descriptor set
        VkDescriptorSetAllocateInfo allocInfo ={};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = _descPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &_setInfoList[targetSet].layout;

        l->vk_res(vkAllocateDescriptorSets(_device, &allocInfo, &_setInfoList[targetSet].set));

        // update set resource info
        for (auto &item: _setInfoList[targetSet].setWrite) {
            item.dstSet = _setInfoList[targetSet].set;
        }
        vkUpdateDescriptorSets(_device, _setInfoList[targetSet].setWrite.size(),
                               _setInfoList[targetSet].setWrite.data(), 0, nullptr);

        return _setInfoList[targetSet].set;
    }

    return nullptr;
}

DescriptorBuilder& DescriptorBuilder::setTotalSet(int total) {
    auto l = SLog::get();
    if (total <= 0) {
        l->error(fmt::format("total set cannot be < 0 {:d}", total));
    } else {
        _setInfoList.resize(total);
    }
    return *this;
}

DescriptorBuilder& DescriptorBuilder::pushDefaultUniformVertex(int targetSet) {
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
        newBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        _setInfoList[targetSet].setBinding.push_back(newBinding);
    }

    return *this;
}

DescriptorBuilder&
DescriptorBuilder::pushDefaultFragmentSamplerBinding(int targetSet) {
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
    if (targetSet < 0) {
        l->error(fmt::format("target set cannot be < 0 {:d}", targetSet));
    } else if (targetSet >= _setInfoList.size()) {
        l->error(fmt::format("target set out of range {:d}", targetSet));
    } else {
        _setInfoList[targetSet].setWrite.clear();
    }
    return *this;
}
DescriptorBuilder&
DescriptorBuilder::pushSetWriteImgSampler(int targetSet, VkImageView imgView, VkSampler sampler) {
    auto l = SLog::get();
    if (targetSet < 0) {
        l->error(fmt::format("target set cannot be < 0 {:d}", targetSet));
    } else if (targetSet >= _setInfoList.size()) {
        l->error(fmt::format("target set out of range {:d}", targetSet));
    } else {
        auto imageInfo = new VkDescriptorImageInfo();
        _dynamicImageInfo.push_back(imageInfo);
        imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo->imageView = imgView;
        imageInfo->sampler = sampler;

        // resource
        VkWriteDescriptorSet setWrite = {};
        setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        setWrite.dstBinding = _setInfoList[targetSet].setWrite.size();
        // setWrite.dstSet = _mrtDescSet; // no set before building
        setWrite.descriptorCount = 1;
        setWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        setWrite.pImageInfo = imageInfo;

        _setInfoList[targetSet].setWrite.push_back(setWrite);
    }
    return *this;
}