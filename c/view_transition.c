
/* Render a newly exposed strip before the backend scroll publishes it. Failed
 * or interrupted draws roll the camera change back. */

struct RenderEnd __far c_render_transition_strip(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t transition_bank,
    uint16_t flags)
{
    struct RenderEnd end = {0};
    struct PolygonScanEnd *work = &end.polygon;
    uint8_t mode = camera_transition_mode;
    uint8_t slot = view_object_slot;
    if (mode >= INPUT_DIRECTION_COUNT || (transition_bank & 0xff00) || (observer_bank & 0xff00) ||
        slot >= 64) {
        work->flags = flags | CF;
        return end;
    }
    work->flags = flags;
    work->changed = 63;
    int horizontal = mode < INPUT_DIRECTION_FIRST_VERTICAL;
    uint16_t redraw_row = horizontal ? 24 : 8;
    uint16_t redraw_columns = horizontal ? 16 : 40;
    uint16_t draw_flags = flags;
    if (horizontal)
        object_heading[slot] += camera_transition_steps[mode];
    else {
        uint8_t pitch = object_view_pitch[slot];
        draw_flags = subtract8(
            flags, pitch, camera_transition_pitch_targets[mode - INPUT_DIRECTION_FIRST_VERTICAL]);
        if (pitch == camera_transition_pitch_targets[mode - INPUT_DIRECTION_FIRST_VERTICAL]) {
            work->start_vertex = slot;
            work->end_slot = work->span_destination = mode;
            work->value = pitch;
            work->flags = draw_flags;
            return end;
        }
        object_view_pitch[slot] += camera_transition_steps[mode];
    }
    render_pass_columns = redraw_columns;
    {
        struct RenderEnd band =
            render_background_band(data, redraw_row, redraw_columns, 25, draw_flags);
        render_merge(&end, &band);
        if (end.data_output)
            output = data;
    }
    if (horizontal)
        c_clip_view_dynamic();
    else
        c_clip_view_full();
    /* Retain fallback ABI outputs when the terrain renderer does not replace them. */
    work->start_vertex = horizontal ? 4 : 0;
    work->end_slot = work->span_destination = horizontal ? 1 : 0;

    enum RenderOutcome terrain_outcome;
    {
        struct RenderEffects terrain = draw_terrain_view(data, output, !!(work->flags & 0x400));
        terrain_outcome = terrain.outcome;
        render_geometry_effect(&end, &terrain);
        if (end.data_output)
            output = data;
    }

    /* Rendering may change the selected object, mode and published high banks. */
    uint16_t camera_object =
        (work->start_vertex & 0xff00) | (horizontal ? view_object_slot : player_robot_slot);
    uint8_t current_mode = camera_transition_mode;
    uint16_t step_index = (work->end_slot & 0xff00) | current_mode;
    work->start_vertex = camera_object;
    work->end_slot = work->span_destination = step_index;
    if (horizontal) {
        if (terrain_outcome == RENDER_INTERRUPTED) {
            uint8_t heading = object_heading[camera_object];
            uint8_t amount = camera_transition_steps[step_index];
            object_heading[camera_object] -= amount;
            work->value = (work->value & 0xff00) | amount;
            work->flags = subtract8(work->flags, heading, amount);
            return end;
        }
        work->flags = status(work->flags, current_mode, 8);
        if (!current_mode) {
            uint8_t heading = object_heading[camera_object];
            object_heading[camera_object] -= 12;
            work->flags = subtract8(work->flags, heading, 12);
        }
        video_transition_step_count = 16;
        return end;
    }
    if (terrain_outcome == RENDER_INTERRUPTED) {
        uint8_t pitch = object_view_pitch[camera_object];
        uint8_t amount = camera_transition_steps[step_index];
        object_view_pitch[camera_object] -= amount;
        work->value = (work->value & 0xff00) | amount;
        work->flags = subtract8(work->flags, pitch, amount);
    } else {
        work->flags = subtract8(work->flags, current_mode, INPUT_DIRECTION_DOWN);
        if (current_mode == INPUT_DIRECTION_DOWN) {
            uint8_t pitch = object_view_pitch[camera_object];
            object_view_pitch[camera_object] = pitch + 8;
            work->flags = add8(work->flags, pitch, 8);
        }
        video_transition_step_count = 8;
    }
    work->value = (work->value & 0xff00) | c_clip_view_one();
    work->end_slot = work->span_destination = 1;

    return end;
}

