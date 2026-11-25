
/* Pause, end-game dissolves and the displaced overview camera. */

/* Native effects carry only completion/IF decisions. Original-entry gates
 * request the optional terminal observations from these same operations. */
uint8_t __far show_warning(
    SegmentBytes data, int backwards, uint16_t idle_port, struct WarningDispatchEnd *observation)
{
    WarningCallback binding = segment_record(data, graphics_session).warning;
    if (observation)
        observation->executed = binding != 0;
    if (!binding)
        return 0;
    unsigned mode = binding == video_warning_cga ? 0
        : binding == video_warning_tandy         ? 1
        : binding == video_warning_ega           ? 2
                                                 : 3;
    if (observation) {
        observation->mode = mode;
        call_video_warning(binding, data, backwards, idle_port, &observation->warning);
        return (observation->warning.completed ? 1 : 0) | (observation->warning.drawn ? 2 : 0);
    }
    if (binding == video_warning_cga || binding == video_warning_tandy ||
        binding == video_warning_ega || binding == video_warning_vga)
        return warning_bar(data, backwards, idle_port, 0, mode);
    struct WarningEnd opaque;
    call_video_warning(binding, data, backwards, idle_port, &opaque);
    return (opaque.completed ? 1 : 0) | (opaque.drawn ? 2 : 0);
}

struct WarningDispatchEnd __far draw_warning_bar(
    SegmentBytes data, int backwards, uint16_t idle_port)
{
    struct WarningDispatchEnd end = {0};
    show_warning(data, backwards, idle_port, &end);
    return end;
}

static void pause_game(
    SegmentBytes data, int backwards, uint16_t idle_port, struct PauseEnd *observation)
{
    uint8_t action = input_action_slots[INPUT_SLOT_PAUSE];
    if (observation)
        observation->action = action;
    if ((action & 128) || action != PAUSE_COMMAND_PAUSE)
        return;
    if (observation)
        observation->entered = 1;
    game_pause_flags |= 128;
    sentry_warning_pattern = 8;
    uint8_t before =
        show_warning(data, backwards, idle_port, observation ? &observation->before : 0);
    uint8_t control = c_silence_all();
    if (observation)
        observation->control = control;
    sound_mute_saved = sound_muted;
    do {
        sound_muted = 255;
        action = input_action_slots[INPUT_SLOT_PAUSE];
    } while (action != PAUSE_COMMAND_RESUME);
    sound_muted = sound_mute_saved;
    if (observation)
        observation->restored_mute = sound_mute_saved;
    sentry_warning_pattern = 0;
    show_warning(
        data, backwards, before & 1 ? 0 : idle_port, observation ? &observation->after : 0);
    game_pause_flags &= 127;
    if (observation)
        observation->pause_flags = game_pause_flags;
}

void __far check_game_pause(SegmentBytes data, int backwards, uint16_t idle_port)
{
    pause_game(data, backwards, idle_port, 0);
}

struct PauseEnd __far c_pause_check(SegmentBytes data, int backwards, uint16_t idle_port)
{
    struct PauseEnd end = {0};
    pause_game(data, backwards, idle_port, &end);
    return end;
}

uint8_t __far scatter_effect(
    SegmentBytes data, uint16_t pixels, uint8_t color, struct ScatterDispatchEnd *observation)
{
    ScatterCallback binding = segment_record(data, graphics_session).scatter;
    if (observation)
        observation->executed = binding != 0;
    if (!binding)
        return 0;
    if (observation) {
        call_video_scatter(binding, pixels, color, &observation->scatter);
        return 1 | (observation->scatter.interrupts_enabled ? 2 : 0);
    }
    unsigned mode = binding == video_scatter_cga ? 0
        : binding == video_scatter_tandy         ? 1
        : binding == video_scatter_ega           ? 2
                                                 : 3;
    if (binding == video_scatter_cga || binding == video_scatter_tandy ||
        binding == video_scatter_ega || binding == video_scatter_vga)
        return 1 | (scatter_pixels(pixels, color, 0, mode) ? 2 : 0);
    struct ScatterEnd opaque;
    call_video_scatter(binding, pixels, color, &opaque);
    return 1 | (opaque.interrupts_enabled ? 2 : 0);
}

struct ScatterDispatchEnd __far c_scatter_dispatch(SegmentBytes data, uint16_t count, uint8_t color)
{
    struct ScatterDispatchEnd end = {0};
    scatter_effect(data, count, color, &end);
    return end;
}

struct ScatterDispatchEnd __far c_scatter_highlight(SegmentBytes data)
{
    return c_scatter_dispatch(data, 50, 14);
}

struct ScatterDispatchEnd __far c_scatter_clear(SegmentBytes data)
{
    return c_scatter_dispatch(data, 200, 0);
}

uint8_t __far scatter_repeated(
    SegmentBytes data, uint8_t passes, struct ScatterDispatchEnd *observation)
{
    uint8_t effects = 0;
    scene_scatter_outer_count = passes;
    do {
        segment_record(data, shared_work).pass_counter = 30;
        do {
            struct ScatterDispatchEnd next;
            if (observation)
                next = (struct ScatterDispatchEnd){0};
            uint8_t current = scatter_effect(data, 200, 0, observation ? &next : 0);
            effects |= current;
            if (observation && (current & 1)) {
                if (!next.scatter.mask_updated && observation->scatter.mask_updated) {
                    next.scatter.mask_updated = 1;
                    next.scatter.mask_index = observation->scatter.mask_index;
                }
                next.scatter.interrupts_enabled |= observation->scatter.interrupts_enabled;
                *observation = next;
            }
            --segment_record(data, shared_work).pass_counter;
        } while (segment_record(data, shared_work).pass_counter);
        --scene_scatter_outer_count;
    } while (scene_scatter_outer_count);
    return effects;
}

