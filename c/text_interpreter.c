/* Text execution consumes bytecode and direction. Raw entry observations are
 * optional and never form the native interpreter's control state. */
#include <stdint.h>
#include "register_compat.h"
#include "typed_api.h"
#include "text_bytecode.h"
#include "data_aliases.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"

static int write_character(
    SegmentBytes data, uint8_t character, int backwards, struct TextCharacterEnd *capture);

static void text_newline(struct TextCharacterEnd *capture)
{
    uint16_t row = c_text_newline();
    if (capture) {
        capture->flags = subtract16(capture->flags, row, 25);
        if (row >= 25) {
            row = 0;
            capture->flags = status(capture->flags, 0, 16);
        }
        capture->value = row;
    }
}

static int emit_character(
    SegmentBytes data, uint8_t character, int backwards, struct TextCharacterEnd *capture)
{
    if (!capture)
        return write_character(data, character, backwards, 0);
    struct TextCharacterEnd next =
        c_text_character(data, (capture->value & 0xff00) | character, capture->flags);
    if (!next.rendered) {
        next.rendered = capture->rendered;
        next.port = capture->port;
        next.port_mask = capture->port_mask;
    } else {
        next.port = (capture->port & ~next.port_mask) | next.port;
        next.port_mask |= capture->port_mask;
    }
    *capture = next;
    return next.rendered;
}

static struct NumberTextEnd number_text_merge(
    struct NumberTextEnd before, struct NumberTextEnd after)
{
    after.character.port =
        (before.character.port & ~after.character.port_mask) | after.character.port;
    after.character.port_mask |= before.character.port_mask;
    after.character.rendered |= before.character.rendered;
    if (!after.terrain_written && before.terrain_written) {
        after.terrain_written = 1;
        after.terrain_bank = before.terrain_bank;
        after.terrain_offset = before.terrain_offset;
    }
    if (!after.pair_written && before.pair_written) {
        after.pair_written = 1;
        after.pair = before.pair;
    }
    return after;
}

