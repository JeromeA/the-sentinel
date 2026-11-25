#include "render_state.h"
#include "text_bytecode.h"

/* Decimal input retains least-significant digit first. Screen repainting uses
 * the same symbolic target as the C prompt renderer. */

static struct DecimalInput finish_decimal_input(
    struct DecimalInput input, struct DigitInputEnd *observation)
{
    if (observation) {
        observation->render.polygon.flags = input.flags;
        observation->render.data_output = input.data_output;
        observation->count = input.count;
        observation->leave = input.leave;
    }
    return input;
}

struct DecimalInput __far enter_decimal_digits(SegmentBytes data,
    SegmentBytes output,
    uint8_t maximum,
    uint16_t flags,
    struct DigitInputEnd *observation)
{
    struct DecimalInput end = {0};
    if (observation)
        *observation = (struct DigitInputEnd){0};
    struct PolygonScanEnd *raw = observation ? &observation->render.polygon : 0;
    unsigned count = 0;
    if (maximum > 8 || (flags & 0x400)) {
        end.flags = flags | CF;
        return finish_decimal_input(end, observation);
    }
    math_work_lo = maximum;
    c_clear_pending(output, !!(flags & 0x400));
    flags = status(flags, 0, 16);
    fill_segment_bytes(
        output + ((uint16_t)&number_entry_buffer.ascii[0]), 8, TEXT_SPACE, !!(flags & 0x400));
    end.flags = flags;
    unsigned i;
    for (;;) {
        if (repaint_digit_field(data, output, !!(end.flags & 0x400)))
            end.data_output = 1;
        if (observation) {
            raw->span_count = 0;
            raw->changed |= POLYGON_COUNT;
        }
        if (end.data_output)
            output = data;
        for (;;) {
            struct KeyWaitEnd raw_key;
            struct InputKey input = wait_key(data, output, end.flags, observation ? &raw_key : 0);
            uint8_t key = input.character;
            if (observation) {
                raw->value = (raw->value & 0xff00) | key;
                raw->span_count = input.remaining;
                raw->span_destination = raw_key.scan;
                raw->changed |= POLYGON_COUNT | POLYGON_DESTINATION;
            }
            end.flags = subtract8(input.flags, key, 13);
            end.leave = input.leave;
            if (input.leave || key == 13) {
                end.count = count;
                return finish_decimal_input(end, observation);
            }
            if (key < 48)
                continue;
            if (key >= 127) {
                SegmentBytes source = data + (uint16_t)&number_entry_buffer.ascii[0] + 1;
                SegmentBytes destination = output + (uint16_t)&number_entry_buffer.ascii[0];
                if (!count)
                    continue;
                --count;
                copy_video_bytes(&destination, &source, 7, !!(end.flags & 0x400));
                *destination = TEXT_SPACE;
                break;
            }
            if (key >= 58)
                continue;
            if (count == math_work_lo) {
                sound_mute_saved = sound_muted;
                sound_muted = 0;
                {
                    struct PlatformReply bell = platform_video(
                        0x0e07, 0, input.remaining, count, 0, 0, status(end.flags, 0, 16));
                    end.flags = bell.flags;
                    if (observation) {
                        raw->value = bell.value;
                        raw->start_vertex = bell.handle;
                        raw->span_count = bell.count;
                        raw->changed |= POLYGON_VERTEX | POLYGON_COUNT;
                    }
                }
                sound_muted = sound_mute_saved;
                continue;
            }
            ++count;
            for (i = 7; i > 0; --i)
                segment_record(output, number_entry_buffer).ascii[i] =
                    segment_record(data, number_entry_buffer).ascii[i - 1];
            segment_record(output, number_entry_buffer).ascii[0] = key;
            end.flags &= ~0x400;
            break;
        }
    }
}

struct DigitInputEnd __far c_enter_decimal_digits(
    SegmentBytes data, SegmentBytes output, uint8_t maximum, uint16_t flags)
{
    struct DigitInputEnd end;
    enter_decimal_digits(data, output, maximum, flags, &end);
    return end;
}

uint16_t __far pack_decimal_digits(
    SegmentBytes data, SegmentBytes output, int backwards, struct DecimalPackEnd *observation)
{
    if (observation)
        *observation = (struct DecimalPackEnd){0};
    if (backwards)
        return 0;
    uint16_t buffer = (uint16_t)&number_entry_buffer.ascii[0];
    uint16_t last_low = 0;
    uint16_t packed = 0;
    unsigned pair;
    for (pair = 0; pair < 4; ++pair) {
        uint8_t low_digit = segment_record(data, number_entry_buffer).ascii[2 * pair];
        uint8_t high_digit = segment_record(data, number_entry_buffer).ascii[2 * pair + 1];
        low_digit = (low_digit == TEXT_SPACE ? '0' : low_digit) - '0';
        high_digit = (high_digit == TEXT_SPACE ? '0' : high_digit) - '0';
        if (observation)
            last_low = low_digit;
        packed = ((uint16_t)high_digit << 4) | low_digit;
        segment_record(output, number_entry_buffer).packed.digits.pairs[pair].raw = (uint8_t)packed;
    }
    segment_record(output, number_entry_buffer).packed.terminator = 255;
    if (observation)
        *observation = (struct DecimalPackEnd){
            packed, last_low, (uint16_t)(buffer + 8), (uint16_t)(buffer + 4)};
    return packed;
}

struct DecimalPackEnd __far c_pack_decimal_digits(
    SegmentBytes data, SegmentBytes output, int backwards)
{
    struct DecimalPackEnd end;
    pack_decimal_digits(data, output, backwards, &end);
    return end;
}

/* Owned subsystem data. */
union NumericEntry number_entry_buffer = {0};
