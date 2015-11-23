#ifndef CHE_UTIL_H

#include <stddef.h>
#define che_containerof(ptr, type, member) ({                \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#endif