static int write_character(
    SegmentBytes data, uint8_t character, int backwards, struct TextCharacterEnd *capture)
{
    uint8_t opcode = game_text_pending_opcode;
    game_text_input_character = character;
    if (opcode) {
        uint8_t parameter = ++game_text_parameter_index;
        switch (opcode) {
        case TEXT_SET_GLYPH_FLAGS:
            if (parameter != TEXT_GLYPH_FLAGS_PARAMETERS) {
                if (capture) {
                    capture->value = (capture->value & 0xff00) | parameter;
                    capture->flags =
                        subtract8(capture->flags, parameter, TEXT_GLYPH_FLAGS_PARAMETERS);
                }
                return 0;
            }
            game_text_glyph_flags = (game_text_glyph_flags & 0xff00) | character;
            break;
        case TEXT_SET_POSITION:
            if (parameter != TEXT_POSITION_PARAMETERS) {
                uint8_t doubled = character << 1;
                uint16_t column = (int16_t)(int8_t)doubled;
                game_text_column = column;
                if (capture) {
                    capture->value = column;
                    capture->flags = add8(capture->flags, character, character);
                }
                return 0;
            }
            game_text_row = (int16_t)(int8_t)character;
            break;
        case TEXT_SET_ALTERNATE_COLOR: {
            if (parameter == TEXT_ALTERNATE_COLOR_IGNORED_PARAMETERS) {
                if (capture) {
                    capture->value = (capture->value & 0xff00) | parameter;
                    capture->flags = subtract8(
                        capture->flags, parameter, TEXT_ALTERNATE_COLOR_IGNORED_PARAMETERS);
                }
                return 0;
            }
            uint16_t color = character & TEXT_COLOR_INDEX_MASK;
            if (character & TEXT_COLOR_BACKGROUND) {
                game_text_alternate_background = color;
                if (game_text_alternate_mode & 128)
                    game_text_active_background = color;
            } else {
                game_text_alternate_foreground = color;
                if (game_text_alternate_mode & 128)
                    game_text_active_foreground = color;
            }
            break;
        }
        case TEXT_SET_NORMAL_BACKGROUND: {
            uint16_t color = character & TEXT_COLOR_INDEX_MASK;
            game_text_normal_background = color;
            if (!(game_text_alternate_mode & 128))
                game_text_active_background = color;
            break;
        }
        default:
            break;
        }
        game_text_pending_opcode = 0;
        if (capture) {
            capture->value = 0;
            capture->flags = status(capture->flags, 0, 16);
        }
        return 0;
    }
    switch (character) {
    case TEXT_SET_POSITION:
    case TEXT_SET_ALTERNATE_COLOR:
    case TEXT_SET_GLYPH_FLAGS:
    case TEXT_SET_NORMAL_BACKGROUND:
        game_text_pending_opcode = character;
        game_text_parameter_index = 0;
        if (capture)
            capture->flags = subtract8(capture->flags, character, character);
        return 0;
    case TEXT_NEXT_GLYPH: {
        uint16_t column = game_text_column + 2;
        if (capture)
            capture->flags = subtract16(capture->flags, column, 40);
        if (column >= 40) {
            column = 0;
            if (capture)
                capture->flags = status(capture->flags, 0, 16);
        }
        game_text_column = column;
        if (capture)
            capture->value = column;
        return 0;
    }
    case TEXT_PREVIOUS_GLYPH: {
        uint16_t before = game_text_column;
        uint16_t column = before - 2;
        game_text_column = column;
        if (capture)
            capture->flags = subtract16(capture->flags, before, 2);
        if ((int16_t)column < 0) {
            uint16_t row = game_text_row - 1;
            game_text_row = row;
            if (capture)
                capture->flags =
                    (subtract16(capture->flags, row + 1, 1) & ~CF) | (capture->flags & CF);
            if ((int16_t)row < 0)
                game_text_row = 24;
            game_text_column = 38;
        }
        return 0;
    }
    case TEXT_NEWLINE:
        text_newline(capture);
        return 0;
    case TEXT_CARRIAGE_RETURN:
        game_text_column = 0;
        if (capture)
            capture->flags = subtract8(capture->flags, character, TEXT_CARRIAGE_RETURN);
        return 0;
    case TEXT_NORMAL_STYLE:
        game_text_transparent &= 0xff00;
        game_text_alternate_mode &= 0xff00;
        game_text_active_background = game_text_normal_background;
        game_text_active_foreground = game_text_normal_foreground;
        if (capture) {
            capture->value = game_text_normal_foreground;
            capture->flags = status(capture->flags, 0, 16);
        }
        return 0;
    case TEXT_ALTERNATE_STYLE:
        game_text_transparent |= 255;
        game_text_alternate_mode |= 255;
        game_text_active_background = game_text_alternate_background;
        game_text_active_foreground = game_text_alternate_foreground;
        if (capture) {
            capture->value = game_text_alternate_foreground;
            capture->flags = subtract8(capture->flags, character, TEXT_ALTERNATE_STYLE);
        }
        return 0;
    case TEXT_ERASE_GLYPH: {
        if (capture)
            capture->flags = subtract8(capture->flags, character, TEXT_CARRIAGE_RETURN);
        int selected = emit_character(data, TEXT_PREVIOUS_GLYPH, backwards, capture);
        uint16_t transparent = game_text_transparent;
        game_text_transparent = 0;
        selected |= emit_character(data, TEXT_SPACE, backwards, capture);
        game_text_transparent = transparent;
        selected |= emit_character(data, TEXT_PREVIOUS_GLYPH, backwards, capture);
        return selected;
    }
    default:
        break;
    }
    if (character < TEXT_FIRST_GLYPH) {
        if (capture)
            capture->flags = subtract8(capture->flags, character, TEXT_ALTERNATE_STYLE);
        return 0;
    }
    if (capture)
        capture->flags = subtract8(capture->flags, character, TEXT_GLYPH_LIMIT);
    if (character >= TEXT_GLYPH_LIMIT)
        return 0;
    if (capture) {
        struct TextGlyphEnd glyph;
        text_render_glyph(data, character, backwards, &glyph);
        capture->rendered = 1;
        capture->port_mask = glyph.mode == 1 ? 255 : 0xffff;
        capture->port = glyph.mode == 0 ? glyph.glyph.bank_step
            : glyph.mode == 1           ? (uint8_t)glyph.request.transparent
            : glyph.mode == 2           ? 0
                                        : 0x3ce;
    } else
        text_render_glyph(data, character, backwards, 0);
    uint16_t column = game_text_column + 2;
    if (capture)
        capture->flags = subtract16(capture->flags, column, 40);
    if (column >= 40) {
        text_newline(capture);
        column = 0;
        if (capture)
            capture->flags = status(capture->flags, 0, 16);
    }
    game_text_column = column;
    if (capture)
        capture->value = column;
    /* A screen glyph selects DS as output even if the lower request rejects. */
    return 1;
}

