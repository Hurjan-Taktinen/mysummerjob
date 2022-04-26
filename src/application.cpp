#include "application.h"
#include "logs/log.h"
#include "timer/timer.h"
#include "core/scene/components.h"

#include "imgui/imgui_impl_glfw.h"
#include "event/keyevent.h"

namespace app
{

Application::Application() :
    m_Log(logs::Log::create("Application")),
    m_BaseConfig(config::Config::getBaseConfig())
{
}

Application::~Application()
{
    m_VulkanContext->deviceWaitIdle();
    m_Scene->clear();
    m_Scene.reset();
    ImGui_ImplGlfw_Shutdown();
    m_Log->info("Stopping");
    m_VulkanContext.reset();
    m_Camera.reset();
    m_Window.reset();
    glfwTerminate();
}

void Application::run()
{
    try
    {
        initilizeGLFW();
    }
    catch(...)
    {
        m_Log->error("Initialization failed, exiting...");
        return;
    }

    m_Camera = std::make_shared<core::scene::TrackBall>();
    m_Camera->initCamera(m_FrameBufferSize.width, m_FrameBufferSize.height);

    m_Scene = std::make_shared<core::scene::Scene>(m_Registry, m_Camera.get());

    m_VulkanContext = std::make_shared<core::vk::Context>(
            m_Window, m_Scene.get(), m_Registry);
    m_VulkanContext->init(m_FrameBufferSize);
    m_UiLayer = std::make_unique<ui::UiLayer>();

    m_Scene->loadModels(m_VulkanContext->getDevice());

    {
        // append descriptor resources to context
    }

    m_VulkanContext->generatePipelines();

    ImGui_ImplGlfw_InitForVulkan(m_Window.get(), true);

    m_Log->info("Initialization complete, entering mainloop");
    mainloop();
}

void Application::initilizeGLFW()
{
    glfwSetErrorCallback(onErrorCallback);
    glfwInit();

    if(glfwVulkanSupported() == GLFW_FALSE)
    {
        m_Log->critical("No Vulkan support! Exiting... ");
        throw std::runtime_error("No vulkan support");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    auto* monitor = glfwGetPrimaryMonitor();
    const auto* mode = glfwGetVideoMode(monitor);

    m_MonitorResolution.width = static_cast<uint32_t>(mode->width);
    m_MonitorResolution.height = static_cast<uint32_t>(mode->height);

    m_Log->info(
            "Display resolution ({}, {})",
            m_MonitorResolution.width,
            m_MonitorResolution.height);

    m_Window = std::shared_ptr<GLFWwindow>(
            glfwCreateWindow(
                    m_BaseConfig.width,
                    m_BaseConfig.height,
                    "MySummerJob the Game",
                    nullptr,
                    nullptr),
            [](GLFWwindow* window) { glfwDestroyWindow(window); });

    assert(m_Window);

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_Window.get(), &width, &height);

    // If these are zero, window is minimized?
    if(width <= 0 && height <= 0)
    {
        m_Log->warn("Invalid window resolutions ({}, {})", width, height);
        throw std::runtime_error("Invalid resolution");
    }

    m_FrameBufferSize.width = static_cast<uint32_t>(width);
    m_FrameBufferSize.height = static_cast<uint32_t>(height);

    m_Log->info(
            "GLFW Window surface created with size of ({}, {})", width, height);

    glfwSetWindowUserPointer(m_Window.get(), this);
    glfwSetKeyCallback(m_Window.get(), onKeyCallback);
    glfwSetFramebufferSizeCallback(m_Window.get(), onFrameBufferResized);
    glfwSetCursorPosCallback(m_Window.get(), onCursorPositionCallback);
    glfwSetScrollCallback(m_Window.get(), onMouseScrollCallback);
    glfwSetMouseButtonCallback(m_Window.get(), onMouseButtonCallback);
}

// -----------------------------------------------------------------------------
// Mainloop of the application, everything should be synced to this and it
// should be running at framerate of the monitor
//

void Application::mainloop()
{
    while(!glfwWindowShouldClose(m_Window.get()))
    {
        Timer timer;
        glfwPollEvents();
        m_Camera->update(m_FrameTime);

        m_UiLayer->begin();
        { // UI related updates
            auto& io = ImGui::GetIO();

            ImGui::Begin("Performance metrics");
            ImGui::Text(
                    "%.3f ms / %.0f fps", 1000.0f / io.Framerate, io.Framerate);
            ImGui::Text("framecounter: %lu frame", m_FrameCounter);
            ImGui::End();

            ImGui::Begin("Settings");
            // if(auto filename = m_UiLayer->openFileButton("Load Model");
            //    !filename.empty())
            // {
            //     m_Log->info("Loadfile {}", filename);
            //     m_Scene->addModel(m_VulkanContext->getDevice(), filename);
            //     m_VulkanContext->recreateSwapchain();
            // }
            ImGui::End();
        }
        m_UiLayer->end();

        m_VulkanContext->renderFrame(m_FrameTime);

        m_FrameTime = timer.elapsed();
        m_ApprunTime += m_FrameTime;
        m_FrameCounter += 1;
    }
}

// -----------------------------------------------------------------------------
// GLFW Error callback
//

void Application::onErrorCallback(int error, const char* description)
{
    LGWARNING("GLFW Error ({}): {}", error, description);
}

void Application::handleKeyboardInput(int key, int action, int mods)
{
    (void)mods;

    if(action == GLFW_PRESS)
    {
        if(key == GLFW_KEY_ESCAPE)
        {
            glfwSetWindowShouldClose(m_Window.get(), true);
            return;
        }
        if(key == GLFW_KEY_LEFT_SHIFT)
        {
            // TODO TEE PAREMPI
            m_Keyboard.lShift = true;
        }
    }
    else if(action == GLFW_RELEASE)
    {
        if(key == GLFW_KEY_LEFT_SHIFT)
        {
            m_Keyboard.lShift = false;
        }
    }
}

void Application::onKeyCallback(
        GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;

    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureKeyboard)
    {
        return;
    }

    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if(action == GLFW_PRESS)
    {
        app->handleKeyboardInput(key, action, mods);
    }
    else if(action == GLFW_RELEASE)
    {
        app->handleKeyboardInput(key, action, mods);
    }
}

