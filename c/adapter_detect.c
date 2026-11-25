/* Probe records are visited in order: BIOS display combination, EGA switches,
 * Tandy columns, color CRTC and monochrome CRTC. Successful probes disable
 * redundant later probes and fill at most two adapter descriptors. */

/* Keep original unaligned whole-word transfers at the segment boundary. */
static struct AdapterDescriptor load_adapter_descriptor(
    const volatile struct AdapterDescriptor GAME_FAR *descriptor)
{
    return adapter_descriptor_from_word(
        ((const volatile struct UnalignedWord GAME_FAR *)descriptor)->value);
}

static void store_adapter_descriptor(
    volatile struct AdapterDescriptor GAME_FAR *destination, struct AdapterDescriptor descriptor)
{
    ((volatile struct UnalignedWord GAME_FAR *)destination)->value =
        adapter_descriptor_word(descriptor);
}

static volatile struct AdapterResults GAME_FAR *adapter_results(SegmentBytes data, uint16_t result)
{
    return (volatile struct AdapterResults GAME_FAR *)(data + result);
}

struct AdapterStoreEnd __far c_probe_store_adapter(
    SegmentBytes data, uint16_t result, struct AdapterDescriptor descriptor)
{
    volatile struct AdapterResults GAME_FAR *results = adapter_results(data, result);
    struct AdapterStoreEnd end;
    end.previous = results->entries[0].adapter;
    unsigned entry = results->entries[0].adapter ? 1 : 0;
    end.destination = result + entry * sizeof(struct AdapterDescriptor);
    store_adapter_descriptor(&results->entries[entry], descriptor);
    return end;
}

struct AdapterProbeEnd __far c_probe_display_combination(
    SegmentBytes data, uint16_t result, uint16_t flags)
{
    struct AdapterProbeEnd end = {0};
    struct PlatformReply reply = platform_video(0x1a00, 0, 0, 0, 0, 0, flags);
    end.value = reply.value;
    end.flags = subtract8(reply.flags, (uint8_t)reply.value, 0x1a);
    if ((uint8_t)reply.value != 0x1a)
        return end;
    end.switches = reply.handle;
    end.switches_written = 1;
    end.index_mask = 0xffff;
    if ((reply.handle >> 8) >= 13 || (reply.handle & 255) >= 13)
        return end;
    volatile struct AdapterResults GAME_FAR *results = adapter_results(data, result);
    const volatile struct AdapterDescriptor GAME_FAR *table =
        (const volatile struct AdapterDescriptor GAME_FAR *)(data +
            (uint16_t)bios_display_combination_descriptors);
    if (reply.handle >> 8)
        store_adapter_descriptor(
            &results->entries[1], load_adapter_descriptor(&table[reply.handle >> 8]));
    struct AdapterDescriptor primary = load_adapter_descriptor(&table[reply.handle & 255]);
    end.value = adapter_descriptor_word(primary);
    store_adapter_descriptor(&results->entries[0], primary);
    video_probe_color_crtc_enabled = video_probe_mode8_columns_enabled =
        video_probe_ega_configuration_enabled = video_probe_mono_crtc_enabled = 0;
    end.value &= 0xff00;
    unsigned entry = 0;
    end.flags = subtract8(end.flags, results->entries[0].adapter, 1);
    if (results->entries[0].adapter != 1) {
        entry = 1;
        end.flags = subtract8(end.flags, results->entries[1].adapter, 1);
    }
    if (results->entries[entry].adapter == 1) {
        store_adapter_descriptor(&results->entries[entry], (struct AdapterDescriptor){0});
        video_probe_mono_crtc_enabled = 1;
    }
    end.index = result + entry * sizeof(struct AdapterDescriptor);
    return end;
}

struct AdapterProbeEnd __far c_probe_ega_configuration(
    SegmentBytes data, uint16_t result, uint8_t previous_low, uint16_t flags)
{
    struct AdapterProbeEnd end = {0};
    struct PlatformReply reply = platform_video(0x1200 | previous_low, 16, 0, 0, 0, 0, flags);
    end.value = reply.value;
    end.index = reply.handle;
    end.index_mask = 255;
    end.flags = subtract8(reply.flags, (uint8_t)reply.handle, 16);
    if ((uint8_t)reply.handle == 16)
        return end;
    end.switches = reply.count;
    end.switches_written = 1;
    end.index_mask = 0xffff;
    uint16_t adapter = (reply.handle & 0xff00) | (reply.handle & 3);
    end.index = adapter;
    end.flags = subtract8(end.flags, (uint8_t)adapter, 3);
    if ((uint8_t)adapter != 3)
        return end;
    if (((uint8_t)reply.count >> 1) >= 6)
        return end;
    struct AdapterDescriptor descriptor = {4, ega_switch_subtypes[(uint8_t)reply.count >> 1]};
    end.value = adapter_descriptor_word(descriptor);
    {
        struct AdapterStoreEnd stored = c_probe_store_adapter(data, result, descriptor);
        end.index = stored.destination;
    }
    end.flags = subtract8(end.flags, descriptor.display_class, 1);
    if (descriptor.display_class == 1)
        video_probe_mono_crtc_enabled = 0;
    else
        video_probe_color_crtc_enabled = video_probe_mode8_columns_enabled = 0;
    return end;
}

