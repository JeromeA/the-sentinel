#ifndef SENTINEL_VIDEO_ROWS_H
#define SENTINEL_VIDEO_ROWS_H
#include "video_copy.h"

enum VideoRowLayout {
    ROWS_CGA,
    ROWS_FOUR_BANK,
    ROWS_PLANAR,
    ROWS_LINEAR
};

static inline uint16_t video_row_offset(unsigned row, enum VideoRowLayout layout)
{
    switch (layout) {
    case ROWS_CGA:
        return (row & 1) * 8192 + (row >> 1) * 80;
    case ROWS_FOUR_BANK:
        return (row & 3) * 8192 + (row >> 2) * 160;
    case ROWS_PLANAR:
        return row * 40;
    default:
        return row * 320;
    }
}

void __far build_video_rows(SegmentBytes target, int backwards, enum VideoRowLayout layout);

#endif
