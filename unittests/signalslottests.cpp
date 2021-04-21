#include "catch2/catch.hpp"

#include "entt/signal/dispatcher.hpp"
#include "entt/signal/delegate.hpp"
#include "entt/signal/sigh.hpp"

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
        entt::delegate<int(int)> delegate {};
        delegate.connect<&f>();
    }
    {
        entt::delegate<int(int)> delegate {};

        foo_struct foo;
        delegate.connect<&foo_struct::f>(foo);
    }
    {
        entt::delegate<int(int)> func {entt::connect_arg<&f>};
        auto ret = func(42);
        REQUIRE(42 == ret);
    }
}

TEST_CASE("Signals")
{
    {
    }
}
