#ifndef SENTINEL_OWNED_STATE_H
#define SENTINEL_OWNED_STATE_H
#include "register_compat.h"
#include "work_state.h"
#include "storage.h"
#include "text_scripts.h"
/* Owned state and intentional byte/word views. Assembly aliases serve only
 * hardware/compatibility boundaries; ordinary C sees typed globals/members. */
extern uint8_t text_script_bank[188];
#define game_text_script_data ((uint8_t *)text_script_bank + 0)
#define text_color_a_parameter (((uint8_t *)text_script_bank)[1])
#define text_character_controls_a ((uint8_t *)text_script_bank + 2)
#define text_color_b_parameter (((uint8_t *)text_script_bank)[12])
extern uint8_t crt_error_registry[221];
extern struct UnalignedWord cursor_video_offset_storage[48];
#define cursor_saved_video_offsets (cursor_video_offset_storage + 0)
extern uint8_t cursor_pixel_storage[288];
#define cursor_saved_pixels ((uint8_t *)cursor_pixel_storage + 0)

enum AdapterProbeKind {
    PROBE_DISPLAY_COMBINATION = 1,
    PROBE_EGA_CONFIGURATION,
    PROBE_TANDY_COLUMNS,
    PROBE_COLOR_CRTC,
    PROBE_MONO_CRTC
};

struct __attribute__((packed)) AdapterProbeRecord {
    uint8_t enabled;
    uint16_t kind;
};

_Static_assert(sizeof(struct AdapterProbeRecord) == 3, "probe record byte format");
_Static_assert(__builtin_offsetof(struct AdapterProbeRecord, kind) == 1, "probe kind offset");

struct __attribute__((packed)) AdapterProbeList {
    struct AdapterProbeRecord records[5];
    uint8_t terminator;
};
extern struct AdapterProbeList adapter_probe_list;
#define video_probe_records ((uint8_t *)&adapter_probe_list)
#define video_probe_display_combination_enabled (adapter_probe_list.records[0].enabled)
#define video_probe_ega_configuration_enabled (adapter_probe_list.records[1].enabled)
#define video_probe_mode8_columns_enabled (adapter_probe_list.records[2].enabled)
#define video_probe_color_crtc_enabled (adapter_probe_list.records[3].enabled)
#define video_probe_mono_crtc_enabled (adapter_probe_list.records[4].enabled)
extern union NumericEntry number_entry_buffer;
extern uint8_t generation_reference_zero_run[8];
extern volatile uint8_t game_timer_suspend_flags;
extern volatile union LandscapeNumber landscape_number_bcd;
extern uint8_t terrain_visibility_bits[128];
extern uint8_t cga_text_color_pixels[16];
extern struct UnalignedWord cga_font_nibble_masks[16];
extern uint16_t video_crtc_port;
extern uint8_t text_attribute;
extern uint8_t text_cursor_row;
#define text_cursor_column (text_cursor_column_storage.value)

struct __attribute__((packed)) TextCursorColumnStorage {
    uint8_t value;
    uint8_t trailing[1];
};
extern struct TextCursorColumnStorage text_cursor_column_storage;
extern uint8_t glyph_pair_terrain_cells[4];
extern uint8_t title_secret_screen_flags;
extern volatile uint8_t bcd_random_digit_work;
extern uint8_t title_banner_markup[16];
#define text_inline_control_bytes_left (text_inline_control_bytes_left_storage.value)

struct __attribute__((packed)) TextInlineControlBytesLeftStorage {
    uint8_t value;
    uint8_t trailing[1];
};
extern struct TextInlineControlBytesLeftStorage text_inline_control_bytes_left_storage;
extern uint8_t preview_view_y_offsets[4];
extern uint8_t preview_view_pitch_values[3];
extern uint8_t preview_view_height_offsets[3];
extern uint8_t preview_view_heading_values[3];
extern uint8_t preview_view_companion_heading_values[6];
extern uint8_t terrain_generation_saved_noise;
#define text_prng_compare_limit terrain_generation_saved_noise
extern uint8_t game_text_pending_opcode;
extern uint8_t game_text_parameter_index;
extern uint16_t game_text_alternate_mode;
extern uint16_t game_text_transparent;
extern uint16_t game_text_glyph_flags;
extern uint16_t game_text_normal_foreground;
extern uint16_t game_text_normal_background;
extern uint16_t game_text_alternate_foreground;
extern uint16_t game_text_alternate_background;
extern uint16_t game_text_active_foreground;
extern uint16_t game_text_active_background;
extern uint16_t game_text_column;
extern uint16_t game_text_row;
#define game_text_input_character (game_text_input_character_storage.value)

