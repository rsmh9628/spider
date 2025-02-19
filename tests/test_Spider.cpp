#include <rack.hpp>
#include <catch2/catch_all.hpp>

using namespace rack;

TEST_CASE("HelloWorldTest") {
    REQUIRE(1 == 1);
    INFO("Hello, World!");
}