/* DOS runtime services. The argv parser stages its result before a short
 * stack handoff, because its output becomes the caller's persistent stack. */

static uint8_t crt_argument_strings[512] __attribute__((used));
static uint8_t crt_argument_image[768] __attribute__((used));
static uint16_t crt_argument_offsets[66];
static volatile uint16_t crt_argument_destination __attribute__((used));
static volatile uint16_t crt_argument_bytes __attribute__((used));
extern void crt_argument_handoff(void);
__asm__(".intel_syntax noprefix\n.code16\n.section .text.crt_argument_handoff,\"ax\"\n"
        "crt_argument_handoff:\nmov si,OFFSET FLAT:crt_argument_image\n"
        "mov di,[crt_argument_destination]\nmov cx,[crt_argument_bytes]\n"
        "cld\nrep movsb\nmov sp,[crt_argument_destination]\n"
        "jmp word ptr [crt_argv_saved_return]\n.att_syntax prefix\n");

static void crt_fail(uint16_t code)
{
    __asm__ volatile("call abi_c_crt_fatal" : : "a"(code) : "memory", "cc");
    __builtin_unreachable();
}

struct NearReserveEnd __far c_crt_reserve_near(
    uint16_t data_segment, uint16_t amount, uint16_t error_code, uint16_t flags)
{
    struct NearReserveEnd reserve = {0};
    uint16_t old = crt_near_break;
    uint16_t end = old + amount;
    if (end < old)
        crt_fail(error_code);
    if (end > crt_near_limit) {
        uint16_t rounded = end + 15;
        reserve.paragraphs = data_segment - crt_psp_segment + (uint16_t)(((uint32_t)end + 15) >> 4);
        struct PlatformReply reply = platform_dos(0x4a00,
            reserve.paragraphs,
            data_segment - crt_psp_segment,
            0,
            data_segment,
            crt_psp_segment,
            flags);
        if (reply.flags & CF)
            crt_fail(error_code);
        flags = reply.flags;
        crt_near_limit = (rounded & 0xfff0) - 1;
        reserve.resized = 1;
    }
    reserve.offset = old;
    reserve.flags = add16(flags, old, amount);
    crt_near_break = end;
    return reserve;
}

struct EnvironmentEnd __far c_crt_prepare_environment(SegmentBytes data, uint16_t flags)
{
    SegmentBytes psp = segment_bytes(crt_psp_segment);
    uint16_t segment = segment_word(psp, 0x2c);
    uint16_t cursor = 0;
    uint16_t count = 0;
    uint16_t bytes = 0;
    SegmentBytes source = segment_bytes(segment);
    if (segment) {
        do {
            while (source[cursor++]) {
            }
            ++count;
        } while (source[cursor++]);
        /* SCASB consumed the first character of each following string. */
        bytes = cursor;
    }
    uint16_t vector = 2 * (count + 1);
    struct EnvironmentEnd end;
    end.amount = vector + ((bytes + 1) & 0xfffe);
    end.reserve = c_crt_reserve_near((uint32_t)data >> 16, end.amount, 9, flags);
    uint16_t destination = end.reserve.offset;
    uint16_t strings = destination + vector;
    crt_envp = destination;
    cursor = 0;
    while (count--) {
        if (segment_word(source, cursor) != 0x433b) {
            segment_word_store(data, destination, strings);
            destination += 2;
        }
        uint8_t value;
        do {
            value = source[cursor++];
            data[strings++] = value;
        } while (value);
    }
    segment_word_store(data, destination, 0);
    end.source_end = cursor;
    end.string_end = strings;
    return end;
}

