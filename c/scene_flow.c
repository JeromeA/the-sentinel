#include <stdint.h>
#include "register_compat.h"
#include "typed_api.h"
#include "legacy_targets.h"
#include "data_aliases.h"
#include "render_state.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"

/* Scene setup and title/preview flow. The two retained CALL instructions that
 * the original generation logic rewrites are deliberate continuation anchors. */

uint8_t __far c_clip_view_index(uint16_t preset)
{
    if (preset >= 2)
        return viewport_y_min;
    viewport_y_end = view_y_end_presets[preset];
    viewport_y_min = view_y_min_presets[preset];
    return viewport_y_min;
}

uint8_t __far c_clip_view_full(void)
{
    c_viewport_preset(0);
    return c_clip_view_index(0);
}

uint8_t __far c_clip_view_one(void)
{
    return c_clip_view_index(1);
}

uint8_t __far c_clip_view_dynamic(void)
{
    c_viewport_preset(2);
    return c_clip_view_one();
}

int __far initialize_scene(
    SegmentBytes data, SegmentBytes output, int backwards, struct WorkspaceEnd *observation)
{
    if (backwards) {
        if (observation)
            *observation = (struct WorkspaceEnd){0, 255};
        return 0;
    }
    game_timer_suspend_flags |= 128;
    /* Each subsystem owns and resets its arrays and work state. */
    reset_polygon_arrays(output);
    reset_world_objects(output);
    reset_terrain_arrays(output);
    reset_occlusion_rows(output);
    reset_keyboard_state(output);
    reset_generation_validation(output);
    reset_mouse_state(output);
    reset_raster_work(output);
    reset_projection_work(output);
    reset_viewport_work(output);
    reset_ray_work(output);
    reset_sample_work(output);
    reset_terrain_work(output);
    reset_sentry_work(output);
    reset_game_work(output);
    reset_input_work(output);
    reset_selection_work(output);
    reset_letter_work(output);
    reset_sound_work(output);
    reset_random_work(output);
    reset_shared_work(output);
    if (graphics_adapter != 2) {
        FrontCallback front = segment_record(data, graphics_session).front;
        if (front)
            call_video_front(front);
    }
    input_request_ready_flags = sentry_clock_gate_flags = secret_code_pair_work =
        sound_melody_cursor = 128;
    unsigned i;
    for (i = 0; i < INPUT_SLOT_COUNT; ++i)
        input_action_slots[i] = INPUT_ACTION_NONE;
    return finish_scene_initialization(output, backwards, observation);
}

int __far finish_scene_initialization(
    SegmentBytes output, int backwards, struct WorkspaceEnd *observation)
{
    if (backwards) {
        if (observation)
            *observation = (struct WorkspaceEnd){0, 255};
        return 0;
    }
    c_memory_fill(output + ((uint16_t)terrain_visibility_bits), 255, 128, backwards);
    c_memory_fill(output + ((uint16_t)object_stack_links), OBJECT_LINK_UNUSED, 64, backwards);
    uint8_t previous_noise = prng_state_low.middle;
    if (observation)
        observation->previous_noise = previous_noise;
    ++prng_state_low.middle;
    uint8_t y_min = c_clip_view_one();
    if (observation)
        observation->y_min = y_min;
    return y_min != 255;
}

struct WorkspaceEnd __far c_scene_workspace(SegmentBytes data, SegmentBytes output, int backwards)
{
    struct WorkspaceEnd end;
    initialize_scene(data, output, backwards, &end);
    return end;
}

struct WorkspaceEnd __far c_scene_workspace_final(SegmentBytes output, int backwards)
{
    struct WorkspaceEnd end;
    finish_scene_initialization(output, backwards, &end);
    return end;
}

/* Native scene setup carries only actual rendering effects. Arithmetic status
 * is captured at raw entries or finalized by the palette/continuation caller. */
