#include "tinycsocket.h"

#include <assert.h>

int main(void)
{
    assert(tcs_lib_init() == TCS_SUCCESS);
    assert(tcs_lib_free() == TCS_SUCCESS);
}
