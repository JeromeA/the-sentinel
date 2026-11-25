/* Terrain/object height sampling and the visibility workspace. Rows are 256
 * bytes apart so a ray's integer Y directly selects its threshold row. */

#include "ray_position.h"

/* Far row views keep the supplied segment; row indexing wraps its word offset. */
static volatile struct OcclusionRow GAME_FAR *occlusion_rows(SegmentBytes data)
{
    return (volatile struct OcclusionRow GAME_FAR *)(data + (uint16_t)occlusion_workspace);
}

uint8_t __far ray_center_distance(
    uint8_t fraction_x, uint8_t fraction_y, struct RayCenterDistance *capture)
{
    uint8_t x = fraction_x - 128, y = fraction_y - 128;
    if (x & 128)
        x = ~x;
    if (y & 128)
        y = ~y;
    math_work_lo = x > y ? x : y;
    uint8_t distance = math_work_lo;
    if (capture) {
        capture->x = x;
        capture->y = y;
        capture->distance = distance;
    }
    return distance;
}

struct RayCenterDistance __far c_ray_center_distance(uint8_t x, uint8_t y)
{
    struct RayCenterDistance end;
    ray_center_distance(x, y, &end);
    return end;
}

struct SurfaceSample __far sample_surface_at(
    uint8_t x, uint8_t y, uint16_t object_bank, struct WorldHeightEnd *capture)
{
    struct TerrainCellAddress address = c_terrain_address(x, y);
    union TerrainCell cell = address.value;
    if (capture) {
        capture->bank = address.bank;
        capture->offset = address.offset;
    }
    if (cell.object.tag != TERRAIN_OBJECT_TAG) {
        uint8_t shape = cell.terrain.shape;
        if (capture) {
            capture->index = shape;
            capture->value = ((uint16_t)shape << 8) | cell.terrain.height;
            capture->flag_left = shape;
            capture->flag_right = 1;
        }
        return (struct SurfaceSample){shape, cell.terrain.height, 0};
    }
    if (capture)
        capture->stack = 1;
    for (;;) {
        uint16_t slot = (object_bank & 0xff00) | cell.object.slot;
        if (capture)
            capture->index = slot;
        uint8_t model = object_model_id[slot];
        if (ray_sample_mode_flags & 128) {
            if ((uint8_t)slot == sentry_test_target_slot)
                ray_target_hit_flags = (ray_target_hit_flags >> 1) | 128;
            if (model == MODEL_TREE || model == MODEL_BOULDER || model == MODEL_SENTINEL_PEDESTAL) {
                uint8_t distance = ray_center_distance(
                    ray_position_x.fraction_high, ray_position_y.fraction_high, 0);
                if (model == MODEL_SENTINEL_PEDESTAL && distance < 100) {
                    uint8_t fraction = object_height_fraction[slot];
                    unsigned height = object_height_integer[slot] + (fraction >= 224);
                    edge_delta_y = (edge_delta_y & 0xff00) | 16;
                    ray_sample_fraction = fraction + 32;
                    if (capture) {
                        capture->value = (address.bank & 0xff00) | (uint8_t)height;
                        capture->flag_left = object_height_integer[slot];
                        capture->flag_right = fraction >= 224;
                        capture->operation = 1;
                    }
                    return (struct SurfaceSample){slot, height, 1};
                }
                if (model == MODEL_BOULDER && distance < 64) {
                    uint8_t fraction = object_height_fraction[slot];
                    ray_sample_special_flags |= 128;
                    ray_sample_fraction = fraction - 96;
                    uint8_t height = object_height_integer[slot] - (fraction < 96);
                    if (capture) {
                        capture->value = (address.bank & 0xff00) | height;
                        capture->flag_left = object_height_integer[slot];
                        capture->flag_right = fraction < 96;
                        capture->operation = 2;
                    }
                    return (struct SurfaceSample){slot, height, 1};
                }
                if (model == MODEL_TREE && distance < 64) {
                    uint16_t object_height =
                        ((uint16_t)object_height_integer[slot] << 8) | object_height_fraction[slot];
                    uint16_t ray_height = ((uint16_t)ray_position_height.integer << 8) |
                        ray_position_height.fraction_high;
                    uint16_t difference = object_height - ray_height + 224;
                    math_work_hi = (uint8_t)difference;
                    if ((int16_t)difference >= 0 && difference < 1024) {
                        uint8_t quarter = difference >> 2;
                        if (capture) {
                            capture->quarter = quarter;
                            capture->quarter_set = 1;
                        }
                        if (quarter >= distance && !(ray_target_hit_flags & 128))
                            ray_intervening_hit_flags = (ray_intervening_hit_flags >> 1) | 128;
                    }
                }
            }
            if (model == MODEL_BOULDER || model == MODEL_SENTINEL_PEDESTAL)
                ray_sample_mode_flags = 0xc0;
        }
        uint8_t link = object_stack_links[slot];
        if (link >= OBJECT_LINK_SUPPORT) {
            cell.raw = link;
            continue;
        }
        uint8_t height = object_height_integer[slot];
        if (capture) {
            capture->value = (address.bank & 0xff00) | height;
            capture->flag_left = link;
            capture->flag_right = OBJECT_LINK_SUPPORT;
            capture->operation = 2;
        }
        return (struct SurfaceSample){slot, height, 1};
    }
}

