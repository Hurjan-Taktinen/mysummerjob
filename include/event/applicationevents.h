#pragma once

#include "event/event.h"
#include "core/workqueue.h"

namespace event
{

struct ApplicationStopEvent final : event::EventBase
{
    ApplicationStopEvent(bool stop) : stop(stop) {}
    bool stop = false;
};

struct ApplicationEventIf :
    virtual public event::EventService,
    public Sub<ApplicationStopEvent>
{
    virtual ~ApplicationEventIf() = default;
    using Sub<ApplicationStopEvent>::handleEvent;
};

}
