#pragma once

#include "riften/thiefpool.hpp"
#include "utils/singleton.h"
#include "event/event.h"

#include <memory>
#include <map>
#include <mutex>

namespace core
{

template<class Event>
struct Sub
{
    virtual ~Sub() = default;
    virtual void handleEvent(Event&&) = 0;
};

class WorkQueue final : public utils::Singleton<WorkQueue>
{
    friend class utils::Singleton<WorkQueue>;

public:
    template<class Sub, class Event>
    void unicast(const std::shared_ptr<Sub>& target, Event&& event)
    {
        auto f = [event = std::move(event), target = target]() mutable {
            std::unique_lock<std::mutex> lock(target->mutex);
            target->handleEvent(std::move(event));
        };

        [[maybe_unused]] auto ret = m_Pool.enqueue(std::move(f));
    }

    template<class Event>
    void multicast(event::EventType type, Event&& event)
    {
        auto [f, l] = m_Subs.equal_range(type);

        for(auto i = f; i != l; ++i)
        {
            if(auto ptr = i->second.lock(); ptr)
            {
                auto f = [event = std::move(event),
                          mptr = ptr,
                          tptr = std::dynamic_pointer_cast<Sub<Event>>(
                                  ptr)]() mutable {
                    std::unique_lock<std::mutex> lock(mptr->mutex);
                    tptr->handleEvent(std::move(event));
                };

                [[maybe_unused]] auto ret = m_Pool.enqueue(std::move(f));
            }
        }
    }

    template<class Sub>
    void subscribeToMulticast(
            event::EventType type, const std::shared_ptr<Sub>& sub)
    {
        std::unique_lock lock {m_Mutex};
        m_Subs.emplace(type, sub);
    }

private:
    WorkQueue() = default;
    riften::Thiefpool m_Pool;
    std::mutex m_Mutex;

    std::multimap<event::EventType, std::weak_ptr<event::EventService>> m_Subs;
};

[[nodiscard]] WorkQueue& getWorkQueue();

} // namespace core

namespace event
{
template<class Receiver, class Event>
void sendUnicast(const std::shared_ptr<Receiver>& receiver, Event&& event)
{
    core::getWorkQueue().unicast(receiver, std::move(event));
}

template<class Event>
void sendMulticast(EventType type, Event&& event)
{
    core::getWorkQueue().multicast(type, std::move(event));
}

} // namespace event

