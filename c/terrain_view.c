/* Traverse terrain in a heading-aligned frame, caching two projected corner rows
 * and reconciling their visible intervals before drawing the intervening cells. */

enum TerrainCornerClip {
    TERRAIN_CORNER_VISIBLE = 0,
    TERRAIN_CORNER_CLIPPED_RIGHT = 128,
    TERRAIN_CORNER_CLIPPED_LEFT = 129
};

struct TerrainCornerEnd __far c_project_terrain_corner(uint8_t column, uint16_t observer_bank)
{
    uint8_t slot = column | terrain_projected_row_bank;
    uint8_t dx = (uint8_t)(column - 1 - view_grid_x - view_lateral_offset - view_grid_x_overflow);
    uint8_t dy = (uint8_t)(terrain_scan_y - 1 - view_grid_y);
    terrain_scan_x = column;
    terrain_corner_column = column;
    projected_vertex_slot = slot;
    projection_clip_status = TERRAIN_CORNER_VISIBLE;

    uint16_t absolute_x = ((uint16_t)dx << 8) | 128;
    if (dx & 128)
        absolute_x = -absolute_x;
    uint16_t absolute_y = ((uint16_t)dy << 8) | 128;
    if (dy & 128)
        absolute_y = -absolute_y;

    struct BearingGeometry bearing = c_bearing(absolute_x, absolute_y, dx, dy);
    {
        uint16_t screen_x = bearing.angle - terrain_azimuth_bias;
        projected_x_low[slot] = (uint8_t)screen_x;
        projected_x_high[slot] = screen_x >> 8;
    }

    uint16_t distance = planar_distance(&bearing, 0);
    terrain_corner_distance[slot] = distance >> 8;
    uint8_t world_x;
    uint8_t world_y;
    switch (terrain_view_rotation >> 6) {
    case 0:
        world_x = column;
        world_y = terrain_scan_y;
        break;
    case 1:
        world_x = terrain_scan_y;
        world_y = TERRAIN_LAST_VERTEX - column;
        break;
    case 2:
        world_x = TERRAIN_LAST_VERTEX - column;
        world_y = TERRAIN_LAST_VERTEX - terrain_scan_y;
        break;
    default:
        world_x = TERRAIN_LAST_VERTEX - terrain_scan_y;
        world_y = column;
        break;
    }
    uint16_t index = terrain_index(world_x, world_y);
    union TerrainCell cell = {.raw = terrain_grid[index]};
    terrain_cached_cells[slot] = cell.raw;
    struct TerrainCornerEnd end;
    end.terrain_bank =
        (uint16_t)terrain_grid + ((uint16_t)(world_x & TERRAIN_STORAGE_BANK_MASK) << 8);
    uint8_t height;
    if (cell.object.tag == TERRAIN_OBJECT_TAG) {
        uint8_t object = cell.object.slot;
        do {
            object &= OBJECT_SLOT_MASK;
            height = object_height_integer[object];
            object = object_stack_links[object];
        } while (object >= OBJECT_LINK_SUPPORT);
    } else {
        uint8_t packed = ((world_x << 3) & 0xe0) | world_y;
        uint8_t bit = ((world_x & TERRAIN_STORAGE_BANK_MASK) << 1) | (packed & 1);
        height = cell.terrain.height;
        if (!(terrain_visibility_bits[packed >> 1] & terrain_visible_bit_masks[bit]))
            terrain_cached_cells[slot] = 0;
    }
    uint16_t height_delta = ((uint16_t)height << 8) -
        (((uint16_t)object_height_integer[view_object_slot] << 8) |
            object_height_fraction[view_object_slot]);
    uint16_t observer = (observer_bank & 0xff00) | view_object_slot;
    end.vertical = c_vertical_angle(height_delta, distance, observer);
    uint16_t screen_y;
    if (observer < 64) {
        screen_y = end.vertical.screen_y;
        publish_vertical_projection(&end.vertical);
    } else {
        /* The legacy invalid-observer operation preserves the preceding screen Y. */
        screen_y = projection_screen_y;
        publish_projection_axes(&bearing);
        publish_projection_bearing(&bearing);
        projection_abs_delta_x = height_delta;
    }
    projection_distance = distance;
    projected_y_high[slot] = screen_y >> 8;
    projected_y_low[slot] = (uint8_t)screen_y;
    if (projected_x_high[slot] > viewport_projected_x_limit ||
        (projected_x_high[slot] == viewport_projected_x_limit &&
            projected_x_low[slot] >= viewport_x_fraction_limit)) {
        projection_clip_status = TERRAIN_CORNER_CLIPPED_RIGHT;
        if (projected_x_high[slot] >= viewport_negative_x_threshold)
            projection_clip_status = TERRAIN_CORNER_CLIPPED_LEFT;
    }
    end.slot = slot;
    end.column = column;
    end.clip = projection_clip_status;
    return end;
}