struct TextCharacterEnd __far c_text_character(
    SegmentBytes data, uint16_t character_word, uint16_t initial_flags)
{
    struct TextCharacterEnd end = {0};
    end.value = character_word;
    end.flags = initial_flags;
    write_character(data, (uint8_t)character_word, !!(initial_flags & 0x400), &end);
    return end;
}

static int write_formatted(
    SegmentBytes data, uint8_t character, int backwards, struct FormattedCharacterEnd *capture)
{
    uint8_t mode = text_input_render_flags & 128;
    if (capture)
        capture->character.flags = status(capture->character.flags, mode, 8);
    int selected = 0;
    if (!(text_input_render_flags & 128) && character >= TEXT_FIRST_GLYPH &&
        character < TEXT_GLYPH_LIMIT) {
        if (capture)
            capture->formatted = 1;
        game_text_script_data[0] = character;
        text_character_controls_a[9] = character;
        for (int index = 22; index >= 0; --index)
            selected |= emit_character(
                data, game_text_script_data[index], backwards, capture ? &capture->character : 0);
        if (capture)
            capture->character.flags = (subtract16(capture->character.flags, 0, 1) & ~CF) |
                (capture->character.flags & CF);
    } else
        selected = emit_character(data, character, backwards, capture ? &capture->character : 0);
    return selected;
}

struct FormattedCharacterEnd __far c_formatted_character(
    SegmentBytes data, uint16_t character_word, uint16_t initial_flags)
{
    struct FormattedCharacterEnd end = {0};
    end.character.value = character_word;
    end.character.flags = initial_flags;
    write_formatted(data, (uint8_t)character_word, !!(initial_flags & 0x400), &end);
    return end;
}

static int draw_script(
    SegmentBytes data, TextScriptId script, int backwards, struct TextRunEnd *capture);

static int write_script_character(
    SegmentBytes data, uint8_t character, int backwards, struct TextRunEnd *capture)
{
    if (character < TEXT_SCRIPT_CALL_BASE) {
        if (!capture)
            return write_formatted(data, character, backwards, 0);
        struct FormattedCharacterEnd formatted =
            c_formatted_character(data, capture->character.value, capture->character.flags);
        capture->character = formatted.character;
        capture->formatted = formatted.formatted;
        return formatted.character.rendered;
    }
    TextScriptId script = (int16_t)(int8_t)(character - TEXT_SCRIPT_CALL_BASE);
    if (capture) {
        if (script >= TEXT_SCRIPT_COUNT)
            capture->character.flags =
                subtract8(capture->character.flags, character, TEXT_SCRIPT_CALL_BASE);
        capture->character.value = script;
    }
    int selected = draw_script(data, script, backwards, capture);
    if (capture)
        capture->source_written = 0;
    return selected;
}

struct TextRunEnd __far c_script_character(
    SegmentBytes data, uint16_t character_word, uint16_t initial_flags)
{
    struct TextRunEnd end = {0};
    end.character.value = character_word;
    end.character.flags = initial_flags;
    write_script_character(data, (uint8_t)character_word, !!(initial_flags & 0x400), &end);
    return end;
}

