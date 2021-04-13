#include "catch2/catch.hpp"
#include "utils/singleton.h"

class SingleFoo final : public utils::Singleton<SingleFoo>
{
public:
    void add(int a) { value = a; }
    int value = 0;

private:
    SingleFoo() = default;
    // SingleFoo(const SingleFoo&) = delete;
    // SingleFoo(SingleFoo&&) = delete;

    // SingleFoo& operator=(const SingleFoo&) = delete;
    // SingleFoo& operator=(SingleFoo&&) = delete;

    friend class utils::Singleton<SingleFoo>;
};

TEST_CASE("singletonpattern")
{
    {
        auto& f1 = SingleFoo::getInstance();
        auto& f2 = SingleFoo::getInstance();
        REQUIRE(&f1 == &f2);

        f1.add(10);
        REQUIRE(10 == f1.value);
    }
    {
        const auto& f1 = SingleFoo::getInstance();
        const auto& f2 = SingleFoo::getInstance();
        REQUIRE(&f1 == &f2);
    }
}
