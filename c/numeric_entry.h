#ifndef SENTINEL_NUMERIC_ENTRY_H
#define SENTINEL_NUMERIC_ENTRY_H
#include <stdint.h>
#include <stddef.h>

/* Nibble views decode local snapshots. Producers keep their raw arithmetic:
 * malformed ASCII can produce more than one byte of packed result. */
union __attribute__((packed, may_alias)) BcdPair {
    uint8_t raw;

    struct __attribute__((packed)) {
        uint8_t ones : 4;
        uint8_t tens : 4;
    } digits;
};

union __attribute__((packed, may_alias)) LandscapeNumber {
    uint16_t raw;

    struct __attribute__((packed)) {
        union BcdPair low, high;
    } pairs;

    uint8_t bytes[2];
};

union __attribute__((packed, may_alias)) NumericEntry {
    /* Input is least-significant character first, opposite screen order. */
    uint8_t ascii[8];

    struct __attribute__((packed)) {
        union {
            union BcdPair pairs[4];
            union LandscapeNumber halves[2];
        } digits;

        uint8_t terminator;
        uint8_t retained[3];
    } packed;

    uint8_t raw[8];
};

_Static_assert(sizeof(union BcdPair) == 1, "BCD pair byte");
_Static_assert(sizeof(union LandscapeNumber) == 2, "landscape packed word");
_Static_assert(sizeof(union NumericEntry) == 8, "numeric entry extent");
_Static_assert(offsetof(union NumericEntry, packed.terminator) == 4, "entry marker byte");
_Static_assert(offsetof(union NumericEntry, packed.retained) == 5, "retained entry tail");
#endif
