/* Adapter bindings, menu layout and resource loading use typed C services. */
#include <stdint.h>
#include "register_compat.h"
#include "typed_api.h"
#include "legacy_targets.h"
#include "data_aliases.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"

extern uint8_t msg_cannot_find[], msg_no_memory_1[];

static const struct VideoCallbacks video_backends[4] = {
    {
        .palette = video_palette_cga,
        .colors = video_colors_cga,
        .decode = video_decode_cga,
        .blit = video_blit_cga,
        .glyph = video_font_cga,
        .tile = video_tile_cga,
        .background = video_background_cga,
        .scatter = video_scatter_cga,
        .spans = video_spans_cga,
        .draw_cursor = video_draw_cursor_cga,
        .restore_cursor = video_restore_cursor_cga,
        .front = video_front_cga,
        .select_work = video_work_segment,
        .warning = video_warning_cga,
        .selection_copy = video_selection_copy_cga,
        .reveal = video_reveal_cga,
    },
    {
        .palette = video_palette_tandy,
        .colors = video_colors_tandy,
        .decode = video_decode_packed,
        .blit = video_blit_tandy,
        .glyph = video_font_packed,
        .tile = video_tile_tandy,
        .background = video_background_tandy,
        .scatter = video_scatter_tandy,
        .spans = video_spans_tandy,
        .draw_cursor = video_draw_cursor_tandy,
        .restore_cursor = video_restore_cursor_tandy,
        .front = video_front_tandy,
        .select_work = video_work_segment,
        .warning = video_warning_tandy,
        .selection_copy = video_selection_copy_tandy,
        .reveal = video_reveal_tandy,
    },
    {
        .palette = video_palette_ega,
        .colors = video_colors_ega,
        .decode = video_decode_ega,
        .blit = video_blit_ega,
        .glyph = video_font_ega,
        .tile = video_tile_ega,
        .background = video_background_ega,
        .scatter = video_scatter_ega,
        .spans = video_spans_ega,
        .draw_cursor = video_draw_cursor_ega,
        .restore_cursor = video_restore_cursor_ega,
        .front = video_front_ega,
        .select_work = video_work_ega,
        .warning = video_warning_ega,
        .selection_copy = video_selection_copy_ega,
        .reveal = video_reveal_ega,
    },
    {
        .palette = video_palette_vga,
        .colors = video_colors_vga,
        .decode = video_decode_packed,
        .blit = video_blit_vga,
        .glyph = video_font_vga,
        .tile = video_tile_vga,
        .background = video_background_vga,
        .scatter = video_scatter_vga,
        .spans = video_spans_vga,
        .draw_cursor = video_draw_cursor_vga,
        .restore_cursor = video_restore_cursor_vga,
        .front = video_front_vga,
        .select_work = video_work_segment,
        .warning = video_warning_vga,
        .selection_copy = video_selection_copy_vga,
        .reveal = video_reveal_vga,
    },
};

/* Keep the near line pointer visible at this call boundary before converting
 * it to the supplied segment's offset. The caller must publish its buffer. */
static __attribute__((noinline)) uint16_t menu_write_line(SegmentBytes data,
    uint16_t row,
    uint16_t column,
    uint8_t *line,
    uint16_t control,
    struct ConsoleTextEnd *observation)
{
    return write_console_line(data + (uint16_t)line, row, column, control, observation);
}

struct MenuAddEnd __far c_menu_add(SegmentBytes strings, uint16_t id, uint16_t name, int backwards)
{
    struct TextLengthEnd length = c_string_length(strings + name, backwards);
    uint16_t index = graphics_session.menu_count;
    graphics_session.menu_ids[index] = id;
    graphics_session.menu_names[index] = name;
    /* Reverse SCASB and the unterminated bound retain their historical extent. */
    graphics_session.menu_lengths[index] =
        backwards || length.length == 0xffff ? length.legacy_extent : length.length;
    ++graphics_session.menu_count;
    struct MenuAddEnd end;
    end.index = index;
    end.remaining = length.remaining;
    end.count = graphics_session.menu_count;
    return end;
}