struct __attribute__((packed)) GameTextInputCharacterStorage {
    uint8_t value;
    uint8_t trailing[1];
};
extern struct GameTextInputCharacterStorage game_text_input_character_storage;
extern uint8_t game_text_script_offsets[TEXT_SCRIPT_COUNT];
extern uint8_t terrain_quadrant_corner_bias[4];
extern uint8_t viewport_origin_high_table[4];
extern struct UnalignedWord viewport_x_min_table[3];
extern struct UnalignedWord viewport_width_table[3];
extern uint8_t viewport_x_limit_table[4];
extern uint8_t terrain_corner_distance[64];
extern uint8_t terrain_corner_distance_tail[32];
extern uint8_t terrain_rows_remaining;
extern uint8_t view_grid_x_overflow;
#define terrain_shading_mode (terrain_shading_mode_storage.value)

struct __attribute__((packed)) TerrainShadingModeStorage {
    uint8_t value;
    uint8_t trailing[1];
};
extern struct TerrainShadingModeStorage terrain_shading_mode_storage;
extern uint8_t terrain_visible_bit_masks[8];
extern uint8_t selection_view_total_columns;
extern uint8_t selection_view_origin_column;
extern uint8_t selection_saved_heading;
extern volatile uint8_t selection_copy_iteration_budget;
extern uint16_t selection_copy_origin;
extern uint8_t ray_contact_edge_selectors[8];
extern uint8_t model_selection_extents[7];
extern uint8_t object_energy_values[7];
extern uint8_t camera_transition_steps[4];
extern volatile uint8_t video_transition_countdown;
extern uint8_t view_y_end_presets[2];
extern uint8_t view_y_min_presets[3];
extern uint8_t preview_escape_restarts;
extern volatile uint8_t cursor_mode_toggle_latch;
extern union LandscapeNumber landscape_zero_entry_bcd[2];
extern uint8_t intro_once_flags;
extern uint8_t camera_transition_pitch_targets[2];
extern volatile uint8_t sentry_countdown_phase;
extern uint8_t input_key_bindings[20];
extern uint8_t input_binding_actions[20];
extern uint8_t scene_camera_height_table[2];
extern uint8_t scene_camera_pitch_table[2];
extern uint8_t scene_camera_x_table[2];
extern uint8_t scene_lateral_offset_table[2];
extern uint8_t scene_camera_y_table[2];
extern uint8_t scene_camera_heading_table[2];
extern uint8_t scene_text_color_presets_a[4];
extern uint8_t scene_text_color_presets_b[4];
extern uint8_t scene_view_preset;
extern uint8_t scene_lateral_offset;
extern uint8_t sentry_sight_saved_source_slot;
extern uint16_t sentry_scheduler_saved_sp;
extern uint8_t placement_count_masks[9];
extern uint8_t ega_switch_subtypes[6];
extern struct AdapterDescriptor bios_display_combination_descriptors[13];
extern uint16_t saved_hardware_sp;
extern uint8_t saved_int09_offset[4];
#define bios_tick_prescaler (bios_tick_prescaler_storage.value)

struct __attribute__((packed)) BiosTickPrescalerStorage {
    uint8_t value;
    uint8_t trailing[1];
};
extern struct BiosTickPrescalerStorage bios_tick_prescaler_storage;
extern uint16_t pit_calibration_max;
#define keyboard_e0_prefix (keyboard_e0_prefix_storage.value)

struct __attribute__((packed)) KeyboardE0PrefixStorage {
    volatile uint8_t value;
    uint8_t trailing[1];
};
extern struct KeyboardE0PrefixStorage keyboard_e0_prefix_storage;
extern uint8_t cga_palette_phase;
#define pixel_scatter_sequence_low (pixel_scatter_sequence_low_storage.low)
#define pixel_scatter_sequence_high (pixel_scatter_sequence_low_storage.high)

struct __attribute__((packed)) PixelScatterSequenceLowStorage {
    uint16_t low;
    uint16_t high;
};
extern struct PixelScatterSequenceLowStorage pixel_scatter_sequence_low_storage;
extern uint8_t cga_clear_pixel_masks[4];
extern uint8_t cga_set_pixel_masks[4];
extern uint8_t tandy_clear_pixel_masks[2];
extern uint8_t tandy_set_pixel_masks[2];
extern uint8_t ega_single_pixel_masks[8];
#define mouse_x (mouse_input_state_storage.x)
#define mouse_y (mouse_input_state_storage.y)
#define input_buttons (mouse_input_state_storage.buttons)
#define mouse_moved (mouse_input_state_storage.moved)
#define mouse_installed (mouse_input_state_storage.installed)

