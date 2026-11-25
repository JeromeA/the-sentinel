/* Segmented byte initialization and banked display copies, including wrapping
 * rules. GAME_FAR only describes DOS segmented addresses; no assembly is used. */
#include "video_copy.h"

/* Ordinary segmented initialization; callers retain their ABI publication. */
static __attribute__((noinline, regparmcall)) SegmentBytes fill_segment_bytes(
    SegmentBytes destination, uint16_t count, uint8_t value, int backwards)
{
    int step = backwards ? -1 : 1;
    while (count--) {
        *destination = value;
        destination += step;
    }
    return destination;
}

/* Bank seams can be odd addresses. Make those word accesses explicit rather
 * than promising the compiler that every uint16_t pointer is aligned. */
#include "register_compat.h"

struct GameStripCopy game_copy_banked_strip(SegmentBytes source,
    SegmentBytes destination,
    uint16_t source_offset,
    uint16_t destination_offset,
    unsigned banks,
    int backwards)
{
    struct GameStripCopy result = {source_offset, destination_offset};
    uint16_t step = backwards ? (uint16_t)-2 : 2;
    unsigned bank;

    for (bank = 0; bank < banks; ++bank) {
        uint16_t bank_bits = (uint16_t)(bank * 0x2000u);
        uint16_t read_offset = source_offset | bank_bits;
        uint16_t write_offset = destination_offset;
        unsigned word;
        for (word = 0; word < 80; ++word) {
            write_offset = (write_offset & 0x1fffu) | bank_bits;
            /* A word is read completely before it is written. This matters
             * when the buffers alias or an unaligned word straddles a bank. */
            uint16_t value =
                ((volatile struct UnalignedWord GAME_FAR *)(source + read_offset))->value;
            ((volatile struct UnalignedWord GAME_FAR *)(destination + write_offset))->value = value;
            read_offset = (uint16_t)(read_offset + step);
            write_offset = (uint16_t)(write_offset + step);
        }
        result.source_end = read_offset;
        result.destination_end = write_offset;
    }
    return result;
}
