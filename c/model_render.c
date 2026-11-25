/* Polar mesh vertices are transformed relative to the viewing robot, then faces
 * are drawn in one or two ordered groups through the polygon entry point. */

extern uint8_t model_vertex_ranges[], model_vertex_azimuth[], model_vertex_radius[];
extern union MeshHeight model_vertex_height[];

extern uint8_t model_visibility_flags[];
extern uint8_t model_face_ranges[];
extern union MeshFaceHeader model_face_flags_and_counts[];
extern union MaterialColors model_face_color_pairs[];
extern struct UnalignedWord model_face_index_pointers[];

struct ModelOrigin __far prepare_model_origin(
    uint16_t object_index, uint16_t drawn_index, struct ModelOriginEnd *observation)
{
    struct ModelOrigin end = {0};
    if (observation)
        *observation = (struct ModelOriginEnd){0};
    if (view_object_slot >= 64 || object_index >= 64 || drawn_index >= 64)
        return end;
    uint16_t observer = view_object_slot;
    drawn_object_slot = (uint8_t)drawn_index;
    model_mesh_id = object_model_id[object_index];
    struct ModelPlanarDelta delta = model_planar_delta(observer, object_index, view_lateral_offset);
    struct ModelHeights height = model_heights(observer, object_index);
    model_height_delta = height.object_height - height.observer_height;
    if (observation)
        observation->observer_height = height.observer_height;
    struct BearingGeometry bearing = c_bearing(delta.x, delta.y, delta.x_sign, delta.y_sign);
    end.angle = bearing.angle;
    if (observation)
        observation->bearing = bearing;
    view_heading_byte = object_heading[observer];
    end.transform.azimuth_bias = bearing.angle - view_azimuth_reference + 0x0a00;
    model_azimuth_bias = end.transform.azimuth_bias;
    end.transform.heading = ((uint16_t)object_heading[drawn_index] << 8) - bearing.angle;
    model_relative_heading = end.transform.heading;
    end.distance = planar_distance(&bearing, observation ? &observation->distance : 0);
    uint8_t preset = scene_view_preset;
    int scaled = !preset && (uint8_t)drawn_index != 63;
    if (observation) {
        observation->preset = preset;
        observation->scaled = scaled;
    }
    if (scaled) {
        end.transform.heading = 0x8000;
        model_relative_heading = end.transform.heading;
        end.distance >>= 1;
        model_height_delta = (model_height_delta >> 1) | 0x8000;
        if (observation)
            observation->height_before_bias = model_height_delta;
        model_height_delta += 0x70;
    }
    end.transform.distance = end.distance;
    model_planar_distance = end.transform.distance;
    publish_projection_axes(&bearing);
    publish_projection_bearing(&bearing);
    projection_distance = end.distance;
    end.transform.height = model_height_delta;
    model_relative_height = end.transform.height;
    if (observation) {
        observation->height = model_height_delta;
        observation->distance.value = end.distance;
    }
    return end;
}

struct ModelOriginEnd __far c_prepare_model_origin(uint16_t object, uint16_t drawn)
{
    struct ModelOriginEnd end;
    prepare_model_origin(object, drawn, &end);
    return end;
}

/* Publish the byte-sized magnitude in the shared arithmetic workspace, then
 * return its signed fixed-point offset. Keep the negative path's two high-byte
 * stores: the original negation and its caller both publish that byte. */
static __attribute__((regparmcall)) int16_t mesh_signed_offset(uint8_t magnitude, int negative)
{
    uint16_t offset = 0;
    math_work_lo = magnitude;
    if (negative) {
        offset = -(uint16_t)magnitude;
        math_work_lo = (uint8_t)offset;
        math_work_hi = offset >> 8;
    }
    math_work_hi = offset >> 8;
    return negative ? (int16_t)offset : magnitude;
}

