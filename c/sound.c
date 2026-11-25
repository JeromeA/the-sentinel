#include <stddef.h>

/* Melody bytecode and the PC speaker's three software voices. Hardware access
 * is confined to port helpers; sequencing and effect records are ordinary C. */

static __attribute__((regparmcall)) uint16_t pitch_division(
    uint8_t pitch, uint8_t octave, struct PitchDivision *observation)
{
    uint8_t pitch_class = pitch % 48;
    octave = (uint8_t)(octave + pitch / 48);
    uint16_t divisor = speaker_pitch_divisors[pitch_class];
    int16_t shift = (int8_t)octave;
    if (observation) {
        observation->octave = shift;
        observation->carry = 0;
    }
    unsigned count = shift < 0 ? -shift : shift;
    if (count > 16)
        divisor = 0;
    else if (count) {
        if (shift > 0) {
            if (observation)
                observation->carry = (divisor >> (count - 1)) & 1;
            divisor = count == 16 ? 0 : divisor >> count;
        } else {
            if (observation)
                observation->carry = (divisor >> (16 - count)) & 1;
            divisor = count == 16 ? 0 : (uint16_t)(divisor << count);
        }
    }
    return divisor;
}

static void write_pitch(uint16_t divisor)
{
    port_out8(0x43, 0xb6);
    port_out8(0x42, (uint8_t)divisor);
    port_out8(0x42, divisor >> 8);
}

struct PitchDivision __far c_speaker_pitch(uint8_t pitch, uint8_t octave)
{
    struct PitchDivision end;
    end.divisor = pitch_division(pitch, octave, &end);
    write_pitch(end.divisor);
    return end;
}

/* Effect 2 is deliberately cut off by player_poll_actions when the selected-view
 * redraw returns. Its noise divisor starts at 5; only a sound tick substitutes
 * a PRNG-derived divisor. A redraw ending before that tick can produce just a
 * short transient. This also happens in the original executable. */
uint8_t __far c_sound_effect(int8_t index)
{
    if (index < 0 || index >= 13)
        return 0;
    const struct SoundEffect *effect = (const struct SoundEffect *)(sound_effect_definitions +
        index * (int)sizeof(struct SoundEffect));
    union SoundControl initial = {.raw = effect->control.raw};
    if (initial.decoded.voice >= 3)
        return 0;
    struct SoundVoice *voice = &sound_storage.voices[initial.decoded.voice];
    voice->effect.control.raw = 0;
    voice->effect.level = effect->level;
    /* Byte order matters when a definition aliases the destination voice. */
    unsigned field;
    for (field = 2; field < sizeof(struct SoundEffect); ++field)
        ((uint8_t *)voice)[field] = ((const uint8_t *)effect)[field];
    voice->current_pitch.step = voice->effect.first_pitch.step;
    voice->current_pitch.steps = voice->effect.first_pitch.steps;
    voice->envelope_countdown = voice->effect.envelope.period;
    voice->pitch_countdown = voice->effect.pitch_period;
    voice->pitch_phase = 0;
    __asm__ volatile("cli" ::: "memory");
    /* Aliased definitions may have changed during initialization. */
    union SoundControl playback = {.raw = effect->control.raw};
    if (playback.decoded.noise)
        write_pitch(((uint16_t)(uint8_t)~effect->pitch << 8) | 5);
    else {
        union SoundControl octave = {.raw = effect->control.raw};
        write_pitch(
            pitch_division(voice->effect.pitch, -(int8_t)octave.decoded.octave_reduction, 0));
    }
    __asm__ volatile("sti" ::: "memory");
    uint8_t observed = sound_muted;
    if (!sound_muted) {
        __asm__ volatile("cli" ::: "memory");
        uint8_t control = (port_in8(0x61) & 0xfc) | 3;
        port_out8(0x61, control);
        __asm__ volatile("sti" ::: "memory");
        observed = control;
        voice->effect.control.raw = effect->control.raw;
    }
    return observed;
}

