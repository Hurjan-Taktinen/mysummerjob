#include "catch2/catch.hpp"
#include "riften/thiefpool.hpp"

#include "event/keyevent.h"

#include <iostream>

TEST_CASE("example")
{
    riften::Thiefpool pool(4);
    auto result = pool.enqueue([](int x) { return x; }, 42);
    REQUIRE(42 == result.get());
}