void __far c_crt_prepare_argv(SegmentBytes stack, uint16_t return_slot)
{
    SegmentBytes psp = segment_bytes(crt_psp_segment);
    uint16_t cursor = 0x81;
    uint16_t length = 0;
    uint16_t count = 1;
    SegmentBytes program;
    if ((uint8_t)crt_dos_version != 2) {
        uint16_t segment = segment_word(psp, 0x2c);
        uint16_t offset = 0;
        program = segment_bytes(segment);
        while (program[offset] || program[(uint16_t)(offset + 1)])
            ++offset;
        crt_program_name_pointer[0] = offset + 4;
        crt_program_name_pointer[1] = segment;
    }
    program = segment_bytes(crt_program_name_pointer[1]);
    uint16_t source = crt_program_name_pointer[0];
    crt_argument_offsets[0] = 0;
    do {
        crt_argument_strings[length++] = program[source++];
    } while (crt_argument_strings[length - 1]);
    for (;;) {
        int quoted = 0;
        while (psp[cursor] == ' ' || psp[cursor] == '\t')
            ++cursor;
        uint8_t value = psp[cursor];
        if (!value || value == '\r')
            break;
        crt_argument_offsets[count++] = length;
        for (;;) {
            unsigned slashes = 0;
            value = psp[cursor];
            if (!value || value == '\r' || (!quoted && (value == ' ' || value == '\t')))
                break;
            while (psp[cursor] == '\\') {
                ++cursor;
                ++slashes;
            }
            value = psp[cursor];
            if (value == '"') {
                unsigned n = slashes / 2;
                while (n--)
                    crt_argument_strings[length++] = '\\';
                if (slashes & 1)
                    crt_argument_strings[length++] = '"';
                else
                    quoted = !quoted;
                ++cursor;
            } else {
                while (slashes--)
                    crt_argument_strings[length++] = '\\';
                if (value && value != '\r' && (quoted || (value != ' ' && value != '\t'))) {
                    crt_argument_strings[length++] = value;
                    ++cursor;
                }
            }
        }
        crt_argument_strings[length++] = 0;
    }
    crt_argc = count;
    crt_argument_bytes = (2 * (count + 1) + length + 1) & 0xfffe;
    crt_argument_destination = return_slot + 2 - crt_argument_bytes;
    crt_argv = crt_argument_destination;
    unsigned i;
    for (i = 0; i < count; ++i) {
        uint16_t pointer = crt_argument_destination + 2 * (count + 1) + crt_argument_offsets[i];
        crt_argument_image[2 * i] = pointer;
        crt_argument_image[2 * i + 1] = pointer >> 8;
    }
    crt_argument_image[2 * count] = crt_argument_image[2 * count + 1] = 0;
    for (i = 0; i < length; ++i)
        crt_argument_image[2 * (count + 1) + i] = crt_argument_strings[i];
    if (crt_argument_bytes > 2 * (count + 1) + length)
        crt_argument_image[crt_argument_bytes - 1] = 0;
    crt_argv_saved_return = segment_word(stack, return_slot);
}

static uint16_t crt_data_segment(void)
{
    uint16_t segment;
    __asm__("movw %%ss,%0" : "=r"(segment));
    return segment;
}

void c_crt_near_callbacks(struct LegacyRegisters *r)
{
    while (r->si < r->di) {
        r->di -= 2;
        r->cx = segment_word(segment_bytes(r->ds), r->di);
        if (r->cx)
            legacy_invoke(r, r->cx);
    }
    r->flags = subtract16(r->flags, r->si, r->di);
}

void c_crt_far_callbacks(struct LegacyRegisters *r)
{
    while (r->si < r->di) {
        r->di -= 4;
        uint32_t callback = (uint32_t)segment_word(segment_bytes(r->ds), r->di) |
            ((uint32_t)segment_word(segment_bytes(r->ds), r->di + 2) << 16);
        r->ax = (uint16_t)callback | (callback >> 16);
        if (r->ax)
            legacy_far_invoke(r, callback);
    }
    r->flags = subtract16(r->flags, r->si, r->di);
}

