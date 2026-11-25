/* Smooth view movement updates the CRTC origin and copies the newly exposed
 * strip during retrace. CGA wraps its two banks independently; EGA uses latches. */

static uint16_t video_status_port(void)
{
    return (video_crtc_port & 0xff00) | (uint8_t)(video_crtc_port + 6);
}

static uint8_t wait_video_bit(uint8_t mask, unsigned set)
{
    uint8_t status;
    do {
        status = port_in8(video_status_port());
    } while (!!(status & mask) != set);
    return status;
}

void __far wait_for_video_retrace(struct VideoRetraceEnd *observation)
{
    wait_video_bit(8, 0);
    wait_video_bit(8, 1);
    uint8_t sample = wait_video_bit(1, 0);
    if (observation) {
        observation->status = sample;
        observation->port = video_status_port();
    }
}

struct VideoRetraceEnd __far c_wait_video_retrace(void)
{
    struct VideoRetraceEnd end;
    wait_for_video_retrace(&end);
    return end;
}

static struct GameStripCopy transition_status_copy(SegmentBytes source,
    SegmentBytes destination,
    uint16_t src,
    uint16_t dst,
    unsigned halves,
    int backwards)
{
    struct GameStripCopy result;
    unsigned i;
    for (i = 0; i < halves; ++i) {
        result = game_copy_banked_strip(source, destination, src, dst, 2, backwards);
        src = result.source_end;
        dst = result.destination_end;
        if (i + 1 < halves)
            src &= 0x1fff;
    }
    return result;
}

static void display_start(uint16_t value)
{
    __asm__ volatile("cli" ::: "memory");
    port_out8(video_crtc_port, 12);
    port_out8(video_crtc_port + 1, value >> 8);
    port_out8(video_crtc_port, 13);
    port_out8(video_crtc_port + 1, (uint8_t)value);
    __asm__ volatile("sti" ::: "memory");
}

