#ifndef SENTINEL_REGISTER_COMPAT_H
#define SENTINEL_REGISTER_COMPAT_H
#include <stdint.h>
#include "video_copy.h"

struct __attribute__((packed, may_alias)) UnalignedWord {
    uint16_t value;
};

/* Retain the caller's segment while accessing an existing record through its C type.
 * Each member access remains volatile; the record expression supplies only its offset/type. */
#define segment_record(segment, record)                                                            \
    (*((volatile __typeof__(record) GAME_FAR *)((segment) + (uint16_t)&(record))))

struct LegacyRegisters {
    uint16_t es, ds, bp, di, si, dx, cx, bx, ax, flags;
};

#define CF 0x001u
#define PF 0x004u
#define AF 0x010u
#define ZF 0x040u
#define SF 0x080u
#define OF 0x800u
#define ARITH_FLAGS (CF | PF | AF | ZF | SF | OF)

static uint16_t status(uint16_t previous, uint16_t value, unsigned width)
{
    uint8_t parity = (uint8_t)value;
    uint16_t result = previous & ~ARITH_FLAGS;
    parity ^= parity >> 4;
    parity ^= parity >> 2;
    parity ^= parity >> 1;
    if (!(parity & 1))
        result |= PF;
    if (!value)
        result |= ZF;
    if (value & (width == 8 ? 0x80 : 0x8000))
        result |= SF;
    return result;
}

static uint16_t subtract8(uint16_t flags, uint8_t left, uint8_t right)
{
    uint8_t value = left - right;
    uint16_t result = status(flags, value, 8);
    if (left < right)
        result |= CF;
    if ((left ^ right ^ value) & 16)
        result |= AF;
    if ((left ^ right) & (left ^ value) & 128)
        result |= OF;
    return result;
}

static uint16_t add8(uint16_t flags, uint8_t left, uint8_t right)
{
    uint8_t value = left + right;
    uint16_t result = status(flags, value, 8);
    if ((unsigned)left + right > 255)
        result |= CF;
    if ((left ^ right ^ value) & 16)
        result |= AF;
    if (~(left ^ right) & (left ^ value) & 128)
        result |= OF;
    return result;
}

static uint16_t original_argument(uint32_t original_frame, unsigned index)
{
    volatile struct UnalignedWord GAME_FAR *word =
        (volatile struct UnalignedWord GAME_FAR *)(original_frame + 22 + index * 2);
    return word->value;
}

static void original_argument_store(uint32_t original_frame, unsigned index, uint16_t value)
{
    volatile struct UnalignedWord GAME_FAR *word =
        (volatile struct UnalignedWord GAME_FAR *)(original_frame + 22 + index * 2);
    word->value = value;
}

static uint16_t add16(uint16_t flags, uint16_t left, uint16_t right)
{
    uint16_t value = left + right;
    uint16_t result = status(flags, value, 16);
    if (value < left)
        result |= CF;
    if ((left ^ right ^ value) & 16)
        result |= AF;
    if (~(left ^ right) & (left ^ value) & 0x8000)
        result |= OF;
    return result;
}

static uint16_t subtract16(uint16_t flags, uint16_t left, uint16_t right)
{
    uint16_t value = left - right;
    uint16_t result = status(flags, value, 16);
    if (left < right)
        result |= CF;
    if ((left ^ right ^ value) & 16)
        result |= AF;
    if ((left ^ right) & (left ^ value) & 0x8000)
        result |= OF;
    return result;
}

static uint16_t segment_word(SegmentBytes segment, uint16_t offset)
{
    return ((volatile struct UnalignedWord GAME_FAR *)(segment + offset))->value;
}

static void segment_word_store(SegmentBytes segment, uint16_t offset, uint16_t value)
{
    ((volatile struct UnalignedWord GAME_FAR *)(segment + offset))->value = value;
}

static SegmentBytes segment_bytes(uint16_t segment)
{
    return (SegmentBytes)((uint32_t)segment << 16);
}

/* An absolute offset in the same segment, without adding a pointer's old offset
 * or normalizing an offset wrap into the segment. */
static SegmentBytes segment_at_offset(SegmentBytes base, uint16_t offset)
{
    return (SegmentBytes)(((uint32_t)base & 0xffff0000UL) | offset);
}

#endif
