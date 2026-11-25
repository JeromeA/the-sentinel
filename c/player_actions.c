#include <stdint.h>
#include "register_compat.h"
#include "typed_api.h"
#include "legacy_targets.h"
#include "data_aliases.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"

#include "render_state.h"

/* Cursor actions: absorb/create, transfer, turn, hyperspace and overview. */

uint8_t __far action_error(SegmentBytes data)
{
    segment_record(data, sound_storage).effects[5].pitch = 0xaa;
    uint8_t observed = c_sound_effect(5);
    segment_record(data, sound_storage).effects[5].pitch = 0x90;
    return observed;
}

void __far action_refresh(uint8_t melody)
{
    c_start_melody(melody);
    player_view_refresh_flags = 128;
}

struct CursorEffects __far operate_cursor(SegmentBytes data,
    uint16_t object_bank,
    int restoring,
    uint16_t *flags,
    struct CursorOperationEnd *observation)
{
    struct CursorEffects effects = {0};
    CursorRestoreCallback restore = 0;
    CursorDrawCallback draw = 0;
    if (restoring)
        restore = segment_record(data, graphics_session).restore_cursor;
    else
        draw = segment_record(data, graphics_session).draw_cursor;
    effects.restoring = restoring;
    effects.invoked = restoring ? restore != 0 : draw != 0;
    if (observation) {
        observation->restoring = restoring;
        observation->invoked = effects.invoked;
    }
    if (!effects.invoked)
        return effects;
    unsigned mode = restoring ? (restore == video_restore_cursor_cga            ? 0
                                        : restore == video_restore_cursor_tandy ? 1
                                        : restore == video_restore_cursor_ega   ? 2
                                                                                : 3)
                              : (draw == video_draw_cursor_cga            ? 0
                                        : draw == video_draw_cursor_tandy ? 1
                                        : draw == video_draw_cursor_ega   ? 2
                                                                          : 3);
    effects.mode = mode;
    if (observation)
        observation->mode = mode;
    if (restoring) {
        if (observation)
            call_video_cursor_restore(
                restore, data, object_bank & 0xff00, &observation->result.restore);
        else if (restore == video_restore_cursor_cga || restore == video_restore_cursor_tandy ||
            restore == video_restore_cursor_ega || restore == video_restore_cursor_vga)
            restore_cursor(data, object_bank & 0xff00, 0, mode);
        else {
            struct CursorRestoreEnd opaque;
            call_video_cursor_restore(restore, data, object_bank & 0xff00, &opaque);
        }
        if (flags)
            *flags = status(*flags, 0, 16);
    } else if (observation) {
        call_video_cursor_draw(draw, data, object_bank & 0xff00, &observation->result.draw);
        effects.executed = observation->result.draw.executed;
        effects.lookup_bank = observation->result.draw.terminal_step & 0xff00;
    } else if (draw == video_draw_cursor_cga || draw == video_draw_cursor_tandy ||
        draw == video_draw_cursor_ega || draw == video_draw_cursor_vga) {
        effects.executed = draw_cursor_effects(data, object_bank & 0xff00, 0, mode, flags);
        /* Accepted step/plane observations are bytes, so their high bank is zero. */
        effects.lookup_bank = 0;
    } else {
        struct CursorDrawEnd opaque;
        call_video_cursor_draw(draw, data, object_bank & 0xff00, &opaque);
        effects.executed = opaque.executed;
        effects.lookup_bank = opaque.terminal_step & 0xff00;
        if (flags) {
            *flags = status(*flags, opaque.skip_status, 8);
            if (opaque.executed)
                *flags = subtract8(*flags, opaque.saved_count, 1) & ~CF;
        }
    }
    if (!restoring && mode >= 2 && flags)
        *flags = status(*flags, 0, 16);
    return effects;
}

struct CursorOperationEnd __far operate_software_cursor(
    SegmentBytes data, uint16_t object_bank, int restore)
{
    struct CursorOperationEnd end = {0};
    operate_cursor(data, object_bank, restore, 0, &end);
    return end;
}

struct CursorEffects __far restore_action_cursor(SegmentBytes data,
    uint16_t object_bank,
    uint16_t *flags,
    struct CursorOperationEnd *observation)
{
    struct CursorEffects effects = {0};
    uint8_t mode = cursor_mode_flags;
    if (flags)
        *flags = status(*flags, mode, 8);
    if (mode & 128)
        effects = operate_cursor(data, object_bank, 0, flags, observation);
    if (observation)
        observation->initial_mode = mode;
    cursor_update_guard_flags = 0;
    return effects;
}