static int draw_script(
    SegmentBytes data, TextScriptId script, int backwards, struct TextRunEnd *capture)
{
    if (script >= TEXT_SCRIPT_COUNT) {
        if (capture)
            capture->character.flags |= CF;
        return 0;
    }
    uint16_t cursor = data[(uint16_t)game_text_script_offsets + script];
    cursor += (uint16_t)game_text_script_data;
    if (capture)
        capture->nested_script = script;
    int selected = 0;
    for (;;) {
        if ((uint16_t)(cursor - (uint16_t)game_text_script_data) >= sizeof(text_script_bank)) {
            if (capture)
                capture->character.flags |= CF;
            break;
        }
        uint8_t character = data[cursor];
        cursor += backwards ? -1 : 1;
        if (capture)
            capture->character.value = (capture->character.value & 0xff00) | character;
        if (character == TEXT_SCRIPT_END)
            break;
        if (capture) {
            struct TextRunEnd next =
                c_script_character(data, capture->character.value, capture->character.flags);
            if (next.script_written)
                capture->nested_script = next.nested_script;
            if (!next.character.rendered) {
                next.character.rendered = capture->character.rendered;
                next.character.port = capture->character.port;
                next.character.port_mask = capture->character.port_mask;
            } else {
                next.character.port =
                    (capture->character.port & ~next.character.port_mask) | next.character.port;
                next.character.port_mask |= capture->character.port_mask;
            }
            capture->character = next.character;
            selected |= next.character.rendered;
        } else
            selected |= write_script_character(data, character, backwards, 0);
    }
    if (capture) {
        capture->source = cursor;
        capture->source_written = capture->script_written = capture->script_run = 1;
        capture->character.flags = status(capture->character.flags, 0, 16);
    }
    return selected;
}

struct TextRunEnd __far c_draw_text_script(
    SegmentBytes data, TextScriptId script, uint16_t initial_character_word, uint16_t initial_flags)
{
    struct TextRunEnd end = {0};
    end.character.value = initial_character_word;
    end.character.flags = initial_flags;
    draw_script(data, script, !!(initial_flags & 0x400), &end);
    return end;
}

int __far write_text_script(SegmentBytes data, TextScriptId script, int backwards)
{
    return draw_script(data, script, backwards, 0);
}

static struct TextStreamEffect write_inline(SegmentBytes data,
    uint8_t character,
    int backwards,
    uint16_t source,
    struct TextRunEnd *capture)
{
    struct TextStreamEffect effect = {source, 0};
    if (character != TEXT_SET_GLYPH_FLAGS) {
        if (capture) {
            struct FormattedCharacterEnd formatted =
                c_formatted_character(data, capture->character.value, capture->character.flags);
            capture->character = formatted.character;
            capture->formatted = formatted.formatted;
            effect.output_selected = formatted.character.rendered;
        } else
            effect.output_selected = write_formatted(data, character, backwards, 0);
        return effect;
    }
    uint8_t remaining;
    do {
        character = data[source];
        source += backwards ? -1 : 1;
        effect.output_selected |=
            emit_character(data, character, backwards, capture ? &capture->character : 0);
        --text_inline_control_bytes_left;
        remaining = text_inline_control_bytes_left;
    } while (text_inline_control_bytes_left);
    if (capture)
        capture->character.flags = (subtract8(capture->character.flags, remaining + 1, 1) & ~CF) |
            (capture->character.flags & CF);
    text_inline_control_bytes_left = TEXT_INLINE_PACKET_BYTES;
    effect.next_source = source;
    if (capture) {
        capture->source = source;
        capture->source_written = 1;
    }
    return effect;
}

struct TextStreamEffect __far write_inline_text(
    SegmentBytes data, uint8_t character, int backwards, uint16_t source)
{
    return write_inline(data, character, backwards, source, 0);
}

