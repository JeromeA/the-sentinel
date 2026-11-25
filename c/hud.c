
/* Energy is shown as object costs: golden robot=15, robot=3, boulder=2,
 * tree=1. The last eleven columns hold the warning-bar frame and padding. */
enum EnergyHudTile {
    STATUS_TILE_BLANK,
    STATUS_TILE_ROBOT,
    STATUS_TILE_TREE_LEFT,
    STATUS_TILE_TREE_RIGHT,
    STATUS_TILE_BOULDER_LEFT,
    STATUS_TILE_BOULDER_RIGHT,
    STATUS_TILE_GOLDEN_ROBOT,
    STATUS_TILE_FRAME_LEFT,
    STATUS_TILE_FRAME_MIDDLE,
    STATUS_TILE_FRAME_RIGHT
};

enum {
    ENERGY_TREE = 1,
    ENERGY_BOULDER = 2,
    ENERGY_ROBOT = 3,
    HUD_GOLDEN_ROBOT_ENERGY = 15
};

static void present_hud(int backwards, struct HudPublishEnd *capture);

static int hud_tile(uint8_t tile, SegmentBytes data, int backwards, struct HudTileEnd *capture)
{
    TileCallback binding = graphics_draw_status_tile;
    uint16_t number = (int16_t)(int8_t)tile;
    int ega = binding == video_tile_ega;
    if (capture) {
        capture->mode = binding == video_tile_cga ? 0
            : binding == video_tile_tandy         ? 1
            : ega                                 ? 2
                                                  : 3;
        capture->number = number;
    }
    call_video_tile(binding ? binding : video_tile_vga,
        number,
        hud_tile_column,
        data,
        backwards,
        capture ? &capture->tile : 0);
    uint8_t column = hud_tile_column;
    if (capture)
        capture->column = column;
    ++hud_tile_column;
    return ega;
}

struct HudTileEnd __far c_hud_append(uint8_t tile, SegmentBytes data, int backwards)
{
    struct HudTileEnd end;
    hud_tile(tile, data, backwards, &end);
    return end;
}

static void hud_finish(const struct HudTileEnd *last, int backwards, struct HudDrawEnd *capture)
{
    if (!capture) {
        present_hud(backwards, 0);
        return;
    }
    struct HudPublishEnd copy;
    present_hud(backwards, &copy);
    capture->source = copy.copied ? copy.source_end : last->tile.source_offset;
    capture->destination = copy.copied ? copy.destination_end : last->tile.destination_offset;
    capture->cga = copy.cga;
    capture->ega = last->mode == 2;
}

static int hud_energy(SegmentBytes data, int backwards, struct HudDrawEnd *capture)
{
    hud_tile_column = 0;
    struct HudTileEnd last;
    int ega = hud_tile(STATUS_TILE_BLANK, data, backwards, capture ? &last : 0);
    hud_energy_remaining = player_energy;
    while (hud_energy_remaining >= HUD_GOLDEN_ROBOT_ENERGY) {
        hud_energy_remaining -= HUD_GOLDEN_ROBOT_ENERGY;
        ega = hud_tile(STATUS_TILE_GOLDEN_ROBOT, data, backwards, capture ? &last : 0);
        ega = hud_tile(STATUS_TILE_BLANK, data, backwards, capture ? &last : 0);
    }
    while (hud_energy_remaining >= ENERGY_ROBOT) {
        hud_energy_remaining -= ENERGY_ROBOT;
        ega = hud_tile(STATUS_TILE_ROBOT, data, backwards, capture ? &last : 0);
        ega = hud_tile(STATUS_TILE_BLANK, data, backwards, capture ? &last : 0);
    }
    if (hud_energy_remaining) {
        uint8_t remainder_tile = hud_energy_remaining * 2;
        ega = hud_tile(remainder_tile, data, backwards, capture ? &last : 0);
        ++remainder_tile;
        ega = hud_tile(remainder_tile, data, backwards, capture ? &last : 0);
    }
    do {
        ega = hud_tile(STATUS_TILE_BLANK, data, backwards, capture ? &last : 0);
    } while (hud_tile_column < 29);
    ega = hud_tile(STATUS_TILE_FRAME_LEFT, data, backwards, capture ? &last : 0);
    do {
        ega = hud_tile(STATUS_TILE_FRAME_MIDDLE, data, backwards, capture ? &last : 0);
    } while (hud_tile_column < 38);
    ega = hud_tile(STATUS_TILE_FRAME_RIGHT, data, backwards, capture ? &last : 0);
    ega = hud_tile(STATUS_TILE_BLANK, data, backwards, capture ? &last : 0);
    hud_finish(capture ? &last : 0, backwards, capture);
    return ega;
}

static int hud_clear(SegmentBytes data, int backwards, struct HudDrawEnd *capture)
{
    hud_tile_column = 0;
    struct HudTileEnd last;
    int ega;
    do {
        ega = hud_tile(STATUS_TILE_BLANK, data, backwards, capture ? &last : 0);
    } while (hud_tile_column < 40);
    hud_finish(capture ? &last : 0, backwards, capture);
    return ega;
}

static void present_hud(int backwards, struct HudPublishEnd *capture)
{
    if (capture)
        *capture = (struct HudPublishEnd){0};
    uint8_t adapter = (uint8_t)graphics_adapter;
    uint16_t source = 0;
    uint16_t destination = video_page_offset;
    SegmentBytes from = segment_bytes(video_work_buffer_segment);
    SegmentBytes to = segment_bytes(adapter == 2 ? 0xb800 : 0xa000);
    if (adapter == 4) {
        port_out16(0x3ce, 0x105);
        from = segment_bytes(0xa000);
        source = video_page_offset + 0x8000;
        SegmentBytes input = from + source;
        SegmentBytes output = to + destination;
        copy_video_bytes(&output, &input, 320, backwards);
        if (capture) {
            capture->copied = 1;
            capture->source_end = video_buffer_offset(input, from);
            capture->destination_end = video_buffer_offset(output, to);
        }
    } else if (adapter == 5) {
        video_copy_words_retrace(to, &destination, from, &source, 1280, (backwards ? 0x400 : 0));
        if (capture) {
            capture->copied = 1;
            capture->source_end = source;
            capture->destination_end = destination;
        }
    } else if (adapter == 2 && cga_palette_phase != 2) {
        struct GameStripCopy result =
            game_copy_banked_strip(from, to, source, destination, 2, backwards);
        source = result.source_end & 0x1fff;
        destination = result.destination_end;
        result = game_copy_banked_strip(from, to, source, destination, 2, backwards);
        if (capture) {
            capture->copied = capture->cga = 1;
            capture->source_end = result.source_end;
            capture->destination_end = result.destination_end;
        }
    }
}

int __far draw_hud(SegmentBytes data, int clear, int backwards)
{
    return clear ? hud_clear(data, backwards, 0) : hud_energy(data, backwards, 0);
}

struct HudDrawEnd __far c_hud_energy(SegmentBytes data, int backwards)
{
    struct HudDrawEnd end;
    hud_energy(data, backwards, &end);
    return end;
}

struct HudDrawEnd __far c_hud_clear(SegmentBytes data, int backwards)
{
    struct HudDrawEnd end;
    hud_clear(data, backwards, &end);
    return end;
}

struct HudPublishEnd __far c_present_hud(int backwards)
{
    struct HudPublishEnd end;
    present_hud(backwards, &end);
    return end;
}