struct RenderEffects __far scene_background(
    SegmentBytes data, SegmentBytes output, int backwards, struct RenderEnd *observation)
{
    struct RenderEffects end = {0};
    end.backwards = backwards;
    c_viewport_preset(0);
    if (observation) {
        uint16_t flags = observation->polygon.flags;
        *observation = (struct RenderEnd){0};
        observation->polygon.changed = POLYGON_VERTEX;
        observation->polygon.flags = status(flags, 0, 16);
    }
    if (scene_flags & 128) {
        draw_panel(RENDER_TITLE, end.backwards);
        end.data_output = 1;
        if (observation) {
            uint16_t flags = observation->polygon.flags;
            *observation = (struct RenderEnd){0};
            observation->polygon.flags = flags;
            observation->data_output = 1;
        }
        if (scene_flags & 32) {
            draw_panel(RENDER_SKYBOX, end.backwards);
            if (observation)
                observation->polygon.value = 0;
        }
    } else {
        end = gameplay_hud(data, 1, backwards, observation);
        /* This scene binding is own-data, unlike the supplied-segment
         * gameplay background service. Preserve its live lookup point. */
        BackgroundCallback binding = graphics_fill_background_band;
        if (binding)
            call_video_background(binding, 24, 40, end.backwards, 0);
        if (observation) {
            observation->polygon.value = 0;
            observation->polygon.end_slot = binding ? 0 : 24;
            observation->polygon.changed |= POLYGON_END_SLOT;
            if (binding) {
                observation->polygon.start_vertex = 0;
                observation->polygon.changed |= POLYGON_VERTEX;
                observation->polygon.flags = status(observation->polygon.flags, 0, 16);
                observation->data_output = 1;
            }
        }
        if (scene_background_style == 3) {
            scene_scatter_passes = 3;
            do {
                uint8_t scatter = scatter_effect(data, 50, 14, 0);
                if (observation) {
                    observation->polygon.start_vertex =
                        (observation->polygon.start_vertex & 0xff00) | 14;
                    observation->polygon.span_count = 50;
                    observation->polygon.changed |= POLYGON_VERTEX | POLYGON_COUNT;
                }
                gameplay_scatter(&end, scatter, observation);
            } while (--scene_scatter_passes);
        }
    }
    end.source = end.target = 0;
    end.source_updated = end.target_updated = 1;
    if (observation) {
        observation->polygon.start_vertex = observation->polygon.end_slot = 0;
        observation->polygon.changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
        observation->polygon.flags = status(observation->polygon.flags, 0, 16);
    }
    return end;
}

struct RenderEnd __far c_scene_background(
    SegmentBytes data, SegmentBytes output, uint16_t initial_flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = initial_flags;
    scene_background(data, output, !!(initial_flags & 0x400), &end);
    return end;
}

