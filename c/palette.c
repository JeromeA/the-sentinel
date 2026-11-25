#include <stdint.h>
#include "register_compat.h"
#include "typed_api.h"
#include "legacy_targets.h"
#include "data_aliases.h"
#define RUNTIME_LINKAGE static __attribute__((used))
#include "runtime_support.c"

/* Scene palettes and sixteen VGA distance-shading banks. */
extern uint8_t cga_scene_palettes[], tandy_scene_palettes[], ega_scene_palettes[];
extern uint8_t vga_attribute_identity[];
extern uint8_t cga_palette_variant_selectors[], cga_background_variant_controls[];
extern uint8_t tandy_landscape_color_1_variants[], tandy_landscape_color_2_variants[],
    tandy_landscape_color_3_variants[];
extern uint8_t ega_landscape_color_1_variants[], ega_landscape_color_2_variants[],
    ega_landscape_color_3_variants[];

/* Three phase records per palette bank; phases intentionally share one array. */
uint8_t cga_scene_palettes[6] = {
    0x00,
    0x01,
    0x00,
    0x01,
    0x09,
    0x00,
};
uint8_t cga_palette_variant_selectors[8] = {
    0x00,
    0x01,
    0x00,
    0x01,
    0x00,
    0x01,
    0x00,
    0x00,
};
uint8_t cga_background_variant_controls[8] = {
    0x10,
    0x00,
    0x00,
    0x00,
    0x10,
    0x00,
    0x00,
    0x10,
};
uint8_t tandy_scene_palettes[51] = {
    0x00,
    0x09,
    0x0f,
    0x0a,
    0x02,
    0x0b,
    0x0c,
    0x01,
    0x0a,
    0x0e,
    0x0d,
    0x03,
    0x09,
    0x06,
    0x0f,
    0x01,
    0x00,
    0x00,
    0x02,
    0x0b,
    0x09,
    0x04,
    0x01,
    0x0c,
    0x01,
    0x01,
    0x06,
    0x00,
    0x00,
    0x01,
    0x00,
    0x07,
    0x0e,
    0x00,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
    0x09,
};
uint8_t ega_scene_palettes[51] = {
    0x00,
    0x11,
    0x17,
    0x12,
    0x02,
    0x12,
    0x14,
    0x01,
    0x12,
    0x16,
    0x15,
    0x13,
    0x11,
    0x06,
    0x17,
    0x01,
    0x00,
    0x00,
    0x02,
    0x13,
    0x11,
    0x04,
    0x01,
    0x14,
    0x01,
    0x01,
    0x06,
    0x00,
    0x00,
    0x01,
    0x00,
    0x07,
    0x16,
    0x00,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
    0x11,
};
uint8_t vga_attribute_identity[17] = {
    0x00,
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0a,
    0x0b,
    0x0c,
    0x0d,
    0x0e,
    0x0f,
    0x00,
};
struct RgbPalette vga_scene_rgb_palettes[3] = {
    {.colors =
            {
                {0x00, 0x00, 0x00},
                {0x27, 0x27, 0x3f},
                {0x3f, 0x3f, 0x3f},
                {0x00, 0x3f, 0x00},
                {0x00, 0x1f, 0x00},
                {0x17, 0x37, 0x17},
                {0x3f, 0x00, 0x00},
                {0x00, 0x0f, 0x27},
                {0x00, 0x3f, 0x00},
                {0x3f, 0x3f, 0x00},
                {0x27, 0x00, 0x27},
                {0x17, 0x2f, 0x2f},
                {0x27, 0x27, 0x3f},
                {0x1f, 0x0f, 0x00},
                {0x3f, 0x3f, 0x3f},
                {0x00, 0x00, 0x17},
            }},
    {.colors =
            {
                {0x00, 0x00, 0x00},
                {0x00, 0x27, 0x17},
                {0x17, 0x3f, 0x3f},
                {0x1f, 0x1f, 0x37},
                {0x27, 0x0f, 0x0f},
                {0x0f, 0x0f, 0x27},
                {0x37, 0x1f, 0x1f},
                {0x00, 0x00, 0x1f},
                {0x0f, 0x0f, 0x27},
                {0x27, 0x27, 0x0f},
                {0x00, 0x00, 0x17},
                {0x00, 0x00, 0x0f},
                {0x0f, 0x0f, 0x37},
                {0x00, 0x00, 0x0f},
                {0x2f, 0x2f, 0x2f},
                {0x37, 0x37, 0x17},
            }},
    {.colors =
            {
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
                {0x00, 0x00, 0x3f},
            }},
};
struct RgbPalette vga_work_palette = {0};
uint8_t tandy_landscape_color_3_variants[8] = {
    0x0a,
    0x0e,
    0x0b,
    0x0e,
    0x0f,
    0x0d,
    0x0f,
    0x0e,
};
uint8_t tandy_landscape_color_2_variants[8] = {
    0x0f,
    0x0b,
    0x0d,
    0x0f,
    0x0b,
    0x0e,
    0x0d,
    0x0c,
};
uint8_t tandy_landscape_color_1_variants[8] = {
    0x03,
    0x05,
    0x03,
    0x06,
    0x09,
    0x0c,
    0x05,
    0x09,
};
uint8_t ega_landscape_color_3_variants[8] = {
    0x12,
    0x16,
    0x13,
    0x16,
    0x17,
    0x15,
    0x17,
    0x16,
};
uint8_t ega_landscape_color_2_variants[8] = {
    0x17,
    0x13,
    0x15,
    0x17,
    0x13,
    0x16,
    0x15,
    0x14,
};
uint8_t ega_landscape_color_1_variants[8] = {
    0x03,
    0x05,
    0x03,
    0x06,
    0x11,
    0x14,
    0x05,
    0x11,
};
struct RgbColor vga_landscape_color_3_variants[8] = {
    {0x00, 0x3f, 0x00},
    {0x3f, 0x3f, 0x27},
    {0x27, 0x3f, 0x3f},
    {0x3f, 0x3f, 0x27},
    {0x3f, 0x3f, 0x3f},
    {0x3f, 0x2f, 0x2f},
    {0x3f, 0x3f, 0x3f},
    {0x3f, 0x3f, 0x00},
};
struct RgbColor vga_landscape_color_2_variants[8] = {
    {0x3f, 0x3f, 0x3f},
    {0x27, 0x3f, 0x3f},
    {0x3f, 0x2f, 0x2f},
    {0x3f, 0x3f, 0x3f},
    {0x27, 0x3f, 0x3f},
    {0x3f, 0x3f, 0x27},
    {0x3f, 0x27, 0x3f},
    {0x3f, 0x00, 0x00},
};
struct RgbColor vga_landscape_color_1_variants[8] = {
    {0x00, 0x27, 0x27},
    {0x27, 0x00, 0x27},
    {0x00, 0x27, 0x27},
    {0x37, 0x17, 0x00},
    {0x17, 0x17, 0x3f},
    {0x3f, 0x00, 0x00},
    {0x27, 0x00, 0x27},
    {0x17, 0x17, 0x3f},
};

