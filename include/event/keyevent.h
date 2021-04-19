#pragma once

#include "event.h"
#include "utils/namedtype.h"
#include "core/workqueue.h"

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

struct KeyPressedEventIf :
    virtual public EventService,
    core::Sub<KeyPressedEvent>
{
    virtual ~KeyPressedEventIf() = default;
    using core::Sub<KeyPressedEvent>::handleEvent;
};

struct KeyReleasedEventIf :
    virtual public EventService,
    core::Sub<KeyReleasedEvent>
{
    virtual ~KeyReleasedEventIf() = default;
    using core::Sub<KeyReleasedEvent>::handleEvent;
};

} // namespace event

