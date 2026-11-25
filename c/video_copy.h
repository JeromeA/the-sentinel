#ifndef SENTINEL_VIDEO_COPY_H
#define SENTINEL_VIDEO_COPY_H
#include <stdint.h>

#ifdef __ia16__
#define GAME_FAR __far
#else
#define GAME_FAR
#endif

typedef volatile uint8_t GAME_FAR *SegmentBytes;

/* Logical offsets are compatibility/geometry observations, separate from a
 * far cursor's low base. Truncation does not normalize into its segment. */
static inline uint16_t video_buffer_offset(SegmentBytes cursor, SegmentBytes origin)
{
    return (uint16_t)((unsigned long)cursor - (unsigned long)origin);
}

struct GameStripCopy {
    uint16_t source_end;
    uint16_t destination_end;
};

/* Copy 80 words in each interleave bank. The CGA variant copies two 80-byte
 * rows per bank; the four-bank variant copies one 160-byte row per bank. Both
 * cover four screen rows; their callers repeat the operation for an 8-row HUD.
 * banks is 2 or 4. */
struct GameStripCopy game_copy_banked_strip(SegmentBytes source,
    SegmentBytes destination,
    uint16_t source_offset,
    uint16_t destination_offset,
    unsigned banks,
    int backwards);
#endif
