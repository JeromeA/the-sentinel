#ifndef SENTINEL_RENDER_STATE_H
#define SENTINEL_RENDER_STATE_H

/* Aggregate only outputs written by a render operation. Native entry callers
 * still observe these work values; typed callers pass model/buffer arguments. */

static void render_merge(struct RenderEnd *before, const struct RenderEnd *after)
{
    const struct PolygonScanEnd *next = &after->polygon;
    struct PolygonScanEnd *work = &before->polygon;
    work->value = next->value;
    work->flags = next->flags;
    if (next->changed & POLYGON_VERTEX)
        work->start_vertex = next->start_vertex;
    if (next->changed & POLYGON_COUNT)
        work->span_count = next->span_count;
    if (next->changed & POLYGON_END_SLOT)
        work->end_slot = next->end_slot;
    if (next->changed & POLYGON_DESTINATION)
        work->span_destination = next->span_destination;
    if (next->changed & POLYGON_VERTEX_OFFSET)
        work->vertex_offset = next->vertex_offset;
    if (next->changed & POLYGON_ERROR)
        work->error = next->error;
    work->changed |= next->changed;
    before->data_output |= after->data_output;
}

/* Arithmetic conditions belong to the operation or a genuine final boundary.
 * Geometry forwards only its live indices, output choice and copy direction. */
static void render_geometry_effect(struct RenderEnd *render, const struct RenderEffects *effects)
{
    if (effects->source_updated) {
        render->polygon.start_vertex = effects->source;
        render->polygon.changed |= POLYGON_VERTEX;
    }
    if (effects->target_updated) {
        render->polygon.end_slot = effects->target;
        render->polygon.changed |= POLYGON_END_SLOT;
    }
    if (effects->backwards)
        render->polygon.flags |= 0x400;
    else
        render->polygon.flags &= ~0x400;
    render->data_output |= effects->data_output;
    if (effects->interrupts_enabled)
        render->polygon.flags |= 0x200;
}

static void actor_render(struct ActorState *actor, const struct RenderEffects *render)
{
    if (render->source_updated)
        actor->source = render->source;
    if (render->target_updated)
        actor->target = render->target;
    actor->backwards = render->backwards;
    actor->interrupts_enabled |= render->interrupts_enabled;
    actor->data_output |= render->data_output;
}

static void actor_merge(struct ActorState *before, const struct ActorState *after)
{
    uint8_t output = before->data_output, enabled = before->interrupts_enabled;
    *before = *after;
    before->data_output |= output;
    before->interrupts_enabled |= enabled;
}

static void publish_actor_control(struct RenderEnd *render,
    const struct ActorState *actor,
    int source_updated,
    int target_updated)
{
    if (source_updated) {
        render->polygon.start_vertex = actor->source;
        render->polygon.changed |= POLYGON_VERTEX;
    }
    if (target_updated) {
        render->polygon.end_slot = actor->target;
        render->polygon.changed |= POLYGON_END_SLOT;
    }
    if (actor->backwards)
        render->polygon.flags |= 0x400;
    else
        render->polygon.flags &= ~0x400;
    if (actor->interrupts_enabled)
        render->polygon.flags |= 0x200;
    render->data_output |= actor->data_output;
}

static void render_cursor_effect(struct RenderEnd *, const struct CursorEffects *);
static void render_scatter_effect(struct RenderEnd *, uint8_t);

/* Effect-only presentation services used by native gameplay. Raw arithmetic
 * is reconstructed only when an original-entry caller supplies a sink. */
static void merge_render_effects(
    struct RenderEffects *before, const struct RenderEffects *after, struct RenderEnd *observation)
{
    if (after->source_updated) {
        before->source = after->source;
        before->source_updated = 1;
    }
    if (after->target_updated) {
        before->target = after->target;
        before->target_updated = 1;
    }
    before->backwards = after->backwards;
    before->interrupts_enabled |= after->interrupts_enabled;
    before->data_output |= after->data_output;
    if (observation)
        render_geometry_effect(observation, after);
}

