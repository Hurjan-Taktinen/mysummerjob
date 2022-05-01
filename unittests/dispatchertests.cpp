#include "catch2/catch.hpp"
#include "entt/signal/dispatcher.hpp"
#include "entt/signal/delegate.hpp"
#include "entt/signal/sigh.hpp"
#include "entt/core/utility.hpp"

#include <iostream>
#include <string>

struct Event1
{
};
struct Event2
{
};
struct Event3
{
};

struct SystemA
{
};

struct SystemC
{
    SystemC(entt::dispatcher& disp) : disp(disp)
    {
        disp.sink<Event1>().connect<&SystemC::onEvent>(this);
    }

    ~SystemC() { disp.sink<Event1>().disconnect(this); }

    void onEvent(Event1 const&) { std::cout << "AWAW Event1" << std::endl; }

    void listento(entt::dispatcher& disp)
    {
        disp.sink<Event1>().connect<&SystemC::onEvent>(this);
    }

    entt::dispatcher& disp;
};

template<typename Listener>
class EventConnector
{
public:
    ~EventConnector() { _dispatcher.disconnect(_listener); }
    explicit EventConnector(entt::dispatcher& d, Listener* const listener) :
        _dispatcher(d), _listener(listener)
    {
    }

    template<typename Event>
    void attach()
    {
        auto sink = _dispatcher.template sink<Event>();
        // if constexpr(std::is_same_v<Event, Event1>)
        {
            sink.template connect<entt::overload<void(Event const&)>(
                    &Listener::onEvent)>(_listener);
        }
    }

private:
    entt::dispatcher& _dispatcher;
    Listener* const _listener;
};

struct SystemB
{
    SystemB(entt::dispatcher& disp) : conn(disp, this)
    {
        conn.attach<Event1>();
        conn.attach<Event2>();
    }

    void onEvent(Event1 const&) { std::cout << "AWAW Event1" << std::endl; }
    void onEvent(Event2 const&) { std::cout << "AWAW Event2" << std::endl; }

    EventConnector<SystemB> conn;
};

TEST_CASE("Dispaaja")
{
    std::cout << "AWAW ALKAA1" << std::endl;

    entt::dispatcher disp{};
    SystemB b{disp};

    {
        std::cout << "AWAW JONOON" << std::endl;
        disp.enqueue<Event1>();
        disp.enqueue<Event1>();
        disp.enqueue<Event2>();
        disp.enqueue<Event2>();
        std::cout << "AWAW JONOON LAITETTU" << std::endl;
        disp.update();
    }

    std::cout << "AWAW LOPPUU1" << std::endl;
}

TEST_CASE("Dispatcher")
{
    std::cout << "AWAW ALKAA2" << std::endl;

    entt::dispatcher disp{};

    SystemC c{disp};

    // c.listento(disp);

    disp.trigger<Event1>();

    {
        std::cout << "AWAW JONOON" << std::endl;
        disp.enqueue<Event1>();
        disp.enqueue<Event1>();
        std::cout << "AWAW JONOON LAITETTU" << std::endl;
        disp.update();
    }

    std::cout << "AWAW LOPPUU2" << std::endl;
}