struct CursorOperationEnd __far c_restore_action_cursor(SegmentBytes data, uint16_t object_bank)
{
    struct CursorOperationEnd end = {0};
    restore_action_cursor(data, object_bank, 0, &end);
    return end;
}

struct HyperspaceResult __far hyperspace_object(
    SegmentBytes data, struct HyperspaceEnd *observation)
{
    if (observation)
        *observation = (struct HyperspaceEnd){0};
    struct HyperspaceResult result = {0};
    c_allocate_model(MODEL_ROBOT);
    uint8_t height = object_height_integer[player_robot_slot] + 1;
    result.robot = segment_record(data, game_work).work_allocated_object_slot;
    result.found = place_object_flat(segment_record(data, game_work).work_allocated_object_slot,
        height,
        data,
        observation ? &observation->placement : 0);
    if (observation)
        observation->robot = result.robot;
    if (!result.found)
        return result;
    struct EnergyChange energy;
    int accepted =
        c_adjust_energy(segment_record(data, game_work).work_allocated_object_slot, 1, &energy);
    result.model = energy.model;
    if (observation)
        observation->energy = energy;
    if (!accepted) {
        if (observation)
            observation->stage = 1;
        detach_object(segment_record(data, game_work).work_allocated_object_slot,
            data,
            observation ? &observation->detached : 0);
        segment_record(data, game_work).work_scene_background_style = 3;
        hyperspace_outcome_flags = 128;
    } else {
        if (observation)
            observation->stage = 2;
        c_start_melody(0);
        if (object_grid_x[player_robot_slot] ==
                segment_record(data, sentry_work).work_sentinel_start_square_x &&
            object_grid_y[player_robot_slot] ==
                segment_record(data, sentry_work).work_sentinel_start_square_y) {
            hyperspace_outcome_flags = 192;
            landscape_generation_flags = 128;
        }
        c_input_ready();
        player_robot_slot = segment_record(data, game_work).work_allocated_object_slot;
    }
    player_view_refresh_flags = 128;
    return result;
}

struct HyperspaceEnd __far c_hyperspace(SegmentBytes data)
{
    struct HyperspaceEnd end;
    hyperspace_object(data, &end);
    return end;
}

static struct ActorAction finish_actor_action(
    struct ActorAction action, struct PlayerActionEnd *observation)
{
    if (observation) {
        observation->render.polygon.start_vertex = action.state.source;
        observation->render.polygon.end_slot = action.state.target;
        observation->render.data_output = action.state.data_output;
        observation->outcome = action.outcome;
    }
    return action;
}

static void reject_actor_action(
    struct ActorAction *action, SegmentBytes data, struct PlayerActionEnd *observation)
{
    uint8_t observed = action_error(data);
    action->outcome = PLAYER_ACTION_REJECTED;
    action->state.interrupts_enabled = 1;
    if (observation) {
        struct PolygonScanEnd *raw = &observation->render.polygon;
        raw->value = (raw->value & 0xff00) | 5;
        raw->flags = status(raw->flags, observed, 8) | 0x200 | CF;
    }
}

static void refresh_actor_action(
    struct ActorAction *action, uint8_t melody, struct PlayerActionEnd *observation)
{
    action->outcome = PLAYER_ACTION_REFRESH_STARTED;
    action_refresh(melody);
    if (observation) {
        observation->render.polygon.value = (observation->render.polygon.value & 0xff00) | melody;
        observation->render.polygon.flags |= CF;
    }
}

static int actor_energy(
    struct ActorState *actor, uint16_t slot, int debit, struct PlayerActionEnd *observation)
{
    struct EnergyChange change;
    int accepted = c_adjust_energy(slot, debit, &change);
    actor->target = (actor->target & 0xff00) | change.model;
    if (observation) {
        struct PolygonScanEnd *raw = &observation->render.polygon;
        raw->end_slot = actor->target;
        raw->span_destination = actor->target;
        raw->changed |= POLYGON_END_SLOT | POLYGON_DESTINATION;
        if (accepted) {
            raw->value = (raw->value & 0xff00) | change.after;
            raw->flags = status(raw->flags, change.after, 8);
        } else {
            raw->value = (raw->value & 0xff00) | (uint8_t)(change.before - change.cost);
            raw->flags = subtract8(raw->flags, change.before, change.cost) | CF;
        }
    }
    return accepted;
}

