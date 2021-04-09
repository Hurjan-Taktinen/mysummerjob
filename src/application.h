#pragma once

#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#include "logs/log.h"
#include "config/config.h"

#include <memory>

namespace app
{
class Application final
{
public:
    ~Application();
    void init();

private:
    void initilizeGLFW();
    static void onErrorCallback(int error, const char* description);

private:
    config::BaseConfig m_BaseConfig;
    std::shared_ptr<GLFWwindow> m_Window;

    VkExtent2D m_WindowResolution = {0, 0};
    VkExtent2D m_MonitorResolution = {0, 0};

    Logger m_Log;
};

} // namespace app