/* Keep byte wrapping before the signed clamp, even for non-DAC-range data. */
static inline uint8_t fade_rgb_channel(uint8_t channel, uint8_t decrement)
{
    uint8_t value = channel - decrement;
    return (int8_t)value < 0 ? 0 : value;
}

uint16_t __far darken_palette(struct PaletteFade *observation)
{
    uint8_t quotient = 0;
    uint8_t value = 0;
    unsigned maximum = 0;
    for (unsigned color = 0; color < RGB_PALETTE_COLORS; ++color) {
        struct RgbColor *rgb = &vga_work_palette.colors[color];
        maximum = rgb->red > rgb->green ? rgb->red : rgb->green;
        if (rgb->blue > maximum)
            maximum = rgb->blue;
        quotient = maximum / 13;
        rgb->red = fade_rgb_channel(rgb->red, quotient);
        rgb->green = fade_rgb_channel(rgb->green, quotient);
        value = fade_rgb_channel(rgb->blue, quotient);
        rgb->blue = value;
    }
    /* AX remains a conservatively retained palette value at the scene boundary. */
    uint8_t remainder = maximum % 13;
    if (observation)
        *observation = (struct PaletteFade){quotient, remainder, value, 0};
    return ((uint16_t)remainder << 8) | value;
}

struct PaletteFade __far c_darken_palette(void)
{
    struct PaletteFade end;
    darken_palette(&end);
    return end;
}

