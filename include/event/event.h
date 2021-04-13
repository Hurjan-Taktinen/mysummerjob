#pragma once

#include <mutex>

namespace event
{

enum struct Eventtypes
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
