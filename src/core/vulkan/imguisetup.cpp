#include "imguisetup.h"

#include "core/model/vertex.h"

namespace core::vk
{
ImGuiSetup::ImGuiSetup(Device* device, VkRenderPass renderpass) :
    m_Log(logs::Log::create("Imgui Overlay")), m_Device(device)
{
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    unsigned char* fontData;
    int texWidth = 0, texHeight = 0;
    // ImFont* font;

    io.Fonts->AddFontFromFileTTF("data/fonts/Roboto/Roboto-Medium.ttf", 16.0f);
    io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

    VkDeviceSize bufferSize = texWidth * texHeight * 4;

    m_FontTexture.loadFromBuffer(
            m_Device,
            fontData,
            bufferSize,
            VK_FORMAT_R8G8B8A8_UNORM,
            texWidth,
            texHeight);

    io.Fonts->SetTexID((void*)&m_FontTexture);

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameBorderSize = 0.0f;
    style.WindowBorderSize = 0.0f;

    std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};

    {
        VkDescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreateInfo.pNext = nullptr;
        poolCreateInfo.flags = 0;
        poolCreateInfo.maxSets = 1;
        poolCreateInfo.poolSizeCount = 1;
        poolCreateInfo.pPoolSizes = poolSizes.data();
        VK_CHECK(vkCreateDescriptorPool(
                m_Device->getLogicalDevice(),
                &poolCreateInfo,
                nullptr,
                &m_DescriptorPool));
    }

    {
        VkDescriptorSetLayoutBinding setLayoutBinding = {};
        setLayoutBinding.binding = 0;
        setLayoutBinding.descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        setLayoutBinding.descriptorCount = 1;
        setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        setLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.pNext = nullptr;
        layoutCreateInfo.flags = 0;
        layoutCreateInfo.bindingCount = 1;
        layoutCreateInfo.pBindings = &setLayoutBinding;

        VK_CHECK(vkCreateDescriptorSetLayout(
                m_Device->getLogicalDevice(),
                &layoutCreateInfo,
                nullptr,
                &m_DescriptorSetLayout));
    }

    {
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
        descriptorSetAllocateInfo.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.pNext = nullptr;
        descriptorSetAllocateInfo.descriptorPool = m_DescriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        descriptorSetAllocateInfo.pSetLayouts = &m_DescriptorSetLayout;

        VK_CHECK(vkAllocateDescriptorSets(
                m_Device->getLogicalDevice(),
                &descriptorSetAllocateInfo,
                &m_DescriptorSet));

        VkWriteDescriptorSet writeDescriptorSet = {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.pNext = nullptr;
        writeDescriptorSet.dstSet = m_DescriptorSet;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.pImageInfo = &(*m_FontTexture.descriptor);
        vkUpdateDescriptorSets(
                m_Device->getLogicalDevice(),
                1,
                &writeDescriptorSet,
                0,
                nullptr);
    }

    {
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstantBlock);

        VkPipelineLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.pNext = nullptr;
        layoutInfo.flags = 0;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &m_DescriptorSetLayout;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstantRange;

        VK_CHECK(vkCreatePipelineLayout(
                m_Device->getLogicalDevice(),
                &layoutInfo,
                nullptr,
                &m_PipelineLayout));
    }

