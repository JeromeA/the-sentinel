#ifndef SENTINEL_GAME_LOGIC_H
#define SENTINEL_GAME_LOGIC_H
#include <stdint.h>
#include "numeric_entry.h"

#ifndef GAME_FAR
#ifdef __ia16__
#define GAME_FAR __far
#else
#define GAME_FAR
#endif
#endif

/* Terrain samples include the boundary vertex row/column. Playable cells stop
 * one sample earlier; storage interleave and projected cache banks are separate. */
enum TerrainGeometry {
    TERRAIN_VERTEX_SIDE = 32,
    TERRAIN_LAST_VERTEX = TERRAIN_VERTEX_SIDE - 1,
    TERRAIN_COORDINATE_MASK = TERRAIN_VERTEX_SIDE - 1,
    TERRAIN_CELL_SIDE = TERRAIN_VERTEX_SIDE - 1,
    TERRAIN_LAST_CELL = TERRAIN_CELL_SIDE - 1,
    TERRAIN_STORAGE_BANKS = 4,
    TERRAIN_STORAGE_BANK_MASK = TERRAIN_STORAGE_BANKS - 1,
    TERRAIN_GRID_BYTES = TERRAIN_VERTEX_SIDE * TERRAIN_VERTEX_SIDE,
    TERRAIN_PROJECTED_ROW_BANKS = 2,
    TERRAIN_PROJECTED_ROW_SIZE = TERRAIN_VERTEX_SIDE,
    TERRAIN_PROJECTED_CACHE_SIZE = TERRAIN_PROJECTED_ROW_BANKS * TERRAIN_PROJECTED_ROW_SIZE,
    TERRAIN_PROJECTED_CACHE_MASK = TERRAIN_PROJECTED_CACHE_SIZE - 1
};

enum GameObjectModel {
    MODEL_ROBOT,
    MODEL_SENTRY,
    MODEL_TREE,
    MODEL_BOULDER,
    MODEL_MEANIE,
    MODEL_SENTINEL,
    MODEL_SENTINEL_PEDESTAL,
    MODEL_TEXT_BLOCK_01,
    MODEL_TEXT_BLOCK_10,
    MODEL_TEXT_BLOCK_11
};

enum GameObjectLink {
    OBJECT_SLOT_MASK = 0x3f,
    OBJECT_LINK_SUPPORT = 0x40,
    OBJECT_LINK_UNUSED = 0x80
};

enum GameSightBits {
    SIGHT_TARGET_VISIBLE = 0x40,
    SIGHT_SQUARE_VISIBLE = 0x80
};

/* A binding byte stores an action above its two-bit destination slot.
 * Pause commands share slot 3, not the object-action vocabulary. */
enum GameInputProtocol {
    INPUT_SLOT_HORIZONTAL = 0,
    INPUT_SLOT_OBJECT = 1,
    INPUT_SLOT_VERTICAL = 2,
    INPUT_SLOT_PAUSE = 3,
    INPUT_SLOT_COUNT = 4,
    INPUT_BINDING_SLOT_MASK = 3,
    INPUT_BINDING_ACTION_SHIFT = 2,
    INPUT_ACTION_NONE = 0x80,

    INPUT_DIRECTION_RIGHT = 0,
    INPUT_DIRECTION_LEFT = 1,
    INPUT_DIRECTION_UP = 2,
    INPUT_DIRECTION_DOWN = 3,
    INPUT_DIRECTION_FIRST_VERTICAL = 2,
    INPUT_DIRECTION_COUNT = 4,

    OBJECT_ACTION_ABSORB = 0x20,
    OBJECT_ACTION_TRANSFER = 0x21,
    OBJECT_ACTION_HYPERSPACE = 0x22,
    OBJECT_ACTION_TURN = 0x23,
    OBJECT_ACTION_OVERVIEW = 0x24,
    OBJECT_ACTION_EXISTING_BIT = 0x20,
    OBJECT_ACTION_TRANSFER_BIT = 1,

    PAUSE_COMMAND_PAUSE = 2,
    PAUSE_COMMAND_RESUME = 3
};

#define INPUT_BINDING(action, slot) (((action) << INPUT_BINDING_ACTION_SHIFT) | (slot))

enum GameMelody {
    MELODY_LANDSCAPE_COMPLETION = 0x42
};

/* The game's 40-bit feedback generator, with its original packed state order. */
struct __attribute__((packed)) GameRandom {
    uint16_t low;
    uint8_t middle;
    uint16_t high;
};

struct GameRandomStep {
    uint8_t value;
    uint16_t upper_input;
    uint8_t upper_carry;
};
struct GameRandomStep GAME_FAR game_random_next(volatile struct GameRandom *state);
uint8_t game_random_decimal_pair(volatile struct GameRandom *state, uint8_t *low_digit);
uint8_t game_random_small_sum(volatile struct GameRandom *state, uint8_t *sample);
uint8_t game_random_grid_coordinate(volatile struct GameRandom *state);
uint8_t game_random_sentry_count(
    volatile struct GameRandom *state, uint8_t landscape_high_pair, uint8_t *offset);
uint8_t game_landscape_difficulty(union LandscapeNumber landscape);
uint8_t game_energy_to_bcd(uint8_t energy);
int game_find_free_object(const uint8_t links[64]);
int game_adjust_energy(uint8_t *energy, uint8_t cost, int spend);
#endif