struct TerrainStepEnd __far c_previous_terrain_corner(uint16_t observer_bank)
{
    struct TerrainStepEnd end = {0};
    end.column = terrain_scan_x;
    if (!end.column) {
        end.boundary = 1;
        return end;
    }
    --end.column;
    end.corner = c_project_terrain_corner(end.column, observer_bank);
    return end;
}

struct TerrainStepEnd __far c_next_terrain_corner(uint16_t observer_bank)
{
    struct TerrainStepEnd end = {0};
    end.column = terrain_scan_x + 1;
    if (end.column == TERRAIN_VERTEX_SIDE) {
        end.boundary = 2;
        return end;
    }
    end.corner = c_project_terrain_corner(end.column, observer_bank);
    return end;
}

static void terrain_interval_advance(
    struct TerrainIntervalEnd *interval, uint16_t observer_bank, int direction)
{
    struct TerrainStepEnd step = direction > 0 ? c_next_terrain_corner(observer_bank)
                                               : c_previous_terrain_corner(observer_bank);
    interval->column = step.column;
    interval->boundary = step.boundary;
    if (!step.boundary)
        interval->corner = step.corner;
}

struct TerrainIntervalEnd __far c_find_terrain_interval(uint16_t observer_bank)
{
    struct TerrainIntervalEnd end = {0};
    end.column = terrain_row_left;
    end.corner = c_project_terrain_corner(end.column, observer_bank);
    if (end.corner.clip == TERRAIN_CORNER_CLIPPED_LEFT) {
        /* Starting left of the viewport, retain the last clipped corner as a bracket. */
        for (;;) {
            terrain_row_left = terrain_scan_x;
            terrain_interval_advance(&end, observer_bank, 1);
            if (end.boundary)
                break;
            if (end.corner.clip == TERRAIN_CORNER_CLIPPED_LEFT)
                continue;
            if (end.corner.clip == TERRAIN_CORNER_CLIPPED_RIGHT)
                break;
            do {
                terrain_interval_advance(&end, observer_bank, 1);
            } while (!(end.boundary) && end.corner.clip == TERRAIN_CORNER_VISIBLE);
            break;
        }
        terrain_row_right = terrain_scan_x;
        end.scan = terrain_scan_x;
        return end;
    }
    if (end.corner.clip == TERRAIN_CORNER_VISIBLE) {
        /* Find the right bracket, then restart at the original visible corner. */
        do {
            terrain_interval_advance(&end, observer_bank, 1);
        } while (!(end.boundary) && end.corner.clip == TERRAIN_CORNER_VISIBLE);
        terrain_row_right = terrain_scan_x;
        terrain_scan_x = terrain_row_left;
    } else {
        /* Starting right of the viewport, walk left until visible or out of bounds. */
        do {
            terrain_row_right = terrain_scan_x;
            terrain_interval_advance(&end, observer_bank, -1);
        } while (!(end.boundary) && end.corner.clip == TERRAIN_CORNER_CLIPPED_RIGHT);
        if ((end.boundary) || end.corner.clip != TERRAIN_CORNER_VISIBLE) {
            terrain_row_left = terrain_scan_x;
            end.scan = terrain_scan_x;
            return end;
        }
    }
    do {
        terrain_interval_advance(&end, observer_bank, -1);
    } while (!(end.boundary) && end.corner.clip == TERRAIN_CORNER_VISIBLE);
    terrain_row_left = terrain_scan_x;
    end.scan = terrain_scan_x;
    return end;
}

struct TerrainCornerEnd __far terrain_corner_at(uint8_t column, uint16_t observer_bank)
{
    return c_project_terrain_corner(column, observer_bank);
}

/* Owned subsystem data. */
uint8_t terrain_visibility_bits[128] = {0};
uint8_t terrain_quadrant_corner_bias[4] = {
    0x00,
    0x01,
    0x21,
    0x20,
};
uint8_t terrain_corner_distance[64] = {0};
uint8_t terrain_corner_distance_tail[32] = {0};
uint8_t terrain_rows_remaining = 0x0;
uint8_t view_grid_x_overflow = 0x0;
struct TerrainShadingModeStorage terrain_shading_mode_storage = {0};
uint8_t terrain_visible_bit_masks[8] = {
    0x80,
    0x40,
    0x20,
    0x10,
    0x08,
    0x04,
    0x02,
    0x01,
};

/* TerrainWork is reset independently of other subsystems and data placement. */
struct TerrainWork terrain_work = {0};

void __far reset_terrain_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&terrain_work), 0, sizeof(terrain_work), 0);
}
