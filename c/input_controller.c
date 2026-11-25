#include <stdint.h>
#include "register_compat.h"
#include "typed_api.h"
#include "legacy_targets.h"
#include "data_aliases.h"
#include "render_state.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"

/* Timer-side cursor decisions and the foreground action wait. Cursor mode is
 * toggled by Space; mouse activation and the activation-click latch stay distinct. */

#include "input_state.h"

void __far c_keyboard_cursor_horizontal(void)
{
    uint8_t direction = input_action_slots[INPUT_SLOT_HORIZONTAL];
    uint8_t x = camera_cursor_x_pairs;
    if (direction & 128)
        return;
    if (!direction) {
        ++x;
        software_cursor_x_pairs = x;
        if (x >= 144) {
            x -= 64;
            camera_transition_request = direction;
        }
    } else {
        --x;
        software_cursor_x_pairs = x;
        if (x < 16) {
            x += 64;
            camera_transition_request = direction;
        }
    }
    camera_cursor_x_pairs = x;
}

void __far c_keyboard_cursor_vertical(uint16_t robot_bank)
{
    uint8_t direction = input_action_slots[INPUT_SLOT_VERTICAL];
    uint8_t y = camera_cursor_inverted_y;
    uint8_t pitch = object_view_pitch[robot_bank | player_robot_slot];
    if (direction & 128)
        return;
    if (direction == INPUT_DIRECTION_UP) {
        ++y;
        software_cursor_inverted_y = y;
        if (y >= 160) {
            if (pitch == camera_transition_pitch_targets[0])
                goto blocked;
            y -= 64;
            camera_transition_request = direction;
        }
    } else {
        --y;
        software_cursor_inverted_y = y;
        if (y < 32) {
            if (pitch == camera_transition_pitch_targets[1])
                goto blocked;
            y += 64;
            camera_transition_request = direction;
        }
    }
    camera_cursor_inverted_y = y;
    return;
blocked:
    software_cursor_inverted_y = camera_cursor_inverted_y;
}

CursorDrawCallback __far c_keyboard_cursor_move(SegmentBytes data, uint16_t robot_bank)
{
    software_cursor_x_pairs = camera_cursor_x_pairs;
    software_cursor_inverted_y = camera_cursor_inverted_y;
    c_keyboard_cursor_horizontal();
    uint8_t transition = camera_transition_request;
    if (transition & 128)
        c_keyboard_cursor_vertical(robot_bank);
    (void)cursor_update_guard_flags;
    if (!(cursor_update_guard_flags & 128))
        return segment_record(data, graphics_session).draw_cursor;
    return 0;
}

uint8_t __far c_input_snapshot(void)
{
    uint8_t ready = input_request_ready_flags;
    input_ready_snapshot = ready;
    return ready;
}

uint8_t __far c_input_ready(void)
{
    input_request_ready_flags = selection_copy_interrupt_flags = 128;
    return c_input_snapshot();
}

static int scan_pressed(uint16_t scan)
{
    return c_test_scan(scan) == 255;
}

/* Keep the live transition reads, but no discarded accumulator/FLAGS result. */
static void finish_cursor_transition(void)
{
    (void)camera_transition_request;
    (void)camera_transition_request;
    if (camera_transition_request & 128)
        c_input_snapshot();
    else {
        held_direction_action = camera_transition_request;
        c_input_ready();
    }
}

/* These callback outputs still steer real later lookups. Other cursor
 * observations only recreated registers restored by the IRQ epilogue. */
static void input_cursor_banks(
    const struct CursorEffects *cursor, uint16_t *lookup_bank, uint16_t *binding_bank)
{
    if (!cursor->invoked)
        return;
    if (cursor->restoring) {
        *lookup_bank = 0;
        if (cursor->mode == 2)
            *binding_bank = 0;
    } else {
        if (cursor->executed)
            *lookup_bank = cursor->lookup_bank;
        if (cursor->mode >= 2)
            *binding_bank = 0;
    }
}

