/* Two height samples distinguish full from partial visibility. Native callers
 * receive restored full indices; visibility and hit histories stay persistent. */

static void restore_sentry_sight_slots(struct SightIndices *indices, struct SentrySightEnd *capture)
{
    indices->observer = (indices->observer & 0xff00) | sentry_sight_saved_source_slot;
    indices->target = (indices->target & 0xff00) | sentry_test_target_slot;
    if (capture) {
        capture->ray.observer_index = indices->observer;
        capture->ray.position_index = indices->target;
        capture->ray.cell_offset = indices->target;
    }
}

static void trace_sentry_sight_sample(struct SightIndices *indices,
    SegmentBytes data,
    uint16_t height_delta,
    uint16_t distance,
    int valid_origin,
    uint16_t *sight_angle,
    struct SentrySightEnd *capture)
{
    struct VerticalAngleEnd vertical = c_vertical_angle(
        height_delta, valid_origin ? distance : projection_distance, indices->observer);
    if (indices->observer < 64) {
        *sight_angle = vertical.angle;
        publish_vertical_projection(&vertical);
    } else {
        projection_abs_delta_x = height_delta;
        *sight_angle = projection_angle;
    }
    if (capture)
        capture->lookup = bearing_table_index(&vertical.bearing);
    ray_pitch_angle = *sight_angle;
    uint8_t pitch_fraction = *sight_angle;
    math_work_lo = pitch_fraction;
    uint16_t pitch = (*sight_angle & 0xff00) | pitch_fraction;
    if (!(indices->target & 0xff00) && !(indices->observer & 0xff00))
        c_synthesize_ray_direction(pitch, data);
    int contact;
    if (capture) {
        struct WorldRayEnd traced = c_trace_world_ray(data, 0, 0, capture->ray.flags);
        if (!traced.cell_sampled && capture->ray.cell_sampled) {
            traced.cell_bank = capture->ray.cell_bank;
            traced.cell_sampled = 1;
        }
        capture->ray = traced;
        indices->observer = traced.observer_index;
        indices->target = traced.position_index;
        contact = !(traced.flags & CF);
    } else {
        struct RayContact traced = trace_world_contact(data, 0, 0);
        indices->observer = traced.observer_index;
        indices->target = traced.position_index;
        contact = traced.contact;
    }
    uint8_t hit = ray_target_hit_flags;
    uint8_t result = sentry_target_sight_result;
    uint8_t intervening = ray_intervening_hit_flags;
    ray_target_hit_flags = (hit << 1) | !contact;
    sentry_target_sight_result = (result >> 1) | (hit & 128);
    ray_intervening_hit_flags = (intervening << 1) | (result & 1);
    sentry_intervening_sight_bits = (sentry_intervening_sight_bits >> 1) | (intervening & 128);
}

static struct SightIndices test_sentry_sight(SegmentBytes data,
    uint8_t filter,
    uint16_t source_index,
    uint16_t target_index,
    uint16_t object_record,
    struct SentrySightEnd *capture)
{
    struct SightIndices indices = {source_index, target_index};
    math_work_lo = filter;
    sentry_sight_saved_source_slot = (uint8_t)source_index;
    sentry_test_target_slot = (uint8_t)target_index;
    sentry_target_sight_result = 0;
    uint8_t link = object_stack_links[object_record];
    if (capture) {
        capture->ray.value = (capture->ray.value & 0xff00) | link;
        capture->ray.flags = status(capture->ray.flags, link, 8);
    }
    if (link & OBJECT_LINK_UNUSED) {
        if (capture)
            capture->ray.flags &= ~CF;
        goto finish;
    }
    uint8_t model = object_model_id[object_record];
    if (capture) {
        capture->ray.value = (capture->ray.value & 0xff00) | model;
        capture->ray.flags = subtract8(capture->ray.flags, model, filter);
    }
    if (model != filter) {
        if (capture)
            capture->ray.flags &= ~CF;
        goto finish;
    }
    uint16_t sight_angle, sight_distance;
    int valid_origin = !(source_index & 0xff00) && view_object_slot < 64 && object_record < 64 &&
        target_index < 64;
    {
        struct ModelOriginEnd raw_origin;
        if (capture)
            raw_origin = (struct ModelOriginEnd){0};
        struct ModelOrigin origin = {0};
        if (!(source_index & 0xff00))
            origin = prepare_model_origin(object_record, target_index, capture ? &raw_origin : 0);
        if (capture) {
            capture->prepared = 1;
            capture->lookup = bearing_table_index(&raw_origin.bearing);
            capture->ray.height_work = raw_origin.observer_height;
            capture->ray.value = raw_origin.height;
        }
        sight_angle = valid_origin ? origin.angle : projection_angle;
        sight_distance = origin.distance;
    }
    {
        uint8_t noise = terrain_generation_saved_noise;
        uint8_t increment = (noise & 15) == 9 ? 7 : 1;
        indices.observer = (indices.observer & 0xff00) | increment;
        math_work_lo = noise;
        sentry_sight_noise_limit = noise + increment;
        math_work_lo = sentry_scan_angle_width >> 1;
        uint8_t angle = (uint8_t)((model_azimuth_bias >> 8) - 10 + math_work_lo);
        if (capture)
            capture->ray.value = (capture->ray.value & 0xff00) | angle;
        uint8_t width = sentry_scan_angle_width;
        if (capture)
            capture->ray.flags = subtract8(capture->ray.flags, angle, width) ^ CF;
        if (angle >= width)
            goto finish;
    }
    ray_heading_angle = sight_angle;
    sentry_sight_rays_left = 2;
    if (!segment_record(data, projection_work).mesh_id.bytes[0]) {
        ray_target_test_flags |= 128;
        trace_sentry_sight_sample(&indices,
            data,
            model_height_delta,
            sight_distance,
            valid_origin,
            &sight_angle,
            capture);
    }
    ray_target_test_flags &= 127;
    uint16_t lower_height = model_height_delta - 224;
    projection_abs_delta_x = lower_height;
    if (capture)
        capture->ray.value = lower_height;
    --sentry_sight_rays_left;
    while (sentry_sight_rays_left) {
        trace_sentry_sight_sample(
            &indices, data, lower_height, sight_distance, valid_origin, &sight_angle, capture);
        ray_target_test_flags &= 127;
        lower_height = model_height_delta - 224;
        projection_abs_delta_x = lower_height;
        if (capture)
            capture->ray.value = lower_height;
        --sentry_sight_rays_left;
    }
    if (capture)
        capture->ray.flags &= ~CF;
finish:
    restore_sentry_sight_slots(&indices, capture);
    return indices;
}

struct SightIndices __far test_sentry_visibility(
    SegmentBytes data, uint8_t filter, uint16_t source, uint16_t target, uint16_t object_record)
{
    return test_sentry_sight(data, filter, source, target, object_record, 0);
}

struct SentrySightEnd __far c_sentry_test_sight(SegmentBytes data,
    uint16_t model_word,
    uint16_t source_index,
    uint16_t target_index,
    uint16_t object_record,
    uint16_t initial_flags)
{
    struct SentrySightEnd end = {0};
    end.ray.value = model_word;
    end.ray.flags = initial_flags;
    end.source = (uint8_t)source_index;
    end.target = (uint8_t)target_index;
    test_sentry_sight(data, (uint8_t)model_word, source_index, target_index, object_record, &end);
    return end;
}