uint8_t __far c_silence_voice(uint8_t voice)
{
    if (voice >= 3)
        return 0;
    sound_storage.voices[voice].effect.control.raw = 0;
    __asm__ volatile("cli" ::: "memory");
    uint8_t control = (port_in8(0x61) & 0xfc) | 1;
    port_out8(0x61, control);
    __asm__ volatile("sti" ::: "memory");
    return control;
}

uint8_t __far c_silence_all(void)
{
    uint8_t control = 0;
    int voice;
    for (voice = 2; voice >= 0; --voice)
        control = c_silence_voice(voice);
    return control;
}

static void advance_melody(
    uint16_t stream_bank, struct SoundTickEffects *effects, struct MelodyStep *observation)
{
    for (;;) {
        uint8_t cursor = sound_melody_cursor;
        uint16_t index = stream_bank | cursor;
        effects->lookup_bank = index & 0xff00;
        if (observation) {
            observation->cursor = index;
            observation->event = MELODY_IDLE;
            observation->payload.idle.scan_position = cursor;
        }
        if (cursor & 0x80)
            return;
        if (stream_bank || cursor >= sizeof(intro_theme_stream)) {
            sound_melody_cursor = 255;
            if (observation) {
                observation->event = MELODY_INVALID_STREAM;
                observation->observations.accumulator_low = 255;
            }
            return;
        }
        ++sound_melody_cursor;
        uint8_t note = intro_theme_stream[index];
        if (observation) {
            observation->observations.accumulator_low = note;
            observation->observations.accumulator_written = 1;
        }
        if (note == 255) {
            sound_melody_cursor = 255;
            if (observation)
                observation->event = MELODY_ENDED;
            return;
        }
        if (note >= 200) {
            note -= 200;
            if (observation) {
                observation->observations.duration_units = note;
                observation->observations.duration_written = 1;
                observation->observations.accumulator_low = note * 4;
            }
            sound_melody_duration = note * 4;
            continue;
        }
        melody_effect_pitch = note;
        sound_trigger_countdown = sound_melody_duration;
        melody_effect_control.raw = 0x80;
        if (observation) {
            observation->observations.accumulator_low = 3;
            observation->event = MELODY_NOTE;
            observation->payload.note.sound_status = c_sound_effect(3);
        } else
            c_sound_effect(3);
        effects->interrupts_enabled = 1;
        return;
    }
}

struct MelodyStep __far c_advance_melody(uint16_t stream_bank)
{
    struct MelodyStep end = {0};
    struct SoundTickEffects effects = {0};
    advance_melody(stream_bank, &effects, &end);
    return end;
}

static struct SoundTickEffects sound_triggers(
    SegmentBytes data, uint16_t stream_bank, struct SoundTriggerEnd *observation)
{
    struct SoundTickEffects effects = {0};
    uint8_t countdown = sound_trigger_countdown;
    if (observation)
        observation->payload.pending.countdown = countdown;
    if (sound_trigger_countdown)
        return effects;
    uint8_t mode = sound_sequence_mode;
    if (observation) {
        observation->event = SOUND_IDLE;
        observation->payload.idle.mode = mode;
    }
    if (mode == SOUND_MODE_FIXED) {
        sound_trigger_countdown = 0x36;
        if (observation) {
            observation->event = SOUND_FIXED_EFFECT;
            observation->payload.fixed_effect.effect = 4;
            observation->payload.fixed_effect.sound_status = c_sound_effect(4);
        } else
            c_sound_effect(4);
        effects.interrupts_enabled = 1;
        return effects;
    }
    if (mode == SOUND_MODE_MELODY) {
        if (observation)
            observation->event = SOUND_MELODY;
        advance_melody(stream_bank, &effects, observation ? &observation->payload.melody : 0);
        return effects;
    }
    if (mode == SOUND_MODE_RAMP) {
        uint8_t ramp = sound_ramp_index;
        effects.warning_port = ramp;
        if (observation) {
            observation->event = SOUND_RAMP_LIMIT;
            observation->payload.ramp_limit.index = ramp;
        }
        if (ramp >= 0xaa)
            return effects;
        c_sound_effect(6);
        {
            struct GameRandomStep random = game_random_next(&prng_state_low);
            sound_trigger_countdown = (random.value & 3) + 1;
            uint8_t random_high = prng_state_low.middle;
            if (observation)
                observation->payload.ramp_advance.random_high = random_high;
        }
        ++ramp;
        sound_ramp_index = ramp;
        uint8_t value = (ramp >> 2) + 1;
        if (value >= 32)
            value = 31;
        value |= 0x32;
        segment_word_store(data,
            (uint16_t)&sound_storage.effects[6].pitch,
            (segment_word(data, (uint16_t)&sound_storage.effects[6].pitch) & 0xff00) | value);
        if (observation) {
            observation->event = SOUND_RAMP_ADVANCED;
            observation->payload.ramp_advance.previous_index = effects.warning_port;
            observation->payload.ramp_advance.pitch = value;
        }
        effects.interrupts_enabled = 1;
    }
    return effects;
}