void __far c_update_cursor_input(SegmentBytes data,
    SegmentBytes scans,
    uint16_t observer_bank,
    uint16_t binding_bank,
    int backwards)
{
    uint16_t lookup_bank = observer_bank & 0xff00;
    binding_bank &= 0xff00;
    int preserve_toggle_latch = 0;
    camera_transition_request = INPUT_ACTION_NONE;
    if (scan_pressed(lookup_bank | 1))
        restart_requested_flags |= 128;
    if (scan_pressed(lookup_bank | 0x39) || scan_pressed(lookup_bank | 0x77)) {
        mouse_cursor_mode_flags = mouse_moved = 0;
        if (!(cursor_update_guard_flags & 128)) {
            preserve_toggle_latch = 1;
            if (!cursor_mode_toggle_latch) {
                cursor_mode_flags ^= 128;
                if (cursor_mode_flags & 128) {
                    c_center_cursor();
                    struct CursorEffects cursor = operate_cursor(data, lookup_bank, 0, 0, 0);
                    input_cursor_banks(&cursor, &lookup_bank, &binding_bank);
                } else {
                    struct CursorEffects cursor = operate_cursor(data, lookup_bank, 1, 0, 0);
                    input_cursor_banks(&cursor, &lookup_bank, &binding_bank);
                }
                cursor_mode_toggle_latch = 128;
            }
        }
    } else if (mouse_cursor_mode_flags && (cursor_mode_flags & 128)) {
        if (!(input_buttons & 1))
            mouse_activation_click_latch = 0;
    } else if ((input_buttons & 1) && !(cursor_update_guard_flags & 128)) {
        mouse_cursor_mode_flags = cursor_mode_flags = mouse_activation_click_latch = 128;
        c_mouse_view_edges(lookup_bank | player_robot_slot);
        struct CursorEffects cursor = operate_cursor(data, lookup_bank, 0, 0, 0);
        input_cursor_banks(&cursor, &lookup_bank, &binding_bank);
    }
    if (!preserve_toggle_latch)
        cursor_mode_toggle_latch = 0;
    uint8_t combined = collect_actions(scans, backwards, binding_bank | 19, lookup_bank, 0);
    lookup_bank &= 0xff00;
    if (combined & 128) {
        if (mouse_cursor_mode_flags) {
            if (mouse_moved) {
                c_mouse_view_edges(lookup_bank | player_robot_slot);
                if (!(cursor_update_guard_flags & 128)) {
                    struct CursorEffects cursor = operate_cursor(data, lookup_bank, 0, 0, 0);
                    input_cursor_banks(&cursor, &lookup_bank, &binding_bank);
                    mouse_moved = 0;
                }
                finish_cursor_transition();
                return;
            }
            if (!mouse_activation_click_latch && (input_buttons & 1)) {
                input_buttons = 0;
                input_action_slots[INPUT_SLOT_OBJECT] = OBJECT_ACTION_ABSORB;
            } else if (input_buttons & 2) {
                input_buttons = 0;
                input_action_slots[INPUT_SLOT_OBJECT] = OBJECT_ACTION_TRANSFER;
            }
        } else {
            mouse_moved = 0;
            cursor_repeat_phase = 0x6b;
        }
        (void)input_action_slots[INPUT_SLOT_OBJECT];
        if (!(input_action_slots[INPUT_SLOT_OBJECT] & 128)) {
            c_input_ready();
            return;
        }
        turn_and_occlusion_flags = 64;
        c_input_snapshot();
        return;
    }
    mouse_cursor_mode_flags = mouse_moved = 0;
    (void)cursor_mode_flags;
    (void)cursor_mode_flags;
    if (cursor_mode_flags & 128) {
        uint8_t repeat_phase = cursor_repeat_phase;
        cursor_repeat_phase <<= 1;
        if (repeat_phase & 128) {
            c_input_snapshot();
            return;
        }
        CursorDrawCallback draw = c_keyboard_cursor_move(data, lookup_bank);
        if (draw) {
            /* The operation performs the original second live binding read. */
            struct CursorEffects cursor = operate_cursor(data, lookup_bank, 0, 0, 0);
            input_cursor_banks(&cursor, &lookup_bank, &binding_bank);
        }
    } else {
        uint8_t action = input_action_slots[INPUT_SLOT_HORIZONTAL];
        if (action & 128) {
            action = input_action_slots[INPUT_SLOT_VERTICAL];
            if (action & 128) {
                c_input_snapshot();
                return;
            }
        }
        camera_transition_request = action;
    }
    finish_cursor_transition();
}