struct MeshProjection __far project_mesh_vertices(
    const struct ModelTransform *transform, struct MeshVerticesEnd *observation)
{
    struct MeshProjection end = {0};
    if (observation)
        *observation = (struct MeshVerticesEnd){0};
    if (model_mesh_id >= 10 || view_object_slot >= 64)
        return end;
    if (model_vertex_ranges[model_mesh_id] >= model_vertex_range_ends[model_mesh_id] ||
        model_vertex_range_ends[model_mesh_id] > 160 ||
        model_vertex_range_ends[model_mesh_id] - model_vertex_ranges[model_mesh_id] > 32)
        return end;
    end.valid = 1;
    if (observation)
        observation->valid = 1;
    uint16_t mesh = model_mesh_id;
    uint16_t vertex_index = model_vertex_ranges[mesh];
    uint8_t slot = 64;
    projected_vertex_slot = slot;
    uint8_t vertex_end = model_vertex_range_ends[mesh];
    model_element_end = vertex_end;
    model_element_index = model_vertex_ranges[mesh];
    do {
        math_work_lo = (uint8_t)transform->heading;
        uint8_t heading = mapped_projection_heading(
            (uint8_t)((transform->heading >> 8) + model_vertex_azimuth[vertex_index]));
        projection_direction_flags = heading;

        uint8_t fraction = math_work_lo;
        struct ProjectionWave wave = lookup_projection_wave(heading, fraction);
        uint16_t vertex = vertex_index;
        uint8_t radius = model_vertex_radius[vertex];
        /* Keep the sample-corner publications; products use local geometry. */
        math_work_hi = radius;
        uint16_t depth_product = (uint16_t)radius * wave.cosine;
        math_work_lo = (uint8_t)depth_product;
        int16_t depth_offset = mesh_signed_offset(depth_product >> 8, heading & 64);
        uint16_t distance = transform->distance + depth_offset;
        uint16_t absolute_distance = (int16_t)distance < 0 ? -distance : distance;
        math_work_hi = radius;
        uint16_t lateral_product = (uint16_t)radius * wave.sine;
        math_work_lo = (uint8_t)lateral_product;

        /* Bearing receives the lateral/depth magnitudes; product flags are dead. */
        struct BearingGeometry bearing =
            c_bearing(lateral_product >> 8, absolute_distance, heading, distance >> 8);

        {
            uint16_t screen_x = bearing.angle + transform->azimuth_bias;
            projected_x_low[slot] = (uint8_t)screen_x;
            projected_x_high[slot] = screen_x >> 8;
        }

        uint16_t distance_value = planar_distance(&bearing, 0);
        union MeshHeight encoded_height = {.raw = model_vertex_height[vertex_index].raw};
        int16_t height =
            mesh_signed_offset(encoded_height.fields.magnitude * 2, encoded_height.fields.negative);
        struct VerticalAngleEnd vertical =
            c_vertical_angle(height + transform->height, distance_value, view_object_slot);
        if (observation)
            observation->vertical = vertical;
        /* Output-only phase observations; subsequent geometry uses returned values. */
        projection_distance = distance_value;
        publish_vertical_projection(&vertical);
        projected_y_high[slot] = vertical.screen_y >> 8;
        projected_y_low[slot] = (uint8_t)vertical.screen_y;
        model_element_index = ++vertex_index;
        projected_vertex_slot = ++slot;
    } while ((uint8_t)vertex_index != vertex_end);
    end.next_vertex = vertex_index;
    if (observation)
        observation->next_vertex = vertex_index;
    return end;
}

struct MeshVerticesEnd __far c_project_mesh_vertices(void)
{
    struct MeshVerticesEnd end;
    /* The independently callable legacy entry starts from its published state. */
    struct ModelTransform transform = {
        model_relative_heading, model_planar_distance, model_relative_height, model_azimuth_bias};
    project_mesh_vertices(&transform, &end);
    return end;
}

uint8_t __far c_configure_preview_object(uint8_t model, uint16_t record)
{
    if (model >= 10 || record >= 3)
        return object_heading[1];
    object_model_id[1] = model;
    object_grid_y[1] = preview_view_y_offsets[record] + object_grid_y[2];
    object_height_integer[1] = preview_view_height_offsets[record] + object_height_integer[2];
    object_grid_x[1] = object_grid_x[2];
    object_view_pitch[2] = preview_view_pitch_values[record];
    object_heading[2] = preview_view_companion_heading_values[record];
    object_height_fraction[1] = object_height_fraction[2] = 0;
    object_heading[1] = preview_view_heading_values[record];
    view_object_slot = 2;
    return object_heading[1];
}

/* Owned subsystem data. */
uint8_t preview_view_y_offsets[4] = {
    0x05,
    0x07,
    0x07,
    0x00,
};
uint8_t preview_view_pitch_values[3] = {
    0xf4,
    0xfb,
    0xfb,
};
uint8_t preview_view_height_offsets[3] = {
    0x00,
    0x01,
    0x00,
};
uint8_t preview_view_heading_values[3] = {
    0x80,
    0x8e,
    0xce,
};
uint8_t preview_view_companion_heading_values[6] = {
    0x00,
    0xf8,
    0xf8,
    0x00,
    0x00,
    0xa9,
};
uint8_t model_selection_extents[7] = {
    0x3e,
    0x46,
    0x72,
    0x7a,
    0x4a,
    0x4e,
    0xc1,
};