struct SoundTriggerEnd __far c_sound_triggers(SegmentBytes data, uint16_t stream_bank)
{
    struct SoundTriggerEnd end = {0};
    sound_triggers(data, stream_bank, &end);
    return end;
}

struct SoundTickEffects __far c_tick_sound_sequence(SegmentBytes data)
{
    return sound_triggers(data, 0, 0);
}

/* Finish a tick without changing the interrupt state, unlike the public
 * silence entry. A background voice can disconnect the shared speaker. */
static void finish_silent_voice(struct SoundVoice *voice, struct VoiceTickEnd *observation)
{
    voice->effect.control.raw = 0;
    uint8_t control = (port_in8(0x61) & 0xfc) | 1;
    port_out8(0x61, control);
    if (observation) {
        observation->sample = (observation->sample & 0xff00) | control;
        observation->sample_mask |= 0xff;
    }
}

/* The wrapped volume drives the envelope; sample/mask writes are entry observations. */
static int advance_voice_envelope(struct SoundVoice *voice, struct VoiceTickEnd *observation)
{
    if (voice->envelope_countdown >= 2) {
        --voice->envelope_countdown;
        return 1;
    }
    voice->envelope_countdown = voice->effect.envelope.period;
    if (observation) {
        observation->sample = (observation->sample & 0xff00) | voice->effect.envelope.period;
        observation->sample_mask |= 0xff;
    }
    if (!voice->effect.envelope.steps) {
        if (observation) {
            observation->sample =
                (observation->sample & 0xff00) | (uint8_t)voice->effect.envelope.step;
            observation->sample_mask |= 0xff;
        }
        if (voice->effect.envelope.step == voice->effect.envelope.next_step)
            return 0;
        voice->effect.envelope.step = voice->effect.envelope.next_step;
        voice->effect.envelope.steps = voice->effect.envelope.next_steps;
        voice->envelope_countdown = voice->effect.next_envelope_period;
        voice->effect.envelope.period = voice->effect.next_envelope_period;
    }
    --voice->effect.envelope.steps;
    uint8_t volume = voice->effect.level + voice->effect.envelope.step;
    if (volume & 128) {
        volume = 0;
        if (observation) {
            observation->sample = 0;
            observation->sample_mask |= 0xffff;
        }
    } else {
        if (volume >= 16)
            volume = 15;
        if (observation) {
            observation->sample = (observation->sample & 0xff00) | volume;
            observation->sample_mask |= 0xff;
        }
    }
    voice->effect.level = volume;
    return 1;
}

/* Keep slot2/1/0 and the shared-speaker disconnect ordering. Native ticks have
 * no accumulator/pitch masks; raw gates request them from these same operations. */
