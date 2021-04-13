#include "core/eventqueue.h"

namespace core
{

EventQueue& getEventQueue()
{
    return EventQueue::getInstance();
}

} // namespace core
