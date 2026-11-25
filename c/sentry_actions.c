#ifdef SENTRY_ACTIONS_DATA_ONLY
/* Owned subsystem data. */
volatile uint8_t sentry_countdown_phase = 0x0;
uint8_t sentry_sight_saved_source_slot = 0x0;
uint16_t sentry_scheduler_saved_sp = 0x0;

/* SentryWork is reset independently of other subsystems and data placement. */
struct SentryWork sentry_work = {0};

void __far reset_sentry_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&sentry_work), 0, sizeof(sentry_work), 0);
}
#else
#include <stdint.h>
#include "register_compat.h"
#include "typed_api.h"
#include "legacy_targets.h"
#include "data_aliases.h"
#include "render_state.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"

/* Sentries conserve absorbed energy and replant it as trees. A partial redraw
 * can defer a mutation through the retained scheduler stack-restore anchor. */

static struct SentryTask finish_sentry_task(struct SentryTask task, struct SentryEnd *raw)
{
    if (raw) {
        raw->state.source = task.state.source;
        raw->state.target = task.state.target;
        raw->state.data_output = task.state.data_output;
        raw->route = task.route;
        raw->outcome = task.outcome;
    }
    return task;
}

static void task_sight(struct SentryTask *task, const struct SightIndices *sight)
{
    task->state.source = sight->observer;
    task->state.target = sight->target;
}

static void task_sound(struct SentryTask *task, uint8_t effect, struct SentryEnd *raw)
{
    uint8_t observed = c_sound_effect(effect);
    task->state.interrupts_enabled = 1;
    if (raw)
        raw->state.flags = status(raw->state.flags, observed, 8) | 0x200;
}

static uint16_t sentry_detach_flags(uint16_t flags, const struct ObjectDetachEnd *detached)
{
    flags = status(flags, detached->value, 8);
    if (detached->support < OBJECT_LINK_SUPPORT)
        flags |= ((detached->height & 16) ? CF : 0) |
            ((detached->height & 24) == 8 || (detached->height & 24) == 16 ? OF : 0);
    return flags;
}

static uint16_t sentry_attachment_flags(uint16_t flags, const struct ObjectAttachEnd *attached)
{
    if (attached->cell.value.object.tag == TERRAIN_OBJECT_TAG) {
        flags = subtract8(
            flags, attached->support_model, attached->support_model == MODEL_BOULDER ? 3 : 6);
        if (!attached->accepted)
            return flags | CF;
    }
    return add8(flags, attached->random & 0xf8, 0x60) & ~CF;
}

static void task_hyperspace(struct SentryTask *task, SegmentBytes data, struct SentryEnd *raw)
{
    struct HyperspaceEnd jump;
    struct HyperspaceResult result = hyperspace_object(data, raw ? &jump : 0);
    task->state.source = result.robot;
    if (!result.found) {
        if (raw)
            raw->state.flags = subtract8(raw->state.flags, jump.placement.height_limit, 12) | CF;
        return;
    }
    task->state.target = (task->state.target & 0xff00) | result.model;
    if (raw) {
        uint16_t flags = sentry_attachment_flags(raw->state.flags, &jump.placement.attachment);
        flags =
            jump.stage == 1 ? sentry_detach_flags(flags, &jump.detached) : status(flags, 128, 8);
        raw->state.flags = flags & ~CF;
    }
}

static int task_merge(struct SentryTask *before,
    const struct SentryTask *after,
    SegmentBytes data,
    SegmentBytes *output)
{
    actor_merge(&before->state, &after->state);
    before->route = after->route;
    before->outcome = after->outcome;
    if (before->state.data_output)
        *output = data;
    return before->route != SENTRY_RETURN;
}

static int task_guard(struct SentryTask *task,
    SegmentBytes data,
    uint8_t selected,
    uint16_t source,
    uint16_t target,
    struct SentryEnd *raw)
{
    struct SentryPartialEnd partial = c_sentry_partial_change(data, selected, source, target);
    if (raw)
        raw->state.flags = status(raw->state.flags, partial.mode, 8);
    if (!partial.mode) {
        if (raw)
            raw->state.flags |= CF;
        return 0;
    }
    if (raw) {
        struct RenderEnd interval = render_selection_interval(&partial.interval, raw->state.flags);
        sentry_render(&raw->state, &interval);
    }
    task->state.source = source;
    task->state.target = target;
    return partial.interval.accepted;
}

struct SentryTask __far task_sentry_find_meanie_tree(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);
struct SentryTask __far task_sentry_absorb_unit(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);
struct SentryTask __far task_sentry_replant(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);
struct SentryTask __far task_sentry_find_stack_energy(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);
struct SentryTask __far task_sentry_schedule_start(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);
struct SentryTask __far task_sentry_active_dispatch(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);
struct SentryTask __far task_sentry_active_meanie(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);
struct SentryTask __far task_sentry_publish_change(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);
struct SentryTask __far task_sentry_publish_creation(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);
struct SentryTask __far task_meanie_turn(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);
struct SentryTask __far task_meanie_expire(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);
struct SentryTask __far task_sentry_track_visible_target(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);
struct SentryTask __far task_sentry_scan_world(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation);

