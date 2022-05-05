#pragma once

#include <functional>
#include <vulkan/vulkan.h>

namespace event
{
struct DescriptorSetAllocateEvent
{
    std::function<void(VkDescriptorSet set)> callback;
};
} // namespace event