static void gameplay_cursor(
    struct RenderEffects *effects, SegmentBytes data, int restoring, struct RenderEnd *observation)
{
    struct CursorEffects cursor = operate_cursor(
        data, effects->source, restoring, observation ? &observation->polygon.flags : 0, 0);
    if (!cursor.invoked)
        return;
    if (cursor.restoring || cursor.executed) {
        effects->source = cursor.lookup_bank;
        effects->source_updated = 1;
        effects->data_output = 1;
        if (!cursor.restoring && cursor.mode == 1) {
            effects->target = (effects->target & 0xff00) | 0xee;
            effects->target_updated = 1;
        }
    }
    if ((cursor.restoring && cursor.mode == 2) || (!cursor.restoring && cursor.mode >= 2)) {
        effects->target = 0;
        effects->target_updated = 1;
    }
    if (observation)
        render_cursor_effect(observation, &cursor);
}

static struct RenderEffects gameplay_background(SegmentBytes data,
    uint16_t row,
    uint16_t columns,
    uint16_t initial_word,
    int backwards,
    struct RenderEnd *observation)
{
    struct RenderEffects effects = {0};
    effects.backwards = backwards;
    effects.target = row;
    effects.target_updated = 1;
    BackgroundCallback binding = segment_record(data, graphics_session).background;
    if (observation) {
        uint16_t flags = observation->polygon.flags;
        *observation = (struct RenderEnd){0};
        observation->polygon.flags = flags;
        observation->polygon.value = initial_word;
        observation->polygon.end_slot = row;
        observation->polygon.changed = POLYGON_END_SLOT;
    }
    if (!binding)
        return effects;
    call_video_background(binding, row, columns, backwards, 0);
    effects.source = effects.target = 0;
    effects.source_updated = 1;
    effects.data_output = 1;
    if (observation) {
        observation->polygon.start_vertex = observation->polygon.end_slot = 0;
        observation->polygon.changed |= POLYGON_VERTEX;
        observation->polygon.flags = status(observation->polygon.flags, 0, 16);
        observation->data_output = 1;
    }
    return effects;
}

static struct RenderEffects gameplay_hud(
    SegmentBytes data, int clear, int backwards, struct RenderEnd *observation)
{
    int ega = draw_hud(data, clear, backwards);
    struct RenderEffects effects = {0};
    effects.backwards = backwards;
    effects.source_updated = effects.target_updated = effects.data_output = 1;
    effects.interrupts_enabled = !!ega;
    if (observation) {
        observation->polygon.flags = status(observation->polygon.flags, 0, 16) | (ega ? 0x200 : 0);
        observation->polygon.start_vertex = observation->polygon.end_slot = 0;
        observation->polygon.changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
        observation->data_output = 1;
    }
    return effects;
}

static void gameplay_scatter(
    struct RenderEffects *effects, uint8_t scatter, struct RenderEnd *observation)
{
    if (!(scatter & 1))
        return;
    effects->source = effects->target = 0;
    effects->source_updated = effects->target_updated = 1;
    effects->data_output = 1;
    effects->interrupts_enabled |= !!(scatter & 2);
    if (observation)
        render_scatter_effect(observation, scatter);
}

static void gameplay_reveal(
    struct RenderEffects *effects, SegmentBytes data, struct RenderEnd *observation)
{
    RevealCallback binding = graphics_reveal_selection_pixels;
    if (!binding)
        return;
    uint8_t enabled = dissolve_selection(binding, effects->backwards);
    effects->source = effects->target = 0;
    effects->source_updated = effects->target_updated = 1;
    effects->data_output = 1;
    effects->interrupts_enabled |= enabled;
    if (observation) {
        observation->polygon.start_vertex = observation->polygon.end_slot = 0;
        observation->polygon.changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
        observation->data_output = 1;
        observation->polygon.flags =
            status(observation->polygon.flags, 0, 16) | (enabled ? 0x200 : 0);
    }
}