    {
        const auto bindingDescription =
                model::VertexImGui::getBindingDescription();
        const auto attributeDescription =
                model::VertexImGui::getAttributeDescription();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType =
                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.pNext = nullptr;
        vertexInputInfo.flags = 0;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount =
                static_cast<uint32_t>(attributeDescription.size());
        vertexInputInfo.pVertexAttributeDescriptions =
                attributeDescription.data();

        // --

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType =
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.pNext = nullptr;
        inputAssembly.flags = 0;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // --

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType =
                VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.pNext = nullptr;
        viewportState.flags = 0;
        viewportState.viewportCount = 1;
        // viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        // viewportState.pScissors = &scissor;

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
        depthInfo.depthTestEnable = VK_FALSE;
        depthInfo.depthWriteEnable = VK_FALSE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthInfo.depthBoundsTestEnable = VK_FALSE;
        depthInfo.stencilTestEnable = VK_FALSE;
        depthInfo.front = depthInfo.back;
        depthInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
        depthInfo.minDepthBounds = 0.0f;
        depthInfo.maxDepthBounds = 1.0f;

        // --

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor =
                VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor =
                VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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
        dynamicState.sType =
                VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.pNext = nullptr;
        dynamicState.flags = 0;
        dynamicState.dynamicStateCount =
                static_cast<uint32_t>(dynStates.size());
        dynamicState.pDynamicStates = dynStates.data();

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
        pipelineInfo.renderPass = renderpass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        {
            shaderStages[0] = m_Device->loadShaderFromFile(
                    "data/shaders/ui_shader.vert.spv",
                    VK_SHADER_STAGE_VERTEX_BIT),
            shaderStages[1] = m_Device->loadShaderFromFile(
                    "data/shaders/ui_shader.frag.spv",
                    VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        VK_CHECK(vkCreateGraphicsPipelines(
                m_Device->getLogicalDevice(),
                nullptr,
                1,
                &pipelineInfo,
                nullptr,
                &m_Pipeline));

        for(auto& shader : shaderStages)
        {
            vkDestroyShaderModule(
                    m_Device->getLogicalDevice(), shader.module, nullptr);
        }
    }
}

ImGuiSetup::~ImGuiSetup()
{
    ImGui::DestroyContext();
    if(m_VertexBuffer)
    {
        vmaDestroyBuffer(
                m_Device->getAllocator(), m_VertexBuffer, m_VertexMemory);
    }
    if(m_IndexBuffer)
    {
        vmaDestroyBuffer(
                m_Device->getAllocator(), m_IndexBuffer, m_IndexMemory);
    }
    if(m_Pipeline)
    {
        vkDestroyPipeline(m_Device->getLogicalDevice(), m_Pipeline, nullptr);
    }
    if(m_PipelineLayout)
    {
        vkDestroyPipelineLayout(
                m_Device->getLogicalDevice(), m_PipelineLayout, nullptr);
    }
    if(m_DescriptorSetLayout)
    {
        vkDestroyDescriptorSetLayout(
                m_Device->getLogicalDevice(), m_DescriptorSetLayout, nullptr);
    }
    if(m_DescriptorPool)
    {
        vkDestroyDescriptorPool(
                m_Device->getLogicalDevice(), m_DescriptorPool, nullptr);
    }
}

void ImGuiSetup::setDarkTheme()
{
    auto& colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

    // Headers
    colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Buttons
    colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
    colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
    colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
}

void ImGuiSetup::update()
{
    auto* drawData = ImGui::GetDrawData();

    if(drawData)
    {
        VkDeviceSize vertexBufferSize =
                drawData->TotalVtxCount * sizeof(ImDrawVert);
        VkDeviceSize indexBufferSize =
                drawData->TotalIdxCount * sizeof(ImDrawIdx);

        // assert(vertexBufferSize > 0 && indexBufferSize > 0);

        // Check if ImGui buffers need to be updated
        if(m_VertexBuffer == VK_NULL_HANDLE
           || vertexCount != drawData->TotalVtxCount
           || m_IndexBuffer == VK_NULL_HANDLE
           || drawData->TotalIdxCount != indexCount)
        {
            vertexCount = drawData->TotalVtxCount;
            indexCount = drawData->TotalIdxCount;

            if(vertexBufferSize == 0 || indexBufferSize == 0)
                return;

            // TODO Instead of devicewaitidle could this use a ringbuffer to
            // swap active buffers
            vkDeviceWaitIdle(m_Device->getLogicalDevice());
            if(m_VertexBuffer)
            {
                vmaDestroyBuffer(
                        m_Device->getAllocator(),
                        m_VertexBuffer,
                        m_VertexMemory);
            }
            if(m_IndexBuffer)
            {
                vmaDestroyBuffer(
                        m_Device->getAllocator(), m_IndexBuffer, m_IndexMemory);
            }

            m_Device->createBuffer(
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                    vertexBufferSize,
                    &m_VertexBuffer,
                    &m_VertexMemory);

            m_Device->createBuffer(
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                    indexBufferSize,
                    &m_IndexBuffer,
                    &m_IndexMemory);
        }

        void* vertexDst;
        void* indexDst;

        vmaMapMemory(m_Device->getAllocator(), m_VertexMemory, &vertexDst);
        vmaMapMemory(m_Device->getAllocator(), m_IndexMemory, &indexDst);

        for(int32_t n = 0; n < drawData->CmdListsCount; ++n)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            std::memcpy(
                    vertexDst,
                    cmdList->VtxBuffer.Data,
                    cmdList->VtxBuffer.Size * sizeof(ImDrawVert));

            std::memcpy(
                    indexDst,
                    cmdList->IdxBuffer.Data,
                    cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

            vertexDst = static_cast<ImDrawVert*>(vertexDst)
                        + cmdList->VtxBuffer.Size;
            indexDst =
                    static_cast<ImDrawIdx*>(indexDst) + cmdList->IdxBuffer.Size;
        }

        vmaUnmapMemory(m_Device->getAllocator(), m_VertexMemory);
        vmaUnmapMemory(m_Device->getAllocator(), m_IndexMemory);

        vmaFlushAllocation(
                m_Device->getAllocator(), m_VertexMemory, 0, VK_WHOLE_SIZE);
        vmaFlushAllocation(
                m_Device->getAllocator(), m_IndexMemory, 0, VK_WHOLE_SIZE);
    }
}

void ImGuiSetup::draw(VkCommandBuffer cmdBuf)
{

    if(vertexCount == 0 || indexCount == 0)
        return;

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    vkCmdBindDescriptorSets(
            cmdBuf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_PipelineLayout,
            0,
            1,
            &m_DescriptorSet,
            0,
            nullptr);

    const VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &m_VertexBuffer, offsets);
    vkCmdBindIndexBuffer(cmdBuf, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdPushConstants(
            cmdBuf,
            m_PipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(PushConstantBlock),
            &pushConstants);

    auto* drawData = ImGui::GetDrawData();
    int32_t vertexOffset = 0;
    int32_t indexOffset = 0;

    for(int32_t j = 0; j < drawData->CmdListsCount; j++)
    {
        const ImDrawList* cmd_list = drawData->CmdLists[j];
        for(int32_t k = 0; k < cmd_list->CmdBuffer.Size; k++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[k];
            VkRect2D scissorRect;
            scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
            scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
            scissorRect.extent.width =
                    (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
            scissorRect.extent.height =
                    (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
            vkCmdSetScissor(cmdBuf, 0, 1, &scissorRect);
            vkCmdDrawIndexed(
                    cmdBuf, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
            indexOffset += pcmd->ElemCount;
        }
        vertexOffset += cmd_list->VtxBuffer.Size;
    }
}
} // namespace core::vk
