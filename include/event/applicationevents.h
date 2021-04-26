#pragma once

#include "core/workqueue.h"

namespace event
{

struct ApplicationStopEvent final
{
    ApplicationStopEvent(bool stop) : stop(stop) {}
    bool stop = false;
};

}
