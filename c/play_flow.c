/* Foreground play loop, view publication, loss/retry and completion screens. */

struct GameFlowEnd __far play_render_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    work->flags = flags;
    uint8_t outcome = hyperspace_outcome_flags;
    view_object_slot = player_robot_slot;
    work->value = ((uint16_t)outcome << 8) | player_robot_slot;
    if (outcome & 128) {
        if (outcome & 64) {
            end.route = FLOW_PLAY_WIN;
            return end;
        }
        if (observed)
            end.render = c_scene_background(data, output, work->flags);
        else {
            struct RenderEffects background =
                scene_background(data, output, !!(work->flags & 0x400), 0);
            render_geometry_effect(&end.render, &background);
        }
        if (end.render.data_output)
            output = data;
    } else {
        if (!(turn_and_occlusion_flags & 128))
            rebuild_visibility(data, output, 0, 0, !!(work->flags & 0x400));
        if (observed)
            end.render = c_scene_background(data, output, work->flags);
        else {
            struct RenderEffects background =
                scene_background(data, output, !!(work->flags & 0x400), 0);
            render_geometry_effect(&end.render, &background);
        }
        if (end.render.data_output)
            output = data;
        segment_record(data, terrain_shading_mode_storage).value = 2;
        {
            struct RenderEffects terrain = draw_terrain_view(data, output, !!(work->flags & 0x400));
            render_geometry_effect(&end.render, &terrain);
            if (end.render.data_output)
                output = data;
        }
        {
            struct RenderEnd hud = render_hud(data, 0, work->flags);
            render_merge(&end.render, &hud);
            if (end.render.data_output)
                output = data;
        }
    }
    c_viewport_preset(2);
    work->start_vertex = 4;
    work->changed |= POLYGON_VERTEX;
    work->value = 0;
    work->flags = status(work->flags, 0, 16);
    while (!(sound_melody_cursor & 128)) {
    }
    render_palette_mode(&end.render, data, output, 0, observed);
    if (end.render.data_output)
        output = data; /* Rendering a view starts on the visible page. Subsequent strips must
     * be drawn into the work page before the scroll backend publishes them. */
    {
        WorkCallback callback = segment_record(data, graphics_session).select_work;
        if (callback) {
            uint16_t value = call_video_work(callback);
            if (callback == video_work_segment)
                work->value = value;
        }
    }
    work->value = (work->value & 0xff00) | hyperspace_outcome_flags;
    if (hyperspace_outcome_flags & 128) {
        game_end_flags = hyperspace_outcome_flags;
        sound_sequence_mode = 6;
        struct RenderEnd effect = c_end_effect(data, data, 5, work->flags);
        render_merge(&end.render, &effect);
        end.route = FLOW_PLAY_LOSS;
    } else
        end.route = FLOW_PLAY_ACTIONS;
    return end;
}

struct GameFlowEnd __far c_play_render(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return play_render_flow(data, output, flags, 1);
}

struct GameFlowEnd __far play_loss_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    work->flags = flags;
    render_palette_mode(&end.render, data, output, 2, observed);
    if (end.render.data_output)
        output = data;
    render_workspace_mode(&end.render, data, output, observed);
    union LandscapeNumber landscape = {.raw = segment_record(data, landscape_number_bcd).raw};
    render_select_landscape(&end.render, landscape);
    end.route = FLOW_GENERATION_PALETTE;
    return end;
}

struct GameFlowEnd __far c_play_loss(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return play_loss_flow(data, output, flags, 1);
}

struct GameFlowEnd __far play_win_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    work->flags = flags;
    render_palette_mode(&end.render, data, output, 2, observed);
    if (end.render.data_output)
        output = data;
    sound_sequence_mode = 0;
    prng_state_low.low = prng_state_low.middle = prng_state_low.high = 0;
    {
        render_workspace_final_mode(&end.render, output, observed);
    }
    {
        struct RenderEnd completion =
            completion_landscape_flow(data, output, work->flags, observed);
        render_merge(&end.render, &completion);
        if (end.render.data_output)
            output = data;
    }
    render_palette_mode(&end.render, data, output, 1, observed);
    if (end.render.data_output)
        output = data;
    sound_trigger_countdown = 10;
    work->value = (work->value & 0xff00) | MELODY_LANDSCAPE_COMPLETION;
    c_start_melody(MELODY_LANDSCAPE_COMPLETION);
    while (!(sound_melody_cursor & 128)) {
    }
    render_text(&end.render, data, TEXT_SCRIPT_COMPLETION_PROMPT);
    if (end.render.data_output)
        output = data;
    end.leave = render_wait_key_mode(&end.render, data, output, observed);
    end.route = FLOW_TITLE;
    return end;
}