struct RenderEffects __far scene_render(SegmentBytes data,
    SegmentBytes output,
    uint8_t model,
    uint16_t background,
    uint16_t camera,
    int backwards,
    struct RenderEnd *observation)
{
    struct RenderEffects end = {0};
    end.backwards = backwards;
    uint16_t text_style = background;
    if (background >= 4 || camera >= 2) {
        end.outcome = RENDER_REJECTED;
        if (observation)
            observation->polygon.flags |= CF;
        return end;
    }
    scene_view_model_selector = model;
    scene_background_style = background;
    if (scene_flags & 128)
        text_style = (text_style & 0xff00) | 2;
    text_color_a_parameter = scene_text_color_presets_a[text_style];
    text_color_b_parameter = scene_text_color_presets_b[text_style];
    scene_view_preset = camera;
    object_height_integer[16] = scene_camera_height_table[camera];
    object_view_pitch[16] = scene_camera_pitch_table[camera];
    object_grid_x[16] = scene_camera_x_table[camera];
    object_grid_y[16] = scene_camera_y_table[camera];
    object_heading[16] = scene_camera_heading_table[camera];
    scene_lateral_offset = scene_lateral_offset_table[camera];
    end = scene_background(data, output, backwards, observation);
    if (end.data_output)
        output = data;
    if (!(scene_flags & 64)) {
        if (observation) {
            observation->polygon.value =
                (observation->polygon.value & 0xff00) | scene_view_model_selector;
            observation->polygon.flags =
                status(observation->polygon.flags, scene_view_model_selector, 8);
        }
        if (!(scene_view_model_selector & 128)) {
            struct RenderEffects mesh = draw_clipped_view_model(
                data, output, scene_view_model_selector, end.source, end.backwards);
            merge_render_effects(&end, &mesh, observation);
            if (end.data_output)
                output = data;
        }
        view_lateral_offset = scene_lateral_offset;
        view_object_slot = 16;
        terrain_shading_mode = 0;
        struct RenderEffects terrain = draw_terrain_view(data, output, end.backwards);
        merge_render_effects(&end, &terrain, observation);
        if (end.data_output)
            output = data;
    }
    if (!scene_view_preset) {
        c_memory_fill(output + ((uint16_t)terrain_visibility_bits), 0, 128, end.backwards);
        scene_view_preset = 127;
        if (observation) {
            observation->polygon.span_destination =
                (uint16_t)terrain_visibility_bits + (end.backwards ? -128 : 128);
            observation->polygon.span_count = 0;
            observation->polygon.changed |= POLYGON_DESTINATION | POLYGON_COUNT;
        }
    }
    view_lateral_offset = scene_background_style = 0;
    if (observation) {
        observation->polygon.value = 0;
        observation->polygon.flags = status(observation->polygon.flags, 0, 16);
    }
    return end;
}

struct RenderEnd __far c_scene_render(SegmentBytes data,
    SegmentBytes output,
    uint8_t model,
    uint16_t background,
    uint16_t camera,
    uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    scene_render(data, output, model, background, camera, !!(flags & 0x400), &end);
    return end;
}

struct RenderEnd __far title_screen_flow(
    SegmentBytes data, SegmentBytes output, uint8_t completion, uint16_t flags, int observed)
{
    title_secret_screen_flags = completion;
    data[(uint16_t)&object_heading + 63] = 128;
    data[(uint16_t)&object_height_fraction + 63] = 224;
    data[(uint16_t)&object_height_integer + 63] = 2;
    segment_record(data, terrain_work).model_render_flags |= 128;
    transform_terrain(0, 0);
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    if (completion & 128) {
        write_completion_terrain(data, output, !!(flags & 0x400));
        scene_flags = 160;
        struct RenderEffects scene =
            scene_render(data, output, 0, 3, 1, !!(flags & 0x400), observed ? &end : 0);
        if (!observed)
            render_geometry_effect(&end, &scene);
    } else {
        uint16_t source = (uint16_t)title_banner_markup;
        unsigned count = 15;
        do {
            uint8_t character = data[source];
            source += flags & 0x400 ? -1 : 1;
            draw_terrain_character(character, data, output, !!(flags & 0x400), 0);
        } while (--count);
        scene_flags = 192;
        struct RenderEffects scene =
            scene_render(data, output, 5, 1, 1, !!(flags & 0x400), observed ? &end : 0);
        if (!observed)
            render_geometry_effect(&end, &scene);
    }
    segment_record(data, terrain_work).model_render_flags &= 127;
    scene_flags = 0;
    end.polygon.flags =
        status(end.polygon.flags, segment_record(data, terrain_work).model_render_flags, 8);
    return end;
}

struct RenderEnd __far c_title_screen(
    SegmentBytes data, SegmentBytes output, uint8_t completion, uint16_t flags)
{
    return title_screen_flow(data, output, completion, flags, 1);
}

