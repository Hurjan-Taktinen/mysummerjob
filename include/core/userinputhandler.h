#pragma once

#include "event/keyevent.h"
#include "core/subsystemregistry.h"

// #include "GLFW/glfw3.h"
// #include "imgui/imgui_impl_glfw.h"

#include <memory>

namespace core
{

class UserInputHandler final :
    public SystemBase,
    public std::enable_shared_from_this<UserInputHandler>
{
    // friend class Application;

public:
    void preinit(core::SystemRegistry& reg) override
    {
        (void)reg;
        // reg.registerSubsystem<event::KeyEventReceiverIf>(shared_from_this());
    }

    void init(const core::SystemRegistry& reg) override { (void)reg; }

private:
};

} // namespace core
