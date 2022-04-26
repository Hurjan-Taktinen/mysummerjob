#include "catch2/catch.hpp"

#include "entt/signal/dispatcher.hpp"
#include "entt/signal/delegate.hpp"
#include "entt/signal/sigh.hpp"

#include <iostream>
#include <string>

int f(int i)
{
    return i;
}

struct foo_struct
{
    int f(const int& i) const { return i; }
};

TEST_CASE("delegates")
{
    {
        entt::delegate<int(int)> delegate{};
        delegate.connect<&f>();
    }
    {
        entt::delegate<int(int)> delegate{};

        foo_struct foo;
        delegate.connect<&foo_struct::f>(foo);
    }
    {
        entt::delegate<int(int)> func{entt::connect_arg<&f>};
        auto ret = func(42);
        REQUIRE(42 == ret);
    }
}

int foo(int i, char)
{
    std::cout << "foo()" << std::endl;
    return i;
}

int fooo()
{
    return 1;
}

struct Listener
{
    int bar(const int& i, char c)
    {
        std::cout << "Listener::bar()" << std::endl;
        return i + static_cast<int>(c);
    }

    int barr() { return 2; }
};

TEST_CASE("Signals")
{
    {
        entt::sigh<void(int, char)> signal;
        entt::sink sink{signal};

        Listener instance;

        sink.connect<&foo>();
        sink.connect<&Listener::bar>(instance);

        // entt::delegate<int(int, char)> dele{};
        signal.publish(42, 'c');
    }

    {
        entt::sigh<int()> signal;
        entt::sink sink{signal};

        Listener instance;

        sink.connect<&fooo>();
        sink.connect<&Listener::barr>(instance);

        std::vector<int> vec{};
        signal.collect([&vec](int value) { vec.push_back(value); });

        REQUIRE(1 == vec[0]);
        REQUIRE(2 == vec[1]);
    }
}

struct SomeEvent
{
    std::string name;
    int value = 123;
};

struct AnotherEvent
{
    std::string name;
    int value = 123;
};

struct SomeSystem
{
    void onSomeEvent(const SomeEvent& event)
    {
        std::cout << "SomeSystem::onSomeEvent() " << event.name << std::endl;
    }

    void onAnotherEvent(const AnotherEvent& event)
    {
        std::cout << "SomeSystem::onSomeEvent() " << event.name << std::endl;
    }
};

TEST_CASE("events")
{
    SomeSystem sys;

    // SomeSystem* syslink = &sys;
    SomeSystem* syslink = &sys;
    {
        entt::dispatcher dispatcher{};
        dispatcher.sink<SomeEvent>().connect<&SomeSystem::onSomeEvent>(
                *syslink);
        dispatcher.sink<AnotherEvent>().connect<&SomeSystem::onAnotherEvent>(
                *syslink);

        dispatcher.trigger<SomeEvent>("hatti", 42);
        dispatcher.trigger<AnotherEvent>("patti", 43);
    }
}