void c_crt_initialize_io(struct LegacyRegisters *r)
{
    uint16_t data_segment = crt_data_segment();
    uint16_t offset = 0;
    r->ax = (r->ax & 255) | 0x3000;
    native_invoke(r, (uint16_t)platform_int21);
    crt_dos_version = r->ax;
    r->ax = 0x3500;
    native_invoke(r, (uint16_t)platform_int21);
    crt_saved_int00_offset = r->bx;
    crt_saved_int00_segment = r->es;
    r->ds = game_code_segment();
    r->ax = 0x2500;
    r->dx = (uint16_t)crt_divide_error;
    native_invoke(r, (uint16_t)platform_int21);
    r->ds = data_segment;
    if (crt_optional_service_hook >> 16) {
        r->es = crt_psp_segment;
        r->si = segment_word(segment_bytes(r->es), 0x2c);
        r->ax = crt_service_init_argument;
        r->ds = crt_service_init_argument >> 16;
        r->dx = r->ds;
        r->bx = 0;
        legacy_far_invoke(r, crt_optional_service_hook);
        r->ds = data_segment;
        if (r->flags & CF) {
            r->ax = 2;
            legacy_tail(r, (uint16_t)&target_c_crt_fatal);
            return;
        }
        r->ax = crt_service_secondary_argument;
        r->ds = crt_service_secondary_argument >> 16;
        r->dx = r->ds;
        r->bx = 3;
        legacy_far_invoke(r, crt_optional_service_hook);
        r->ds = data_segment;
    }
    r->es = crt_psp_segment;
    uint16_t segment = segment_word(segment_bytes(r->es), 0x2c);
    if (segment) {
        SegmentBytes environment = segment_bytes(segment);
        r->es = segment;
        while (environment[offset]) {
            unsigned n;
            for (n = 0; n < 12 && environment[(uint16_t)(offset + n)] == crt_file_info_name[n];
                ++n) {
            }
            if (n == 12) {
                uint16_t count = (int16_t)(int8_t)environment[(uint16_t)(offset + 12)];
                offset += 13;
                n = 0;
                do {
                    uint8_t flag = environment[offset++];
                    crt_file_flags[n++] = flag == 255 ? 0 : flag;
                } while (--count);
                break;
            }
            while (environment[offset++]) {
            }
        }
    }
    int handle;
    for (handle = 4; handle >= 0; --handle) {
        crt_file_flags[handle] &= ~64;
        r->bx = handle;
        r->ax = 0x4400;
        native_invoke(r, (uint16_t)platform_int21);
        if (!(r->flags & CF) && (r->dx & 128))
            crt_file_flags[handle] |= 64;
    }
    r->bx = 0xffff;
    r->si = r->di = (uint16_t)crt_none;
    c_crt_far_callbacks(r);
    r->si = r->di = (uint16_t)crt_none;
    c_crt_near_callbacks(r);
}

void c_crt_restore_hooks(struct LegacyRegisters *r)
{
    uint16_t data_segment = r->ds;
    r->cx = crt_optional_service_hook >> 16;
    if (r->cx) {
        r->bx = 2;
        legacy_far_invoke(r, crt_optional_service_hook);
    }
    r->ds = crt_saved_int00_segment;
    r->dx = crt_saved_int00_offset;
    r->ax = 0x2500;
    native_invoke(r, (uint16_t)platform_int21);
    r->ds = data_segment;
    r->flags = subtract8(r->flags, crt_optional_interrupt_enabled, 0);
    if (crt_optional_interrupt_enabled) {
        r->ax = 0x2500 | crt_optional_interrupt_number;
        r->ds = crt_optional_interrupt_vector >> 16;
        r->dx = crt_optional_interrupt_vector;
        native_invoke(r, (uint16_t)platform_int21);
        r->ds = data_segment;
    }
}

