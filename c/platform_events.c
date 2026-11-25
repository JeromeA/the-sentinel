/* Keyboard, mouse and timer setup use C state updates and small interrupt/port
 * operations. Hardware interrupt prologues, IRET/RETF and the saved-SP restore
 * remain short ABI boundaries in the legacy code segment. */

/* Arm the return into the preview dispatch within the hardware session's
 * saved-stack protocol. Capture and patch deliberately evaluate the saved SP
 * separately: a supplied observation may alias that word. The volatile second
 * evaluation prevents the compiler from reusing the pre-capture value. */
extern void landscape_preview_return_dispatch(void);

void __far arm_generation_preview(SegmentBytes data, struct TerrainFilterEnd *observation)
{
    if (observation) {
        observation->work_pointer = saved_hardware_sp;
        observation->work_pointer_set = 1;
    }
    segment_word_store(data,
        (uint16_t)(*(volatile uint16_t *)&saved_hardware_sp - 2),
        (uint16_t)&landscape_preview_return_dispatch);
}

static uint16_t game_data_segment(void)
{
    uint16_t value;
    __asm__("movw %%ss,%0" : "=r"(value));
    return value;
}

static uint16_t game_code_segment(void)
{
    uint16_t value;
    __asm__("movw %%cs,%0" : "=r"(value));
    return value;
}

static struct KeyboardScanEnd keyboard_scan_event(uint8_t scan)
{
    uint8_t index = scan & 127;
    if (scan == 0x3c) {
        sound_muted = ~sound_muted;
        sound_mute_saved = sound_muted;
    }
    struct KeyboardScanEnd end;
    end.index = ((uint16_t)(scan & 128) << 8) | index;
    end.prefix = scan == 0xe0;
    end.buttons = 0;
    if (end.prefix) {
        keyboard_e0_prefix = 255;
        return end;
    }
    end.index = index;
    key_down[index] = scan & 128 ? 0 : 255;
    input_buttons |= (key_down[0x7e] & 1) | (key_down[0x7d] & 2);
    end.buttons = input_buttons;
    return end;
}

struct KeyboardScanEnd __far c_keyboard_irq_scan(void)
{
    uint8_t scan = port_in8(0x60);
    uint8_t control = port_in8(0x61);
    port_out8(0x61, control | 128);
    port_out8(0x61, control);
    return keyboard_scan_event(scan);
}

struct KeyboardScanEnd __far c_keyboard_bios_scan(uint8_t scan)
{
    return keyboard_scan_event(scan);
}

struct DosOpenEnd __far set_interrupt_vector(
    unsigned vector, uint16_t offset, uint16_t segment, uint16_t flags)
{
    struct PlatformReply reply = platform_dos(0x2500 | vector, 0, 0, offset, segment, 0, flags);
    struct DosOpenEnd end = {reply.value, reply.flags};
    return end;
}

struct KeyboardInstallEnd __far c_install_keyboard(uint8_t request_low, uint16_t flags)
{
    struct PlatformReply config = platform_system_bios(0xc000 | request_low, 0, 0, 0, 0, 0, flags);
    int available = !(config.flags & CF) && !(config.value >> 8);
    int intercept = available && !!(segment_bytes(config.extra_segment)[config.handle + 5] & 16);
    struct KeyboardInstallEnd end;
    end.offset = intercept ? (uint16_t)bios_keyboard_intercept : (uint16_t)keyboard_irq_handler;
    end.vector =
        set_interrupt_vector(intercept ? 0x15 : 9, end.offset, game_code_segment(), config.flags);
    end.configuration_offset = config.handle;
    end.configuration_segment = config.extra_segment;
    end.configuration_available = available;
    return end;
}

/* Keep latch + low/high reads in one I/O boundary: neither byte may move
 * outside CLI/STI. The loop and maximum calculation remain C. */