uint8_t __far transition_view(
    SegmentBytes data, int backwards, unsigned mode, struct ViewTransitionEnd *end)
{
    uint8_t interrupts_enabled = 0;
    SegmentBytes target = segment_bytes(mode == 0 ? 0xb800 : 0xa000);
    SegmentBytes source = segment_bytes(mode == 1 ? 0xa000 : video_work_buffer_segment);
    wait_for_video_retrace(0);
    if (end)
        end->bank_base = mode == 0 ? 0x1fff : mode == 1 ? 0xa000 : video_work_buffer_segment;
    if (mode == 1)
        port_out16(0x3ce, 0x105);
    for (;;) {
        uint8_t step = video_transition_steps_left - 1;
        uint8_t direction = camera_transition_mode;
        video_transition_steps_left = step;
        if (step & 128)
            break;
        if (direction >= INPUT_DIRECTION_COUNT ||
            step >= (direction < INPUT_DIRECTION_FIRST_VERTICAL ? 16 : 8))
            break;
        struct UnalignedWord *table;
        if (mode == 0)
            table = direction == INPUT_DIRECTION_RIGHT ? cga_transition_source_mode_0
                : direction == INPUT_DIRECTION_LEFT    ? cga_transition_source_mode_1
                : direction == INPUT_DIRECTION_UP      ? cga_transition_source_mode_2
                                                       : cga_transition_source_mode_3;
        else if (mode == 1)
            table = direction == INPUT_DIRECTION_RIGHT ? ega_transition_source_mode_0
                : direction == INPUT_DIRECTION_LEFT    ? ega_transition_source_mode_1
                : direction == INPUT_DIRECTION_UP      ? ega_transition_source_mode_2
                                                       : ega_transition_source_mode_3;
        else
            table = direction == INPUT_DIRECTION_RIGHT ? vga_transition_source_mode_0
                : direction == INPUT_DIRECTION_LEFT    ? vga_transition_source_mode_1
                : direction == INPUT_DIRECTION_UP      ? vga_transition_source_mode_2
                                                       : vga_transition_source_mode_3;
        uint16_t offset = table[step].value;
        uint16_t pending = video_page_offset +
            (mode == 0          ? cga_transition_page_deltas
                    : mode == 1 ? ega_transition_page_deltas
                                : vga_transition_page_deltas)[direction]
                .value;
        if (mode == 0)
            pending &= 0x1fff;
        video_pending_page_offset = pending;
        video_transition_wait_count = video_display_enable_wait_counts[direction].value;
        uint16_t crtc = mode == 1 ? pending : pending >> 1;
        if (mode == 2 && segment_record(data, graphics_session).detected_card != 5)
            crtc >>= 1;
        unsigned wait = video_transition_wait_count;
        do {
            wait_video_bit(1, 1);
            wait_video_bit(1, 0);
        } while (--wait);
        video_transition_busy_flags = 255;
        display_start(crtc);
        interrupts_enabled = 1;
        if (end)
            end->interrupts_enabled = 1;
        uint16_t src = mode == 1 ? video_previous_page_offset + 0x8000 : 0;
        uint16_t dst = pending;
        if (mode == 0) {
            if (direction == INPUT_DIRECTION_UP) {
                transition_status_copy(source, target, 0xa0, pending + 0xa0, 1, backwards);
                transition_status_copy(source, target, offset, pending + 0x140, 2, backwards);
            } else
                transition_status_copy(source, target, 0, pending, 2, backwards);
        } else if (mode == 1) {
            SegmentBytes input = source + src;
            SegmentBytes output = target + dst;
            copy_video_bytes(&output, &input, 0x140, backwards);
        } else if (direction == INPUT_DIRECTION_UP) {
            src = 0x400;
            dst = pending + 0x400;
            video_copy_words_retrace(target, &dst, source, &src, 0x300, (backwards ? 0x400 : 0));
            src = offset;
            dst = pending + 0xa00;
            video_copy_words_retrace(target, &dst, source, &src, 0x500, (backwards ? 0x400 : 0));
        } else
            video_copy_words_retrace(target, &dst, source, &src, 0x500, (backwards ? 0x400 : 0));
        __asm__ volatile("cli" ::: "memory");
        wait_video_bit(8, 1);
        video_page_offset = pending;
        if (direction < INPUT_DIRECTION_FIRST_VERTICAL) {
            src = offset + (mode == 1 ? video_previous_page_offset + 0x8000 : 0);
            dst = pending +
                (direction == INPUT_DIRECTION_RIGHT ? (mode == 0          ? 0x18e
                                                              : mode == 1 ? 0x167
                                                                          : 0xb38)
                                                    : (mode == 2 ? 0xa00 : 0x140));
            SegmentBytes input = source + src;
            SegmentBytes output = target + dst;
            unsigned row;
            for (row = 0; row < (mode == 0 ? 96 : 192); ++row) {
                if (mode == 0) {
                    SegmentBytes first_input =
                        source + (video_buffer_offset(input, source) & 0x1fff);
                    SegmentBytes first_output =
                        target + (video_buffer_offset(output, target) & 0x1fff);
                    copy_video_words(&first_output, &first_input, 1, backwards);
                    SegmentBytes second_input =
                        source + (video_buffer_offset(input, source) | 0x2000);
                    SegmentBytes second_output =
                        target + ((video_buffer_offset(output, target) & 0x1fff) | 0x2000);
                    copy_video_words(&second_output, &second_input, 1, backwards);
                    input = second_input + 0x4e;
                    output = second_output + 0x4e;
                } else {
                    if (mode == 1)
                        copy_video_bytes(&output, &input, 1, backwards);
                    else
                        copy_video_words(&output, &input, 4, backwards);
                    input += mode == 1 ? 39 : 312;
                    output += mode == 1 ? 39 : 312;
                }
            }
            src = video_buffer_offset(input, source);
            dst = video_buffer_offset(output, target);
        } else if (direction == INPUT_DIRECTION_UP) {
            if (mode == 0) {
                {
                    struct GameStripCopy copy =
                        transition_status_copy(source, target, 0, pending, 1, backwards);
                    src = copy.source_end;
                    dst = copy.destination_end;
                }
            } else if (mode == 1) {
                src = offset + video_previous_page_offset + 0x8000;
                dst = pending + 0x140;
                SegmentBytes input = source + src;
                SegmentBytes output = target + dst;
                copy_video_bytes(&output, &input, 0x140, backwards);
                src = video_buffer_offset(input, source);
                dst = video_buffer_offset(output, target);
            } else {
                src = 0;
                dst = pending;
                video_copy_words_retrace(
                    target, &dst, source, &src, 0x280, (backwards ? 0x400 : 0));
            }
        } else {
            src = offset + (mode == 1 ? video_previous_page_offset + 0x8000 : 0);
            dst = pending + (mode == 2 ? 0xf000 : 0x1e00);
            if (mode == 0) {
                {
                    struct GameStripCopy copy =
                        transition_status_copy(source, target, src, dst, 2, backwards);
                    src = copy.source_end;
                    dst = copy.destination_end;
                }
            } else if (mode == 1) {
                SegmentBytes input = source + src;
                SegmentBytes output = target + dst;
                copy_video_bytes(&output, &input, 0x140, backwards);
                src = video_buffer_offset(input, source);
                dst = video_buffer_offset(output, target);
            } else
                video_copy_words_retrace(
                    target, &dst, source, &src, 0x500, (backwards ? 0x400 : 0));
        }
        __asm__ volatile("sti" ::: "memory");
        interrupts_enabled = 1;
        if (end) {
            end->interrupts_enabled = 1;
            end->source_end = src;
            end->destination_end = dst;
            end->copy_count = mode == 0 && direction >= INPUT_DIRECTION_FIRST_VERTICAL ? 0x2000 : 0;
            end->copied = 1;
        }
        video_transition_busy_flags = 0;
    }
    return interrupts_enabled;
}

struct ViewTransitionEnd __far c_transition_cga(SegmentBytes data, int backwards)
{
    struct ViewTransitionEnd end = {0};
    transition_view(data, backwards, 0, &end);
    return end;
}

struct ViewTransitionEnd __far c_transition_ega(SegmentBytes data, int backwards)
{
    struct ViewTransitionEnd end = {0};
    transition_view(data, backwards, 1, &end);
    return end;
}