/* Preserve full-width phase addressing and the caller's existing low offset.
 * The foreign segment is selected before interpreting the bytes as a palette. */
static RgbPaletteSource scene_rgb_palette(SegmentBytes output, uint16_t phase)
{
    return (RgbPaletteSource)(output + (uint16_t)vga_scene_rgb_palettes +
        (uint16_t)(phase * sizeof(struct RgbPalette)));
}

struct PaletteUploadEnd __far upload_palette(
    RgbPaletteSource colors, uint16_t start, uint16_t initial_flags)
{
    struct PlatformReply reply = platform_video(0x1012,
        start,
        RGB_PALETTE_COLORS,
        (uint16_t)(uint32_t)colors,
        0,
        (uint32_t)colors >> 16,
        initial_flags);
    struct PaletteUploadEnd end = {reply.value, reply.flags};
    return end;
}

uint16_t __far finish_palette(uint16_t flags)
{
    return status(flags, 0, 16);
}

static struct PaletteValue palette_cga(
    uint16_t data_segment, uint16_t phase, uint16_t initial_flags, struct PaletteEnd *observation)
{
    if (observation)
        *observation = (struct PaletteEnd){0};
    uint16_t offset = phase * 2;
    uint16_t src = 0, dst = 0;
    cga_palette_phase = phase;
    struct PlatformReply first = platform_video(
        0xb00 | (phase & 255), cga_scene_palettes[offset], 0, 0, 0, 0, initial_flags);
    struct PlatformReply second = platform_video(0xb00 | (first.value & 255),
        (offset & 0xff00) | 0x100 | cga_scene_palettes[offset + 1],
        0,
        0,
        0,
        0,
        first.flags);
    if ((uint8_t)phase == 2) {
        SegmentBytes screen = segment_bytes(0xb800);
        for (unsigned i = 0; i < 8192; ++i) {
            segment_word_store(screen, dst, 0);
            dst += second.flags & 0x400 ? -2 : 2;
        }
        video_target_base = video_work_buffer_segment;
    } else {
        front_buffer(PIXEL_CGA);
        SegmentBytes from = segment_bytes(video_work_buffer_segment);
        SegmentBytes to = segment_bytes(0xb800);
        SegmentBytes input = from + src;
        SegmentBytes output = to + dst;
        copy_video_words(&output, &input, 8192, !!(second.flags & 0x400));
        if (observation) {
            observation->source = video_buffer_offset(input, from);
            observation->changed |= PALETTE_SOURCE;
        }
        if (observation)
            dst = video_buffer_offset(output, to);
        video_target_base = 0xb800;
    }
    struct PaletteValue result = {data_segment, finish_palette(second.flags)};
    if (observation) {
        observation->value = result.value;
        observation->count = 0;
        observation->destination = dst;
        observation->flags = result.flags;
        observation->changed |=
            PALETTE_VALUE | PALETTE_COUNT | PALETTE_DESTINATION | PALETTE_DATA_OUTPUT;
    }
    return result;
}

static struct PaletteValue palette_attribute(
    SegmentBytes colors, uint16_t phase, uint16_t initial_flags, struct PaletteEnd *observation)
{
    uint16_t offset = (uint16_t)(uint32_t)colors + phase * 17;
    struct PlatformReply reply =
        platform_video(0x1002, phase * 17, 4, offset, 0, (uint32_t)colors >> 16, initial_flags);
    struct PaletteValue result = {reply.value, finish_palette(reply.flags)};
    if (observation) {
        *observation = (struct PaletteEnd){0};
        observation->value = result.value;
        observation->flags = result.flags;
        observation->changed = PALETTE_VALUE | PALETTE_ATTRIBUTE_COUNT;
    }
    return result;
}