struct SurfaceSample __far sample_world_surface(
    uint16_t object_bank, struct WorldHeightEnd *capture)
{
    /* Clearing a raw capture may alias published coordinates: read them only
     * after this store, just as the original standalone entry does. */
    if (capture)
        *capture = (struct WorldHeightEnd){0};
    return sample_surface_at(terrain_scan_x, terrain_scan_y, object_bank, capture);
}

struct WorldHeightEnd __far c_sample_world_height(uint16_t object_bank)
{
    struct WorldHeightEnd end;
    sample_world_surface(object_bank, &end);
    return end;
}

static void build_occlusion_heights(
    SegmentBytes data, uint16_t object_bank, struct OcclusionHeightsEnd *capture)
{
    volatile struct OcclusionRow GAME_FAR *rows = occlusion_rows(data);
    ray_sample_mode_flags = 0;
    int x;
    int y;
    for (y = TERRAIN_LAST_VERTEX; y >= 0; --y) {
        terrain_scan_y = y;
        occlusion_height_row_pointer = (uint16_t)&occlusion_workspace[y].heights[0];
        for (x = TERRAIN_LAST_VERTEX; x >= 0; --x) {
            terrain_scan_x = x;
            /* The previous supplied-segment height store may change scanY.
             * X is freshly published from this loop, but Y must be read now. */
            struct SurfaceSample sample = capture
                ? sample_world_surface(object_bank, &capture->sample)
                : sample_surface_at((uint8_t)x, terrain_scan_y, object_bank, 0);
            uint8_t height = (sample.height << 1) | (!sample.stack && sample.selected_index != 0);
            /* Sampling may replace the published row; reload it before storing. */
            uint16_t row_offset = occlusion_height_row_pointer;
            unsigned displaced_y = y + (object_bank >> 8);
            if (row_offset == (uint16_t)&occlusion_workspace[y].heights[0] &&
                displaced_y < TERRAIN_VERTEX_SIDE)
                rows[displaced_y].heights[x] = height;
            else
                data[(uint16_t)(row_offset + (object_bank & 0xff00) + x)] = height;
        }
        terrain_scan_x = 255;
    }
    terrain_scan_y = 255;
    occlusion_height_row_pointer = (uint16_t)occlusion_workspace - sizeof(struct OcclusionRow);
    for (y = TERRAIN_LAST_CELL; y >= 0; --y) {
        for (x = TERRAIN_LAST_CELL; x >= 0; --x) {
            uint8_t a = rows[y].heights[x];
            uint8_t height = a >> 1;
            if (a & 1) {
                uint8_t b = rows[y].heights[x + 1];
                uint8_t c = rows[y + 1].heights[x];
                uint8_t d = rows[y + 1].heights[x + 1];
                if (b < a)
                    a = b;
                if (c < a)
                    a = c;
                if (d < a)
                    a = d;
                height = a >> 1;
            }
            rows[y].thresholds[x] = height;
            if (capture)
                capture->threshold = height;
        }
    }
}

struct OcclusionHeightsEnd __far c_build_occlusion_heights(SegmentBytes data, uint16_t bank)
{
    struct OcclusionHeightsEnd end;
    build_occlusion_heights(data, bank, &end);
    return end;
}

static uint16_t ray_word(struct RayPosition *position)
{
    return ((uint16_t)position->integer << 8) | position->fraction_high;
}

