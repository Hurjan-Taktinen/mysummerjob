#pragma once

// #include "buffer.h"
#include "config/config.h"
#include "debugutils.h"
#include "core/model/model.h"
#include "core/texture/texture.h"
#include "core/vulkan/descriptorgen.h"
#include "core/vulkan/device.h"
#include "core/scene/camera.h"
#include "core/scene/scene.h"
#include "swapchain.h"
#include "uilayer.h"

#include "logs/log.h"

#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define PRINT_QUEUE_INFO

namespace core::vk
{
class Context final
{
public:
    Context(const std::shared_ptr<GLFWwindow> window, scene::Scene* scene);

    ~Context();

    void init(VkExtent2D swapchainExtent);
    void renderFrame(float dt);
    void deviceWaitIdle() { vkDeviceWaitIdle(m_Device->getLogicalDevice()); }
    void generatePipelines();

    [[nodiscard]] auto* getDevice() { return m_Device.get(); }
    [[nodiscard]] const auto* getDevice() const { return m_Device.get(); }

    bool m_FrameBufferResized = false;
    bool renderImGui = true;

private:
    void initImGui();
    void updateOverlay(float deltaTime);
    void createInstance();
    [[nodiscard]] VkPhysicalDevice selectPhysicalDevice();
    void createSynchronizationPrimitives();
    void createRenderPass();
    void createGraphicsPipeline();
    void allocateCommandBuffers();
    void recordCommandBuffers(uint32_t nextImageIndex);
    void renderSceneItems(VkCommandBuffer cmdBuf);

    void createUniformBuffers();
    void updateUniformBuffers(float dt);
    void setupDescriptors();
    void setupDescriptors2();

    void recreateSwapchain();
    void cleanupSwapchain();

private:
    struct UniformBufferObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 modelInverse;
        glm::mat4 projViewInverse;
        float time;
    };

    struct
    {
        VkPipeline skybox = VK_NULL_HANDLE;
        VkPipeline obj = VK_NULL_HANDLE;
    } m_Pipelines;

    logs::Logger m_Log;
    config::VulkanConfig m_Config;

    std::shared_ptr<GLFWwindow> m_Window;
    std::unique_ptr<DebugUtils> m_DebugUtils;
    std::unique_ptr<Swapchain> m_Swapchain;
    std::unique_ptr<Device> m_Device;

    scene::Scene* m_Scene = nullptr;

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkRenderPass m_Renderpass = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    uint32_t m_FrameIndex = 0;

    VkExtent2D m_SwapchainExtent = {0, 0};

    std::vector<VkSemaphore> m_PresentCompleteSemaphores;
    std::vector<VkSemaphore> m_RenderingCompleteSemaphores;
    std::vector<VkFence> m_Fences;
    std::vector<VkFence> m_ImagesInFlight;
    std::vector<VkCommandBuffer> m_RenderingCommandBuffers;

    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_DescriptorSet;

    std::unique_ptr<ImGuiOverlay> m_ui;

    std::vector<VkBuffer> m_UniformBuffer;
    std::vector<VmaAllocation> m_UniformMemory;

    // std::optional<DescriptorSetGenerator> gen;
};
} // namespace core::vk
