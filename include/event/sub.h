#pragma once

#include "entt/signal/dispatcher.hpp"
#include "entt/signal/delegate.hpp"
#include "entt/core/utility.hpp"

namespace event
{

template<typename Listener>
class Subs
{
public:
    ~Subs() { detachAll(); }
    explicit Subs(entt::dispatcher& d, Listener* const listener) :
        _dispatcher(d), _listener(listener)
    {
    }

    template<typename Event>
    void attach()
    {
        _dispatcher.sink<Event>()
                .template connect<entt::overload<void(Event const&)>(
                        &Listener::onEvent)>(_listener);
    }

    template<typename Event>
    void detach()
    {
        _dispatcher.sink<Event>()
                .template disconnect<entt::overload<void(Event const&)>(
                        &Listener::onEvent)>(_listener);
    }

    void detachAll() { _dispatcher.disconnect(_listener); }

    template<typename Event>
    void enqueue(Event&& event)
    {
        _dispatcher.enqueue<Event>(std::move(event));
    }

    template<typename Event>
    void trigger(Event&& event)
    {
        _dispatcher.trigger<Event>(std::move(event));
    }

private:
    entt::dispatcher& _dispatcher;
    Listener* const _listener;
};

} // namespace event
