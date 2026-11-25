#include "render_state.h"
/* A selected object determines a clipped angular interval. Wider intervals are
 * rendered as two passes, then copied or dissolved over the main view. */

extern uint8_t selected_object_slot;

struct SelectionIntervalEnd __far c_prepare_selection_interval(
    uint8_t selected, uint16_t observer_bank, uint16_t target_bank)
{
    struct SelectionIntervalEnd end = {0};
    end.selected = selected;
    end.selected_index = (target_bank & 0xff00) | selected;
    if (selected == player_robot_slot || selected >= 64 || (observer_bank & 0xff00) ||
        (target_bank & 0xff00))
        return end;
    if (object_model_id[selected] >= 7)
        return end;
    int valid_origin = view_object_slot < 64;
    struct ModelOrigin origin = prepare_model_origin(end.selected_index, end.selected_index, 0);
    uint16_t distance = valid_origin ? origin.distance : projection_distance;
    end.model_index = (observer_bank & 0xff00) | object_model_id[end.selected_index];
    uint8_t extent = model_selection_extents[end.model_index];
    if (extent < selection_extent_override)
        extent = selection_extent_override;
    selection_extent_override = 0;
    /* Preserve the shipped model-index pitch lookup at this entry. */
    end.vertical = c_vertical_angle(extent, distance, end.model_index);
    publish_vertical_projection(&end.vertical);
    end.prepared = 1;
    uint16_t bound = model_azimuth_bias - end.vertical.angle;
    uint8_t left;
    if ((int16_t)bound < 0)
        left = 0;
    else {
        left = (uint16_t)(bound * 2) >> 8;
        if (left >= 40)
            return end;
    }
    selection_view_column = left;
    selection_view_origin_column = left;
    bound = model_azimuth_bias + end.vertical.angle;
    if ((int16_t)bound < 0)
        return end;
    uint8_t right = (uint16_t)(bound * 2) >> 8;
    if (right >= 40)
        right = 39;
    uint8_t width = right + 1 - left;
    selection_view_columns_left = width;
    selection_view_total_columns = width;
    if (!width)
        return end;
    end.columns = width >= 21 ? 20 : width;
    render_pass_columns = end.columns;
    end.accepted = 1;
    return end;
}