void __far c_input_check_draw_interrupt(
    SegmentBytes output, int backwards, uint16_t first_binding, uint16_t scan_bank)
{
    uint8_t saved = input_action_slots[INPUT_SLOT_OBJECT];
    collect_actions(output, backwards, first_binding, scan_bank, 0);
    input_action_slots[INPUT_SLOT_OBJECT] = saved;
    if (mouse_cursor_mode_flags)
        c_mouse_edge_actions();
    uint8_t stopped = 0;
    (void)held_direction_action;
    if (held_direction_action != input_action_slots[INPUT_SLOT_HORIZONTAL] &&
        held_direction_action != input_action_slots[INPUT_SLOT_VERTICAL]) {
        stopped = 128;
        (void)held_direction_action;
        (void)input_action_slots[INPUT_SLOT_VERTICAL];
    }
    terrain_draw_stop = stopped;
}

uint16_t __far c_timer_tick(SegmentBytes data, SegmentBytes scans, uint16_t flags)
{
    uint16_t irq_control = flags & ~ARITH_FLAGS;
    uint16_t lookup_bank = 0;
    uint16_t warning_port = 0;
    if ((int8_t)--video_transition_countdown < 0)
        ++video_transition_countdown;
    if ((int8_t)--sound_trigger_countdown < 0)
        ++sound_trigger_countdown;
    struct SoundTickEffects sound = c_tick_sound_sequence(data);
    lookup_bank = sound.lookup_bank;
    warning_port = sound.warning_port;
    if (sound.interrupts_enabled)
        irq_control |= 0x200;
    c_tick_voices();
    if (game_timer_suspend_flags & 128) {
        (void)selection_model_draw_flags;
        if (selection_model_draw_flags & 128) {
            uint8_t scatter = scatter_effect(data, 200, 0, 0);
            if (scatter & 2)
                irq_control |= 0x200;
        }
        return irq_control;
    }
    if (game_end_flags & 128) {
        collect_actions(scans, !!(irq_control & 0x400), 17, lookup_bank, 0);
        segment_record(scans, input_work).action_slots[INPUT_SLOT_HORIZONTAL] =
            segment_record(scans, input_work).action_slots[INPUT_SLOT_OBJECT] =
                segment_record(scans, input_work).action_slots[INPUT_SLOT_VERTICAL] =
                    INPUT_ACTION_NONE;
        return irq_control;
    }
    if (game_pause_flags & 128)
        return irq_control;
    if (!(sentry_clock_gate_flags & 128)) {
        struct CountdownTick tick = c_sentry_countdowns();
        if ((unsigned)tick.previous_phase + 0xa1 <= 255)
            lookup_bank = 0;
    }
    uint8_t warning = show_warning(data, !!(irq_control & 0x400), warning_port, 0);
    if (warning & 1) {
        scans = data;
        /* The accepted pattern is 0/4/8 and noise adds at most three;
         * both the byte and doubled-word lookup indices have a zero bank. */
        if (warning & 2)
            lookup_bank = 0;
    }
    if (input_request_ready_flags & 128) {
        c_input_check_draw_interrupt(scans, !!(irq_control & 0x400), 7, lookup_bank);
        return irq_control;
    }
    if (input_poll_delay_ticks && --input_poll_delay_ticks)
        return irq_control;
    c_update_cursor_input(data, scans, lookup_bank, 0, !!(irq_control & 0x400));
    return irq_control;
}

/* An interrupt can make input ready before the first scheduler iteration.
 * Track the two optional index updates without carrying raster observations. */
static void wait_effect(struct ActionWait *wait, const struct RenderEffects *effects)
{
    actor_render(&wait->state, effects);
    wait->source_updated |= effects->source_updated;
    wait->target_updated |= effects->target_updated;
}

static struct ActionWait finish_action_wait(struct ActionWait wait, struct PlayerWaitEnd *raw)
{
    if (raw) {
        raw->state.source = wait.state.source;
        raw->state.target = wait.state.target;
        raw->state.data_output = wait.state.data_output;
        raw->source_updated = wait.source_updated;
        raw->target_updated = wait.target_updated;
        raw->outcome = wait.outcome;
    }
    return wait;
}

