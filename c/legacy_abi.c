/* Register/flag adapters for the reconstructed game entry interfaces. Game
 * rules are C; the small assembly boundaries preserve hardware/stack contracts. */
#include "game_logic.h"

#include "register_compat.h"

/* A converted nonlocal/tail branch returns to its ABI bridge, which restores
 * the legacy registers and jumps without consuming the caller's return word. */
static void legacy_tail(struct LegacyRegisters *r, uint16_t target)
{
    ((uint16_t *)r)[-1] = target;
}

/* Nonlocal legacy continuations choose a stack and IP in C. The final stack
 * switch cannot be expressed by an ordinary C return. These paths run only in
 * foreground code, as did the original saved-SP scheduler/graphics exits. */
static volatile uint16_t legacy_resume_sp __attribute__((used)),
    legacy_resume_ip __attribute__((used));
extern void legacy_resume(void), legacy_return(void);

static void legacy_set_stack(struct LegacyRegisters *r, uint16_t sp, uint16_t ip)
{
    legacy_resume_sp = sp;
    legacy_resume_ip = ip;
    legacy_tail(r, (uint16_t)&target_legacy_resume);
}

__asm__(
    ".code16\n.section .text.legacy_resume,\"ax\"\nlegacy_resume:\nmovw legacy_resume_sp,%sp\njmp "
    "*legacy_resume_ip\n");

void legacy_return(void)
{
}

uint8_t __far c_random_decimal_pair(void)
{
    uint8_t low;
    uint8_t value = game_random_decimal_pair(&prng_state_low, &low);
    bcd_random_digit_work = low;
    return value;
}

uint8_t __far c_random_small_sum(uint8_t *sample)
{
    return game_random_small_sum(&prng_state_low, sample);
}

uint8_t __far c_random_grid_coordinate(void)
{
    return game_random_grid_coordinate(&prng_state_low);
}

uint8_t __far c_select_landscape(union LandscapeNumber landscape)
{
    uint8_t difficulty = game_landscape_difficulty(landscape);
    prng_state_low.low = landscape.raw;
    landscape_number_bcd.pairs.low.raw = landscape.pairs.low.raw;
    landscape_number_bcd.pairs.high.raw = landscape.pairs.high.raw;
    landscape_nonzero =
        landscape.pairs.high.raw ? landscape.pairs.high.raw : landscape.pairs.low.raw;
    landscape_difficulty = difficulty;
    return difficulty;
}

uint8_t __far c_random_sentry_count(uint8_t *offset)
{
    uint8_t count =
        game_random_sentry_count(&prng_state_low, landscape_number_bcd.pairs.high.raw, offset);
    union BcdPair high_pair = {.raw = landscape_number_bcd.pairs.high.raw};
    math_work_lo = high_pair.digits.tens + 2;
    return count;
}

uint8_t __far c_energy_bcd(void)
{
    return game_energy_to_bcd(player_energy);
}

void __far c_start_melody(uint8_t start)
{
    sound_melody_cursor = start;
    sound_sequence_mode = 3;
    melody_effect_control.raw = 0x81;
}

int __far c_allocate_saved(void)
{
    int slot = game_find_free_object(object_stack_links);
    if (slot >= 0) {
        allocated_object_slot = slot;
        object_model_id[slot] = pending_object_model;
    }
    return slot;
}

int __far c_allocate_model(uint8_t model)
{
    pending_object_model = model;
    return c_allocate_saved();
}

int __far c_adjust_energy(uint16_t slot, int spend, struct EnergyChange *change)
{
    *change = (struct EnergyChange){0};
    if (slot >= 64)
        return 0;
    uint8_t model = object_model_id[slot];
    if (model >= 7) {
        change->model = model;
        change->before = change->after = player_energy;
        return 0;
    }
    uint8_t cost = object_energy_values[model];
    uint8_t before = player_energy;
    uint8_t value = before;
    int accepted = game_adjust_energy(&value, cost, spend);
    if (accepted)
        player_energy = value;
    change->model = model;
    change->cost = cost;
    change->before = before;
    change->after = value;
    return accepted;
}

static struct GameStripCopy copy_banked_strip(SegmentBytes source,
    SegmentBytes destination,
    uint16_t source_start,
    uint16_t destination_start,
    unsigned banks,
    int backwards)
{
    return game_copy_banked_strip(
        source, destination, source_start, destination_start, banks, backwards);
}

struct GameStripCopy __far c_copy_cga_strip(SegmentBytes source,
    SegmentBytes destination,
    uint16_t source_start,
    uint16_t destination_start,
    int backwards)
{
    return copy_banked_strip(source, destination, source_start, destination_start, 2, backwards);
}

struct GameStripCopy __far c_copy_four_bank_strip(SegmentBytes source,
    SegmentBytes destination,
    uint16_t source_start,
    uint16_t destination_start,
    int backwards)
{
    return copy_banked_strip(source, destination, source_start, destination_start, 4, backwards);
}