static uint16_t crt_message(uint16_t code)
{
    SegmentBytes data = segment_bytes(crt_data_segment());
    uint16_t base = (uint16_t)crt_error_registry;
    uint16_t offset = 8;
    while (offset + 2 <= sizeof(crt_error_registry)) {
        uint16_t tag = segment_word(data, base + offset);
        offset += 2;
        if (tag == code)
            return base + offset;
        if (tag == 0xffff)
            return 0;
        while (offset < sizeof(crt_error_registry) && data[base + offset++]) {
        }
    }
    return 0;
}

struct CrtMessageEnd __far crt_write_message(SegmentBytes data, uint16_t code, uint16_t flags)
{
    struct CrtMessageEnd end = {0};
    uint16_t message = crt_message(code);
    uint16_t length = 0;
    end.message = message;
    end.value = message;
    end.flags = status(flags, message, 16);
    if (message) {
        while (data[message + length])
            ++length;
        struct PlatformReply reply = platform_dos(
            0x4000, 2, length, message, (uint32_t)data >> 16, (uint32_t)data >> 16, end.flags);
        end.value = reply.value;
        end.flags = reply.flags;
        end.length = reply.count;
        end.written = 1;
    }
    return end;
}

/* Only the message service's documented partial outputs cross the CRT ABI. */
static void publish_crt_message(
    struct LegacyRegisters *r, const struct CrtMessageEnd *message, uint16_t code)
{
    r->ax = message->value;
    r->dx = message->written ? message->message : code;
    r->es = r->ds;
    r->flags = message->flags;
    if (message->written) {
        r->bx = 2;
        r->cx = message->length;
    }
}

void c_crt_error_prefix(struct LegacyRegisters *r, uint32_t frame, uint16_t selector)
{
    struct CrtMessageEnd prefix = crt_write_message(segment_bytes(r->ds), 0xfc, r->flags);
    publish_crt_message(r, &prefix, 0xfc);
    if (crt_error_callback)
        legacy_invoke(r, crt_error_callback);
    /* The callback can replace the output segment and flags for the suffix. */
    struct CrtMessageEnd suffix = crt_write_message(segment_bytes(r->ds), 0xff, r->flags);
    publish_crt_message(r, &suffix, 0xff);
}

void c_crt_check_guard(struct LegacyRegisters *r)
{
    uint8_t checksum = 0;
    unsigned i;
    for (i = 0; i < sizeof(crt_null_guard); ++i)
        checksum ^= segment_bytes(r->ds)[(uint16_t)&crt_null_guard + i];
    r->cx = 0;
    r->ax = ((uint16_t)(checksum ^ 0x55) << 8) |
        segment_record(segment_bytes(r->ds), crt_null_guard).trailer[1];
    r->flags = status(r->flags, checksum ^ 0x55, 8) & ~0x400;
    if (checksum != 0x55) {
        c_crt_error_prefix(r, 0, 0);
        struct CrtMessageEnd message = crt_write_message(segment_bytes(r->ds), 1, r->flags);
        publish_crt_message(r, &message, 1);
        r->ax = 1;
    }
}

struct StackReturnEnd __far crt_discard_argument(SegmentBytes stack, uint16_t return_slot)
{
    struct StackReturnEnd end = {return_slot + 4, segment_word(stack, return_slot)};
    return end;
}

uint16_t __far c_crt_lookup_message(uint16_t code)
{
    return crt_message(code);
}

struct CrtMessageEnd __far c_crt_write_message(SegmentBytes data, uint16_t code, uint16_t flags)
{
    return crt_write_message(data, code, flags);
}

/* The common shutdown phase takes an ordinary C status, already snapshotted at
 * the caller's required point. The carrier stays live through native callbacks. */