struct TextRunEnd __far c_script_inline_character(
    SegmentBytes data, uint16_t character_word, uint16_t initial_flags, uint16_t source)
{
    struct TextRunEnd end = {0};
    end.character.value = character_word;
    end.character.flags = initial_flags;
    write_inline(data, (uint8_t)character_word, !!(initial_flags & 0x400), source, &end);
    return end;
}

static int write_number_character(SegmentBytes data,
    SegmentBytes output,
    uint8_t character,
    int backwards,
    struct NumberTextEnd *capture)
{
    if (character == '0')
        character = 'O';
    uint8_t mode = number_terrain_text_flags & 128;
    if (capture) {
        capture->character.value = (capture->character.value & 0xff00) | character;
        capture->character.flags = status(capture->character.flags, mode, 8);
    }
    if (number_terrain_text_flags & 128) {
        if (!capture) {
            draw_terrain_character(character, data, output, backwards, 0);
            return 0;
        }
        struct TerrainGlyphEnd glyph;
        draw_terrain_character(character, data, output, backwards, &glyph);
        uint16_t word = capture->character.value;
        if (glyph.control) {
            capture->character.value = (word & 0x7f00) | glyph.coordinate;
            capture->character.flags = status(capture->character.flags, glyph.coordinate, 8) |
                ((word & 64) ? CF : 0) | ((word & 0x4000) ? OF : 0);
        } else {
            capture->pair = glyph.pair;
            capture->pair_written = capture->terrain_written = 1;
            capture->terrain_bank = glyph.bank;
            capture->terrain_offset = glyph.offset;
            capture->character.port_mask = 255;
            capture->character.flags = add8(capture->character.flags, glyph.column, 4);
        }
        return 0;
    }
    if (!capture)
        return write_formatted(data, character, backwards, 0);
    struct FormattedCharacterEnd formatted =
        c_formatted_character(data, capture->character.value, capture->character.flags);
    capture->character = formatted.character;
    if (formatted.formatted) {
        capture->pair_written = 1;
        capture->pair = 0xffff;
    }
    return formatted.character.rendered;
}

struct NumberTextEnd __far c_decimal_character(
    SegmentBytes data, SegmentBytes output, uint16_t character_word, uint16_t initial_flags)
{
    struct NumberTextEnd end = {0};
    end.character.value = character_word;
    end.character.flags = initial_flags;
    write_number_character(data, output, (uint8_t)character_word, !!(initial_flags & 0x400), &end);
    return end;
}

static int emit_number_character(SegmentBytes data,
    SegmentBytes output,
    uint8_t character,
    int backwards,
    struct NumberTextEnd *capture,
    uint16_t high)
{
    if (!capture)
        return write_number_character(data, output, character, backwards, 0);
    struct NumberTextEnd next =
        c_decimal_character(data, output, high | character, capture->character.flags);
    *capture = number_text_merge(*capture, next);
    return capture->character.rendered;
}

static int draw_pair(SegmentBytes data,
    SegmentBytes output,
    union BcdPair pair,
    int backwards,
    struct NumberTextEnd *capture)
{
    uint16_t high = capture ? capture->character.value & 0xff00 : 0;
    int selected =
        emit_number_character(data, output, pair.digits.tens + '0', backwards, capture, high);
    if (selected)
        output = data;
    selected |=
        emit_number_character(data, output, pair.digits.ones + '0', backwards, capture, high);
    return selected;
}

int __far write_decimal_pair(
    SegmentBytes data, SegmentBytes output, union BcdPair pair, int backwards)
{
    return draw_pair(data, output, pair, backwards, 0);
}

static int emit_pair(SegmentBytes data,
    SegmentBytes output,
    union BcdPair pair,
    int backwards,
    struct NumberTextEnd *capture)
{
    if (!capture)
        return write_decimal_pair(data, output, pair, backwards);
    struct NumberTextEnd next = c_draw_decimal_pair(
        data, output, (capture->character.value & 0xff00) | pair.raw, capture->character.flags);
    *capture = number_text_merge(*capture, next);
    return capture->character.rendered;
}