struct __attribute__((packed)) MouseInputStateStorage {
    volatile uint16_t x;
    volatile uint16_t y;
    volatile uint8_t buttons;
    volatile uint8_t moved;
    volatile uint8_t installed;
};
extern struct MouseInputStateStorage mouse_input_state_storage;
extern uint8_t filename_sent[10];
extern uint8_t cursor_step_ink_records[42];
extern uint8_t cursor_step_x_deltas[22];
extern uint8_t cursor_step_scanline_deltas[22];
extern uint8_t cursor_cga_clear_pair_masks[2];
extern uint8_t cursor_cga_set_pair_masks[2];
extern uint8_t cursor_ega_pair_masks[4];
extern uint8_t untraced_signed_step_vectors[17];
extern uint8_t msg_menu_navigation[42];
extern uint8_t msg_menu_select[24];
extern uint8_t name_tandy[16];
extern uint8_t name_ega[26];
extern uint8_t name_vga[22];
extern uint8_t name_cga[24];
extern uint16_t menu_normal_background;
extern uint16_t menu_selected_background;
extern uint16_t menu_normal_foreground;
extern uint16_t menu_selected_foreground;
extern struct UnalignedWord video_display_enable_wait_counts[4];
extern uint16_t video_page_offset;
extern uint16_t video_previous_page_offset;
#define video_pending_page_offset (video_pending_page_offset_storage.value)

struct __attribute__((packed)) VideoPendingPageOffsetStorage {
    uint16_t value;
    uint8_t trailing[10];
};
extern struct VideoPendingPageOffsetStorage video_pending_page_offset_storage;
extern uint16_t video_transition_wait_count;
extern struct UnalignedWord cga_transition_source_mode_0[16];
extern struct UnalignedWord cga_transition_source_mode_1[16];
extern struct UnalignedWord cga_transition_source_mode_2[8];
extern struct UnalignedWord cga_transition_source_mode_3[8];
extern struct UnalignedWord cga_transition_page_deltas[4];
extern struct UnalignedWord ega_transition_source_mode_0[16];
extern struct UnalignedWord ega_transition_source_mode_1[16];
extern struct UnalignedWord ega_transition_source_mode_2[8];
extern struct UnalignedWord ega_transition_source_mode_3[8];
extern struct UnalignedWord ega_transition_page_deltas[4];
extern struct UnalignedWord vga_transition_source_mode_0[16];
extern struct UnalignedWord vga_transition_source_mode_1[16];
extern struct UnalignedWord vga_transition_source_mode_2[8];
extern struct UnalignedWord vga_transition_source_mode_3[8];
extern struct UnalignedWord vga_transition_page_deltas[4];
#define warning_bar_noise_low (warning_bar_noise_low_storage.low)
#define warning_bar_noise_high (warning_bar_noise_low_storage.high)

struct __attribute__((packed)) WarningBarNoiseLowStorage {
    uint16_t low;
    uint16_t high;
};
extern struct WarningBarNoiseLowStorage warning_bar_noise_low_storage;
extern uint8_t cga_sentry_warning_patterns[24];
extern uint8_t tandy_sentry_warning_patterns[48];
extern uint8_t ega_sentry_warning_patterns[12];
extern uint8_t vga_sentry_warning_patterns[96];
#define copy_sequence_low (copy_sequence_low_storage.low)
#define copy_sequence_high (copy_sequence_low_storage.high)

struct __attribute__((packed)) CopySequenceLowStorage {
    uint16_t low;
    uint16_t high;
};
extern struct CopySequenceLowStorage copy_sequence_low_storage;
extern struct UnalignedWord cga_random_copy_nibble_masks[4];
extern uint8_t ega_random_copy_bit_masks[4];
extern uint16_t crt_near_limit;
extern uint16_t crt_initial_break;
extern uint16_t crt_exit_callback;
extern uint16_t crt_near_break;
extern uint16_t crt_loader_data_segment_reference[39];
extern uint16_t crt_near_break_address;
extern uint8_t crt_file_info_name[13];
#define crt_saved_int00_offset (crt_saved_int00_offset_storage.offset)
#define crt_saved_int00_segment (crt_saved_int00_offset_storage.segment)

