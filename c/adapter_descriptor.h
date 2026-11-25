#ifndef SENTINEL_ADAPTER_DESCRIPTOR_H
#define SENTINEL_ADAPTER_DESCRIPTOR_H
#include <stdint.h>
#include <stddef.h>

struct __attribute__((packed, may_alias)) AdapterDescriptor {
    uint8_t adapter, display_class;
};

struct __attribute__((packed, may_alias)) AdapterResults {
    struct AdapterDescriptor entries[2];
};

_Static_assert(sizeof(struct AdapterDescriptor) == 2, "adapter descriptor size");
_Static_assert(offsetof(struct AdapterDescriptor, display_class) == 1, "display class offset");
_Static_assert(sizeof(struct AdapterResults) == 4, "adapter result list size");

/* Word encoding belongs to the original probe/ABI boundaries. */
union AdapterDescriptorWord {
    uint16_t word;
    struct AdapterDescriptor descriptor;
};

static inline struct AdapterDescriptor adapter_descriptor_from_word(uint16_t word)
{
    union AdapterDescriptorWord value = {.word = word};
    return value.descriptor;
}

static inline uint16_t adapter_descriptor_word(struct AdapterDescriptor descriptor)
{
    union AdapterDescriptorWord value = {.descriptor = descriptor};
    return value.word;
}
#endif