struct NumberTextEnd __far c_decimal_digit(
    SegmentBytes data, SegmentBytes output, uint16_t digit_word, uint16_t flags)
{
    return c_decimal_character(
        data, output, (digit_word & 0xff00) | (uint8_t)(digit_word + 48), flags);
}

struct NumberTextEnd __far c_draw_decimal_pair(
    SegmentBytes data, SegmentBytes output, uint16_t packed_pair, uint16_t flags)
{
    struct NumberTextEnd end = {0};
    end.character.value = packed_pair;
    end.character.flags = flags;
    union BcdPair pair = {.raw = (uint8_t)packed_pair};
    draw_pair(data, output, pair, !!(flags & 0x400), &end);
    return end;
}

static int draw_landscape(
    SegmentBytes data, SegmentBytes output, int backwards, struct NumberTextEnd *capture)
{
    union BcdPair pair = {.raw = landscape_number_bcd.pairs.high.raw};
    int selected = emit_pair(data, output, pair, backwards, capture);
    if (selected)
        output = data;
    pair.raw = landscape_number_bcd.pairs.low.raw;
    selected |= emit_pair(data, output, pair, backwards, capture);
    return selected;
}

int __far write_landscape_number(SegmentBytes data, SegmentBytes output, int backwards)
{
    return draw_landscape(data, output, backwards, 0);
}

struct NumberTextEnd __far c_draw_landscape_bcd(
    SegmentBytes data, SegmentBytes output, uint16_t initial_character_word, uint16_t flags)
{
    struct NumberTextEnd end = {0};
    end.character.value = initial_character_word;
    end.character.flags = flags;
    draw_landscape(data, output, !!(flags & 0x400), &end);
    return end;
}

static int repaint_digits(
    SegmentBytes data, SegmentBytes output, int backwards, struct NumberTextEnd *capture)
{
    uint16_t index = (int16_t)(int8_t)math_work_lo;
    if (capture)
        capture->character.value = index;
    text_input_render_flags |= 128;
    int selected = 0;
    do {
        --index;
        selected |= emit_number_character(data,
            output,
            data[(uint16_t)&number_entry_buffer.ascii[0] + index],
            backwards,
            capture,
            capture ? capture->character.value & 0xff00 : 0);
        if (selected)
            output = data;
    } while (index);
    /* Glyph callbacks may change this signed window between the two passes. */
    index = (int16_t)(int8_t)math_work_lo;
    if (capture)
        capture->character.value = index;
    do {
        selected |= emit_number_character(data,
            output,
            TEXT_PREVIOUS_GLYPH,
            backwards,
            capture,
            capture ? capture->character.value & 0xff00 : 0);
        if (selected)
            output = data;
    } while (--index);
    text_input_render_flags &= 127;
    uint8_t mode = text_input_render_flags;
    if (capture) {
        capture->character.flags = status(capture->character.flags, mode, 8);
        capture->pair_written = 1;
        capture->pair = 0xffff;
    }
    return selected;
}

int __far repaint_digit_field(SegmentBytes data, SegmentBytes output, int backwards)
{
    return repaint_digits(data, output, backwards, 0);
}

struct NumberTextEnd __far c_repaint_digit_field(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags)
{
    struct NumberTextEnd end = {0};
    end.character.flags = initial_flags;
    repaint_digits(data, output, !!(initial_flags & 0x400), &end);
    return end;
}

static void completion_terrain(SegmentBytes data,
    SegmentBytes output,
    int backwards,
    struct NumberTextEnd *capture,
    uint16_t counter_bank)
{
    number_terrain_text_flags = 128;
    draw_terrain_character(128, data, output, backwards, 0);
    draw_terrain_character(0xc7, data, output, backwards, 0);
    secret_code_pair_work >>= 1;
    int pair_index = secret_code_pair_work;
    for (; pair_index >= 0; --pair_index) {
        union BcdPair pair = {.raw = c_random_decimal_pair()};
        if (pair_index < 4) {
            uint8_t random_high = prng_state_low.middle;
            if (capture)
                capture->character.value = ((uint16_t)random_high << 8) | pair.raw;
            emit_pair(data, output, pair, backwards, capture);
        }
    }
    secret_code_pair_work = 255;
    struct GameRandomStep random = game_random_next(&prng_state_low);
    uint8_t random_high = prng_state_low.middle;
    if (capture)
        capture->character.value = ((uint16_t)random_high << 8) | random.value;
    number_terrain_text_flags &= 127;
    uint8_t mode = number_terrain_text_flags;
    if (capture) {
        capture->character.flags = status(capture->character.flags, mode, 8);
        capture->pair = (counter_bank & 0xff00) | 255;
        capture->pair_written = 1;
    }
}

