#include "catch2/catch.hpp"
#include "riften/thiefpool.hpp"

#include "event/keyevent.h"
#include "core/workqueue.h"

#include <iostream>

TEST_CASE("example")
{
    {
        riften::Thiefpool pool(4);
        auto result = pool.enqueue([](int x) { return x; }, 42);
        REQUIRE(42 == result.get());
    }
    {
        riften::Thiefpool pool(4);
        auto result = pool.enqueue([]() { return 42; });
        REQUIRE(42 == result.get());
    }
}

TEST_CASE("WorkQueue[queue]")
{
    auto& que = core::getWorkQueue();

    auto work = [](auto var) {
        int sum = 0;
        for(int i = 0; i < var; ++i)
        {
            for(int j = 0; j < var; ++j)
            {
                for(int k = 0; k < var; ++k)
                {
                    sum += 1;
                }
            }
        }

        return sum;
    };

    auto work2 = []() {
        const int var = 100;
        int sum = 0;
        for(int i = 0; i < var; ++i)
        {
            for(int j = 0; j < var; ++j)
            {
                for(int k = 0; k < var; ++k)
                {
                    sum += 1;
                }
            }
        }

        return sum;
    };

    {
        auto result = que.submitWork(work, 10);
        result.wait();
        int value = result.get();
        std::cout << "RESULT >> " << value << std::endl;
        REQUIRE(1000 == value);
    }
    {
        auto result = que.submitWork(work2);
        result.wait();
        int value = result.get();
        std::cout << "RESULT >> " << value << std::endl;
        REQUIRE(1000000 == value);
    }
}