static void prepare_preview(SegmentBytes data, struct PreviewPrepareEnd *observation)
{
    uint8_t count;
    if (!landscape_nonzero)
        count = 1;
    else {
        count = c_random_sentry_count(observation ? &observation->random_offset : 0);
        if (observation)
            observation->randomized = 1;
        if (count >= landscape_difficulty)
            count = landscape_difficulty;
    }
    landscape_sentry_count = count;
    if (observation)
        observation->placement = c_place_principal_objects(data);
    else
        c_place_initial_objects(data);
    uint16_t variant = (landscape_sentry_count - 1) & 7;
    ColorsCallback binding = segment_record(data, graphics_session).colors;
    if (observation) {
        observation->variant = variant;
        observation->colored = binding != 0;
    }
    if (binding) {
        if (observation)
            observation->mode = binding == video_colors_cga ? 0
                : binding == video_colors_tandy             ? 1
                : binding == video_colors_ega               ? 2
                                                            : 3;
        call_video_colors(binding, variant, observation ? &observation->color : 0);
    }
}

void __far c_build_preview(SegmentBytes data)
{
    prepare_preview(data, 0);
}

struct PreviewPrepareEnd __far c_prepare_preview(SegmentBytes data)
{
    struct PreviewPrepareEnd end = {0};
    prepare_preview(data, &end);
    return end;
}

static void preview_record_attachment(
    struct PreviewPopulateEnd *end, const struct ObjectAttachEnd *attached)
{
    end->cell_bank = attached->cell.bank;
    if (attached->cell.value.object.tag == TERRAIN_OBJECT_TAG) {
        end->support_slot = attached->support_slot;
        end->support_written = 1;
    }
}

static void preview_record_placement(
    struct PreviewPopulateEnd *end, const struct ObjectPlacementEnd *placed)
{
    if (placed->found)
        preview_record_attachment(end, &placed->attachment);
    else
        end->cell_bank = placed->failure_cell.bank;
}

/* The validation stream compares a 43-byte logical reference: prefix,
 * packed entry, zero run, timer latch, landscape number and visibility bits.
 * These inputs are owned separately; their former DS adjacency is irrelevant. */
static uint8_t generation_reference_byte(SegmentBytes data, uint8_t offset)
{
    if (!offset)
        return data[(uint16_t)generation_reference_window];
    --offset;
    if (offset < 8)
        return segment_record(data, number_entry_buffer).raw[offset];
    offset -= 8;
    if (offset < 8)
        return data[(uint16_t)generation_reference_zero_run + offset];
    offset -= 8;
    if (!offset)
        return data[(uint16_t)&game_timer_suspend_flags];
    --offset;
    if (offset < 2)
        return segment_record(data, landscape_number_bcd).bytes[offset];
    offset -= 2;
    return offset < 23 ? data[(uint16_t)terrain_visibility_bits + offset] : 0;
}