struct GameFlowEnd __far preview_key_gate_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    work->flags = flags;
    preview_escape_restarts = 1;
    render_palette_mode(&end.render, data, output, 0, observed);
    if (end.render.data_output)
        output = data;
    end.leave = render_wait_key_mode(&end.render, data, output, observed);
    if (end.leave)
        return end;
    game_timer_suspend_flags &= 127;
    work->flags = status(work->flags, game_timer_suspend_flags, 8);
    end.route = FLOW_POST_ACTION;
    return end;
}

struct GameFlowEnd __far c_preview_key_gate(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return preview_key_gate_flow(data, output, flags, 1);
}

struct PostActionEnd __far c_post_action_gate(void)
{
    struct PostActionEnd end;
    end.speaker_control = c_silence_all();
    end.observed_state = restart_requested_flags;
    if (restart_requested_flags & 128) {
        end.route = POST_ACTION_TITLE;
        return end;
    }
    end.observed_state = game_end_flags;
    end.route = game_end_flags & 128 ? POST_ACTION_LOSS : POST_ACTION_PLAY;
    return end;
}

struct GameFlowEnd __far begin_play_view_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    work->flags = flags;
    render_palette_mode(&end.render, data, output, 2, observed);
    work->flags = subtract16(work->flags, graphics_adapter, 2);
    if (graphics_adapter != 2) {
        FrontCallback front = segment_record(data, graphics_session).front;
        if (front)
            call_video_front(front);
    }
    work->value = 0;
    camera_transition_mode = software_cursor_saved_last_step = cursor_mode_flags =
        sentry_warning_pattern = 0;
    work->flags = status(work->flags, 0, 16);
    end.route = FLOW_OVERVIEW;
    return end;
}

struct GameFlowEnd __far c_begin_play_view(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return begin_play_view_flow(data, output, flags, 1);
}

struct GameFlowEnd __far overview_gate_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    work->flags = flags;
    work->flags = status(work->flags, overview_pending, 8);
    if (overview_pending) {
        game_timer_suspend_flags |= 128;
        render_palette_mode(&end.render, data, output, 2, observed);
        if (end.render.data_output)
            output = data;
        struct RenderEnd overview = overview_scene_flow(data, output, work->flags, observed);
        render_merge(&end.render, &overview);
        if (end.render.data_output)
            output = data;
        render_palette_mode(&end.render, data, output, 0, observed);
        if (end.render.data_output)
            output = data;
        render_text(&end.render, data, TEXT_SCRIPT_POSITION_BOTTOM_INSET);
        render_text(&end.render, data, TEXT_SCRIPT_PRESS_ANY_KEY);
        while (!(sound_melody_cursor & 128)) {
        }
        end.leave = render_wait_key_mode(&end.render, data, output, observed);
        if (end.leave)
            return end;
        game_timer_suspend_flags &= 127;
        overview_pending = 0;
        render_palette_mode(&end.render, data, output, 2, observed);
    }
    end.route = FLOW_PLAY_RENDER;
    return end;
}

struct GameFlowEnd __far c_overview_gate(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return overview_gate_flow(data, output, flags, 1);
}

struct TwoBandCopyEnd __far c_copy_two_cga_bands(
    SegmentBytes source, SegmentBytes destination, uint16_t src, uint16_t dst, int backwards)
{
    struct GameStripCopy copy = c_copy_cga_strip(source, destination, src, dst, backwards);
    struct TwoBandCopyEnd end;
    end.second_source = copy.source_end & 0x1fff;
    end.second_destination = copy.destination_end;
    end.copy =
        c_copy_cga_strip(source, destination, end.second_source, end.second_destination, backwards);
    return end;
}

struct TwoBandCopyEnd __far c_copy_two_four_bank_bands(
    SegmentBytes source, SegmentBytes destination, uint16_t src, uint16_t dst, int backwards)
{
    struct GameStripCopy copy = c_copy_four_bank_strip(source, destination, src, dst, backwards);
    struct TwoBandCopyEnd end;
    end.second_source = copy.source_end & 0x1fff;
    end.second_destination = copy.destination_end;
    end.copy = c_copy_four_bank_strip(
        source, destination, end.second_source, end.second_destination, backwards);
    return end;
}

/* Owned subsystem data. */
uint8_t camera_transition_steps[4] = {
    0x14,
    0xf8,
    0x04,
    0xf4,
};
volatile uint8_t video_transition_countdown = 0x0;
uint8_t camera_transition_pitch_targets[2] = {
    0x35,
    0xcd,
};
