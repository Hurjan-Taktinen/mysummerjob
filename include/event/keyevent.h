#pragma once

#include "event.h"
#include "utils/namedtype.h"
#include "core/eventqueue.h"

namespace event
{

using KeyCode = utils::NamedType<int, struct KeyCodeTag>;
using KeyMods = utils::NamedType<int, struct KeyModsTag>;
using KeyAction = utils::NamedType<int, struct KeyActionTag>;

struct KeyEvent : public EventBase
{
    explicit constexpr KeyEvent(KeyCode code, KeyAction action, KeyMods mods) :
        keyCode(code), action(action), mods(mods)
    {
    }

    KeyCode keyCode;
    KeyMods action;
    KeyAction mods;
};

struct KeyPressedEvent : KeyEvent
{
    explicit constexpr KeyPressedEvent(
            KeyCode code, KeyAction action, KeyMods mods) :
        KeyEvent(code, action, mods)
    {
    }
};

struct KeyReleasedEvent : KeyEvent
{
    explicit constexpr KeyReleasedEvent(
            KeyCode code, KeyAction action, KeyMods mods) :
        KeyEvent(code, action, mods)
    {
    }
};

// struct KeyEventSenderIf : EventService, core::Sub<KeyPressedEvent>
// {
// virtual ~KeyEventSenderIf() = default;
// };

struct KeyEventReceiverIf :
    EventService,
    core::Sub<KeyPressedEvent>,
    core::Sub<KeyReleasedEvent>
{
    virtual ~KeyEventReceiverIf() = default;
};

} // namespace event

