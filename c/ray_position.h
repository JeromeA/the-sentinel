#ifndef RAY_POSITION_H
#define RAY_POSITION_H

#include <stdint.h>
#include <stddef.h>

/* Little-endian 24-bit fixed-point storage. Each component stays byte-sized;
 * packed word accesses would change the original read/write granularity. */
struct __attribute__((packed)) RayPosition {
    uint8_t fraction_low, fraction_high, integer;
};

_Static_assert(sizeof(struct RayPosition) == 3, "24-bit ray position size");
_Static_assert(offsetof(struct RayPosition, fraction_low) == 0, "ray fraction low offset");
_Static_assert(offsetof(struct RayPosition, fraction_high) == 1, "ray fraction high offset");
_Static_assert(offsetof(struct RayPosition, integer) == 2, "ray integer offset");

/* Stores discard overflow above 24 bits, from low byte to high byte. */
static inline uint32_t load_ray_position(const struct RayPosition *position)
{
    return ((uint32_t)position->integer << 16) | ((uint16_t)position->fraction_high << 8) |
        position->fraction_low;
}

static inline void store_ray_position(struct RayPosition *position, uint32_t value)
{
    position->fraction_low = (uint8_t)value;
    position->fraction_high = value >> 8;
    position->integer = value >> 16;
}

#endif
