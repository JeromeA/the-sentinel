/* The only register constraints here describe the documented interrupt ABI.
 * Game code calls the service wrappers with filenames, handles and buffers. */
#define PLATFORM_INTERRUPT(name, number)                                                           \
    struct PlatformReply __far name(uint16_t function,                                             \
        uint16_t handle,                                                                           \
        uint16_t count,                                                                            \
        uint16_t offset,                                                                           \
        uint16_t data_segment,                                                                     \
        uint16_t extra_segment,                                                                    \
        uint16_t initial_flags)                                                                    \
    {                                                                                              \
        struct PlatformReply reply;                                                                \
        uint16_t flags;                                                                            \
        uint16_t returned_segment;                                                                 \
        __asm__ volatile(                                                                          \
            "pushw %%ds\n\tpushw %%es\n\tpushw %%bp\n\t"                                           \
            "pushw %[initial]\n\tpopf\n\tmovw %[data],%%si\n\tmovw %%si,%%ds\n\t"                  \
            "movw %[extra],%%si\n\tmovw %%si,%%es\n\tint $" number "\n\t"                          \
            "popw %%bp\n\tpushf\n\tpopw %%si\n\tmovw %%es,%%di\n\tpopw %%es\n\tpopw %%ds\n\tcld"   \
            : "+a"(function),                                                                      \
            "+b"(handle),                                                                          \
            "+c"(count),                                                                           \
            "+d"(offset),                                                                          \
            "=S"(flags),                                                                           \
            "=D"(returned_segment)                                                                 \
            : [data] "m"(data_segment), [extra] "m"(extra_segment), [initial] "m"(initial_flags)   \
            : "memory", "cc");                                                                     \
        reply.value = function;                                                                    \
        reply.handle = handle;                                                                     \
        reply.count = count;                                                                       \
        reply.offset = offset;                                                                     \
        reply.extra_segment = returned_segment;                                                    \
        reply.flags = flags;                                                                       \
        return reply;                                                                              \
    }
PLATFORM_INTERRUPT(platform_video, "0x10")
PLATFORM_INTERRUPT(platform_dos, "0x21")
PLATFORM_INTERRUPT(platform_keyboard, "0x16")
PLATFORM_INTERRUPT(platform_mouse, "0x33")
PLATFORM_INTERRUPT(platform_system_bios, "0x15")
#undef PLATFORM_INTERRUPT

/* DOS/BIOS services exposed as C wrappers. Only legacy_invoke's small interrupt
 * trampolines use assembly; argument handling and error paths are C. */

struct DosMemoryEnd __far dos_allocate(
    uint16_t paragraphs, uint8_t previous_low, uint16_t initial_flags)
{
    struct PlatformReply reply =
        platform_dos(0x4800 | previous_low, paragraphs, 0, 0, 0, 0, initial_flags);
    struct DosMemoryEnd end = {reply.value, reply.handle, reply.flags};
    if (end.flags & CF) {
        end.segment = 0;
        end.flags = status(end.flags, 0, 16);
    }
    return end;
}

struct VideoModeSetEnd __far bios_mode(uint16_t mode, uint16_t initial_flags)
{
    struct PlatformReply reply = platform_video(mode, 0, 0, 0, 0, 0, initial_flags);
    video_crtc_port = segment_word(segment_bytes(0x40), 0x63);
    struct VideoModeSetEnd end;
    end.crtc_port = video_crtc_port;
    end.flags = reply.flags;
    return end;
}

struct VideoModeSetEnd __far c_bios_mode(uint16_t mode, uint16_t initial_flags)
{
    return bios_mode(mode, initial_flags);
}

struct VideoModeEnd __far c_bios_get_mode(uint16_t initial_flags)
{
    struct PlatformReply reply = platform_video(0x0f00, 0, 0, 0, 0, 0, initial_flags);
    struct VideoModeEnd end = {reply.value & 255, reply.handle >> 8, status(reply.flags, 0, 8)};
    return end;
}

struct DosOpenEnd __far c_dos_open(SegmentBytes filename, uint8_t mode, uint16_t initial_flags)
{
    struct PlatformReply reply = platform_dos(0x3d00 | mode,
        0,
        0,
        (uint16_t)(uint32_t)filename,
        (uint32_t)filename >> 16,
        0,
        initial_flags);
    struct DosOpenEnd end = {reply.value, reply.flags};
    if (end.flags & CF) {
        end.handle = 0;
        end.flags = status(end.flags, 0, 16);
    }
    return end;
}