struct ActionWait __far wait_player_action(
    SegmentBytes data, SegmentBytes output, int backwards, struct PlayerWaitEnd *observation)
{
    struct ActionWait end = {0};
    struct ActorState *work = &end.state;
    struct SentryState *raw = observation ? &observation->state : 0;
    work->backwards = backwards;
    for (;;) {
        input_request_ready_flags = turn_and_occlusion_flags = 0;
        mouse_moved = mouse_cursor_mode_flags;
        if (mouse_cursor_mode_flags)
            cursor_mode_flags = 128;
        input_poll_delay_ticks = 2;
        while (!(input_request_ready_flags & 128)) {
            partial_view_update_flags &= 127;
            if ((input_ready_snapshot & 128) && !terrain_draw_stop)
                partial_view_update_flags |= 128;
            struct ActorState sentry = run_sentry_schedule(data, output, work->backwards, raw);
            actor_merge(work, &sentry);
            end.source_updated = end.target_updated = 1;
            if (work->data_output)
                output = data;
            int refresh_view;
            if (game_end_flags) {
                struct RenderEnd effect;
                if (observation) {
                    effect = (struct RenderEnd){0};
                    effect.polygon.flags = raw->flags;
                }
                struct RenderEffects effects = present_end_effects(
                    data, output, 30, work->backwards, observation ? &effect : 0);
                wait_effect(&end, &effects);
                if (observation)
                    raw->flags = effect.polygon.flags;
                refresh_view = 1;
            } else {
                uint8_t refresh = player_view_refresh_flags;
                player_view_refresh_flags <<= 1;
                refresh_view = (refresh & 128) || (restart_requested_flags & 128);
            }
            if (refresh_view) {
                uint8_t ready = c_input_ready();
                if (observation)
                    raw->flags = status(raw->flags, ready, 8) | CF;
                end.outcome = PLAYER_WAIT_SCENE_REFRESH;
                return finish_action_wait(end, observation);
            }
            struct WarningAggregateEnd warning = c_aggregate_sentry_warning(work->source);
            work->target = warning.warning;
            work->source = warning.last_slot;
            if (observation) {
                raw->flags = subtract8(raw->flags, warning.warning, warning.previous_mode);
                if (warning.warning != warning.previous_mode)
                    raw->flags = subtract8(raw->flags, warning.previous_mode, 3);
                if (warning.silenced)
                    raw->flags = status(raw->flags, warning.control, 8) | 0x200;
            }
            work->interrupts_enabled |= warning.silenced;
            check_game_pause(data, work->backwards, work->target);
        }
        uint8_t transition = camera_transition_request;
        if (observation)
            raw->flags = status(raw->flags, transition, 8);
        if (!(camera_transition_request & 128)) {
            if (observation)
                raw->flags &= ~CF;
            return finish_action_wait(end, observation);
        }
        if (!(input_action_slots[INPUT_SLOT_OBJECT] & 128)) {
            uint8_t action = input_action_slots[INPUT_SLOT_OBJECT];
            if (action >= OBJECT_ACTION_HYPERSPACE || (cursor_mode_flags & 128)) {
                player_action_code = action;
                sentry_clock_gate_flags &= 127;
                uint8_t control = c_silence_voice(0);
                work->interrupts_enabled = 1;
                if (observation)
                    raw->flags = status(raw->flags, control, 8) | 0x200;
                struct PlayerActionEnd action_raw;
                if (observation) {
                    action_raw = (struct PlayerActionEnd){0};
                    action_raw.render.polygon.flags = raw->flags;
                }
                struct ActorAction result = execute_actor_action(data,
                    work->source,
                    work->target,
                    work->backwards,
                    observation ? &action_raw : 0);
                actor_merge(work, &result.state);
                end.source_updated = end.target_updated = 1;
                if (observation)
                    raw->flags = action_raw.render.polygon.flags;
                if (work->data_output)
                    output = data;
                if (result.outcome == PLAYER_ACTION_CHANGED) {
                    uint8_t sound = c_sound_effect(2);
                    work->interrupts_enabled = 1;
                    if (observation)
                        raw->flags = status(raw->flags, sound, 8) | 0x200;
                    selection_presentation_flags = 192;
                    selection_copy_interrupt_flags &= 127;
                    struct RenderEnd selected;
                    if (observation) {
                        selected = (struct RenderEnd){0};
                        selected.polygon.flags = raw->flags;
                    }
                    struct RenderEffects effects = present_selected_effects(data,
                        output,
                        work->source,
                        work->target,
                        work->backwards,
                        observation ? &selected : 0);
                    wait_effect(&end, &effects);
                    if (observation)
                        raw->flags = selected.polygon.flags;
                    if (work->data_output)
                        output = data;
                    c_silence_voice(0);
                    struct RenderEnd hud;
                    if (observation) {
                        hud = (struct RenderEnd){0};
                        hud.polygon.flags = raw->flags;
                    }
                    effects = gameplay_hud(data, 0, work->backwards, observation ? &hud : 0);
                    wait_effect(&end, &effects);
                    if (observation)
                        raw->flags = hud.polygon.flags;
                    if (work->data_output)
                        output = data;
                }
                uint8_t refresh = player_view_refresh_flags;
                player_view_refresh_flags &= 127;
                if (refresh & 128) {
                    if (observation)
                        raw->flags |= CF;
                    end.outcome = PLAYER_WAIT_SCENE_REFRESH;
                    return finish_action_wait(end, observation);
                }
            }
        }
    }
}

