#include "catch2/catch.hpp"
#include "utils/namedtype.h"

#include <string>
#include <vector>
#include <iostream>

TEST_CASE("customtypes")
{
    using namespace utils;
    using namespace std::string_literals;

    {
        using KeyCode = NamedType<int, struct KeyCodeTag>;
        using IP = NamedType<std::string, struct IPTag>;

        constexpr auto k = KeyCode(42);
        STATIC_REQUIRE(k == 42);

        auto ip = IP(std::string("127.0.0.1"));
        REQUIRE(ip == "127.0.0.1"s);
        REQUIRE("127.0.0.1"s == ip);
        REQUIRE("127.0.0.1"s == ip.get());
    }

    {
        using Myvec = NamedType<std::vector<int>, struct myvectag>;
        std::vector<int> vec{1, 2, 3, 4, 5};

        Myvec myvec(std::move(vec));

        REQUIRE(1 == myvec.get().at(0));
        REQUIRE(2 == myvec.get().at(1));
        REQUIRE(3 == myvec.get().at(2));
        REQUIRE(4 == myvec.get().at(3));
        REQUIRE(5 == myvec.get().at(4));
    }

    {
        using Myvec = NamedType<std::vector<int>, struct myvectag>;
        std::vector<int> vec{1, 2, 3, 4, 5};

        Myvec myvec(std::move(vec));

        auto l = [](auto r) {
            REQUIRE(1 == r.at(0));
            REQUIRE(2 == r.at(1));
            REQUIRE(3 == r.at(2));
            REQUIRE(4 == r.at(3));
            REQUIRE(5 == r.at(4));
        };

        l(myvec.take());
        REQUIRE(myvec.get().empty());
    }
}