static struct PaletteValue palette_vga_ega(
    SegmentBytes output, uint16_t phase, uint16_t initial_flags, struct PaletteEnd *observation)
{
    struct PaletteUploadEnd uploaded =
        upload_palette(scene_rgb_palette(output, phase), 0, initial_flags);
    struct PlatformReply reply = platform_video(
        0x1002, 0, 16, (uint16_t)vga_attribute_identity, 0, (uint32_t)output >> 16, uploaded.flags);
    struct PaletteValue result = {reply.value, finish_palette(reply.flags)};
    if (observation) {
        *observation = (struct PaletteEnd){0};
        observation->value = result.value;
        observation->flags = result.flags;
        observation->count = 16;
        observation->changed = PALETTE_VALUE | PALETTE_COUNT;
    }
    return result;
}

static struct PaletteValue palette_intensity_banks(
    SegmentBytes output, unsigned start, uint16_t initial_flags, struct PaletteEnd *observation)
{
    struct PaletteValue result = {0, initial_flags};
    if (observation)
        *observation = (struct PaletteEnd){0};
    for (unsigned bank = start; bank < 256; bank += 16) {
        struct PaletteUploadEnd uploaded = upload_palette(
            (RgbPaletteSource)(output + (uint16_t)&vga_work_palette), bank, result.flags);
        result.value = darken_palette(0);
        result.flags = subtract16(uploaded.flags, 16, 16);
        if (observation) {
            observation->count = 0;
            observation->source = observation->destination =
                (uint16_t)&vga_work_palette + sizeof(struct RgbPalette);
            observation->changed =
                PALETTE_VALUE | PALETTE_COUNT | PALETTE_SOURCE | PALETTE_DESTINATION;
        }
    }
    result.flags = finish_palette(result.flags);
    if (observation) {
        observation->value = result.value;
        observation->flags = result.flags;
    }
    return result;
}

static struct PaletteValue palette_intensities(
    SegmentBytes data, SegmentBytes output, uint16_t flags, struct PaletteEnd *observation)
{
    c_memory_copy(output + ((uint16_t)&vga_work_palette),
        data + ((uint16_t)vga_scene_rgb_palettes),
        sizeof(struct RgbPalette),
        !!(flags & 0x400));
    return palette_intensity_banks(output, 0, flags, observation);
}

static struct PaletteValue palette_fixed_banks(
    RgbPaletteSource colors, unsigned start, uint16_t initial_flags, struct PaletteEnd *observation)
{
    struct PaletteValue result = {0, initial_flags};
    if (observation)
        *observation = (struct PaletteEnd){0};
    for (unsigned bank = start; bank < 256; bank += 16) {
        struct PaletteUploadEnd uploaded = upload_palette(colors, bank, result.flags);
        result.value = uploaded.value;
        result.flags = uploaded.flags;
        if (observation) {
            observation->count = 16;
            observation->changed = PALETTE_VALUE | PALETTE_COUNT;
        }
    }
    result.flags = finish_palette(result.flags);
    if (observation) {
        observation->value = result.value;
        observation->flags = result.flags;
    }
    return result;
}

static struct PaletteValue palette_vga(SegmentBytes data,
    SegmentBytes output,
    uint16_t phase,
    uint16_t flags,
    struct PaletteEnd *observation)
{
    if (!(uint8_t)phase)
        return palette_intensities(data, output, flags, observation);
    return palette_fixed_banks(scene_rgb_palette(output, phase), 0, flags, observation);
}

struct PaletteEnd __far c_palette_cga(uint16_t segment, uint16_t phase, uint16_t flags)
{
    struct PaletteEnd end;
    palette_cga(segment, phase, flags, &end);
    return end;
}

