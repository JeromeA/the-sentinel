/* Fixed-point direction vectors for cursor rays and displaced overview cameras.
 * Each trig component carries its sign in bit zero, with magnitude above it. */

static uint16_t ray_trig_magnitude(uint16_t phase)
{
    if (phase < 0x7af0) {
        uint8_t hi = phase >> 8;
        uint16_t first = (uint16_t)hi * 0xab;
        uint16_t second = (uint16_t)(uint8_t)(first >> 8) * hi;
        uint16_t correction = ((uint32_t)(phase & 0xff00) * second) >> 16;
        math_work_lo = (uint8_t)second;
        math_work_hi = second >> 8;
        return (phase - correction) * 2;
    } else {
        uint16_t complement = 0xc900 - phase;
        uint16_t high = ((uint32_t)(complement & 0xff00) * complement) >> 16;
        uint16_t magnitude = -(high * 2);
        return magnitude ? magnitude : 0xfffe;
    }
}

static void synthesize_ray_trig(
    uint16_t direction_angle, SegmentBytes data, struct RayTrigEnd *observation)
{
    uint8_t angle = direction_angle >> 8;
    trig_source_angle = angle;
    math_work_hi = angle;
    uint16_t phase = ((uint32_t)(uint16_t)(direction_angle * 4) * 0xc900) >> 16;
    trig_phase_work = phase;
    uint16_t first = angle & 64 ? 2 : 0;
    segment_record(data, ray_work).sample_mode_flags = 2 - first;
    segment_word_store(data, (uint16_t)&trig_component_a + first, ray_trig_magnitude(phase));
    trig_phase_work = 0xc900 - phase;
    segment_word_store(
        data, (uint16_t)&trig_component_a + 2 - first, ray_trig_magnitude(trig_phase_work));
    if (observation) {
        observation->phase = 0;
        observation->phase_used = 0;
        observation->angle = angle;
        if (phase >= 0x7af0) {
            observation->phase = 0xc900 - phase;
            observation->phase_used = 1;
        } else if (phase <= 0xc900 - 0x7af0) {
            observation->phase = phase;
            observation->phase_used = 1;
        }
    }
    if (angle & 128)
        trig_component_a |= 1;
    uint8_t sign = (uint8_t)((angle << 1) ^ angle);
    if (sign & 128) {
        trig_component_b |= 1;
        if (observation)
            sign = trig_component_b & 255;
    }
    if (observation)
        observation->sign_test = sign;
}

struct RayTrigEnd __far c_ray_trig(uint16_t direction_angle, SegmentBytes data)
{
    struct RayTrigEnd end;
    synthesize_ray_trig(direction_angle, data, &end);
    return end;
}

uint16_t __far c_ray_decode_component(SegmentBytes data, uint16_t component_offset)
{
    return component_offset < 4 && !(component_offset & 1)
        ? segment_word(data, (uint16_t)&trig_component_a + component_offset)
        : 0;
}

static void scale_ray_component(SegmentBytes data,
    uint16_t component_offset,
    uint16_t vector_offset,
    struct EncodedProduct *observation)
{
    if (component_offset >= 4 || (component_offset & 1) || vector_offset >= 6 ||
        (vector_offset & 1))
        return;
    projection_direction_flags = 0;
    uint16_t factor = ray_horizontal_scale;
    math_signed_factor = factor;
    /* Supplied data may alias the factor just published. Sample only now, and
     * repeat this sequence for every component after the preceding step store. */
    uint16_t encoded = segment_word(data, (uint16_t)&trig_component_a + component_offset);
    math_encoded_factor = encoded;
    uint16_t value = multiply_encoded(factor, encoded, 0, observation);
    segment_word_store(data, (uint16_t)&occlusion_ray_step_x + vector_offset, value);
}

struct EncodedProduct __far c_ray_scale_component(
    SegmentBytes data, uint16_t component_offset, uint16_t vector_offset)
{
    struct EncodedProduct end = {0};
    scale_ray_component(data, component_offset, vector_offset, &end);
    return end;
}

/* Observation capture is reserved for original-entry gates. Native consumers
 * need the ordered segmented step writes, without a nested register packet. */
static void synthesize_ray_direction(
    uint16_t pitch, SegmentBytes data, struct RayDirectionEnd *observation)
{
    synthesize_ray_trig(pitch, data, observation ? &observation->pitch : 0);
    uint16_t encoded = c_ray_decode_component(data, 2);
    ray_horizontal_scale = encoded & 1 ? -(encoded >> 4) : encoded >> 4;
    encoded = c_ray_decode_component(data, 0);
    uint16_t vertical = encoded & 1 ? -(encoded >> 4) : encoded >> 4;
    segment_word_store(data, (uint16_t)&occlusion_ray_step_height, vertical);
    uint8_t heading_fraction = ray_heading_angle;
    math_work_lo = heading_fraction;
    uint16_t heading = (ray_heading_angle & 0xff00) | heading_fraction;
    synthesize_ray_trig(heading, data, observation ? &observation->heading : 0);
    scale_ray_component(data, 2, 4, 0);
    scale_ray_component(data, 0, 0, observation ? &observation->x : 0);
}

void __far c_synthesize_ray_direction(uint16_t pitch, SegmentBytes data)
{
    synthesize_ray_direction(pitch, data, 0);
}

struct RayDirectionEnd __far c_ray_pitch_direction(uint16_t pitch, SegmentBytes data)
{
    struct RayDirectionEnd end;
    synthesize_ray_direction(pitch, data, &end);
    return end;
}

static void synthesize_cursor_ray(
    uint16_t observer, SegmentBytes data, struct RayDirectionEnd *observation)
{
    if (observer >= 64)
        return;
    ray_heading_angle = ((uint16_t)camera_cursor_x_pairs << 8) >> 3;
    ray_heading_angle += (uint16_t)(uint8_t)(object_heading[observer] - 10) << 8;
    ray_pitch_angle = (((uint16_t)camera_cursor_inverted_y << 8) - 0x500u) >> 4;
    ray_pitch_angle += 32;
    ray_pitch_angle += (uint16_t)(uint8_t)(object_view_pitch[observer] + 3) << 8;
    uint8_t pitch_fraction = ray_pitch_angle;
    math_work_lo = pitch_fraction;
    uint16_t pitch = (ray_pitch_angle & 0xff00) | pitch_fraction;
    synthesize_ray_direction(pitch, data, observation);
}

void __far c_synthesize_cursor_ray(uint16_t observer, SegmentBytes data)
{
    synthesize_cursor_ray(observer, data, 0);
}

struct RayDirectionEnd __far c_build_cursor_ray(uint16_t observer, SegmentBytes data)
{
    struct RayDirectionEnd end = {0};
    synthesize_cursor_ray(observer, data, &end);
    return end;
}

/* Owned subsystem data. */
uint8_t ray_contact_edge_selectors[8] = {
    0x00,
    0x03,
    0x01,
    0x00,
    0x01,
    0x02,
    0x02,
    0x03,
};

/* RayWork is reset independently of other subsystems and data placement. */
struct RayWork ray_work = {0};

void __far reset_ray_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&ray_work), 0, sizeof(ray_work), 0);
}
