#pragma once

#include "event/keyevent.h"
#include "core/subsystemregistry.h"

#include <memory>

namespace core
{

class UserInputHandler :
    public SystemBase,
    public event::KeyEventReceiverIf,
    public std::enable_shared_from_this<UserInputHandler>
{
public:
    void preinit(core::SystemRegistry& reg) override
    {
        reg.registerSubsystem<event::KeyEventReceiverIf>(shared_from_this());
    }

    void init(const core::SystemRegistry& reg) override {
        (void)reg;
    }

private:
    void handleEvent(event::KeyPressedEvent&&) override {}
    void handleEvent(event::KeyReleasedEvent&&) override {}
};

} // namespace core
