
/* Original DS:4B16, now owned independently by this module. */
uint8_t keyboard_ascii_by_scan_code[128] = "##1234567890##\x7f#" /* 00..0F */
                                           "############\x0d###" /* 10..1F */
                                           "################"    /* 20..2F */
                                           "################"    /* 30..3F */
                                           "#######789#456#1"    /* 40..4F */
                                           "230\x7f############" /* 50..5F */
                                           "################"    /* 60..6F */
                                           "####\x0d###########" /* 70..7F */;

/* Prompt input consumes the first FF scan state in scan-code order. BIOS key
 * buffering is not used after the game's IRQ/intercept handler is installed. */
extern uint8_t keyboard_ascii_by_scan_code[];

struct PressedKeySearch {
    uint16_t cursor, remaining;
    uint8_t state;
};

static struct PressedKeySearch find_pressed_key(SegmentBytes scans, uint16_t start, uint16_t count)
{
    struct PressedKeySearch result;
    result.cursor = start;
    result.remaining = count;
    do {
        result.state = scans[result.cursor];
        ++result.cursor;
        --result.remaining;
        if (result.state == 255)
            break;
    } while (result.remaining);
    return result;
}

static struct InputKey finish_input_key(struct InputKey key, struct KeyWaitEnd *observation)
{
    if (observation) {
        observation->character = key.character;
        observation->leave = key.leave;
        observation->flags = key.flags;
        observation->remaining = key.remaining;
    }
    return key;
}

__attribute__((noinline)) struct InputKey __far wait_key(
    SegmentBytes data, SegmentBytes scans, uint16_t initial_flags, struct KeyWaitEnd *observation)
{
    struct InputKey end = {0};
    if (observation)
        *observation = (struct KeyWaitEnd){0};
    end.flags = initial_flags;
    if (initial_flags & 0x400) {
        end.flags |= CF;
        end.leave = 1;
        return finish_input_key(end, observation);
    }
    for (;;) {
        end.character = 255;
        struct PressedKeySearch search = find_pressed_key(scans, (uint16_t)key_down, 125);
        uint16_t cursor = search.cursor;
        end.flags = subtract8(end.flags, 255, search.state);
        end.remaining = search.remaining;
        if (observation)
            observation->scan = cursor;
        if (search.state == 255) {
            uint16_t scan = cursor - 1 - (uint16_t)key_down;
            data[cursor - 1] = 0;
            if (observation)
                observation->scan = scan;
            end.flags = subtract16(end.flags, scan, 1);
            if (scan == 1) {
                end.flags = subtract8(end.flags, preview_escape_restarts, 0);
                if (!preview_escape_restarts) {
                    end.leave = 1;
                    return finish_input_key(end, observation);
                }
                restart_requested_flags = 128;
            }
            end.character = data[(uint16_t)keyboard_ascii_by_scan_code + scan];
            end.flags = status(end.flags, keyboard_e0_prefix, 8);
            if (keyboard_e0_prefix) {
                uint8_t character = end.character;
                keyboard_e0_prefix = 0;
                end.flags = subtract8(end.flags, character, 48);
                if (character >= 48) {
                    end.flags = subtract8(end.flags, character, 57);
                    if (character <= 57)
                        end.character = 35;
                }
            }
            return finish_input_key(end, observation);
        }
        end.flags = status(end.flags, input_buttons & 1, 8);
        if (input_buttons & 1) {
            input_buttons = 0;
            end.character = 35;
            return finish_input_key(end, observation);
        }
    }
}

struct InputKey __far wait_fresh_key(
    SegmentBytes data, SegmentBytes scans, uint16_t flags, struct KeyWaitEnd *observation)
{
    c_clear_pending(scans, !!(flags & 0x400));
    input_buttons = 0;
    struct InputKey key = wait_key(data, scans, status(flags, 0, 16), observation);
    if (observation)
        observation->clear_high = 1;
    return key;
}

struct KeyWaitEnd __far c_wait_key(SegmentBytes data, SegmentBytes scans, uint16_t flags)
{
    struct KeyWaitEnd end;
    wait_key(data, scans, flags, &end);
    return end;
}

struct KeyWaitEnd __far c_wait_fresh_key(SegmentBytes data, SegmentBytes scans, uint16_t flags)
{
    struct KeyWaitEnd end;
    wait_fresh_key(data, scans, flags, &end);
    return end;
}
