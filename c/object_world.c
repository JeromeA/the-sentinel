/* Objects occupy the head of a terrain-cell stack. Their linked support may be
 * bare terrain, a boulder, or the Sentinel's pedestal. */

uint8_t __far c_ray_object_origin(uint16_t slot)
{
    ray_position_x.fraction_low = ray_position_y.fraction_low = ray_position_height.fraction_low =
        0;
    ray_position_x.fraction_high = ray_position_y.fraction_high = 128;
    ray_position_height.fraction_high = object_height_fraction[slot];
    ray_position_x.integer = object_grid_x[slot];
    ray_position_y.integer = object_grid_y[slot];
    ray_position_height.integer = object_height_integer[slot];
    return object_grid_y[slot];
}

void __far detach_object(uint16_t slot, SegmentBytes data, struct ObjectDetachEnd *observation)
{
    uint8_t support = object_stack_links[slot];
    terrain_scan_x = object_grid_x[slot];
    terrain_scan_y = object_grid_y[slot];
    struct TerrainCellAddress cell = c_terrain_address(terrain_scan_x, terrain_scan_y);
    uint8_t height = object_height_integer[slot];
    union TerrainCell replacement =
        support >= OBJECT_LINK_SUPPORT ? terrain_object_cell(support) : terrain_flat_cell(height);
    segment_word_store(data,
        cell.bank + cell.offset,
        (segment_word(data, cell.bank + cell.offset) & 0xff00) | replacement.raw);
    object_stack_links[slot] = OBJECT_LINK_UNUSED;
    if (observation) {
        observation->cell = cell;
        observation->support = support;
        observation->height = height;
        observation->value = replacement.raw;
    }
}

struct ObjectDetachEnd __far c_detach_object(uint16_t slot, SegmentBytes data)
{
    struct ObjectDetachEnd end;
    detach_object(slot, data, &end);
    return end;
}

int __far attach_object(uint16_t slot, SegmentBytes data, struct ObjectAttachEnd *observation)
{
    if (observation)
        *observation = (struct ObjectAttachEnd){0};
    object_grid_x[slot] = terrain_scan_x;
    object_grid_y[slot] = terrain_scan_y;
    struct TerrainCellAddress cell = c_terrain_address(terrain_scan_x, terrain_scan_y);
    if (observation)
        observation->cell = cell;
    uint8_t height;
    if (cell.value.object.tag == TERRAIN_OBJECT_TAG) {
        uint16_t support = cell.value.object.slot;
        uint8_t model = object_model_id[support];
        if (observation) {
            observation->support_slot = support;
            observation->support_model = model;
        }
        if (model != MODEL_BOULDER && model != MODEL_SENTINEL_PEDESTAL)
            return 0;
        object_stack_links[slot] = support | OBJECT_LINK_SUPPORT;
        uint8_t fraction = object_height_fraction[support];
        if (model == MODEL_SENTINEL_PEDESTAL) {
            height = object_height_integer[support] + 1;
            object_height_fraction[slot] = fraction;
        } else {
            height = object_height_integer[support] + (fraction >= 128);
            object_height_fraction[slot] = fraction + 128;
        }
    } else {
        object_stack_links[slot] = 0;
        object_height_fraction[slot] = 0xe0;
        height = cell.value.terrain.height;
    }
    object_height_integer[slot] = height;
    union TerrainCell head = terrain_object_cell((uint8_t)slot);
    data[cell.bank + cell.offset] = head.raw;
    object_view_pitch[slot] = 0xf5;
    struct GameRandomStep random = game_random_next(&prng_state_low);
    /* Keep this volatile phase read even when no legacy AX is requested. */
    uint8_t random_high = prng_state_low.middle;
    object_heading[slot] = (random.value & 0xf8) + 0x60;
    if (observation) {
        observation->random = random.value;
        observation->random_high = random_high;
        observation->heading = object_heading[slot];
        observation->accepted = 1;
    }
    return 1;
}

struct ObjectAttachEnd __far c_attach_object(uint16_t slot, SegmentBytes data)
{
    struct ObjectAttachEnd end;
    attach_object(slot, data, &end);
    return end;
}

static __attribute__((regparmcall)) int find_random_flat_square(
    uint8_t height_limit, SegmentBytes samples)
{
    placement_height_threshold = height_limit;
    placement_retry_counter = 0;
    for (;;) {
        --placement_retry_counter;
        if (!placement_retry_counter) {
            ++placement_height_threshold;
            if (placement_height_threshold >= 12)
                return 0;
        }
        terrain_scan_x = game_random_grid_coordinate(&prng_state_low);
        terrain_scan_y = game_random_grid_coordinate(&prng_state_low);
        uint16_t index = terrain_index(terrain_scan_x, terrain_scan_y);
        union TerrainCell cell = {.raw = terrain_grid[index]};
        if (cell.object.tag == TERRAIN_OBJECT_TAG || cell.terrain.shape)
            continue;
        /* Shape/occupancy use game terrain; height uses the supplied segment. */
        union TerrainCell sampled = {.raw = samples[(uint16_t)terrain_grid + index]};
        if (sampled.terrain.height >= placement_height_threshold)
            continue;
        return 1;
    }
}