struct ViewTransitionEnd __far c_transition_vga(SegmentBytes data, int backwards)
{
    struct ViewTransitionEnd end = {0};
    transition_view(data, backwards, 2, &end);
    return end;
}

uint8_t __far c_transition_tandy(void)
{
    uint8_t old = video_transition_steps_left;
    --video_transition_steps_left;
    return old;
}

/* Owned subsystem data. */
struct UnalignedWord video_display_enable_wait_counts[4] = {{0x0010}, {0x0010}, {0x0010}, {0x0020}};
uint16_t video_page_offset = 0x0;
uint16_t video_previous_page_offset = 0x0;
struct VideoPendingPageOffsetStorage video_pending_page_offset_storage = {
    .value = 0x0, .trailing = {0x00, 0x00, 0x11, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00}};
uint16_t video_transition_wait_count = 0x7;
struct UnalignedWord cga_transition_source_mode_0[16] = {{0x015e},
    {0x015c},
    {0x015a},
    {0x0158},
    {0x0156},
    {0x0154},
    {0x0152},
    {0x0150},
    {0x014e},
    {0x014c},
    {0x014a},
    {0x0148},
    {0x0146},
    {0x0144},
    {0x0142},
    {0x0140}};
struct UnalignedWord cga_transition_source_mode_1[16] = {{0x0140},
    {0x0142},
    {0x0144},
    {0x0146},
    {0x0148},
    {0x014a},
    {0x014c},
    {0x014e},
    {0x0150},
    {0x0152},
    {0x0154},
    {0x0156},
    {0x0158},
    {0x015a},
    {0x015c},
    {0x015e}};
struct UnalignedWord cga_transition_source_mode_2[8] = {
    {0x0140}, {0x0280}, {0x03c0}, {0x0500}, {0x0640}, {0x0780}, {0x08c0}, {0x0a00}};
struct UnalignedWord cga_transition_source_mode_3[8] = {
    {0x0a00}, {0x08c0}, {0x0780}, {0x0640}, {0x0500}, {0x03c0}, {0x0280}, {0x0140}};
struct UnalignedWord cga_transition_page_deltas[4] = {{0x0002}, {0xfffe}, {0xfec0}, {0x0140}};
struct UnalignedWord ega_transition_source_mode_0[16] = {{0x014f},
    {0x014e},
    {0x014d},
    {0x014c},
    {0x014b},
    {0x014a},
    {0x0149},
    {0x0148},
    {0x0147},
    {0x0146},
    {0x0145},
    {0x0144},
    {0x0143},
    {0x0142},
    {0x0141},
    {0x0140}};
struct UnalignedWord ega_transition_source_mode_1[16] = {{0x0140},
    {0x0141},
    {0x0142},
    {0x0143},
    {0x0144},
    {0x0145},
    {0x0146},
    {0x0147},
    {0x0148},
    {0x0149},
    {0x014a},
    {0x014b},
    {0x014c},
    {0x014d},
    {0x014e},
    {0x014f}};
struct UnalignedWord ega_transition_source_mode_2[8] = {
    {0x0140}, {0x0280}, {0x03c0}, {0x0500}, {0x0640}, {0x0780}, {0x08c0}, {0x0a00}};
struct UnalignedWord ega_transition_source_mode_3[8] = {
    {0x0a00}, {0x08c0}, {0x0780}, {0x0640}, {0x0500}, {0x03c0}, {0x0280}, {0x0140}};
struct UnalignedWord ega_transition_page_deltas[4] = {{0x0001}, {0xffff}, {0xfec0}, {0x0140}};
struct UnalignedWord vga_transition_source_mode_0[16] = {{0x0a78},
    {0x0a70},
    {0x0a68},
    {0x0a60},
    {0x0a58},
    {0x0a50},
    {0x0a48},
    {0x0a40},
    {0x0a38},
    {0x0a30},
    {0x0a28},
    {0x0a20},
    {0x0a18},
    {0x0a10},
    {0x0a08},
    {0x0a00}};
struct UnalignedWord vga_transition_source_mode_1[16] = {{0x0a00},
    {0x0a08},
    {0x0a10},
    {0x0a18},
    {0x0a20},
    {0x0a28},
    {0x0a30},
    {0x0a38},
    {0x0a40},
    {0x0a48},
    {0x0a50},
    {0x0a58},
    {0x0a60},
    {0x0a68},
    {0x0a70},
    {0x0a78}};
struct UnalignedWord vga_transition_source_mode_2[8] = {
    {0x0a00}, {0x1400}, {0x1e00}, {0x2800}, {0x3200}, {0x3c00}, {0x4600}, {0x5000}};
struct UnalignedWord vga_transition_source_mode_3[8] = {
    {0x5000}, {0x4600}, {0x3c00}, {0x3200}, {0x2800}, {0x1e00}, {0x1400}, {0x0a00}};
struct UnalignedWord vga_transition_page_deltas[4] = {{0x0008}, {0xfff8}, {0xf600}, {0x0a00}};