static void actor_cursor(struct ActorState *actor,
    SegmentBytes data,
    int restore,
    int action_restore,
    struct PlayerActionEnd *observation)
{
    struct CursorEffects cursor = action_restore
        ? restore_action_cursor(
              data, actor->source, observation ? &observation->render.polygon.flags : 0, 0)
        : operate_cursor(data,
              actor->source,
              restore,
              observation ? &observation->render.polygon.flags : 0,
              0);
    struct RenderEffects effects = {0};
    effects.source = actor->source;
    effects.target = actor->target;
    effects.backwards = actor->backwards;
    if (cursor.invoked) {
        if (cursor.restoring || cursor.executed) {
            effects.source = cursor.lookup_bank;
            effects.source_updated = 1;
            effects.data_output = 1;
            if (!cursor.restoring && cursor.mode == 1) {
                effects.target = (effects.target & 0xff00) | 0xee;
                effects.target_updated = 1;
            }
        }
        if ((cursor.restoring && cursor.mode == 2) || (!cursor.restoring && cursor.mode >= 2)) {
            effects.target = 0;
            effects.target_updated = 1;
        }
    }
    actor_render(actor, &effects);
    if (observation) {
        observation->render.polygon.start_vertex = actor->source;
        observation->render.polygon.end_slot = actor->target;
        render_cursor_effect(&observation->render, &cursor);
    }
}

