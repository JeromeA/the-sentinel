#include <stdint.h>
extern void abi_c_preview_key_gate(void);

extern void legacy_return(void);

#include "register_compat.h"
#include "typed_entries.h"
#include "legacy_targets.h"
#include "typed_api.h"
#include "data_aliases.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"

static void publish_bearing(struct LegacyRegisters *r, const struct BearingEnd *end)
{
    r->si = end->octant.table_index;
    r->di = end->major;
    if (!end->octant.equal)
        r->ax = end->octant.angle;
    if (end->y_major)
        r->ax = 0;
    r->ax = (r->ax & 0xff00) | end->adjustment;
    r->flags = add8(r->flags, end->angle_high, end->adjustment);
}

static void publish_attachment(struct LegacyRegisters *r, const struct ObjectAttachEnd *end)
{
    r->bp = end->cell.bank;
    r->di = end->cell.offset;
    r->ax = (end->cell.bank & 0xff00) | end->cell.value.raw;
    if (end->cell.value.object.tag == TERRAIN_OBJECT_TAG) {
        r->si = end->support_slot;
        r->ax = end->support_model;
        r->flags =
            subtract8(r->flags, end->support_model, end->support_model == MODEL_BOULDER ? 3 : 6);
        if (!end->accepted) {
            r->flags |= CF;
            return;
        }
    }
    r->ax = ((uint16_t)end->random_high << 8) | end->heading;
    r->cx = 0;
    r->flags = add8(r->flags, end->random & 0xf8, 0x60) & ~CF;
}

static void publish_melody(struct LegacyRegisters *r, const struct MelodyStep *end)
{
    r->bx = end->cursor;
    if (end->observations.accumulator_written)
        r->ax = (r->ax & 0xff00) | end->observations.accumulator_low;
    if (end->observations.duration_written)
        r->cx = (r->cx & 0xff00) | end->observations.duration_units;
    switch (end->event) {
    case MELODY_INVALID_STREAM:
    case MELODY_ENDED:
        r->flags = subtract8(r->flags, 255, 255);
        break;
    case MELODY_NOTE:
        r->flags = status(r->flags, end->payload.note.sound_status, 8) | 0x200;
        break;
    case MELODY_IDLE:
        r->flags = status(r->flags, end->payload.idle.scan_position, 8);
        break;
    }
}

static void publish_terrain_corner(struct LegacyRegisters *r, const struct TerrainCornerEnd *end)
{
    r->bp = end->terrain_bank;
    r->di = end->vertical.bearing.major;
    r->bx = (r->bx & 0xff00) | view_object_slot;
    r->cx = end->vertical.pitch;
    r->si = (r->dx & 0xff00) | end->slot;
    r->dx = (r->dx & 0xff00) | end->column;
    r->ax = (end->vertical.screen_y & 0xff00) | end->clip;
    r->flags = status(r->flags, end->clip, 8);
}

static void publish_text_character(struct LegacyRegisters *r, const struct TextCharacterEnd *end)
{
    r->ax = end->value;
    r->flags = end->flags;
    if (end->rendered) {
        r->dx = (r->dx & ~end->port_mask) | end->port;
        r->es = r->ds;
    }
}

static void publish_number_text(struct LegacyRegisters *r, const struct NumberTextEnd *end)
{
    publish_text_character(r, &end->character);
    r->dx = (r->dx & ~end->character.port_mask) | end->character.port;
    if (end->pair_written)
        r->bx = end->pair;
    if (end->terrain_written) {
        r->bp = end->terrain_bank;
        r->di = end->terrain_offset;
    }
}

static void publish_polygon_scan(struct LegacyRegisters *r, const struct PolygonScanEnd *end)
{
    r->ax = end->value;
    r->flags = end->flags;
    if (end->changed & POLYGON_VERTEX)
        r->bx = end->start_vertex;
    if (end->changed & POLYGON_COUNT)
        r->cx = end->span_count;
    if (end->changed & POLYGON_END_SLOT)
        r->dx = end->end_slot;
    if (end->changed & POLYGON_DESTINATION)
        r->di = end->span_destination;
    if (end->changed & POLYGON_VERTEX_OFFSET)
        r->si = end->vertex_offset;
    if (end->changed & POLYGON_ERROR)
        r->bp = end->error;
}

static void publish_palette(struct LegacyRegisters *r, const struct PaletteEnd *end)
{
    if (!(end->changed & PALETTE_NO_OPERATION))
        r->bx = r->dx = 0;
    r->flags = end->flags;
    if (end->changed & PALETTE_VALUE)
        r->ax = end->value;
    if (end->changed & PALETTE_COUNT)
        r->cx = end->count;
    if (end->changed & PALETTE_ATTRIBUTE_COUNT)
        r->cx = (r->cx & 0xff00) | 4;
    if (end->changed & PALETTE_SOURCE)
        r->si = end->source;
    if (end->changed & PALETTE_DESTINATION)
        r->di = end->destination;
    if (end->changed & PALETTE_DATA_OUTPUT)
        r->es = r->ds;
}

static void publish_selection_interval(
    struct LegacyRegisters *r, const struct SelectionIntervalEnd *end)
{
    r->dx = r->di = end->selected_index;
    if (!end->prepared) {
        r->flags = subtract8(r->flags, end->selected, player_robot_slot) | CF;
        return;
    }
    r->bx = end->model_index;
    r->di = end->vertical.bearing.major;
    r->si = bearing_table_index(&end->vertical.bearing);
    r->cx = end->vertical.pitch;
    r->ax = end->vertical.screen_y;
    r->flags = (status(r->flags, r->ax, 16) & ~CF) |
        (((uint16_t)(end->vertical.angle - end->vertical.pitch) & 8) ? CF : 0) |
        (subtract16(r->flags, end->vertical.angle, end->vertical.pitch) & AF);
    if (end->accepted) {
        r->ax = (r->ax & 0xff00) | end->columns;
        r->flags &= ~CF;
    } else
        r->flags |= CF;
}

static void publish_sentry_partial(struct LegacyRegisters *r, const struct SentryPartialEnd *end)
{
    uint16_t source = r->bx;
    uint16_t target = r->dx;
    r->flags = status(r->flags, end->mode, 8);
    if (!end->mode) {
        r->flags |= CF;
        return;
    }
    r->ax = (r->ax & 0xff00) | player_robot_slot;
    publish_selection_interval(r, &end->interval);
    r->bx = source;
    r->dx = r->di = target;
}

static void publish_warning(struct LegacyRegisters *r, const struct WarningDispatchEnd *dispatch)
{
    const struct WarningEnd *end = &dispatch->warning;
    unsigned mode = dispatch->mode;
    if (!dispatch->executed)
        return;
    r->flags = status(r->flags, end->skip_status, 8);
    if (end->completed) {
        if (end->drawn) {
            r->di = end->destination_offset;
            r->bp = end->last_noise;
            if (mode == 0) {
                r->si = end->pattern;
                r->bx = end->pattern_index * 2;
                r->cx = 0x1fff;
            } else if (mode == 2) {
                r->si = end->pattern;
                r->bx = end->pattern_index;
                r->cx = 0;
            } else {
                r->si = end->source_offset;
                r->bx = end->pattern;
                r->cx = 0;
            }
        }
        r->dx = 0;
        r->ax = r->es = r->ds;
        r->flags = status(r->flags, 0, 16);
    }
}

static void publish_cursor_operation(
    struct LegacyRegisters *r, const struct CursorOperationEnd *operation)
{
    unsigned mode = operation->mode;
    if (!operation->invoked)
        return;
    if (operation->restoring) {
        const struct CursorRestoreEnd *end = &operation->result.restore;
        if (end->restored) {
            r->di = end->destination_offset;
            if (mode == 2)
                r->cx = (r->cx & 0xff00) | end->plane_value;
        }
        r->bx = 0;
        if (mode == 2)
            r->dx = 0;
        r->ax = r->es = r->ds;
        r->flags = status(r->flags, 0, 16);
        return;
    }
    {
        const struct CursorDrawEnd *end = &operation->result.draw;
        r->flags = status(r->flags, end->skip_status, 8);
        if (end->executed) {
            r->ax = r->es = r->ds;
            r->bx = end->terminal_step;
            r->bp = end->row_offset;
            r->cx = end->x_pairs;
            if (end->destination_updated)
                r->di = end->destination_offset;
            if (end->mask_updated)
                r->si = end->mask_index;
            r->flags = subtract8(r->flags, end->saved_count, 1) & ~CF;
            if (mode == 1)
                r->dx = (r->dx & 0xff00) | 0xee;
        }
        if (mode >= 2) {
            r->dx = 0;
            r->flags = status(r->flags, 0, 16);
        }
    }
}

/* Scene routes call typed C operations directly. Only generation/population
 * return sites and hardware cleanup hand control back to a native boundary. */
static struct GameFlowEnd continue_post_action(struct LegacyRegisters *r, int observed)
{
    struct PostActionEnd post = c_post_action_gate();
    r->ax = post.speaker_control;
    r->bx = 255;
    r->si = 0;
    r->cx = (r->cx & 0xff00) | 5;
    r->flags = status(r->flags, post.observed_state, 8) | 0x200;
    if (post.route == POST_ACTION_TITLE)
        return startup_title_flow(segment_bytes(r->ds), segment_bytes(r->es), r->flags, observed);
    if (post.route == POST_ACTION_LOSS)
        return play_loss_flow(segment_bytes(r->ds), segment_bytes(r->es), r->flags, observed);
    return begin_play_view_flow(segment_bytes(r->ds), segment_bytes(r->es), r->flags, observed);
}

static void publish_game_flow_mode(
    struct LegacyRegisters *r, const struct GameFlowEnd *initial, int observed)
{
    struct GameFlowEnd current = *initial;
    for (;;) {
        SegmentBytes data = segment_bytes(r->ds);
        publish_polygon_scan(r, &current.render.polygon);
        if (current.render.data_output)
            r->es = r->ds;
        SegmentBytes output = segment_bytes(r->es);
        if (current.leave) {
            ((uint16_t *)r)[-1] = (uint16_t)&target_c_hardware_leave;
            return;
        }
        switch (current.route) {
        case FLOW_PREVIEW_POPULATION:
            ((uint16_t *)r)[-1] = (uint16_t)&target_preview_population_call;
            return;
        case FLOW_GENERATION_PALETTE: {
            struct PaletteEnd raw_palette;
            struct PaletteEffect palette =
                apply_scene_palette(data, output, 2, r->flags, observed ? &raw_palette : 0);
            if (observed)
                publish_palette(r, &raw_palette);
            else {
                if (palette.operated)
                    r->bx = r->dx = 0;
                if (palette.value_valid)
                    r->ax = palette.value;
                r->flags = palette.flags;
                if (palette.data_output)
                    r->es = r->ds;
            }
            ((uint16_t *)r)[-1] = (uint16_t)&target_generation_call;
            return;
        }
        case FLOW_BUILD_TITLE:
            current = startup_build_title_flow(data, output, r->flags, observed);
            break;
        case FLOW_LANDSCAPE_PROMPT:
            current = prompt_landscape_flow(data, output, r->flags, observed);
            break;
        case FLOW_SECRET_PROMPT:
            current = prompt_secret_flow(data, output, r->flags, observed);
            break;
        case FLOW_TITLE:
            current = startup_title_flow(data, output, r->flags, observed);
            break;
        case FLOW_POST_ACTION:
            current = continue_post_action(r, observed);
            break;
        case FLOW_OVERVIEW:
            current = overview_gate_flow(data, output, r->flags, observed);
            break;
        case FLOW_PLAY_RENDER:
            current = play_render_flow(data, output, r->flags, observed);
            break;
        case FLOW_PLAY_ACTIONS:
            current = observed ? c_play_actions(data, output, r->flags)
                               : run_player_actions(data, output, r->flags);
            break;
        case FLOW_PLAY_LOSS:
            current = play_loss_flow(data, output, r->flags, observed);
            break;
        case FLOW_PLAY_WIN:
            current = play_win_flow(data, output, r->flags, observed);
            break;
        default:
            return;
        }
    }
}

static void publish_game_flow(struct LegacyRegisters *r, const struct GameFlowEnd *initial)
{
    publish_game_flow_mode(r, initial, 1);
}

static void publish_sentry_flow(struct LegacyRegisters *r, const struct SentryEnd *initial)
{
    struct SentryEnd current = *initial;
    for (;;) {
        SegmentBytes data = segment_bytes(r->ds);
        r->bx = current.state.source;
        r->dx = current.state.target;
        r->flags = current.state.flags;
        if (current.state.data_output)
            r->es = r->ds;
        SegmentBytes output = segment_bytes(r->es);
        switch (current.route) {
        case SENTRY_FINISH: {
            struct SentryClockEnd clock = c_sentry_schedule_finish();
            r->ax = clock.value;
            r->cx = 0;
            r->flags = (subtract8(r->flags, clock.previous, 1) & ~CF) | (clock.carry ? CF : 0);
            return;
        }
        case SENTRY_RETRY:
        case SENTRY_PLAYER_LOST:
        case SENTRY_ABORT: {
            struct StackReturnEnd resume;
            if (current.route == SENTRY_RETRY) {
                struct SentryRetryEnd retry = c_sentry_retry_after_redraw(r->bx);
                r->flags = (add8(r->flags, retry.previous, 1) & ~CF) | (r->flags & CF);
                resume = retry.resume;
            } else
                resume = current.route == SENTRY_PLAYER_LOST ? c_sentry_player_absorbed()
                                                             : c_sentry_abort();
            ((uint16_t *)r)[-1] = typed_resume_legacy(resume.stack_pointer, resume.instruction);
            return;
        }
        case SENTRY_CREATION:
            current = c_sentry_publish_creation(data, output, r->bx, r->dx, r->flags);
            break;
        case SENTRY_ACTIVE:
            current = c_sentry_active_dispatch(data, output, r->bx, r->dx, r->flags);
            break;
        case SENTRY_MEANIE:
            current = c_sentry_active_meanie(data, output, r->bx, r->dx, r->flags);
            break;
        case SENTRY_SCAN:
            current = c_sentry_scan_world(data, output, r->bx, r->dx, r->flags);
            break;
        case SENTRY_EXPIRE:
            current = c_meanie_expire(data, output, r->bx, r->dx, r->flags);
            break;
        case SENTRY_TURN:
            current = c_meanie_turn(data, output, r->bx, r->dx, r->flags);
            break;
        case SENTRY_CHANGE:
            current = c_sentry_publish_change(data, output, r->bx, r->dx, r->flags);
            break;
        default:
            return;
        }
    }
}

/* Register publication is confined here while callers migrate to typed C.
 * These values are compatibility observations, not game algorithm inputs. */