static void crt_finish_exit(struct LegacyRegisters *r, uint16_t exit_status)
{
    r->si = r->di = (uint16_t)crt_none;
    c_crt_near_callbacks(r);
    r->si = r->di = (uint16_t)crt_none;
    c_crt_far_callbacks(r);
    c_crt_check_guard(r);
    if (r->ax && !exit_status)
        exit_status = 255;
    unsigned handle;
    for (handle = 5; handle < 20; ++handle)
        if (crt_file_flags[handle] & 1) {
            r->bx = handle;
            r->ax = (r->ax & 255) | 0x3e00;
            native_invoke(r, (uint16_t)platform_int21);
        }
    c_crt_restore_hooks(r);
    r->ax = 0x4c00 | (exit_status & 255);
    native_invoke(r, (uint16_t)platform_int21);
    __builtin_unreachable();
}

static void crt_exit_callbacks(struct LegacyRegisters *r)
{
    r->si = r->di = ((uint16_t)crt_error_registry + sizeof(crt_error_registry) - 4);
    c_crt_near_callbacks(r);
    r->si = r->di = (uint16_t)crt_none;
    c_crt_near_callbacks(r);
}

void c_crt_exit_common(struct LegacyRegisters *r, uint32_t frame, uint16_t selector)
{
    uint16_t exit_status = original_argument(frame, 0);
    crt_finish_exit(r, exit_status);
}

void c_crt_exit(struct LegacyRegisters *r, uint32_t frame, uint16_t selector)
{
    crt_exit_callbacks(r);
    /* Legacy callers can replace their argument during either initial pass. */
    uint16_t exit_status = original_argument(frame, 0);
    crt_finish_exit(r, exit_status);
}

void c_crt_fatal(struct LegacyRegisters *r, uint32_t frame, uint16_t selector)
{
    uint16_t code = r->ax;
    typedef void (*ExitCallback)(uint16_t status);
    ExitCallback callback = (ExitCallback)crt_exit_callback;
    c_crt_error_prefix(r, 0, 0);
    struct CrtMessageEnd message = crt_write_message(segment_bytes(r->ds), code, r->flags);
    publish_crt_message(r, &message, code);
    /* This nonreturning original C ABI callback takes one stack argument. */
    callback(255);
    __builtin_unreachable();
}

void __far c_crt_divide_error(void)
{
    crt_exit_callback = (uint16_t)abi_c_crt_exit;
    crt_fail(3);
}

void __far c_crt_allocation_error(void)
{
    crt_fail(2);
}

void c_crt_initialize(struct LegacyRegisters *r, uint32_t frame, uint16_t selector)
{
    SegmentBytes data = segment_bytes(crt_data_segment());
    SegmentBytes psp = segment_bytes(r->ds);
    uint16_t stack = ((uint16_t)frame + 20) & 0xfffe;
    uint16_t psp_segment = r->ds;
    crt_near_break = crt_initial_break = stack;
    crt_near_limit = (uint16_t)(r->si << 4) - 1;
    r->si += r->di;
    segment_word_store(psp, 2, r->si);
    r->bx = r->si - r->es;
    r->ax = 0x4a00;
    native_invoke(r, (uint16_t)platform_int21);
    crt_psp_segment = psp_segment;
    fill_segment_bytes(
        data + ((uint16_t)crt_error_registry + sizeof(crt_error_registry) - 4), 4, 0, 0);
    reset_graphics_session(data);
    r->ds = r->es = crt_data_segment();
    r->flags &= ~0x400;
    r->ax = 0;
    r->cx = 0;
    r->di = (uint16_t)&graphics_session + sizeof(graphics_session);
    c_crt_initialize_io(r);
    r->ds = crt_data_segment();
    typed_legacy_entry(r, 0, TYPED_c_crt_prepare_environment);
    /* argv permanently lowers SP. Keep its CALL at the original stack depth. */
    legacy_tail(r, (uint16_t)&target_argv_call);
}

void c_crt_run_game(struct LegacyRegisters *r, uint32_t frame, uint16_t selector)
{
    r->bp = 0;
    struct BootstrapEnd game =
        c_game_bootstrap(segment_bytes(r->ds), segment_bytes(r->es), r->flags);
    r->ax = game.status;
    r->flags = game.flags;
    crt_exit_callbacks(r);
    crt_finish_exit(r, game.status);
    __builtin_unreachable();
}