struct PopulationCheck __far populate_preview(
    SegmentBytes data, uint16_t validation_bank, struct PreviewPopulateEnd *observation)
{
    struct PopulationCheck result = {0};
    if (observation)
        *observation = (struct PreviewPopulateEnd){0};
    int slot = c_allocate_model(MODEL_ROBOT);
    if (slot < 0)
        slot = 255;
    player_robot_slot = slot;
    player_energy = 10;
    uint8_t limit;
    if (!landscape_nonzero) {
        terrain_scan_x = 8;
        terrain_scan_y = 17;
        struct ObjectAttachEnd attached;
        attach_object(slot, data, observation ? &attached : 0);
        if (observation)
            preview_record_attachment(observation, &attached);
    } else {
        struct ObjectPlacementEnd placed;
        int found;
        do {
            limit = landscape_object_height_limit;
            if (limit >= 6)
                limit = 6;
            found = place_object_flat(slot, limit, data, observation ? &placed : 0);
            if (observation)
                preview_record_placement(observation, &placed);
        } while (!found);
    }
    math_work_hi = 48 - landscape_sentry_count * 3;
    {
        uint8_t sample;
        limit = c_random_small_sum(&sample) + 10;
    }
    if (limit >= math_work_hi)
        limit = math_work_hi;
    placement_remaining_trees = limit;
    do {
        slot = c_allocate_model(MODEL_TREE);
        if (slot < 0)
            slot = 255;
        struct ObjectPlacementEnd placed;
        int found =
            place_object_flat(slot, landscape_object_height_limit, data, observation ? &placed : 0);
        if (observation)
            preview_record_placement(observation, &placed);
        if (!found)
            break;
    } while (--placement_remaining_trees);
    int reference = landscape_generation_flags & 128 ? 37 : 42;
    uint16_t ring_bank = validation_bank & 0xff00;
    uint8_t ring_cursor = generation_validation_seed;
    for (; reference >= 0; --reference) {
        uint8_t pair = c_random_decimal_pair();
        uint8_t difference = pair - generation_reference_byte(data, reference);
        landscape_validation_bits = (landscape_validation_bits << 1) | (difference == 0);
        difference += object_stack_links[0];
        generation_validation_ring[ring_bank | ring_cursor] = difference;
        ++ring_cursor;
        uint8_t random_high = prng_state_low.middle;
        result.validation = difference;
        if (observation)
            observation->value = ((uint16_t)random_high << 8) | difference;
    }
    if (observation)
        observation->validation_index = ring_bank | ring_cursor;
    result.previous_flags = landscape_generation_flags;
    if (observation)
        observation->previous_generation_flags = result.previous_flags;
    landscape_generation_flags <<= 1;
    if (!(result.previous_flags & 128)) {
        result.validation = landscape_validation_bits & 0x1e;
        result.accepted = result.validation == 0x1e;
        if (observation) {
            observation->value = (observation->value & 0xff00) | result.validation;
            observation->accepted = result.accepted;
        }
    }
    return result;
}

struct PreviewPopulateEnd __far c_populate_preview(SegmentBytes data, uint16_t validation_bank)
{
    struct PreviewPopulateEnd end;
    populate_preview(data, validation_bank, &end);
    return end;
}

uint8_t __far generate_landscape(SegmentBytes data, struct LandscapeGenerateEnd *observation)
{
    if (observation)
        *observation = (struct LandscapeGenerateEnd){0};
    int index;
    for (index = 80; index >= 0; --index)
        terrain_filter_work[index] = game_random_next(&prng_state_low).value;
    if (!landscape_nonzero)
        landscape_height_scale = 24;
    else {
        uint8_t sample;
        landscape_height_scale = c_random_small_sum(&sample) + 14;
        if (observation) {
            observation->noise_sample = sample;
            observation->sampled = 1;
        }
    }
    transform_terrain(128, 0);
    smooth_terrain(0, data, 0);
    transform_terrain(1, 0);
    smooth_terrain(64, data, 0);
    int x;
    int y;
    for (y = TERRAIN_LAST_CELL; y >= 0; --y) {
        terrain_scan_y = y;
        for (x = TERRAIN_LAST_CELL; x >= 0; --x) {
            terrain_scan_x = x;
            uint8_t shape = classify_terrain(x, y, 0);
            if (observation)
                observation->final_shape = shape;
            terrain_grid[(((uint16_t)(x & TERRAIN_STORAGE_BANK_MASK) << 8) |
                (((uint16_t)x << 3) & 0xe0) | y)] |= shape << 4;
        }
        terrain_scan_x = 255;
    }
    terrain_scan_y = 255;
    return transform_terrain(2, observation ? &observation->terrain : 0);
}

struct LandscapeGenerateEnd __far c_generate_landscape(SegmentBytes data)
{
    struct LandscapeGenerateEnd end;
    generate_landscape(data, &end);
    return end;
}

