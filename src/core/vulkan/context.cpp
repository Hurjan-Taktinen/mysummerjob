#include "context.h"

#include "logs/log.h"
#include "core/vulkan/utils.h"
#include "core/scene/components.h"

#include "imgui/imgui_impl_glfw.h"

#include <cassert>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>

namespace core::vk
{

// -----------------------------------------------------------------------------
//
//

Context::Context(
        std::shared_ptr<GLFWwindow> window,
        scene::Scene* scene,
        entt::registry& registry,
        entt::dispatcher& dispatcher) :
    m_Log(logs::Log::create("Vulkan Context")),
    m_Config(config::Config::getVulkanConfig()),
    m_Window(std::move(window)),
    m_Scene(scene),
    m_Registry(registry),
    m_conn(dispatcher, this)
{
    m_Log->info("Vulkan context created");
}

Context::~Context()
{
    vkDeviceWaitIdle(m_Device->getLogicalDevice());
    m_ui.reset();

    for(size_t i = 0; i < m_UniformBuffer.size(); ++i)
    {
        vmaDestroyBuffer(
                m_Device->getAllocator(),
                m_UniformBuffer[i],
                m_UniformMemory[i]);
    }

    if(m_DescSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(
                m_Device->getLogicalDevice(), m_DescSetLayout, nullptr);
    }

    if(m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(
                m_Device->getLogicalDevice(), m_DescriptorPool, nullptr);
    }

    if(m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(
                m_Device->getLogicalDevice(), m_PipelineLayout, nullptr);
    }
    if(m_Pipelines.obj != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(
                m_Device->getLogicalDevice(), m_Pipelines.obj, nullptr);
    }
    if(m_Pipelines.skybox != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(
                m_Device->getLogicalDevice(), m_Pipelines.skybox, nullptr);
    }

    if(m_Renderpass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(
                m_Device->getLogicalDevice(), m_Renderpass, nullptr);
    }

    for(auto& fence : m_Fences)
    {
        vkDestroyFence(m_Device->getLogicalDevice(), fence, nullptr);
    }

    for(auto& semaphore : m_RenderingCompleteSemaphores)
    {
        vkDestroySemaphore(m_Device->getLogicalDevice(), semaphore, nullptr);
    }

    for(auto& semaphore : m_PresentCompleteSemaphores)
    {
        vkDestroySemaphore(m_Device->getLogicalDevice(), semaphore, nullptr);
    }

    // gen.reset();

    m_Swapchain.reset();
    m_Device.reset();
    m_DebugUtils.reset();

    if(m_Instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_Instance, nullptr);
    }
}

// -----------------------------------------------------------------------------
// -
// - Create instance
// - Setup debugutils
// - ...
// - Descriptorsets
//

void Context::init(VkExtent2D swapchainExtent)
{
    assert(swapchainExtent.width > 0 && swapchainExtent.height > 0);
    m_SwapchainExtent = swapchainExtent;

    createInstance();

    if(m_Config.enableDebugUtils != 0)
    {
        VkDebugUtilsMessageSeverityFlagsEXT messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

        VkDebugUtilsMessageTypeFlagsEXT messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        if(m_Config.enableValidationLayers != 0)
        {
            messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        }

        m_DebugUtils = std::make_unique<DebugUtils>(
                m_Instance, messageSeverity, messageType);
    }

    // Look for GPU's
    VkPhysicalDevice gpu = selectPhysicalDevice();
    m_Device = std::make_unique<Device>(gpu, m_Window);

    {
        std::vector<const char*> requestedExtensions = {};

        VkQueueFlags queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT
                                  | VK_QUEUE_TRANSFER_BIT;

        m_Device->createLogicalDevice(
                m_Instance, requestedExtensions, queueFlags);
    }

    // Create surface
    m_Swapchain = std::make_unique<Swapchain>(m_Instance, m_Device.get());

    // Check present support
    VkBool32 supportPresent = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(
            m_Device->getPhysicalDevice(),
            m_Device->getGraphicsQueueFamily(),
            m_Swapchain->getSurface(),
            &supportPresent);

    if(!supportPresent)
    {
        m_Log->warn("Window surface does not support presenting!");
        assert(false);
    }

    m_Swapchain->create(m_Config.vsync);

    createUniformBuffers();
    createSynchronizationPrimitives();
    createRenderPass();
}