uint16_t __far draw_menu_box(SegmentBytes data,
    uint16_t row,
    uint16_t column,
    uint16_t height,
    uint16_t width,
    uint16_t control,
    struct ConsoleTextEnd *observation)
{
    uint8_t line[42];
    set_console_attribute(menu_normal_foreground, menu_normal_background);
    line[0] = 0xc9;
    unsigned i;
    for (i = 0; i < width; ++i)
        line[i + 1] = 0xcd;
    line[width + 1] = 0xbb;
    line[width + 2] = 0;
    control = menu_write_line(data, row, column, line, control, observation);
    unsigned y;
    for (y = 1; y <= height; ++y) {
        line[0] = 0xba;
        line[1] = 0;
        control = menu_write_line(data, row + y, column, line, control, observation);
        line[1] = 0xdb;
        line[2] = 0;
        control = menu_write_line(data, row + y, column + width + 1, line, control, observation);
    }
    line[0] = 0xc8;
    for (i = 0; i < width; ++i)
        line[i + 1] = 0xcd;
    line[width + 1] = 0xbc;
    line[width + 2] = 0xdb;
    line[width + 3] = 0;
    control = menu_write_line(data, row + height + 1, column, line, control, observation);
    set_console_attribute(menu_normal_foreground, 0);
    for (i = 0; i < width + 2; ++i)
        line[i] = 0xdf;
    line[width + 2] = 0;
    return menu_write_line(data, row + height + 2, column + 1, line, control, observation);
}

struct ConsoleTextEnd __far menu_box(SegmentBytes data,
    uint16_t row,
    uint16_t column,
    uint16_t height,
    uint16_t width,
    uint16_t flags)
{
    struct ConsoleTextEnd end;
    draw_menu_box(data, row, column, height, width, flags, &end);
    return end;
}

struct ConsoleTextEnd __far c_menu_frame(SegmentBytes data,
    uint16_t row,
    uint16_t column,
    uint16_t height,
    uint16_t width,
    uint16_t flags)
{
    return menu_box(data, row, column, height, width, flags);
}

uint16_t __far write_menu_option(
    SegmentBytes data, uint16_t option, uint16_t control, struct ConsoleTextEnd *observation)
{
    uint8_t line[44];
    uint16_t width = segment_record(data, graphics_session).menu_width;
    uint16_t length = graphics_session.menu_lengths[option];
    int16_t left = (int16_t)(width - length) / 2;
    unsigned i;
    for (i = 0; i < width; ++i)
        line[i] = 32;
    for (i = 0; i < length; ++i)
        line[left + i] = data[graphics_session.menu_names[option] + i];
    line[width] = 0;
    return menu_write_line(data,
        segment_record(data, graphics_session).menu_row + option + 1,
        segment_record(data, graphics_session).menu_column + 1,
        line,
        control,
        observation);
}

struct ConsoleTextEnd __far menu_option(SegmentBytes data, uint16_t option, uint16_t flags)
{
    struct ConsoleTextEnd end;
    write_menu_option(data, option, flags, &end);
    return end;
}

struct ConsoleTextEnd __far c_menu_option(SegmentBytes data, uint16_t option, uint16_t flags)
{
    return menu_option(data, option, flags);
}