void __far publish_typed_entry(struct LegacyRegisters *r, uint32_t frame, uint16_t entry)
{
    (void)frame;
    switch (entry) {
    case TYPED_c_play_render:
    case TYPED_c_play_loss:
    case TYPED_c_play_win:
    case TYPED_c_play_actions: {
        SegmentBytes data = segment_bytes(r->ds);
        SegmentBytes output = segment_bytes(r->es);
        struct GameFlowEnd end;
        switch (entry) {
        case TYPED_c_play_render:
            end = c_play_render(data, output, r->flags);
            break;
        case TYPED_c_play_loss:
            end = c_play_loss(data, output, r->flags);
            break;
        case TYPED_c_play_win:
            end = c_play_win(data, output, r->flags);
            break;
        case TYPED_c_play_actions:
            end = c_play_actions(data, output, r->flags);
            break;
        default:
            return;
        }
        publish_game_flow(r, &end);
        break;
    }
    case TYPED_c_game_bootstrap: {
        struct BootstrapEnd end =
            c_game_bootstrap(segment_bytes(r->ds), segment_bytes(r->es), r->flags);
        r->ax = end.status;
        r->flags = end.flags;
        break;
    }
    case TYPED_startup_title_flow:
    case TYPED_wrong_secret_flow:
    case TYPED_preview_key_flow:
    case TYPED_preview_caption_flow: {
        SegmentBytes data = segment_bytes(r->ds), output = segment_bytes(r->es);
        struct GameFlowEnd flow;
        if (entry == TYPED_startup_title_flow)
            flow = startup_title_flow(data, output, r->flags, 0);
        else if (entry == TYPED_wrong_secret_flow)
            flow = wrong_secret_flow(data, output, r->flags, 0);
        else if (entry == TYPED_preview_key_flow)
            flow = preview_key_gate_flow(data, output, r->flags, 0);
        else
            flow = preview_caption_flow(data, output, r->flags, 0);
        publish_game_flow_mode(r, &flow, 0);
        break;
    }
    case TYPED_c_preview_key_gate:
    case TYPED_c_begin_play_view:
    case TYPED_c_overview_gate: {
        SegmentBytes data = segment_bytes(r->ds);
        SegmentBytes output = segment_bytes(r->es);
        struct GameFlowEnd end;
        switch (entry) {
        case TYPED_c_preview_key_gate:
            end = c_preview_key_gate(data, output, r->flags);
            break;
        case TYPED_c_begin_play_view:
            end = c_begin_play_view(data, output, r->flags);
            break;
        case TYPED_c_overview_gate:
            end = c_overview_gate(data, output, r->flags);
            break;
        default:
            return;
        }
        publish_game_flow(r, &end);
        break;
    }
    case TYPED_c_player_wait_action: {
        struct PlayerWaitEnd end =
            c_player_wait_action(segment_bytes(r->ds), segment_bytes(r->es), r->flags);
        if (end.source_updated)
            r->bx = end.state.source;
        if (end.target_updated)
            r->dx = end.state.target;
        r->flags = end.state.flags;
        if (end.state.data_output)
            r->es = r->ds;
        break;
    }
    case TYPED_c_timer_tick: {
        uint16_t control = c_timer_tick(segment_bytes(r->ds), segment_bytes(r->es), r->flags);
        r->flags = (r->flags & ARITH_FLAGS) | control;
        break;
    }
    case TYPED_c_update_cursor_input:
        c_update_cursor_input(
            segment_bytes(r->ds), segment_bytes(r->es), r->bx, r->dx, !!(r->flags & 0x400));
        break;
    case TYPED_c_render_transition_strip:
    case TYPED_c_end_effect:
    case TYPED_c_overview_scene: {
        SegmentBytes data = segment_bytes(r->ds);
        SegmentBytes output = segment_bytes(r->es);
        struct RenderEnd end;
        if (entry == TYPED_c_render_transition_strip)
            end = c_render_transition_strip(data, output, r->bx, r->dx, r->flags);
        else if (entry == TYPED_c_end_effect)
            end = c_end_effect(data, output, r->ax, r->flags);
        else
            end = c_overview_scene(data, output, r->flags);
        publish_polygon_scan(r, &end.polygon);
        if (end.data_output)
            r->es = r->ds;
        break;
    }
    case TYPED_c_sentry_find_meanie_tree:
    case TYPED_c_sentry_absorb_unit:
    case TYPED_c_sentry_replant:
    case TYPED_c_sentry_find_stack_energy:
    case TYPED_c_sentry_schedule_start:
    case TYPED_c_sentry_active_dispatch:
    case TYPED_c_sentry_active_meanie:
    case TYPED_c_sentry_publish_change:
    case TYPED_c_sentry_publish_creation:
    case TYPED_c_meanie_turn:
    case TYPED_c_meanie_expire:
    case TYPED_c_sentry_scan_world: {
        SegmentBytes data = segment_bytes(r->ds);
        SegmentBytes output = segment_bytes(r->es);
        if (entry == TYPED_c_sentry_schedule_start)
            sentry_scheduler_saved_sp = (uint16_t)frame + 20;
        struct SentryEnd end;
        switch (entry) {
        case TYPED_c_sentry_find_meanie_tree:
            end = c_sentry_find_meanie_tree(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_sentry_absorb_unit:
            end = c_sentry_absorb_unit(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_sentry_replant:
            end = c_sentry_replant(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_sentry_find_stack_energy:
            end = c_sentry_find_stack_energy(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_sentry_schedule_start:
            end = c_sentry_schedule_start(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_sentry_active_dispatch:
            end = c_sentry_active_dispatch(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_sentry_active_meanie:
            end = c_sentry_active_meanie(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_sentry_publish_change:
            end = c_sentry_publish_change(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_sentry_publish_creation:
            end = c_sentry_publish_creation(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_meanie_turn:
            end = c_meanie_turn(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_meanie_expire:
            end = c_meanie_expire(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_sentry_scan_world:
            end = c_sentry_scan_world(data, output, r->bx, r->dx, r->flags);
            break;
        default:
            return;
        }
        publish_sentry_flow(r, &end);
        break;
    }
    case TYPED_c_preview_caption:
    case TYPED_c_startup_title:
    case TYPED_c_startup_build_title:
    case TYPED_c_prompt_landscape:
    case TYPED_c_prompt_secret:
    case TYPED_c_wrong_secret: {
        SegmentBytes data = segment_bytes(r->ds);
        SegmentBytes output = segment_bytes(r->es);
        struct GameFlowEnd end;
        switch (entry) {
        case TYPED_c_preview_caption:
            end = c_preview_caption(data, output, r->flags);
            break;
        case TYPED_c_startup_title:
            end = c_startup_title(data, output, r->flags);
            break;
        case TYPED_c_startup_build_title:
            end = c_startup_build_title(data, output, r->flags);
            break;
        case TYPED_c_prompt_landscape:
            end = c_prompt_landscape(data, output, r->flags);
            break;
        case TYPED_c_prompt_secret:
            end = c_prompt_secret(data, output, r->flags);
            break;
        case TYPED_c_wrong_secret:
            end = c_wrong_secret(data, output, r->flags);
            break;
        default:
            return;
        }
        publish_game_flow(r, &end);
        break;
    }
    case TYPED_c_execute_player_action: {
        struct PlayerActionEnd end =
            c_execute_player_action(segment_bytes(r->ds), r->bx, r->dx, r->flags);
        publish_polygon_scan(r, &end.render.polygon);
        if (end.render.data_output)
            r->es = r->ds;
        break;
    }
    case TYPED_c_render_selection: {
        struct RenderEnd end =
            c_render_selection(segment_bytes(r->ds), segment_bytes(r->es), r->bx, r->dx, r->flags);
        publish_polygon_scan(r, &end.polygon);
        if (end.data_output)
            r->es = r->ds;
        break;
    }
    case TYPED_c_enter_decimal_digits: {
        struct DigitInputEnd end =
            c_enter_decimal_digits(segment_bytes(r->ds), segment_bytes(r->es), r->ax, r->flags);
        publish_polygon_scan(r, &end.render.polygon);
        if (end.render.data_output)
            r->es = r->ds;
        r->dx = end.count;
        if (end.leave)
            ((uint16_t *)r)[-1] = (uint16_t)&target_c_hardware_leave;
        break;
    }
    case TYPED_c_cga_scroll_software: {
        struct SoftwareScrollEnd end = c_cga_scroll_software(segment_bytes(r->ds), r->flags);
        r->ax = (r->ax & 0xff00) | end.retrace.status;
        r->si = end.source;
        r->di = end.destination;
        r->es = r->bp = r->ds;
        r->bx = r->dx = r->cx = 0;
        r->flags = end.flags;
        break;
    }
    case TYPED_c_menu_select:
    case TYPED_bootstrap_menu: {
        struct MenuSelectionEnd end = entry == TYPED_c_menu_select
            ? c_menu_select(
                  segment_bytes(r->ds), segment_bytes(r->es), original_argument(frame, 0), r->flags)
            : bootstrap_menu(segment_bytes(r->ds),
                  segment_bytes(r->es),
                  original_argument(frame, 0),
                  r->flags);
        original_argument_store(frame, 0, end.selection);
        r->ax = end.adapter;
        r->bx = end.selection * 2;
        r->flags = end.flags;
        if (end.text.written)
            r->cx = end.text.count;
        r->dx = ((uint16_t)text_cursor_row << 8) | text_cursor_column;
        break;
    }
    case TYPED_bootstrap_option: {
        uint16_t name = original_argument(frame, 1);
        struct MenuAddEnd end = bootstrap_option(
            segment_bytes(r->es), original_argument(frame, 0), name, !!(r->flags & 0x400));
        r->dx = name;
        r->cx = end.remaining;
        r->bx = end.index * 2;
        r->ax = end.count == 4 ? 0 : 1;
        r->flags = r->ax ? subtract16(r->flags, end.count, 4) : status(r->flags, 0, 16);
        break;
    }
    case TYPED_c_wait_key:
    case TYPED_c_wait_fresh_key: {
        struct KeyWaitEnd end = entry == TYPED_c_wait_key
            ? c_wait_key(segment_bytes(r->ds), segment_bytes(r->es), r->flags)
            : c_wait_fresh_key(segment_bytes(r->ds), segment_bytes(r->es), r->flags);
        r->ax = end.clear_high ? end.character : (r->ax & 0xff00) | end.character;
        r->cx = end.remaining;
        r->di = end.scan;
        r->flags = end.flags;
        if (end.leave)
            ((uint16_t *)r)[-1] = (uint16_t)&target_c_hardware_leave;
        break;
    }
    case TYPED_c_completion_landscape: {
        struct RenderEnd end =
            c_completion_landscape(segment_bytes(r->ds), segment_bytes(r->es), r->flags);
        publish_polygon_scan(r, &end.polygon);
        if (end.data_output)
            r->es = r->ds;
        break;
    }
    case TYPED_c_scene_background:
    case TYPED_c_draw_view_model:
    case TYPED_c_draw_clipped_view_model:
    case TYPED_c_draw_pedestal_view:
    case TYPED_c_scene_render:
    case TYPED_c_title_screen: {
        SegmentBytes data = segment_bytes(r->ds);
        SegmentBytes output = segment_bytes(r->es);
        struct RenderEnd end;
        switch (entry) {
        case TYPED_c_scene_background:
            end = c_scene_background(data, output, r->flags);
            break;
        case TYPED_c_draw_view_model:
            end = c_draw_view_model(data, output, r->ax, r->di, r->bx, r->flags);
            break;
        case TYPED_c_draw_clipped_view_model:
            end = c_draw_clipped_view_model(data, output, r->ax, r->bx, r->flags);
            break;
        case TYPED_c_draw_pedestal_view:
            end = c_draw_pedestal_view(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_scene_render:
            end = c_scene_render(data, output, r->ax, r->bx, r->dx, r->flags);
            break;
        default:
            end = c_title_screen(data, output, r->ax, r->flags);
            break;
        }
        publish_polygon_scan(r, &end.polygon);
        if (end.data_output)
            r->es = r->ds;
        break;
    }
    case TYPED_c_draw_terrain_right:
    case TYPED_c_draw_terrain_row:
    case TYPED_c_finish_terrain_near_row:
    case TYPED_c_draw_terrain_view: {
        SegmentBytes data = segment_bytes(r->ds);
        SegmentBytes output = segment_bytes(r->es);
        struct RenderEnd end;
        if (entry == TYPED_c_draw_terrain_right)
            end = c_draw_terrain_right_observed(data, output, r->ax, r->bx, r->dx, r->flags);
        else if (entry == TYPED_c_draw_terrain_row)
            end = c_draw_terrain_row_observed(data, output, r->ax, r->bx, r->dx, r->flags);
        else if (entry == TYPED_c_finish_terrain_near_row)
            end = c_finish_terrain_near_row_observed(data, output, r->bx, r->dx, r->flags);
        else
            end = c_draw_terrain_view_observed(data, output, r->flags);
        publish_polygon_scan(r, &end.polygon);
        if (end.data_output)
            r->es = r->ds;
        break;
    }
    case TYPED_c_draw_face:
    case TYPED_c_emit_quad:
    case TYPED_c_draw_checker_quad:
    case TYPED_c_emit_cell_triangles:
    case TYPED_c_cell_model:
    case TYPED_c_cell_oriented_shape:
    case TYPED_c_draw_cell:
    case TYPED_c_draw_mesh:
    case TYPED_c_draw_linked_stack: {
        SegmentBytes data = segment_bytes(r->ds);
        SegmentBytes output = segment_bytes(r->es);
        struct RenderEnd end;
        switch (entry) {
        case TYPED_c_draw_face:
            end = c_draw_face_observed(data, output, r->bx, r->flags);
            break;
        case TYPED_c_emit_quad:
            end = c_emit_quad_observed(data, output, r->bx, r->flags);
            break;
        case TYPED_c_draw_checker_quad:
            end = c_draw_checker_quad_observed(data, output, r->flags);
            break;
        case TYPED_c_emit_cell_triangles:
            end = c_emit_cell_triangles_observed(data, output, r->ax, r->bx, r->flags);
            break;
        case TYPED_c_cell_model:
            end = c_cell_model_observed(data, output, r->bx, r->dx, r->ax, r->flags);
            break;
        case TYPED_c_cell_oriented_shape:
            end = c_cell_oriented_shape_observed(data, output, r->ax, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_draw_cell:
            end = c_draw_cell_observed(data, output, r->bx, r->dx, r->flags);
            break;
        case TYPED_c_draw_mesh:
            end = c_draw_mesh_observed(data, output, r->bx, r->di, r->dx, r->flags);
            break;
        default:
            end = c_draw_linked_stack_observed(data, output, r->ax, r->bx, r->dx, r->flags);
            break;
        }
        publish_polygon_scan(r, &end.polygon);
        if (end.data_output)
            r->es = r->ds;
        break;
    }
    case TYPED_c_sentry_test_sight: {
        struct SentrySightEnd end =
            c_sentry_test_sight(segment_bytes(r->ds), r->ax, r->bx, r->dx, r->di, r->flags);
        r->ax = end.ray.value;
        r->bx = end.ray.observer_index;
        r->dx = r->di = end.ray.position_index;
        r->flags = end.ray.flags;
        if (end.prepared) {
            r->cx = end.ray.height_work;
            r->si = end.lookup;
        }
        if (end.ray.cell_sampled)
            r->bp = end.ray.cell_bank;
        break;
    }
    case TYPED_c_restore_action_cursor: {
        struct CursorOperationEnd end = c_restore_action_cursor(segment_bytes(r->ds), r->bx);
        r->flags = status(r->flags, end.initial_mode, 8);
        publish_cursor_operation(r, &end);
        break;
    }
    case TYPED_c_hyperspace: {
        struct HyperspaceEnd end = c_hyperspace(segment_bytes(r->ds));
        r->bx = end.robot;
        r->cx = 0;
        if (!end.placement.found) {
            r->bp = end.placement.failure_cell.bank;
            r->di = end.placement.failure_cell.offset;
            r->ax = (end.placement.failure_cell.bank & 0xff00) | end.placement.height_limit;
            r->flags = subtract8(r->flags, end.placement.height_limit, 12) | CF;
            break;
        }
        publish_attachment(r, &end.placement.attachment);
        r->dx = (r->dx & 0xff00) | end.energy.model;
        r->di = r->dx;
        if (end.stage == 1) {
            struct ObjectDetachEnd *detached = &end.detached;
            r->bp = detached->cell.bank;
            r->di = detached->cell.offset;
            r->ax = (detached->cell.bank & 0xff00) | detached->value;
            r->flags = status(r->flags, detached->value, 8);
            if (detached->support < OBJECT_LINK_SUPPORT)
                r->flags |=
                    ((detached->height & 24) == 8 || (detached->height & 24) == 16 ? OF : 0);
        } else {
            r->ax = 128;
            r->bx = end.robot;
            r->flags = status(r->flags, 128, 8);
        }
        r->flags &= ~CF;
        break;
    }
    case TYPED_populate_preview_flow: {
        struct PopulationCheck check = populate_preview(segment_bytes(r->ds), r->dx, 0);
        r->flags = add8(r->flags, check.previous_flags, check.previous_flags);
        if (!(check.previous_flags & 128)) {
            r->flags = subtract8(r->flags, check.validation, 0x1e);
            if (check.accepted) {
                r->ax = segment_word(segment_bytes(frame >> 16), (uint16_t)frame + 20);
                r->cx = (uint16_t)abi_c_preview_key_gate;
                segment_word_store(segment_bytes(frame >> 16),
                    (uint16_t)frame + 20,
                    (uint16_t)abi_c_preview_key_gate);
            }
        }
        break;
    }
    case TYPED_generate_landscape_flow: {
        uint8_t tagged = generate_landscape(segment_bytes(r->ds), 0);
        r->flags = (subtract8(r->flags, 0, 1) & ~CF) | (tagged ? CF : 0);
        break;
    }
    case TYPED_c_populate_preview: {
        struct PreviewPopulateEnd end = c_populate_preview(segment_bytes(r->ds), r->dx);
        r->ax = end.value;
        r->bx = 127;
        r->dx = r->di = end.validation_index;
        r->bp = end.cell_bank;
        r->cx = 0;
        if (end.support_written)
            r->si = end.support_slot;
        r->flags = add8(r->flags, end.previous_generation_flags, end.previous_generation_flags);
        if (!(end.previous_generation_flags & 128)) {
            r->flags = subtract8(r->flags, (uint8_t)end.value, 0x1e);
            if (end.accepted) {
                r->ax = segment_word(segment_bytes(frame >> 16), (uint16_t)frame + 20);
                r->cx = (uint16_t)abi_c_preview_key_gate;
                segment_word_store(segment_bytes(frame >> 16),
                    (uint16_t)frame + 20,
                    (uint16_t)abi_c_preview_key_gate);
            }
        }
        break;
    }
    case TYPED_c_prepare_preview: {
        struct PreviewPrepareEnd end = c_prepare_preview(segment_bytes(r->ds));
        r->si = end.placement.lookup;
        r->bp = end.placement.bank;
        r->di = end.placement.offset;
        if (end.randomized) {
            r->cx = 0;
            r->dx = (r->dx & 0xff00) | end.random_offset;
        }
        if (end.placement.placed) {
            r->cx = 0;
            r->dx &= 0x7fff;
        }
        r->ax = end.variant;
        r->bx = end.placement.slot;
        r->flags = status(r->flags, end.variant, 8);
        if (end.colored) {
            r->bx = end.mode == 3 ? end.variant * sizeof(struct RgbColor) : end.variant;
            r->ax = end.mode == 3 ? end.color : (uint8_t)end.color;
            if (end.mode == 3)
                r->flags = add16(r->flags, end.variant * 2, end.variant);
        }
        break;
    }
    case TYPED_c_generate_landscape: {
        struct LandscapeGenerateEnd end = c_generate_landscape(segment_bytes(r->ds));
        r->ax = ((uint16_t)end.terrain.high << 8) | end.terrain.value;
        r->bx = (end.terrain.cell.bank & 0xff00) | end.terrain.nibble;
        r->cx = end.final_shape;
        r->si = 0xffff;
        r->bp = end.terrain.cell.bank;
        r->di = end.terrain.cell.offset;
        if (end.sampled)
            r->dx = (r->dx & 0xff00) | end.noise_sample;
        r->flags = (subtract8(r->flags, 0, 1) & ~CF) | (end.terrain.carry ? CF : 0);
        break;
    }
    case TYPED_c_pause_check: {
        struct PauseEnd end = c_pause_check(segment_bytes(r->ds), !!(r->flags & 0x400), r->dx);
        r->ax = (r->ax & 0xff00) | end.action;
        r->flags = status(r->flags, end.action, 8);
        if (!(end.action & 128))
            r->flags = subtract8(r->flags, end.action, 2);
        if (end.entered) {
            publish_warning(r, &end.before);
            r->bx = 255;
            r->si = 0;
            r->cx = (r->cx & 0xff00) | 5;
            r->ax = end.restored_mute;
            r->flags |= 0x200;
            publish_warning(r, &end.after);
            r->flags = status(r->flags, end.pause_flags, 8);
        }
        break;
    }
    case TYPED_c_place_principal_objects: {
        struct PrincipalPlacementEnd end = c_place_principal_objects(segment_bytes(r->ds));
        r->ax = end.value;
        r->bx = end.slot;
        r->si = end.lookup;
        r->bp = end.bank;
        r->di = end.offset;
        if (end.placed) {
            r->cx = 0;
            r->dx &= 0x7fff;
        }
        r->flags = status(r->flags, (uint8_t)end.value, 8) & ~CF;
        break;
    }
    case TYPED_c_install_keyboard: {
        struct KeyboardInstallEnd end = c_install_keyboard(r->ax, r->flags);
        r->ax = end.vector.handle;
        r->dx = end.offset;
        r->flags = end.vector.flags;
        if (end.configuration_available) {
            r->bx = end.configuration_offset;
            r->es = end.configuration_segment;
        }
        break;
    }
    case TYPED_c_capture_ega_frame: {
        SegmentBytes filename = segment_at_offset(segment_bytes(r->ds), (uint16_t)filename_sent);
        struct EgaCaptureEnd end = c_capture_ega_frame(filename, r->ax, r->flags);
        r->flags = end.flags;
        if (!end.stage)
            break;
        r->ax = end.value;
        r->dx = (uint16_t)filename_sent;
        r->cx = 0;
        if (end.stage >= 2) {
            r->bp = end.handle;
            r->bx = 0x7d0;
        }
        if (end.stage == 3) {
            r->ds = end.data_segment;
            r->ax = r->es = r->ds;
            r->bx = r->dx = 0;
            r->cx = 32000;
        }
        break;
    }
    case TYPED_c_trace_world_ray: {
        struct WorldRayEnd end = c_trace_world_ray(segment_bytes(r->ds), r->bx, r->dx, r->flags);
        r->ax = end.value;
        r->bx = end.observer_index;
        r->cx = end.height_work;
        r->dx = end.position_index;
        r->flags = end.flags;
        if (end.cell_sampled) {
            r->bp = end.cell_bank;
            r->di = end.cell_offset;
        }
        break;
    }
    case TYPED_c_prepare_selection_interval: {
        struct SelectionIntervalEnd end = c_prepare_selection_interval(
            segment_record(segment_bytes(r->ds), game_work).work_allocated_object_slot,
            r->bx,
            r->dx);
        publish_selection_interval(r, &end);
        break;
    }
    case TYPED_c_sentry_partial_change:
    case TYPED_c_sentry_guard_change: {
        struct SentryPartialEnd end =
            c_sentry_partial_change(segment_bytes(r->ds), r->ax, r->bx, r->dx);
        publish_sentry_partial(r, &end);
        if (entry == TYPED_c_sentry_guard_change && end.mode && end.interval.accepted) {
            struct StackReturnEnd resume = c_sentry_abort();
            ((uint16_t *)r)[-1] = typed_resume_legacy(resume.stack_pointer, resume.instruction);
        }
        break;
    }
    case TYPED_c_sentry_abort:
    case TYPED_c_sentry_player_absorbed:
    case TYPED_c_sentry_retry_after_redraw: {
        struct StackReturnEnd resume;
        if (entry == TYPED_c_sentry_retry_after_redraw) {
            struct SentryRetryEnd end = c_sentry_retry_after_redraw(r->bx);
            r->flags = (add8(r->flags, end.previous, 1) & ~CF) | (r->flags & CF);
            resume = end.resume;
        } else
            resume = entry == TYPED_c_sentry_abort ? c_sentry_abort() : c_sentry_player_absorbed();
        ((uint16_t *)r)[-1] = typed_resume_legacy(resume.stack_pointer, resume.instruction);
        break;
    }
    case TYPED_c_spans_tandy: {
        struct TandySpanEnd end = c_spans_tandy(r->ds, !!(r->flags & 0x400));
        r->ax = end.value;
        r->bx = r->dx = 0;
        r->flags = status(r->flags, 0, 16);
        r->cx = end.count_low_only ? (r->cx & 0xff00) | (uint8_t)end.fill.span_count
                                   : end.fill.span_count;
        if (end.fill.valid & 2)
            r->di = end.fill.screen_row;
        if (end.fill.valid & 4)
            r->si = end.fill.span_offset;
        if (end.fill.valid & 8)
            r->bp = end.fill.edge_work;
        if (end.data_output)
            r->es = r->ds;
        if (end.rows)
            ((uint16_t *)r)[-1] =
                typed_resume_legacy((uint16_t)frame + 20 + 4 * end.rows, (uint16_t)legacy_return);
        break;
    }
    case TYPED_c_probe_display_combination:
    case TYPED_c_probe_ega_configuration:
    case TYPED_c_probe_tandy_columns: {
        struct AdapterProbeEnd end;
        if (entry == TYPED_c_probe_display_combination)
            end = c_probe_display_combination(segment_bytes(r->ds), r->di, r->flags);
        else if (entry == TYPED_c_probe_ega_configuration)
            end = c_probe_ega_configuration(segment_bytes(r->ds), r->di, r->ax, r->flags);
        else
            end = c_probe_tandy_columns(segment_bytes(r->ds), r->di, r->bx, r->flags);
        r->ax = end.value;
        r->flags = end.flags;
        r->bx = (r->bx & ~end.index_mask) | end.index;
        if (end.switches_written)
            r->cx = end.switches;
        break;
    }
    case TYPED_c_probe_normalize: {
        struct AdapterNormalizeEnd end = c_probe_normalize(segment_bytes(r->ds), r->di, r->flags);
        r->flags = end.flags;
        if (end.queried) {
            r->ax = end.value;
            r->bx = (r->bx & 255) | (end.page << 8);
        }
        break;
    }
    case TYPED_c_detect_adapters: {
        struct AdapterDetectEnd end =
            c_detect_adapters(segment_bytes(r->ds), original_argument(frame, 0), r->bx, r->flags);
        r->ax = end.value;
        r->bx = end.index;
        r->flags = end.flags;
        r->cx = 0;
        if (end.port_written)
            r->dx = end.port;
        break;
    }
    case TYPED_c_scatter_dispatch:
    case TYPED_c_scatter_highlight:
    case TYPED_c_scatter_clear:
    case TYPED_c_scatter_repeated: {
        SegmentBytes data = segment_bytes(r->ds);
        struct ScatterDispatchEnd result;
        if (entry == TYPED_c_scatter_dispatch)
            result = c_scatter_dispatch(data, r->cx, r->bx);
        else if (entry == TYPED_c_scatter_highlight) {
            r->bx = (r->bx & 0xff00) | 14;
            r->cx = 50;
            result = c_scatter_highlight(data);
        } else {
            r->bx &= 0xff00;
            r->cx = 200;
            r->flags = status(r->flags, 0, 8);
            result = entry == TYPED_c_scatter_clear ? c_scatter_clear(data)
                                                    : c_scatter_repeated(data, r->ax);
        }
        if (result.executed) {
            struct ScatterEnd *end = &result.scatter;
            r->bp = end->noise_product_high;
            r->di = end->destination_offset;
            if (end->mask_updated)
                r->si = end->mask_index;
            if (end->interrupts_enabled)
                r->flags |= 0x200;
            r->ax = r->es = r->ds;
            r->bx = r->cx = r->dx = 0;
            r->flags = status(r->flags, 0, 16);
        }
        if (entry == TYPED_c_scatter_repeated)
            r->flags = status(r->flags, 0, 8);
        break;
    }
    case TYPED_c_scene_workspace: {
        if (r->flags & 0x400) {
            r->flags |= CF;
            break;
        }
        struct WorkspaceEnd end =
            c_scene_workspace(segment_bytes(r->ds), segment_bytes(r->es), !!(r->flags & 0x400));
        r->ax = end.y_min;
        r->cx = 0;
        r->dx = r->di = 1;
        r->flags = add8(r->flags, end.previous_noise, 1) & ~CF;
        break;
    }
    case TYPED_c_generation_palette: {
        struct PaletteEnd end =
            select_scene_palette(segment_bytes(r->ds), segment_bytes(r->es), 2, r->flags);
        publish_palette(r, &end);
        ((uint16_t *)r)[-1] = (uint16_t)&target_generation_call;
        break;
    }
    case TYPED_c_aggregate_sentry_warning: {
        struct WarningAggregateEnd end = c_aggregate_sentry_warning(r->bx);
        r->dx = r->di = end.warning;
        r->bx = end.last_slot;
        r->ax = (r->ax & 0xff00) | end.previous_mode;
        r->flags = subtract8(r->flags, end.warning, end.previous_mode);
        if (end.warning != end.previous_mode)
            r->flags = subtract8(r->flags, end.previous_mode, 3);
        if (end.silenced) {
            r->ax = end.control;
            r->cx = (r->cx & 0xff00) | 5;
            r->si = 32;
            r->flags = status(r->flags, end.control, 8) | 0x200;
        }
        break;
    }
    case TYPED_c_sentry_schedule_finish: {
        struct SentryClockEnd end = c_sentry_schedule_finish();
        r->ax = end.value;
        r->cx = 0;
        r->flags = (subtract8(r->flags, end.previous, 1) & ~CF) | (end.carry ? CF : 0);
        break;
    }
    case TYPED_c_hud_energy:
    case TYPED_c_hud_clear: {
        struct HudDrawEnd end = entry == TYPED_c_hud_energy
            ? c_hud_energy(segment_bytes(r->ds), !!(r->flags & 0x400))
            : c_hud_clear(segment_bytes(r->ds), !!(r->flags & 0x400));
        r->si = end.source;
        r->di = end.destination;
        r->cx = end.cga ? 0x2000 : 0;
        r->ax = r->es = r->ds;
        r->bx = r->dx = 0;
        if (end.cga)
            r->bp = 0x1fff;
        r->flags = status(r->flags, 0, 16) | (end.ega ? 0x200 : 0);
        break;
    }
    case TYPED_set_interrupt_vector: {
        uint16_t offset = original_argument(frame, 1);
        struct DosOpenEnd end = set_interrupt_vector(
            original_argument(frame, 0), offset, original_argument(frame, 2), r->flags);
        r->ax = end.handle;
        r->dx = offset;
        r->flags = end.flags;
        break;
    }
    case TYPED_c_mouse_initialize: {
        struct MouseSetupEnd end = c_mouse_initialize(r->bx, r->flags);
        r->ax = end.reply.value;
        r->bx = end.reply.handle;
        r->es = end.reply.extra_segment;
        r->flags = end.reply.flags;
        if (end.present) {
            r->cx = end.reply.count;
            r->dx = end.reply.offset;
        }
        break;
    }
    case TYPED_c_mouse_remove: {
        struct MouseSetupEnd end = c_mouse_remove(r->bx, r->dx, r->es, r->flags);
        r->flags = end.reply.flags;
        if (end.present) {
            r->ax = end.reply.value;
            r->bx = end.reply.handle;
            r->cx = end.reply.count;
            r->dx = end.reply.offset;
            r->es = end.reply.extra_segment;
        }
        break;
    }
    case TYPED_c_mouse_callback_body: {
        r->bx = c_mouse_callback_body(r->ax, r->bx, r->cx, r->dx);
        r->flags = subtract16(r->flags, 199, r->dx);
        ((uint16_t *)r)[-1] = (uint16_t)&target_mouse_callback_return;
        break;
    }
    case TYPED_c_crt_reserve_near: {
        uint16_t amount = r->ax;
        uint16_t saved_bp = r->bp;
        struct NearReserveEnd end = c_crt_reserve_near(r->ds, amount, r->di, r->flags);
        r->ax = saved_bp;
        r->bp = end.offset;
        r->dx = amount;
        r->flags = end.flags;
        if (end.resized) {
            r->cx = r->ds - crt_psp_segment;
            r->es = crt_psp_segment;
            r->bx = end.paragraphs;
        }
        break;
    }
    case TYPED_c_crt_prepare_environment: {
        struct EnvironmentEnd end = c_crt_prepare_environment(segment_bytes(r->ds), r->flags);
        if (end.reserve.resized)
            r->bx = end.reserve.paragraphs;
        r->ax = r->cx = 0;
        r->dx = end.amount;
        r->si = end.source_end;
        r->di = end.string_end;
        r->es = r->ds;
        r->flags = status(end.reserve.flags, 0, 8);
        break;
    }
    case TYPED_c_crt_prepare_argv: {
        c_crt_prepare_argv(segment_bytes(frame >> 16), (uint16_t)frame + 20);
        r->ds = r->es = frame >> 16;
        r->ax = r->cx = 0;
        ((uint16_t *)r)[-1] = (uint16_t)&target_crt_argument_handoff;
        break;
    }
    case TYPED_crt_write_message:
    case TYPED_c_crt_write_message: {
        uint16_t code = original_argument(frame, 0);
        struct CrtMessageEnd end = crt_write_message(segment_bytes(r->ds), code, r->flags);
        r->ax = end.value;
        r->dx = end.written ? end.message : code;
        r->es = r->ds;
        r->flags = end.flags;
        if (end.written) {
            r->bx = 2;
            r->cx = end.length;
        }
        if (entry == TYPED_c_crt_write_message) {
            struct StackReturnEnd resume =
                crt_discard_argument(segment_bytes(frame >> 16), (uint16_t)frame + 20);
            ((uint16_t *)r)[-1] = typed_resume_legacy(resume.stack_pointer, resume.instruction);
        }
        break;
    }
    case TYPED_c_crt_lookup_message: {
        uint16_t code = original_argument(frame, 0);
        struct StackReturnEnd resume =
            crt_discard_argument(segment_bytes(frame >> 16), (uint16_t)frame + 20);
        r->dx = code;
        r->ax = c_crt_lookup_message(code);
        r->es = r->ds;
        ((uint16_t *)r)[-1] = typed_resume_legacy(resume.stack_pointer, resume.instruction);
        break;
    }
    case TYPED_crt_discard_argument: {
        struct StackReturnEnd resume =
            crt_discard_argument(segment_bytes(frame >> 16), (uint16_t)frame + 20);
        ((uint16_t *)r)[-1] = typed_resume_legacy(resume.stack_pointer, resume.instruction);
        break;
    }
    case TYPED_c_crt_divide_error:
        c_crt_divide_error();
        break;
    case TYPED_c_crt_allocation_error:
        c_crt_allocation_error();
        break;
    case TYPED_c_bios_read_key: {
        struct KeyboardReadEnd end = c_bios_read_key(r->flags);
        r->ax = end.key;
        r->flags = end.flags;
        break;
    }
    case TYPED_c_menu_add: {
        uint16_t name = original_argument(frame, 1);
        struct MenuAddEnd end = c_menu_add(
            segment_bytes(r->es), original_argument(frame, 0), name, !!(r->flags & 0x400));
        r->dx = name;
        r->cx = end.remaining;
        r->bx = end.index * 2;
        r->ax = end.count == 4 ? 0 : 1;
        r->flags = r->ax ? subtract16(r->flags, end.count, 4) : status(r->flags, 0, 16);
        break;
    }
    case TYPED_menu_box:
    case TYPED_c_menu_frame:
    case TYPED_menu_option:
    case TYPED_c_menu_option:
    case TYPED_c_menu_options: {
        struct ConsoleTextEnd end;
        if (entry == TYPED_c_menu_options)
            end = c_menu_options(segment_bytes(r->ds), r->flags);
        else if (entry == TYPED_menu_option || entry == TYPED_c_menu_option)
            end = menu_option(segment_bytes(r->ds), original_argument(frame, 0), r->flags);
        else
            end = menu_box(segment_bytes(r->ds),
                original_argument(frame, 0),
                original_argument(frame, 1),
                original_argument(frame, 2),
                original_argument(frame, 3),
                r->flags);
        r->ax = end.value;
        r->flags = end.flags;
        r->bx = 0;
        r->dx = ((uint16_t)text_cursor_row << 8) | text_cursor_column;
        if (end.written)
            r->cx = end.count;
        break;
    }
    case TYPED_menu_fatal:
        menu_fatal(segment_bytes(r->ds), segment_bytes(r->es), r->dx, r->flags);
        break;
    case TYPED_c_load_scene_resource: {
        uint16_t decoder = original_argument(frame, 1);
        enum ResourceFormat format = decoder == (uint16_t)video_decode_cga ? RESOURCE_CGA
            : decoder == (uint16_t)video_decode_ega                        ? RESOURCE_EGA
                                                                           : RESOURCE_PACKED;
        struct ResourceLoadEnd end = c_load_scene_resource(segment_bytes(r->ds),
            segment_bytes(r->es),
            original_argument(frame, 0),
            (DecodeCallback)decoder,
            r->flags);
        r->ax = (uint32_t)end.buffer >> 16;
        r->bx = format == RESOURCE_CGA ? end.decode.last_color : end.handle;
        r->cx = 0;
        r->dx = end.decode.remaining;
        r->flags = end.flags;
        break;
    }
    case TYPED_graphics_memory_error:
        graphics_memory_error(segment_bytes(r->ds), r->dx);
        break;
    case TYPED_graphics_allocate: {
        struct DosMemoryEnd end = graphics_allocate(segment_bytes(r->ds), r->bx, r->dx, r->flags);
        r->ax = end.segment;
        r->bx = end.paragraphs;
        r->flags = end.flags;
        break;
    }
    case TYPED_initialize_graphics:
    case TYPED_c_init_cga:
    case TYPED_c_init_tandy:
    case TYPED_c_init_ega:
    case TYPED_c_init_vga: {
        unsigned mode = entry == TYPED_initialize_graphics ? r->ax : entry - TYPED_c_init_cga;
        struct GraphicsInitEnd end =
            initialize_graphics(segment_bytes(r->ds), segment_bytes(r->es), mode, r->flags);
        r->ax = end.mode.crtc_port;
        r->cx = 0;
        if (mode != 2) {
            r->bx = mode == 0 ? 0x400 : mode == 1 ? 0x800 : 0xffa;
            r->flags = add16(end.mode.flags, (uint16_t)frame + 16, 2);
        } else {
            r->ax = 7;
            r->dx = 0x3ce;
            r->flags = add16(end.mode.flags, 8000, 40);
        }
        if (mode == 0)
            r->dx = 16192;
        break;
    }
    case TYPED_finish_palette:
        r->bx = r->dx = 0;
        r->flags = finish_palette(r->flags);
        break;
    case TYPED_upload_palette: {
        struct PaletteUploadEnd end =
            upload_palette((RgbPaletteSource)(segment_bytes(r->es) + r->dx), r->bx, r->flags);
        r->ax = end.value;
        r->cx = 16;
        r->flags = end.flags;
        break;
    }
    case TYPED_attribute_palette:
    case TYPED_c_palette_cga:
    case TYPED_c_palette_tandy:
    case TYPED_c_palette_ega:
    case TYPED_c_palette_vga_ega:
    case TYPED_intensity_banks:
    case TYPED_c_palette_intensities:
    case TYPED_c_palette_intensity_continue:
    case TYPED_fixed_banks:
    case TYPED_c_palette_fixed_continue:
    case TYPED_c_palette_vga: {
        SegmentBytes output = segment_bytes(r->es);
        SegmentBytes data = segment_bytes(r->ds);
        struct PaletteEnd end;
        switch (entry) {
        case TYPED_attribute_palette:
            end = attribute_palette(output + r->dx, r->ax, r->flags);
            break;
        case TYPED_c_palette_cga:
            end = c_palette_cga(r->ds, r->ax, r->flags);
            break;
        case TYPED_c_palette_tandy:
            end = c_palette_tandy(output, r->ax, r->flags);
            break;
        case TYPED_c_palette_ega:
            end = c_palette_ega(output, r->ax, r->flags);
            break;
        case TYPED_c_palette_vga_ega:
            end = c_palette_vga_ega(output, r->ax, r->flags);
            break;
        case TYPED_intensity_banks:
        case TYPED_c_palette_intensity_continue:
            end = intensity_banks(output, r->bx, r->flags);
            break;
        case TYPED_c_palette_intensities:
            end = c_palette_intensities(data, output, r->flags);
            break;
        case TYPED_fixed_banks:
        case TYPED_c_palette_fixed_continue:
            end = fixed_banks((RgbPaletteSource)(output + r->dx), r->bx, r->flags);
            break;
        default:
            end = c_palette_vga(data, output, r->ax, r->flags);
            break;
        }
        publish_palette(r, &end);
        break;
    }
    case TYPED_bios_mode:
    case TYPED_c_bios_mode: {
        struct VideoModeSetEnd end =
            bios_mode(entry == TYPED_bios_mode ? r->ax : original_argument(frame, 0), r->flags);
        r->ax = end.crtc_port;
        r->flags = end.flags;
        break;
    }
    case TYPED_c_bios_get_mode: {
        struct VideoModeEnd end = c_bios_get_mode(r->flags);
        r->ax = end.mode;
        r->bx = (r->bx & 255) | (end.page << 8);
        r->flags = end.flags;
        break;
    }
    case TYPED_c_dos_open: {
        uint16_t filename = original_argument(frame, 0);
        struct DosOpenEnd end = c_dos_open(segment_at_offset(segment_bytes(r->ds), filename),
            original_argument(frame, 1),
            r->flags);
        r->ax = end.handle;
        r->dx = filename;
        r->flags = end.flags;
        break;
    }
    case TYPED_c_dos_close: {
        uint16_t handle = original_argument(frame, 0);
        struct DosOpenEnd end = c_dos_close(handle, r->ax, r->flags);
        r->ax = end.handle;
        r->bx = handle;
        r->flags = end.flags;
        break;
    }
    case TYPED_c_dos_size: {
        uint16_t handle = original_argument(frame, 0);
        struct DosSizeEnd end = c_dos_size(handle, r->flags);
        r->ax = end.low;
        r->dx = end.high;
        r->bx = handle;
        r->cx = 0;
        r->flags = end.flags;
        break;
    }
    case TYPED_dos_allocate:
    case TYPED_c_dos_alloc: {
        struct DosMemoryEnd end = dos_allocate(
            entry == TYPED_dos_allocate ? r->bx : original_argument(frame, 0), r->ax, r->flags);
        r->ax = end.segment;
        r->bx = end.paragraphs;
        r->flags = end.flags;
        break;
    }
    case TYPED_c_dos_free: {
        struct DosOpenEnd end = c_dos_free(original_argument(frame, 0), r->flags);
        r->ax = end.handle;
        r->flags = end.flags;
        break;
    }
    case TYPED_c_bios_hide_cursor: {
        struct DosOpenEnd end = c_bios_hide_cursor(r->ax, r->flags);
        r->ax = end.handle;
        r->cx = 0x2000;
        r->flags = end.flags;
        break;
    }
    case TYPED_console_position:
    case TYPED_c_bios_cursor: {
        uint16_t row = original_argument(frame, 0);
        uint16_t column = original_argument(frame, 1);
        struct DosOpenEnd end = console_position(row, column, r->flags);
        r->ax = end.handle;
        r->bx = 0;
        r->dx = ((row & 255) << 8) | (column & 255);
        r->flags = end.flags;
        break;
    }
    case TYPED_console_text:
    case TYPED_c_bios_text: {
        struct ConsoleTextEnd end =
            console_text(segment_bytes(r->ds) + original_argument(frame, 0), r->ax, r->flags);
        r->ax = end.value;
        r->flags = end.flags;
        r->bx = 0;
        r->dx = ((uint16_t)text_cursor_row << 8) | text_cursor_column;
        if (end.written)
            r->cx = end.count;
        break;
    }
    case TYPED_c_dos_print: {
        uint16_t start = original_argument(frame, 0);
        SegmentBytes scan_start = segment_bytes(r->es) + start;
        SegmentBytes terminator_start = segment_bytes(r->ds) + start;
        SegmentBytes display_start = segment_at_offset(segment_bytes(r->ds), start);
        struct DosPrintEnd end = c_dos_print(
            scan_start, terminator_start, display_start, !!(r->flags & 0x400), r->flags);
        r->ax = end.value;
        r->cx = end.remaining;
        r->dx = start;
        r->flags = end.flags;
        break;
    }
    case TYPED_c_polygon_subdivide:
    case TYPED_polygon_project_and_scan:
    case TYPED_c_polygon_triangle:
    case TYPED_c_polygon_build_spans: {
        SegmentBytes data = segment_bytes(r->ds);
        SegmentBytes y_spans = segment_bytes(r->es);
        struct PolygonScanEnd end;
        if (entry == TYPED_c_polygon_subdivide)
            end = c_polygon_subdivide(y_spans, data, r->bx, r->di, r->ax, r->flags);
        else if (entry == TYPED_polygon_project_and_scan)
            end = polygon_project_and_scan(data, y_spans, r->ax, r->bx, r->flags);
        else if (entry == TYPED_c_polygon_triangle)
            end = c_polygon_triangle(data, y_spans, r->ax, r->flags);
        else
            end = c_polygon_build_spans(data, y_spans, r->bx, r->flags);
        publish_polygon_scan(r, &end);
        break;
    }
    case TYPED_c_repaint_digit_field: {
        uint16_t saved_dx = r->dx;
        struct NumberTextEnd end =
            c_repaint_digit_field(segment_bytes(r->ds), segment_bytes(r->es), r->flags);
        publish_number_text(r, &end);
        r->dx = saved_dx;
        r->cx = 0;
        break;
    }
    case TYPED_c_decimal_character:
    case TYPED_c_decimal_digit:
    case TYPED_c_draw_decimal_pair:
    case TYPED_c_draw_landscape_bcd:
    case TYPED_c_completion_code_terrain: {
        SegmentBytes data = segment_bytes(r->ds);
        SegmentBytes output = segment_bytes(r->es);
        struct NumberTextEnd end;
        if (entry == TYPED_c_decimal_character)
            end = c_decimal_character(data, output, r->ax, r->flags);
        else if (entry == TYPED_c_decimal_digit)
            end = c_decimal_digit(data, output, r->ax, r->flags);
        else if (entry == TYPED_c_draw_decimal_pair)
            end = c_draw_decimal_pair(data, output, r->ax, r->flags);
        else if (entry == TYPED_c_draw_landscape_bcd)
            end = c_draw_landscape_bcd(data, output, r->ax, r->flags);
        else {
            end = c_completion_code_terrain(data, output, r->bx, r->flags);
            r->cx = 0;
        }
        publish_number_text(r, &end);
        break;
    }
    case TYPED_c_formatted_character: {
        struct FormattedCharacterEnd end =
            c_formatted_character(segment_bytes(r->ds), r->ax, r->flags);
        publish_text_character(r, &end.character);
        if (end.formatted)
            r->bx = 0xffff;
        break;
    }
    case TYPED_c_script_character:
    case TYPED_c_draw_text_script:
    case TYPED_c_script_inline_character: {
        struct TextRunEnd end;
        if (entry == TYPED_c_script_character)
            end = c_script_character(segment_bytes(r->ds), r->ax, r->flags);
        else if (entry == TYPED_c_draw_text_script) {
            if (r->cx >= TEXT_SCRIPT_COUNT) {
                r->flags |= CF;
                break;
            }
            end = c_draw_text_script(segment_bytes(r->ds), r->cx, r->ax, r->flags);
        } else
            end = c_script_inline_character(segment_bytes(r->ds), r->ax, r->flags, r->si);
        publish_text_character(r, &end.character);
        if (end.formatted)
            r->bx = 0xffff;
        if (end.script_run)
            r->bx = r->dx = 0;
        if (end.source_written)
            r->si = end.source;
        if (end.script_written)
            r->cx = end.nested_script;
        break;
    }
    case TYPED_c_text_character: {
        struct TextCharacterEnd end = c_text_character(segment_bytes(r->ds), r->ax, r->flags);
        publish_text_character(r, &end);
        break;
    }
    case TYPED_c_find_terrain_interval: {
        struct TerrainIntervalEnd end = c_find_terrain_interval(r->bx);
        publish_terrain_corner(r, &end.corner);
        r->dx = (r->dx & 0xff00) | end.column;
        r->ax = (r->ax & 0xff00) | end.scan;
        if (end.boundary)
            r->flags =
                (end.boundary == 1 ? status(r->flags, 0, 8) : subtract8(r->flags, 32, 32)) | CF;
        break;
    }
    case TYPED_terrain_corner_at: {
        struct TerrainCornerEnd end = terrain_corner_at(r->dx, r->bx);
        publish_terrain_corner(r, &end);
        break;
    }
    case TYPED_c_previous_terrain_corner:
    case TYPED_c_next_terrain_corner: {
        struct TerrainStepEnd end = entry == TYPED_c_previous_terrain_corner
            ? c_previous_terrain_corner(r->bx)
            : c_next_terrain_corner(r->bx);
        if (!end.boundary)
            publish_terrain_corner(r, &end.corner);
        else {
            r->dx = (r->dx & 0xff00) | end.column;
            r->flags =
                (end.boundary == 1 ? status(r->flags, 0, 8) : subtract8(r->flags, 32, 32)) | CF;
        }
        break;
    }
    case TYPED_c_project_terrain_corner: {
        struct TerrainCornerEnd end = c_project_terrain_corner(r->dx, r->bx);
        publish_terrain_corner(r, &end);
        break;
    }
    case TYPED_c_rebuild_visibility_bits: {
        struct VisibilityEnd end = c_rebuild_visibility_bits(
            segment_bytes(r->ds), segment_bytes(r->es), r->bx, r->dx, !!(r->flags & 0x400));
        r->flags = status(r->flags, end.outcome, 8);
        if (end.rebuilt) {
            r->ax = ((uint16_t)(uint8_t)~end.last_mask << 8) | end.last_value;
            r->cx = (uint16_t)end.last_old << 8;
            r->dx &= 0xff00;
            r->bx = 255;
            r->si = r->di = 0;
            r->bp = (uint16_t)occlusion_height_rows;
            r->flags = subtract8(r->flags, 0, 1) & ~CF;
        }
        break;
    }
    case TYPED_c_trace_occlusion_row: {
        struct OcclusionRayEnd end = c_trace_occlusion_row(segment_bytes(r->ds), r->bx, r->dx);
        r->bx = (r->bx & 0xff00) | player_robot_slot;
        r->dx = r->di = end.cache_index;
        r->ax = (r->ax & 0xff00) | (uint8_t)end.cache_index;
        if (end.sampled)
            r->bp = end.threshold_row;
        r->flags = subtract8(r->flags, 0, 1) & ~CF;
        break;
    }
    case TYPED_action_error: {
        uint8_t observed = action_error(segment_bytes(r->ds));
        r->ax = (r->ax & 0xff00) | 5;
        r->flags = status(r->flags, observed, 8) | 0x200 | CF;
        break;
    }
    case TYPED_action_refresh:
        action_refresh(r->ax);
        r->flags |= CF;
        break;
    case TYPED_c_build_occlusion_heights: {
        struct OcclusionHeightsEnd end = c_build_occlusion_heights(segment_bytes(r->ds), r->dx);
        r->ax = (end.sample.value & 0xff00) | end.threshold;
        r->dx &= 0xff00;
        r->bx = r->cx = 0;
        r->bp = (uint16_t)occlusion_height_rows - 256;
        r->si = 0xffff;
        r->di = 31;
        r->flags = status(r->flags, 0, 16);
        break;
    }
    case TYPED_c_sample_world_height: {
        struct WorldHeightEnd end = c_sample_world_height(r->dx);
        r->bp = end.bank;
        r->dx = (r->dx & 0xff00) | end.index;
        r->di = r->dx;
        r->ax = end.value;
        if (end.stack) {
            r->cx &= 0xff00;
            if (end.quarter_set)
                r->cx = (uint16_t)end.quarter << 8;
        }
        r->flags = end.operation == 1 ? add8(r->flags, end.flag_left, end.flag_right)
                                      : subtract8(r->flags, end.flag_left, end.flag_right);
        if (end.stack)
            r->flags &= ~CF;
        else
            r->flags ^= CF;
        break;
    }
    case TYPED_c_project_mesh_vertices: {
        if ((r->bx & 0xff00) || (r->dx & 0xff00)) {
            r->flags |= CF;
            break;
        }
        struct MeshVerticesEnd end = c_project_mesh_vertices();
        if (!end.valid) {
            r->flags |= CF;
            break;
        }
        r->si = bearing_table_index(&end.vertical.bearing);
        r->cx = end.vertical.pitch;
        r->ax = end.vertical.screen_y;
        r->bx = (r->bx & 0xff00) | view_object_slot;
        r->dx = r->di = end.next_vertex;
        r->flags = subtract8(r->flags, (uint8_t)end.next_vertex, model_element_end);
        break;
    }
    case TYPED_c_prepare_model_origin: {
        if ((r->bx & 0xff00) || view_object_slot >= 64 || r->di >= 64 || r->dx >= 64) {
            r->flags |= CF;
            break;
        }
        struct ModelOriginEnd end = c_prepare_model_origin(r->di, r->dx);
        struct BearingEnd bearing = bearing_observation(&end.bearing);
        publish_bearing(r, &bearing);
        r->cx = end.observer_height;
        r->bx = (r->bx & 0xff00) | view_object_slot;
        r->di = end.distance.table_index;
        r->flags = status(r->flags, end.preset, 8);
        if (!end.preset) {
            r->flags = subtract8(r->flags, (uint8_t)r->dx, 63);
            if (end.scaled)
                r->flags = add16(r->flags, end.height_before_bias, 0x70);
        }
        r->ax = end.height;
        break;
    }
    case TYPED_c_probe_color_crtc:
    case TYPED_c_probe_mono_crtc: {
        int mono = entry == TYPED_c_probe_mono_crtc;
        struct CrtcAdapterEnd end = mono ? c_probe_mono_crtc(segment_bytes(r->ds), r->di)
                                         : c_probe_color_crtc(segment_bytes(r->ds), r->di);
        r->dx = mono ? 0x3b5 : 0x3d5;
        r->cx = 0;
        r->ax = ((uint16_t)end.probe.observed << 8) | end.probe.saved;
        r->flags = subtract8(r->flags, end.probe.observed, 0x66);
        if (!end.accepted)
            r->flags |= CF;
        else {
            if (mono) {
                r->dx = 0x3ba;
                r->cx = end.remaining;
            }
            r->ax = adapter_descriptor_word(end.descriptor);
            r->bx = end.store.destination;
            r->flags = subtract8(r->flags, end.store.previous, 0);
        }
        break;
    }
    case TYPED_c_post_action_gate: {
        struct GameFlowEnd next = continue_post_action(r, 1);
        publish_game_flow(r, &next);
        break;
    }

    case TYPED_c_advance_melody: {
        if ((r->bx & 0xff00) || (!(sound_melody_cursor & 128) && sound_melody_cursor >= 96)) {
            r->flags |= CF;
            break;
        }
        struct MelodyStep end = c_advance_melody(r->bx & 0xff00);
        publish_melody(r, &end);
        break;
    }
    case TYPED_c_sound_triggers: {
        struct SoundTriggerEnd end = c_sound_triggers(segment_bytes(r->ds), r->bx & 0xff00);
        switch (end.event) {
        case SOUND_PENDING:
            r->flags = status(r->flags, end.payload.pending.countdown, 8);
            break;
        case SOUND_IDLE:
            r->ax = (r->ax & 0xff00) | end.payload.idle.mode;
            r->flags = subtract8(r->flags, end.payload.idle.mode, 6);
            break;
        case SOUND_FIXED_EFFECT:
            r->ax = (r->ax & 0xff00) | end.payload.fixed_effect.effect;
            r->flags = status(r->flags, end.payload.fixed_effect.sound_status, 8) | 0x200;
            break;
        case SOUND_MELODY:
            r->ax = (r->ax & 0xff00) | SOUND_MODE_MELODY;
            publish_melody(r, &end.payload.melody);
            break;
        case SOUND_RAMP_LIMIT:
            r->ax = (r->ax & 0xff00) | SOUND_MODE_RAMP;
            r->dx = (r->dx & 0xff00) | end.payload.ramp_limit.index;
            r->flags = subtract8(r->flags, end.payload.ramp_limit.index, 0xaa);
            break;
        case SOUND_RAMP_ADVANCED:
            r->ax = ((uint16_t)end.payload.ramp_advance.random_high << 8) |
                end.payload.ramp_advance.pitch;
            r->dx = (r->dx & 0xff00) | end.payload.ramp_advance.previous_index;
            r->cx = 0;
            r->flags = status(r->flags, end.payload.ramp_advance.pitch, 8) | 0x200;
            break;
        }
        break;
    }
    case TYPED_c_update_voices: {
        struct VoiceTickEnd end = c_update_voices();
        r->ax = (r->ax & ~end.sample_mask) | end.sample;
        r->cx = (r->cx & ~end.pitch_mask) | end.pitch;
        r->si = 0xffe0;
        r->flags = subtract16(r->flags, 0, 32);
        break;
    }

    case TYPED_c_keyboard_cursor_move: {
        SegmentBytes data = segment_bytes(r->ds);
        CursorDrawCallback draw = c_keyboard_cursor_move(data, r->bx & 0xff00);
        if (draw) {
            struct CursorDrawEnd result;
            call_video_cursor_draw(draw, data, r->bx & 0xff00, &result);
        }
        break;
    }
    case TYPED_c_input_check_draw_interrupt:
        c_input_check_draw_interrupt(
            segment_bytes(r->es), !!(r->flags & 0x400), (r->dx & 0xff00) | 7, r->bx & 0xff00);
        break;

    case TYPED_c_hud_append: {
        struct HudTileEnd result = c_hud_append(r->ax, segment_bytes(r->ds), !!(r->flags & 0x400));
        struct StatusTileEnd *end = &result.tile;
        unsigned mode = result.mode;
        if (mode == 0)
            r->flags =
                add16(r->flags, end->destination_offset - end->next_bank_step, end->next_bank_step);
        else if (mode == 1)
            r->flags = end->wrapped ? add16(r->flags, end->destination_offset - 0x80a0, 0x80a0)
                                    : status(r->flags, 0, 16);
        else if (mode == 2)
            r->flags = add16(r->flags, end->destination_offset - 40, 40) | 0x200;
        else
            r->flags = status(r->flags, 0, 8) | (end->destination_offset < 312 ? CF : 0);
        r->di = end->destination_offset;
        r->si = end->source_offset;
        r->cx = 0;
        if (mode == 0)
            r->bx = end->next_bank_step;
        else if (mode == 1) {
            r->bx = 0x8000;
            r->dx = 0x80a0;
        } else if (mode == 2) {
            r->bx = (result.number * 32 & 0xff00) | end->last_pixel;
            r->dx = 0x3c4;
        } else {
            r->bx = 312;
            r->dx = 0xf00;
        }
        r->es = r->ds;
        r->flags = (add8(r->flags, result.column, 1) & ~CF) | (r->flags & CF);
        break;
    }
    case TYPED_text_render_glyph: {
        struct TextGlyphEnd result;
        text_render_glyph(segment_bytes(r->ds), r->ax, !!(r->flags & 0x400), &result);
        struct GlyphRequest *request = &result.request;
        struct GlyphEnd *end = &result.glyph;
        unsigned mode = result.mode;
        r->ax = r->es = r->ds;
        if (mode == 0) {
            r->bx = end->last_background;
            r->dx = end->bank_step;
        } else if (mode == 1) {
            r->bx = end->packed_colors;
            r->dx = (r->dx & 0xff00) | (uint8_t)request->transparent;
        } else {
            r->bx = ((uint16_t)(uint8_t)request->background << 8) | (uint8_t)request->foreground;
            r->dx = mode == 2 ? 0 : 0x3ce;
        }
        r->cx = mode == 1 ? 0 : (uint16_t)(uint8_t)request->transparent << 8;
        r->flags = mode == 1
            ? (end->wrapped ? add16(r->flags, end->destination_offset - 0x80a0, 0x80a0)
                            : status(r->flags, 0, 16))
            : status(r->flags, 0, 8) | (end->carry ? CF : 0);
        break;
    }
    case TYPED_c_terrain_character: {
        if (r->flags & 0x400) {
            r->flags |= CF;
            break;
        }
        uint16_t saved = r->ax;
        struct TerrainGlyphEnd end = c_terrain_character(
            r->ax, segment_bytes(r->ds), segment_bytes(r->es), !!(r->flags & 0x400));
        if (end.control) {
            r->ax = (saved & 0x7f00) | end.coordinate;
            r->flags = status(r->flags, end.coordinate, 8) | ((saved & 64) ? CF : 0) |
                ((saved & 0x4000) ? OF : 0);
        } else {
            r->bx = end.pair;
            r->bp = end.bank;
            r->di = end.offset;
            r->dx &= 0xff00;
            r->flags = add8(r->flags, end.column, 4);
        }
        break;
    }

    case TYPED_c_adapter_name: {
        struct AdapterNameEnd end =
            c_adapter_name(segment_bytes(r->ds), original_argument(frame, 0));
        r->bx = end.table_offset;
        r->ax = end.name;
        break;
    }

    case TYPED_c_expand_input_panel: {
        struct InputPanelEnd end = c_expand_input_panel(!!(r->flags & 0x400));
        if (end.vga)
            r->bx = (r->bx & 0xff00) | 15;
        if (end.planar)
            r->dx = 0x3c4;
        r->si = end.source_end;
        r->di = end.destination_end;
        r->bp = r->cx = 0;
        r->ax = r->es = r->ds;
        r->flags = (subtract16(r->flags, 1, 1) & ~CF) | (end.carry ? CF : 0);
        break;
    }
    case TYPED_c_background_is_occupied: {
        struct BackgroundProbeEnd end = c_background_is_occupied(segment_bytes(r->ds));
        r->si = end.row;
        r->di = end.destination;
        if (end.planar) {
            r->dx = 0;
            r->flags = status(r->flags, 0, 16);
        } else {
            r->cx = end.alternate;
            r->flags = subtract16(r->flags, end.comparison_left, end.comparison_right);
        }
        r->ax = r->es = r->ds;
        r->flags = (r->flags & ~CF) | (end.occupied ? CF : 0);
        break;
    }
    case TYPED_c_present_hud: {
        struct HudPublishEnd end = c_present_hud(!!(r->flags & 0x400));
        if (end.copied) {
            r->si = end.source_end;
            r->di = end.destination_end;
            r->cx = end.cga ? 0x2000 : 0;
        }
        if (end.cga)
            r->bp = 0x1fff;
        r->ax = r->es = r->ds;
        r->bx = r->dx = 0;
        r->flags = status(r->flags, 0, 16);
        break;
    }

    case TYPED_c_transition_cga:
    case TYPED_c_transition_ega:
    case TYPED_c_transition_vga: {
        int backwards = !!(r->flags & 0x400);
        struct ViewTransitionEnd end = entry == TYPED_c_transition_cga
            ? c_transition_cga(segment_bytes(r->ds), backwards)
            : entry == TYPED_c_transition_ega ? c_transition_ega(segment_bytes(r->ds), backwards)
                                              : c_transition_vga(segment_bytes(r->ds), backwards);
        r->bp = end.bank_base;
        if (end.copied) {
            r->si = end.source_end;
            r->di = end.destination_end;
            r->cx = end.copy_count;
        }
        r->ax = r->es = r->ds;
        r->bx = r->dx = 0;
        r->flags = status(r->flags, 0, 16) | (end.interrupts_enabled ? 0x200 : 0);
        break;
    }
    case TYPED_c_copy_two_cga_bands:
    case TYPED_c_copy_two_four_bank_bands: {
        int backwards = !!(r->flags & 0x400);
        struct TwoBandCopyEnd end = entry == TYPED_c_copy_two_cga_bands
            ? c_copy_two_cga_bands(
                  segment_bytes(r->ds), segment_bytes(r->es), r->si, r->di, backwards)
            : c_copy_two_four_bank_bands(
                  segment_bytes(r->ds), segment_bytes(r->es), r->si, r->di, backwards);
        r->ax = end.second_source;
        r->bx = end.second_destination;
        r->bp = 0x1fff;
        r->cx = entry == TYPED_c_copy_two_cga_bands ? 0x2000 : 0x6000;
        r->si = end.copy.source_end;
        r->di = end.copy.destination_end;
        r->flags =
            status(r->flags, (uint16_t)(end.copy.destination_end + (backwards ? 2 : -2)), 16);
        break;
    }

    case TYPED_c_terrain_filter:
    case TYPED_c_terrain_smooth: {
        uint8_t mode = (uint8_t)r->ax | terrain_smoothing_flags;
        struct TerrainFilterEnd end = entry == TYPED_c_terrain_filter
            ? c_terrain_filter(
                  mode, mode & 0x80 ? terrain_scan_x : terrain_scan_y, segment_bytes(r->ds))
            : c_terrain_smooth(r->ax, segment_bytes(r->ds));
        r->bp = end.cell.bank;
        r->di = end.cell.offset;
        r->ax = (end.cell.bank & 0xff00) | end.value;
        r->si = 0xffff;
        if (end.work_pointer_set)
            r->bx = end.work_pointer;
        r->flags = subtract8(r->flags, end.cell.value.raw, 0xc0) ^ CF;
        r->flags = ((entry == TYPED_c_terrain_filter ? subtract16(r->flags, 0, 1)
                                                     : subtract8(r->flags, 1, 1)) &
                       ~CF) |
            (r->flags & CF);
        break;
    }
    case TYPED_c_terrain_transform: {
        uint8_t mode = r->ax;
        struct TerrainTransformEnd end = c_terrain_transform(mode);
        r->bp = end.cell.bank;
        r->di = end.cell.offset;
        r->ax = ((uint16_t)end.high << 8) | end.value;
        if (mode & 128)
            r->cx = 0;
        else if (mode && !(mode & 1))
            r->bx = (end.cell.bank & 0xff00) | end.nibble;
        r->flags = (subtract8(r->flags, 0, 1) & ~CF) | (end.carry ? CF : 0);
        break;
    }
    case TYPED_c_terrain_classify: {
        struct TerrainClassifyEnd end = c_terrain_classify(terrain_scan_x, terrain_scan_y);
        r->bp = end.cell.bank;
        r->di = end.cell.offset;
        r->bx = 15;
        r->ax = (end.cell.bank & 0xff00) | end.value;
        if (end.flat) {
            r->cx = 0;
            r->flags = status(r->flags, 0, 16);
        } else {
            r->cx = (r->cx & 0xff00) | end.shape;
            r->flags = subtract8(r->flags, end.value, end.comparison);
        }
        break;
    }

    case TYPED_c_polygon_rasterize_edge: {
        struct EdgeRasterEnd end =
            c_polygon_rasterize_edge(segment_bytes(r->es), segment_bytes(r->ds));
        if (!end.completed) {
            r->ax = (r->ax & 0xff00) | end.value;
            r->flags = end.compare_subtract
                ? subtract8(r->flags, end.compare_left, end.compare_right)
                : status(r->flags, end.compare_left, 8);
        } else {
            r->cx = end.count;
            if (end.x_major) {
                r->bp = end.error;
                r->si = end.remaining_y;
            }
            r->ax = end.value;
            r->di = end.destination;
            r->bx = r->dx = 0;
            r->flags = status(r->flags, 0, 16) & ~0x400;
        }
        break;
    }

    case TYPED_c_spans_cga:
    case TYPED_c_spans_ega:
    case TYPED_c_spans_vga: {
        struct SpanFillEnd end = entry == TYPED_c_spans_cga ? c_spans_cga(!!(r->flags & 0x400))
            : entry == TYPED_c_spans_ega                    ? c_spans_ega(!!(r->flags & 0x400))
                                                            : c_spans_vga(!!(r->flags & 0x400));
        if (end.valid & 1)
            r->cx = end.span_count;
        if (end.valid & 2)
            r->di = end.screen_row;
        if (end.valid & 4)
            r->si = end.span_offset;
        if (end.valid & 8)
            r->bp = end.edge_work;
        r->ax = r->es = r->ds;
        r->bx = r->dx = 0;
        r->flags = status(r->flags, 0, 16);
        break;
    }

    case TYPED_c_probe_store_adapter: {
        struct AdapterStoreEnd end =
            c_probe_store_adapter(segment_bytes(r->ds), r->di, adapter_descriptor_from_word(r->ax));
        r->bx = end.destination;
        r->flags = subtract8(r->flags, end.previous, 0);
        break;
    }
    case TYPED_c_probe_crtc_register: {
        struct CrtcProbeEnd end = c_probe_crtc_register(r->dx);
        ++r->dx;
        r->ax = ((uint16_t)end.observed << 8) | end.saved;
        r->cx = 0;
        r->flags = subtract8(r->flags, end.observed, 0x66);
        if (end.observed != 0x66)
            r->flags |= CF;
        break;
    }
    case TYPED_c_clip_view_index:
        if (r->di >= 2) {
            r->flags |= CF;
            break;
        }
        r->ax = (r->ax & 0xff00) | c_clip_view_index(r->di);
        break;
    case TYPED_c_clip_view_one:
        r->dx = r->di = 1;
        r->ax = (r->ax & 0xff00) | c_clip_view_one();
        break;
    case TYPED_c_clip_view_full:
        r->ax = c_clip_view_full();
        r->bx = r->dx = r->di = 0;
        r->flags = status(r->flags, 0, 16);
        break;
    case TYPED_c_clip_view_dynamic:
        r->ax = c_clip_view_dynamic();
        r->bx = 4;
        r->dx = r->di = 1;
        r->flags = status(r->flags, 0, 16);
        break;
    case TYPED_c_scene_workspace_final: {
        if (r->flags & 0x400) {
            r->flags |= CF;
            break;
        }
        struct WorkspaceEnd end =
            c_scene_workspace_final(segment_bytes(r->es), !!(r->flags & 0x400));
        r->cx = 0;
        r->ax = (r->ax & 0xff00) | end.y_min;
        r->dx = r->di = 1;
        r->flags = (add8(r->flags, end.previous_noise, 1) & ~CF) | (r->flags & CF);
        break;
    }
    case TYPED_c_pack_decimal_digits: {
        if (r->flags & 0x400) {
            r->flags |= CF;
            break;
        }
        struct DecimalPackEnd end =
            c_pack_decimal_digits(segment_bytes(r->ds), segment_bytes(r->es), !!(r->flags & 0x400));
        r->ax = end.packed;
        r->bx = end.last_low;
        r->cx = 0;
        r->si = end.source_end;
        r->di = end.destination_end;
        r->flags = status(r->flags, end.packed, 16);
        break;
    }
    case TYPED_c_keyboard_irq_scan:
    case TYPED_c_keyboard_bios_scan: {
        struct KeyboardScanEnd end = entry == TYPED_c_keyboard_irq_scan
            ? c_keyboard_irq_scan()
            : c_keyboard_bios_scan(r->ax);
        r->bx = end.index;
        if (!end.prefix)
            r->flags = status(r->flags, end.buttons, 8);
        ((uint16_t *)r)[-1] = entry == TYPED_c_keyboard_irq_scan
            ? (uint16_t)&target_keyboard_irq_return
            : (uint16_t)&target_bios_keyboard_return;
        break;
    }
    case TYPED_c_calibrate_pit: {
        struct PitCalibrationEnd end = c_calibrate_pit();
        r->dx = end.maximum;
        r->bx = end.last;
        r->ax = end.last >> 8;
        r->cx = r->si = 0;
        r->flags = status(r->flags, 0, 16) | 0x200 | (end.last < end.maximum ? CF : 0);
        break;
    }

    case TYPED_c_terrain_address: {
        struct TerrainCellAddress cell = c_terrain_address(terrain_scan_x, terrain_scan_y);
        r->bp = cell.bank;
        r->di = cell.offset;
        r->ax = (cell.bank & 0xff00) | cell.value.raw;
        r->flags = subtract8(r->flags, cell.value.raw, 0xc0) ^ CF;
        break;
    }
    case TYPED_c_ray_object_origin:
        r->ax = (r->ax & 0xff00) | c_ray_object_origin(r->bx);
        r->flags = status(r->flags, 0, 8);
        break;
    case TYPED_c_detach_object: {
        struct ObjectDetachEnd end = c_detach_object(r->bx, segment_bytes(r->ds));
        r->bp = end.cell.bank;
        r->di = end.cell.offset;
        r->ax = (end.cell.bank & 0xff00) | end.value;
        r->flags = status(r->flags, end.value, 8);
        if (end.support < OBJECT_LINK_SUPPORT)
            r->flags |= ((end.height & 16) ? CF : 0) |
                ((end.height & 24) == 8 || (end.height & 24) == 16 ? OF : 0);
        break;
    }
    case TYPED_c_attach_object: {
        struct ObjectAttachEnd end = c_attach_object(r->bx, segment_bytes(r->ds));
        publish_attachment(r, &end);
        break;
    }
    case TYPED_c_place_object_flat: {
        if (r->bx >= 64) {
            r->flags |= CF;
            break;
        }
        struct ObjectPlacementEnd end = c_place_object_flat(r->bx, r->ax, segment_bytes(r->ds));
        r->cx = 0;
        if (end.found) {
            publish_attachment(r, &end.attachment);
            r->flags &= ~CF;
        } else {
            r->bp = end.failure_cell.bank;
            r->di = end.failure_cell.offset;
            r->ax = (end.failure_cell.bank & 0xff00) | end.height_limit;
            r->flags = subtract8(r->flags, end.height_limit, 12) | CF;
        }
        break;
    }

    case TYPED_c_resource_word: {
        SegmentBytes data = segment_bytes(r->ds);
        struct ResourceReader reader = {r->bx, r->dx, data + r->si, r->flags, data};
        r->ax = c_resource_word(&reader);
        r->si = (uint16_t)((uint32_t)reader.cursor - (uint32_t)reader.data);
        r->dx = reader.remaining;
        r->flags = reader.flags;
        break;
    }
    case TYPED_c_decode_cga:
    case TYPED_c_decode_ega:
    case TYPED_c_decode_packed: {
        uint16_t handle = original_argument(frame, 1);
        SegmentBytes destination = segment_bytes(original_argument(frame, 0));
        struct ResourceDecodeEnd end = entry == TYPED_c_decode_cga
            ? c_decode_cga(handle, destination, segment_bytes(r->ds), r->flags)
            : entry == TYPED_c_decode_ega
            ? c_decode_ega(handle, destination, segment_bytes(r->ds), r->flags)
            : c_decode_packed(handle, destination, segment_bytes(r->ds), r->flags);
        r->ax = end.last_word;
        r->bx = entry == TYPED_c_decode_cga ? end.last_color : handle;
        r->cx = 0;
        r->dx = end.remaining;
        if (entry == TYPED_c_decode_cga)
            r->flags = status(r->flags, 0, 16) | CF;
        else if (entry == TYPED_c_decode_ega)
            r->flags = (add16(r->flags, 7999, 1) & ~CF) | (end.io_flags & CF);
        else
            r->flags = status(r->flags, 0, 16) | end.carry | AF;
        break;
    }

    case TYPED_c_ray_trig: {
        uint16_t angle = ((uint16_t)(uint8_t)r->ax << 8) | math_work_lo;
        struct RayTrigEnd end = c_ray_trig(angle, segment_bytes(r->ds));
        if (end.phase_used)
            r->cx = end.phase;
        r->cx = (r->cx & 0xff00) | end.angle;
        r->flags = status(r->flags, end.sign_test, 8);
        break;
    }
    case TYPED_c_ray_decode_component: {
        if (r->di >= 4 || (r->di & 1)) {
            r->flags |= CF;
            break;
        }
        uint16_t component = c_ray_decode_component(segment_bytes(r->ds), r->di);
        r->ax = component >> 4;
        r->flags = (status(r->flags, component >> 1, 16) & ~CF) | (component & 1) |
            ((component & 0x8000) ? OF : 0);
        if (component & 1) {
            r->flags = subtract16(r->flags, 0, r->ax);
            r->ax = -r->ax;
        }
        break;
    }
    case TYPED_c_ray_scale_component: {
        if (r->di >= 4 || (r->di & 1) || r->bx >= 6 || (r->bx & 1)) {
            r->flags |= CF;
            break;
        }
        struct EncodedProduct end = c_ray_scale_component(segment_bytes(r->ds), r->di, r->bx);
        r->ax = end.value;
        r->flags = status(r->flags, end.negative, 8);
        if (end.negative)
            r->flags = subtract16(r->flags, 0, end.magnitude);
        break;
    }
    case TYPED_c_ray_pitch_direction:
    case TYPED_c_build_cursor_ray: {
        if ((r->dx & 0xff00) || (r->bx & 0xff00) ||
            (entry == TYPED_c_build_cursor_ray && r->bx >= 64)) {
            r->flags |= CF;
            break;
        }
        struct RayDirectionEnd end = entry == TYPED_c_build_cursor_ray
            ? c_build_cursor_ray(r->bx, segment_bytes(r->ds))
            : c_ray_pitch_direction(
                  ((uint16_t)(uint8_t)r->ax << 8) | math_work_lo, segment_bytes(r->ds));
        if (end.pitch.phase_used)
            r->cx = end.pitch.phase;
        if (end.heading.phase_used)
            r->cx = end.heading.phase;
        r->cx = (r->cx & 0xff00) | end.heading.angle;
        r->dx = r->di = r->bx = 0;
        r->ax = end.x.value;
        r->flags = status(r->flags, end.x.negative, 8);
        if (end.x.negative)
            r->flags = subtract16(r->flags, 0, end.x.magnitude);
        break;
    }

    case TYPED_c_keyboard_cursor_horizontal:
        c_keyboard_cursor_horizontal();
        break;
    case TYPED_c_keyboard_cursor_vertical:
        c_keyboard_cursor_vertical(r->bx & 0xff00);
        break;
    case TYPED_c_input_snapshot:
        r->ax = (r->ax & 0xff00) | c_input_snapshot();
        break;
    case TYPED_c_input_ready:
        r->ax = (r->ax & 0xff00) | c_input_ready();
        break;
    case TYPED_c_center_cursor:
        r->ax = (r->ax & 0xff00) | c_center_cursor();
        break;
    case TYPED_c_reset_sentry_scan:
        c_reset_sentry_scan(r->bx);
        r->ax = (r->ax & 0xff00) | 128;
        break;
    case TYPED_c_collect_height_candidates: {
        struct HeightCandidatesEnd end = c_collect_height_candidates(
            placement_height_threshold, placement_region_heights, placement_candidates);
        c_publish_height_candidates(&end);
        r->bx = 255;
        r->si = end.count;
        r->ax = end.count;
        r->flags = status(r->flags, end.count, 8);
        if (!end.count)
            r->flags |= CF;
        else {
            r->si = end.bit;
            r->ax = end.mask;
            r->flags = add16(r->flags, end.bit - 1, 1) & ~CF;
        }
        break;
    }
    case TYPED_c_sample_placement: {
        struct PlacementSampleEnd end = c_sample_placement();
        r->bp = end.bank;
        r->di = end.offset;
        r->ax = (end.bank & 0xff00) | end.value;
        r->bx = 64;
        r->flags = subtract8(r->flags, 64, 64);
        break;
    }
    case TYPED_c_configure_preview_object: {
        if (r->di >= 3 || (uint8_t)r->ax >= 10) {
            r->flags |= CF;
            break;
        }
        uint8_t heading = c_configure_preview_object(r->ax, r->di);
        r->ax = (r->ax & 0xff00) | heading;
        r->bx = (r->bx & 0xff00) | 2;
        r->flags = status(r->flags, 0, 8);
        break;
    }
    case TYPED_c_ray_center_distance: {
        struct RayCenterDistance end =
            c_ray_center_distance(ray_position_x.fraction_high, ray_position_y.fraction_high);
        r->ax = (r->ax & 0xff00) | end.distance;
        r->flags = subtract8(r->flags, end.y, end.x);
        break;
    }
    case TYPED_c_text_newline: {
        uint16_t row = c_text_newline();
        r->flags = subtract16(r->flags, row, 25);
        if (row >= 25) {
            row = 0;
            r->flags = status(r->flags, 0, 16);
        }
        r->ax = row;
        break;
    }
    case TYPED_c_wait_video_retrace: {
        struct VideoRetraceEnd end = c_wait_video_retrace();
        r->dx = end.port;
        r->ax = (r->ax & 0xff00) | end.status;
        r->flags = status(r->flags, 0, 8);
        break;
    }
    case TYPED_c_transition_tandy: {
        uint8_t old = c_transition_tandy();
        r->flags = (subtract8(r->flags, old, 1) & ~CF) | (r->flags & CF);
        break;
    }

    case TYPED_c_speaker_pitch: {
        struct PitchDivision end = c_speaker_pitch(r->ax, r->cx);
        uint16_t flags = status(r->flags, 0, 8);
        if (end.octave > 0)
            flags = subtract8(r->flags, 1, 1);
        else if (end.octave < 0)
            flags = add8(r->flags, 255, 1);
        r->flags = (flags & ~CF) | end.carry;
        r->cx = end.divisor;
        r->ax = (end.divisor >> 8) | (end.divisor << 8);
        break;
    }
    case TYPED_c_sound_effect: {
        if ((int8_t)r->ax < 0 || (int8_t)r->ax >= 13) {
            r->flags |= CF;
            break;
        }
        uint8_t observed = c_sound_effect(r->ax);
        r->flags = status(r->flags, observed, 8) | 0x200;
        break;
    }
    case TYPED_c_silence_voice:
    case TYPED_c_first_voice:
    case TYPED_c_silence_all: {
        if (entry == TYPED_c_silence_voice && (uint8_t)r->bx >= 3) {
            r->flags |= CF;
            break;
        }
        uint8_t control;
        if (entry == TYPED_c_silence_all) {
            control = c_silence_all();
            r->bx = 255;
            r->si = 0;
        } else {
            if (entry == TYPED_c_first_voice)
                r->bx = 0;
            r->si = ((uint8_t)r->bx & 3) * 32;
            control = c_silence_voice(r->bx);
        }
        r->ax = control;
        r->cx = (r->cx & 0xff00) | 5;
        r->flags = status(r->flags, control, 8) | 0x200;
        if (entry == TYPED_c_silence_all)
            r->flags = (subtract8(r->flags, 0, 1) & ~CF) | (r->flags & CF);
        break;
    }

    case TYPED_c_text_attribute: {
        uint16_t background = original_argument(frame, 1);
        uint16_t foreground = original_argument(frame, 0);
        r->ax = c_text_attribute(foreground, background);
        r->flags = status(r->flags, (foreground & 15) | (uint8_t)(background << 4), 8);
        break;
    }
    case TYPED_c_string_length: {
        uint16_t start = original_argument(frame, 0);
        struct TextLengthEnd end =
            c_string_length(segment_bytes(r->es) + start, !!(r->flags & 0x400));
        r->ax = end.legacy_extent;
        r->dx = start;
        r->cx = end.remaining;
        r->flags = subtract16(r->flags, start + end.legacy_extent, start);
        break;
    }
    case TYPED_c_string_compare: {
        uint16_t left = original_argument(frame, 0);
        uint16_t right = original_argument(frame, 1);
        struct TextCompareEnd end = c_string_compare(
            segment_bytes(r->ds) + left, segment_bytes(r->ds) + right, !!(r->flags & 0x400));
        r->ax = (int16_t)(int8_t)(end.left - end.right);
        r->flags = subtract8(r->flags, end.left, end.right);
        break;
    }
    case TYPED_c_memory_fill: {
        uint16_t destination = original_argument(frame, 0);
        uint16_t value = original_argument(frame, 1);
        uint16_t count = original_argument(frame, 2);
        c_memory_fill(segment_bytes(r->es) + (destination), value, count, !!(r->flags & 0x400));
        r->ax = value;
        r->cx = 0;
        break;
    }
    case TYPED_c_memory_copy: {
        uint16_t destination = original_argument(frame, 0);
        uint16_t source = original_argument(frame, 1);
        uint16_t count = original_argument(frame, 2);
        c_memory_copy(segment_bytes(r->es) + (destination),
            segment_bytes(r->ds) + (source),
            count,
            !!(r->flags & 0x400));
        r->cx = 0;
        break;
    }

    case TYPED_c_model_planar_delta: {
        if (r->bx >= 64 || r->di >= 64) {
            r->flags |= CF;
            break;
        }
        struct PlanarDeltaEnd end = c_model_planar_delta(r->bx, r->di);
        r->ax = (r->ax & 0xff00) | end.absolute_y;
        r->flags = (int8_t)end.delta_y < 0 ? subtract8(r->flags, 0, end.delta_y)
                                           : subtract8(r->flags, end.object_y, end.observer_y);
        break;
    }
    case TYPED_c_model_height_delta: {
        if (r->bx >= 64 || r->di >= 64) {
            r->flags |= CF;
            break;
        }
        struct HeightDeltaEnd end = c_model_height_delta(r->bx, r->di);
        r->ax = end.object_height - end.observer_height;
        r->cx = end.observer_height;
        r->flags = subtract16(r->flags, end.object_height, end.observer_height);
        break;
    }
    case TYPED_c_multiply_bytes: {
        uint16_t product = (uint16_t)(uint8_t)r->ax * math_work_hi;
        math_work_lo = (uint8_t)product;
        r->ax = (product >> 8) | (product << 8);
        r->flags = (r->flags & ~(CF | OF)) | (product > 255 ? CF | OF : 0);
        break;
    }
    case TYPED_c_atan_equal:
        c_atan_equal();
        break;
    case TYPED_c_atan_octant: {
        struct OctantEnd end = c_atan_octant(r->si, r->di);
        r->flags = subtract16(r->flags, r->si, r->di);
        if (!end.equal) {
            r->si = end.table_index;
            r->ax = end.angle;
            r->flags = status(r->flags, r->si, 16);
        }
        break;
    }
    case TYPED_c_lookup_trig: {
        if ((r->bx & 0xff00) || (r->dx & 0xff00)) {
            r->flags |= CF;
            break;
        }
        struct TrigLookupEnd end = c_lookup_trig(r->ax);
        r->cx = (r->cx & 0xff00) | end.phase;
        r->dx = (r->dx & 0xff00) | end.reverse_index;
        r->di = r->dx;
        r->bx = (r->bx & 0xff00) | end.cosine;
        r->ax = ((uint16_t)end.heading << 8) | end.sine;
        r->flags = status(r->flags, end.heading & 0x40, 8);
        break;
    }
    case TYPED_c_multiply_encoded: {
        struct EncodedProduct end = c_multiply_encoded();
        r->ax = end.value;
        r->flags = status(r->flags, end.negative, 8);
        if (end.negative)
            r->flags = subtract16(r->flags, 0, end.magnitude);
        break;
    }
    case TYPED_c_negate_if_negative:
        if (!(r->flags & SF))
            break;
        /* Fall through: this entry differs only in its sign guard. */
    case TYPED_c_negate_work: {
        uint16_t work = ((uint16_t)(uint8_t)r->ax << 8) | math_work_lo;
        uint16_t negative = -work;
        math_work_lo = (uint8_t)negative;
        math_work_hi = negative >> 8;
        r->ax = (r->ax & 0xff00) | (negative >> 8);
        r->flags = subtract16(r->flags, 0, work);
        break;
    }
    case TYPED_c_bearing_y_major:
    case TYPED_c_bearing: {
        struct BearingEnd end;
        if (entry == TYPED_c_bearing) {
            struct BearingGeometry geometry = c_bearing(projection_abs_delta_x,
                projection_abs_delta_y,
                projection_delta_x_sign,
                projection_delta_y_sign);
            publish_projection_bearing(&geometry);
            end = bearing_observation(&geometry);
        } else
            end = c_bearing_y_major(r->si, r->di);
        publish_bearing(r, &end);
        break;
    }
    case TYPED_c_vertical_angle: {
        if (r->bx >= 64) {
            r->flags |= CF;
            break;
        }
        struct VerticalAngleEnd end =
            c_vertical_angle(projection_abs_delta_x, projection_distance, r->bx);
        publish_vertical_projection(&end);
        struct BearingEnd bearing = bearing_observation(&end.bearing);
        publish_bearing(r, &bearing);
        r->cx = end.pitch;
        r->ax = end.screen_y;
        r->flags = (status(r->flags, r->ax, 16) & ~CF) |
            (((uint16_t)(end.angle - end.pitch) & 8) ? CF : 0) |
            (subtract16(r->flags, end.angle, end.pitch) & AF);
        break;
    }
    case TYPED_c_planar_distance: {
        struct BearingGeometry geometry = {0};
        geometry.ratio = (uint16_t)projection_axis_ratio << 8;
        geometry.minor = projection_minor_axis;
        geometry.major = projection_major_axis;
        struct PlanarDistanceEnd end = c_planar_distance(&geometry);
        projection_distance = end.value;
        r->di = end.table_index;
        r->ax = end.product_low;
        r->flags = add16(r->flags, end.correction, end.major);
        break;
    }

    case TYPED_c_rows_cga:
    case TYPED_c_rows_four_bank:
    case TYPED_c_rows_planar:
    case TYPED_c_rows_linear: {
        enum VideoRowLayout layout = entry - TYPED_c_rows_cga;
        uint16_t count = layout < ROWS_PLANAR ? 200 : 201;
        uint16_t operand = video_row_offset(count - 1, layout);
        build_video_rows(segment_bytes(r->es), !!(r->flags & 0x400), layout);
        uint16_t increment;
        switch (layout) {
        case ROWS_CGA:
            increment = 80;
            break;
        case ROWS_FOUR_BANK:
            operand += 8192;
            increment = 0x80a0;
            break;
        case ROWS_PLANAR:
            increment = 40;
            break;
        default:
            increment = 320;
            break;
        }
        r->flags = add16(r->flags, operand, increment);
        r->ax = video_row_offset(count, layout);
        r->cx = 0;
        if (layout == ROWS_CGA)
            r->dx = video_row_offset(count + 1, layout);
        if (layout == ROWS_PLANAR) {
            r->ax = 7;
            r->dx = 0x3ce;
        }
        break;
    }
    case TYPED_c_viewport_preset: {
        uint16_t preset = r->ax;
        if (preset >= 3) {
            r->flags |= CF;
            break;
        }
        c_viewport_preset(preset);
        r->bx = preset * 2;
        r->ax = 0;
        r->flags = status(r->flags, 0, 16);
        break;
    }
    case TYPED_c_viewport_width: {
        uint8_t columns = r->ax;
        c_viewport_width(columns);
        r->cx = (r->cx & 0xff00) | columns;
        r->ax = 2;
        r->flags = status(r->flags, 0, 16);
        break;
    }

    case TYPED_c_blit_cga:
    case TYPED_c_blit_tandy:
    case TYPED_c_blit_ega:
    case TYPED_c_blit_vga: {
        unsigned mode = entry - TYPED_c_blit_cga;
        struct ImageSlice slice = {r->ax, r->si, r->cx, r->bp, !!(r->flags & 0x400)};
        struct ImageCopyEnd end;
        blit_image(&slice, &end, (enum VideoPixelLayout)mode);
        if (mode == 3)
            r->flags = add16(r->flags, end.destination_offset - end.row_tail, end.row_tail);
        else {
            unsigned carry = mode == 0 ? end.destination_offset < end.next_bank_step
                : mode == 1            ? end.wrapped
                                       : end.destination_offset < end.row_tail;
            r->flags = status(r->flags, 0, 16) | (carry ? CF : 0);
        }
        r->bx = mode == 0 ? end.next_bank_step : mode == 1 ? 0x8000 : mode == 2 ? 0 : 0xa00f;
        r->dx = mode == 0 ? end.bank_step : mode == 1 ? 0 : mode == 2 ? 0x3c4 : 160;
        r->ax = r->ds;
        r->es = r->ds;
        r->di = end.destination_offset;
        r->si = end.source_offset;
        r->cx = mode == 3 ? 0 : end.row_tail;
        if (mode == 1 || mode == 3)
            r->bp = end.word_width;
        break;
    }
    case TYPED_c_background_cga:
    case TYPED_c_background_tandy:
    case TYPED_c_background_ega:
    case TYPED_c_background_vga: {
        unsigned mode = entry - TYPED_c_background_cga;
        struct BackgroundEnd end;
        fill_background(r->dx, r->cx, !!(r->flags & 0x400), &end, (enum VideoPixelLayout)mode);
        r->di = end.destination_offset;
        r->ax = r->ds;
        r->es = r->ds;
        r->cx = end.word_width;
        r->bx = r->dx = 0;
        r->flags = status(r->flags, 0, 16);
        if (mode == 0) {
            r->si = end.other_step;
            r->bp = end.second_pattern;
        } else if (mode == 1)
            r->si = 0x80a0;
        else if (mode == 2) {
            r->si = 0;
            r->bp = end.word_width;
        } else
            r->si = end.word_width;
        break;
    }
    case TYPED_c_front_cga:
    case TYPED_c_front_tandy:
    case TYPED_c_front_ega:
    case TYPED_c_front_vga: {
        unsigned mode = entry - TYPED_c_front_cga;
        front_buffer((enum VideoPixelLayout)mode);
        r->ax &= 0xff00;
        r->cx = r->dx = 0;
        r->flags = status(r->flags, 0, 16);
        break;
    }
    case TYPED_c_work_segment:
        r->ax = c_work_segment();
        break;
    case TYPED_c_work_ega:
        c_work_ega();
        break;
    case TYPED_c_tile_cga:
    case TYPED_c_tile_tandy:
    case TYPED_c_tile_ega:
    case TYPED_c_tile_vga: {
        unsigned mode = entry - TYPED_c_tile_cga;
        uint16_t tile = r->ax;
        struct StatusTileEnd end;
        status_tile(tile,
            r->bx,
            segment_bytes(r->ds),
            !!(r->flags & 0x400),
            &end,
            (enum VideoPixelLayout)mode);
        if (!end.valid) {
            r->flags |= CF;
            break;
        }
        if (mode == 0)
            r->flags =
                add16(r->flags, end.destination_offset - end.next_bank_step, end.next_bank_step);
        else if (mode == 1)
            r->flags = end.wrapped ? add16(r->flags, end.destination_offset - 0x80a0, 0x80a0)
                                   : status(r->flags, 0, 16);
        else if (mode == 2)
            r->flags = add16(r->flags, end.destination_offset - 40, 40);
        else
            r->flags = status(r->flags, 0, 8) | (end.destination_offset < 312 ? CF : 0);
        r->di = end.destination_offset;
        r->si = end.source_offset;
        r->cx = 0;
        if (mode == 0) {
            r->ax = end.bank_step;
            r->bx = end.next_bank_step;
            r->es = video_work_buffer_segment;
        } else if (mode == 1) {
            r->ax = 0x1ffc;
            r->bx = 0x8000;
            r->dx = 0x80a0;
            r->es = 0xb000;
        } else if (mode == 2) {
            r->ax = 0x0f02;
            r->bx = (tile * 32 & 0xff00) | end.last_pixel;
            r->dx = 0x3c4;
            r->es = 0xa000;
            r->flags |= 0x200;
        } else {
            r->ax = end.pattern;
            r->bx = 312;
            r->dx = 0xf00;
            r->es = video_work_buffer_segment;
        }
        break;
    }

    case TYPED_c_font_cga:
    case TYPED_c_font_packed:
    case TYPED_c_font_vga:
    case TYPED_c_font_ega: {
        unsigned mode = entry - TYPED_c_font_cga;
        struct GlyphRequest request;
        unsigned argument;
        for (argument = 0; argument < 7; ++argument)
            ((uint16_t *)&request)[argument] = original_argument(frame, argument);
        if (!glyph_request_valid(&request)) {
            r->flags |= CF;
            break;
        }
        prepare_glyph_request(segment_bytes(r->ds), &request, mode);
        if (mode == 0) {
            original_argument_store(frame, 3, request.foreground);
            original_argument_store(frame, 4, request.background);
        }
        struct GlyphEnd end;
        font_glyph(
            segment_bytes(r->ds), &request, !!(r->flags & 0x400), &end, (enum GlyphLayout)mode);
        r->ax = r->es = r->ds;
        if (mode == 0) {
            r->bx = end.last_background;
            r->dx = end.bank_step;
        } else if (mode == 1) {
            r->bx = end.packed_colors;
            r->dx = (r->dx & 0xff00) | (uint8_t)request.transparent;
        } else {
            r->bx = ((uint16_t)(uint8_t)request.background << 8) | (uint8_t)request.foreground;
            r->dx = mode == 2 ? 0 : 0x3ce;
        }
        r->cx = mode == 1 ? 0 : (uint16_t)(uint8_t)request.transparent << 8;
        r->flags = mode == 1
            ? (end.wrapped ? add16(r->flags, end.destination_offset - 0x80a0, 0x80a0)
                           : status(r->flags, 0, 16))
            : status(r->flags, 0, 8) | (end.carry ? CF : 0);
        break;
    }

    case TYPED_c_test_scan: {
        uint8_t state = c_test_scan(r->bx);
        r->bx = (r->bx & 0xff00) | state;
        r->flags = subtract8(r->flags, state, 255);
        break;
    }
    case TYPED_c_clear_pending:
        if (r->flags & 0x400) {
            r->flags |= CF;
            break;
        }
        r->di = (uint16_t)(uint32_t)c_clear_pending(segment_bytes(r->es), !!(r->flags & 0x400));
        r->cx = r->ax = 0;
        r->flags = status(r->flags, 0, 16);
        break;
    case TYPED_c_mouse_edge_actions: {
        uint16_t y = c_mouse_edge_actions();
        r->ax = (r->ax & 0xff00) | 128;
        r->flags = subtract16(r->flags, y, 12);
        break;
    }
    case TYPED_c_sentry_countdowns: {
        struct CountdownTick tick = c_sentry_countdowns();
        r->flags = add8(r->flags, tick.previous_phase, 0xa1);
        if (!(r->flags & CF)) {
            r->bx = 255;
            r->flags = (subtract8(r->flags, 0, 1) & ~CF) | (tick.last_counter < 2 ? CF : 0);
        }
        break;
    }
    case TYPED_c_collect_actions: {
        struct ActionScanEnd end;
        c_collect_actions(segment_bytes(r->es), !!(r->flags & 0x400), r->dx, r->bx & 0xff00, &end);
        r->di = end.binding_end;
        r->bx = end.last_slot;
        r->cx = 0;
        r->ax = (r->ax & 0xff00) | end.combined_action;
        r->flags = status(r->flags, end.combined_action, 8);
        break;
    }

    case TYPED_c_random_byte:
    case TYPED_c_random_alias: {
        struct GameRandomStep step = game_random_next(&prng_state_low);
        uint16_t before = step.upper_input;
        uint16_t after = prng_state_low.high;
        r->flags = status(r->flags, after, 16);
        if (before & 0x8000)
            r->flags |= CF;
        if ((before ^ after) & 0x8000)
            r->flags |= OF;
        if ((before & 15) * 2 + step.upper_carry > 15)
            r->flags |= AF;
        r->ax = ((uint16_t)prng_state_low.middle << 8) | step.value;
        r->cx = 0;
        break;
    }
    case TYPED_c_random_decimal_pair: {
        uint8_t value = c_random_decimal_pair();
        r->ax = ((uint16_t)prng_state_low.middle << 8) | value;
        r->cx = 0;
        r->flags = status(r->flags, value, 8);
        break;
    }
    case TYPED_c_random_small_sum: {
        uint8_t sample;
        uint8_t value = c_random_small_sum(&sample);
        r->ax = ((uint16_t)(sample & 7) << 8) | value;
        r->dx = (r->dx & 0xff00) | sample;
        r->cx = 0;
        r->flags = add8(r->flags, sample & 7, (sample >> 3) & 15);
        break;
    }
    case TYPED_c_random_grid_coordinate: {
        uint8_t value = c_random_grid_coordinate();
        r->ax = ((uint16_t)prng_state_low.middle << 8) | value;
        r->cx = 0;
        r->flags = subtract8(r->flags, value, 31);
        break;
    }
    case TYPED_c_select_landscape: {
        union LandscapeNumber landscape = {.raw = r->bx};
        uint8_t difficulty = c_select_landscape(landscape);
        r->bx = (landscape.raw & 0xff00) | difficulty;
        r->flags = landscape.pairs.high.raw
            ? status(r->flags, landscape.pairs.high.raw, 8)
            : subtract8(r->flags, landscape.pairs.low.digits.tens + 1, 9);
        break;
    }
    case TYPED_c_random_sentry_count: {
        uint8_t offset;
        uint8_t count = c_random_sentry_count(&offset);
        r->ax = ((uint16_t)prng_state_low.middle << 8) | count;
        r->dx = (r->dx & 0xff00) | offset;
        r->cx = 0;
        r->flags = status(r->flags, count, 8) | CF;
        break;
    }
    case TYPED_c_energy_bcd: {
        uint8_t value = c_energy_bcd();
        r->ax = (r->ax & 0xff00) | value;
        r->cx = 0;
        r->flags = status(r->flags, value, 8);
        if (player_energy && player_energy % 10 == 0)
            r->flags |= AF;
        break;
    }
    case TYPED_c_start_melody:
        c_start_melody((uint8_t)r->ax);
        break;
    case TYPED_c_allocate_saved:
    case TYPED_c_allocate_model: {
        int slot =
            entry == TYPED_c_allocate_model ? c_allocate_model((uint8_t)r->ax) : c_allocate_saved();
        if (slot < 0) {
            r->bx = 255;
            r->ax = (r->ax & 0xff00) | object_stack_links[0];
            r->flags = (r->flags & ~ARITH_FLAGS) | CF | PF | AF | SF;
        } else {
            r->bx = slot;
            r->ax = (r->ax & 0xff00) | pending_object_model;
            r->flags = status(r->flags, object_stack_links[slot], 8);
        }
        break;
    }
    case TYPED_c_adjust_energy: {
        struct EnergyChange change;
        int accepted = c_adjust_energy(r->bx, !!(r->flags & CF), &change);
        r->dx = (r->dx & 0xff00) | change.model;
        r->di = r->dx;
        if (accepted) {
            r->ax = (r->ax & 0xff00) | change.after;
            r->flags = status(r->flags, change.after, 8);
        } else {
            r->ax = (r->ax & 0xff00) | (uint8_t)(change.before - change.cost);
            r->flags = subtract8(r->flags, change.before, change.cost) | CF;
        }
        break;
    }
    case TYPED_c_copy_cga_strip:
    case TYPED_c_copy_four_bank_strip: {
        uint16_t source = r->si;
        uint16_t destination = r->di;
        int backwards = !!(r->flags & 0x400);
        struct GameStripCopy end;
        if (entry == TYPED_c_copy_cga_strip)
            end = c_copy_cga_strip(
                segment_bytes(r->ds), segment_bytes(r->es), source, destination, backwards);
        else
            end = c_copy_four_bank_strip(
                segment_bytes(r->ds), segment_bytes(r->es), source, destination, backwards);
        r->ax = source;
        r->bx = destination;
        r->bp = 0x1fff;
        r->cx = entry == TYPED_c_copy_cga_strip ? 0x2000 : 0x6000;
        r->si = end.source_end;
        r->di = end.destination_end;
        r->flags = status(r->flags, (uint16_t)(end.destination_end + (backwards ? 2 : -2)), 16);
        break;
    }

    case TYPED_c_darken_palette: {
        struct PaletteFade fade = c_darken_palette();
        r->ax = ((uint16_t)fade.remainder << 8) | fade.last_value;
        r->bx = (r->bx & 0xff00) | fade.decrement;
        r->cx = 0;
        r->dx = 16;
        r->si = (uint16_t)&vga_work_palette + sizeof(struct RgbPalette);
        r->di = r->si;
        r->flags = subtract16(r->flags, 16, 16);
        break;
    }
    case TYPED_c_colors_cga:
    case TYPED_c_colors_tandy:
    case TYPED_c_colors_ega:
    case TYPED_c_colors_vga: {
        unsigned mode = entry - TYPED_c_colors_cga;
        uint16_t variant = r->ax;
        uint16_t value;
        switch (mode) {
        case 0:
            value = c_colors_cga(variant);
            break;
        case 1:
            value = c_colors_tandy(variant);
            break;
        case 2:
            value = c_colors_ega(variant);
            break;
        default:
            value = c_colors_vga(variant);
            break;
        }
        r->bx = mode == 3 ? variant * sizeof(struct RgbColor) : variant;
        r->ax = mode == 3 ? value : (r->ax & 0xff00) | (uint8_t)value;
        if (mode == 3)
            r->flags = add16(r->flags, variant * 2, variant);
        break;
    }

    case TYPED_c_blit_title_image:
    case TYPED_c_blit_skybox_image:
    case TYPED_c_blit_landscape_panel:
    case TYPED_c_blit_prompt_header:
    case TYPED_c_blit_prompt_background: {
        enum RenderPanel panel = entry - TYPED_c_blit_title_image;
        int backwards = !!(r->flags & 0x400);
        if (panel == RENDER_TITLE || panel == RENDER_SKYBOX)
            r->flags = status(r->flags, 0, 16);
        struct ImageCopyEnd end;
        unsigned mode = c_blit_panel(panel, backwards, &end);
        if (mode == 3)
            r->flags = add16(r->flags, end.destination_offset - end.row_tail, end.row_tail);
        else {
            unsigned carry = mode == 0 ? end.destination_offset < end.next_bank_step
                : mode == 1            ? end.wrapped
                                       : end.destination_offset < end.row_tail;
            r->flags = status(r->flags, 0, 16) | (carry ? CF : 0);
        }
        r->bx = mode == 0 ? end.next_bank_step : mode == 1 ? 0x8000 : mode == 2 ? 0 : 0xa00f;
        r->dx = mode == 0 ? end.bank_step : mode == 1 ? 0 : mode == 2 ? 0x3c4 : 160;
        r->ax = r->es = r->ds;
        r->di = end.destination_offset;
        r->si = end.source_offset;
        r->cx = mode == 3 ? 0 : end.row_tail;
        r->bp = end.word_width;
        break;
    }

    case TYPED_c_scatter_cga:
    case TYPED_c_scatter_tandy:
    case TYPED_c_scatter_ega:
    case TYPED_c_scatter_vga: {
        unsigned mode = entry - TYPED_c_scatter_cga;
        struct ScatterEnd end;
        scatter_pixels(r->cx, (uint8_t)r->bx, &end, (enum VideoPixelLayout)mode);
        r->bp = end.noise_product_high;
        r->di = end.destination_offset;
        if (end.mask_updated)
            r->si = end.mask_index;
        if (end.interrupts_enabled)
            r->flags |= 0x200;
        r->ax = r->es = r->ds;
        r->bx = r->cx = r->dx = 0;
        r->flags = status(r->flags, 0, 16);
        break;
    }

    case TYPED_c_restore_cursor_cga:
    case TYPED_c_restore_cursor_tandy:
    case TYPED_c_restore_cursor_ega:
    case TYPED_c_restore_cursor_vga: {
        unsigned mode = entry - TYPED_c_restore_cursor_cga;
        struct CursorRestoreEnd end;
        restore_cursor(segment_bytes(r->ds), r->bx & 0xff00, &end, (enum VideoPixelLayout)mode);
        if (end.restored) {
            r->di = end.destination_offset;
            if (mode == 2)
                r->cx = (r->cx & 0xff00) | end.plane_value;
        }
        r->bx = 0;
        if (mode == 2)
            r->dx = 0;
        r->ax = r->es = r->ds;
        r->flags = status(r->flags, 0, 16);
        break;
    }
    case TYPED_c_draw_cursor_cga:
    case TYPED_c_draw_cursor_tandy:
    case TYPED_c_draw_cursor_ega:
    case TYPED_c_draw_cursor_vga: {
        unsigned mode = entry - TYPED_c_draw_cursor_cga;
        struct CursorDrawEnd end;
        draw_cursor(segment_bytes(r->ds), r->bx & 0xff00, &end, (enum VideoPixelLayout)mode);
        r->flags = status(r->flags, end.skip_status, 8);
        if (end.executed) {
            r->ax = r->es = r->ds;
            r->bx = end.terminal_step;
            r->bp = end.row_offset;
            r->cx = end.x_pairs;
            if (end.destination_updated)
                r->di = end.destination_offset;
            if (end.mask_updated)
                r->si = end.mask_index;
            r->flags = subtract8(r->flags, end.saved_count, 1) & ~CF;
            if (mode == 1)
                r->dx = (r->dx & 0xff00) | 0xee;
        }
        if (mode >= 2) {
            r->dx = 0;
            r->flags = status(r->flags, 0, 16);
        }
        break;
    }

    case TYPED_c_selection_copy_cga:
    case TYPED_c_selection_copy_tandy:
    case TYPED_c_selection_copy_ega:
    case TYPED_c_selection_copy_vga: {
        unsigned mode = entry - TYPED_c_selection_copy_cga;
        uint16_t columns = r->ax;
        struct GameStripCopy end;
        end =
            copy_selection_band(columns, r->bx, !!(r->flags & 0x400), (enum VideoPixelLayout)mode);
        if (mode == 0) {
            r->ax = 0x2000;
            r->bp = 0x1fff;
        } else {
            r->ax = 0;
            if (mode == 1)
                r->bp = 0x2000 - columns * 4;
        }
        r->si = end.source_end;
        r->di = end.destination_end;
        r->es = r->ds;
        r->bx = r->cx = r->dx = 0;
        r->flags = status(r->flags, 0, 16);
        break;
    }

    case TYPED_c_mouse_view_edges:
        c_mouse_view_edges((r->bx & 0xff00) | player_robot_slot);
        break;
    case TYPED_c_sync_software_cursor: {
        struct CursorSync state = c_sync_software_cursor();
        r->si = (r->bx & 0xff00) | state.transition_index;
        r->ax = (r->ax & 0xff00) | state.inverted_y;
        r->bx = 0;
        r->flags = status(r->flags, 0, 16);
        break;
    }
    case TYPED_c_empty_entry:
        break;
    case TYPED_c_carry_failure:
        r->flags |= CF;
        break;
    case TYPED_c_numeric_character: {
        uint8_t digit = r->ax;
        r->ax = (r->ax & 0xff00) | (uint8_t)(digit + 48);
        r->flags = add8(r->flags, digit, 48);
        {
            struct NumberTextEnd end =
                c_decimal_character(segment_bytes(r->ds), segment_bytes(r->es), r->ax, r->flags);
            publish_number_text(r, &end);
        }
        break;
    }

    case TYPED_c_warning_cga:
    case TYPED_c_warning_tandy:
    case TYPED_c_warning_ega:
    case TYPED_c_warning_vga: {
        unsigned mode = entry - TYPED_c_warning_cga;
        struct WarningEnd end;
        warning_bar(
            segment_bytes(r->ds), !!(r->flags & 0x400), r->dx, &end, (enum VideoPixelLayout)mode);
        r->flags = status(r->flags, end.skip_status, 8);
        if (end.completed) {
            if (end.drawn) {
                r->di = end.destination_offset;
                r->bp = end.last_noise;
                if (mode == 0) {
                    r->si = end.pattern;
                    r->bx = end.pattern_index * 2;
                    r->cx = 0x1fff;
                } else if (mode == 2) {
                    r->si = end.pattern;
                    r->bx = end.pattern_index;
                    r->cx = 0;
                } else {
                    r->si = end.source_offset;
                    r->bx = end.pattern;
                    r->cx = 0;
                }
            }
            r->dx = 0;
            r->ax = r->es = r->ds;
            r->flags = status(r->flags, 0, 16);
        }
        break;
    }
    case TYPED_c_reveal_cga:
    case TYPED_c_reveal_tandy:
    case TYPED_c_reveal_ega:
    case TYPED_c_reveal_vga: {
        unsigned mode = entry - TYPED_c_reveal_cga;
        struct RevealEnd end;
        reveal_selection(!!(r->flags & 0x400), &end, (enum VideoPixelLayout)mode);
        r->si = end.source_offset;
        r->di = end.destination_offset;
        r->bp = r->ds;
        r->cx = end.last_merge_operand;
        r->ax = r->es = r->ds;
        r->bx = r->dx = 0;
        if (end.interrupts_enabled)
            r->flags |= 0x200;
        r->flags = status(r->flags, 0, 16);
        break;
    }
    }
}