static void gameplay_selection_copy(struct RenderEffects *effects,
    SegmentBytes data,
    uint16_t columns,
    uint16_t origin,
    struct RenderEnd *observation)
{
    SelectionCopyCallback binding = graphics_copy_selection_strip;
    if (!binding) {
        effects->source = origin;
        effects->source_updated = 1;
        if (observation) {
            observation->polygon.value = columns;
            observation->polygon.start_vertex = origin;
            observation->polygon.changed |= POLYGON_VERTEX;
        }
        return;
    }
    present_selection_band(binding, columns, origin, effects->backwards);
    effects->source = effects->target = 0;
    effects->source_updated = effects->target_updated = 1;
    effects->data_output = 1;
    if (observation) {
        observation->polygon.start_vertex = observation->polygon.end_slot = 0;
        observation->polygon.changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
        observation->polygon.flags = status(observation->polygon.flags, 0, 16);
        observation->data_output = 1;
    }
}

/* Scheduler continuations need actor/target indices and rendering side effects,
 * not the terminal polygon accumulator, counters, or raster addresses. */
static void sentry_render(struct SentryState *state, const struct RenderEnd *render)
{
    if (render->polygon.changed & POLYGON_VERTEX)
        state->source = render->polygon.start_vertex;
    if (render->polygon.changed & POLYGON_END_SLOT)
        state->target = render->polygon.end_slot;
    state->flags = render->polygon.flags;
    state->data_output |= render->data_output;
}

static struct RenderEnd render_corner_observation(const struct TerrainCornerEnd *corner,
    uint16_t observer_bank,
    uint16_t position_bank,
    uint16_t flags)
{
    struct RenderEnd end = {0};
    struct PolygonScanEnd *work = &end.polygon;
    work->value = (corner->vertical.screen_y & 0xff00) | corner->clip;
    work->flags = status(flags, corner->clip, 8);
    work->start_vertex = (observer_bank & 0xff00) | view_object_slot;
    work->span_count = corner->vertical.pitch;
    work->end_slot = (position_bank & 0xff00) | corner->column;
    work->vertex_offset = (position_bank & 0xff00) | corner->slot;
    work->span_destination = corner->vertical.bearing.major;
    work->error = corner->terrain_bank;
    work->changed = 63;
    return end;
}

static struct RenderEnd render_blit(SegmentBytes data, enum RenderPanel panel, uint16_t flags)
{
    draw_panel(panel, !!(flags & 0x400));
    struct RenderEnd result = {0};
    result.polygon.flags = flags;
    result.data_output = 1;
    return result;
}

static struct RenderEnd render_hud(SegmentBytes data, int clear, uint16_t flags)
{
    int ega = draw_hud(data, clear, !!(flags & 0x400));
    struct RenderEnd result = {0};
    result.polygon.flags = status(flags, 0, 16) | (ega ? 0x200 : 0);
    /* Subsequent selection/actor operations use these zero indices. */
    result.polygon.changed = POLYGON_VERTEX | POLYGON_END_SLOT;
    result.data_output = 1;
    return result;
}

static struct RenderEnd render_background_band(
    SegmentBytes data, uint16_t row, uint16_t columns, uint16_t initial_word, uint16_t flags)
{
    BackgroundCallback binding = graphics_fill_background_band;
    struct RenderEnd result = {0};
    result.polygon.value = initial_word;
    result.polygon.end_slot = row;
    result.polygon.flags = flags;
    result.polygon.changed = POLYGON_END_SLOT;
    if (!binding)
        return result;
    call_video_background(binding, row, columns, !!(flags & 0x400), 0);
    result.polygon.start_vertex = result.polygon.end_slot = 0;
    result.polygon.flags = status(flags, 0, 16);
    result.polygon.changed |= POLYGON_VERTEX;
    result.data_output = 1;
    return result;
}

static void render_scatter_effect(struct RenderEnd *render, uint8_t effects)
{
    if (!(effects & 1))
        return;
    struct PolygonScanEnd *work = &render->polygon;
    if (effects & 2)
        work->flags |= 0x200;
    work->start_vertex = work->end_slot = 0;
    work->changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
    work->flags = status(work->flags, 0, 16);
    render->data_output = 1;
}

static void render_cursor_effect(struct RenderEnd *render, const struct CursorEffects *cursor)
{
    if (!cursor->invoked)
        return;
    struct PolygonScanEnd *work = &render->polygon;
    if (cursor->restoring || cursor->executed) {
        work->start_vertex = cursor->lookup_bank;
        work->changed |= POLYGON_VERTEX;
        render->data_output = 1;
        if (!cursor->restoring && cursor->mode == 1) {
            work->end_slot = (work->end_slot & 0xff00) | 0xee;
            work->changed |= POLYGON_END_SLOT;
        }
    }
    if ((cursor->restoring && cursor->mode == 2) || (!cursor->restoring && cursor->mode >= 2)) {
        work->end_slot = 0;
        work->changed |= POLYGON_END_SLOT;
    }
}