struct GameFlowEnd __far c_play_win(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return play_win_flow(data, output, flags, 1);
}

static struct GameFlowEnd play_actions(
    SegmentBytes data, SegmentBytes output, uint16_t boundary_control, int capture)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    /* Only the genuine camera/presentation boundary receives a flag word.
     * Ordinary action/scheduler tasks receive direction and report STI effects. */
    work->flags = capture ? boundary_control : boundary_control & ~ARITH_FLAGS;
    for (;;) {
        struct PlayerWaitEnd observation;
        if (capture) {
            observation = (struct PlayerWaitEnd){0};
            observation.state.flags = work->flags;
        }
        struct ActionWait action =
            wait_player_action(data, output, !!(work->flags & 0x400), capture ? &observation : 0);
        if (action.source_updated) {
            work->start_vertex = action.state.source;
            work->changed |= POLYGON_VERTEX;
        }
        if (action.target_updated) {
            work->end_slot = action.state.target;
            work->changed |= POLYGON_END_SLOT;
        }
        if (capture)
            work->flags = observation.state.flags;
        else {
            work->flags = (work->flags & ~0x400) | (action.state.backwards ? 0x400 : 0);
            if (action.state.interrupts_enabled)
                work->flags |= 0x200;
        }
        end.render.data_output |= action.state.data_output;
        if (end.render.data_output)
            output = data;
        if (action.outcome == PLAYER_WAIT_SCENE_REFRESH) {
            end.route = FLOW_POST_ACTION;
            return end;
        }
        camera_transition_mode = camera_transition_request;
        video_transition_step_count = selection_copy_interrupt_flags = 0;
        if (capture)
            work->value = 0;
        if (mouse_cursor_mode_flags || !(cursor_mode_flags & 128))
            segment_record(data, terrain_work).query_flags |= 128;
        terrain_draw_stop = 0;
        {
            struct RenderEnd strip = c_render_transition_strip(
                data, output, work->start_vertex, work->end_slot, work->flags);
            render_merge(&end.render, &strip);
            if (end.render.data_output)
                output = data;
        }
        segment_record(data, terrain_work).query_flags &= 127;
        uint8_t steps = video_transition_step_count;
        if (capture)
            work->value = (work->value & 0xff00) | steps;
        if (!steps)
            continue;
        video_transition_steps_left = steps;
        while (video_transition_countdown) {
        }
        video_transition_countdown = (uint8_t)(steps + (steps >> 1));
        video_previous_page_offset = video_page_offset;
        if (capture)
            work->value = video_page_offset;
        if (mouse_cursor_mode_flags) {
            struct CursorEffects cursor =
                operate_cursor(data, work->start_vertex, 1, &work->flags, 0);
            render_cursor_effect(&end.render, &cursor);
        }
        render_transition(&end.render, data);
        if (end.render.data_output)
            output = data;
        if (graphics_adapter == 4) {
            struct RenderEnd hud;
            if (capture) {
                hud = (struct RenderEnd){0};
                hud.polygon.flags = work->flags;
            }
            struct RenderEffects effects =
                gameplay_hud(data, 0, !!(work->flags & 0x400), capture ? &hud : 0);
            if (capture)
                render_merge(&end.render, &hud);
            else
                render_geometry_effect(&end.render, &effects);
            if (end.render.data_output)
                output = data;
        }
        if (cursor_mode_flags & 128) {
            struct CursorSync cursor = c_sync_software_cursor();
            if (capture)
                work->vertex_offset = (work->start_vertex & 0xff00) | cursor.transition_index;
            if (capture)
                work->value = (work->value & 0xff00) | cursor.inverted_y;
            work->start_vertex = 0;
            work->changed |= POLYGON_VERTEX_OFFSET | POLYGON_VERTEX;
            if (capture)
                work->flags = status(work->flags, 0, 16);
        }
    }
    return end;
}

struct GameFlowEnd __far run_player_actions(
    SegmentBytes data, SegmentBytes output, uint16_t control)
{
    return play_actions(data, output, control, 0);
}

struct GameFlowEnd __far c_play_actions(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return play_actions(data, output, flags, 1);
}

/* GameWork is reset independently of other subsystems and data placement. */
struct GameWork game_work = {0};

void __far reset_game_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&game_work), 0, sizeof(game_work), 0);
}
