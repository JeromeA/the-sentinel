/* Register entries required by DOS/interrupt or saved-stack boundaries.
 * Ordinary game operations and video callbacks have no descriptor. */
#ifndef SENTINEL_LEGACY_TARGETS_H
#define SENTINEL_LEGACY_TARGETS_H
#include <stdint.h>

struct LegacyRegisters;
/* Boundary callbacks share one signature; the selector is used by typed entries. */
typedef void (*LegacyCFunction)(
    struct LegacyRegisters *registers, uint32_t frame, uint16_t selector);

struct LegacyTarget {
    LegacyCFunction function;
    uint16_t native_ip;
};

_Static_assert(sizeof(struct LegacyTarget) == 4, "register target descriptor layout");
extern const struct LegacyTarget target_c_configure_preview_object;
extern const struct LegacyTarget target_c_crt_divide_error;
extern const struct LegacyTarget target_c_crt_error_prefix;
extern const struct LegacyTarget target_c_crt_exit;
extern const struct LegacyTarget target_c_crt_exit_common;
extern const struct LegacyTarget target_c_crt_fatal;
extern const struct LegacyTarget target_c_crt_initialize;
extern const struct LegacyTarget target_c_crt_prepare_argv;
extern const struct LegacyTarget target_c_crt_run_game;
extern const struct LegacyTarget target_c_crt_write_message;
extern const struct LegacyTarget target_c_generate_landscape;
extern const struct LegacyTarget target_c_hardware_enter;
extern const struct LegacyTarget target_c_hardware_leave;
extern const struct LegacyTarget target_c_keyboard_bios_scan;
extern const struct LegacyTarget target_c_keyboard_irq_scan;
extern const struct LegacyTarget target_c_mouse_callback_body;
extern const struct LegacyTarget target_c_populate_preview;
extern const struct LegacyTarget target_c_preview_caption;
extern const struct LegacyTarget target_c_preview_key_gate;
extern const struct LegacyTarget target_c_sentry_schedule_finish;
extern const struct LegacyTarget target_c_timer_original_body;
extern const struct LegacyTarget target_c_timer_triple_body;
extern const struct LegacyTarget target_c_wrong_secret;
extern const struct LegacyTarget target_legacy_resume;
extern const struct LegacyTarget target_crt_argument_handoff;
extern const struct LegacyTarget target_preview_population_call;
extern const struct LegacyTarget target_generation_call;
extern const struct LegacyTarget target_keyboard_irq_return;
extern const struct LegacyTarget target_bios_keyboard_return;
extern const struct LegacyTarget target_timer_chain_return;
extern const struct LegacyTarget target_timer_eoi_return;
extern const struct LegacyTarget target_timer_original_return;
extern const struct LegacyTarget target_hardware_stack_return;
extern const struct LegacyTarget target_mouse_callback_return;
extern const struct LegacyTarget target_argv_call;
extern void abi_c_crt_exit(uint16_t status) __attribute__((noreturn));
extern void abi_c_crt_exit_common(void);
extern void abi_c_preview_key_gate(void);
extern void abi_c_sentry_schedule_finish(void);
extern void argv_call(void);
extern void bios_keyboard_intercept(void);
extern void bios_keyboard_return(void);
extern void crt_argument_handoff(void);
extern void crt_divide_error(void);
extern void crt_entry(void);
extern void generation_call(void);
extern void hardware_enter_game(void);
extern void hardware_stack_return(void);
extern void keyboard_irq_handler(void);
extern void keyboard_irq_return(void);
extern void landscape_preview_return_dispatch(void);
extern void legacy_resume(void);
extern void mouse_callback_return(void);
extern void mouse_event_callback(void);
extern void preview_population_call(void);
extern uint16_t saved_int08_vector[2], saved_int15_vector[2];
extern void timer_chain_return(void);
extern void timer_eoi_return(void);
extern void timer_irq_original_rate(void);
extern void timer_irq_triple_rate(void);
extern void timer_original_return(void);
#endif

extern const struct LegacyTarget target_generate_landscape_flow;
extern const struct LegacyTarget target_populate_preview_flow;

extern const struct LegacyTarget target_wrong_secret_flow;
extern const struct LegacyTarget target_preview_key_flow;
extern const struct LegacyTarget target_preview_caption_flow;