struct GameFlowEnd __far preview_caption_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    work->flags = flags;
    c_build_preview(data);
    struct RenderEffects scene =
        scene_render(data, output, 128, 3, 0, !!(flags & 0x400), observed ? &end.render : 0);
    if (!observed)
        render_geometry_effect(&end.render, &scene);
    if (end.render.data_output)
        output = data;
    render_text(&end.render, data, TEXT_SCRIPT_PREVIEW_CAPTION);
    if (end.render.data_output)
        output = data;
    {
        if (write_landscape_number(data, output, !!(work->flags & 0x400)))
            end.render.data_output = 1;
    }
    work->start_vertex = work->end_slot = 0;
    work->changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
    work->flags = status(work->flags, 0, 16);
    end.route = FLOW_PREVIEW_POPULATION;
    return end;
}

struct GameFlowEnd __far c_preview_caption(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return preview_caption_flow(data, output, flags, 1);
}

static struct RenderEffects draw_view_model_core(SegmentBytes data,
    SegmentBytes output,
    uint8_t model,
    uint16_t record,
    uint16_t observer,
    int backwards,
    struct RenderEnd *observation)
{
    if (model >= 10 || record >= 3) {
        struct RenderEffects rejected = {0};
        rejected.backwards = backwards;
        rejected.outcome = RENDER_REJECTED;
        if (observation)
            observation->polygon.flags |= CF;
        return rejected;
    }
    c_configure_preview_object(model, record);
    if (observation)
        observation->polygon.flags = status(observation->polygon.flags, 0, 8);
    return project_mesh(data, output, (observer & 0xff00) | 2, 1, 1, backwards, observation);
}

struct RenderEffects __far draw_clipped_view_model(
    SegmentBytes data, SegmentBytes output, uint8_t model, uint16_t observer, int backwards)
{
    uint8_t saved_y = viewport_y_min;
    uint8_t saved_pass = model_face_pass;
    viewport_y_min = 91;
    struct RenderEffects result =
        draw_view_model_core(data, output, model, 1, observer, backwards, 0);
    viewport_y_min = saved_y;
    model_face_pass = saved_pass;
    return result;
}

struct RenderEnd __far c_draw_clipped_view_model(
    SegmentBytes data, SegmentBytes output, uint8_t model, uint16_t observer_bank, uint16_t flags)
{
    uint8_t saved_y = viewport_y_min;
    uint8_t saved_pass = model_face_pass;
    viewport_y_min = 91;
    struct RenderEnd result = c_draw_view_model(data, output, model, 1, observer_bank, flags);
    viewport_y_min = saved_y;
    model_face_pass = saved_pass;
    return result;
}

struct RenderEnd __far c_draw_view_model(SegmentBytes data,
    SegmentBytes output,
    uint8_t model,
    uint16_t record,
    uint16_t observer,
    uint16_t flags)
{
    struct RenderEnd result = {0};
    result.polygon.flags = flags;
    draw_view_model_core(data, output, model, record, observer, !!(flags & 0x400), &result);
    return result;
}

struct RenderEnd __far c_draw_pedestal_view(
    SegmentBytes data, SegmentBytes output, uint16_t observer, uint16_t position, uint16_t flags)
{
    return c_draw_view_model(
        data, output, MODEL_SENTINEL_PEDESTAL, (position & 0xff00) | 2, observer, flags);
}

struct GameFlowEnd __far startup_title_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    work->flags = flags;
    preview_escape_restarts = 0;
    render_palette_mode(&end.render, data, output, 2, observed);
    if (end.render.data_output)
        output = data;
    render_workspace_mode(&end.render, data, output, observed);
    {
        uint8_t once = intro_once_flags;
        intro_once_flags >>= 1;
        if (once & 1) {
            work->value = 0;
            c_start_melody(0);
        }
    }
    end.route = FLOW_BUILD_TITLE;
    return end;
}

struct GameFlowEnd __far c_startup_title(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return startup_title_flow(data, output, flags, 1);
}

struct GameFlowEnd __far startup_build_title_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    work->flags = flags;
    end.render = title_screen_flow(data, output, 0, flags, observed);
    if (end.render.data_output)
        output = data;
    render_text(&end.render, data, TEXT_SCRIPT_TITLE_PROMPT);
    while (!(sound_melody_cursor & 128)) {
    }
    render_palette_mode(&end.render, data, output, 1, observed);
    if (end.render.data_output)
        output = data;
    end.leave = render_wait_key_mode(&end.render, data, output, observed);
    end.route = FLOW_LANDSCAPE_PROMPT;
    return end;
}