struct ActorAction __far execute_actor_action(SegmentBytes data,
    uint16_t observer_bank,
    uint16_t target_bank,
    int backwards,
    struct PlayerActionEnd *observation)
{
    struct ActorAction end = {0};
    struct ActorState *work = &end.state;
    work->backwards = backwards;
    struct PolygonScanEnd *raw = observation ? &observation->render.polygon : 0;
    uint8_t action = player_action_code;
    if (observation) {
        raw->changed = 63;
    }
    work->source = observer_bank;
    if (observation) {
        raw->value = (raw->value & 0xff00) | action;
    }
    if (action == OBJECT_ACTION_HYPERSPACE) {
        struct HyperspaceEnd jump;
        struct HyperspaceResult result = hyperspace_object(data, observation ? &jump : 0);
        work->source = result.robot;
        if (result.found)
            work->target = (work->target & 0xff00) | result.model;
        if (observation) {
            struct RenderEnd *render = &observation->render;
            render->polygon.start_vertex = work->source;
            render->polygon.end_slot = work->target;
            render->polygon.span_count = 0;
            render->polygon.changed |= POLYGON_VERTEX | POLYGON_COUNT;
            if (!jump.placement.found) {
                render->polygon.error = jump.placement.failure_cell.bank;
                render->polygon.span_destination = jump.placement.failure_cell.offset;
                render->polygon.value =
                    (jump.placement.failure_cell.bank & 0xff00) | jump.placement.height_limit;
                render->polygon.changed |= POLYGON_ERROR | POLYGON_DESTINATION;
                render->polygon.flags =
                    subtract8(render->polygon.flags, jump.placement.height_limit, 12) | CF;
            } else {
                render_attachment_observation(render, &jump.placement.attachment);
                render->polygon.end_slot = work->target;
                render->polygon.span_destination = work->target;
                render->polygon.changed |= POLYGON_END_SLOT | POLYGON_DESTINATION;
                if (jump.stage == 1)
                    render_detach_observation(render, &jump.detached);
                else {
                    render->polygon.value = 128;
                    render->polygon.start_vertex = jump.robot;
                    render->polygon.flags = status(render->polygon.flags, 128, 8);
                }
                render->polygon.flags &= ~CF;
            }
        }
        segment_record(data, game_work).work_scene_view_model_selector = 0;
        end.outcome = PLAYER_ACTION_HANDLED;
        if (observation) {
            raw->flags |= CF;
        }
        return finish_actor_action(end, observation);
    }
    if (action == OBJECT_ACTION_OVERVIEW) {
        cursor_update_guard_flags |= 128;
        {
            actor_cursor(work, data, 1, 0, observation);
        }

        struct BackgroundProbeEnd probe = c_background_is_occupied(data);
        {
            if (observation) {
                raw->error = (uint32_t)data >> 16;

                raw->span_destination = probe.destination;
            }
            work->target = probe.row;
            if (observation) {
                raw->span_count = probe.alternate;

                raw->value = 255;

                raw->changed |=
                    POLYGON_ERROR | POLYGON_DESTINATION | POLYGON_END_SLOT | POLYGON_COUNT;

                raw->flags = subtract16(raw->flags, probe.comparison_left, probe.comparison_right);

                if (probe.occupied)
                    raw->flags |= CF;
                else
                    raw->flags &= ~CF;
            }
        }

        if (!probe.occupied) {
            {
                actor_cursor(work, data, 0, 1, observation);
            }
            overview_pending = 128;
            refresh_actor_action(&end, 0x19, observation);
            return finish_actor_action(end, observation);
        }
        {
            actor_cursor(work, data, 0, 1, observation);
        }
        reject_actor_action(&end, data, observation);
        return finish_actor_action(end, observation);
    }
    work->source = (work->source & 0xff00) | view_object_slot;
    if (action == OBJECT_ACTION_TURN) {
        uint8_t before = turn_and_occlusion_flags;
        turn_and_occlusion_flags <<= 1;
        if (observation) {
            raw->flags = add8(raw->flags, before, before);
        }
        if (!(turn_and_occlusion_flags & 128)) {
            end.outcome = PLAYER_ACTION_HANDLED;
            if (observation) {
                raw->flags |= CF;
            }
            return finish_actor_action(end, observation);
        }
        object_heading[work->source] ^= 128;
        refresh_actor_action(&end, 0x28, observation);
        return finish_actor_action(end, observation);
    }
    ray_target_test_flags &= 127;
    int contact;
    {
        uint16_t observer = work->source;
        if (!(target_bank & 0xff00))
            c_synthesize_cursor_ray(observer, data);
        /* Direction setup clears the actual ray lookup banks. Preserve the
         * fallback destination when a rejected ray never samples a cell. */
        if (observation) {
            raw->span_destination = 0;

            raw->changed |= POLYGON_DESTINATION;
        }
        struct RayContact ray = trace_world_contact(data, 0, 0);
        work->source = ray.observer_index;
        work->target = ray.position_index;
        if (observation) {
            raw->changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
        }
        contact = ray.contact;
    }
    if (!contact) {
        reject_actor_action(&end, data, observation);
        return finish_actor_action(end, observation);
    }
    uint8_t action_bits = player_action_code & OBJECT_ACTION_EXISTING_BIT;
    if (observation) {
        raw->value = (raw->value & 0xff00) | action_bits;

        raw->flags = status(raw->flags, (uint8_t)raw->value, 8);
    }
    if (player_action_code & OBJECT_ACTION_EXISTING_BIT) {
        struct TerrainCellAddress cell = c_terrain_address(terrain_scan_x, terrain_scan_y);
        if (observation) {
            raw->error = cell.bank;
            raw->span_destination = cell.offset;

            raw->value = (cell.bank & 0xff00) | cell.value.raw;

            raw->flags = subtract8(raw->flags, cell.value.raw, 0xc0) ^ CF;

            raw->changed |= POLYGON_ERROR | POLYGON_DESTINATION;
        }
        if (cell.value.object.tag != TERRAIN_OBJECT_TAG) {
            reject_actor_action(&end, data, observation);
            return finish_actor_action(end, observation);
        }
        uint16_t object = (work->source & 0xff00) | cell.value.object.slot;
        work->source = object;
        uint8_t shifted_action = player_action_code >> 1;
        if (observation) {
            raw->value = (raw->value & 0xff00) | shifted_action;
        }
        if (player_action_code & OBJECT_ACTION_TRANSFER_BIT) {
            uint8_t model = object_model_id[object];
            work->target = (work->target & 0xff00) | model;
            if (observation) {
                raw->span_destination = work->target;
            }
            if (model != MODEL_ROBOT) {
                reject_actor_action(&end, data, observation);
                return finish_actor_action(end, observation);
            }
            uint8_t ready = c_input_ready();
            if (observation) {
                raw->flags = status(raw->flags, ready, 8);
            }
            player_robot_slot = (uint8_t)object;
            for (;;) {
                uint8_t link = object_stack_links[object];
                if (link < OBJECT_LINK_SUPPORT)
                    break;
                object = (object & 0xff00) | (link & OBJECT_SLOT_MASK);
                if ((link & OBJECT_SLOT_MASK) == OBJECT_SLOT_MASK) {
                    secret_code_pair_work = object_model_id[object];
                    /* Retain the reserved-slot scratch store and readback. */
                    uint8_t secret = secret_code_pair_work;
                    if (observation) {
                        raw->value = (raw->value & 0xff00) | secret;
                    }
                    break;
                }
            }
            work->source = object;
            refresh_actor_action(&end, 0x19, observation);
            return finish_actor_action(end, observation);
        }
        if (object_stack_links[0] & OBJECT_LINK_UNUSED) {
            reject_actor_action(&end, data, observation);
            return finish_actor_action(end, observation);
        }
        uint8_t model = object_model_id[object];
        if (model == MODEL_SENTINEL_PEDESTAL) {
            reject_actor_action(&end, data, observation);
            return finish_actor_action(end, observation);
        }
        if (model == MODEL_MEANIE) {
            uint16_t owner_bank = work->target & 0xff00;
            uint16_t owner_index = owner_bank;
            int owner;
            for (owner = 7; owner >= 0; --owner) {
                owner_index = owner_bank | (uint8_t)owner;
                if (!(object_stack_links[owner_index] & OBJECT_LINK_UNUSED) &&
                    (object_model_id[owner_index] == MODEL_SENTRY ||
                        object_model_id[owner_index] == MODEL_SENTINEL) &&
                    sentry_meanie_slots[owner_index] == (uint8_t)object) {
                    sentry_meanie_slots[owner_index] = 128;
                    break;
                }
            }
            /* Exhaustion leaves the last attempted owner (zero), not -1. */
            work->target = owner_index;
            if (observation) {
                raw->span_destination = owner_index;
            }
        }
        {
            struct ObjectDetachEnd detached;
            detach_object(object, data, observation ? &detached : 0);
            if (observation)
                render_detach_observation(&observation->render, &detached);
        }
        segment_record(data, game_work).work_allocated_object_slot = (uint8_t)work->source;
        actor_energy(work, work->source, 0, observation);
        end.outcome = PLAYER_ACTION_CHANGED;
        if (observation) {
            raw->flags &= ~CF;
        }
        return finish_actor_action(end, observation);
    }
    int allocated = render_allocate(observation ? &observation->render : 0, -1);
    work->source = allocated < 0 ? 255 : allocated;
    if (allocated < 0) {
        reject_actor_action(&end, data, observation);
        return finish_actor_action(end, observation);
    }
    if (!actor_energy(work, work->source, 1, observation)) {
        reject_actor_action(&end, data, observation);
        return finish_actor_action(end, observation);
    }
    work->source =
        (work->source & 0xff00) | segment_record(data, game_work).work_allocated_object_slot;
    terrain_scan_x = ray_position_x.integer;
    terrain_scan_y = ray_position_y.integer;
    {
        struct ObjectAttachEnd attached;
        int accepted = attach_object(work->source, data, observation ? &attached : 0);
        if (observation)
            render_attachment_observation(&observation->render, &attached);
        if (!accepted) {
            actor_energy(work, work->source, 0, observation);
            reject_actor_action(&end, data, observation);
            return finish_actor_action(end, observation);
        }
    }
    if (object_model_id[work->source] == MODEL_ROBOT) {
        work->target = (work->target & 0xff00) | player_robot_slot;
        if (observation) {
            raw->span_destination = work->target;
        }
        object_heading[work->source] = object_heading[work->target] ^ 128;
        if (observation) {
            raw->value = (raw->value & 0xff00) | object_heading[work->source];
        }
    }
    end.outcome = PLAYER_ACTION_CHANGED;
    if (observation) {
        raw->flags &= ~CF;
    }

    return finish_actor_action(end, observation);
}

struct PlayerActionEnd __far c_execute_player_action(
    SegmentBytes data, uint16_t observer_bank, uint16_t target_bank, uint16_t initial_flags)
{
    struct PlayerActionEnd end = {0};
    end.render.polygon.flags = initial_flags;
    execute_actor_action(data, observer_bank, target_bank, !!(initial_flags & 0x400), &end);
    return end;
}