void Application::handleMouseButtonInput(int button, bool isPressed)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
        m_Mouse.left = isPressed;
    }
    else if(button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        m_Mouse.middle = isPressed;
    }
    else if(button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        m_Mouse.right = isPressed;
    }
}

void Application::handleMouseScrollInput(double yoffset)
{
    m_Camera->updateZoom(yoffset);
}

void Application::handleMousePositionInput(double xpos, double ypos)
{
    const glm::vec2 newPosition(xpos, ypos);
    const glm::vec2 delta = m_Mouse.position - newPosition;
    m_Mouse.position = newPosition;

    if(m_Mouse.middle)
    {
        if(m_Keyboard.lShift)
        {
            m_Camera->pan(delta.x, delta.y);
        }
        else
        {
            m_Camera->updateRotation(delta.x, delta.y);
        }
    }
}

void Application::handleFramebufferResize(int width, int height)
{
    m_FrameBufferSize.width = static_cast<uint32_t>(width);
    m_FrameBufferSize.height = static_cast<uint32_t>(height);

    m_VulkanContext->m_FrameBufferResized = true;
}

void Application::onWindowResized(GLFWwindow* window, int width, int height)
{
    // Framebuffer resize cb is all I need
    (void)window;
    (void)width;
    (void)height;
}

void Application::onFrameBufferResized(
        GLFWwindow* window, int width, int height)
{
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->handleFramebufferResize(width, height);
}

void Application::onCursorPositionCallback(
        GLFWwindow* window, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
    {
        return;
    }

    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->handleMousePositionInput(xpos, ypos);
}

void Application::onMouseButtonCallback(
        GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;

    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
    {
        return;
    }

    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if(action == GLFW_PRESS)
    {
        app->handleMouseButtonInput(button, true);
    }
    else if(action == GLFW_RELEASE)
    {
        app->handleMouseButtonInput(button, false);
    }
}

void Application::onMouseScrollCallback(
        GLFWwindow* window, double xoffset, double yoffset)
{
    (void)xoffset;

    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
    {
        return;
    }

    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->handleMouseScrollInput(yoffset);
}
} // namespace app