void __far write_completion_terrain(SegmentBytes data, SegmentBytes output, int backwards)
{
    completion_terrain(data, output, backwards, 0, 0);
}

struct NumberTextEnd __far c_completion_code_terrain(
    SegmentBytes data, SegmentBytes output, uint16_t counter_bank, uint16_t initial_flags)
{
    struct NumberTextEnd end = {0};
    end.character.flags = initial_flags;
    completion_terrain(data, output, !!(initial_flags & 0x400), &end, counter_bank);
    return end;
}

/* Owned subsystem data. */
/* Offsets in game_text_script_offsets are relative to this mutable script bank. */
// clang-format off
uint8_t text_script_bank[188] =
    /* Mutable formatted-glyph work window, before the script entries. */
    "\x43\x02\x00\x12\x00\x04\x00\x00\x00\x19\x08\x43\x03\x00\x12\xff\xfc\x00\x00\x00\x19\x7f\x20"
    /* TEXT_SCRIPT_TITLE_PROMPT */
    TEXT_CALL_PROMPT_STYLE TEXT_CALL_POSITION_BOTTOM_LEFT TEXT_CALL_PRESS_ANY_KEY "\x11\x80\xff"
    /* TEXT_SCRIPT_LANDSCAPE_NUMBER_PROMPT */
    TEXT_CALL_POSITION_BOTTOM_LEFT TEXT_CALL_SKIP_FIVE_GLYPHS TEXT_CALL_SKIP_FIVE_GLYPHS
    TEXT_CALL_SKIP_THREE_GLYPHS TEXT_CALL_POSITION_TOP_LEFT TEXT_CALL_LANDSCAPE_LABEL
    "\x09NUMBER?\x04\x1f\x05\x16\xff"
    /* TEXT_SCRIPT_SECRET_CODE_PROMPT */
    TEXT_CALL_PROMPT_STYLE TEXT_CALL_POSITION_TOP_LEFT TEXT_CALL_SECRET_ENTRY_CODE_LABEL
    "?\x04\x1f\x03\x16\xff"
    /* TEXT_SCRIPT_WRONG_SECRET_CODE */
    TEXT_CALL_PROMPT_STYLE TEXT_CALL_POSITION_TOP_LEFT "WRONG\x09SECRET\x09"
    "CODE" TEXT_CALL_TITLE_PROMPT "\xff"
    /* TEXT_SCRIPT_PREVIEW_CAPTION */
    TEXT_CALL_CAPTION_STYLE TEXT_CALL_POSITION_BOTTOM_INSET TEXT_CALL_PRESS_ANY_KEY
    TEXT_CALL_POSITION_TOP_LEFT "\x09\x09" TEXT_CALL_LANDSCAPE_LABEL "\x09\xff"
    /* TEXT_SCRIPT_COMPLETION_CAPTION */
    TEXT_CALL_POSITION_TOP_LEFT TEXT_CALL_SECRET_ENTRY_CODE_LABEL TEXT_CALL_POSITION_TOP_INSET
    TEXT_CALL_LANDSCAPE_LABEL "\x09\xff"
    /* TEXT_SCRIPT_COMPLETION_PROMPT */
    TEXT_CALL_POSITION_BOTTOM_LEFT TEXT_CALL_PRESS_ANY_KEY "\xff"
    /* TEXT_SCRIPT_POSITION_TOP_LEFT */
    "\x1f\x01\x01\xff\x00\x00\x00"
    /* TEXT_SCRIPT_POSITION_TOP_INSET */
    "\x1f\x03\x03\xff\x00\x00\x00"
    /* TEXT_SCRIPT_POSITION_BOTTOM_INSET */
    "\x1f\x03\x17\xff\x00\x00\x00"
    /* TEXT_SCRIPT_PROMPT_STYLE */
    "\x05\x12\x00\x80\xff"
    /* TEXT_SCRIPT_CAPTION_STYLE */
    "\x05\x12\x00\x80\xff"
    /* TEXT_SCRIPT_POSITION_BOTTOM_LEFT */
    "\x1f\x01\x16\xff\x00\x00\x00"
    /* TEXT_SCRIPT_LANDSCAPE_LABEL */
    "LANDSCAPE\xff"
    /* TEXT_SCRIPT_SECRET_ENTRY_CODE_LABEL */
    "SECRET\x09"
    "ENTRY\x09"
    "CODE\xff"
    /* TEXT_SCRIPT_SKIP_FIVE_GLYPHS */
    "\x09\x09\x09\x09\x09\xff"
    /* TEXT_SCRIPT_SKIP_THREE_GLYPHS */
    "\x09\x09\x09\xff"
    /* TEXT_SCRIPT_PRESS_ANY_KEY */
    "PRESS\x09"
    "ANY\x09KEY\xff\x00\x00";
