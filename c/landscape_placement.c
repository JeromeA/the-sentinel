/* Coarse placement regions retain the highest flat square, with the last
 * scanned square winning ties. Adjacent regions are retired after placement. */

void __far c_reset_sentry_scan(uint16_t slot)
{
    sentry_meanie_slots[slot] = 128;
    sentry_scan_target[slot] = 128;
    sentry_scan_pass[slot] = 0;
    sentry_scan_cursor[slot] = 64;
}

struct HeightCandidatesEnd __far c_collect_height_candidates(
    uint8_t required_height, const uint8_t *region_heights, uint8_t *indices)
{
    struct HeightCandidatesEnd end = {0};
    int region;
    for (region = 63; region >= 0; --region)
        if (region_heights[region] == required_height)
            indices[end.count++] = region;
    if (end.count) {
        uint8_t value = end.count;
        unsigned bit = 0;
        while (!(value & 128)) {
            value <<= 1;
            ++bit;
        }
        end.bit = bit;
    }
    return end;
}

/* Keep scratch publication ordered before the mask lookup, including for legacy callers. */
void __far c_publish_height_candidates(struct HeightCandidatesEnd *end)
{
    if (end->count) {
        math_work_lo = end->count;
        end->mask = placement_count_masks[end->bit];
        placement_candidate_mask = end->mask;
    }
}

/* Keep packed heights; a flat zero is a valid candidate, unlike an empty region. */
struct PlacementCandidate {
    uint8_t x, y, height, valid;
};

static __attribute__((regparmcall)) struct PlacementCandidate scan_placement_region(
    unsigned x_origin, unsigned y_origin, unsigned width, unsigned height)
{
    struct PlacementCandidate candidate = {0};
    unsigned y;
    unsigned x;
    for (y = y_origin; y < y_origin + height; ++y) {
        for (x = x_origin; x < x_origin + width; ++x) {
            union TerrainCell cell = {.raw = terrain_grid[terrain_index(x, y)]};
            uint8_t elevation = cell.terrain.height << 4;
            if (cell.terrain.shape || elevation < candidate.height)
                continue;
            candidate.x = x;
            candidate.y = y;
            candidate.height = elevation;
            candidate.valid = 1;
        }
    }
    return candidate;
}

static void sample_placement(struct PlacementSampleEnd *observation)
{
    const uint8_t last_x = TERRAIN_LAST_CELL;
    const uint8_t last_y = TERRAIN_LAST_CELL;
    placement_height_threshold = 0;
    unsigned region;
    for (region = 0; region < 64; ++region) {
        unsigned x_origin = (region & 7) * 4;
        unsigned y_origin = (region & 56) / 2;
        unsigned width = x_origin == 28 ? 3 : 4;
        unsigned height = y_origin == 28 ? 3 : 4;
        struct PlacementCandidate candidate =
            scan_placement_region(x_origin, y_origin, width, height);
        placement_region_heights[region] = candidate.height;
        if (candidate.valid) {
            placement_candidate_x[region] = candidate.x;
            placement_candidate_y[region] = candidate.y;
        }
        if (candidate.height >= placement_height_threshold)
            placement_height_threshold = candidate.height;
    }
    /* Preserve the sampler's legacy scan position and last-cell result. */
    placement_region_x_origin = last_x & ~3;
    placement_region_y_origin = last_y & ~3;
    terrain_scan_x = last_x + 1;
    terrain_scan_y = last_y + 1;
    placement_rows_remaining = placement_columns_remaining = 0;
    if (observation) {
        observation->bank =
            (uint16_t)terrain_grid + ((uint16_t)(last_x & TERRAIN_STORAGE_BANK_MASK) << 8);
        observation->offset = ((last_x << 3) & 0xe0) | last_y;
        union TerrainCell final_cell = {.raw = terrain_grid[terrain_index(last_x, last_y)]};
        uint8_t final_value = final_cell.terrain.shape;
        if (!final_value)
            final_value = (final_cell.terrain.height << 4) < placement_region_heights[63]
                ? (final_cell.terrain.height << 4)
                : last_y;
        observation->value = final_value;
    }
}

struct PlacementSampleEnd __far c_sample_placement(void)
{
    struct PlacementSampleEnd end;
    sample_placement(&end);
    return end;
}