uint16_t __far draw_menu_options(
    SegmentBytes data, uint16_t control, struct ConsoleTextEnd *observation)
{
    uint16_t width = 0;
    uint16_t count = graphics_session.menu_count;
    set_console_attribute(menu_normal_foreground, menu_normal_background);
    uint16_t i;
    for (i = 0; i < count; ++i)
        if (graphics_session.menu_lengths[i] > width)
            width = graphics_session.menu_lengths[i];
    width = (width + 3) & 0xfffe;
    segment_record(data, graphics_session).menu_row = 13 - count / 2;
    segment_record(data, graphics_session).menu_column = 20 - (width + 2) / 2;
    segment_record(data, graphics_session).menu_width = width;
    control = draw_menu_box(data,
        segment_record(data, graphics_session).menu_row,
        segment_record(data, graphics_session).menu_column,
        count,
        width,
        control,
        observation);
    if (observation) {
        observation->value = c_text_attribute(menu_normal_foreground, menu_normal_background);
        observation->flags = status(
            control, (menu_normal_foreground & 15) | (uint8_t)(menu_normal_background << 4), 8);
    } else
        set_console_attribute(menu_normal_foreground, menu_normal_background);
    for (i = 0; i < count; ++i)
        control = write_menu_option(data, i, control, observation);
    return control;
}

struct ConsoleTextEnd __far c_menu_options(SegmentBytes data, uint16_t flags)
{
    struct ConsoleTextEnd end;
    draw_menu_options(data, flags, &end);
    return end;
}

struct LoadedResource __far load_scene_resource(SegmentBytes data,
    SegmentBytes strings,
    uint16_t name,
    DecodeCallback decoder,
    uint16_t flags,
    struct ResourceLoadEnd *observation)
{
    enum ResourceFormat format = decoder == video_decode_cga ? RESOURCE_CGA
        : decoder == video_decode_ega                        ? RESOURCE_EGA
                                                             : RESOURCE_PACKED;
    struct DosOpenEnd opened = c_dos_open(segment_at_offset(data, name), 0, flags);
    if (!opened.handle) {
        SegmentBytes scan_start = strings + (uint16_t)msg_cannot_find;
        SegmentBytes terminator_start = data + (uint16_t)msg_cannot_find;
        SegmentBytes display_start = segment_at_offset(data, (uint16_t)msg_cannot_find);
        struct DosPrintEnd message = c_dos_print(
            scan_start, terminator_start, display_start, !!(opened.flags & 0x400), opened.flags);
        menu_fatal(data, strings, name, message.flags);
    }
    struct DosSizeEnd size = c_dos_size(opened.handle, opened.flags);
    uint16_t length = graphics_adapter == 2 ? 0x3e80 : size.low;
    struct DosMemoryEnd memory = c_dos_alloc((length >> 4) + 4, size.low, size.flags);
    if (!memory.segment)
        menu_fatal(data, strings, (uint16_t)msg_no_memory_1, memory.flags);
    struct LoadedResource loaded;
    loaded.buffer = segment_bytes(memory.segment);
    if (decoder == video_decode_cga || decoder == video_decode_ega ||
        decoder == video_decode_packed)
        loaded.flags = decode_resource(opened.handle,
            loaded.buffer,
            data,
            memory.flags,
            format,
            observation ? &observation->decode : 0);
    else {
        struct ResourceDecodeEnd opaque =
            call_video_decode(decoder, opened.handle, loaded.buffer, data, memory.flags);
        loaded.flags =
            resource_completion_flags(format, memory.flags, opaque.io_flags, opaque.carry);
        if (observation)
            observation->decode = opaque;
    }
    if (observation) {
        observation->buffer = loaded.buffer;
        observation->handle = opened.handle;
        observation->format = format;
        observation->flags = loaded.flags;
    }
    return loaded;
}

struct ResourceLoadEnd __far c_load_scene_resource(
    SegmentBytes data, SegmentBytes strings, uint16_t name, DecodeCallback decoder, uint16_t flags)
{
    struct ResourceLoadEnd end;
    load_scene_resource(data, strings, name, decoder, flags, &end);
    return end;
}

struct DosMemoryEnd __far graphics_allocate(
    SegmentBytes data, uint16_t paragraphs, uint16_t error, uint16_t initial_flags)
{
    struct DosMemoryEnd end = dos_allocate(paragraphs, 0, initial_flags);
    if (!end.segment)
        graphics_memory_error(data, error);
    return end;
}

