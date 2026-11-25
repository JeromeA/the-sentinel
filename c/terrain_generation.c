/* Four-bank landscape storage, noise conversion and the two smoothing filters.
 * Filtering is deliberately in place: replacing it with a symmetric convolution
 * changes both the generated landscape and the secret-code sequence. */

static uint16_t terrain_index(uint8_t x, uint8_t y)
{
    return ((uint16_t)(x & TERRAIN_STORAGE_BANK_MASK) << 8) | (((uint16_t)x << 3) & 0xe0) | y;
}

struct TerrainCellAddress __far c_terrain_address(uint8_t x, uint8_t y)
{
    uint16_t bank = (uint16_t)(x & TERRAIN_STORAGE_BANK_MASK) << 8;
    struct TerrainCellAddress cell;
    cell.offset = (((uint16_t)x << 3) & 0xe0) | y;
    cell.bank = (uint16_t)terrain_grid + bank;
    cell.value.raw = terrain_grid[bank + cell.offset];
    return cell;
}

void __far filter_terrain(
    uint8_t mode, uint8_t fixed_coordinate, SegmentBytes data, struct TerrainFilterEnd *observation)
{
    if (observation)
        *observation = (struct TerrainFilterEnd){0};
    terrain_generation_mode = mode;
    int i;
    for (i = TERRAIN_VERTEX_SIDE + 2; i >= 0; --i) {
        uint8_t coordinate = i & TERRAIN_COORDINATE_MASK;
        uint8_t x = mode & 0x80 ? fixed_coordinate : coordinate;
        uint8_t y = mode & 0x80 ? coordinate : fixed_coordinate;
        terrain_filter_work[i] = terrain_grid[terrain_index(x, y)];
    }
    /* The completed gather is visible to the preview stack patch below. */
    terrain_scan_x = mode & 0x80 ? fixed_coordinate : 0;
    terrain_scan_y = mode & 0x80 ? 0 : fixed_coordinate;
    if (mode & 0x40) {
        for (i = TERRAIN_LAST_VERTEX; i >= 0; --i) {
            uint8_t left = terrain_filter_work[i];
            uint8_t middle = terrain_filter_work[i + 1];
            uint8_t right = terrain_filter_work[i + 2];
            if (middle < left && middle < right)
                terrain_filter_work[i + 1] = left < right ? left : right;
            else if (middle > left && middle > right)
                terrain_filter_work[i + 1] = left > right ? left : right;
        }
        if (!(landscape_generation_flags & 0x80)) {
            arm_generation_preview(data, observation);
        }
    } else {
        for (i = 0; i < TERRAIN_VERTEX_SIDE; ++i)
            terrain_filter_work[i] =
                ((uint16_t)terrain_filter_work[i] + terrain_filter_work[i + 1] +
                    terrain_filter_work[i + 2] + terrain_filter_work[i + 3]) >>
                2;
        terrain_generation_saved_noise = terrain_filter_work[78];
        if (observation) {
            observation->work_pointer = 0;
            observation->work_pointer_set = 1;
        }
    }
    /* A supplied segment can make the preview patch alias the fixed coordinate. */
    fixed_coordinate = mode & 0x80 ? terrain_scan_x : terrain_scan_y;
    for (i = TERRAIN_LAST_VERTEX; i >= 0; --i) {
        uint8_t x = mode & 0x80 ? fixed_coordinate : i;
        uint8_t y = mode & 0x80 ? i : fixed_coordinate;
        terrain_grid[terrain_index(x, y)] = terrain_filter_work[i];
    }
    terrain_scan_x = mode & 0x80 ? fixed_coordinate : 0;
    terrain_scan_y = mode & 0x80 ? 0 : fixed_coordinate;
    if (observation) {
        observation->cell = c_terrain_address(terrain_scan_x, terrain_scan_y);
        observation->value = terrain_filter_work[0];
    }
}

