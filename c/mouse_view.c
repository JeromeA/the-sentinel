/* Mouse-edge requests use the same camera priority as keyboard cursor mode. */

void __far c_mouse_view_edges(uint16_t robot_slot)
{
    uint16_t x = mouse_x >> 1;
    uint16_t y = mouse_y;
    uint8_t priority = mouse_axis_priority - 2;
    uint8_t pitch = object_view_pitch[robot_slot];
    if (x >= 151) {
        x = 151;
        camera_transition_request = INPUT_DIRECTION_RIGHT;
    }
    if (x < 8) {
        x = 8;
        camera_transition_request = INPUT_DIRECTION_LEFT;
    }
    camera_cursor_x_pairs = software_cursor_x_pairs = x;
    if (y >= 187) {
        y = 187;
        if ((int8_t)priority < 0 || (camera_transition_request & 128))
            if (pitch != camera_transition_pitch_targets[0])
                camera_transition_request = INPUT_DIRECTION_UP;
    }
    if ((uint8_t)y < 12) {
        y = (y & 0xff00) | 12;
        if ((int8_t)priority < 0 || (camera_transition_request & 128)) {
            uint8_t target = camera_transition_pitch_targets[1];
            if (pitch != target)
                camera_transition_request = INPUT_DIRECTION_DOWN;
        } else
            (void)camera_transition_request;
    }
    camera_cursor_inverted_y = software_cursor_inverted_y = (uint8_t)y;
}

struct CursorSync __far c_sync_software_cursor(void)
{
    struct CursorSync state;
    state.transition_index = camera_transition_mode * 2;
    software_cursor_x_pairs = camera_cursor_x_pairs;
    software_cursor_inverted_y = camera_cursor_inverted_y;
    state.inverted_y = camera_cursor_inverted_y;
    return state;
}