static __attribute__((always_inline)) inline uint16_t pit_counter_sample(void)
{
    uint16_t value;
    __asm__ volatile("cli\n\txorb %%al,%%al\n\toutb %%al,$0x43\n\t"
                     "jmp 1f\n1:\tinb $0x40,%%al\n\tmovb %%al,%%ah\n\t"
                     "jmp 2f\n2:\tinb $0x40,%%al\n\txchgb %%al,%%ah\n\tsti"
        : "=a"(value)
        :
        : "memory", "cc");
    return value;
}

struct PitCalibrationEnd __far c_calibrate_pit(void)
{
    uint16_t maximum = 0;
    uint16_t sample = 0;
    unsigned pass;
    for (pass = 2; pass; --pass) {
        /* Match the original two passes of 65,535 samples. */
        uint16_t count = 0xffff;
        do {
            sample = pit_counter_sample();
            if (sample > maximum)
                maximum = sample;
        } while (--count);
    }
    {
        struct PitCalibrationEnd end = {maximum, sample};
        return end;
    }
}

/* DOS vector queries inherit the complete interrupt register state. Keep that
 * compatibility carrier here, including the ordered segment-relative saves. */
static struct PlatformReply capture_interrupt_vector(struct LegacyRegisters *state,
    unsigned vector,
    SegmentBytes saved_segment,
    uint16_t saved_offset)
{
    state->ax = 0x3500 | vector;
    native_invoke(state, (uint16_t)platform_int21);
    segment_word_store(saved_segment, saved_offset, state->bx);
    segment_word_store(saved_segment, saved_offset + 2, state->es);
    struct PlatformReply reply = {
        state->ax, state->bx, state->cx, state->dx, state->es, state->flags};
    return reply;
}

void c_hardware_enter(struct LegacyRegisters *r, uint32_t frame, uint16_t selector)
{
    SegmentBytes data = segment_bytes(r->ds);
    uint16_t code_segment = game_code_segment();
    SegmentBytes code = segment_bytes(code_segment);
    saved_hardware_sp = (uint16_t)frame + 20;
    struct MouseSetupEnd mouse = c_mouse_initialize(r->bx, r->flags);
    /* These partial outputs are inherited by the following DOS query. */
    r->bx = mouse.reply.handle;
    r->es = mouse.reply.extra_segment;
    r->flags = mouse.reply.flags;
    if (mouse.present) {
        r->cx = mouse.reply.count;
        r->dx = mouse.reply.offset;
    }
    capture_interrupt_vector(r, 9, data, (uint16_t)&saved_int09_offset);
    struct PlatformReply keyboard_request =
        capture_interrupt_vector(r, 0x15, code, (uint16_t)saved_int15_vector);
    struct KeyboardInstallEnd keyboard =
        c_install_keyboard(keyboard_request.value, keyboard_request.flags);
    r->dx = keyboard.offset;
    r->flags = keyboard.vector.flags;
    if (keyboard.configuration_available) {
        r->bx = keyboard.configuration_offset;
        r->es = keyboard.configuration_segment;
    }
    struct PlatformReply timer_request =
        capture_interrupt_vector(r, 8, code, (uint16_t)saved_int08_vector);
    struct PitCalibrationEnd calibration = c_calibrate_pit();
    pit_calibration_max = calibration.maximum;
    uint16_t calibration_flags = status(timer_request.flags, 0, 16) | 0x200 |
        (calibration.last < calibration.maximum ? CF : 0);
    int triple_rate = calibration.maximum > 0x6000;
    uint16_t timer_handler =
        triple_rate ? (uint16_t)timer_irq_triple_rate : (uint16_t)timer_irq_original_rate;
    struct DosOpenEnd timer_vector =
        set_interrupt_vector(8, timer_handler, code_segment, calibration_flags);
    if (triple_rate) {
        __asm__ volatile("cli" ::: "memory");
        port_out8(0x43, 0x36);
        port_out8(0x40, 0x55);
        port_out8(0x40, 0x55);
        __asm__ volatile("sti" ::: "memory");
        timer_vector.flags |= 0x200;
    }
    /* Publish the surviving calibration/vector outputs at the nonlocal startup
     * handoff. DOS-returned DS/DI/BP and all other inherited fields stay intact. */
    r->bx = calibration.last;
    r->cx = r->si = 0;
    r->dx = timer_handler;
    r->flags = timer_vector.flags;
    r->ax = r->ds;
    r->es = r->ds;
    typed_legacy_entry(r, frame, TYPED_startup_title_flow);
}