static struct RenderEnd render_selection_interval(
    const struct SelectionIntervalEnd *interval, uint16_t flags)
{
    struct RenderEnd result = {0};
    struct PolygonScanEnd *work = &result.polygon;
    work->end_slot = work->span_destination = interval->selected_index;
    work->changed = POLYGON_END_SLOT | POLYGON_DESTINATION;
    if (!interval->prepared) {
        work->flags = subtract8(flags, interval->selected, player_robot_slot) | CF;
        return result;
    }
    work->start_vertex = interval->model_index;
    work->span_destination = interval->vertical.bearing.major;
    work->vertex_offset = bearing_table_index(&interval->vertical.bearing);
    work->span_count = interval->vertical.pitch;
    work->value = interval->vertical.screen_y;
    work->changed |= POLYGON_VERTEX | POLYGON_VERTEX_OFFSET | POLYGON_COUNT;
    work->flags = (status(flags, work->value, 16) & ~CF) |
        (((uint16_t)(interval->vertical.angle - interval->vertical.pitch) & 8) ? CF : 0) |
        (subtract16(flags, interval->vertical.angle, interval->vertical.pitch) & AF);
    if (interval->accepted) {
        work->value = (work->value & 0xff00) | interval->columns;
        work->flags &= ~CF;
    } else
        work->flags |= CF;
    return result;
}

static void render_reveal(struct RenderEnd *render, SegmentBytes data)
{
    struct PolygonScanEnd *work = &render->polygon;
    RevealCallback binding = graphics_reveal_selection_pixels;
    int backwards = !!(work->flags & 0x400);
    if (!binding)
        return;
    uint8_t enabled = dissolve_selection(binding, backwards);
    render->data_output = 1;
    work->start_vertex = work->end_slot = 0;
    work->changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
    if (enabled)
        work->flags |= 0x200;
    work->flags = status(work->flags, 0, 16);
}

static void render_copy_selection(
    struct RenderEnd *render, SegmentBytes data, uint16_t columns, uint16_t origin)
{
    struct PolygonScanEnd *work = &render->polygon;
    SelectionCopyCallback binding = graphics_copy_selection_strip;
    int backwards = !!(work->flags & 0x400);
    if (!binding) {
        work->value = columns;
        work->start_vertex = origin;
        work->changed |= POLYGON_VERTEX;
        return;
    }
    present_selection_band(binding, columns, origin, backwards);
    render->data_output = 1;
    work->start_vertex = work->end_slot = 0;
    work->changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
    work->flags = status(work->flags, 0, 16);
}

static struct TerrainCellAddress render_terrain_address(struct RenderEnd *render)
{
    struct TerrainCellAddress cell = c_terrain_address(terrain_scan_x, terrain_scan_y);
    struct PolygonScanEnd *work = &render->polygon;
    work->error = cell.bank;
    work->span_destination = cell.offset;
    work->value = (cell.bank & 0xff00) | cell.value.raw;
    work->flags = subtract8(work->flags, cell.value.raw, 0xc0) ^ CF;
    work->changed |= POLYGON_ERROR | POLYGON_DESTINATION;
    return cell;
}

static void render_attachment_observation(
    struct RenderEnd *render, const struct ObjectAttachEnd *attached)
{
    struct PolygonScanEnd *work = &render->polygon;
    work->error = attached->cell.bank;
    work->span_destination = attached->cell.offset;
    work->value = (attached->cell.bank & 0xff00) | attached->cell.value.raw;
    work->changed |= POLYGON_ERROR | POLYGON_DESTINATION;
    if (attached->cell.value.object.tag == TERRAIN_OBJECT_TAG) {
        work->vertex_offset = attached->support_slot;
        work->value = attached->support_model;
        work->changed |= POLYGON_VERTEX_OFFSET;
        work->flags = subtract8(
            work->flags, attached->support_model, attached->support_model == MODEL_BOULDER ? 3 : 6);
        if (!attached->accepted) {
            work->flags |= CF;
            return;
        }
    }
    work->value = ((uint16_t)attached->random_high << 8) | attached->heading;
    work->span_count = 0;
    work->changed |= POLYGON_COUNT;
    work->flags = add8(work->flags, attached->random & 0xf8, 0x60) & ~CF;
}