static void advance_ray_word(struct RayPosition *position, uint16_t delta)
{
    uint16_t value = ray_word(position) + (int8_t)(delta >> 8);
    position->fraction_high = (uint8_t)value;
    position->integer = value >> 8;
}

static void trace_occlusion_row(
    SegmentBytes data, uint16_t observer_bank, uint16_t cache_bank, struct OcclusionRayEnd *capture)
{
    if (capture)
        *capture = (struct OcclusionRayEnd){0};

    volatile struct OcclusionRow GAME_FAR *rows = occlusion_rows(data);
    uint8_t y = edge_end_y;
    uint16_t row_offset = (uint16_t)occlusion_workspace + (uint16_t)y * sizeof(struct OcclusionRow);
    occlusion_height_row_pointer = row_offset;
    int x;
    for (x = TERRAIN_LAST_VERTEX; x >= 0; --x) {
        uint8_t steps = 255;
        uint8_t visible = 255;
        edge_end_x = x;
        math_work_lo = 0;
        polygon_vertex_count = 255;
        uint16_t current_row = occlusion_height_row_pointer;
        uint8_t target_height = (y < TERRAIN_VERTEX_SIDE && current_row == row_offset
                                        ? rows[y].heights[x]
                                        : data[(uint16_t)(current_row + x)]) >>
            1;
        polygon_packed_colors = target_height;
        c_ray_object_origin((observer_bank & 0xff00) | player_robot_slot);
        occlusion_ray_step_x = ((uint16_t)x << 8) - ray_word(&ray_position_x);
        occlusion_ray_step_y = ((uint16_t)y << 8) - ray_word(&ray_position_y);
        occlusion_ray_step_height = ((uint16_t)target_height << 8) - ray_word(&ray_position_height);
        uint16_t ax =
            (int16_t)occlusion_ray_step_x < 0 ? -occlusion_ray_step_x : occlusion_ray_step_x;
        uint16_t ay =
            (int16_t)occlusion_ray_step_y < 0 ? -occlusion_ray_step_y : occlusion_ray_step_y;
        uint16_t ah = (int16_t)occlusion_ray_step_height < 0 ? -occlusion_ray_step_height
                                                             : occlusion_ray_step_height;
        uint8_t extent = (ax > ay ? ax : ay) >> 8;
        if ((ah >> 8) > extent)
            extent = ah >> 8;
        math_work_lo = extent;
        unsigned normalized_extent = (uint8_t)(extent * 4);
        if (normalized_extent >= 6) {
            unsigned sample_intervals = 256;
            while (normalized_extent < 256) {
                occlusion_ray_step_x *= 2;
                occlusion_ray_step_y *= 2;
                occlusion_ray_step_height *= 2;
                sample_intervals /= 2;
                normalized_extent *= 2;
            }
            /* March the interior samples, excluding the target endpoint. */
            steps = sample_intervals - 1;
            polygon_vertex_count = steps;
            if (!(occlusion_reference_check_flags & 128)) {
                uint8_t index = generation_validation_seed + 0x29;
                edge_delta_y = (edge_delta_y & 0xff00) | index;
                unsigned i;
                for (i = 0; i < 4; ++i, --index)
                    if (generation_validation_ring[index] != 0x7f)
                        goto invalid_reference;
                if (generation_validation_ring[index] == 0x7f)
                    goto invalid_reference;
                occlusion_reference_check_flags |= 128;
            }
            do {
                uint32_t h = load_ray_position(&ray_position_height);
                advance_ray_word(&ray_position_x, occlusion_ray_step_x);
                advance_ray_word(&ray_position_y, occlusion_ray_step_y);
                h += (int16_t)occlusion_ray_step_height;
                store_ray_position(&ray_position_height, h);
                uint8_t ray_y = ray_position_y.integer;
                uint16_t threshold_row = (uint16_t)occlusion_cell_threshold_rows +
                    (uint16_t)ray_y * sizeof(struct OcclusionRow);
                if (capture) {
                    capture->threshold_row = threshold_row;
                    capture->sampled = 1;
                }
                uint8_t ray_x = ray_position_x.integer;
                /* A ray can leave the grid or cross into another row's bytes. */
                uint8_t threshold = ray_y < TERRAIN_VERTEX_SIDE && ray_x < TERRAIN_VERTEX_SIDE
                    ? rows[ray_y].thresholds[ray_x]
                    : data[(uint16_t)(threshold_row + ray_x)];
                if (ray_position_height.integer < threshold) {
                    visible = 0;
                    break;
                }
            } while (--steps);
        }
        uint16_t cache_index = (cache_bank & 0xff00) |
            ((uint8_t)x | segment_record(data, terrain_work).projected_row_bank);
        terrain_cached_cells[cache_index] = visible;
        if (capture)
            capture->cache_index = cache_index;
    }
    edge_end_x = 255;
    return;
invalid_reference:
    __asm__ volatile("jmp hardware_leave_game");
    __builtin_unreachable();
}