static void place_principal_objects(SegmentBytes data, struct PrincipalPlacementEnd *observation)
{
    uint8_t height_limit;
    if (observation) {
        struct PlacementSampleEnd sample;
        sample_placement(&sample);
        observation->bank = sample.bank;
        observation->offset = sample.offset;
    } else
        sample_placement(0);
    uint16_t slot = 0;
    for (;;) {
        view_object_slot = slot;
        object_model_id[slot] = MODEL_SENTRY;
        struct HeightCandidatesEnd candidates;
        for (;;) {
            candidates = c_collect_height_candidates(
                placement_height_threshold, placement_region_heights, placement_candidates);
            c_publish_height_candidates(&candidates);
            if (observation) {
                observation->lookup = candidates.count ? candidates.bit : 0;
            }
            if (candidates.count)
                break;
            placement_height_threshold -= 16;
            if (!placement_height_threshold) {
                landscape_sentry_count = 255;
                if (observation) {
                    observation->slot = 255;
                    observation->value = 0;
                }
                goto finished;
            }
        }
        uint8_t random;
        do {
            random = game_random_next(&prng_state_low).value & candidates.mask;
        } while (random >= candidates.count);
        uint16_t region = placement_candidates[random];
        {
            static const int8_t neighbors[] = {-9, -8, -7, -1, 0, 1, 7, 8, 9};
            unsigned i;
            for (i = 0; i < 9; ++i)
                data[(uint16_t)placement_region_heights + region + neighbors[i]] = 0;
        }
        terrain_scan_x = placement_candidate_x[region];
        terrain_scan_y = placement_candidate_y[region];
        if (!view_object_slot) {
            sentinel_start_square_y = terrain_scan_y;
            sentinel_start_square_x = terrain_scan_x;
            object_model_id[0] = MODEL_SENTINEL;
            int pedestal = c_allocate_model(MODEL_SENTINEL_PEDESTAL);
            if (pedestal < 0)
                pedestal = 255;
            if (observation) {
                struct ObjectAttachEnd attached = c_attach_object(pedestal, data);
                if (attached.cell.value.object.tag == TERRAIN_OBJECT_TAG)
                    observation->lookup = attached.support_slot;
            } else
                attach_object(pedestal, data, 0);
            object_heading[pedestal] = 0;
        }
        slot = view_object_slot;
        if (observation) {
            struct ObjectAttachEnd attached = c_attach_object(slot, data);
            observation->bank = attached.cell.bank;
            observation->offset = attached.cell.offset;
            if (attached.cell.value.object.tag == TERRAIN_OBJECT_TAG)
                observation->lookup = attached.support_slot;
        } else
            attach_object(slot, data, 0);
        c_reset_sentry_scan(slot);
        random = game_random_next(&prng_state_low).value;
        uint8_t random_high = prng_state_low.middle;
        if (observation) {
            observation->value = (uint16_t)random_high << 8;
            observation->placed = 1;
        }
        sentry_scan_delays[slot] = ((random >> 1) & 63) | 5;
        sentry_rotation_steps[slot] = (random & 1) ? 0xec : 0x14;
        ++slot;
        if (observation) {
            observation->slot = slot;
        }
        if ((uint8_t)slot >= landscape_sentry_count)
            break;
    }
finished:
    height_limit = placement_height_threshold >> 4;
    landscape_object_height_limit = height_limit;
    if (observation)
        observation->value = (observation->value & 0xff00) | height_limit;
}

void __far c_place_initial_objects(SegmentBytes data)
{
    place_principal_objects(data, 0);
}

struct PrincipalPlacementEnd __far c_place_principal_objects(SegmentBytes data)
{
    struct PrincipalPlacementEnd end = {0};
    place_principal_objects(data, &end);
    return end;
}

struct WarningAggregateEnd __far c_aggregate_sentry_warning(uint16_t sentry_bank)
{
    struct WarningAggregateEnd end = {0};
    uint8_t warning = 0;
    uint8_t sight = 0;
    math_work_lo = 0;
    int slot;
    for (slot = 7; slot >= 0; --slot) {
        uint16_t index = (sentry_bank & 0xff00) | slot;
        end.last_slot = index;
        if (object_stack_links[index] & OBJECT_LINK_UNUSED)
            continue;
        if (object_model_id[index] != MODEL_SENTRY && object_model_id[index] != MODEL_SENTINEL)
            continue;
        if (sentry_target_slots[index] != player_robot_slot || !sentry_absorb_delays[index])
            continue;
        warning = 4;
        sight = sentry_target_sight[index];
        math_work_lo = sight;
        if (sight & 128)
            break;
    }
    if (slot < 0)
        end.last_slot = (sentry_bank & 0xff00) | 255;
    sentry_warning_pattern = warning;
    warning_bar_sight = sight;
    uint8_t old_mode = sound_sequence_mode;
    sound_sequence_mode = warning;

    if (warning != old_mode) {

        if (old_mode != 3) {
            end.silenced = 1;
            end.control = c_silence_voice(1);
            end.last_slot = (sentry_bank & 0xff00) | 1;
        }
    }

    end.warning = warning;
    end.sight = sight;
    end.previous_mode = old_mode;
    return end;
}

/* Owned subsystem data. */
uint8_t placement_count_masks[9] = {
    0xff,
    0x7f,
    0x3f,
    0x1f,
    0x0f,
    0x07,
    0x03,
    0x01,
    0x00,
};