void c_hardware_leave(struct LegacyRegisters *r, uint32_t frame, uint16_t selector)
{
    SegmentBytes data = segment_bytes(r->ds);
    SegmentBytes code = segment_bytes(game_code_segment());
    uint8_t control = c_silence_all();
    uint16_t silence_flags = status(r->flags, control, 8) | 0x200;
    silence_flags = (subtract8(silence_flags, 0, 1) & ~CF) | (silence_flags & CF);
    struct MouseSetupEnd mouse = c_mouse_remove(255, r->dx, r->es, silence_flags);
    /* Only surviving compatibility outputs cross the teardown boundary. */
    r->si = 0;
    r->bx = mouse.present ? mouse.reply.handle : 255;
    r->cx = mouse.present ? mouse.reply.count : (r->cx & 0xff00) | 5;
    if (mouse.present)
        r->es = mouse.reply.extra_segment;
    uint16_t restore_flags = mouse.reply.flags;
    if (pit_calibration_max > 0x6000) {
        __asm__ volatile("cli" ::: "memory");
        port_out8(0x43, 0x36);
        port_out8(0x40, 0);
        port_out8(0x40, 0);
        __asm__ volatile("sti" ::: "memory");
        restore_flags |= 0x200;
    }
    /* Saved vector words are read offset first, immediately before each install. */
    uint16_t offset = segment_word(data, (uint16_t)&saved_int09_offset);
    uint16_t segment = segment_word(data, (uint16_t)&saved_int09_offset + 2);
    struct DosOpenEnd vector = set_interrupt_vector(9, offset, segment, restore_flags);
    offset = segment_word(code, (uint16_t)saved_int15_vector);
    segment = segment_word(code, (uint16_t)saved_int15_vector + 2);
    vector = set_interrupt_vector(0x15, offset, segment, vector.flags);
    offset = segment_word(code, (uint16_t)saved_int08_vector);
    segment = segment_word(code, (uint16_t)saved_int08_vector + 2);
    vector = set_interrupt_vector(8, offset, segment, vector.flags);
    r->ax = vector.handle;
    r->dx = offset;
    r->flags = vector.flags;
    legacy_tail(r, (uint16_t)&target_hardware_stack_return);
}

void c_timer_triple_body(struct LegacyRegisters *r, uint32_t frame, uint16_t selector)
{
    r->flags &= ~0x400;
    uint16_t control = c_timer_tick(segment_bytes(r->ds), segment_bytes(r->es), r->flags);
    r->flags = (r->flags & ARITH_FLAGS) | control;
    --bios_tick_prescaler;
    if (!bios_tick_prescaler) {
        bios_tick_prescaler = 3;
        legacy_tail(r, (uint16_t)&target_timer_chain_return);
    } else
        legacy_tail(r, (uint16_t)&target_timer_eoi_return);
}

void c_timer_original_body(struct LegacyRegisters *r, uint32_t frame, uint16_t selector)
{
    r->flags &= ~0x400;
    uint16_t control = c_timer_tick(segment_bytes(r->ds), segment_bytes(r->es), r->flags);
    r->flags = (r->flags & ARITH_FLAGS) | control;
    legacy_tail(r, (uint16_t)&target_timer_original_return);
}