struct OcclusionRayEnd __far c_trace_occlusion_row(
    SegmentBytes data, uint16_t observer_bank, uint16_t cache_bank)
{
    struct OcclusionRayEnd end;
    trace_occlusion_row(data, observer_bank, cache_bank, &end);
    return end;
}

static void rebuild_visibility_core(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t cache_bank,
    int backwards,
    struct VisibilityEnd *capture)
{
    if (capture)
        *capture = (struct VisibilityEnd){0};

    if (backwards)
        return;
    uint8_t outcome = hyperspace_outcome_flags;
    if (capture)
        capture->outcome = outcome;
    if (outcome & 128)
        return;
    if (capture)
        capture->rebuilt = 1;
    terrain_projected_row_bank = 0;
    fill_segment_bytes(output + ((uint16_t)terrain_visibility_bits), 128, 0, backwards);
    build_occlusion_heights(data, cache_bank, 0);
    edge_end_y = TERRAIN_LAST_VERTEX;
    trace_occlusion_row(data, 0, cache_bank, 0);
    int x;
    int y;
    for (y = TERRAIN_LAST_CELL; y >= 0; --y) {
        terrain_projected_row_bank ^= TERRAIN_PROJECTED_ROW_SIZE;
        edge_end_y = y;
        trace_occlusion_row(data, 0, cache_bank, 0);
        terrain_corner_height_b = object_height_integer[player_robot_slot];
        /* The trace publishes the row to combine with cached ray visibility. */
        uint16_t row_offset = occlusion_height_row_pointer;
        volatile struct OcclusionRow GAME_FAR *rows = occlusion_rows(data);
        int canonical_row = row_offset == (uint16_t)&occlusion_workspace[y].heights[0];
        for (x = TERRAIN_LAST_CELL; x >= 0; --x) {
            uint8_t sample = canonical_row ? rows[y].heights[x] : data[(uint16_t)(row_offset + x)];
            uint8_t visible = (sample & 1) || (sample >> 1) <= terrain_corner_height_b ? 255 : 0;
            uint8_t packed = ((x << 3) & 0xe0) | y;
            uint8_t index = packed >> 1;
            uint8_t mask =
                terrain_visible_bit_masks[((x & TERRAIN_STORAGE_BANK_MASK) << 1) | (packed & 1)];
            uint8_t value = (terrain_cached_cells[x] | terrain_cached_cells[x + 1] |
                                terrain_cached_cells[x + TERRAIN_PROJECTED_ROW_SIZE] |
                                terrain_cached_cells[x + (TERRAIN_PROJECTED_ROW_SIZE + 1)]) &
                visible & mask;
            uint8_t old = terrain_visibility_bits[index];
            value |= old & ~mask;
            terrain_visibility_bits[index] = value;
            if (capture) {
                capture->last_old = old;
                capture->last_value = value;
                capture->last_mask = mask;
            }
        }
    }
    edge_end_y = 255;
}

void __far rebuild_visibility(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t cache_bank,
    int backwards)
{
    rebuild_visibility_core(data, output, observer_bank, cache_bank, backwards, 0);
}

struct VisibilityEnd __far c_rebuild_visibility_bits(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t cache_bank,
    int backwards)
{
    struct VisibilityEnd end;
    rebuild_visibility_core(data, output, observer_bank, cache_bank, backwards, &end);
    return end;
}

/* Row stride is part of ray indexing; unused row tails belong to each row. */
struct OcclusionRow occlusion_workspace[TERRAIN_VERTEX_SIDE] = {0};

void __far reset_occlusion_rows(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&occlusion_workspace), 0, sizeof(occlusion_workspace), 0);
}
