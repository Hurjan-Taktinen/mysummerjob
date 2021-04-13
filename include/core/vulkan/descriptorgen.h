#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace core::vk
{

class DescriptorSetGenerator
{
public:
    DescriptorSetGenerator(VkDevice device);
    void addBinding(
            std::uint32_t binding,
            std::uint32_t descriptorCount,
            VkDescriptorType type,
            VkShaderStageFlags stageFlags,
            VkSampler* sampler = nullptr);

    VkDescriptorPool generatePool(std::uint32_t maxSets = 1);
    VkDescriptorSetLayout generateLayout();
    VkDescriptorSet generateSet(
            VkDescriptorPool pool, VkDescriptorSetLayout layout);

    void bind(
            VkDescriptorSet set,
            std::uint32_t binding,
            const std::vector<VkDescriptorBufferInfo>& bufferInfos,
            bool overwriteold = false);
    void bind(
            VkDescriptorSet set,
            std::uint32_t binding,
            const std::vector<VkDescriptorImageInfo>& imageInfos,
            bool overwriteold = false);

    void updateSetContents();

    template<class T, std::uint32_t offset>
    struct WriteInfo
    {
        std::vector<VkWriteDescriptorSet> writeDescriptors;
        std::vector<std::vector<T>> contents;

        void setPointers()
        {
            for(std::size_t i = 0; i < writeDescriptors.size(); ++i)
            {
                T** dest = reinterpret_cast<T**>(
                        reinterpret_cast<std::uint8_t*>(&writeDescriptors[i])
                        + offset);
                *dest = contents[i].data();
            }
        }

        void bind(
                VkDescriptorSet set,
                std::uint32_t binding,
                VkDescriptorType type,
                const std::vector<T>& info,
                bool overwriteold = false)
        {
            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.pNext = nullptr;
            descriptorWrite.dstSet = set;
            descriptorWrite.dstBinding = binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorCount = static_cast<std::uint32_t>(
                    info.size());
            descriptorWrite.descriptorType = type;
            descriptorWrite.pImageInfo = nullptr;
            descriptorWrite.pBufferInfo = nullptr;
            descriptorWrite.pTexelBufferView = nullptr;

            if(overwriteold)
            {
                for(std::size_t i = 0; i < writeDescriptors.size(); ++i)
                {
                    if(writeDescriptors[i].dstBinding == binding
                       && writeDescriptors[i].dstSet == set)
                    {
                        writeDescriptors[i] = descriptorWrite;
                        contents[i] = info;
                        return;
                    }
                }
            }

            // if(!overwriteold)
            {
                writeDescriptors.push_back(descriptorWrite);
                contents.push_back(info);
            }
        }
    };

private:
    VkDevice m_Device;

    std::unordered_map<std::uint32_t, VkDescriptorSetLayoutBinding> m_Bindings;
    WriteInfo<
            VkDescriptorBufferInfo,
            offsetof(VkWriteDescriptorSet, pBufferInfo)>
            m_Buffers;
    WriteInfo<VkDescriptorImageInfo, offsetof(VkWriteDescriptorSet, pImageInfo)>
            m_Images;
};

}; // namespace core::vk