// -----------------------------------------------------------------------------
// When calling generatePipelines rest of the application should have it's
// assets loaded and static descriptors generated
//

void Context::generatePipelines()
{
    setupDescriptors2();
    createGraphicsPipeline();
    m_Swapchain->createFrameBuffers(m_Renderpass);
    allocateCommandBuffers();
    IMGUI_CHECKVERSION();
    m_ui = std::make_unique<ImGuiSetup>(m_Device.get(), m_Renderpass);
}

// -----------------------------------------------------------------------------
//
//

void Context::updateOverlay(float dt)
{
    (void)dt;
    auto& io = ImGui::GetIO();

    m_ui->pushConstants.scale =
            glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    m_ui->pushConstants.translate = glm::vec2(-1.0f);

    // ImGui::ShowDemoWindow();
    // ImGui::SetWindowSize(windowSize);
    // ImGui::SetWindowPos(windowPos);

    // ImGui::ColorEdit3("Color", &color);

    // ImGui::Render();

    m_ui->update();
}

// -----------------------------------------------------------------------------
//
//

void Context::renderFrame(float dt)
{
    updateOverlay(dt);

    // /////////////////////////////////////////

    vkWaitForFences(
            m_Device->getLogicalDevice(),
            1,
            &m_Fences[m_FrameIndex],
            VK_TRUE,
            UINT64_MAX);

    // /////////////////////////////////////////

    uint32_t imageIndex = 0;
    VkResult result = m_Swapchain->acquireNextImage(
            m_PresentCompleteSemaphores[m_FrameIndex], &imageIndex);
    if(result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        vkDeviceWaitIdle(m_Device->getLogicalDevice());
        recreateSwapchain();

        // imageIndex received previously is stale, reacquire (or could just
        // bail out)
        result = m_Swapchain->acquireNextImage(
                m_PresentCompleteSemaphores[m_FrameIndex], &imageIndex);
    }
    else if(result != VK_SUCCESS)
    {
        m_Log->warn(
                "renderFrame::acquireNextImage({}): ",
                imageIndex,
                utils::errorString(result));
        assert(false);
    }

    if(m_ImagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(
                m_Device->getLogicalDevice(),
                1,
                &m_ImagesInFlight[imageIndex],
                VK_TRUE,
                UINT64_MAX);
    }

    m_ImagesInFlight[imageIndex] = m_Fences[m_FrameIndex];

    updateUniformBuffers(dt);
    recordCommandBuffers(imageIndex);

    VkCommandBuffer cmdBuffers[] = {m_RenderingCommandBuffers[imageIndex]};
    VkPipelineStageFlags graphicsWaitStages[] = {
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore graphicsWaitSemaphores[] = {
            m_PresentCompleteSemaphores[m_FrameIndex]};

    VkSemaphore graphicsSignalSemaphores[] = {
            m_RenderingCompleteSemaphores[m_FrameIndex]};

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = graphicsWaitSemaphores;
    submitInfo.pWaitDstStageMask = graphicsWaitStages;
    submitInfo.commandBufferCount = 1; //////
    submitInfo.pCommandBuffers = cmdBuffers;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = graphicsSignalSemaphores;

    vkResetFences(m_Device->getLogicalDevice(), 1, &m_Fences[m_FrameIndex]);

    result = vkQueueSubmit(
            m_Device->getGraphicsQueue(),
            1,
            &submitInfo,
            m_Fences[m_FrameIndex]);

    if(result != VK_SUCCESS)
    {
        m_Log->warn(
                "renderFrame::vkQueueSubmit {}", utils::errorString(result));
        assert(false);
    }

    result = m_Swapchain->queuePresent(
            m_Device->getGraphicsQueue(),
            &imageIndex,
            &m_RenderingCompleteSemaphores[m_FrameIndex]);

    if(result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR
       || m_FrameBufferResized)
    {
        m_FrameBufferResized = false;
        recreateSwapchain();
    }
    else if(result != VK_SUCCESS)
    {
        m_Log->warn(
                "renderFrame::vkQueuePresent {}", utils::errorString(result));
        assert(false);
    }

    m_FrameIndex = imageIndex;
}

// ----------------------------------------------------------------------------
//
//

void Context::createInstance()
{
    std::vector<const char*> instanceExtensions = {};
    std::vector<const char*> instanceLayers = {};

    { // GLFW should ask for VK_KHR_SURFACE and platform depended surface
        uint32_t count = 0;
        const char** glfwExtensions = nullptr;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
        assert(count > 0);
        instanceExtensions.resize(count);
        std::copy(
                glfwExtensions,
                glfwExtensions + count,
                instanceExtensions.begin());
    }

    if(m_Config.enableDebugUtils != 0)
    {
        m_Log->info("DebugUtils enabled");
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    if(m_Config.enableValidationLayers != 0)
    {
        // instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
        m_Log->info("ValidationLayers enabled");
        instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
    }

    { // Print instance layers
        uint32_t count = 0;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        std::vector<VkLayerProperties> layers(count);
        vkEnumerateInstanceLayerProperties(&count, layers.data());

        m_Log->info("Instance layers found({})", count);
        for(auto v : layers)
        {
            m_Log->info("\t{}", v.layerName);
        }
    }
    { // Print instance extensions
        uint32_t count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateInstanceExtensionProperties(
                nullptr, &count, extensions.data());

        m_Log->info("Instance Extensions found({})", count);
        for(auto v : extensions)
        {
            m_Log->info("\t{}", v.extensionName);
        }
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "MySummerJob";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Light Wood Laminate";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
    createInfo.ppEnabledLayerNames = instanceLayers.data();
    createInfo.enabledExtensionCount =
            static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_Instance));
}

// -----------------------------------------------------------------------------
//
//

VkPhysicalDevice Context::selectPhysicalDevice()
{
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(
            m_Instance, &physicalDeviceCount, physicalDevices.data());

    assert(!physicalDevices.empty());

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    for(const auto& gpu : physicalDevices)
    {
        VkPhysicalDeviceProperties gpuProperties;
        vkGetPhysicalDeviceProperties(gpu, &gpuProperties);

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(gpu, &memProperties);

        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
                gpu, &queueFamilyPropertyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(
                queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(
                gpu, &queueFamilyPropertyCount, queueFamilies.data());

        // Pick first available discrete GPU
        if(gpuProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            physicalDevice = gpu;
            break;
        }
    }

    if(physicalDevice == VK_NULL_HANDLE)
    {
        physicalDevice = physicalDevices[0];
    }

#ifdef PRINT_QUEUE_INFO
    m_Log->info("**** GPU's and queues found in system  ****");
    std::cout << "Found ( " << physicalDeviceCount
              << " ) GPUs supporting vulkan" << std::endl;
    for(const auto& device : physicalDevices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        std::cout << "===== " << deviceProperties.deviceName
                  << " =====" << std::endl;

        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
                device, &queueFamilyPropertyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(
                queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(
                device,
                &queueFamilyPropertyCount,
                queueFamilyProperties.data());

        std::cout << std::endl;
        std::cout << "Found ( " << queueFamilyPropertyCount
                  << " ) queue families" << std::endl;
        for(uint32_t i = 0; i < queueFamilyPropertyCount; ++i)
        {
            std::cout << "\tQueue family [" << i
                      << "] supports following operations:" << std::endl;
            if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                std::cout << "\t\tVK_QUEUE_GRAPHICS_BIT" << std::endl;
            }
            if(queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                std::cout << "\t\tVK_QUEUE_COMPUTE_BIT" << std::endl;
            }
            if(queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                std::cout << "\t\tVK_QUEUE_TRANSFER_BIT" << std::endl;
            }
            if(queueFamilyProperties[i].queueFlags
               & VK_QUEUE_SPARSE_BINDING_BIT)
            {
                std::cout << "\t\tVK_QUEUE_SPARSE_BINDING_BIT" << std::endl;
            }
        }
        std::cout << std::endl;
    }
#endif
    return physicalDevice;
}

// ----------------------------------------------------------------------------
//
//

void Context::createSynchronizationPrimitives()
{
    const uint32_t imageCount = m_Swapchain->getImageCount();
    m_PresentCompleteSemaphores.resize(imageCount);
    m_RenderingCompleteSemaphores.resize(imageCount);
    m_Fences.resize(imageCount);
    m_ImagesInFlight.resize(imageCount, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = 0;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = nullptr;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(uint32_t i = 0; i < imageCount; ++i)
    {
        VK_CHECK(vkCreateSemaphore(
                m_Device->getLogicalDevice(),
                &semaphoreInfo,
                nullptr,
                &m_PresentCompleteSemaphores[i]));
        VK_CHECK(vkCreateSemaphore(
                m_Device->getLogicalDevice(),
                &semaphoreInfo,
                nullptr,
                &m_RenderingCompleteSemaphores[i]));
        VK_CHECK(vkCreateFence(
                m_Device->getLogicalDevice(),
                &fenceInfo,
                nullptr,
                &m_Fences[i]));
    }
}

// ----------------------------------------------------------------------------
//
//

void Context::createRenderPass()
{
    std::array<VkAttachmentDescription, 2> attachments = {};
    attachments[0].flags = 0;
    attachments[0].format = m_Swapchain->getImageFormat();
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[1].flags = 0;
    attachments[1].format = VK_FORMAT_D32_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference, 2> attachmentReferences = {};
    attachmentReferences[0].attachment = 0;
    attachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentReferences[1].attachment = 1;
    attachmentReferences[1].layout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkSubpassDescription, 1> subpassDescriptions = {};
    subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[0].colorAttachmentCount = 1;
    subpassDescriptions[0].pColorAttachments = &attachmentReferences[0];
    subpassDescriptions[0].pDepthStencilAttachment = &attachmentReferences[1];

    std::array<VkSubpassDependency, 2> subpassDependencies = {};

    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[0].dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[0].dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
            | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[1].srcAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
            | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderpassInfo = {};
    renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpassInfo.pNext = nullptr;
    renderpassInfo.flags = 0;
    renderpassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderpassInfo.pAttachments = attachments.data();
    renderpassInfo.subpassCount =
            static_cast<uint32_t>(subpassDescriptions.size());
    renderpassInfo.pSubpasses = subpassDescriptions.data();
    renderpassInfo.dependencyCount =
            static_cast<uint32_t>(subpassDependencies.size());
    renderpassInfo.pDependencies = subpassDependencies.data();

    VK_CHECK(vkCreateRenderPass(
            m_Device->getLogicalDevice(),
            &renderpassInfo,
            nullptr,
            &m_Renderpass));
}

// ----------------------------------------------------------------------------
//
//

void Context::createGraphicsPipeline()
{
    const auto bindingDescription = model::VertexPNTC::getBindingDescription();
    const auto attributeDescription =
            model::VertexPNTC::getAttributeDescription();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount =
            static_cast<uint32_t>(attributeDescription.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

    // --

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.pNext = nullptr;
    inputAssembly.flags = 0;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // --

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(m_SwapchainExtent.width);
    viewport.height = static_cast<float>(m_SwapchainExtent.height);
    ;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // --

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_SwapchainExtent;

    // --

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;
    viewportState.flags = 0;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // --

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.pNext = nullptr;
    rasterizer.flags = 0;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;
    rasterizer.lineWidth = 1.0f;

    // --

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.pNext = nullptr;
    multisampling.flags = 0;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // --

    VkPipelineDepthStencilStateCreateInfo depthInfo = {};
    depthInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthInfo.pNext = nullptr;
    depthInfo.flags = 0;
    depthInfo.depthTestEnable = VK_TRUE;
    depthInfo.depthWriteEnable = VK_TRUE;
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthInfo.depthBoundsTestEnable = VK_FALSE;
    depthInfo.stencilTestEnable = VK_FALSE;
    // depthInfo.front;
    // depthInfo.back;
    depthInfo.minDepthBounds = 0.0f;
    depthInfo.maxDepthBounds = 1.0f;

    // --

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = 0xF;

    // --

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType =
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;
    colorBlending.flags = 0;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    // colorBlending.blendConstants;

    // --

    std::array<VkDynamicState, 2> dynStates{
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = nullptr;
    dynamicState.flags = 0;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dynamicState.pDynamicStates = dynStates.data();

    // --

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::vec4);

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = nullptr;
    layoutInfo.flags = 0;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_DescSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VK_CHECK(vkCreatePipelineLayout(
            m_Device->getLogicalDevice(),
            &layoutInfo,
            nullptr,
            &m_PipelineLayout));
    // --

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {};

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pTessellationState = nullptr;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthInfo;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = m_Renderpass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    {
        shaderStages[0] = m_Device->loadShaderFromFile(
                "data/shaders/obj.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
        shaderStages[1] = m_Device->loadShaderFromFile(
                "data/shaders/obj.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    VK_CHECK(vkCreateGraphicsPipelines(
            m_Device->getLogicalDevice(),
            nullptr,
            1,
            &pipelineInfo,
            nullptr,
            &m_Pipelines.obj));

    for(auto& shader : shaderStages)
    {
        vkDestroyShaderModule(
                m_Device->getLogicalDevice(), shader.module, nullptr);
    }

    // --
}

// ----------------------------------------------------------------------------
//
//

void Context::allocateCommandBuffers()
{
    m_RenderingCommandBuffers.resize(m_Swapchain->getImageCount());

    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.pNext = nullptr;
    allocateInfo.commandPool = m_Device->getGraphicsCommandPool();
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = m_Swapchain->getImageCount();

    VK_CHECK(vkAllocateCommandBuffers(
            m_Device->getLogicalDevice(),
            &allocateInfo,
            m_RenderingCommandBuffers.data()));
}

// ----------------------------------------------------------------------------
// Record command buffer for single frame
//

void Context::recordCommandBuffers(uint32_t nextImageIndex)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = m_Renderpass;
    renderPassBeginInfo.framebuffer =
            m_Swapchain->getFrameBuffer(nextImageIndex);
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = m_SwapchainExtent;
    renderPassBeginInfo.clearValueCount =
            static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    // Use opengl like viewport with y-axis pointing up
    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = static_cast<float>(m_SwapchainExtent.height);
    viewport.width = static_cast<float>(m_SwapchainExtent.width);
    viewport.height = -static_cast<float>(m_SwapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_SwapchainExtent;

    {
        VkCommandBuffer cmdBuf = m_RenderingCommandBuffers[nextImageIndex];
        vkBeginCommandBuffer(cmdBuf, &beginInfo);

        vkCmdBeginRenderPass(
                cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(
                cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipelines.obj);

        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

        vkCmdBindDescriptorSets(
                cmdBuf,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_PipelineLayout,
                0,
                1,
                &m_DescriptorSet[nextImageIndex],
                0,
                nullptr);

        // Draw scene
        renderSceneItems(cmdBuf);

        viewport.y = 0;
        viewport.height = static_cast<float>(m_SwapchainExtent.height);

        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

        if(renderImGui)
        {
            m_ui->draw(cmdBuf);
        }

        vkCmdEndRenderPass(cmdBuf);
        VK_CHECK(vkEndCommandBuffer(cmdBuf));
    }
}

// ----------------------------------------------------------------------------
//
//

void Context::renderSceneItems(VkCommandBuffer cmdBuf)
{
    assert(m_Scene);
    VkDeviceSize offsets[] = {0};

    auto view = m_Registry
                        .view<scene::component::VertexInfo,
                              scene::component::Position>();
    for(auto entity : view)
    {
        auto vertexInfo = view.get<scene::component::VertexInfo>(entity);
        auto pos = view.get<scene::component::Position>(entity).pos;
        const auto* vb = &vertexInfo.vertexBuffer;
        const auto& ib = vertexInfo.indexBuffer;
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, vb, offsets);
        vkCmdBindIndexBuffer(cmdBuf, ib, 0, VK_INDEX_TYPE_UINT32);

        vkCmdPushConstants(
                cmdBuf,
                m_PipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(glm::vec4),
                &pos[0]);
        vkCmdDrawIndexed(cmdBuf, vertexInfo.numIndices, 1, 0, 0, 0);
    }
}

// ----------------------------------------------------------------------------
//
//

void Context::createUniformBuffers()
{
    m_UniformBuffer.resize(m_Swapchain->getImageCount());
    m_UniformMemory.resize(m_Swapchain->getImageCount());

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    for(size_t i = 0; i < m_UniformBuffer.size(); ++i)
    {
        m_Device->createBuffer(
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                        | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                bufferSize,
                &m_UniformBuffer[i],
                &m_UniformMemory[i]);
    }
}

// ----------------------------------------------------------------------------
//
//

void Context::updateUniformBuffers(float dt)
{
    UniformBufferObject ubo;

    static float timepass = 0.0f;
    timepass += dt;

    const auto* camera = m_Scene->getCamera();

    ubo.model = glm::mat4(1.0f);
    ubo.model = glm::translate(
            ubo.model, glm::vec3(std::sin(timepass), 0.0f, 0.0f));
    ubo.view = camera->matrices.view;
    ubo.proj = camera->matrices.proj;
    ubo.proj[0][0] *= -1.0f;
    ubo.modelInverse = glm::inverse(ubo.model);
    ubo.projViewInverse = glm::inverse(ubo.view) * glm::inverse(ubo.proj);
    ubo.time = timepass;

    void* data;
    vmaMapMemory(
            m_Device->getAllocator(), m_UniformMemory[m_FrameIndex], &data);
    memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(m_Device->getAllocator(), m_UniformMemory[m_FrameIndex]);
}

// ----------------------------------------------------------------------------
//
//

void Context::setupDescriptors()
{
    { // Descriptor pool
        std::array<VkDescriptorPoolSize, 3> poolSizes = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = m_Swapchain->getImageCount();

        // TODO: why this is needed?
        poolSizes[0].descriptorCount += 5;

        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = m_Swapchain->getImageCount();
        // poolSizes[1].descriptorCount += 5;

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[2].descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.pNext = nullptr;
        poolInfo.flags = 0;
        // poolInfo.maxSets = 5 * static_cast<uint32_t>(m_UniformBuffer.size());
        poolInfo.maxSets = 100;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();

        VK_CHECK(vkCreateDescriptorPool(
                m_Device->getLogicalDevice(),
                &poolInfo,
                nullptr,
                &m_DescriptorPool));
    }

    { // Descripor set layout
        std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings = {};
        layoutBindings[0].binding = 0;
        layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBindings[0].descriptorCount = 1;
        layoutBindings[0].stageFlags =
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[0].pImmutableSamplers = nullptr;

        layoutBindings[1].binding = 1;
        layoutBindings[1].descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBindings[1].descriptorCount =
                1; // TODO count as many as there is textures
        layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[1].pImmutableSamplers = nullptr;

        layoutBindings[2].binding = 2;
        layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        layoutBindings[2].descriptorCount = 1;
        layoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[2].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.flags = 0;
        layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
        layoutInfo.pBindings = layoutBindings.data();

        // TODO AWAW alloc for every image
        VK_CHECK(vkCreateDescriptorSetLayout(
                m_Device->getLogicalDevice(),
                &layoutInfo,
                nullptr,
                &m_DescSetLayout));
    }

    { // Allocate descriptor sets
        m_DescriptorSet.resize(m_Swapchain->getImageCount());
        std::vector<VkDescriptorSetLayout> layouts(
                m_Swapchain->getImageCount(), m_DescSetLayout);

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = layouts.size();
        allocInfo.pSetLayouts = layouts.data();

        VK_CHECK(vkAllocateDescriptorSets(
                m_Device->getLogicalDevice(),
                &allocInfo,
                m_DescriptorSet.data()));
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    const auto& imageInfos = m_Scene->getImageInfos();
    const auto& materialBufferInfos = m_Scene->getbufferinfos();
    std::vector<VkDescriptorBufferInfo> uniformInfos;

    for(const auto& ub : m_UniformBuffer)
    {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = ub;
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;
        uniformInfos.push_back(bufferInfo);
    }

    for(uint32_t i = 0; i < m_Swapchain->getImageCount(); ++i)
    {
        { // Uniform buffer
            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.pNext = nullptr;
            descriptorWrite.dstSet = m_DescriptorSet[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.pBufferInfo = &uniformInfos[i];

            descriptorWrites.push_back(descriptorWrite);
        }

        if(imageInfos.size())
        {
            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.pNext = nullptr;
            descriptorWrite.dstSet = m_DescriptorSet[i];
            descriptorWrite.dstBinding = 1;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorCount =
                    static_cast<uint32_t>(imageInfos.size());
            descriptorWrite.descriptorType =
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.pImageInfo = imageInfos.data();

            descriptorWrites.push_back(descriptorWrite);
        }

        { // Material buffer
            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.pNext = nullptr;
            descriptorWrite.dstSet = m_DescriptorSet[i];
            descriptorWrite.dstBinding = 2;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorCount =
                    static_cast<uint32_t>(materialBufferInfos.size());
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrite.pBufferInfo = materialBufferInfos.data();

            descriptorWrites.push_back(descriptorWrite);
        }
    }

    vkUpdateDescriptorSets(
            m_Device->getLogicalDevice(),
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(),
            0,
            nullptr);
}

void Context::setupDescriptors2()
{
    m_DescriptorSetGenerator = std::make_unique<DescriptorSetGenerator>(
            m_Device->getLogicalDevice());

    m_DescriptorSetGenerator->addBinding(
            0, // binding
            1, // count
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    m_DescriptorSetGenerator->addBinding(
            1, // binding
            1, // count
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT);

    m_DescriptorSetGenerator->addBinding(
            2, // binding
            1, // count
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_FRAGMENT_BIT);

    m_DescriptorPool = m_DescriptorSetGenerator->generatePool(100);
    m_DescSetLayout = m_DescriptorSetGenerator->generateLayout();

    m_DescriptorSet.resize(m_Swapchain->getImageCount());

    for(auto& set : m_DescriptorSet)
    {
        set = m_DescriptorSetGenerator->generateSet(
                m_DescriptorPool, m_DescSetLayout);
    }

    updateDescriptorSets();
}

// ----------------------------------------------------------------------------
//
//

void Context::updateDescriptorSets()
{
    std::vector<VkDescriptorImageInfo> imageInfos = {};
    std::vector<VkDescriptorBufferInfo> materialBufferInfos = {};

    auto view = m_Registry.view<scene::component::RenderInfo>();
    for(auto entity : view)
    {
        auto renderInfo = view.get<scene::component::RenderInfo>(entity);
        imageInfos.insert(
                imageInfos.end(),
                renderInfo.imageInfos.begin(),
                renderInfo.imageInfos.end());
        materialBufferInfos.push_back(renderInfo.buffeInfo);
        break;
        // TODO AWAW this breaks without break
    }

    for(std::size_t i = 0; i < m_DescriptorSet.size(); ++i)
    {
        // Uniform buffer info
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_UniformBuffer[i];
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        m_DescriptorSetGenerator->bind(m_DescriptorSet[i], 0, {bufferInfo});
        m_DescriptorSetGenerator->bind(m_DescriptorSet[i], 1, imageInfos);
        m_DescriptorSetGenerator->bind(
                m_DescriptorSet[i], 2, materialBufferInfos);
    }
    m_DescriptorSetGenerator->updateSetContents();
}

// ----------------------------------------------------------------------------
//
//

void Context::recreateSwapchain()
{
    vkDeviceWaitIdle(m_Device->getLogicalDevice());
    cleanupSwapchain();

    int width, height;
    glfwGetFramebufferSize(m_Window.get(), &width, &height);
    while(width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_Window.get(), &width, &height);
        glfwWaitEvents();
    }

    m_SwapchainExtent = {
            static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    m_Swapchain->create(m_Config.vsync);
    createRenderPass();
    m_Swapchain->createFrameBuffers(m_Renderpass);
    allocateCommandBuffers();
}

// ----------------------------------------------------------------------------
//
//

void Context::cleanupSwapchain()
{
    m_Swapchain->destroyFrameBuffers();
    vkFreeCommandBuffers(
            m_Device->getLogicalDevice(),
            m_Device->getGraphicsCommandPool(),
            static_cast<uint32_t>(m_RenderingCommandBuffers.size()),
            m_RenderingCommandBuffers.data());

    vkDestroyRenderPass(m_Device->getLogicalDevice(), m_Renderpass, nullptr);
}

void Context::onEvent(event::DescriptorSetAllocateEvent const& event)
{
    auto set = m_DescriptorSetGenerator->generateSet(
            m_DescriptorPool, m_DescSetLayout);
    event.callback(set);
}

} // namespace core::vk
