/* File refills have their own bounded buffer, independent of game state. */
static uint8_t resource_input_buffer[4096];

/* Streaming four-plane image decoding. Platform-specific I/O is restricted to
 * the refill callback; all pixel conversion and buffering below is C. */

static uint16_t resource_word(struct ResourceReader *reader)
{
    if (!reader->remaining) {
        SegmentBytes buffer = segment_at_offset(reader->data, (uint16_t)resource_input_buffer);
        struct DosTransferEnd read =
            c_dos_read(reader->handle, buffer, sizeof(resource_input_buffer), reader->flags);
        reader->remaining = read.bytes;
        /* Refill writes at the absolute buffer offset; consumption retains
         * the caller's low pointer base, just as its earlier data+cursor did. */
        reader->cursor = reader->data + (uint16_t)resource_input_buffer;
        reader->flags = read.flags;
    } else {
        /* Decode loops only observe the OR DX,DX carry here. Reconstruct
         * its other arithmetic flags only for the public word-reader entry. */
        reader->flags &= ~CF;
    }
    uint16_t value = ((volatile struct UnalignedWord GAME_FAR *)reader->cursor)->value;
    reader->cursor += reader->flags & 0x400 ? -2 : 2;
    reader->remaining -= 2;
    return value;
}

uint16_t __far c_resource_word(struct ResourceReader *reader)
{
    if (reader->remaining)
        reader->flags = status(reader->flags, reader->remaining, 16);
    uint16_t word = resource_word(reader);
    reader->flags =
        (subtract16(reader->flags, reader->remaining + 1, 1) & ~CF) | (reader->flags & CF);
    return word;
}

/* Spread four bits into the low bit of four pixel nibbles. Gathering four
 * pixels at once avoids per-pixel variable shifts on the 8086. */
static const uint16_t planar_nibble_bits[16] = {0x0000,
    0x0001,
    0x0010,
    0x0011,
    0x0100,
    0x0101,
    0x0110,
    0x0111,
    0x1000,
    0x1001,
    0x1010,
    0x1011,
    0x1100,
    0x1101,
    0x1110,
    0x1111};

static uint16_t four_planar_pixels(uint16_t plane[4])
{
    uint16_t pixels = planar_nibble_bits[plane[0] >> 12] |
        (planar_nibble_bits[plane[1] >> 12] << 1) | (planar_nibble_bits[plane[2] >> 12] << 2) |
        (planar_nibble_bits[plane[3] >> 12] << 3);
    unsigned p;
    for (p = 0; p < 4; ++p)
        plane[p] <<= 4;
    return pixels;
}

uint16_t __far decode_resource(uint16_t file_handle,
    SegmentBytes destination,
    SegmentBytes data,
    uint16_t io_flags,
    unsigned format,
    struct ResourceDecodeEnd *observation)
{
    struct ResourceReader reader = {file_handle, 0, data, io_flags, data};
    SegmentBytes output = destination;
    uint16_t last = 0;
    uint16_t last_color = 0;
    uint16_t previous_output_bit = 0;
    uint16_t group;
    for (group = 0; group != 4000; ++group) {
        uint16_t plane[4];
        unsigned p;
        for (p = 0; p != 4; ++p) {
            uint16_t word = resource_word(&reader);
            plane[p] = (uint16_t)((word << 8) | (word >> 8));
            if (format == RESOURCE_EGA) {
                SegmentBytes planar_word = destination + (uint16_t)(group * 2 + p * 8000u);
                ((volatile struct UnalignedWord GAME_FAR *)planar_word)->value = word;
                if (observation)
                    last = word;
            }
        }
        if (format == RESOURCE_EGA)
            continue;
        unsigned pixel;
        for (pixel = 0; pixel != 16;) {
            uint16_t packed = 0;
            if (format == RESOURCE_PACKED || observation)
                previous_output_bit = last & 1;
            if (format == RESOURCE_CGA) {
                unsigned parity = (200 - group / 20) & 1;
                uint16_t colors = 0;
                uint16_t mask = 0xc000;
                unsigned n;
                for (n = 0; n < 8; ++n, ++pixel) {
                    if (!(n & 3))
                        colors = four_planar_pixels(plane);
                    uint8_t color = colors >> 12;
                    colors <<= 4;
                    if (observation)
                        last_color = color * 4 + parity * 2;
                    packed |= resource_cga_color_patterns[color * 2 + parity].value & mask;
                    mask >>= 2;
                }
            } else {
                packed = four_planar_pixels(plane);
                pixel += 4;
            }
            last = (uint16_t)((packed << 8) | (packed >> 8));
            ((volatile struct UnalignedWord GAME_FAR *)output)->value = last;
            output += io_flags & 0x400 ? -2 : 2;
        }
    }
    {
        /* The packed rotate leaves the preceding output bit in CF. After the
         * byte swap, bit 7 holds its final sign bit for the OF comparison. */
        uint16_t carry = previous_output_bit;
        if (format == RESOURCE_PACKED)
            carry |= ((((last >> 7) & 1) ^ previous_output_bit) ? OF : 0);
        if (observation)
            *observation =
                (struct ResourceDecodeEnd){last, last_color, reader.remaining, reader.flags, carry};
        return resource_completion_flags(format, io_flags, reader.flags, carry);
    }
}

uint16_t __far resource_completion_flags(
    unsigned format, uint16_t original_flags, uint16_t read_flags, uint16_t carry)
{
    if (format == RESOURCE_CGA)
        return status(original_flags, 0, 16) | CF;
    if (format == RESOURCE_EGA)
        return (add16(original_flags, 7999, 1) & ~CF) | (read_flags & CF);
    return status(original_flags, 0, 16) | carry | AF;
}

struct ResourceDecodeEnd __far c_decode_cga(
    uint16_t handle, SegmentBytes destination, SegmentBytes data, uint16_t io_flags)
{
    struct ResourceDecodeEnd end;
    decode_resource(handle, destination, data, io_flags, RESOURCE_CGA, &end);
    return end;
}

struct ResourceDecodeEnd __far c_decode_ega(
    uint16_t handle, SegmentBytes destination, SegmentBytes data, uint16_t io_flags)
{
    struct ResourceDecodeEnd end;
    decode_resource(handle, destination, data, io_flags, RESOURCE_EGA, &end);
    return end;
}

struct ResourceDecodeEnd __far c_decode_packed(
    uint16_t handle, SegmentBytes destination, SegmentBytes data, uint16_t io_flags)
{
    struct ResourceDecodeEnd end;
    decode_resource(handle, destination, data, io_flags, RESOURCE_PACKED, &end);
    return end;
}

/* Sixteen source colors, each with even/odd CGA row patterns. */
struct UnalignedWord resource_cga_color_patterns[32] = {
    {0x0000},
    {0x0000},
    {0x5555},
    {0x5555},
    {0xffff},
    {0xffff},
    {0x5555},
    {0x5555},
    {0xaaaa},
    {0xaaaa},
    {0x5555},
    {0x5555},
    {0xaaaa},
    {0xaaaa},
    {0x8888},
    {0x2222},
    {0x7777},
    {0xdddd},
    {0xbbbb},
    {0xeeee},
    {0x0000},
    {0x0000},
    {0x0000},
    {0x0000},
    {0xffff},
    {0x0000},
    {0x0000},
    {0xffff},
    {0x5555},
    {0x5555},
    {0xffff},
    {0xffff},
};