struct DosOpenEnd __far c_dos_create(
    SegmentBytes filename, uint8_t request_low, uint16_t initial_flags)
{
    struct PlatformReply reply = platform_dos(0x3c00 | request_low,
        0,
        0,
        (uint16_t)(uint32_t)filename,
        (uint32_t)filename >> 16,
        0,
        initial_flags);
    struct DosOpenEnd end = {reply.value, reply.flags};
    return end;
}

struct DosTransferEnd __far c_dos_read(
    uint16_t handle, SegmentBytes buffer, uint16_t length, uint16_t initial_flags)
{
    /* Refills inherit this precise interrupt contract, rather than the generic
     * wrapper's compiler-selected SI/DI/BP inputs. */
    struct LegacyRegisters io = {0};
    io.ds = (uint32_t)buffer >> 16;
    io.es = io.ds;
    io.ax = 0x3f00;
    io.bx = handle;
    io.cx = length;
    io.dx = (uint16_t)(uint32_t)buffer;
    io.flags = initial_flags;
    native_invoke(&io, (uint16_t)platform_int21);
    struct DosTransferEnd end = {io.ax, io.flags};
    return end;
}

struct DosTransferEnd __far c_dos_write(uint16_t handle,
    SegmentBytes buffer,
    uint16_t length,
    uint8_t previous_low,
    uint16_t initial_flags)
{
    struct PlatformReply reply = platform_dos(0x4000 | previous_low,
        handle,
        length,
        (uint16_t)(uint32_t)buffer,
        (uint32_t)buffer >> 16,
        0,
        initial_flags);
    struct DosTransferEnd end = {reply.value, reply.flags};
    return end;
}

struct DosOpenEnd __far c_dos_close(uint16_t handle, uint8_t previous_low, uint16_t initial_flags)
{
    struct PlatformReply reply =
        platform_dos(0x3e00 | previous_low, handle, 0, 0, 0, 0, initial_flags);
    struct DosOpenEnd end = {reply.value, reply.flags};
    return end;
}

struct DosSizeEnd __far c_dos_size(uint16_t handle, uint16_t initial_flags)
{
    struct PlatformReply end =
        platform_dos(0x4202, handle, 0, 0, 0, 0, status(initial_flags, 0, 16));
    struct PlatformReply reset =
        platform_dos(0x4200, handle, end.count, 0, 0, 0, status(end.flags, 0, 16));
    struct DosSizeEnd size = {end.value, end.offset, reset.flags};
    return size;
}

struct DosMemoryEnd __far c_dos_alloc(
    uint16_t paragraphs, uint8_t previous_low, uint16_t initial_flags)
{
    return dos_allocate(paragraphs, previous_low, initial_flags);
}

struct DosOpenEnd __far c_dos_free(uint16_t segment, uint16_t initial_flags)
{
    struct PlatformReply reply =
        platform_dos(0x4900 | (segment & 255), 0, 0, 0, 0, segment, initial_flags);
    struct DosOpenEnd end = {reply.value, reply.flags};
    return end;
}

struct DosOpenEnd __far c_bios_hide_cursor(uint8_t previous_low, uint16_t initial_flags)
{
    struct PlatformReply reply =
        platform_video(0x100 | previous_low, 0, 0x2000, 0, 0, 0, initial_flags);
    struct DosOpenEnd end = {reply.value, reply.flags};
    return end;
}

struct DosOpenEnd __far console_position(uint16_t row, uint16_t column, uint16_t initial_flags)
{
    text_cursor_column = column;
    text_cursor_row = row;

    struct PlatformReply reply = platform_video(0x200 | (row & 255),
        0,
        0,
        ((row & 255) << 8) | (column & 255),
        0,
        0,
        status(initial_flags, 0, 16));
    {
        struct DosOpenEnd end = {reply.value, reply.flags};
        return end;
    }
}

static uint8_t apply_console_attribute(uint16_t foreground, uint16_t background)
{
    uint8_t shifted = (uint8_t)(background << 4);
    text_attribute = (foreground & 15) | shifted;
    return shifted;
}

void __far set_console_attribute(uint16_t foreground, uint16_t background)
{
    apply_console_attribute(foreground, background);
}

static uint16_t console_attributes(uint16_t foreground, uint16_t background)
{
    uint8_t shifted = apply_console_attribute(foreground, background);
    return (background & 0xff00) | shifted;
}

/* Only firmware control bits and the next cursor-request AL continue through
 * native text. Arithmetic status is established at each real BIOS boundary. */