static void task_absorb_stack(
    struct SentryTask *, SegmentBytes, SegmentBytes, uint16_t, uint16_t, struct SentryEnd *);

struct SentryPartialEnd __far c_sentry_partial_change(
    SegmentBytes data, uint8_t selected, uint16_t observer_bank, uint16_t target_bank)
{
    struct SentryPartialEnd end = {0};
    end.mode = partial_view_update_flags & 128;
    if (!end.mode)
        return end;
    segment_record(data, game_work).work_allocated_object_slot = selected;
    view_object_slot = player_robot_slot;
    end.interval = c_prepare_selection_interval(selected, observer_bank, target_bank);
    return end;
}

struct SentryTask __far task_sentry_find_meanie_tree(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;

    sentry_scan_angle_width = 40;
    view_object_slot = current_sentry_slot;
    for (;;) {
        uint8_t source = current_sentry_slot;
        uint8_t candidate = sentry_scan_cursor[source];
        uint16_t sentry_index = (work->source & 0xff00) | source;
        if (!candidate) {
            ++sentry_scan_pass[source];
            sentry_scan_target[source] = sentry_target_slots[source];
            work->source = sentry_index;
            work->target &= 0xff00;
            if (observation)
                raw->flags |= CF;
            return finish_sentry_task(end, observation);
        }
        --sentry_scan_cursor[source];
        --candidate;
        uint16_t candidate_index = (work->target & 0xff00) | candidate;
        if ((object_stack_links[candidate_index] & OBJECT_LINK_UNUSED) ||
            object_model_id[candidate_index] != MODEL_TREE)
            continue;
        uint8_t target = sentry_target_slots[source];
        if ((unsigned)(object_grid_x[target] > object_grid_x[candidate]
                    ? object_grid_x[target] - object_grid_x[candidate]
                    : object_grid_x[candidate] - object_grid_x[target]) >= 10)
            continue;
        if ((unsigned)(object_grid_y[target] > object_grid_y[candidate]
                    ? object_grid_y[target] - object_grid_y[candidate]
                    : object_grid_y[candidate] - object_grid_y[target]) >= 10)
            continue;
        uint16_t observer = (work->source & 0xff00) | target;
        uint16_t model_filter = MODEL_TREE;
        {
            struct SightIndices sight = test_sentry_visibility(
                data, model_filter, observer, candidate_index, candidate_index);
            task_sight(&end, &sight);
        }
        if (!(sentry_target_sight_result & 128))
            continue;
        /* Sight and guard can replace the banks; current sentry is sampled again. */
        sentry_index = (work->source & 0xff00) | current_sentry_slot;
        uint16_t tree = work->target;
        uint8_t selected = tree;
        work->source = sentry_index;
        if (task_guard(&end, data, selected, sentry_index, tree, observation)) {
            end.route = SENTRY_RETRY;
            return finish_sentry_task(end, observation);
        }
        uint16_t owner = work->source;
        uint16_t meanie = work->target;
        sentry_meanie_slots[owner] = (uint8_t)work->target;
        object_model_id[meanie] = MODEL_MEANIE;
        selection_extent_override = 0x68;
        end.outcome = SENTRY_TARGET_FOUND;
        if (observation)
            raw->flags &= ~CF;
        return finish_sentry_task(end, observation);
    }
}