static void update_voices(struct VoiceTickEnd *observation)
{
    int slot;
    for (slot = 2; slot >= 0; --slot) {
        struct SoundVoice *voice = &sound_storage.voices[slot];
        if (sound_muted) {
            finish_silent_voice(voice, observation);
            continue;
        }
        if (observation) {
            observation->pitch = (observation->pitch & 0xff00) | voice->effect.control.raw;
            observation->pitch_mask |= 0xff;
        }
        if (!voice->effect.control.raw)
            continue;
        union SoundControl noise = {.raw = voice->effect.control.raw};
        if (noise.decoded.noise) {
            struct GameRandomStep random = game_random_next(&prng_state_low);
            uint16_t divisor = ((uint16_t)(uint8_t)~voice->effect.pitch << 8) | random.value;
            write_pitch(divisor);
            if (observation) {
                observation->pitch = divisor;
                observation->pitch_mask |= 0xffff;
                observation->sample = (divisor >> 8) | (divisor << 8);
                observation->sample_mask |= 0xffff;
            }
        }
        if (!advance_voice_envelope(voice, observation)) {
            finish_silent_voice(voice, observation);
            continue;
        }
        if (voice->pitch_countdown & 128)
            continue;
        if (voice->pitch_countdown >= 2) {
            --voice->pitch_countdown;
            continue;
        }
        voice->pitch_countdown = voice->effect.pitch_period;
        if (observation) {
            observation->sample = (observation->sample & 0xff00) | voice->effect.pitch_period;
            observation->sample_mask |= 0xff;
        }
        while (!voice->current_pitch.steps) {
            uint8_t phase = voice->pitch_phase + 1;
            if (phase >= 3)
                phase = 0;
            voice->pitch_phase = phase;
            const struct SoundPitchStep *pitch =
                phase ? &voice->effect.later_pitch[phase - 1] : &voice->effect.first_pitch;
            voice->current_pitch.step = pitch->step;
            voice->current_pitch.steps = pitch->steps;
        }
        --voice->current_pitch.steps;
        voice->effect.pitch += voice->current_pitch.step;
        union SoundControl octave = {.raw = voice->effect.control.raw};
        uint16_t divisor = pitch_division(voice->effect.pitch, -octave.decoded.octave_reduction, 0);
        write_pitch(divisor);
        if (observation) {
            observation->pitch = divisor;
            observation->pitch_mask |= 0xffff;
            observation->sample = (divisor >> 8) | (divisor << 8);
            observation->sample_mask |= 0xffff;
        }
    }
}

void __far c_tick_voices(void)
{
    update_voices(0);
}

struct VoiceTickEnd __far c_update_voices(void)
{
    struct VoiceTickEnd end = {0};
    update_voices(&end);
    return end;
}

/* SoundWork is reset independently of other subsystems and data placement. */
struct SoundWork sound_work = {0};

void __far reset_sound_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&sound_work), 0, sizeof(sound_work), 0);
}

/* Effect definitions and live voices share this explicit format: the
 * legacy alias-effect entries address halves of live voice records. */
struct SoundStorage sound_storage = {
    .effects = {
        {.control = {.raw = 0x84},
            .level = 0,
            .pitch = 191,
            .envelope = {.period = 1, .step = 2, .steps = 7, .next_step = -2, .next_steps = 7},
            .first_pitch = {0},
            .pitch_period = 255,
            .next_envelope_period = 1,
            .later_pitch = {0}},
        {.control = {.raw = 0x88},
            .level = 0,
            .pitch = 0,
            .envelope = {.period = 1, .step = 1, .steps = 15, .next_step = -2, .next_steps = 7},
            .first_pitch = {.step = 0, .steps = 16},
            .pitch_period = 1,
            .next_envelope_period = 1,
            .later_pitch = {0}},
        {.control = {.raw = 0x84},
            .level = 0,
            .pitch = 255,
            .envelope = {.period = 4, .step = 1, .steps = 15, .next_step = 0, .next_steps = 255},
            .first_pitch = {0},
            .pitch_period = 255,
            .next_envelope_period = 255,
            .later_pitch = {0}},
        {.control = {.raw = 0x80},
            .level = 0,
            .pitch = 34,
            .envelope = {.period = 1, .step = 15, .steps = 1, .next_step = -1, .next_steps = 4},
            .first_pitch = {.step = 1, .steps = 1},
            .pitch_period = 1,
            .next_envelope_period = 9,
            .later_pitch = {{.step = -1, .steps = 1}, {.step = 0, .steps = 8}}},
        {.control = {.raw = 0x81},
            .level = 4,
            .pitch = 34,
            .envelope = {.period = 5, .step = 1, .steps = 6, .next_step = -1, .next_steps = 6},
            .first_pitch = {.step = 6, .steps = 2},
            .pitch_period = 1,
            .next_envelope_period = 5,
            .later_pitch = {{.step = -6, .steps = 2}, {0}}},
        {.control = {.raw = 0x82},
            .level = 0,
            .pitch = 144,
            .envelope = {.period = 1, .step = 15, .steps = 1, .next_step = -1, .next_steps = 15},
            .first_pitch = {.step = 1, .steps = 2},
            .pitch_period = 1,
            .next_envelope_period = 1,
            .later_pitch = {{.step = -1, .steps = 1}, {.step = 0, .steps = 7}}},
        {.control = {.raw = 0x84},
            .level = 0,
            .pitch = 0,
            .envelope = {.period = 1, .step = 15, .steps = 1, .next_step = -15, .next_steps = 1},
            .first_pitch = {0},
            .pitch_period = 255,
            .next_envelope_period = 1,
            .later_pitch = {0}},
    }};

