#include "core/vulkan/descriptorgen.h"
#include "core/vulkan/utils.h"

#include <stdexcept>
#include <algorithm>

namespace core::vk
{
DescriptorSetGenerator::DescriptorSetGenerator(VkDevice device) :
    m_Device(device)
{
}

void DescriptorSetGenerator::addBinding(
        std::uint32_t binding,
        std::uint32_t descriptorCount,
        VkDescriptorType type,
        VkShaderStageFlags stageFlags,
        VkSampler* sampler)
{
    VkDescriptorSetLayoutBinding b = {};
    b.binding = binding;
    b.descriptorType = type;
    b.descriptorCount = descriptorCount;
    b.stageFlags = stageFlags;
    b.pImmutableSamplers = sampler;

    if(auto [it, inserted] = m_Bindings.emplace(binding, b); !inserted)
    {
        throw std::logic_error("Binding reassignment");
    }
}

VkDescriptorPool DescriptorSetGenerator::generatePool(std::uint32_t maxSets)
{
    VkDescriptorPool pool = VK_NULL_HANDLE;

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(m_Bindings.size());

    for(const auto& [index, bindingLayout] : m_Bindings)
    {
        poolSizes.push_back(
                {bindingLayout.descriptorType,
                 bindingLayout.descriptorCount * 5});
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.flags = 0;
    poolInfo.maxSets = maxSets;
    poolInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    VK_CHECK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &pool));

    return pool;
}

VkDescriptorSetLayout DescriptorSetGenerator::generateLayout()
{
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(m_Bindings.size());

    for(const auto& [index, bindingLayout] : m_Bindings)
    {
        bindings.push_back(bindingLayout);
    }

    VkDescriptorSetLayoutCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.bindingCount = static_cast<std::uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(
            m_Device, &createInfo, nullptr, &layout));

    return layout;
}

VkDescriptorSet DescriptorSetGenerator::generateSet(
        VkDescriptorPool pool, VkDescriptorSetLayout layout)
{
    VkDescriptorSet set = VK_NULL_HANDLE;

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VK_CHECK(vkAllocateDescriptorSets(m_Device, &allocInfo, &set));

    return set;
}

void DescriptorSetGenerator::bind(
        VkDescriptorSet set,
        std::uint32_t binding,
        const std::vector<VkDescriptorBufferInfo>& bufferInfos,
        bool overwriteold)
{
    m_Buffers.bind(
            set,
            binding,
            m_Bindings.at(binding).descriptorType,
            bufferInfos,
            overwriteold);
}

void DescriptorSetGenerator::bind(
        VkDescriptorSet set,
        std::uint32_t binding,
        const std::vector<VkDescriptorImageInfo>& imageInfos,
        bool overwriteold)
{
    m_Images.bind(
            set,
            binding,
            m_Bindings.at(binding).descriptorType,
            imageInfos,
            overwriteold);
}

void DescriptorSetGenerator::updateSetContents()
{
    m_Buffers.setPointers();
    m_Images.setPointers();

    vkUpdateDescriptorSets(
            m_Device,
            static_cast<std::uint32_t>(m_Buffers.writeDescriptors.size()),
            m_Buffers.writeDescriptors.data(),
            0,
            nullptr);

    vkUpdateDescriptorSets(
            m_Device,
            static_cast<std::uint32_t>(m_Images.writeDescriptors.size()),
            m_Images.writeDescriptors.data(),
            0,
            nullptr);
}

} // namespace core::vk
