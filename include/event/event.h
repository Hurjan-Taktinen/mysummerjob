#pragma once

#include <mutex>

namespace event
{

enum struct EventType
{
    StopApplication,
    WindowResize, WindowFocus, WindowLostFocus,
    AppTick, AppUpdate, AppRendered,
    KeyPressed, KeyReleased,
    MouseMoved, MouseButtonPressed, MouseButtonReleased
};

struct EventBase
{
    virtual ~EventBase() = default;
};

struct EventService
{
    virtual ~EventService() = default;
    std::mutex mutex;
};

} // namespace event