struct GameFlowEnd __far c_startup_build_title(
    SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return startup_build_title_flow(data, output, flags, 1);
}

struct GameFlowEnd __far prompt_landscape_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    work->flags = flags;
    render_workspace_mode(&end.render, data, output, observed);
    {
        struct RenderEnd panel = render_blit(data, RENDER_PROMPT_HEADER, work->flags);
        render_merge(&end.render, &panel);
        if (end.render.data_output)
            output = data;
    }
    render_text(&end.render, data, TEXT_SCRIPT_LANDSCAPE_NUMBER_PROMPT);
    if (end.render.data_output)
        output = data;
    {
        struct RenderEnd panel = render_blit(data, RENDER_LANDSCAPE_PANEL, work->flags);
        render_merge(&end.render, &panel);
        if (end.render.data_output)
            output = data;
    }
    {
        struct DigitInputEnd raw_input;
        struct DecimalInput input =
            enter_decimal_digits(data, output, 4, work->flags, observed ? &raw_input : 0);
        if (observed)
            render_merge(&end.render, &raw_input.render);
        else {
            work->flags = input.flags;
            end.render.data_output |= input.data_output;
        }
        work->end_slot = input.count;
        work->changed |= POLYGON_END_SLOT;
        if (input.leave) {
            end.leave = 1;
            return end;
        }
    }
    if (end.render.data_output)
        output = data;
    render_pack_digits_mode(&end.render, data, output, observed);
    union LandscapeNumber landscape = {
        .raw = segment_record(data, number_entry_buffer).packed.digits.halves[0].raw};
    render_select_landscape(&end.render, landscape);
    work->flags = status(work->flags, landscape_nonzero, 8);
    if (landscape_nonzero)
        end.route = FLOW_SECRET_PROMPT;
    else {
        segment_record(data, number_entry_buffer).packed.digits.halves[0].raw =
            landscape_zero_entry_bcd[0].raw;
        segment_record(data, number_entry_buffer).packed.digits.halves[1].raw =
            landscape_zero_entry_bcd[1].raw;
        work->value = landscape_zero_entry_bcd[1].raw;
        end.route = FLOW_GENERATION_PALETTE;
    }
    return end;
}

struct GameFlowEnd __far c_prompt_landscape(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return prompt_landscape_flow(data, output, flags, 1);
}

struct GameFlowEnd __far prompt_secret_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    work->flags = flags;
    {
        struct RenderEnd panel = render_blit(data, RENDER_PROMPT_HEADER, flags);
        render_merge(&end.render, &panel);
        if (end.render.data_output)
            output = data;
    }
    render_text(&end.render, data, TEXT_SCRIPT_SECRET_CODE_PROMPT);
    if (end.render.data_output)
        output = data;
    c_expand_input_panel(!!(work->flags & 0x400));
    {
        struct DigitInputEnd raw_input;
        struct DecimalInput input =
            enter_decimal_digits(data, output, 8, work->flags, observed ? &raw_input : 0);
        if (observed)
            render_merge(&end.render, &raw_input.render);
        else {
            work->flags = input.flags;
            end.render.data_output |= input.data_output;
        }
        work->end_slot = input.count;
        work->changed |= POLYGON_END_SLOT;
        if (input.leave) {
            end.leave = 1;
            return end;
        }
    }
    if (end.render.data_output)
        output = data;
    render_pack_digits_mode(&end.render, data, output, observed);
    end.route = FLOW_GENERATION_PALETTE;
    return end;
}

struct GameFlowEnd __far c_prompt_secret(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return prompt_secret_flow(data, output, flags, 1);
}

