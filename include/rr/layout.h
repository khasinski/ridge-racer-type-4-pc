#ifndef RR_LAYOUT_H
#define RR_LAYOUT_H

#include "types.h"

/* Byte offsets supported by the original GCC and native fixture compilers. */
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 4)
#define RR_FIELD_OFFSET(type, field) __builtin_offsetof(type, field)
#else
#include <stddef.h>
#define RR_FIELD_OFFSET(type, field) offsetof(type, field)
#endif

/* Checked on every host, including MSVC (which does not define
 * __SIZEOF_POINTER__). Pointer-bearing layouts express their native widths;
 * disc/card/wire records retain their fixed sizes and offsets. */
#define RR_ALIGN_UP(bytes, alignment) \
    (((bytes) + (alignment) - 1) & ~((alignment) - 1))
#define RR_ASSERT_SIZE(type, size) \
    _Static_assert(sizeof(type) == (size), #type " size")
#define RR_ASSERT_OFFSET(type, field, offset) \
    _Static_assert(RR_FIELD_OFFSET(type, field) == (offset), #type "." #field " offset")

#endif