struct PaletteEnd __far attribute_palette(SegmentBytes colors, uint16_t phase, uint16_t flags)
{
    struct PaletteEnd end;
    palette_attribute(colors, phase, flags, &end);
    return end;
}

struct PaletteEnd __far c_palette_tandy(SegmentBytes output, uint16_t phase, uint16_t flags)
{
    return attribute_palette(output + (uint16_t)tandy_scene_palettes, phase, flags);
}

struct PaletteEnd __far c_palette_ega(SegmentBytes output, uint16_t phase, uint16_t flags)
{
    return attribute_palette(output + (uint16_t)ega_scene_palettes, phase, flags);
}

struct PaletteEnd __far c_palette_vga_ega(SegmentBytes output, uint16_t phase, uint16_t flags)
{
    struct PaletteEnd end;
    palette_vga_ega(output, phase, flags, &end);
    return end;
}

struct PaletteEnd __far intensity_banks(SegmentBytes output, unsigned start, uint16_t flags)
{
    struct PaletteEnd end;
    palette_intensity_banks(output, start, flags, &end);
    return end;
}

struct PaletteEnd __far c_palette_intensities(
    SegmentBytes data, SegmentBytes output, uint16_t flags)
{
    struct PaletteEnd end;
    palette_intensities(data, output, flags, &end);
    return end;
}

struct PaletteEnd __far c_palette_intensity_continue(
    SegmentBytes output, unsigned start, uint16_t flags)
{
    return intensity_banks(output, start, flags);
}

struct PaletteEnd __far fixed_banks(RgbPaletteSource colors, unsigned start, uint16_t flags)
{
    struct PaletteEnd end;
    palette_fixed_banks(colors, start, flags, &end);
    return end;
}

struct PaletteEnd __far c_palette_fixed_continue(
    RgbPaletteSource colors, unsigned start, uint16_t flags)
{
    return fixed_banks(colors, start, flags);
}

struct PaletteEnd __far c_palette_vga(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags)
{
    struct PaletteEnd end;
    palette_vga(data, output, phase, flags, &end);
    return end;
}

static void landscape_colors(uint8_t *palette,
    const uint8_t *color_1,
    const uint8_t *color_2,
    const uint8_t *color_3,
    unsigned channel_count,
    uint16_t variant)
{
    const uint8_t *table[3] = {color_1, color_2, color_3};
    unsigned channel;
    unsigned color;
    for (color = 3; color; --color) {
        for (channel = 0; channel < channel_count; ++channel)
            palette[color * channel_count + channel] =
                table[color - 1][variant * channel_count + channel];
    }
}

static void apply_colors_cga(uint16_t variant, uint16_t *observation)
{
    cga_scene_palettes[1] = cga_palette_variant_selectors[variant];
    uint8_t color = cga_background_variant_controls[variant];
    cga_scene_palettes[0] = color;
    if (observation)
        *observation = color;
}

uint16_t __far c_colors_cga(uint16_t variant)
{
    uint16_t value;
    apply_colors_cga(variant, &value);
    return value;
}

static void apply_colors_tandy(uint16_t variant, uint16_t *observation)
{
    landscape_colors(tandy_scene_palettes,
        tandy_landscape_color_1_variants,
        tandy_landscape_color_2_variants,
        tandy_landscape_color_3_variants,
        1,
        variant);
    if (observation)
        *observation = tandy_landscape_color_1_variants[variant];
}

uint16_t __far c_colors_tandy(uint16_t variant)
{
    uint16_t value;
    apply_colors_tandy(variant, &value);
    return value;
}

static void apply_colors_ega(uint16_t variant, uint16_t *observation)
{
    landscape_colors(ega_scene_palettes,
        ega_landscape_color_1_variants,
        ega_landscape_color_2_variants,
        ega_landscape_color_3_variants,
        1,
        variant);
    if (observation)
        *observation = ega_landscape_color_1_variants[variant];
}

uint16_t __far c_colors_ega(uint16_t variant)
{
    uint16_t value;
    apply_colors_ega(variant, &value);
    return value;
}

