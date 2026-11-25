/* The DOS text-mode adapter menu, including its box-drawing frame and centered
 * options. Hardware operations use the same small BIOS/DOS boundary as the game. */

extern uint8_t msg_cannot_find[], msg_no_memory_1[];

struct KeyboardReadEnd __far c_bios_read_key(uint16_t initial_flags)
{
    struct PlatformReply reply = platform_keyboard(0, 0, 0, 0, 0, 0, status(initial_flags, 0, 16));
    struct KeyboardReadEnd end = {reply.value, reply.flags};
    return end;
}

struct MenuChoice __far choose_adapter_menu(SegmentBytes data,
    SegmentBytes strings,
    uint16_t selection,
    uint16_t flags,
    struct MenuSelectionEnd *observation)
{
    struct VideoModeSetEnd mode =
        bios_mode(segment_record(data, graphics_session).text_mode < 1 ? 1 : 0, flags);
    /* Hide-cursor inherits the COMPLETE mode reply, including arithmetic. */
    struct DosOpenEnd hidden = c_bios_hide_cursor(mode.crtc_port, mode.flags);
    uint16_t control = hidden.flags & ~ARITH_FLAGS;
    struct ConsoleTextEnd *line = observation ? &observation->text : 0;
    int backwards = !!(control & 0x400);
    struct TextLengthEnd length =
        c_string_length(strings + (uint16_t)msg_menu_navigation, backwards);
    uint16_t width = backwards || length.length == 0xffff ? length.legacy_extent : length.length;
    control = write_console_line(
        data + (uint16_t)msg_menu_navigation, 3, 20 - (width >> 1), control, line);
    backwards = !!(control & 0x400);
    length = c_string_length(strings + (uint16_t)msg_menu_select, backwards);
    width = backwards || length.length == 0xffff ? length.legacy_extent : length.length;
    control =
        write_console_line(data + (uint16_t)msg_menu_select, 4, 20 - (width >> 1), control, line);
    control = draw_menu_options(data, control, line);
    for (;;) {
        set_console_attribute(menu_selected_foreground, menu_selected_background);
        control = write_menu_option(data, selection, control, line);
        struct KeyboardReadEnd key = c_bios_read_key(control);
        uint8_t scan = key.key >> 8;
        if (scan == 0x1c) {
            struct MenuChoice end = {graphics_session.menu_ids[selection], selection, key.flags};
            if (observation) {
                observation->adapter = end.adapter;
                observation->selection = end.selection;
                observation->flags = end.flags;
            }
            return end;
        }
        control = key.flags & ~ARITH_FLAGS;
        if (scan == 0x48 || scan == 0x50) {
            set_console_attribute(menu_normal_foreground, menu_normal_background);
            control = write_menu_option(data, selection, control, line);
            if (scan == 0x48)
                selection = selection ? selection - 1 : graphics_session.menu_count - 1;
            else if (++selection >= graphics_session.menu_count)
                selection = 0;
        } else if (observation)
            line->flags = key.flags;
    }
}

struct MenuSelectionEnd __far c_menu_select(
    SegmentBytes data, SegmentBytes strings, uint16_t selection, uint16_t flags)
{
    struct MenuSelectionEnd end;
    choose_adapter_menu(data, strings, selection, flags, &end);
    return end;
}

struct AdapterNameEnd __far c_adapter_name(SegmentBytes data, uint8_t adapter)
{
    uint16_t index = adapter & 128 ? (adapter & 127) + 6 : adapter;
    struct AdapterNameEnd end = {index * 2, 0};
    if (index < 9)
        end.name = adapter_name_pointers[index].value;
    return end;
}

void __far menu_fatal(SegmentBytes data, SegmentBytes strings, uint16_t message, uint16_t flags)
{
    SegmentBytes scan_start = strings + message;
    SegmentBytes terminator_start = data + message;
    SegmentBytes display_start = segment_at_offset(data, message);
    c_dos_print(scan_start, terminator_start, display_start, !!(flags & 0x400), flags);
    abi_c_crt_exit(1);
}

/* Owned subsystem data. */
uint8_t text_attribute = 0x7;
uint8_t text_cursor_row = 0x0;
struct TextCursorColumnStorage text_cursor_column_storage = {0};
uint8_t filename_sent[10] = "sent.bin";
uint8_t msg_menu_navigation[42] = "Move to menu selection with cursor keys.";
uint8_t msg_menu_select[24] = "Press ENTER to select.";
uint8_t name_tandy[16] = "Tandy Graphics";
uint8_t name_ega[26] = "Enhanced Graphics Adaptor";
uint8_t name_vga[22] = "Video Graphics Array";
uint8_t name_cga[24] = "Colour Graphics Adaptor";
uint16_t menu_normal_background = 0x1;
uint16_t menu_selected_background = 0x4;
uint16_t menu_normal_foreground = 0x3;
uint16_t menu_selected_foreground = 0x7;
