#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <tinycsocket.h>

TEST_CASE("TCP test")
{
    CHECK(tcs_lib_init() == TINYCSOCKET_SUCCESS);
    CHECK(tcs_lib_free() == TINYCSOCKET_SUCCESS);
}
