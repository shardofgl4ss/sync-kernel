#include "types.h"
#include "macros.h"

_SY_PRIMITIVE _Bool checked_add(uint64_t a, uint64_t b, uint64_t *res)
{
        return __builtin_add_overflow(a, b, res);
}