static void render_detach_observation(
    struct RenderEnd *render, const struct ObjectDetachEnd *detached)
{
    struct PolygonScanEnd *work = &render->polygon;
    work->error = detached->cell.bank;
    work->span_destination = detached->cell.offset;
    work->value = (detached->cell.bank & 0xff00) | detached->value;
    work->flags = status(work->flags, detached->value, 8);
    work->changed |= POLYGON_ERROR | POLYGON_DESTINATION;
    if (detached->support < OBJECT_LINK_SUPPORT)
        work->flags |= ((detached->height & 16) ? CF : 0) |
            ((detached->height & 24) == 8 || (detached->height & 24) == 16 ? OF : 0);
}

static int render_change_energy(struct RenderEnd *render, uint16_t slot, int debit)
{
    struct EnergyChange change;
    int accepted = c_adjust_energy(slot, debit, &change);
    struct PolygonScanEnd *work = &render->polygon;
    work->end_slot = (work->end_slot & 0xff00) | change.model;
    work->span_destination = work->end_slot;
    work->changed |= POLYGON_END_SLOT | POLYGON_DESTINATION;
    if (accepted) {
        work->value = (work->value & 0xff00) | change.after;
        work->flags = status(work->flags, change.after, 8);
    } else {
        work->value = (work->value & 0xff00) | (uint8_t)(change.before - change.cost);
        work->flags = subtract8(work->flags, change.before, change.cost) | CF;
    }
    return accepted;
}

static int render_allocate(struct RenderEnd *render, int model)
{
    int slot = model < 0 ? c_allocate_saved() : c_allocate_model(model);
    if (!render)
        return slot;
    struct PolygonScanEnd *work = &render->polygon;
    work->changed |= POLYGON_VERTEX;
    if (slot < 0) {
        work->start_vertex = 255;
        work->value = (work->value & 0xff00) | object_stack_links[0];
        work->flags = (work->flags & ~ARITH_FLAGS) | CF | PF | AF | SF;
    } else {
        work->start_vertex = slot;
        work->value = (work->value & 0xff00) | pending_object_model;
        work->flags = status(work->flags, object_stack_links[slot], 8);
    }
    return slot;
}

static struct ObjectPlacementEnd render_place_object(
    struct RenderEnd *render, SegmentBytes data, uint16_t slot, uint8_t height)
{
    struct ObjectPlacementEnd placed = c_place_object_flat(slot, height, data);
    struct PolygonScanEnd *work = &render->polygon;
    work->span_count = 0;
    work->changed |= POLYGON_COUNT;
    if (placed.found) {
        render_attachment_observation(render, &placed.attachment);
        work->flags &= ~CF;
    } else {
        work->error = placed.failure_cell.bank;
        work->span_destination = placed.failure_cell.offset;
        work->value = (placed.failure_cell.bank & 0xff00) | placed.height_limit;
        work->changed |= POLYGON_ERROR | POLYGON_DESTINATION;
        work->flags = subtract8(work->flags, placed.height_limit, 12) | CF;
    }
    return placed;
}

static void render_hyperspace(struct RenderEnd *render, SegmentBytes data)
{
    struct HyperspaceEnd jump = c_hyperspace(data);
    struct PolygonScanEnd *work = &render->polygon;
    work->start_vertex = jump.robot;
    work->span_count = 0;
    work->changed |= POLYGON_VERTEX | POLYGON_COUNT;
    if (!jump.placement.found) {
        work->error = jump.placement.failure_cell.bank;
        work->span_destination = jump.placement.failure_cell.offset;
        work->value = (jump.placement.failure_cell.bank & 0xff00) | jump.placement.height_limit;
        work->changed |= POLYGON_ERROR | POLYGON_DESTINATION;
        work->flags = subtract8(work->flags, jump.placement.height_limit, 12) | CF;
        return;
    }
    render_attachment_observation(render, &jump.placement.attachment);
    work->end_slot = (work->end_slot & 0xff00) | jump.energy.model;
    work->span_destination = work->end_slot;
    work->changed |= POLYGON_END_SLOT | POLYGON_DESTINATION;
    if (jump.stage == 1) {
        render_detach_observation(render, &jump.detached);
    } else {
        work->value = 128;
        work->start_vertex = jump.robot;
        work->flags = status(work->flags, 128, 8);
    }
    work->flags &= ~CF;
}