/* Owned subsystem data. */
/* Packed tag/string records, ending in FFFFh. The leading marker borrows the tag-zero NUL. */
uint8_t crt_error_registry[221] = "<<NMSG>>"
                                  "\x00\x00R6000\r\n- stack overflow\r\n\x00"
                                  "\x03\x00R6003\r\n- integer divide by 0\r\n\x00"
                                  "\x09\x00R6009\r\n- not enough space for environment\r\n\x00"
                                  "\xfc\x00\r\n\x00"
                                  "\xff\x00run-time error \x00"
                                  "\x02\x00R6002\r\n- floating point not loaded\r\n\x00"
                                  "\x01\x00R6001\r\n- null pointer assignment\r\n\x00"
                                  "\xff\xff\xff\x00\x00\x00\x00";
uint16_t crt_near_limit = 0x0;
uint16_t crt_initial_break = 0x0;
uint16_t crt_exit_callback = (uint16_t)abi_c_crt_exit_common;
uint16_t crt_near_break = 0x0;
uint16_t crt_loader_data_segment_reference[39] = {(uint16_t)&__game_data_paragraph};
uint16_t crt_near_break_address = (uint16_t)&crt_near_break;
uint8_t crt_file_info_name[13] = ";C_FILE_INFO\x00";
struct CrtSavedInt00OffsetStorage crt_saved_int00_offset_storage = {0};
uint16_t crt_psp_segment = 0x0;
struct CrtDosVersionStorage crt_dos_version_storage = {0};
uint8_t crt_handle_flag_capacity[2] = {
    0x14,
    0x00,
};
uint8_t crt_file_flags[20] = {
    0x81,
    0x81,
    0x81,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};
uint16_t crt_argc = 0x0;
uint16_t crt_argv = 0x0;
uint16_t crt_envp = 0x0;
uint16_t crt_program_name_pointer[2] = {
    (uint16_t)crt_program_name_fallback, (uint16_t)&__game_data_paragraph};
uint8_t crt_program_name_fallback[4] = "C";
uint8_t crt_optional_interrupt_enabled = 0x0;
uint8_t crt_optional_interrupt_number = 0x0;
uint32_t crt_optional_interrupt_vector = 0x0;
uint16_t crt_error_callback = 0x0;
struct CrtArgvSavedReturnStorage crt_argv_saved_return_storage = {0};
uint32_t crt_optional_service_hook = 0x0;
uint32_t crt_service_init_argument = 0x0;
uint32_t crt_service_secondary_argument = 0x0;
uint8_t crt_none[8] = "(none)";

/* The DOS null-assignment guard alone occupies DS:0000..0041. */
struct CrtNullGuard crt_null_guard __attribute__((used, section(".crt.guard"), aligned(1))) = {{0},
    {
        0x4d,
        0x53,
        0x20,
        0x52,
        0x75,
        0x6e,
        0x2d,
        0x54,
        0x69,
        0x6d,
        0x65,
        0x20,
        0x4c,
        0x69,
        0x62,
        0x72,
        0x61,
        0x72,
        0x79,
        0x20,
        0x2d,
        0x20,
        0x43,
        0x6f,
        0x70,
        0x79,
        0x72,
        0x69,
        0x67,
        0x68,
        0x74,
        0x20,
        0x28,
        0x63,
        0x29,
        0x20,
        0x31,
        0x39,
        0x38,
        0x37,
        0x2c,
        0x20,
        0x4d,
        0x69,
        0x63,
        0x72,
        0x6f,
        0x73,
        0x6f,
        0x66,
        0x74,
        0x20,
        0x43,
        0x6f,
        0x72,
        0x70,
    },
    {
        0x1e,
        0x00,
    }};