/* Symbolic targets dispatch to C. Native/far calls implement hardware and stack boundaries. */
extern void legacy_invoke(struct LegacyRegisters *, uint16_t target);
extern void native_invoke(struct LegacyRegisters *, uint16_t target);
extern void legacy_far_invoke(struct LegacyRegisters *, uint32_t target);
extern void platform_int21(void);

static void port_out16(uint16_t port, uint16_t value)
{
    __asm__ volatile("outw %0,%1" : : "a"(value), "d"(port));
}

static void port_out8(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %%al,%%dx" : : "a"((uint16_t)value), "d"(port));
}

static uint8_t port_in8(uint16_t port)
{
    uint16_t value;
    __asm__ volatile("inb %%dx,%%al" : "=a"(value) : "d"(port));
    return value;
}

__asm__(
    ".intel_syntax noprefix\n.code16\n.section .text.legacy_far_invoke,\"ax\"\n"
    "legacy_far_invoke:\npush bp\nmov bp,sp\npush bx\npush si\npush di\npush ds\npush es\n"
    "push ss\npop ds\nmov di,[bp+4]\npush cs\n"
    "mov ax,OFFSET FLAT:.Lfar_legacy_return\npush ax\npush word ptr [bp+8]\npush word ptr [bp+6]\n"
    "push word ptr [di+2]\npush word ptr [di+18]\npopf\n"
    "mov es,[di]\nmov ax,[di+16]\nmov bx,[di+14]\nmov cx,[di+12]\n"
    "mov dx,[di+10]\nmov bp,[di+4]\nmov si,[di+8]\nmov di,[di+6]\npop ds\nretf\n"
    ".Lfar_legacy_return:\npushf\npush ax\npush bx\npush cx\npush dx\npush si\npush di\npush "
    "bp\npush ds\npush es\n"
    "mov bp,sp\npush ss\npop ds\npush ss\npop es\ncld\n"
    "mov di,[bp+34]\nmov si,bp\nmov cx,10\nrep movsw\nadd sp,20\n"
    "pop es\npop ds\npop di\npop si\npop bx\npop bp\nret 6\n.att_syntax prefix\n");

__asm__(".intel_syntax noprefix\n.code16\n.section .text.legacy_invoke,\"ax\"\n"
        "legacy_invoke:\n"
        "push bp\nmov bp,sp\npush bx\npush si\npush di\npush ds\npush es\n"
        "push ss\npop ds\nmov di,[bp+4]\n"
        "mov ax,OFFSET FLAT:.Llegacy_return\npush ax\npush word ptr [bp+6]\n"
        "mov ax,OFFSET FLAT:c_enter_entry\npush ax\njmp .Lload_legacy_registers\n"
        "native_invoke:\n"
        "push bp\nmov bp,sp\npush bx\npush si\npush di\npush ds\npush es\n"
        "push ss\npop ds\n"
        "mov di,[bp+4]\n"
        "mov ax,OFFSET FLAT:.Llegacy_return\npush ax\npush word ptr [bp+6]\n"
        ".Lload_legacy_registers:\n"
        "push word ptr [di+2]\npush word ptr [di+18]\npopf\n"
        "mov es,[di]\nmov ax,[di+16]\nmov bx,[di+14]\nmov cx,[di+12]\n"
        "mov dx,[di+10]\nmov bp,[di+4]\nmov si,[di+8]\nmov di,[di+6]\npop ds\nret\n"
        ".Llegacy_return:\n"
        "pushf\npush ax\npush bx\npush cx\npush dx\npush si\npush di\npush bp\npush ds\npush es\n"
        "mov bp,sp\npush ss\npop ds\npush ss\npop es\ncld\n"
        "mov di,[bp+34]\nmov si,bp\nmov cx,10\nrep movsw\nadd sp,20\n"
        "pop es\npop ds\npop di\npop si\npop bx\npop bp\nret 4\n"
        ".section .text.platform_interrupts,\"ax\"\nplatform_int21:\nint 0x21\nret\n"
        ".att_syntax prefix\n");

/* One near compatibility gate keeps far-call sites out of ordinary callers. */
void typed_legacy_entry(struct LegacyRegisters *r, uint32_t frame, uint16_t entry)
{
    publish_typed_entry(r, frame, entry);
}

/* A typed operation may request an as-yet-unconverted graphics callback. */
void __far typed_invoke_legacy(struct LegacyRegisters *r, uint16_t target)
{
    legacy_invoke(r, target);
}

uint16_t __far typed_resume_legacy(uint16_t sp, uint16_t ip)
{
    legacy_resume_sp = sp;
    legacy_resume_ip = ip;
    return (uint16_t)&target_legacy_resume;
}
