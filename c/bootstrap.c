/* DOS main: choose an adapter, initialize its buffers, decode both resource
 * images, enter the game and release resources after normal hardware cleanup. */
extern uint8_t filename_skybox[], filename_tfinal[], msg_unsupported_adapter[];

struct MenuAddEnd __far bootstrap_option(
    SegmentBytes strings, uint16_t id, uint16_t name, int backwards)
{
    return c_menu_add(strings, id, name, backwards);
}

struct MenuSelectionEnd __far bootstrap_menu(
    SegmentBytes data, SegmentBytes strings, uint16_t initial, uint16_t flags)
{
    struct MenuSelectionEnd end = c_menu_select(data, strings, initial, flags);
    graphics_adapter = end.adapter;
    return end;
}

/* The compiler may use DS/ES for far data while globals remain SS-relative.
 * Enter the native session with the game DS explicitly, then restore both
 * caller segments. The game restores the C caller's SI, DI and BP. */
static void run_hardware_session(void)
{
    __asm__ volatile("pushw %%ds\n\tpushw %%es\n\tpushw %%ss\n\tpopw %%ds\n\tpushw %%ss\n\tpopw "
                     "%%es\n\tcall hardware_enter_game\n\tpopw %%es\n\tpopw %%ds"
        :
        :
        : "ax", "bx", "cx", "dx", "memory", "cc");
}

struct BootstrapEnd __far c_game_bootstrap(SegmentBytes data, SegmentBytes strings, uint16_t flags)
{
    struct BootstrapEnd end = {0};
    struct VideoModeEnd mode = c_bios_get_mode(flags);
    uint16_t saved_mode = mode.mode;
    graphics_session.menu_count = 0;
    flags = mode.flags;
    if (saved_mode == 0 || saved_mode == 2 || saved_mode == 5 || saved_mode == 7) {
        segment_record(data, graphics_session).text_mode = 1;
        menu_normal_background = menu_selected_foreground = 0;
        menu_normal_foreground = menu_selected_background = 7;
    } else
        segment_record(data, graphics_session).text_mode = 0;
    {
        struct AdapterDetectEnd detected =
            c_detect_adapters(data, (uint16_t)&graphics_session.probe_results, 0, flags);
        flags = detected.flags;
    }
    uint16_t adapter = segment_record(data, graphics_session).probe_results.entries[0].adapter;
    segment_record(data, graphics_session).detected_card = adapter;
    if (adapter == 1 || (adapter >= 0x80 && adapter <= 0x82)) {
        SegmentBytes scan_start = strings + (uint16_t)msg_unsupported_adapter;
        SegmentBytes terminator_start = data + (uint16_t)msg_unsupported_adapter;
        SegmentBytes display_start = segment_at_offset(data, (uint16_t)msg_unsupported_adapter);
        struct DosPrintEnd printed =
            c_dos_print(scan_start, terminator_start, display_start, !!(flags & 0x400), flags);
        struct AdapterNameEnd named = c_adapter_name(data, adapter);
        uint16_t name = named.name;
        scan_start = strings + name;
        terminator_start = data + name;
        display_start = segment_at_offset(data, name);
        printed = c_dos_print(
            scan_start, terminator_start, display_start, !!(printed.flags & 0x400), printed.flags);
        end.status = 1;
        end.flags = printed.flags;
        return end;
    }
    switch (adapter) {
    case 2:
        graphics_adapter = 2;
        break;
    case 3:
        bootstrap_option(strings, 2, (uint16_t)name_cga, !!(flags & 0x400));
        bootstrap_option(strings, 3, (uint16_t)name_tandy, !!(flags & 0x400));
        {
            struct MenuChoice selected = choose_adapter_menu(data, strings, 1, flags, 0);
            graphics_adapter = selected.adapter;
            flags = selected.flags;
        }
        break;
    case 4:
        if (!segment_record(data, graphics_session).text_mode) {
            bootstrap_option(strings, 2, (uint16_t)name_cga, !!(flags & 0x400));
            bootstrap_option(strings, 4, (uint16_t)name_ega, !!(flags & 0x400));
            {
                struct MenuChoice selected = choose_adapter_menu(data, strings, 1, flags, 0);
                graphics_adapter = selected.adapter;
                flags = selected.flags;
            }
        } else
            graphics_adapter = 2;
        break;
    case 6:
        bootstrap_option(strings, 2, (uint16_t)name_cga, !!(flags & 0x400));
        bootstrap_option(strings, 4, (uint16_t)name_ega, !!(flags & 0x400));
        bootstrap_option(strings, 5, (uint16_t)name_vga, !!(flags & 0x400));
        {
            struct MenuChoice selected = choose_adapter_menu(data, strings, 2, flags, 0);
            graphics_adapter = selected.adapter;
            flags = selected.flags;
        }
        segment_word_store(data, ((uint16_t)&video_display_enable_wait_counts), 32);
        segment_word_store(data, ((uint16_t)&video_display_enable_wait_counts + 2), 32);
        segment_word_store(data, ((uint16_t)&video_display_enable_wait_counts + 4), 20);
        segment_word_store(data, ((uint16_t)&video_display_enable_wait_counts + 6), 48);
        break;
    default:
        segment_record(data, graphics_session).detected_card = 0;
        graphics_adapter = 2;
        break;
    }
    struct GraphicsInitEnd graphics =
        initialize_graphics(data, strings, graphics_adapter - 2, flags);
    flags = graphics.mode.flags;
    strings = data;
    DecodeCallback decoder = segment_record(data, graphics_session).decode;
    struct LoadedResource resource =
        load_scene_resource(data, strings, (uint16_t)filename_skybox, decoder, flags, 0);
    skybox_image_segment = (uint32_t)resource.buffer >> 16;
    flags = resource.flags;
    resource = load_scene_resource(data, strings, (uint16_t)filename_tfinal, decoder, flags, 0);
    title_final_image_segment = (uint32_t)resource.buffer >> 16;
    run_hardware_session();
    flags = resource.flags;
    if (video_work_buffer_segment != 0xa000) {
        struct DosOpenEnd freed = c_dos_free(video_work_buffer_segment, flags);
        flags = freed.flags;
    }
    if (segment_record(data, graphics_session).scroll_segment) {
        struct DosOpenEnd freed =
            c_dos_free(segment_record(data, graphics_session).scroll_segment, flags);
        flags = freed.flags;
    }
    {
        struct DosOpenEnd freed = c_dos_free(title_final_image_segment, flags);
        flags = freed.flags;
        freed = c_dos_free(skybox_image_segment, flags);
        flags = freed.flags;
    }
    {
        struct VideoModeSetEnd restored = bios_mode(saved_mode, flags);
        end.flags = status(restored.flags, 0, 16);
    }
    return end;
}