void __far smooth_terrain(uint8_t flags, SegmentBytes data, struct TerrainFilterEnd *observation)
{
    terrain_smoothing_flags = flags;
    terrain_smoothing_passes = 2;
    int line;
    unsigned pass;
    for (pass = 0; pass < 2; ++pass) {
        for (line = TERRAIN_LAST_VERTEX; line >= 0; --line) {
            terrain_scan_y = line;
            filter_terrain(terrain_smoothing_flags,
                terrain_smoothing_flags & 0x80 ? terrain_scan_x : line,
                data,
                observation);
        }
        terrain_scan_y = 255;
        for (line = TERRAIN_LAST_VERTEX; line >= 0; --line) {
            terrain_scan_x = line;
            filter_terrain(terrain_smoothing_flags | 0x80, line, data, observation);
        }
        terrain_scan_x = 255;
        --terrain_smoothing_passes;
    }
}

struct TerrainFilterEnd __far c_terrain_filter(uint8_t mode, uint8_t fixed, SegmentBytes data)
{
    struct TerrainFilterEnd end;
    filter_terrain(mode, fixed, data, &end);
    return end;
}

struct TerrainFilterEnd __far c_terrain_smooth(uint8_t flags, SegmentBytes data)
{
    struct TerrainFilterEnd end;
    smooth_terrain(flags, data, &end);
    return end;
}

uint8_t __far transform_terrain(uint8_t mode, struct TerrainTransformEnd *observation)
{
    if (observation)
        *observation = (struct TerrainTransformEnd){0};
    uint8_t final_tagged = 0;
    terrain_generation_mode = mode;
    int x;
    int y;
    for (y = TERRAIN_LAST_VERTEX; y >= 0; --y) {
        terrain_scan_y = y;
        for (x = TERRAIN_LAST_VERTEX; x >= 0; --x) {
            terrain_scan_x = x;
            uint16_t index = terrain_index(x, y);
            union TerrainCell cell;
            if (observation) {
                observation->cell = c_terrain_address(x, y);
                cell = observation->cell.value;
                observation->high = observation->cell.bank >> 8;
            } else
                cell.raw = terrain_grid[index];
            final_tagged = cell.object.tag == TERRAIN_OBJECT_TAG;
            if (observation)
                observation->carry = final_tagged;
            uint8_t value;
            if (!mode)
                value = 0;
            else if (mode & 0x80) {
                struct GameRandomStep step = game_random_next(&prng_state_low);
                value = step.value;
                uint8_t random_high = prng_state_low.middle;
                if (observation) {
                    observation->high = random_high;
                    observation->carry = step.upper_input >> 15;
                }
            } else if (!(mode & 1)) {
                value = terrain_grid[index];
                if (observation)
                    observation->nibble = value >> 4;
                value = (value << 4) | (value >> 4);
            } else {
                int16_t centered_noise = (int16_t)terrain_grid[index] - 128;
                int16_t product = centered_noise * (int16_t)landscape_height_scale;
                /* Retain the legacy product-byte workspace and adapter result. */
                math_work_lo = (uint8_t)product;
                if (observation)
                    observation->high = (uint8_t)(centered_noise < 0 ? -product : product);
                math_work_hi = centered_noise < 0 ? (uint8_t)((uint16_t)product >> 8)
                                                  : (uint8_t)centered_noise;
                /* Quantize downward; C division alone truncates negative fractions toward zero. */
                int16_t height_units = product / 256;
                if (product < 0 && product % 256)
                    --height_units;
                /* The bias wraps as a byte before the signed clamp, including large scales. */
                uint8_t height = height_units + 6;
                if ((int8_t)height < 0)
                    height = 0;
                ++height;
                value = height >= 12 ? 11 : height;
            }
            if (observation)
                observation->value = value;
            terrain_grid[index] = value;
        }
        terrain_scan_x = 255;
    }
    terrain_scan_y = 255;
    return final_tagged;
}

struct TerrainTransformEnd __far c_terrain_transform(uint8_t mode)
{
    struct TerrainTransformEnd end;
    transform_terrain(mode, &end);
    return end;
}