static void render_palette_mode(
    struct RenderEnd *render, SegmentBytes data, SegmentBytes output, uint16_t phase, int observed)
{
    struct PaletteEnd palette;
    struct PaletteEffect effect =
        apply_scene_palette(data, output, phase, render->polygon.flags, observed ? &palette : 0);
    if (!observed) {
        render->polygon.value = effect.value;
        render->polygon.flags = effect.flags;
        if (effect.operated) {
            render->polygon.start_vertex = render->polygon.end_slot = 0;
            render->polygon.changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
        }
        render->data_output |= effect.data_output;
        return;
    }
    struct PolygonScanEnd *work = &render->polygon;
    work->value = phase;
    if (!(palette.changed & PALETTE_NO_OPERATION)) {
        work->start_vertex = work->end_slot = 0;
        work->changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
    }
    work->flags = palette.flags;
    if (palette.changed & PALETTE_VALUE)
        work->value = palette.value;
    if (palette.changed & PALETTE_COUNT) {
        work->span_count = palette.count;
        work->changed |= POLYGON_COUNT;
    }
    if (palette.changed & PALETTE_ATTRIBUTE_COUNT) {
        work->span_count = (work->span_count & 0xff00) | 4;
        work->changed |= POLYGON_COUNT;
    }
    if (palette.changed & PALETTE_SOURCE) {
        work->vertex_offset = palette.source;
        work->changed |= POLYGON_VERTEX_OFFSET;
    }
    if (palette.changed & PALETTE_DESTINATION) {
        work->span_destination = palette.destination;
        work->changed |= POLYGON_DESTINATION;
    }
    if (palette.changed & PALETTE_DATA_OUTPUT)
        render->data_output = 1;
}

static void render_palette(
    struct RenderEnd *render, SegmentBytes data, SegmentBytes output, uint16_t phase)
{
    render_palette_mode(render, data, output, phase, 1);
}

static void render_workspace_observation(
    struct RenderEnd *render, const struct WorkspaceEnd *workspace)
{
    struct PolygonScanEnd *work = &render->polygon;
    if (workspace->y_min == 255) {
        work->flags |= CF;
        return;
    }
    work->span_count = 0;
    work->value = (work->value & 0xff00) | workspace->y_min;
    work->end_slot = work->span_destination = 1;
    work->changed |= POLYGON_COUNT | POLYGON_END_SLOT | POLYGON_DESTINATION;
    work->flags = (add8(work->flags, workspace->previous_noise, 1) & ~CF) | (work->flags & CF);
}

/* Only target1 continues from native initialization. Arithmetic observations
 * remain at original-entry gates; scene callers finalize their own status. */
static void publish_initialized_target(struct RenderEnd *render, int initialized)
{
    if (!initialized)
        return;
    render->polygon.end_slot = render->polygon.span_destination = 1;
    render->polygon.changed |= POLYGON_END_SLOT | POLYGON_DESTINATION;
}

static void render_workspace_mode(
    struct RenderEnd *render, SegmentBytes data, SegmentBytes output, int observed)
{
    struct WorkspaceEnd workspace;
    int initialized = initialize_scene(
        data, output, !!(render->polygon.flags & 0x400), observed ? &workspace : 0);
    if (observed)
        render_workspace_observation(render, &workspace);
    else
        publish_initialized_target(render, initialized);
}

static void render_workspace_final_mode(struct RenderEnd *render, SegmentBytes output, int observed)
{
    struct WorkspaceEnd workspace;
    int initialized = finish_scene_initialization(
        output, !!(render->polygon.flags & 0x400), observed ? &workspace : 0);
    render->polygon.value = 0;
    if (observed)
        render_workspace_observation(render, &workspace);
    else
        publish_initialized_target(render, initialized);
}

