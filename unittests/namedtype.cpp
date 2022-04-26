#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "glm/vec3.hpp"

TEST_CASE("Add two vec3s")
{
    glm::vec3 a{1, 2, 3};
    glm::vec3 b{2, 3, 4};
    glm::vec3 c{3, 5, 7};
    REQUIRE(c == (a + b));
}
