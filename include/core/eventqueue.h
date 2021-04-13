#pragma once

#include "riften/thiefpool.hpp"
#include "utils/singleton.h"
#include "event/event.h"

#include <memory>

namespace core
{

template<class Event>
struct Sub
{
    virtual ~Sub() = default;
    virtual void handleEvent(const Event&) = 0;
};

class EventQueue final : public utils::Singleton<EventQueue>
{
    friend class utils::Singleton<EventQueue>;

public:
    template<class Sub, class Event>
    void unicast(const std::shared_ptr<Sub>& target, const Event& event)
    {
        auto ret = m_Pool.enqueue(
                [event = event,
                 target = std::dynamic_pointer_cast<Sub>(target)] {
                    target->handleEvent(event);
                });

        (void)ret;
    }

private:
    riften::Thiefpool m_Pool;
};

[[nodiscard]] auto& getEventQueue()
{
    return EventQueue::getInstance();
}

} // namespace core