struct AdapterProbeEnd __far c_probe_tandy_columns(
    SegmentBytes data, uint16_t result, uint16_t previous_index, uint16_t flags)
{
    struct AdapterProbeEnd end = {0};
    struct PlatformReply saved = platform_video(0x0f00, previous_index, 0, 0, 0, 0, flags);
    struct PlatformReply mode = platform_video(8, saved.handle, 0, 0, 0, 0, saved.flags);
    mode = platform_video(0x0f00 | (mode.value & 255), mode.handle, 0, 0, 0, 0, mode.flags);
    flags = subtract8(mode.flags, mode.value >> 8, 20);
    uint16_t adapter_index = mode.handle;
    if ((mode.value >> 8) == 20) {
        struct AdapterStoreEnd stored =
            c_probe_store_adapter(data, result, (struct AdapterDescriptor){3, 2});
        adapter_index = stored.destination;
        flags = subtract8(flags, stored.previous, 0);
        video_probe_color_crtc_enabled = 0;
    }
    mode = platform_video(saved.value & 255, adapter_index, 0, 0, 0, 0, flags);
    end.index = mode.handle;
    end.index_mask = 0xffff;
    end.value = mode.value;
    end.flags = mode.flags;
    return end;
}

struct CrtcProbeEnd __far c_probe_crtc_register(uint16_t port)
{
    port_out8(port, 15);
    ++port;
    struct CrtcProbeEnd end;
    end.saved = port_in8(port);
    port_out8(port, 0x66);
    unsigned delay;
    for (delay = 0; delay < 256; ++delay)
        __asm__ volatile("" ::: "memory");
    end.observed = port_in8(port);
    port_out8(port, end.saved);
    return end;
}

struct CrtcAdapterEnd __far c_probe_color_crtc(SegmentBytes data, uint16_t result)
{
    struct CrtcAdapterEnd end = {0};
    end.probe = c_probe_crtc_register(0x3d4);
    if (end.probe.observed == 0x66) {
        end.accepted = 1;
        end.descriptor = (struct AdapterDescriptor){2, 2};
        end.store = c_probe_store_adapter(data, result, end.descriptor);
    }
    return end;
}

struct CrtcAdapterEnd __far c_probe_mono_crtc(SegmentBytes data, uint16_t result)
{
    struct CrtcAdapterEnd end = {0};
    end.probe = c_probe_crtc_register(0x3b4);
    if (end.probe.observed != 0x66)
        return end;
    uint8_t first = port_in8(0x3ba) & 128;
    uint8_t current = first;
    unsigned remaining;
    for (remaining = 0x8000; remaining; --remaining) {
        current = port_in8(0x3ba) & 128;
        if (current != first)
            break;
    }
    end.remaining = remaining ? remaining - 1 : 0;
    if (current == first)
        end.descriptor = (struct AdapterDescriptor){1, 1};
    else {
        uint8_t subtype = port_in8(0x3ba) & 0x70;
        uint8_t id = subtype == 8 ? 0x81 : subtype == 0x50 ? 0x82 : 0x80;
        end.descriptor = (struct AdapterDescriptor){id, id == 0x82 ? 3 : 1};
    }
    end.accepted = 1;
    end.store = c_probe_store_adapter(data, result, end.descriptor);
    return end;
}

