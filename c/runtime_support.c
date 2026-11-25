/* Minimal freestanding C support for compiler-generated aggregate copies. */
#include <stddef.h>
#include <stdint.h>
#ifndef RUNTIME_LINKAGE
#define RUNTIME_LINKAGE
#endif
RUNTIME_LINKAGE void *memset(void *destination, int value, size_t count)
{
    volatile uint8_t *out = destination;
    while (count--)
        *out++ = (uint8_t)value;
    return destination;
}

RUNTIME_LINKAGE void *memcpy(void *destination, const void *source, size_t count)
{
    volatile uint8_t *out = destination;
    const volatile uint8_t *in = source;
    while (count--)
        *out++ = *in++;
    return destination;
}