struct SentryEnd __far c_sentry_find_meanie_tree(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_sentry_find_meanie_tree(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

struct SentryTask __far task_sentry_absorb_unit(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;

    uint16_t victim = (source_slot & 0xff00) | sentry_test_target_slot;
    work->source = victim;
    if ((uint8_t)victim == player_robot_slot) {
        if (!player_energy) {
            end.route = SENTRY_PLAYER_LOST;
            return finish_sentry_task(end, observation);
        }
        --player_energy;
        {
            struct RenderEnd hud;
            if (observation) {
                hud = (struct RenderEnd){0};
                hud.polygon.flags = raw->flags;
            }
            struct RenderEffects effects =
                gameplay_hud(data, 0, work->backwards, observation ? &hud : 0);
            actor_render(work, &effects);
            if (observation)
                raw->flags = hud.polygon.flags;
            if (end.state.data_output)
                output = data;
        }
        task_sound(&end, 5, observation);
        end.outcome = SENTRY_PLAYER_DRAINED;
        if (observation)
            raw->flags |= CF;
    } else {
        if (task_guard(&end, data, (uint8_t)victim, victim, target_slot, observation)) {
            end.route = SENTRY_ABORT;
            return finish_sentry_task(end, observation);
        }
        /* Guard rendering can replace the victim and owner banks. */
        victim = work->source;
        uint8_t model = object_model_id[victim];
        if (model == MODEL_ROBOT) {
            uint16_t owner = (work->target & 0xff00) | current_sentry_slot;
            work->target = owner;
            sentry_absorb_delays[owner] = 0;
            object_model_id[victim] = MODEL_BOULDER;
        } else if (model == MODEL_TREE) {
            struct ObjectDetachEnd detached;
            detach_object(victim, data, observation ? &detached : 0);
            if (observation)
                raw->flags = sentry_detach_flags(raw->flags, &detached);
        } else {
            selection_extent_override = 0x74;
            object_model_id[victim] = MODEL_TREE;
        }
        end.outcome = SENTRY_ENERGY_ABSORBED;
        if (observation)
            raw->flags &= ~CF;
    }
    uint16_t owner = (work->target & 0xff00) | current_sentry_slot;
    work->target = owner;
    {
        uint8_t before = sentry_replant_energy[owner];
        ++sentry_replant_energy[owner];
        if (observation)
            raw->flags = (add8(raw->flags, before, 1) & ~CF) | (raw->flags & CF);
    }

    return finish_sentry_task(end, observation);
}

struct SentryEnd __far c_sentry_absorb_unit(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_sentry_absorb_unit(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

struct SentryTask __far task_sentry_replant(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;

    uint8_t source = current_sentry_slot;
    work->source = (work->source & 0xff00) | source;
    uint8_t energy = sentry_replant_energy[source];
    if (observation)
        raw->flags = status(raw->flags, energy, 8) | CF;
    if (!sentry_replant_energy[source])
        return finish_sentry_task(end, observation);
    int allocated = c_allocate_model(MODEL_TREE);
    if (allocated < 0) {
        work->source = 255;
        if (observation)
            raw->flags = (raw->flags & ~ARITH_FLAGS) | CF | PF | AF | SF;
    } else {
        work->source = allocated;
        (void)pending_object_model;
        if (observation)
            raw->flags = status(raw->flags, object_stack_links[allocated], 8);
    }
    uint16_t tree = work->source;
    struct ObjectPlacementEnd placed;
    int found = place_object_flat(
        tree, segment_record(data, game_work).object_height_limit, data, observation ? &placed : 0);
    if (observation)
        raw->flags = placed.found ? sentry_attachment_flags(raw->flags, &placed.attachment) & ~CF
                                  : subtract8(raw->flags, placed.height_limit, 12) | CF;
    if (!found)
        return finish_sentry_task(end, observation);
    tree = work->source;
    uint16_t target = work->target;
    if (task_guard(&end, data, (uint8_t)tree, tree, target, observation)) {
        {
            struct ObjectDetachEnd detached;
            detach_object(work->source, data, observation ? &detached : 0);
            if (observation)
                raw->flags = sentry_detach_flags(raw->flags, &detached);
        }
        end.route = SENTRY_ABORT;
        return finish_sentry_task(end, observation);
    }
    uint16_t owner = (work->source & 0xff00) | current_sentry_slot;
    work->source = owner;
    --sentry_replant_energy[owner];
    tree = (owner & 0xff00) | segment_record(data, game_work).work_allocated_object_slot;
    work->source = tree;
    end.outcome = SENTRY_TREE_CREATED;
    if (observation)
        raw->flags &= ~CF;

    return finish_sentry_task(end, observation);
}

struct SentryEnd __far c_sentry_replant(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_sentry_replant(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

static uint8_t add_bcd_byte(uint8_t left, uint8_t right, unsigned carry, unsigned *out)
{
    unsigned sum = (unsigned)left + right + carry;
    uint8_t original = sum;
    uint8_t result = original;
    union BcdPair original_pair = {.raw = original};
    union BcdPair left_pair = {.raw = left};
    union BcdPair right_pair = {.raw = right};
    if (original_pair.digits.ones > 9 ||
        (left_pair.digits.ones + right_pair.digits.ones + carry) > 15)
        result += 6;
    *out = original > 0x99 || sum > 255;
    if (*out)
        result += 0x60;
    return result;
}

struct RenderEnd __far completion_landscape_flow(
    SegmentBytes data, SegmentBytes output, uint16_t flags, int observed)
{
    unsigned carry = 0;
    uint8_t low = add_bcd_byte(c_energy_bcd(), landscape_number_bcd.pairs.low.raw, 0, &carry);
    uint8_t high = add_bcd_byte(landscape_number_bcd.pairs.high.raw, 0, carry, &carry);
    union LandscapeNumber next = {.raw = ((uint16_t)high << 8) | low};
    c_select_landscape(next);
    generate_landscape(data, 0);
    c_build_preview(data);
    populate_preview(data, 0, 0);
    struct RenderEnd end = title_screen_flow(data, output, 128, flags, observed);
    if (end.data_output)
        output = data;
    {
        render_text(&end, data, TEXT_SCRIPT_COMPLETION_CAPTION);
        if (end.data_output)
            output = data;
    }
    {
        if (write_landscape_number(data, output, !!(end.polygon.flags & 0x400)))
            end.data_output = 1;
    }
    return end;
}

struct SentryTask __far task_sentry_find_stack_energy(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;

    int source;
    for (source = 63; source >= 0; --source) {
        uint8_t link = object_stack_links[source];
        uint16_t observer = (work->source & 0xff00) | source;
        /* Keep the last examined link for an exhausted search's result. */
        work->source = observer;
        if (link & OBJECT_LINK_UNUSED)
            continue;
        if (link < OBJECT_LINK_SUPPORT && object_model_id[source] != MODEL_BOULDER)
            continue;
        terrain_scan_x = object_grid_x[source];
        terrain_scan_y = object_grid_y[source];
        struct TerrainCellAddress cell = c_terrain_address(terrain_scan_x, terrain_scan_y);
        if (observation)
            raw->flags = subtract8(raw->flags, cell.value.raw, 0xc0) ^ CF;
        if (cell.value.object.tag != TERRAIN_OBJECT_TAG)
            continue;
        uint16_t candidate = (work->target & 0xff00) | cell.value.object.slot;
        uint8_t model = object_model_id[candidate];
        /* Preserve near-miss observations even when this model cannot supply energy. */
        work->target = candidate;
        if (model != MODEL_TREE && model != MODEL_BOULDER)
            continue;
        uint16_t model_filter = (cell.bank & 0xff00) | model;
        {
            struct SightIndices sight =
                test_sentry_visibility(data, model_filter, observer, candidate, candidate);
            task_sight(&end, &sight);
        }
        if (!(sentry_target_sight_result & 128))
            continue;
        sentry_test_target_slot = (uint8_t)work->target;
        end.outcome = SENTRY_TARGET_FOUND;
        if (observation)
            raw->flags &= ~CF;
        return finish_sentry_task(end, observation);
    }
    work->source = (work->source & 0xff00) | 255;
    if (observation)
        raw->flags |= CF;
    return finish_sentry_task(end, observation);
}

struct SentryEnd __far c_sentry_find_stack_energy(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_sentry_find_stack_energy(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

struct SentryTask __far task_sentry_schedule_start(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;

    uint8_t source = current_sentry_slot;
    uint8_t model = object_model_id[source];
    work->source = (work->source & 0xff00) | source;
    if (model != MODEL_SENTRY && model != MODEL_SENTINEL) {
        end.route = SENTRY_FINISH;
        return finish_sentry_task(end, observation);
    }
    segment_record(data, game_work).work_scene_view_model_selector = model;
    uint8_t link = object_stack_links[source];
    if (observation)
        raw->flags = status(raw->flags, link, 8);
    if (object_stack_links[source] & OBJECT_LINK_UNUSED) {
        {
            struct SentryEnd next_raw;
            if (observation)
                next_raw.state.flags = raw->flags;
            struct SentryTask next = task_sentry_replant(data,
                output,
                work->source,
                work->target,
                work->backwards,
                observation ? &next_raw : 0);
            if (observation)
                raw->flags = next_raw.state.flags;
            if (task_merge(&end, &next, data, &output))
                return finish_sentry_task(end, observation);
        }
        end.route = end.outcome == SENTRY_TREE_CREATED ? SENTRY_CREATION : SENTRY_FINISH;
        return finish_sentry_task(end, observation);
    }
    end.route = SENTRY_ACTIVE;

    return finish_sentry_task(end, observation);
}

struct SentryEnd __far c_sentry_schedule_start(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_sentry_schedule_start(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

struct RenderEnd __far c_completion_landscape(
    SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    return completion_landscape_flow(data, output, flags, 1);
}

static void advance_sentry_clock(struct SentryClockEnd *observation)
{
    struct GameRandomStep random = game_random_next(&prng_state_low);
    uint8_t previous = current_sentry_slot;
    --current_sentry_slot;
    if ((int8_t)current_sentry_slot < 0)
        current_sentry_slot = 7;
    view_object_slot = player_robot_slot;
    /* This volatile read belongs to the clock phase even without AX capture. */
    uint8_t random_high = prng_state_low.middle;
    uint8_t player = player_robot_slot;
    if (observation) {
        observation->carry = !!(random.upper_input & 0x8000);
        observation->previous = previous;
        observation->value = ((uint16_t)random_high << 8) | player;
    }
}

/* The real assembly finish entry retains its independent register contract. */
struct SentryClockEnd __far c_sentry_schedule_finish(void)
{
    struct SentryClockEnd end;
    advance_sentry_clock(&end);
    return end;
}

struct SentryTask __far task_sentry_active_dispatch(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;

    uint8_t slot = source_slot;
    uint8_t delay = sentry_scan_delays[source_slot];
    if (delay >= 2) {
        if (observation)
            raw->flags = subtract8(raw->flags, delay, 2);
        end.route = SENTRY_FINISH;
        return finish_sentry_task(end, observation);
    }
    sentry_scan_delays[slot] = 4;
    sentry_scan_angle_width = 20;
    uint8_t meanie = sentry_meanie_slots[slot];
    if (observation)
        raw->flags = status(raw->flags, meanie, 8);
    end.route = meanie & 128 ? SENTRY_SCAN : SENTRY_MEANIE;
    return finish_sentry_task(end, observation);
}

struct SentryEnd __far c_sentry_active_dispatch(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_sentry_active_dispatch(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

struct SentryTask __far task_sentry_active_meanie(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;

    view_object_slot = sentry_meanie_slots[source_slot];
    uint16_t target = (target_slot & 0xff00) | sentry_target_slots[source_slot];
    uint8_t link = object_stack_links[target];

    if (link & OBJECT_LINK_UNUSED) {
        work->target = target;
        if (observation)
            raw->flags = status(raw->flags, link, 8);
        end.route = SENTRY_EXPIRE;
        return finish_sentry_task(end, observation);
    }
    {
        struct SightIndices sight = test_sentry_visibility(data, 0, source_slot, target, target);
        task_sight(&end, &sight);
    }
    uint8_t angle = model_azimuth_bias >> 8;
    if (observation)
        raw->flags = subtract8(raw->flags, angle, 20);
    if (angle >= 20) {
        end.route = SENTRY_TURN;
        return finish_sentry_task(end, observation);
    }
    uint8_t sight_target = work->target;
    if (sight_target != player_robot_slot || !sentry_target_sight_result) {
        end.route = SENTRY_EXPIRE;
        return finish_sentry_task(end, observation);
    }
    task_hyperspace(&end, data, observation);
    scene_view_model_selector = MODEL_MEANIE;
    end.route = SENTRY_FINISH;
    return finish_sentry_task(end, observation);
}

struct SentryEnd __far c_sentry_active_meanie(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_sentry_active_meanie(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

struct SentryTask __far task_sentry_publish_change(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;

    segment_record(data, game_work).work_allocated_object_slot = (uint8_t)work->source;
    view_object_slot = player_robot_slot;
    (void)player_robot_slot;
    {
        struct RenderEnd selection;
        if (observation) {
            selection = (struct RenderEnd){0};
            selection.polygon.flags = raw->flags;
        }
        struct RenderEffects effects = present_selected_effects(data,
            output,
            work->source,
            work->target,
            work->backwards,
            observation ? &selection : 0);
        actor_render(work, &effects);
        if (observation)
            raw->flags = selection.polygon.flags;
    }
    end.route = SENTRY_FINISH;

    return finish_sentry_task(end, observation);
}

struct SentryEnd __far c_sentry_publish_change(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_sentry_publish_change(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

struct SentryTask __far task_sentry_publish_creation(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;
    selection_presentation_flags = 64;
    {
        struct SentryEnd next_raw;
        if (observation)
            next_raw.state.flags = raw->flags;
        struct SentryTask next = task_sentry_publish_change(
            data, output, work->source, work->target, work->backwards, observation ? &next_raw : 0);
        if (observation)
            raw->flags = next_raw.state.flags;
        if (task_merge(&end, &next, data, &output))
            return finish_sentry_task(end, observation);
    }
    return finish_sentry_task(end, observation);
}

struct SentryEnd __far c_sentry_publish_creation(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_sentry_publish_creation(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

struct SentryTask __far task_meanie_turn(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;

    uint8_t step = data[((uint16_t)&projection_work + 28)] & 128 ? 0xf8 : 8;
    segment_record(data, sentry_work).work_meanie_turn_step[0] = step;
    uint16_t owner = (target_slot & 0xff00) | current_sentry_slot;
    uint16_t meanie = (source_slot & 0xff00) | sentry_meanie_slots[owner];
    /* The guard can leave its inputs as the published result when no redraw is needed. */
    work->target = owner;
    work->source = meanie;
    if (task_guard(&end, data, (uint8_t)meanie, meanie, owner, observation)) {
        end.route = SENTRY_ABORT;
        return finish_sentry_task(end, observation);
    }
    uint16_t turning_meanie = work->source;
    uint16_t owning_sentry = work->target;
    object_heading[turning_meanie] += step;
    sentry_scan_delays[owning_sentry] = 10;
    task_sound(&end, 1, observation);
    work->source = turning_meanie;
    {
        struct SentryEnd next_raw;
        if (observation)
            next_raw.state.flags = raw->flags;
        struct SentryTask next = task_sentry_publish_change(data,
            output,
            turning_meanie,
            work->target,
            work->backwards,
            observation ? &next_raw : 0);
        if (observation)
            raw->flags = next_raw.state.flags;
        if (task_merge(&end, &next, data, &output))
            return finish_sentry_task(end, observation);
    }
    return finish_sentry_task(end, observation);
}

struct SentryEnd __far c_meanie_turn(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_meanie_turn(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

struct SentryTask __far task_meanie_expire(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;

    sentry_absorb_delays[source_slot] = 0;
    uint16_t owner = (target_slot & 0xff00) | current_sentry_slot;
    uint16_t meanie = (source_slot & 0xff00) | sentry_meanie_slots[owner];
    /* Keep the guard's full-index observations, including its unchanged-result path. */
    work->target = owner;
    work->source = meanie;
    if (task_guard(&end, data, (uint8_t)meanie, meanie, owner, observation)) {
        end.route = SENTRY_ABORT;
        return finish_sentry_task(end, observation);
    }
    uint16_t retiring_meanie = work->source;
    uint16_t owning_sentry = work->target;
    sentry_meanie_slots[owning_sentry] = 128;
    object_model_id[retiring_meanie] = MODEL_TREE;
    end.route = SENTRY_CREATION;
    return finish_sentry_task(end, observation);
}

struct SentryEnd __far c_meanie_expire(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_meanie_expire(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

struct SentryTask __far task_sentry_track_visible_target(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;

    sentry_target_slots[source_slot] = (uint8_t)target_slot;
    sentry_target_sight[source_slot] = sentry_target_sight_result;
    uint8_t delay = sentry_absorb_delays[source_slot];
    if (observation)
        raw->flags = subtract8(raw->flags, delay, 1);
    if (!delay) {
        sentry_absorb_delays[source_slot] = 120;
        end.route = SENTRY_FINISH;
        return finish_sentry_task(end, observation);
    }
    if (delay != 1) {
        end.route = SENTRY_FINISH;
        return finish_sentry_task(end, observation);
    }
    if (sentry_target_sight_result & 128) {
        {
            struct SentryEnd next_raw;
            if (observation)
                next_raw.state.flags = raw->flags;
            struct SentryTask next = task_sentry_absorb_unit(data,
                output,
                source_slot,
                target_slot,
                work->backwards,
                observation ? &next_raw : 0);
            if (observation)
                raw->flags = next_raw.state.flags;
            if (task_merge(&end, &next, data, &output))
                return finish_sentry_task(end, observation);
        }
        uint16_t owner = (work->target & 0xff00) | current_sentry_slot;
        work->target = owner;
        /* Retained-target absorption cools down even when it consumes the player. */
        sentry_scan_delays[owner] = 30;
        if (end.outcome == SENTRY_PLAYER_DRAINED) {
            end.route = SENTRY_FINISH;
            return finish_sentry_task(end, observation);
        }
        end.route = SENTRY_CREATION;
        return finish_sentry_task(end, observation);
    }
    {
        struct SentryEnd next_raw;
        if (observation)
            next_raw.state.flags = raw->flags;
        struct SentryTask next = task_sentry_find_meanie_tree(
            data, output, source_slot, target_slot, work->backwards, observation ? &next_raw : 0);
        if (observation)
            raw->flags = next_raw.state.flags;
        if (task_merge(&end, &next, data, &output))
            return finish_sentry_task(end, observation);
    }
    uint16_t owner = (work->target & 0xff00) | current_sentry_slot;
    work->target = owner;
    if (end.outcome != SENTRY_TARGET_FOUND) {
        if (sentry_scan_pass[owner] < 2)
            sentry_stack_search_flags[owner] = 128;
        else
            sentry_absorb_delays[owner] = 0;
        end.route = SENTRY_FINISH;
        return finish_sentry_task(end, observation);
    }
    sentry_scan_delays[owner] = 50;
    work->source = (work->source & 0xff00) | sentry_meanie_slots[owner];
    end.route = SENTRY_CREATION;

    return finish_sentry_task(end, observation);
}

struct SentryEnd __far sentry_track_visible_target(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_sentry_track_visible_target(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

/* Stack searches share absorption handling; unlike a retained visible target,
 * a player-energy result returns before the scan cooldown is written. */
static void task_absorb_stack(struct SentryTask *end,
    SegmentBytes data,
    SegmentBytes output,
    uint16_t source,
    uint16_t target,
    struct SentryEnd *observation)
{
    struct ActorState *work = &end->state;
    struct SentryEnd absorbed_raw;
    if (observation)
        absorbed_raw.state.flags = observation->state.flags;
    struct SentryTask absorbed = task_sentry_absorb_unit(
        data, output, source, target, work->backwards, observation ? &absorbed_raw : 0);
    if (observation)
        observation->state.flags = absorbed_raw.state.flags;
    if (task_merge(end, &absorbed, data, &output))
        return;
    if (absorbed.outcome == SENTRY_PLAYER_DRAINED) {
        end->route = SENTRY_FINISH;
        return;
    }
    uint16_t owner = (work->target & 0xff00) | current_sentry_slot;
    work->target = owner;
    sentry_scan_delays[owner] = 30;
    end->route = SENTRY_CREATION;
}

struct SentryTask __far task_sentry_scan_world(SegmentBytes data,
    SegmentBytes output,
    uint16_t source_slot,
    uint16_t target_slot,
    int backwards,
    struct SentryEnd *observation)
{
    struct SentryTask end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    work->source = source_slot;
    work->target = target_slot;

    view_object_slot = (uint8_t)source_slot;
    {
        struct SentryEnd replanted_raw;
        if (observation)
            replanted_raw.state.flags = raw->flags;
        struct SentryTask replanted = task_sentry_replant(data,
            output,
            source_slot,
            target_slot,
            work->backwards,
            observation ? &replanted_raw : 0);
        if (observation)
            raw->flags = replanted_raw.state.flags;
        if (task_merge(&end, &replanted, data, &output))
            return finish_sentry_task(end, observation);
    }
    if (end.outcome == SENTRY_TREE_CREATED) {
        end.route = SENTRY_CREATION;
        return finish_sentry_task(end, observation);
    }
    uint16_t source = (work->source & 0xff00) | current_sentry_slot;
    uint16_t target = work->target;
    work->source = source;

    /* A requested stack search precedes rechecking the retained target. */
    if (sentry_stack_search_flags[source] & 128) {
        struct SentryEnd stack_raw;
        if (observation)
            stack_raw.state.flags = raw->flags;
        struct SentryTask stack = task_sentry_find_stack_energy(
            data, output, source, target, work->backwards, observation ? &stack_raw : 0);
        if (observation)
            raw->flags = stack_raw.state.flags;
        if (task_merge(&end, &stack, data, &output))
            return finish_sentry_task(end, observation);
        source = (work->source & 0xff00) | current_sentry_slot;
        target = work->target;
        work->source = source;
        if (end.outcome == SENTRY_TARGET_FOUND) {
            sentry_scan_cursor[source] = 64;
            task_absorb_stack(&end, data, output, source, target, observation);
            return finish_sentry_task(end, observation);
        }
        sentry_stack_search_flags[source] &= 127;
    }
    /* The countdown snapshot and the following live branch are separate reads. */
    (void)sentry_absorb_delays[source];
    if (sentry_absorb_delays[source]) {
        target = (target & 0xff00) | sentry_target_slots[source];
        struct SightIndices sight = test_sentry_visibility(data, 0, source, target, target);
        task_sight(&end, &sight);
        source = sight.observer;
        target = sight.target;
        if (sentry_target_sight_result) {
            struct SentryEnd tracked_raw;
            if (observation)
                tracked_raw.state.flags = raw->flags;
            struct SentryTask tracked = task_sentry_track_visible_target(
                data, output, source, target, work->backwards, observation ? &tracked_raw : 0);
            if (observation)
                raw->flags = tracked_raw.state.flags;
            task_merge(&end, &tracked, data, &output);
            return finish_sentry_task(end, observation);
        }
        sentry_absorb_delays[source] = 0;
    }

    /* Search down through object IDs, carrying forward the banks returned by
     * sight. Remember a partly visible player in the live terrain scratch. */
    segment_record(data, terrain_work).corner_column = 128;
    target = (target & 0xff00) | 63;
    do {
        struct SightIndices sight = test_sentry_visibility(data, 0, source, target, target);
        task_sight(&end, &sight);
        source = sight.observer;
        target = sight.target;
        if (!(sentry_intervening_sight_bits & 64) && sentry_target_sight_result) {
            if (sentry_target_sight_result & 128) {
                struct SentryEnd tracked_raw;
                if (observation)
                    tracked_raw.state.flags = raw->flags;
                struct SentryTask tracked = task_sentry_track_visible_target(
                    data, output, source, target, work->backwards, observation ? &tracked_raw : 0);
                if (observation)
                    raw->flags = tracked_raw.state.flags;
                task_merge(&end, &tracked, data, &output);
                return finish_sentry_task(end, observation);
            }
            if ((uint8_t)target == player_robot_slot)
                segment_record(data, terrain_work).corner_column = (uint8_t)target;
        }
        target = (target & 0xff00) | (uint8_t)(target - 1);
    } while (!((uint8_t)target & 128));
    target = (target & 0xff00) | segment_record(data, terrain_work).corner_column;
    work->target = target;
    if (!(segment_record(data, terrain_work).corner_column & 128) &&
        (uint8_t)target != sentry_scan_target[source]) {
        c_reset_sentry_scan(source);
        sentry_target_sight_result = SIGHT_TARGET_VISIBLE;
        struct SentryEnd tracked_raw;
        if (observation)
            tracked_raw.state.flags = raw->flags;
        struct SentryTask tracked = task_sentry_track_visible_target(
            data, output, source, target, work->backwards, observation ? &tracked_raw : 0);
        if (observation)
            raw->flags = tracked_raw.state.flags;
        task_merge(&end, &tracked, data, &output);
        return finish_sentry_task(end, observation);
    }

    /* With no acquired target, try stack energy before considering a turn. */
    sentry_absorb_delays[source] = 0;
    {
        struct SentryEnd stack_raw;
        if (observation)
            stack_raw.state.flags = raw->flags;
        struct SentryTask stack = task_sentry_find_stack_energy(
            data, output, source, target, work->backwards, observation ? &stack_raw : 0);
        if (observation)
            raw->flags = stack_raw.state.flags;
        if (task_merge(&end, &stack, data, &output))
            return finish_sentry_task(end, observation);
    }
    source = work->source;
    target = work->target;
    if (end.outcome == SENTRY_TARGET_FOUND) {
        task_absorb_stack(&end, data, output, source, target, observation);
        return finish_sentry_task(end, observation);
    }

    source = (source & 0xff00) | current_sentry_slot;
    work->source = source;
    (void)sentry_rotation_delays[source];
    if (sentry_rotation_delays[source] >= 2) {
        end.route = SENTRY_FINISH;
        return finish_sentry_task(end, observation);
    }
    if (task_guard(&end, data, (uint8_t)source, source, target, observation)) {
        end.route = SENTRY_ABORT;
        return finish_sentry_task(end, observation);
    }
    uint16_t turning_sentry = work->source;
    object_heading[turning_sentry] += sentry_rotation_steps[turning_sentry];
    sentry_rotation_delays[turning_sentry] = 200;
    c_reset_sentry_scan(turning_sentry);
    task_sound(&end, 0, observation);
    work->source = (work->source & 0xff00) | current_sentry_slot;
    end.route = SENTRY_CHANGE;
    return finish_sentry_task(end, observation);
}

struct SentryEnd __far c_sentry_scan_world(
    SegmentBytes data, SegmentBytes output, uint16_t source, uint16_t target, uint16_t flags)
{
    struct SentryEnd observation = {0};
    observation.state.flags = flags;
    task_sentry_scan_world(data, output, source, target, !!(flags & 0x400), &observation);
    return observation;
}

/* Foreground C callers unwind a deferred mutation by returning here. Only the
 * native entry adapter needs to discard the original intermediate stack. */
struct ActorState __far run_sentry_schedule(
    SegmentBytes data, SegmentBytes output, int backwards, struct SentryState *observation)
{
    struct SentryEnd raw;
    if (observation)
        raw.state.flags = observation->flags;
    struct SentryTask state =
        task_sentry_schedule_start(data, output, 0, 0, backwards, observation ? &raw : 0);
    for (;;) {
        uint16_t source = state.state.source;
        uint16_t target = state.state.target;
        if (state.state.data_output)
            output = data;
        struct SentryTask next;
        switch (state.route) {
        case SENTRY_ACTIVE:
            next = task_sentry_active_dispatch(
                data, output, source, target, state.state.backwards, observation ? &raw : 0);
            break;
        case SENTRY_MEANIE:
            next = task_sentry_active_meanie(
                data, output, source, target, state.state.backwards, observation ? &raw : 0);
            break;
        case SENTRY_SCAN:
            next = task_sentry_scan_world(
                data, output, source, target, state.state.backwards, observation ? &raw : 0);
            break;
        case SENTRY_EXPIRE:
            next = task_meanie_expire(
                data, output, source, target, state.state.backwards, observation ? &raw : 0);
            break;
        case SENTRY_TURN:
            next = task_meanie_turn(
                data, output, source, target, state.state.backwards, observation ? &raw : 0);
            break;
        case SENTRY_CREATION:
            next = task_sentry_publish_creation(
                data, output, source, target, state.state.backwards, observation ? &raw : 0);
            break;
        case SENTRY_CHANGE:
            next = task_sentry_publish_change(
                data, output, source, target, state.state.backwards, observation ? &raw : 0);
            break;
        case SENTRY_PLAYER_LOST:
            game_end_flags = 128;
            goto finish;
        case SENTRY_RETRY:
            c_sentry_retry_after_redraw(source);
            goto finish;
        default:
            goto finish;
        }
        task_merge(&state, &next, data, &output);
    }
finish:;
    struct SentryClockEnd clock;
    advance_sentry_clock(observation ? &clock : 0);
    if (observation) {
        raw.state.flags =
            (subtract8(raw.state.flags, clock.previous, 1) & ~CF) | (clock.carry ? CF : 0);
        *observation = raw.state;
        observation->source = state.state.source;
        observation->target = state.state.target;
        observation->data_output = state.state.data_output;
    }
    return state.state;
}

struct SentryState __far c_run_sentry_schedule(
    SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    struct SentryState raw = {0};
    raw.flags = flags;
    run_sentry_schedule(data, output, !!(flags & 0x400), &raw);
    return raw;
}

#endif
