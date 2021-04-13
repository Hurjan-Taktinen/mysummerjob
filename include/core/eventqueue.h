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
    virtual void handleEvent(Event&&) = 0;
};

class EventQueue final : public utils::Singleton<EventQueue>
{
    friend class utils::Singleton<EventQueue>;

public:
    template<class Sub, class Event>
    void unicast(const std::shared_ptr<Sub>& target, Event&& event)
    {
        auto f = [event = std::move(event),
                  target = target]() mutable {
            std::unique_lock<std::mutex> lock(target->mutex);
            target->handleEvent(std::move(event));
        };

        [[maybe_unused]] auto ret = m_Pool.enqueue(std::move(f));
    }

private:
    EventQueue() = default;

    riften::Thiefpool m_Pool;
};

[[nodiscard]] EventQueue& getEventQueue();

} // namespace core

namespace event
{
template<class Receiver, class Event>
void sendUnicast(const std::shared_ptr<Receiver>& receiver, Event&& event)
{
    core::getEventQueue().unicast(receiver, std::move(event));
}

// enum struct EventType
// {
// Key,
// Tick
// };

// template<class Event>
// void sendMulticast(EventType type, const Event& event)
// {
// core::getEventQueue().multicast(
// type, std::move(event));
// }

} // namespace event

