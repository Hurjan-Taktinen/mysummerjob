#include "catch2/catch.hpp"
#include "entt/signal/dispatcher.hpp"
#include "entt/signal/delegate.hpp"
#include "entt/signal/sigh.hpp"
#include "entt/core/utility.hpp"

#include "event/sub.h"

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
    SystemA(entt::dispatcher& disp) : conn(disp, this)
    {
        conn.attach<Event1>();
        conn.attach<Event2>();
        conn.attach<Event3>();
    }

    void onEvent(Event1 const&)
    {
        std::cout << "AWAW SystemA::Event1" << std::endl;
    }
    void onEvent(Event2 const&)
    {
        std::cout << "AWAW SystemA::Event2" << std::endl;
    }
    void onEvent(Event3 const&)
    {
        std::cout << "AWAW SystemA::Event2" << std::endl;
    }

    event::Subs<SystemA> conn;
};

struct SystemB
{
    SystemB(entt::dispatcher& disp) : conn(disp, this)
    {
        conn.attach<Event1>();
        conn.attach<Event2>();
    }

    void onEvent(Event1 const&)
    {
        std::cout << "AWAW SystemB::Event1" << std::endl;
    }
    void onEvent(Event2 const&)
    {
        std::cout << "AWAW SystemB::Event2" << std::endl;
    }

    void removeEvent2()
    {
        conn.detach<Event2>();
    }

    event::Subs<SystemB> conn;
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
    {
        b.removeEvent2();
        disp.enqueue<Event1>();
        disp.enqueue<Event2>();
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

TEST_CASE("Y-combinator")
{
    entt::y_combinator gauss([](auto const& self, auto value) -> unsigned {
        return value ? (value + self(value - 1u)) : 0u;
    });

    REQUIRE(6u == gauss(3u));
}