uint8_t __far classify_terrain(uint8_t x, uint8_t y, struct TerrainClassifyEnd *observation)
{
    if (observation)
        *observation = (struct TerrainClassifyEnd){0};
    uint8_t a = terrain_grid[terrain_index(x, y)] & 15;
    uint8_t b = terrain_grid[terrain_index(x + 1, y)] & 15;
    uint8_t c = terrain_grid[terrain_index(x, y + 1)] & 15;
    uint8_t d = terrain_grid[terrain_index(x + 1, y + 1)] & 15;
    uint8_t value = d;
    terrain_corner_height_a = a;
    terrain_corner_height_b = b;
    terrain_corner_height_c = c;
    terrain_corner_height_d = d;
    if (observation)
        observation->cell = c_terrain_address(x, (uint8_t)(y + 1));
    if (a == b && b == c && c == d) {
        if (observation) {
            observation->flat = 1;
            observation->value = a;
        }
        return 0;
    }
    /* a,b are adjacent along +X; c,d are their neighbors along +Y.
     * Three equal corners leave a single raised or lowered corner. */
    uint8_t compare;
    uint8_t shape;
    if (a == b && b == c) {
        value = a;
        compare = d;
        shape = a < d ? 10 : 3;
    } else if (b == c && c == d) {
        compare = a;
        shape = d < a ? 11 : 2;
    } else if (a == b && b == d) {
        compare = c;
        shape = d < c ? 6 : 15;
    } else if (a == c && c == d) {
        compare = b;
        shape = d < b ? 14 : 7;
        /* Two equal pairs form opposite edges. */
    } else if (a == b && c == d) {
        compare = b;
        shape = d < b ? 1 : 9;
    } else if (a == c && b == d) {
        compare = c;
        shape = d < c ? 5 : 13;
    } else {
        /* An equal-height +Y edge selects the remaining irregular-cell shape. */
        unsigned equal_y_edge = d == b || a == c;
        compare = equal_y_edge ? c : b;
        shape = equal_y_edge ? 4 : 12;
    }
    if (observation) {
        observation->value = value;
        observation->shape = shape;
        observation->comparison = compare;
    }
    return shape;
}

struct TerrainClassifyEnd __far c_terrain_classify(uint8_t x, uint8_t y)
{
    struct TerrainClassifyEnd end;
    classify_terrain(x, y, &end);
    return end;
}

/* Owned subsystem data. */
uint8_t generation_reference_zero_run[8] = {0};
volatile uint8_t bcd_random_digit_work = 0x0;
uint8_t terrain_generation_saved_noise = 0x0;

/* two 32-cell row banks, indexed by column OR row-bank bit 20h; 27CAh copies tagged world cells here, visibility processing can clear entries, and 2955h draws them */
uint8_t terrain_cached_cells[TERRAIN_PROJECTED_CACHE_SIZE] = {0};

/* 64 zero bytes after the two 32-cell terrain row banks. Cached-cell producers and consumers mask the bank/column index to 3Fh. This is outside those banks and ends at terrain_grid; it does not declare two more active rows. */
uint8_t terrain_cached_cells_capacity_tail[64] = {0};

/* 32x32 cells at (x&3)*256+(x>>2)*32+y. Generation first stores random unsigned noise, box-filters it twice per axis, converts it to heights 1..11, then applies the nonlinear filter twice per axis. Before the final nibble swap height is low and slope classification is high; gameplay uses height high/shape low. C0h+ tags object stacks only after placement, so byte interpretation is phase dependent. */
uint8_t terrain_grid[TERRAIN_GRID_BYTES] = {0};

/* 512 zero bytes after the 1024-byte four-bank terrain grid. CS:2B6A constructs a base from two X bank bits and an eight-bit within-bank index, so even byte-valued input coordinates cannot reach this tail. Its next boundary is polygon_left_spans. */
uint8_t terrain_grid_capacity_tail[512] = {0};

void __far reset_terrain_arrays(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&terrain_cached_cells), 0, sizeof(terrain_cached_cells), 0);
    c_memory_fill(output + ((uint16_t)&terrain_cached_cells_capacity_tail),
        0,
        sizeof(terrain_cached_cells_capacity_tail),
        0);
    c_memory_fill(output + ((uint16_t)&terrain_grid), 0, sizeof(terrain_grid), 0);
    c_memory_fill(
        output + ((uint16_t)&terrain_grid_capacity_tail), 0, sizeof(terrain_grid_capacity_tail), 0);
}