struct GameFlowEnd __far wrong_secret_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    struct GameFlowEnd end = {0};
    struct PolygonScanEnd *work = &end.render.polygon;
    work->flags = flags;
    render_palette_mode(&end.render, data, output, 2, observed);
    if (end.render.data_output)
        output = data;
    render_workspace_mode(&end.render, data, output, observed);
    {
        struct RenderEnd title = title_screen_flow(data, output, 0, work->flags, observed);
        render_merge(&end.render, &title);
        if (end.render.data_output)
            output = data;
    }
    render_palette_mode(&end.render, data, output, 1, observed);
    if (end.render.data_output)
        output = data;
    render_text(&end.render, data, TEXT_SCRIPT_WRONG_SECRET_CODE);
    end.leave = render_wait_key_mode(&end.render, data, output, observed);
    end.route = FLOW_LANDSCAPE_PROMPT;
    return end;
}

struct GameFlowEnd __far c_wrong_secret(SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return wrong_secret_flow(data, output, flags, 1);
}

/* Owned subsystem data. */
uint8_t title_banner_markup[16] = "\x88\xd5THE\x80\xc7 SENTRY \x00";
uint8_t view_y_end_presets[2] = {
    0xf0,
    0xf0,
};
uint8_t view_y_min_presets[3] = {
    0xb0,
    0x30,
    0x00,
};
uint8_t preview_escape_restarts = 0x0;
union LandscapeNumber landscape_zero_entry_bcd[2] = {{.raw = 0x9488}, {.raw = 0x6358}};
uint8_t intro_once_flags = 0x1;
uint8_t scene_camera_height_table[2] = {
    0x21,
    0x4b,
};
uint8_t scene_camera_pitch_table[2] = {
    0xea,
    0xd9,
};
uint8_t scene_camera_x_table[2] = {
    0x0f,
    0x00,
};
uint8_t scene_lateral_offset_table[2] = {
    0x00,
    0xef,
};
uint8_t scene_camera_y_table[2] = {
    0xc2,
    0xbf,
};
uint8_t scene_camera_heading_table[2] = {
    0x00,
    0x12,
};
uint8_t scene_text_color_presets_a[4] = {
    0x00,
    0x02,
    0x01,
    0x01,
};
uint8_t scene_text_color_presets_b[4] = {
    0x00,
    0x03,
    0x02,
    0x02,
};
uint8_t scene_view_preset = 0x1;
uint8_t scene_lateral_offset = 0x0;

/* SharedWork is reset independently of other subsystems and data placement. */
struct SharedWork shared_work = {0};

void __far reset_shared_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&shared_work), 0, sizeof(shared_work), 0);
}

/* 256-byte wrap-indexed validation workspace. CS:4142 stores (packed_BCD_random-reference_byte+object_stack_links[0]) modulo 256 and increments DL. Startup compares reference indices AAh..80h (43 outputs); completion uses A5h..80h (38). Indices 84h..81h address the four entered secret-code bytes. Before occlusion tracing, CS:2E95 requires four 7Fh bytes at seed+29h down to seed+26h and CS:2E9F requires seed+25h to differ. The preceding mismatch corresponds to the FFh terminator after the entered BCD code; a valid match adds the Sentinel support link 7Fh (pedestal slot 3Fh plus flag 40h). Ring indexing wraps modulo 256. */
uint8_t generation_validation_ring[256] = {0};

/* Final zero byte of the logical generation reference (index 80h). Its match
 * becomes bit zero, outside the accepted-code mask 1Eh. This was originally
 * before number_entry_buffer; the reference accessor now names it directly. */
uint8_t generation_reference_prefix = 0x0;

void __far reset_generation_validation(SegmentBytes output)
{
    c_memory_fill(
        output + ((uint16_t)&generation_validation_ring), 0, sizeof(generation_validation_ring), 0);
    c_memory_fill(output + ((uint16_t)&generation_reference_prefix),
        0,
        sizeof(generation_reference_prefix),
        0);
}