/* Four relative melody streams, including the transfer suffix entry. */
uint8_t intro_theme_stream[96] = {
    0xc8,
    0x08,
    0x24,
    0xcf,
    0x24,
    0xc8,
    0x14,
    0x30,
    0xcf,
    0x30,
    0xc8,
    0x1c,
    0x38,
    0xcf,
    0x38,
    0xc8,
    0x00,
    0x1c,
    0xcf,
    0x1c,
    0xc8,
    0x08,
    0x24,
    0x24,
    0xff,
    0xc8,
    0x08,
    0x24,
    0xcf,
    0x24,
    0xc8,
    0x14,
    0x30,
    0xcf,
    0x30,
    0xc8,
    0x10,
    0x2c,
    0xcf,
    0x2c,
    0xc8,
    0x00,
    0x1c,
    0xcf,
    0x1c,
    0xc8,
    0x08,
    0x24,
    0x24,
    0xff,
    0xcc,
    0x08,
    0x24,
    0x40,
    0x44,
    0xd7,
    0x54,
    0xca,
    0x68,
    0x54,
    0x44,
    0x40,
    0x24,
    0xd4,
    0x08,
    0xff,
    0xca,
    0x08,
    0x24,
    0xd4,
    0x24,
    0xca,
    0x00,
    0x1c,
    0xcd,
    0x1c,
    0xca,
    0x1c,
    0x38,
    0xce,
    0x38,
    0xca,
    0x14,
    0x30,
    0xcf,
    0x30,
    0xca,
    0x10,
    0x1c,
    0xd1,
    0x1c,
    0xca,
    0x08,
    0x24,
    0x24,
    0xff,
};

/* 48 PIT channel-2 divisors; pitch wraps at 48 */
uint16_t speaker_pitch_divisors[48] = {
    0x3550,
    0x348c,
    0x33cb,
    0x330d,
    0x3252,
    0x3199,
    0x30e3,
    0x3030,
    0x2f7f,
    0x2ed1,
    0x2e25,
    0x2d7b,
    0x2cd5,
    0x2c30,
    0x2b8e,
    0x2aee,
    0x2a50,
    0x29b5,
    0x291c,
    0x2885,
    0x27f0,
    0x275e,
    0x26cd,
    0x263f,
    0x25b3,
    0x2528,
    0x24a0,
    0x2419,
    0x2395,
    0x2312,
    0x2292,
    0x2213,
    0x2196,
    0x211a,
    0x20a1,
    0x2029,
    0x1fb3,
    0x1f3f,
    0x1ecc,
    0x1e5b,
    0x1dec,
    0x1d7e,
    0x1d12,
    0x1ca7,
    0x1c3e,
    0x1bd6,
    0x1b70,
    0x1b0b,
};

/* Global mute latch: zero permits sound, FFh suppresses speaker updates. F2 handlers complement it; prompt helpers save, replace, and restore it. */
volatile uint8_t sound_muted = 0x0;

/* Restore latch for temporary mute overrides: digit-entry beeps and pause helpers save sound_muted here and restore it afterward. F2 handlers also mirror the newly toggled mute value here. */
volatile uint8_t sound_mute_saved = 0x0;