struct AdapterNormalizeEnd __far c_probe_normalize(
    SegmentBytes data, uint16_t result, uint16_t flags)
{
    struct AdapterNormalizeEnd end = {0};
    volatile struct AdapterResults GAME_FAR *results = adapter_results(data, result);
    struct AdapterDescriptor primary = load_adapter_descriptor(&results->entries[0]);
    struct AdapterDescriptor secondary = load_adapter_descriptor(&results->entries[1]);
    end.flags = subtract16(flags, adapter_descriptor_word(secondary), 0);
    if (!secondary.adapter && !secondary.display_class)
        return end;
    end.flags = subtract8(end.flags, primary.adapter, 5);
    if ((int8_t)primary.adapter >= 5)
        return end;
    end.flags = subtract8(end.flags, secondary.adapter, 5);
    if ((int8_t)secondary.adapter >= 5)
        return end;
    struct PlatformReply mode = platform_video(0x0f00, 0, 0, 0, 0, 0, end.flags);
    uint8_t bios_mode = mode.value & 7;
    end.value = (mode.value & 0xff00) | bios_mode;
    end.page = mode.handle >> 8;
    end.queried = 1;
    end.flags = subtract8(mode.flags, bios_mode, 7);
    if ((bios_mode == 7 && primary.display_class != 1) ||
        (bios_mode != 7 && primary.display_class == 1)) {
        store_adapter_descriptor(&results->entries[0], secondary);
        store_adapter_descriptor(&results->entries[1], primary);
        end.value = adapter_descriptor_word(secondary);
    }
    return end;
}

struct AdapterDetectEnd __far c_detect_adapters(
    SegmentBytes data, uint16_t result, uint16_t previous_index, uint16_t flags)
{
    struct AdapterDetectEnd end = {0};
    uint16_t adapter_index = previous_index;
    volatile struct AdapterResults GAME_FAR *results = adapter_results(data, result);
    results->entries[0].adapter = results->entries[1].adapter = 0;
    video_probe_display_combination_enabled = video_probe_ega_configuration_enabled =
        video_probe_mode8_columns_enabled = video_probe_color_crtc_enabled =
            video_probe_mono_crtc_enabled = 1;
    volatile struct AdapterProbeList GAME_FAR *probes = &segment_record(data, adapter_probe_list);
    unsigned probe;
    for (probe = 0; probe < 5; ++probe) {
        /* Keep the unaligned word load before the live enabled-byte read. */
        uint16_t kind = probes->records[probe].kind;
        uint8_t enabled = probes->records[probe].enabled;
        end.value = kind;
        flags = status(flags, enabled, 8);
        if (!enabled)
            continue;
        struct AdapterProbeEnd bios;
        switch (kind) {
        case PROBE_COLOR_CRTC:
        case PROBE_MONO_CRTC: {
            int mono = kind == PROBE_MONO_CRTC;
            struct CrtcAdapterEnd crtc =
                mono ? c_probe_mono_crtc(data, result) : c_probe_color_crtc(data, result);
            end.port = mono ? 0x3b5 : 0x3d5;
            end.port_written = 1;
            end.value = ((uint16_t)crtc.probe.observed << 8) | crtc.probe.saved;
            flags = subtract8(flags, crtc.probe.observed, 0x66) | CF;
            if (crtc.accepted) {
                if (mono)
                    end.port = 0x3ba;
                end.value = adapter_descriptor_word(crtc.descriptor);
                adapter_index = crtc.store.destination;
                flags = subtract8(flags, crtc.store.previous, 0);
            }
            continue;
        }
        case PROBE_DISPLAY_COMBINATION:
            bios = c_probe_display_combination(data, result, flags);
            break;
        case PROBE_EGA_CONFIGURATION:
            bios = c_probe_ega_configuration(data, result, kind, flags);
            break;
        default:
            /* Unrecognized records follow the original Tandy fallback. */
            bios = c_probe_tandy_columns(data, result, adapter_index, flags);
            break;
        }
        end.value = bios.value;
        adapter_index = (adapter_index & ~bios.index_mask) | bios.index;
        flags = bios.flags;
    }
    struct AdapterNormalizeEnd normalized = c_probe_normalize(data, result, flags);
    end.flags = normalized.flags;
    if (normalized.queried) {
        end.value = normalized.value;
        adapter_index = (adapter_index & 255) | (normalized.page << 8);
    }
    end.index = adapter_index;
    return end;
}

/* Owned subsystem data. */
struct AdapterProbeList adapter_probe_list = {.records =
                                                  {
                                                      {0, PROBE_DISPLAY_COMBINATION},
                                                      {0, PROBE_EGA_CONFIGURATION},
                                                      {0, PROBE_TANDY_COLUMNS},
                                                      {0, PROBE_COLOR_CRTC},
                                                      {0, PROBE_MONO_CRTC},
                                                  },
    .terminator = 0};
uint8_t ega_switch_subtypes[6] = {
    0x02,
    0x03,
    0x01,
    0x02,
    0x03,
    0x01,
};
struct AdapterDescriptor bios_display_combination_descriptors[13] = {
    {0, 0},
    {1, 1},
    {2, 2},
    {0, 0},
    {4, 3},
    {4, 1},
    {0, 0},
    {6, 4},
    {6, 5},
    {0, 0},
    {5, 3},
    {5, 4},
    {5, 5},
};
