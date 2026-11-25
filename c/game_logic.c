/* Game rules expressed as C. No registers, CPU dispatcher or inline assembly. */
#include "game_logic.h"

struct GameRandomStep GAME_FAR game_random_next(volatile struct GameRandom *state)
{
    uint16_t high = state->high;
    uint16_t low = state->low;
    uint8_t middle = state->middle;
    unsigned bit;

    /* Feedback taps are bits 19 and 32 of the 40-bit state. */
    struct GameRandomStep step;
    for (bit = 0; bit != 8; ++bit) {
        uint8_t feedback = ((middle >> 3) ^ (high >> 8)) & 1;
        uint8_t low_carry = low >> 15;
        step.upper_input = high;
        step.upper_carry = middle >> 7;
        low = (uint16_t)((low << 1) | feedback);
        middle = (uint8_t)((middle << 1) | low_carry);
        high = (uint16_t)((high << 1) | step.upper_carry);
    }
    state->middle = middle;
    state->low = low;
    state->high = high;
    step.value = high >> 8;
    return step;
}

uint8_t game_random_decimal_pair(volatile struct GameRandom *state, uint8_t *low_digit)
{
    struct GameRandomStep step;
    unsigned advance;
    for (advance = 0; advance != 4; ++advance)
        step = game_random_next(state);
    union BcdPair low_sample = {.raw = step.value};
    uint8_t low = low_sample.digits.ones;
    if (low >= 10)
        low -= 6;
    *low_digit = low;
    uint8_t high;
    union BcdPair high_sample = {.raw = game_random_next(state).value};
    high = high_sample.digits.tens << 4;
    if (high >= 0xa0)
        high -= 0x60;
    return high | low;
}

uint8_t game_random_small_sum(volatile struct GameRandom *state, uint8_t *sample)
{
    *sample = game_random_next(state).value;
    return (*sample & 7) + ((*sample >> 3) & 15);
}

uint8_t game_random_grid_coordinate(volatile struct GameRandom *state)
{
    uint8_t coordinate;
    do {
        coordinate = game_random_next(state).value & TERRAIN_COORDINATE_MASK;
    } while (coordinate == TERRAIN_CELL_SIDE);
    return coordinate;
}

uint8_t game_random_sentry_count(
    volatile struct GameRandom *state, uint8_t landscape_high_pair, uint8_t *offset)
{
    uint8_t base = (landscape_high_pair >> 4) + 2;
    uint8_t count;
    do {
        uint8_t sample = game_random_next(state).value;
        uint8_t bits = sample & 0x7f;
        unsigned magnitude = 0;
        if (!bits)
            magnitude = 7;
        else
            while (!(bits & 0x40)) {
                ++magnitude;
                bits <<= 1;
            }
        uint8_t displacement = sample & 0x80 ? (uint8_t)(-1 - (int)magnitude) : magnitude;
        if (offset)
            *offset = displacement;
        count = (uint8_t)(base + (offset ? *offset : displacement));
    } while (count >= 8);
    return count + 1;
}

uint8_t game_landscape_difficulty(union LandscapeNumber landscape)
{
    if (landscape.pairs.high.raw)
        return 8;
    unsigned difficulty = landscape.pairs.low.digits.tens + 1;
    return difficulty < 9 ? difficulty : 8;
}

uint8_t game_energy_to_bcd(uint8_t energy)
{
    /* Valid player energy is a six-bit count, so at most two decimal digits. */
    return (uint8_t)(((energy / 10) << 4) | (energy % 10));
}

int game_find_free_object(const uint8_t links[64])
{
    int slot;
    for (slot = 63; slot >= 0; --slot)
        if (links[slot] & OBJECT_LINK_UNUSED)
            return slot;
    return -1;
}

int game_adjust_energy(uint8_t *energy, uint8_t cost, int spend)
{
    if (spend && *energy < cost)
        return 0;
    *energy = (uint8_t)((spend ? *energy - cost : *energy + cost) & 63);
    return 1;
}

/* Owned subsystem data. */
volatile union LandscapeNumber landscape_number_bcd = {0};

/* RandomWork is reset independently of other subsystems and data placement. */
struct RandomWork random_work = {0};

void __far reset_random_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&random_work), 0, sizeof(random_work), 0);
}
