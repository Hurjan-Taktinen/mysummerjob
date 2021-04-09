#include "application.h"
#include "logs/log.h"

namespace app
{

Application::~Application()
{
    m_Log->info("Stopping");
    m_Window.reset();
    glfwTerminate();
}

void Application::init()
{
    m_Log = Log::create("application");
    m_BaseConfig = config::Config::getBaseConfig();

    try
    {
        initilizeGLFW();
    }
    catch(...)
    {
        m_Log->error("Initialization failed, exiting...");
        return;
    }
}

// -----------------------------------------------------------------------------
//
//

void Application::initilizeGLFW()
{
    glfwSetErrorCallback(onErrorCallback);
    glfwInit();

    if(glfwVulkanSupported() == GLFW_FALSE)
    {
        m_Log->warn("No Vulkan support! Exiting... ");
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
    glfwGetWindowSize(m_Window.get(), &width, &height);

    // If these are zero, window is minimized?
    if(width <= 0 && height <= 0)
    {
        m_Log->warn("Invalid window resolutions ({}, {})", width, height);
        throw std::runtime_error("Invalid resolution");
    }

    m_WindowResolution.width = static_cast<uint32_t>(width);
    m_WindowResolution.height = static_cast<uint32_t>(height);

    m_Log->info(
            "GLFW Window surface created with size of ({}, {})", width, height);
}

// -----------------------------------------------------------------------------
// GLFW Error callback
//

void Application::onErrorCallback(int error, const char* description)
{
    WARNING("GLFW Error ({}): {}", error, description);
}

} // namespace app