struct __attribute__((packed)) CrtSavedInt00OffsetStorage {
    uint16_t offset;
    uint16_t segment;
    uint8_t trailing[14];
};
extern struct CrtSavedInt00OffsetStorage crt_saved_int00_offset_storage;
extern uint16_t crt_psp_segment;
#define crt_dos_version (crt_dos_version_storage.value)

struct __attribute__((packed)) CrtDosVersionStorage {
    uint16_t value;
    uint8_t trailing[3];
};
extern struct CrtDosVersionStorage crt_dos_version_storage;
extern uint8_t crt_handle_flag_capacity[2];
extern uint8_t crt_file_flags[20];
extern uint16_t crt_argc;
extern uint16_t crt_argv;
extern uint16_t crt_envp;
extern uint16_t crt_program_name_pointer[2];
extern uint8_t crt_program_name_fallback[4];
extern uint8_t crt_optional_interrupt_enabled;
extern uint8_t crt_optional_interrupt_number;
extern uint32_t crt_optional_interrupt_vector;
extern uint16_t crt_error_callback;
#define crt_argv_saved_return (crt_argv_saved_return_storage.value)

struct __attribute__((packed)) CrtArgvSavedReturnStorage {
    uint16_t value;
    uint8_t trailing[2];
};
extern struct CrtArgvSavedReturnStorage crt_argv_saved_return_storage;
extern uint32_t crt_optional_service_hook;
extern uint32_t crt_service_init_argument;
extern uint32_t crt_service_secondary_argument;
extern uint8_t crt_none[8];
__asm__(".equ game_text_script_data, text_script_bank+0\n"
        ".equ text_color_a_parameter, text_script_bank+1\n"
        ".equ text_character_controls_a, text_script_bank+2\n"
        ".equ text_color_b_parameter, text_script_bank+12\n"
        ".equ cursor_saved_video_offsets, cursor_video_offset_storage+0\n"
        ".equ cursor_saved_pixels, cursor_pixel_storage+0\n"
        ".equ video_probe_records, adapter_probe_list+0\n"
        ".equ video_probe_display_combination_enabled, adapter_probe_list+0\n"
        ".equ video_probe_ega_configuration_enabled, adapter_probe_list+3\n"
        ".equ video_probe_mode8_columns_enabled, adapter_probe_list+6\n"
        ".equ video_probe_color_crtc_enabled, adapter_probe_list+9\n"
        ".equ video_probe_mono_crtc_enabled, adapter_probe_list+12\n"
        ".equ text_cursor_column, text_cursor_column_storage+0\n"
        ".equ text_inline_control_bytes_left, text_inline_control_bytes_left_storage+0\n"
        ".equ text_prng_compare_limit, terrain_generation_saved_noise+0\n"
        ".equ game_text_input_character, game_text_input_character_storage+0\n"
        ".equ terrain_shading_mode, terrain_shading_mode_storage+0\n"
        ".equ bios_tick_prescaler, bios_tick_prescaler_storage+0\n"
        ".equ keyboard_e0_prefix, keyboard_e0_prefix_storage+0\n"
        ".equ pixel_scatter_sequence_low, pixel_scatter_sequence_low_storage+0\n"
        ".equ pixel_scatter_sequence_high, pixel_scatter_sequence_low_storage+2\n"
        ".equ mouse_x, mouse_input_state_storage+0\n"
        ".equ mouse_y, mouse_input_state_storage+2\n"
        ".equ input_buttons, mouse_input_state_storage+4\n"
        ".equ mouse_moved, mouse_input_state_storage+5\n"
        ".equ mouse_installed, mouse_input_state_storage+6\n"
        ".equ video_pending_page_offset, video_pending_page_offset_storage+0\n"
        ".equ warning_bar_noise_low, warning_bar_noise_low_storage+0\n"
        ".equ warning_bar_noise_high, warning_bar_noise_low_storage+2\n"
        ".equ copy_sequence_low, copy_sequence_low_storage+0\n"
        ".equ copy_sequence_high, copy_sequence_low_storage+2\n"
        ".equ crt_saved_int00_offset, crt_saved_int00_offset_storage+0\n"
        ".equ crt_saved_int00_segment, crt_saved_int00_offset_storage+2\n"
        ".equ crt_dos_version, crt_dos_version_storage+0\n"
        ".equ crt_argv_saved_return, crt_argv_saved_return_storage+0\n");
#endif