struct PlayerWaitEnd __far c_player_wait_action(
    SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    struct PlayerWaitEnd raw = {0};
    raw.state.flags = flags;
    wait_player_action(data, output, !!(flags & 0x400), &raw);
    return raw;
}

uint8_t __far c_center_cursor(void)
{
    camera_cursor_x_pairs = software_cursor_x_pairs = 80;
    camera_cursor_inverted_y = software_cursor_inverted_y = 95;
    return 95;
}

/* Owned subsystem data. */
volatile uint8_t cursor_mode_toggle_latch = 0x0;
uint8_t input_key_bindings[20] = {
    0x4b,
    0x4d,
    0x48,
    0x50,
    0x7a,
    0x79,
    0x7c,
    0x7b,
    0x1e,
    0x10,
    0x13,
    0x14,
    0x30,
    0x23,
    0x3c,
    0x3c,
    0x3e,
    0x3d,
    0x16,
    0x3b,
};
uint8_t input_binding_actions[20] = {
    INPUT_BINDING(INPUT_DIRECTION_LEFT, INPUT_SLOT_HORIZONTAL),
    INPUT_BINDING(INPUT_DIRECTION_RIGHT, INPUT_SLOT_HORIZONTAL),
    INPUT_BINDING(INPUT_DIRECTION_UP, INPUT_SLOT_VERTICAL),
    INPUT_BINDING(INPUT_DIRECTION_DOWN, INPUT_SLOT_VERTICAL),
    INPUT_BINDING(INPUT_DIRECTION_LEFT, INPUT_SLOT_HORIZONTAL),
    INPUT_BINDING(INPUT_DIRECTION_RIGHT, INPUT_SLOT_HORIZONTAL),
    INPUT_BINDING(INPUT_DIRECTION_UP, INPUT_SLOT_VERTICAL),
    INPUT_BINDING(INPUT_DIRECTION_DOWN, INPUT_SLOT_VERTICAL),
    INPUT_BINDING(OBJECT_ACTION_ABSORB, INPUT_SLOT_OBJECT),
    INPUT_BINDING(OBJECT_ACTION_TRANSFER, INPUT_SLOT_OBJECT),
    INPUT_BINDING(MODEL_ROBOT, INPUT_SLOT_OBJECT),
    INPUT_BINDING(MODEL_TREE, INPUT_SLOT_OBJECT),
    INPUT_BINDING(MODEL_BOULDER, INPUT_SLOT_OBJECT),
    INPUT_BINDING(OBJECT_ACTION_HYPERSPACE, INPUT_SLOT_OBJECT),
    /* Commands 0/1 occupy this slot but are not consumed by c_pause_check. */
    INPUT_BINDING(0, INPUT_SLOT_PAUSE),
    INPUT_BINDING(1, INPUT_SLOT_PAUSE),
    INPUT_BINDING(PAUSE_COMMAND_PAUSE, INPUT_SLOT_PAUSE),
    INPUT_BINDING(PAUSE_COMMAND_RESUME, INPUT_SLOT_PAUSE),
    INPUT_BINDING(OBJECT_ACTION_TURN, INPUT_SLOT_OBJECT),
    INPUT_BINDING(OBJECT_ACTION_OVERVIEW, INPUT_SLOT_OBJECT),
};