static void render_text(struct RenderEnd *render, SegmentBytes data, TextScriptId script)
{
    if (write_text_script(data, script, !!(render->polygon.flags & 0x400)))
        render->data_output = 1;
    /* A completed valid script leaves zero actor/target banks for subsequent
     * native rendering. Invalid full-width IDs do not publish those indices. */
    if (script < TEXT_SCRIPT_COUNT) {
        render->polygon.start_vertex = render->polygon.end_slot = 0;
        render->polygon.changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
    }
}

static int render_wait_key_mode(
    struct RenderEnd *render, SegmentBytes data, SegmentBytes scans, int observed)
{
    struct KeyWaitEnd raw;
    struct InputKey key = wait_fresh_key(data, scans, render->polygon.flags, observed ? &raw : 0);
    struct PolygonScanEnd *work = &render->polygon;
    if (observed) {
        work->value = key.character;
        work->span_count = key.remaining;
        work->span_destination = raw.scan;
        work->changed |= POLYGON_COUNT | POLYGON_DESTINATION;
    }
    work->flags = key.flags;
    return key.leave;
}

static uint8_t render_select_landscape(struct RenderEnd *render, union LandscapeNumber landscape)
{
    uint8_t difficulty = c_select_landscape(landscape);
    struct PolygonScanEnd *work = &render->polygon;
    work->start_vertex = (landscape.raw & 0xff00) | difficulty;
    work->changed |= POLYGON_VERTEX;
    work->flags = landscape.pairs.high.raw
        ? status(work->flags, landscape.pairs.high.raw, 8)
        : subtract8(work->flags, landscape.pairs.low.digits.tens + 1, 9);
    return difficulty;
}

static void render_pack_digits_mode(
    struct RenderEnd *render, SegmentBytes data, SegmentBytes output, int observed)
{
    struct DecimalPackEnd raw;
    uint16_t packed =
        pack_decimal_digits(data, output, !!(render->polygon.flags & 0x400), observed ? &raw : 0);
    struct PolygonScanEnd *work = &render->polygon;
    if (observed) {
        work->value = packed;
        work->start_vertex = raw.last_low;
        work->span_count = 0;
        work->vertex_offset = raw.source_end;
        work->span_destination = raw.destination_end;
        work->changed |=
            POLYGON_VERTEX | POLYGON_COUNT | POLYGON_VERTEX_OFFSET | POLYGON_DESTINATION;
    }
    work->flags = status(work->flags, packed, 16);
}

static void render_sound_effect(struct RenderEnd *render, uint8_t effect)
{
    uint8_t observed = c_sound_effect(effect);
    render->polygon.value = (render->polygon.value & 0xff00) | effect;
    render->polygon.flags = status(render->polygon.flags, observed, 8) | 0x200;
}

static void render_collect_actions(
    struct RenderEnd *render, SegmentBytes scans, uint16_t binding, uint16_t scan_bank)
{
    struct PolygonScanEnd *work = &render->polygon;
    struct ActionScanEnd actions;
    c_collect_actions(scans, !!(work->flags & 0x400), binding, scan_bank, &actions);
    work->span_destination = actions.binding_end;
    work->start_vertex = actions.last_slot;
    work->span_count = 0;
    work->end_slot = binding;
    work->value = (work->value & 0xff00) | actions.combined_action;
    work->changed |= POLYGON_DESTINATION | POLYGON_VERTEX | POLYGON_COUNT | POLYGON_END_SLOT;
    work->flags = status(work->flags, actions.combined_action, 8);
}

static void render_transition(struct RenderEnd *render, SegmentBytes data)
{
    struct PolygonScanEnd *work = &render->polygon;
    TransitionCallback binding = graphics_view_transition;
    int backwards = !!(work->flags & 0x400);
    if (!binding)
        return;
    if (present_view_transition(binding, data, backwards, &work->flags)) {
        render->data_output = 1;
        work->start_vertex = work->end_slot = 0;
        work->changed |= POLYGON_VERTEX | POLYGON_END_SLOT;
    }
}
#endif