struct ScatterDispatchEnd __far c_scatter_repeated(SegmentBytes data, uint8_t passes)
{
    struct ScatterDispatchEnd end = {0};
    scatter_repeated(data, passes, &end);
    return end;
}

struct RenderEffects __far present_end_effects(SegmentBytes data,
    SegmentBytes output,
    uint8_t passes,
    int backwards,
    struct RenderEnd *observation)
{
    c_silence_all();
    struct RenderEffects end = gameplay_hud(data, 1, backwards, observation);
    struct PolygonScanEnd *work = observation ? &observation->polygon : 0;
    if (end.data_output)
        output = data;
    sound_sequence_mode = 6;
    sound_ramp_index = 1;
    uint8_t scatter = scatter_repeated(data, passes, 0);
    gameplay_scatter(&end, scatter, observation);
    if (end.data_output)
        output = data;
    end.target = 0;
    end.target_updated = 1;
    if (observation) {
        work->span_destination = 0;
    }
    end.target_updated = 1;
    if (observation) {
        work->changed |= POLYGON_END_SLOT | POLYGON_DESTINATION;
    }
    software_cursor_saved_last_step = cursor_mode_flags = 0;
    uint8_t configured = c_configure_preview_object(
        segment_record(data, game_work).work_scene_view_model_selector, 0);
    if (observation) {
        work->value = (work->value & 0xff00) | configured;
    }
    end.source = (end.source & 0xff00) | 2;
    end.source_updated = 1;
    if (observation) {
        work->changed |= POLYGON_VERTEX;

        work->flags = status(work->flags, 0, 8);
    }
    segment_record(data, game_work).work_scene_background_style = 3;
    segment_record(data, game_work).work_allocated_object_slot = 1;
    selection_model_draw_flags = selection_presentation_flags = 192;
    selection_copy_interrupt_flags &= 127;
    if (observation) {
        work->value = (work->value & 0xff00) | 50;
    }
    c_start_melody(50);
    {
        struct RenderEnd raw_selection;
        if (observation) {
            raw_selection = (struct RenderEnd){0};
            raw_selection.polygon.flags = work->flags;
        }
        struct RenderEffects selection = present_selected_effects(
            data, output, end.source, end.target, end.backwards, observation ? &raw_selection : 0);
        merge_render_effects(&end, &selection, 0);
        if (observation)
            render_merge(observation, &raw_selection);
    }
    scatter = scatter_repeated(data, 30, 0);
    if (observation) {
        work->value = (work->value & 0xff00) | 30;
    }
    gameplay_scatter(&end, scatter, observation);
    if (observation) {
        observation->polygon.start_vertex = end.source;
        observation->polygon.end_slot = end.target;
        observation->data_output = end.data_output;
    }
    return end;
}

struct RenderEffects __far render_end_effects(
    SegmentBytes data, SegmentBytes output, uint8_t passes, int backwards)
{
    return present_end_effects(data, output, passes, backwards, 0);
}

struct RenderEnd __far c_end_effect(
    SegmentBytes data, SegmentBytes output, uint8_t passes, uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    present_end_effects(data, output, passes, !!(flags & 0x400), &end);
    return end;
}

struct RenderEnd __far overview_scene_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct RenderEnd end = {0};
    struct PolygonScanEnd *work = &end.polygon;
    work->flags = flags;
    if (render_allocate(&end, -1) < 0)
        return end;
    uint8_t camera = work->start_vertex;
    overview_camera_slot = camera;
    uint8_t player = player_robot_slot;
    ray_heading_angle = (uint16_t)object_heading[player] << 8;
    object_heading[camera] = object_heading[player] ^ 128;
    uint8_t pitch = object_view_pitch[player] + 11;
    if ((int8_t)pitch < 0 || pitch < 2)
        pitch = 2;
    if (pitch >= 42)
        pitch = 41;
    ray_pitch_angle = ((uint16_t)pitch << 8) | 32;
    object_view_pitch[camera] = -pitch - 9;
    math_work_lo = 32;
    c_synthesize_ray_direction(((uint16_t)pitch << 8) | 32, data);
    object_grid_x[camera] =
        (segment_word(data, (uint16_t)&occlusion_ray_step_x) >> 6) + object_grid_x[player];
    object_height_integer[camera] =
        (segment_word(data, (uint16_t)&occlusion_ray_step_height) >> 6) +
        object_height_integer[player];
    object_grid_y[camera] =
        (segment_word(data, (uint16_t)&occlusion_ray_step_y) >> 6) + object_grid_y[player];
    object_height_fraction[camera] = 192;
    view_object_slot = camera;
    fill_segment_bytes(
        output + ((uint16_t)terrain_visibility_bits), 128, 255, !!(work->flags & 0x400));
    {
        if (observed) {
            struct RenderEnd background = c_scene_background(data, output, work->flags);
            render_merge(&end, &background);
        } else {
            struct RenderEffects background =
                scene_background(data, output, !!(work->flags & 0x400), 0);
            render_geometry_effect(&end, &background);
        }
        if (end.data_output)
            output = data;
    }
    terrain_shading_mode = 1;
    {
        struct RenderEffects terrain = draw_terrain_view(data, output, !!(work->flags & 0x400));
        render_geometry_effect(&end, &terrain);
    }
    terrain_shading_mode = 2;
    /* Overview's next operation is a palette callback taking full FLAGS. */
    if (!observed)
        work->flags = status(work->flags, 0, 16);
    return end;
}

struct RenderEnd __far c_overview_scene(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return overview_scene_flow(data, output, flags, 1);
}
