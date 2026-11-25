/* Scan-state sampling and action collection use device/game values. */

uint8_t __far c_test_scan(uint16_t scan)
{
    return key_down[scan];
}

SegmentBytes __far c_clear_pending(SegmentBytes target, int backwards)
{
    SegmentBytes keys = target + (uint16_t)key_down;
    if (backwards)
        return keys;
    SegmentBytes end = fill_segment_bytes(keys, 128, 0, 0);
    keyboard_e0_prefix = 0;
    return end;
}

uint16_t __far c_mouse_edge_actions(void)
{
    input_action_slots[INPUT_SLOT_HORIZONTAL] = INPUT_ACTION_NONE;
    input_action_slots[INPUT_SLOT_VERTICAL] = INPUT_ACTION_NONE;
    if (mouse_x >= 302)
        input_action_slots[INPUT_SLOT_HORIZONTAL] = INPUT_DIRECTION_RIGHT;
    if (mouse_x < 16)
        input_action_slots[INPUT_SLOT_HORIZONTAL] = INPUT_DIRECTION_LEFT;
    if (mouse_y >= 187)
        input_action_slots[INPUT_SLOT_VERTICAL] = INPUT_DIRECTION_UP;
    uint16_t y = mouse_y;
    if (y < 12)
        input_action_slots[INPUT_SLOT_VERTICAL] = INPUT_DIRECTION_DOWN;
    return y;
}

struct CountdownTick __far c_sentry_countdowns(void)
{
    struct CountdownTick tick = {sentry_countdown_phase, 0};
    sentry_countdown_phase = tick.previous_phase + 0xa1;
    if ((unsigned)tick.previous_phase + 0xa1 > 255)
        return tick;
    int slot;
    for (slot = 23; slot >= 0; --slot) {
        uint8_t counter = sentry_countdown_bytes[slot];
        if (counter >= 2)
            sentry_countdown_bytes[slot] = counter - 1;
        tick.last_counter = counter;
    }
    return tick;
}

uint8_t __far collect_actions(SegmentBytes destination,
    int backwards,
    uint16_t binding,
    uint16_t scan_bank,
    struct ActionScanEnd *end)
{
    uint16_t slot = scan_bank;
    fill_segment_bytes(destination + ((uint16_t)input_action_slots),
        INPUT_SLOT_COUNT,
        INPUT_ACTION_NONE,
        backwards);
    do {
        uint8_t state = c_test_scan(scan_bank | input_key_bindings[binding]);
        if (end)
            slot = scan_bank | state;
        if (state == 255) {
            uint8_t action = input_binding_actions[binding];
            slot = scan_bank | (action & INPUT_BINDING_SLOT_MASK);
            input_action_slots[slot] = action >> INPUT_BINDING_ACTION_SHIFT;
        }
    } while ((int16_t)--binding >= 0);
    uint8_t combined =
        input_action_slots[INPUT_SLOT_HORIZONTAL] & input_action_slots[INPUT_SLOT_VERTICAL];
    if (end) {
        end->binding_end = binding;
        end->last_slot = slot;
        end->combined_action = combined;
    }
    return combined;
}

void __far c_collect_actions(SegmentBytes destination,
    int backwards,
    uint16_t binding,
    uint16_t scan_bank,
    struct ActionScanEnd *end)
{
    collect_actions(destination, backwards, binding, scan_bank, end);
}

/* 128 scan-state bytes, with exactly FFh recognized as pressed. The key-wait scan at CS:19B6 searches only indices 00h..7Ch in increasing order, consumes the first FFh by clearing it, and handles Escape specially. Binding collection reads states without clearing them. Keyboard aliases 7Eh (left) and 7Dh (right) feed input_buttons bits 0/1; those two indices are outside the key-wait scan. Their physical key identities remain unconfirmed. The mouse driver callback writes input_buttons directly, without modifying those keyboard aliases. */
volatile uint8_t key_down[128] = {0};

void __far reset_keyboard_state(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&key_down), 0, sizeof(key_down), 0);
}
