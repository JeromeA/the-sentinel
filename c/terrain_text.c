/* The title and next-landscape code are made from terrain cells, not screen
 * font blits. Seven glyph rows and one blank row occupy four grid columns. */

static __attribute__((regparmcall)) void stamp_terrain_glyph(
    SegmentBytes data, uint8_t column, uint8_t row, uint16_t *pair_capture)
{
    uint8_t pair = 0;
    int glyph_row;
    /* The work byte precedes the glyph buffer: rows run backward through
     * that combined window, with the work byte cleared for the blank row. */
    for (glyph_row = 7; glyph_row >= 0; --glyph_row) {
        uint8_t bits = data[(uint16_t)&terrain_text_character_work + glyph_row] << 1;
        unsigned x;
        for (x = 0; x < 4; ++x) {
            pair = bits >> 6;
            bits <<= 2;
            data[(uint16_t)terrain_grid + terrain_index(column + x, row)] =
                glyph_pair_terrain_cells[pair];
        }
        ++row;
        if (glyph_row == 1)
            terrain_text_character_work = 0;
    }
    if (pair_capture)
        *pair_capture = pair;
}

void __far draw_terrain_character(uint8_t character,
    SegmentBytes data,
    SegmentBytes output,
    int backwards,
    struct TerrainGlyphEnd *capture)
{
    if (capture) {
        struct TerrainGlyphEnd empty = {0};
        *capture = empty;
    }
    if (backwards)
        return;
    if (!(character & 128) && (character < 32 || character >= 96))
        return;
    terrain_text_character_work = character;
    if (character & 128) {
        uint8_t coordinate = character & TERRAIN_COORDINATE_MASK;
        if (capture) {
            capture->coordinate = coordinate;
            capture->control = 1;
        }
        if (character & 64)
            terrain_text_row = coordinate;
        else
            terrain_text_column = coordinate;
        return;
    }
    {
        SegmentBytes source = data + (uint16_t)game_font_8x8 + (uint8_t)(character - 32) * 8;
        SegmentBytes destination = output + (uint16_t)game_glyph_rows;
        copy_video_bytes(&destination, &source, 8, backwards);
        terrain_scan_y = terrain_text_row;
        if (text_prng_compare_value < text_prng_compare_limit)
            game_random_next(&prng_state_low);
        uint8_t column = terrain_text_column;
        uint8_t row = terrain_scan_y;
        stamp_terrain_glyph(data, column, row, capture ? &capture->pair : 0);
        terrain_scan_x = column + 4;
        terrain_scan_y = row + 8;
        if (capture) {
            uint8_t last_x = column + 3;
            uint8_t last_y = row + 7;
            capture->bank = (uint16_t)terrain_grid + ((uint16_t)(last_x & 3) << 8);
            capture->offset = (((uint16_t)last_x << 3) & 0xe0) | last_y;
        }
    }
    if (capture)
        capture->column = terrain_text_column;
    terrain_text_column += 4;
    return;
}

struct TerrainGlyphEnd __far c_terrain_character(
    uint8_t character, SegmentBytes data, SegmentBytes output, int backwards)
{
    struct TerrainGlyphEnd end;
    draw_terrain_character(character, data, output, backwards, &end);
    return end;
}

/* Owned subsystem data. */
uint8_t glyph_pair_terrain_cells[4] = {
    0x20,
    0x27,
    0x28,
    0x29,
};
uint8_t title_secret_screen_flags = 0x0;

/* LetterWork is reset independently of other subsystems and data placement. */
struct LetterWork letter_work = {0};

void __far reset_letter_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&letter_work), 0, sizeof(letter_work), 0);
}