struct GraphicsInitEnd __far initialize_graphics(
    SegmentBytes data, SegmentBytes output, unsigned mode, uint16_t initial_flags)
{
    const struct VideoCallbacks *callbacks = &video_backends[mode < 3 ? mode : 3];
    volatile struct GraphicsSession GAME_FAR *session = &segment_record(data, graphics_session);
    uint16_t flags = initial_flags;
    session->palette = callbacks->palette;
    session->colors = callbacks->colors;
    session->decode = callbacks->decode;
    session->blit = callbacks->blit;
    session->glyph = callbacks->glyph;
    session->tile = callbacks->tile;
    session->background = callbacks->background;
    session->scatter = callbacks->scatter;
    session->spans = callbacks->spans;
    session->draw_cursor = callbacks->draw_cursor;
    session->restore_cursor = callbacks->restore_cursor;
    session->front = callbacks->front;
    session->select_work = callbacks->select_work;
    session->warning = callbacks->warning;
    session->selection_copy = callbacks->selection_copy;
    session->reveal = callbacks->reveal;
    struct DosMemoryEnd memory;
    struct GraphicsInitEnd end;
    if (mode == 0) {
        uint16_t adapter = segment_record(data, graphics_session).detected_card;
        if ((adapter >= 2 && adapter <= 4) || adapter == 6) {
            session->transition = video_transition_cga;
            session->scroll_segment = 0;
        } else {
            session->transition = video_cga_scroll_software;
            memory = graphics_allocate(data, 0x400, ((uint16_t)msg_no_memory_2), flags);
            flags = memory.flags;
            segment_record(data, graphics_session).scroll_segment = memory.segment;
        }
        build_video_rows(output, !!(flags & 0x400), ROWS_CGA);
        memory = graphics_allocate(data, 0x400, ((uint16_t)msg_no_memory_3), flags);
        video_work_buffer_segment = memory.segment;
        end.mode =
            bios_mode(segment_record(data, graphics_session).text_mode ? 5 : 4, memory.flags);
    } else if (mode == 1) {
        session->transition = video_transition_tandy;
        build_video_rows(output, !!(flags & 0x400), ROWS_FOUR_BANK);
        memory = graphics_allocate(data, 0x800, ((uint16_t)msg_no_memory_4), flags);
        video_work_buffer_segment = memory.segment;
        end.mode = bios_mode(9, memory.flags);
    } else if (mode == 2) {
        uint16_t adapter = segment_record(data, graphics_session).detected_card;
        session->palette = adapter == 6 ? video_palette_vga_ega : video_palette_ega;
        session->colors = adapter == 6 ? video_colors_vga : video_colors_ega;
        session->transition = video_transition_ega;
        video_work_buffer_segment = 0xa000;
        end.mode = bios_mode(0x0d, flags);
        build_video_rows(output, !!(end.mode.flags & 0x400), ROWS_PLANAR);
    } else {
        session->transition = video_transition_vga;
        build_video_rows(output, !!(flags & 0x400), ROWS_LINEAR);
        memory = graphics_allocate(data, 0xffa, ((uint16_t)msg_no_memory_5), flags);
        video_work_buffer_segment = memory.segment;
        end.mode = bios_mode(0x13, memory.flags);
    }
    end.adapter_mode = mode;
    return end;
}

struct GraphicsInitEnd __far c_init_cga(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags)
{
    return initialize_graphics(data, output, 0, initial_flags);
}

struct GraphicsInitEnd __far c_init_tandy(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags)
{
    return initialize_graphics(data, output, 1, initial_flags);
}

struct GraphicsInitEnd __far c_init_ega(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags)
{
    return initialize_graphics(data, output, 2, initial_flags);
}

struct GraphicsInitEnd __far c_init_vga(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags)
{
    return initialize_graphics(data, output, 3, initial_flags);
}

/* Startup/menu/video bindings and segments, reset by the CRT. */
struct GraphicsSession graphics_session = {0};

void __far reset_graphics_session(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&graphics_session), 0, sizeof(graphics_session), 0);
}