/* InputWork is reset independently of other subsystems and data placement. */
struct InputWork input_work = {0};

void __far reset_input_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&input_work), 0, sizeof(input_work), 0);
}

/* Successful F1 overview request sets 80h. The main controller suspends game updates, renders the displaced camera view, waits for a fresh key/click, clears this flag and returns to the normal view. */
uint8_t overview_pending = 0x0;

/* Temporary camera object slot saved by the overview renderer at 1D7Fh. It receives a viewpoint displaced from the player using ray components and becomes view_object_slot for the overview; this is separate from player_robot_slot. */
uint8_t overview_camera_slot = 0x0;

/* Initial input-poll delay in timer ticks: player_poll_actions sets 2; the timer decrements a nonzero value and polls when it reaches zero. Once zero, subsequent eligible timer calls poll immediately. */
volatile uint8_t input_poll_delay_ticks = 0x0;

/* Suppresses absorbing on the same left-button press that enabled mouse cursor mode. Activation sets 80h; observing a left-button release clears it; only a later left press with this latch clear produces ACTION_ABSORB. Right-button transfer has its own path. */
volatile uint8_t mouse_activation_click_latch = 0x0;

/* Mouse-driven cursor-mode latch, separate from cursor_mode_flags. A left click enables it; keyboard cursor toggling or directional input clears it. Input collection uses it to choose mouse movement/button handling, and player_poll_actions restores cursor mode while this latch is active. */
volatile union WorkWord mouse_cursor_mode_flags_storage = {0};

/* Mouse corner-edge arbitration selector, read without modification by this helper. The helper tests the sign of uint8(value-2) in CL without changing storage. Values 00h..01h or 82h..FFh prefer vertical edges; 02h..81h preserve an existing horizontal request. Vertical requests still obey pitch limits. The CGA loss trace reads zero 1,889 times (two cursor activations and 1,887 movement updates), so the local selector is FEh. Native tests cover all 256 values at all four corners and six controls. Workspace reset clears it; an independent nonzero writer remains unconfirmed. */
uint8_t mouse_axis_priority = 0x0;

/* Software-cursor update guard. Rendering/action paths set bit 7 around cursor restore/draw work; interrupt-driven cursor input checks the sign and defers updates while guarded. The action cleanup at 30C5h clears the guard. */
volatile uint8_t cursor_update_guard_flags = 0x0;

/* Cursor draw X coordinate in two-pixel groups. Backends divide by 2 for CGA bytes, use directly for Tandy bytes, divide by 4 for EGA bytes, or multiply by 2 for VGA bytes. */
uint8_t software_cursor_x_pairs = 0x0;

/* Cursor draw vertical anchor: backends use 199 minus this byte as the initial screen row before applying the cursor path deltas. */
uint8_t software_cursor_inverted_y = 0x0;

void __far reset_mouse_state(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&overview_pending), 0, sizeof(overview_pending), 0);
    c_memory_fill(output + ((uint16_t)&overview_camera_slot), 0, sizeof(overview_camera_slot), 0);
    c_memory_fill(
        output + ((uint16_t)&input_poll_delay_ticks), 0, sizeof(input_poll_delay_ticks), 0);
    c_memory_fill(output + ((uint16_t)&mouse_activation_click_latch),
        0,
        sizeof(mouse_activation_click_latch),
        0);
    c_memory_fill(output + ((uint16_t)&mouse_cursor_mode_flags_storage),
        0,
        sizeof(mouse_cursor_mode_flags_storage),
        0);
    c_memory_fill(output + ((uint16_t)&mouse_axis_priority), 0, sizeof(mouse_axis_priority), 0);
    c_memory_fill(
        output + ((uint16_t)&cursor_update_guard_flags), 0, sizeof(cursor_update_guard_flags), 0);
    c_memory_fill(
        output + ((uint16_t)&software_cursor_x_pairs), 0, sizeof(software_cursor_x_pairs), 0);
    c_memory_fill(
        output + ((uint16_t)&software_cursor_inverted_y), 0, sizeof(software_cursor_inverted_y), 0);
    c_memory_fill(output + ((uint16_t)&video_transition_busy_flags),
        0,
        sizeof(video_transition_busy_flags),
        0);
}