int __far place_object_flat(uint16_t slot,
    uint8_t height_limit,
    SegmentBytes samples,
    struct ObjectPlacementEnd *observation)
{
    if (observation)
        *observation = (struct ObjectPlacementEnd){0};
    if (slot >= 64)
        return 0;
    int found = find_random_flat_square(height_limit, samples);
    if (found)
        attach_object(slot, samples, observation ? &observation->attachment : 0);
    else {
        /* The address lookup has no side effects, but retain its original raw read. */
        struct TerrainCellAddress cell = c_terrain_address(terrain_scan_x, terrain_scan_y);
        if (observation) {
            observation->failure_cell = cell;
            observation->height_limit = placement_height_threshold;
        }
    }
    if (observation)
        observation->found = found;
    return found;
}

struct ObjectPlacementEnd __far c_place_object_flat(
    uint16_t slot, uint8_t height_limit, SegmentBytes samples)
{
    struct ObjectPlacementEnd end;
    place_object_flat(slot, height_limit, samples, &end);
    return end;
}

/* Owned subsystem data. */
uint8_t object_energy_values[7] = {
    0x03,
    0x03,
    0x01,
    0x02,
    0x01,
    0x04,
    0x00,
};

/* 64 stack-link/state bytes: negative is free, zero is terrain-supported, and 40h|slot links to a support object. CS:3681 selects the highest negative slot but does not clear its free marker; attaching at CS:346E commits it. Two allocations without attachment therefore select the same slot. Detach at CS:343D pops the cell head to its support or restores a flat height byte, then marks the removed slot 80h without clearing other object arrays. */
uint8_t object_stack_links[64] = {0};

/* 64 per-object pitch bytes, 256 angular units per turn; attachment initializes F5h (-11); camera presets can override the selected viewing slot */
uint8_t object_view_pitch[64] = {0};

/* 64 byte grid-X coordinates; camera slots may lie outside the playable grid */
uint8_t object_grid_x[64] = {0};

/* 64 integer bytes of split 8.8 object origins. Bare-terrain attachment takes the terrain height nibble. Stacking adds the support-specific offset with carry from object_height_fraction; detaching a terrain-supported head restores its integer height<<4 as a flat cell. Freeing a slot retains this byte, so allocation alone does not make it a valid new height. */
uint8_t object_height_integer[64] = {0};

/* 64 byte grid-Y coordinates; coordinate differences use byte wraparound */
uint8_t object_grid_y[64] = {0};

/* 64 azimuth bytes; 256 units per turn, used to orient camera and mesh */
uint8_t object_heading[64] = {0};

/* 64 fractional origin-height bytes. Bare terrain attaches at fraction E0h (7/8 cell). A boulder adds 80h (half a cell), carrying into the integer byte; a Sentinel pedestal preserves the fraction and adds one integer cell. These offsets depend on the support model, not the model being attached. Rejected attachment and detach leave the old fraction unchanged. */
uint8_t object_height_fraction[64] = {0};

/* 64 model selectors, retained even in free slots. Allocation writes only this field and allocated_object_slot; it does not initialize position/height/link state. Attachment accepts occupied-square supports only when the current top model is BOULDER or SENTINEL_PEDESTAL. A rejected attach still leaves the requested model in the free slot, available for a later retry. */
uint8_t object_model_id[64] = {0};

void __far reset_world_objects(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&object_stack_links), 0, sizeof(object_stack_links), 0);
    c_memory_fill(output + ((uint16_t)&object_view_pitch), 0, sizeof(object_view_pitch), 0);
    c_memory_fill(output + ((uint16_t)&object_grid_x), 0, sizeof(object_grid_x), 0);
    c_memory_fill(output + ((uint16_t)&object_height_integer), 0, sizeof(object_height_integer), 0);
    c_memory_fill(output + ((uint16_t)&object_grid_y), 0, sizeof(object_grid_y), 0);
    c_memory_fill(output + ((uint16_t)&object_heading), 0, sizeof(object_heading), 0);
    c_memory_fill(
        output + ((uint16_t)&object_height_fraction), 0, sizeof(object_height_fraction), 0);
    c_memory_fill(output + ((uint16_t)&object_model_id), 0, sizeof(object_model_id), 0);
}