struct RenderEffects __far present_selected_effects(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t target_bank,
    int backwards,
    struct RenderEnd *observation)
{
    struct SelectionIntervalEnd interval = c_prepare_selection_interval(
        segment_record(data, game_work).work_allocated_object_slot, observer_bank, target_bank);
    struct RenderEffects end = {0};
    end.backwards = backwards;
    end.source = interval.prepared ? interval.model_index : 0;
    end.target = interval.selected_index;
    end.source_updated = interval.prepared;
    end.target_updated = 1;
    if (observation)
        *observation = render_selection_interval(&interval, observation->polygon.flags);
    struct PolygonScanEnd *work = observation ? &observation->polygon : 0;
    if (interval.accepted) {
        selection_copy_iteration_budget = 25;
        if (selection_presentation_flags & 128) {
            cursor_update_guard_flags |= 128;
            gameplay_cursor(&end, data, 1, observation);
            cursor_update_guard_flags &= 127;
        }
        {
            uint8_t total_columns = selection_view_total_columns;
            struct RenderEnd raw_background;
            if (observation) {
                raw_background = (struct RenderEnd){0};
                raw_background.polygon.flags = work->flags;
            }
            struct RenderEffects background = gameplay_background(
                data, 24, total_columns, 25, end.backwards, observation ? &raw_background : 0);
            merge_render_effects(&end, &background, 0);
            if (observation)
                render_merge(observation, &raw_background);
            if (end.data_output)
                output = data;
        }

        /* Each terrain pass restores the live viewer's heading. Model-only
         * drawing leaves its adjusted heading in place. */
        for (;;) {
            uint16_t view_slot = view_object_slot;
            uint8_t heading = object_heading[view_slot];
            uint8_t column = selection_view_column;
            view_azimuth_reference = (uint16_t)column * 128;
            selection_saved_heading = heading;
            object_heading[view_slot] = heading + view_heading_byte;
            camera_transition_mode = 0;
            c_viewport_width(render_pass_columns);
            uint8_t columns = render_pass_columns;
            int backwards = end.backwards;
            uint16_t observer = end.source;
            struct RenderEffects drawn;
            int model_only = selection_model_draw_flags & 128;
            if (model_only)
                drawn = draw_mesh(data,
                    output,
                    observer,
                    segment_record(data, game_work).work_allocated_object_slot,
                    segment_record(data, game_work).work_allocated_object_slot,
                    backwards);
            else
                drawn = draw_terrain_view(data, output, backwards);
            /* Keep the pass setup as fallback metadata when drawing leaves
             * those result fields untouched. */
            end.target = 0;
            end.target_updated = 1;
            if (observation) {
                work->span_destination = 0;

                work->span_count = (work->span_count & 0xff00) | columns;
            }
            end.target_updated = 1;
            if (observation) {
                work->changed |= POLYGON_END_SLOT | POLYGON_DESTINATION | POLYGON_COUNT;
            }
            merge_render_effects(&end, &drawn, observation);
            if (model_only)
                break;
            if (end.data_output)
                output = data;
            uint16_t restored_viewer = (end.source & 0xff00) | view_object_slot;
            end.source = restored_viewer;
            end.source_updated = 1;
            if (observation) {
                work->changed |= POLYGON_VERTEX;
            }
            view_azimuth_reference &= 0xff00;
            object_heading[restored_viewer] = selection_saved_heading;
            /* Drawing can replace the live pass width and remaining columns. */
            selection_view_column += render_pass_columns;
            uint8_t remaining = selection_view_columns_left - render_pass_columns;
            if (!remaining)
                break;
            selection_view_columns_left = remaining;
            render_pass_columns = remaining;
            raster_x_word_offset = 40;
        }

        raster_x_word_offset = 0;
        selection_copy_origin = (int16_t)(int8_t)selection_view_origin_column;
        int copy_strip = 1;
        if (selection_presentation_flags & 64) {
            if (game_end_flags & 128)
                selection_copy_iteration_budget = 40;
            gameplay_reveal(&end, data, observation);
            /* Reveal may change game-end state; decide again after it returns. */
            copy_strip = !(game_end_flags & 128);
        }
        if (copy_strip) {
            cursor_update_guard_flags |= 128;
            gameplay_cursor(&end, data, 1, observation);
            software_cursor_hide_flags |= 128;
            cursor_update_guard_flags &= 127;
            gameplay_selection_copy(&end,
                data,
                (int16_t)(int8_t)selection_view_total_columns,
                selection_copy_origin,
                observation);
        }
        software_cursor_hide_flags &= 127;
        if (cursor_mode_flags & 128) {
            cursor_update_guard_flags |= 128;
            gameplay_cursor(&end, data, 0, observation);
            cursor_update_guard_flags &= 127;
        }
    }
    /* Rejected intervals share the same unconditional policy/result cleanup. */
    selection_presentation_flags = 0;
    selection_model_draw_flags = 0;
    selection_copy_interrupt_flags = 0;
    if (observation) {
        work->value = 0;

        work->flags = status(work->flags, 0, 16);

        observation->polygon.start_vertex = end.source;
        observation->polygon.end_slot = end.target;
        observation->data_output = end.data_output;
    }
    return end;
}

struct RenderEffects __far render_selected_effects(
    SegmentBytes data, SegmentBytes output, uint16_t observer, uint16_t target, int backwards)
{
    return present_selected_effects(data, output, observer, target, backwards, 0);
}

struct RenderEnd __far c_render_selection(SegmentBytes data,
    SegmentBytes output,
    uint16_t observer_bank,
    uint16_t target_bank,
    uint16_t flags)
{
    struct RenderEnd end = {0};
    end.polygon.flags = flags;
    present_selected_effects(data, output, observer_bank, target_bank, !!(flags & 0x400), &end);
    return end;
}

/* Owned subsystem data. */
uint8_t selection_view_total_columns = 0x0;
uint8_t selection_view_origin_column = 0x0;
uint8_t selection_saved_heading = 0x0;
volatile uint8_t selection_copy_iteration_budget = 0x0;
uint16_t selection_copy_origin = 0x0;

/* SelectionWork is reset independently of other subsystems and data placement. */
struct SelectionWork selection_work = {0};

void __far reset_selection_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&selection_work), 0, sizeof(selection_work), 0);
}