// clang-format on
struct TextInlineControlBytesLeftStorage text_inline_control_bytes_left_storage = {.value = 0x6};
uint8_t game_text_pending_opcode = 0x0;
uint8_t game_text_parameter_index = 0x0;
uint16_t game_text_alternate_mode = 0x4;
uint16_t game_text_transparent = 0x0;
uint16_t game_text_glyph_flags = 0x0;
uint16_t game_text_normal_foreground = 0xf;
uint16_t game_text_normal_background = 0x0;
uint16_t game_text_alternate_foreground = 0xc;
uint16_t game_text_alternate_background = 0xe;
uint16_t game_text_active_foreground = 0xc;
uint16_t game_text_active_background = 0xe;
uint16_t game_text_column = 0x0;
uint16_t game_text_row = 0x0;
struct GameTextInputCharacterStorage game_text_input_character_storage = {0};
uint8_t game_text_script_offsets[TEXT_SCRIPT_COUNT] = {
    [TEXT_SCRIPT_TITLE_PROMPT] = 0x17,
    [TEXT_SCRIPT_LANDSCAPE_NUMBER_PROMPT] = 0x1d,
    [TEXT_SCRIPT_SECRET_CODE_PROMPT] = 0x30,
    [TEXT_SCRIPT_WRONG_SECRET_CODE] = 0x39,
    [TEXT_SCRIPT_PREVIEW_CAPTION] = 0x4e,
    [TEXT_SCRIPT_COMPLETION_CAPTION] = 0x57,
    [TEXT_SCRIPT_COMPLETION_PROMPT] = 0x5d,
    [TEXT_SCRIPT_POSITION_TOP_LEFT] = 0x60,
    [TEXT_SCRIPT_POSITION_TOP_INSET] = 0x67,
    [TEXT_SCRIPT_POSITION_BOTTOM_INSET] = 0x6e,
    [TEXT_SCRIPT_PROMPT_STYLE] = 0x75,
    [TEXT_SCRIPT_CAPTION_STYLE] = 0x7a,
    [TEXT_SCRIPT_POSITION_BOTTOM_LEFT] = 0x7f,
    [TEXT_SCRIPT_LANDSCAPE_LABEL] = 0x86,
    [TEXT_SCRIPT_SECRET_ENTRY_CODE_LABEL] = 0x90,
    [TEXT_SCRIPT_SKIP_FIVE_GLYPHS] = 0xa2,
    [TEXT_SCRIPT_SKIP_THREE_GLYPHS] = 0xa8,
    [TEXT_SCRIPT_PRESS_ANY_KEY] = 0xac,
};
