#ifndef SENTINEL_SOUND_STATE_H
#define SENTINEL_SOUND_STATE_H
#include <stdint.h>
#include <stddef.h>

/* Decode local snapshots only. Bits6/7 remain uninterpreted; a whole
 * nonzero raw byte, rather than any designated bit, keeps a voice active. */
union __attribute__((packed, may_alias)) SoundControl {
    uint8_t raw;

    struct __attribute__((packed)) {
        uint8_t voice : 2;
        uint8_t noise : 1;
        uint8_t octave_reduction : 3;
        uint8_t uninterpreted : 2;
    } decoded;
};

_Static_assert(sizeof(union SoundControl) == 1, "sound control byte");

/* Signed pitch/envelope deltas retain one-byte storage. Volume sums wrap
 * to a byte before the sign-bit/clamp tests; periods and counts stay unsigned. */
struct SoundEnvelope {
    uint8_t period;
    int8_t step;
    uint8_t steps;
    int8_t next_step;
    uint8_t next_steps;
};

struct SoundPitchStep {
    int8_t step;
    uint8_t steps;
};

struct __attribute__((packed, may_alias)) SoundEffect {
    union SoundControl control;
    uint8_t level, pitch;
    struct SoundEnvelope envelope;
    struct SoundPitchStep first_pitch;
    uint8_t pitch_period, next_envelope_period;
    struct SoundPitchStep later_pitch[2];
};

struct __attribute__((packed, may_alias)) SoundVoice {
    struct SoundEffect effect;
    struct SoundPitchStep current_pitch;
    uint8_t reserved18[2];
    uint8_t envelope_countdown, pitch_countdown, pitch_phase;
    uint8_t reserved23[9];
};

_Static_assert(sizeof(struct SoundEnvelope) == 5, "sound envelope size");
_Static_assert(sizeof(struct SoundPitchStep) == 2, "pitch step size");
_Static_assert(sizeof(struct SoundEffect) == 16, "effect definition size");
_Static_assert(sizeof(struct SoundVoice) == 32, "live voice size");
_Static_assert(offsetof(struct SoundEffect, control) == 0, "effect control offset");
_Static_assert(offsetof(struct SoundEffect, level) == 1, "effect level offset");
_Static_assert(offsetof(struct SoundEffect, pitch) == 2, "effect pitch offset");
_Static_assert(offsetof(struct SoundEffect, envelope.period) == 3, "effect envelope.period offset");
_Static_assert(offsetof(struct SoundEffect, envelope.step) == 4, "effect envelope.step offset");
_Static_assert(offsetof(struct SoundEffect, envelope.steps) == 5, "effect envelope.steps offset");
_Static_assert(
    offsetof(struct SoundEffect, envelope.next_step) == 6, "effect envelope.next_step offset");
_Static_assert(
    offsetof(struct SoundEffect, envelope.next_steps) == 7, "effect envelope.next_steps offset");
_Static_assert(
    offsetof(struct SoundEffect, first_pitch.step) == 8, "effect first_pitch.step offset");
_Static_assert(
    offsetof(struct SoundEffect, first_pitch.steps) == 9, "effect first_pitch.steps offset");
_Static_assert(offsetof(struct SoundEffect, pitch_period) == 10, "effect pitch_period offset");
_Static_assert(
    offsetof(struct SoundEffect, next_envelope_period) == 11, "effect next_envelope_period offset");
_Static_assert(
    offsetof(struct SoundEffect, later_pitch[0].step) == 12, "effect later_pitch[0].step offset");
_Static_assert(
    offsetof(struct SoundEffect, later_pitch[0].steps) == 13, "effect later_pitch[0].steps offset");
_Static_assert(
    offsetof(struct SoundEffect, later_pitch[1].step) == 14, "effect later_pitch[1].step offset");
_Static_assert(
    offsetof(struct SoundEffect, later_pitch[1].steps) == 15, "effect later_pitch[1].steps offset");
_Static_assert(offsetof(struct SoundVoice, effect) == 0, "voice effect offset");
_Static_assert(
    offsetof(struct SoundVoice, current_pitch.step) == 16, "voice current_pitch.step offset");
_Static_assert(
    offsetof(struct SoundVoice, current_pitch.steps) == 17, "voice current_pitch.steps offset");
_Static_assert(offsetof(struct SoundVoice, reserved18) == 18, "voice reserved18 offset");
_Static_assert(
    offsetof(struct SoundVoice, envelope_countdown) == 20, "voice envelope_countdown offset");
_Static_assert(offsetof(struct SoundVoice, pitch_countdown) == 21, "voice pitch_countdown offset");
_Static_assert(offsetof(struct SoundVoice, pitch_phase) == 22, "voice pitch_phase offset");
_Static_assert(offsetof(struct SoundVoice, reserved23) == 23, "voice reserved23 offset");

struct SoundStorage {
    struct SoundEffect effects[7];
    struct SoundVoice voices[3];
};
extern struct SoundStorage sound_storage;
#endif
