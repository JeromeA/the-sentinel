
/* Transition dissolve: a separate 32-bit multiplicative sequence, independent
 * of landscape generation. Advance it once for each requested pixel. */
/* Each effect owns its low/high pair; this is not the landscape PRNG. */
static uint16_t advance_transition_noise(uint16_t *low, uint16_t *high)
{
    uint32_t state = ((uint32_t)*high << 16) | *low;
    state *= 5;
    *low = (uint16_t)state;
    *high = state >> 16;
    return *high;
}

uint8_t __far scatter_pixels(
    uint16_t count, uint8_t color, struct ScatterEnd *end, enum VideoPixelLayout mode)
{
    uint16_t remaining = count;
    uint8_t interrupts_enabled = 0;
    if (end)
        end->mask_updated = end->interrupts_enabled = 0;
    SegmentBytes target =
        segment_bytes(mode == 0 ? (cga_palette_phase == 2 ? video_work_buffer_segment : 0xb800)
                : mode == 1     ? 0xb000
                                : 0xa000);
    do {
        if (end)
            end->noise_product_high =
                (pixel_scatter_sequence_high << 2) | (pixel_scatter_sequence_low >> 14);
        uint16_t high =
            advance_transition_noise(&pixel_scatter_sequence_low, &pixel_scatter_sequence_high);
        uint16_t destination = high;
        if (end)
            end->destination_offset = high;
        if (mode < 2) {
            uint16_t mask = high & (mode == 0 ? 3 : 1);
            if (end) {
                end->mask_index = mask;
                end->mask_updated = 1;
            }
            destination = high >> (mode == 0 ? 2 : 1);
            if (end)
                end->destination_offset = destination;
            uint8_t pixel = target[destination] &
                (mode == 0 ? cga_clear_pixel_masks[mask] : tandy_clear_pixel_masks[mask]);
            if (color)
                pixel |= mode == 0 ? cga_set_pixel_masks[mask] : tandy_set_pixel_masks[mask];
            target[destination] = pixel;
        } else if (mode == 2) {
            if (!(ega_polygon_busy & 0x80)) {
                uint16_t mask = high & 7;
                if (end) {
                    end->mask_index = mask;
                    end->mask_updated = 1;
                }
                destination = (high >> 3) + video_page_offset;
                if (end)
                    end->destination_offset = destination;
                __asm__ volatile("cli" ::: "memory");
                port_out16(0x3ce, ((uint16_t)ega_single_pixel_masks[mask] << 8) | 8);
                port_out16(0x3ce, 0x0a05);
                target[destination] &= color;
                port_out16(0x3ce, 0x105);
                __asm__ volatile("sti" ::: "memory");
                interrupts_enabled = 1;
                if (end)
                    end->interrupts_enabled = 1;
            }
        } else {
            if (end)
                end->noise_product_high = 0xa000;
            target[high] = color ? 14 : 0;
        }
    } while (--remaining);
    return interrupts_enabled;
}