/* Variant arithmetic is an offset calculation, not a bounded array subscript:
 * unusual full-width indices may wrap into other shared bytes. */
static const struct RgbColor *rgb_variant(const struct RgbColor *table, uint16_t variant)
{
    return (const struct RgbColor *)(uint16_t)((uint16_t)table +
        (uint16_t)(variant * sizeof(struct RgbColor)));
}

static void apply_colors_vga(uint16_t variant, uint16_t *observation)
{
    const struct RgbColor *table[3] = {vga_landscape_color_1_variants,
        vga_landscape_color_2_variants,
        vga_landscape_color_3_variants};
    unsigned color;
    for (color = 3; color; --color) {
        const struct RgbColor *rgb = rgb_variant(table[color - 1], variant);
        vga_scene_rgb_palettes[0].colors[color].red = rgb->red;
        vga_scene_rgb_palettes[0].colors[color].green = rgb->green;
        vga_scene_rgb_palettes[0].colors[color].blue = rgb->blue;
    }
    if (observation) {
        const struct RgbColor *last = rgb_variant(vga_landscape_color_1_variants, variant);
        *observation = ((uint16_t)last->green << 8) | last->blue;
    }
}

uint16_t __far c_colors_vga(uint16_t variant)
{
    uint16_t value;
    apply_colors_vga(variant, &value);
    return value;
}

void __far c_apply_landscape_colors(
    uint16_t variant, enum VideoPixelLayout layout, uint16_t *observation)
{
    switch (layout) {
    case PIXEL_CGA:
        apply_colors_cga(variant, observation);
        break;
    case PIXEL_TANDY:
        apply_colors_tandy(variant, observation);
        break;
    case PIXEL_EGA:
        apply_colors_ega(variant, observation);
        break;
    case PIXEL_VGA:
        apply_colors_vga(variant, observation);
        break;
    }
}

struct PaletteEffect __far apply_scene_palette(SegmentBytes data,
    SegmentBytes output,
    uint16_t phase,
    uint16_t flags,
    struct PaletteEnd *observation)
{
    PaletteCallback binding = segment_record(data, graphics_session).palette;
    struct PaletteEffect effect = {phase, flags, binding != 0, 0, 1};
    struct PaletteValue value;
    if (!binding) {
        if (observation) {
            *observation = (struct PaletteEnd){0};
            observation->value = phase;
            observation->flags = flags;
            observation->changed = PALETTE_VALUE | PALETTE_NO_OPERATION;
        }
        return effect;
    }
    if (binding == video_palette_cga) {
        value = palette_cga((uint32_t)data >> 16, phase, flags, observation);
        effect.data_output = 1;
    } else if (binding == video_palette_tandy)
        value =
            palette_attribute(output + (uint16_t)tandy_scene_palettes, phase, flags, observation);
    else if (binding == video_palette_ega)
        value = palette_attribute(output + (uint16_t)ega_scene_palettes, phase, flags, observation);
    else if (binding == video_palette_vga_ega)
        value = palette_vga_ega(output, phase, flags, observation);
    else if (binding == video_palette_vga)
        value = palette_vga(data, output, phase, flags, observation);
    else {
        struct PaletteEnd opaque = call_video_palette(binding, data, output, phase, flags);
        if (observation)
            *observation = opaque;
        effect.value = opaque.changed & PALETTE_VALUE ? opaque.value : phase;
        effect.flags = opaque.flags;
        effect.operated = !(opaque.changed & PALETTE_NO_OPERATION);
        effect.value_valid = !!(opaque.changed & PALETTE_VALUE);
        effect.data_output = !!(opaque.changed & PALETTE_DATA_OUTPUT);
        return effect;
    }
    effect.value = value.value;
    effect.flags = value.flags;
    return effect;
}

struct PaletteEnd __far select_scene_palette(
    SegmentBytes data, SegmentBytes output, uint16_t phase, uint16_t flags)
{
    struct PaletteEnd end;
    apply_scene_palette(data, output, phase, flags, &end);
    return end;
}