struct MouseSetupEnd __far c_mouse_initialize(uint16_t button_state, uint16_t initial_flags)
{
    struct MouseSetupEnd end = {0};
    segment_bytes(0x40)[0x49] = 4;
    struct PlatformReply reply =
        platform_mouse(0, button_state, 0, 0, 0, 0x40, status(initial_flags, 0, 16));
    reply.flags = status(reply.flags, reply.value, 16);
    if (!reply.value) {
        end.reply = reply;
        return end;
    }
    mouse_installed = 128;
    end.present = 1;
    reply = platform_mouse(8, reply.handle, 8, 199, 0, reply.extra_segment, reply.flags);
    reply = platform_mouse(7, reply.handle, 0, 319, 0, reply.extra_segment, reply.flags);
    reply = platform_mouse(4, reply.handle, 160, 95, 0, reply.extra_segment, reply.flags);
    end.reply = platform_mouse(
        12, reply.handle, 31, (uint16_t)mouse_event_callback, 0, game_code_segment(), reply.flags);
    return end;
}

struct MouseSetupEnd __far c_mouse_remove(uint16_t button_state,
    uint16_t callback_offset,
    uint16_t callback_segment,
    uint16_t initial_flags)
{
    struct MouseSetupEnd end = {0};
    end.reply.flags = status(initial_flags, mouse_installed, 8);
    if (!mouse_installed)
        return end;
    end.present = 1;
    end.reply = platform_mouse(
        12, button_state, 0, callback_offset, 0, callback_segment, status(end.reply.flags, 0, 16));
    return end;
}

uint16_t __far c_mouse_callback_body(uint16_t events, uint16_t buttons, uint16_t x, uint16_t y)
{
    if (events & 1)
        mouse_moved = 128;
    buttons = (buttons & 0xff00) | (buttons & 3);
    input_buttons = (uint8_t)buttons;
    mouse_x = x;
    mouse_y = 199 - y;
    return buttons;
}

struct EgaCaptureEnd __far c_capture_ega_frame(
    SegmentBytes filename, uint8_t request_low, uint16_t flags)
{
    struct EgaCaptureEnd end = {0};
    SegmentBytes pixels = segment_bytes(0xa000);
    uint16_t destination = 0;
    end.flags = subtract16(flags, graphics_adapter, 4);
    if (graphics_adapter != 4)
        return end;
    struct DosOpenEnd file = c_dos_create(filename, request_low, status(flags, 0, 16));
    end.stage = 1;
    end.value = file.handle;
    end.flags = file.flags;
    if (file.flags & CF)
        return end;
    end.handle = file.handle;
    struct DosMemoryEnd memory = dos_allocate(0x7d0, file.handle, file.flags);
    end.stage = 2;
    end.value = memory.segment;
    end.flags = memory.flags;
    if (!memory.segment)
        return end;
    SegmentBytes output = segment_bytes(memory.segment);
    uint16_t source = segment_word(pixels, (uint16_t)&video_page_offset);
    port_out16(0x3ce, 0x0205);
    unsigned group;
    unsigned plane;
    for (group = 0; group < 4000; ++group) {
        for (plane = 0; plane < 4; ++plane) {
            port_out16(0x3ce, ((uint16_t)plane << 8) | 4);
            segment_word_store(output, destination, segment_word(pixels, source));
            destination += 2;
        }
        source += 2;
    }

    struct DosTransferEnd written =
        c_dos_write(file.handle, output, 32000, memory.segment, memory.flags);
    {
        struct DosOpenEnd closed = c_dos_close(file.handle, written.bytes, written.flags);
        struct DosOpenEnd freed = c_dos_free(memory.segment, closed.flags);
        end.flags = status(freed.flags, 0, 16);
    }

    /* Preserve the executable's final write to the zero port left by DOS I/O. */
    port_out16(0, 0x0105);
    end.data_segment = game_data_segment();
    end.stage = 3;
    return end;
}

/* Owned subsystem data. */
volatile uint8_t game_timer_suspend_flags = 0x0;
uint16_t saved_hardware_sp = 0x0;
uint8_t saved_int09_offset[4] = {0};
struct BiosTickPrescalerStorage bios_tick_prescaler_storage = {.value = 0x3};
uint16_t pit_calibration_max = 0x0;
struct KeyboardE0PrefixStorage keyboard_e0_prefix_storage = {0};
uint8_t cga_palette_phase = 0x0;
struct PixelScatterSequenceLowStorage pixel_scatter_sequence_low_storage = {
    .low = 0x3239, .high = 0x7584};
