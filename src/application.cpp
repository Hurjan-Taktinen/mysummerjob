#include "application.h"
#include "logs/log.h"
#include "timer/timer.h"
#include "core/scene/components.h"

#include "imgui/imgui_impl_glfw.h"
#include "event/keyevent.h"

namespace app
{

Application::Application() :
    _log(logs::Log::create("Application")),
    _baseConfig(config::Config::getBaseConfig())
{
}

Application::~Application()
{
    _vulkanContext->deviceWaitIdle();
    _scene->clear();
    _scene.reset();
    ImGui_ImplGlfw_Shutdown();
    _log->info("Stopping");
    _vulkanContext.reset();
    _camera.reset();
    _window.reset();
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
        _log->error("Initialization failed, exiting...");
        return;
    }

    _camera = std::make_shared<core::scene::TrackBall>(_dispatcher);
    _camera->setProjection(_frameBufferSize.width, _frameBufferSize.height);

    _scene = std::make_shared<core::scene::Scene>(_registry, _camera.get());

    _vulkanContext = std::make_shared<core::vk::Context>(
            _window, _scene.get(), _registry, _dispatcher);
    _vulkanContext->init(_frameBufferSize);
    _uiLayer = std::make_unique<ui::UiLayer>(_dispatcher);

    _scene->loadModels(_vulkanContext->getDevice());

    {
        // append descriptor resources to context
    }

    _vulkanContext->generatePipelines();

    ImGui_ImplGlfw_InitForVulkan(_window.get(), true);

    _log->info("Initialization complete, entering mainloop");
    mainloop();
}

void Application::initilizeGLFW()
{
    glfwSetErrorCallback(onErrorCallback);
    glfwInit();

    if(glfwVulkanSupported() == GLFW_FALSE)
    {
        _log->critical("No Vulkan support! Exiting... ");
        throw std::runtime_error("No vulkan support");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    auto* monitor = glfwGetPrimaryMonitor();
    const auto* mode = glfwGetVideoMode(monitor);

    _monitorResolution.width = static_cast<uint32_t>(mode->width);
    _monitorResolution.height = static_cast<uint32_t>(mode->height);

    _log->info(
            "Display resolution ({}, {})",
            _monitorResolution.width,
            _monitorResolution.height);

    _window = std::shared_ptr<GLFWwindow>(
            glfwCreateWindow(
                    _baseConfig.width,
                    _baseConfig.height,
                    "MySummerJob the Game",
                    nullptr,
                    nullptr),
            [](GLFWwindow* window) { glfwDestroyWindow(window); });

    assert(_window);

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(_window.get(), &width, &height);

    // If these are zero, window is minimized?
    if(width <= 0 && height <= 0)
    {
        _log->warn("Invalid window resolutions ({}, {})", width, height);
        throw std::runtime_error("Invalid resolution");
    }

    _frameBufferSize.width = static_cast<uint32_t>(width);
    _frameBufferSize.height = static_cast<uint32_t>(height);

    _log->info(
            "GLFW Window surface created with size of ({}, {})", width, height);

    glfwSetWindowUserPointer(_window.get(), this);
    glfwSetKeyCallback(_window.get(), onKeyCallback);
    glfwSetFramebufferSizeCallback(_window.get(), onFrameBufferResized);
    glfwSetCursorPosCallback(_window.get(), onCursorPositionCallback);
    glfwSetScrollCallback(_window.get(), onMouseScrollCallback);
    glfwSetMouseButtonCallback(_window.get(), onMouseButtonCallback);
}

// -----------------------------------------------------------------------------
// Mainloop of the application, everything should be synced to this and it
// should be running at framerate of the monitor
//

void Application::mainloop()
{
    while(!glfwWindowShouldClose(_window.get()))
    {
        Timer timer;
        glfwPollEvents();

        // Dispatch all enqueued events
        _dispatcher.update();

        _camera->update(_frameTime);

        _uiLayer->begin();
        { // UI related updates
            auto& io = ImGui::GetIO();

            ImGui::Begin("Performance metrics");
            ImGui::Text(
                    "%.3f ms / %.0f fps", 1000.0f / io.Framerate, io.Framerate);
            ImGui::Text("framecounter: %lu frame", _frameCounter);
            ImGui::End();

            // ImGui::Begin("Settings");
            // if(auto filename = m_UiLayer->openFileButton("Load Model");
            //    !filename.empty())
            // {
            //     m_Log->info("Loadfile {}", filename);
            //     m_Scene->addModel(m_VulkanContext->getDevice(), filename);
            //     m_VulkanContext->recreateSwapchain();
            // }
            // ImGui::End();
        }
        _uiLayer->end();

        _scene->updatePositions(_apprunTime);
        _vulkanContext->renderFrame(_frameTime);

        _frameTime = timer.elapsed();
        _apprunTime += _frameTime;
        _frameCounter += 1;
    }
}

// -----------------------------------------------------------------------------
// GLFW Error callback
//

void Application::onErrorCallback(int error, const char* description)
{
    LGWARNING("GLFW Error ({}): {}", error, description);
}

void Application::handleKeyboardInput(int key, bool isPressed, int mods)
{
    (void)mods;

    if(key == GLFW_KEY_ESCAPE && isPressed)
    {
        glfwSetWindowShouldClose(_window.get(), true);
        return;
    }
    if(key == GLFW_KEY_LEFT_SHIFT)
    {
        _keyboard.lShift = isPressed;
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
        app->handleKeyboardInput(key, true, mods);
    }
    else if(action == GLFW_RELEASE)
    {
        app->handleKeyboardInput(key, false, mods);
    }
}

void Application::handleMouseButtonInput(int button, bool isPressed)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
        _mouse.left = isPressed;
    }
    else if(button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        _mouse.middle = isPressed;
    }
    else if(button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        _mouse.right = isPressed;
    }
}

void Application::handleMouseScrollInput(double yoffset)
{
    _camera->updateZoom(yoffset);
}

void Application::handleMousePositionInput(double xpos, double ypos)
{
    const glm::vec2 newPosition(xpos, ypos);
    const glm::vec2 delta = _mouse.position - newPosition;
    _mouse.position = newPosition;

    if(_mouse.middle)
    {
        if(_keyboard.lShift)
        {
            _camera->pan(delta.x, delta.y);
        }
        else
        {
            _camera->updateRotation(delta.x, delta.y);
        }
    }
}

void Application::handleFramebufferResize(int width, int height)
{
    _frameBufferSize.width = static_cast<uint32_t>(width);
    _frameBufferSize.height = static_cast<uint32_t>(height);

    _camera->setProjection(_frameBufferSize.width, _frameBufferSize.height);
    _vulkanContext->m_FrameBufferResized = true;
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