uint16_t __far write_console_text(
    SegmentBytes text, uint16_t initial_word, uint16_t control, struct ConsoleTextEnd *observation)
{
    if (observation)
        *observation = (struct ConsoleTextEnd){0};
    uint8_t cursor_low = initial_word;
    control &= ~ARITH_FLAGS;
    for (;;) {
        struct PlatformReply position = platform_video(0x200 | cursor_low,
            0,
            0,
            ((uint16_t)text_cursor_row << 8) | text_cursor_column,
            0,
            0,
            status(control, 0, 16));
        control = position.flags & ~ARITH_FLAGS;
        uint8_t character = *text;
        text += control & 0x400 ? -1 : 1;
        if (!character) {
            if (observation) {
                observation->value = position.value & 0xff00;
                observation->flags = status(position.flags, character, 8);
            }
            return control;
        }
        struct PlatformReply glyph = platform_video(
            0x900 | character, text_attribute, 1, position.offset, 0, 0, status(control, 0, 16));
        cursor_low = glyph.value;
        control = glyph.flags & ~ARITH_FLAGS;
        if (observation) {
            observation->count = glyph.count;
            observation->written = 1;
        }
        ++text_cursor_column;
    }
}

uint16_t __far write_console_line(SegmentBytes text,
    uint16_t row,
    uint16_t column,
    uint16_t control,
    struct ConsoleTextEnd *observation)
{
    struct DosOpenEnd position = console_position(row, column, control);
    return write_console_text(text, position.handle, position.flags & ~ARITH_FLAGS, observation);
}

struct ConsoleTextEnd __far console_text(
    SegmentBytes text, uint16_t initial_word, uint16_t initial_flags)
{
    struct ConsoleTextEnd end;
    write_console_text(text, initial_word, initial_flags, &end);
    return end;
}

struct DosOpenEnd __far c_bios_cursor(uint16_t row, uint16_t column, uint16_t initial_flags)
{
    return console_position(row, column, initial_flags);
}

uint16_t __far c_text_attribute(uint16_t foreground, uint16_t background)
{
    return console_attributes(foreground, background);
}

struct ConsoleTextEnd __far c_bios_text(
    SegmentBytes text, uint16_t initial_word, uint16_t initial_flags)
{
    return console_text(text, initial_word, initial_flags);
}

/* String and memory interfaces preserve their ES/DS and direction contracts. */
static __attribute__((regparmcall)) struct TextLengthEnd scan_zero(SegmentBytes text, int backwards)
{
    struct TextLengthEnd scan = {0, 0xffff, 0};
    do {
        uint8_t character = *text;
        text += backwards ? -1 : 1;
        --scan.remaining;
        if (!character)
            break;
        ++scan.length;
    } while (scan.remaining);
    uint16_t consumed = 0xffff - scan.remaining;
    /* SCASB's legacy end-1 observation differs from length in reverse scans
     * and when all 65,535 samples are nonzero. Keep that compatibility explicit. */
    scan.legacy_extent = backwards ? -consumed - 1 : consumed - 1;
    return scan;
}

struct TextLengthEnd __far c_string_length(SegmentBytes text, int backwards)
{
    return scan_zero(text, backwards);
}

struct TextCompareEnd __far c_string_compare(SegmentBytes left, SegmentBytes right, int backwards)
{
    struct TextCompareEnd end;
    do {
        end.left = *left;
        end.right = *right;
        left += backwards ? -1 : 1;
        if (!end.left || end.left != end.right)
            break;
        ++right;
    } while (1);
    return end;
}

void __far c_memory_fill(SegmentBytes destination, uint8_t value, uint16_t count, int backwards)
{
    fill_segment_bytes(destination, count, value, backwards);
}

void __far c_memory_copy(
    SegmentBytes destination, SegmentBytes source, uint16_t count, int backwards)
{
    while (count--) {
        *destination = *source;
        source += backwards ? -1 : 1;
        destination += backwards ? -1 : 1;
    }
}

struct DosPrintEnd __far c_dos_print(SegmentBytes scan_start,
    SegmentBytes terminator_start,
    SegmentBytes display_start,
    int backwards,
    uint16_t initial_flags)
{
    struct TextLengthEnd scan = scan_zero(scan_start, backwards);
    *(terminator_start + scan.legacy_extent) = '$';
    /* The old display offset ignores the writable buffer's base offset. Its
     * actual address is supplied separately from the terminator store. */
    uint16_t end = (uint16_t)(uint32_t)display_start + scan.legacy_extent;
    struct PlatformReply reply = platform_dos(0x900,
        0,
        scan.remaining,
        (uint16_t)(uint32_t)display_start,
        (uint32_t)display_start >> 16,
        0,
        subtract16(status(initial_flags, 0, 8), end + 1, 1) & ~CF);
    struct DosPrintEnd result = {reply.value, reply.count, reply.flags};
    return result;
}
