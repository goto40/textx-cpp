#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <iostream>
#include "textx/version.h"

TEST_CASE( "hello_world", "[hello]" ) {
    std::cout << "textx "<< textx::VERSION << " " << textx::VERSION_SHORT << " unittests\n";
    REQUIRE( true );
}
