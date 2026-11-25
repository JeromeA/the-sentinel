; Sentinel: complete expanded DOS program and EXEPACK decompressor source.
; Build the packed executable: make
; Expanded EXE: nasm -f bin sentinel.asm -o build/Sentinel.unpacked.exe
; Packer support: nasm -DEXEPACK_SUPPORT=1 -f bin sentinel.asm -o build/exepack-support.bin
; Pack: python3 exepack.py build/Sentinel.unpacked.exe build/Sentinel.exe --support build/exepack-support.bin
; DOS loader relocations have been undone before encoding the file.
; All initialized game bytes are emitted here, including formerly compressed runs.
; times N db V represents N actual initialized bytes in the expanded executable.
; EXEPACK compression records and packed relocations are generated at build time.
; No INCLUDE or INCBIN; no reference EXE or listing is read during a normal build.
; Native size hints and *_load macros preserve alternate instruction encodings.
; db is used for data, undecoded bytes, and remaining exact-encoding fallbacks.
; Ghidra identification is partial; this is lossless assembly, not recovered C source.
; Original SHA-256: 0889629c699dc2bf070f5061884f611c24dcde992596aa724dd8ff581780d987
;
; Visible startup map (addresses are offsets in the expanded load image):
;   5ACF game_main_bootstrap -> 563A graphics_menu_select_adapter (when choices are needed).
;   4B6E hardware_enter_game -> 3B3C startup_title_and_landscape_flow.
;   Blue palette -> first-run five-note intro -> build title/Press Any Key while hidden.
;   3B60 waits for the intro; 3B67 reveals the title and waits for a fresh key/click.
;   3B71 reads the landscape number; nonzero landscapes also use 3BA5 secret-code entry.
;   2A6A generates terrain; 2C0B patches the outer return to 1A78 -> 1ABD preview.
;   415F redirects successful object placement to 3985: reveal preview and wait for a key.
;   A plain generation/placement return reaches 3BC4 Wrong Secret Code and retries.
; Preview renderer: terrain_draw_view -> terrain_project_corner -> projection_bearing_from_deltas / projection_vertical_angle.
; Terrain/model faces -> polygon_build_clipped_spans -> polygon_rasterize_edge -> graphics_fill_polygon_spans.
; Mesh tables expose 10 models, 160 vertices and 160 indexed faces; robot/sentry/tree/boulder/Meanie/Sentinel/pedestal and terrain-text identities name their geometry ranges.
bits 16
org 0

PIT_COMMAND equ 0x43
PIT_SPEAKER_DATA equ 0x42
PPI_PORT_B equ 0x61
PIT_SPEAKER_SQUARE_WAVE equ 0xb6
speaker_pitch_divisors equ 0x42
sound_effect_definitions equ 0xa2
sound_voice_slots equ 0x112
sound_muted equ 0x1d2
BIOS_VIDEO equ 0x10
VIDEO_SEG_CGA equ 0xb800
VIDEO_SEG_MONO equ 0xb000
VIDEO_SEG_EGA equ 0xa000
EGA_GC_INDEX equ 0x3ce
EGA_SEQ_INDEX equ 0x3c4
video_crtc_port equ 0x375c
graphics_adapter equ 0x65d4
melody_effect_flags equ 0xd2
melody_effect_pitch equ 0xd4
filename_sent equ 0x5dc9
msg_menu_navigation equ 0x5fc2
msg_menu_select equ 0x5fec
name_tandy equ 0x6004
name_ega equ 0x6014
name_vga equ 0x602e
name_cga equ 0x6044
crt_file_info_name equ 0x62e8
crt_none equ 0x634a
name_mda equ 0x6352
name_mcga equ 0x6370
name_hercules equ 0x637a
name_hercules_plus equ 0x6392
name_incolor equ 0x63aa
msg_cannot_find equ 0x63b8
msg_no_memory_1 equ 0x63c6
msg_no_memory_2 equ 0x63ea
msg_no_memory_3 equ 0x640e
msg_no_memory_4 equ 0x6432
msg_no_memory_5 equ 0x6456
msg_unsupported_adapter equ 0x647a
filename_skybox equ 0x649c
filename_tfinal equ 0x64a8
text_attribute equ 0x375e
text_cursor_row equ 0x375f
text_cursor_column equ 0x3760
game_font_8x8 equ 0x3876
DOS_SERVICES equ 0x21
DOS_OPEN equ 0x3d
DOS_CLOSE equ 0x3e
DOS_READ equ 0x3f
DOS_ALLOCATE equ 0x48
DOS_FREE equ 0x49
DOS_SEEK_END equ 0x4202
DOS_SEEK_BEGIN equ 0x4200
BIOS_KEYBOARD equ 0x16
MOUSE_SERVICES equ 0x33
KEYBOARD_DATA equ 0x60
PIC_COMMAND equ 0x20
PIC_EOI equ 0x20
SCANCODE_F2 equ 0x3c
MOUSE_SET_VERTICAL_RANGE equ 0x8
MOUSE_SET_HORIZONTAL_RANGE equ 0x7
MOUSE_SET_POSITION equ 0x4
MOUSE_SET_CALLBACK equ 0xc
key_down equ 0x350d
keyboard_e0_prefix equ 0x5be2
mouse_x equ 0x5dc2
mouse_y equ 0x5dc4
input_buttons equ 0x5dc6
mouse_moved equ 0x5dc7
mouse_installed equ 0x5dc8
PIT_TIMER_DATA equ 0x40
PIT_TIMER_SQUARE_WAVE equ 0x36
PIT_TRIPLE_RATE_DIVISOR equ 0x5555
PIC_IRQ0_EOI equ 0x60
saved_hardware_sp equ 0x5bd8
saved_int09_offset equ 0x5bda
saved_int09_segment equ 0x5bdc
bios_tick_prescaler equ 0x5bde
pit_calibration_max equ 0x5be0
saved_int15_offset equ 0x49d0
saved_int15_segment equ 0x49d2
saved_int08_offset equ 0x4adf
saved_int08_segment equ 0x4ae1
PROGRAM_DATA_PARAGRAPH equ 0x78b
DOS_GET_VERSION equ 0x30
DOS_RESIZE_BLOCK equ 0x4a
DOS_EXIT_PROCESS equ 0x4c
DOS_TERMINATE_LEGACY equ 0x20
crt_near_limit equ 0x6290
crt_initial_break equ 0x6292
crt_exit_callback equ 0x6294
crt_near_break equ 0x6296
crt_saved_int00_offset equ 0x62f5
crt_saved_int00_segment equ 0x62f7
crt_psp_segment equ 0x6307
crt_dos_version equ 0x6309
crt_file_flags equ 0x6310
crt_argc equ 0x6324
crt_argv equ 0x6326
crt_envp equ 0x6328
crt_error_callback equ 0x6338
SCENE_PALETTE_GAME equ 0x0
SCENE_PALETTE_TITLE equ 0x1
SCENE_PALETTE_BLUE equ 0x2
TEXT_SCRIPT_LANDSCAPE_PROMPT equ 0x1
TEXT_SCRIPT_SECRET_PROMPT equ 0x2
TEXT_SCRIPT_WRONG_CODE equ 0x3
TEXT_SCRIPT_PREVIEW_CAPTION equ 0x4
graphics_set_scene_palette equ 0x65c0
graphics_blit_image equ 0x65d8
skybox_image_segment equ 0x65e2
title_final_image_segment equ 0x659a
graphics_menu_count equ 0x65a0
graphics_menu_ids equ 0x65a8
graphics_menu_names equ 0x65b0
graphics_menu_lengths equ 0x65b8
intro_once_flags equ 0x5b5a
sound_melody_cursor equ 0x14f7
sound_melody_duration equ 0x1485
sound_trigger_countdown equ 0x14ec
sound_sequence_mode equ 0x1488
intro_theme_stream equ 0x172
number_entry_buffer equ 0x3699
landscape_number_bcd equ 0x36aa
landscape_nonzero equ 0x1472
landscape_difficulty equ 0x1431
prng_state_low equ 0x148e
prng_state_middle equ 0x1490
prng_state_high equ 0x1491
game_text_script_offsets equ 0x37a8
game_text_script_data equ 0x37ba
title_banner_markup equ 0x3768
scene_flags equ 0x14fc
cga_scene_palettes equ 0x5be9
tandy_scene_palettes equ 0x5bff
ega_scene_palettes equ 0x5c32
vga_scene_rgb_palettes equ 0x5c76
vga_attribute_identity equ 0x5c65
vga_work_palette equ 0x5d06
raster_x_word_offset equ 0x1d4
raster_color_index_a equ 0x1d6
raster_color_index_b equ 0x1d8
raster_shade_bank equ 0x1da
cga_left_pixel_masks equ 0x1dc
cga_right_pixel_masks equ 0x1e0
cga_dither_patterns_a equ 0x1e4
cga_dither_patterns_b equ 0x204
ega_span_mask_a equ 0x224
ega_span_mask_b equ 0x22c
ega_span_mask_c equ 0x234
tandy_edge_pixel_masks equ 0x23c
ega_polygon_busy equ 0x244
video_scanline_offsets equ 0x285
video_scanline8_offset equ 0x295
video_target_base equ 0x417
cga_background_pattern_a equ 0x419
cga_background_pattern_b equ 0x41b
tandy_background_patterns equ 0x429
vga_background_patterns equ 0x431
ega_background_patterns equ 0x439
edge_span_buffer equ 0x43e
edge_delta_y equ 0x440
edge_delta_y_high equ 0x441
edge_abs_delta_x equ 0x442
projection_x_bias equ 0x444
projection_x_bias_high equ 0x445
edge_start_y equ 0x446
edge_start_y_high equ 0x447
edge_start_x equ 0x448
edge_end_y equ 0x44a
edge_end_y_high equ 0x44b
edge_end_x equ 0x44c
terrain_azimuth_bias equ 0x44e
polygon_horizontal_x_min equ 0x45a
polygon_horizontal_x_max equ 0x45c
viewport_x_min equ 0x460
viewport_x_end equ 0x462
edge_scaled_delta_x equ 0x464
polygon_index_list equ 0x466
model_relative_heading equ 0x46e
model_relative_heading_high equ 0x46f
projection_minor_axis equ 0x470
viewport_pixel_width equ 0x474
math_encoded_factor equ 0x47a
math_signed_factor equ 0x47c
math_signed_factor_high equ 0x47d
projection_major_axis equ 0x482
projection_distance equ 0x484
projection_distance_high equ 0x485
projection_abs_delta_x equ 0x486
projection_delta_x_high equ 0x487
model_height_delta equ 0x488
projection_abs_delta_y equ 0x48a
projection_delta_y_high equ 0x48b
projection_angle equ 0x48c
projection_angle_high equ 0x48d
projection_screen_y equ 0x48e
view_pitch_reference equ 0x498
model_azimuth_bias equ 0x49c
view_azimuth_reference equ 0x49e
view_heading_byte equ 0x49f
model_relative_height equ 0x4a0
model_relative_height_high equ 0x4a1
model_planar_distance equ 0x4a2
polygon_projected_x_words equ 0x4a6
object_stack_links equ 0x5c6
object_view_pitch equ 0x606
terrain_cached_cells equ 0x646
terrain_grid equ 0x6c6
polygon_left_spans equ 0xcc6
polygon_right_spans equ 0xec6
allocated_object_slot equ 0x10ca
view_grid_x equ 0x10cb
polygon_y_min equ 0x10cc
terrain_projected_row_bank equ 0x10cd
polygon_y_max equ 0x10ce
viewport_projected_x_limit equ 0x10cf
edge_delta_x_sign equ 0x10d2
player_robot_slot equ 0x10d3
edge_saved_vertex equ 0x10d4
terrain_corner_column equ 0x10d5
viewport_preset equ 0x10d6
viewport_negative_x_threshold equ 0x10d7
polygon_vertex_count equ 0x10db
polygon_packed_colors equ 0x10dc
terrain_corner_bias equ 0x10dd
terrain_view_rotation equ 0x10de
view_grid_y equ 0x10df
projected_vertex_slot equ 0x10e1
terrain_scan_x equ 0x10e2
terrain_cell_x equ 0x10e3
terrain_scan_y equ 0x10e4
placement_candidate_mask equ 0x10e5
viewport_x_fraction_limit equ 0x10e6
terrain_row_left equ 0x10e8
terrain_row_right equ 0x10e9
terrain_triangle_shape equ 0x10ea
terrain_previous_row_left equ 0x10eb
terrain_previous_row_right equ 0x10ec
polygon_face_flags equ 0x10ed
edge_subdivision_count equ 0x10ee
terrain_triangle_orientation equ 0x10f0
polygon_edge_index equ 0x10f5
terrain_triangle_view_bias equ 0x10f6
model_mesh_id equ 0x10f7
model_element_index equ 0x10f9
model_element_end equ 0x10fa
viewport_y_end equ 0x10fb
viewport_y_min equ 0x10fc
model_face_pass equ 0x10fd
terrain_face_rotation_bias equ 0x1104
projection_direction_flags equ 0x1105
polygon_clip_work equ 0x1106
view_object_slot equ 0x1108
drawn_object_slot equ 0x1109
terrain_corner_height_a equ 0x110a
math_work_lo equ 0x110b
math_work_hi equ 0x110c
terrain_corner_height_b equ 0x110d
projection_axis_ratio equ 0x1111
projection_clip_status equ 0x1112
projection_delta_x_sign equ 0x1113
projection_delta_y_sign equ 0x1115
projection_sin equ 0x1117
projection_cos equ 0x1118
terrain_view_rotation_work equ 0x1126
terrain_draw_stop equ 0x1127
object_grid_x equ 0x112a
terrain_model_cell_x equ 0x1169
object_height_integer equ 0x116a
object_grid_y equ 0x11aa
terrain_model_cell_y equ 0x11e9
object_heading equ 0x11ea
projected_y_low equ 0x122a
projected_x_low equ 0x128a
object_height_fraction equ 0x12ea
projected_y_high equ 0x132a
projected_x_high equ 0x138a
object_model_id equ 0x13ea
terrain_model_shape equ 0x1429
landscape_object_height_limit equ 0x1430
landscape_height_scale equ 0x1432
sentinel_start_square_x equ 0x1441
sentinel_start_square_y equ 0x1442
terrain_query_flags equ 0x1443
scene_view_model_selector equ 0x1444
terrain_face_indices equ 0x1460
model_face_pass_xor equ 0x1467
terrain_row_left_hint equ 0x1468
terrain_model_render_flags equ 0x146b
scene_background_style equ 0x146c
pending_object_model equ 0x1477
object_stack_count equ 0x1480
object_stack_head equ 0x1481
landscape_sentry_count equ 0x1484
landscape_generation_flags equ 0x1486
view_lateral_offset equ 0x148b
model_dither_flags equ 0x148d
projection_atan_lut equ 0x3a76
projection_distance_correction_lut equ 0x4a86
terrain_triangle_vertex_delta equ 0x4b96
terrain_face_color_pairs equ 0x4b9a
terrain_triangle_orientations equ 0x4bba
model_vertex_ranges equ 0x4bca
model_vertex_range_ends equ 0x4bcb
model_face_ranges equ 0x4bd5
model_face_range_ends equ 0x4bd6
model_visibility_flags equ 0x4be0
model_vertex_azimuth equ 0x4d0a
model_vertex_height equ 0x4e4a
model_vertex_radius equ 0x4f8a
model_face_color_pairs equ 0x50ca
model_face_flags_and_counts equ 0x516a
model_face_index_pointers equ 0x520a
projection_quarter_sine_lut equ 0x568a
terrain_quadrant_corner_bias equ 0x5aaa
viewport_origin_high_table equ 0x5aae
viewport_x_min_table equ 0x5ab2
viewport_width_table equ 0x5ab8
viewport_x_limit_table equ 0x5abe
terrain_corner_distance equ 0x5ac2
terrain_rows_remaining equ 0x5b22
view_grid_x_overflow equ 0x5b23
terrain_shading_mode equ 0x5b24
terrain_visible_bit_masks equ 0x5b26
scene_view_preset equ 0x5b9a
pixel_scatter_sequence_low equ 0x5be5
pixel_scatter_sequence_high equ 0x5be7
status_tiles_ega equ 0x578a
status_tiles_packed equ 0x58ca
status_tiles_cga equ 0x5a0a
sentry_scan_cursor equ 0x1493
sentry_scan_target equ 0x14a3
sentry_scan_pass equ 0x14ab
sentry_meanie_slots equ 0x14b3
sentry_rotation_steps equ 0x14fd
terrain_visibility_bits equ 0x36ac
preview_view_y_offsets equ 0x377a
preview_view_pitch_values equ 0x377e
preview_view_height_offsets equ 0x3781
preview_view_heading_values equ 0x3784
preview_view_companion_heading_values equ 0x3787
scene_camera_height_table equ 0x5b86
scene_camera_pitch_table equ 0x5b88
scene_camera_x_table equ 0x5b8a
scene_lateral_offset_table equ 0x5b8c
scene_camera_y_table equ 0x5b8e
scene_camera_heading_table equ 0x5b90
scene_lateral_offset equ 0x5b9b
placement_count_masks equ 0x5b9f
cga_palette_phase equ 0x5be4
cga_clear_pixel_masks equ 0x5dae
cga_set_pixel_masks equ 0x5db2
tandy_clear_pixel_masks equ 0x5db6
tandy_set_pixel_masks equ 0x5db8
ega_single_pixel_masks equ 0x5dba
video_page_offset equ 0x607e
video_work_buffer_segment equ 0x65d6
raster_row_cursor equ 0x44a
terrain_generation_mode equ 0x10de
terrain_smoothing_flags equ 0x1104
terrain_filter_work equ 0xcc6
terrain_smoothing_passes equ 0x10da
landscape_validation_bits equ 0x4a4
placement_candidates equ 0xd06
placement_region_heights equ 0xed6
placement_candidate_x equ 0xf26
placement_candidate_y equ 0xf66
placement_height_threshold equ 0x10ce
placement_retry_counter equ 0x10da
terrain_corner_height_c equ 0x110b
terrain_corner_height_d equ 0x110c
placement_rows_remaining equ 0x440
placement_columns_remaining equ 0x442
placement_region_y_origin equ 0x44a
placement_region_x_origin equ 0x44c
scene_scatter_passes equ 0x10da
TERRAIN_SIDE equ 0x20
MODEL_FACE_VERTEX_COUNT_MASK equ 0x7
OBJECT_CELL_TAG_BASE equ 0xc0
graphics_fill_polygon_spans equ 0x65c8
graphics_scatter_pixels equ 0x6592
graphics_fill_background_band equ 0x6598
graphics_draw_status_tile equ 0x6596
graphics_select_front_buffer equ 0x65d0
graphics_select_work_buffer equ 0x65da
edge_y_major_step_offset equ 0x2294
edge_x_major_step_offset equ 0x22e8
edge_x_major_carry_displacement equ 0x22ec
edge_clipped_y_step_offset equ 0x230f
edge_clipped_x_step_offset equ 0x2333
hud_tile_column equ 0x142f
KEY_NO_CHARACTER equ 0x23
KEY_DELETE equ 0x7f
KEY_ENTER equ 0xd
keyboard_ascii_by_scan_code equ 0x4b16
RGBI4_BLACK equ 0x0
RGBI4_BLUE equ 0x1
RGBI4_GREEN equ 0x2
RGBI4_CYAN equ 0x3
RGBI4_RED equ 0x4
RGBI4_MAGENTA equ 0x5
RGBI4_BROWN equ 0x6
RGBI4_LIGHT_GRAY equ 0x7
RGBI4_LIGHT_BLUE equ 0x9
RGBI4_LIGHT_GREEN equ 0xa
RGBI4_LIGHT_CYAN equ 0xb
RGBI4_LIGHT_RED equ 0xc
RGBI4_LIGHT_MAGENTA equ 0xd
RGBI4_YELLOW equ 0xe
RGBI4_WHITE equ 0xf
EGA_200_INTENSITY equ 0x10
tandy_landscape_color_3_variants equ 0x5d36
tandy_landscape_color_2_variants equ 0x5d3e
tandy_landscape_color_1_variants equ 0x5d46
ega_landscape_color_3_variants equ 0x5d4e
ega_landscape_color_2_variants equ 0x5d56
ega_landscape_color_1_variants equ 0x5d5e
vga_landscape_color_3_variants equ 0x5d66
vga_landscape_color_2_variants equ 0x5d7e
vga_landscape_color_1_variants equ 0x5d96
graphics_set_landscape_colors equ 0x65c2
SOUND_ENABLED equ 0x80
SOUND_NOISE equ 0x4
SOUND_OCTAVE_DOWN equ 0x8
SOUND_VOICE_0 equ 0x0
SOUND_VOICE_1 equ 0x1
SOUND_VOICE_2 equ 0x2
SOUND_PITCH_MOD_DISABLED equ 0xff
SOUND_LEVEL equ 0x1
SOUND_PITCH equ 0x2
SOUND_ENVELOPE_PERIOD equ 0x3
SOUND_ENVELOPE_STEP equ 0x4
SOUND_ENVELOPE_STEPS equ 0x5
SOUND_ENVELOPE_NEXT_STEP equ 0x6
SOUND_ENVELOPE_NEXT_STEPS equ 0x7
SOUND_PITCH_STEP_1 equ 0x8
SOUND_PITCH_STEPS_1 equ 0x9
SOUND_PITCH_PERIOD equ 0xa
SOUND_ENVELOPE_NEXT_PERIOD equ 0xb
SOUND_PITCH_STEP_2 equ 0xc
SOUND_PITCH_STEPS_2 equ 0xd
SOUND_PITCH_STEP_3 equ 0xe
SOUND_PITCH_STEPS_3 equ 0xf
SOUND_EFFECT_BYTES equ 0x10
SOUND_VOICE_BYTES equ 0x20
SOUND_CURRENT_PITCH_STEP equ 0x10
SOUND_PITCH_STEPS_LEFT equ 0x11
SOUND_ENVELOPE_COUNTDOWN equ 0x14
SOUND_PITCH_COUNTDOWN equ 0x15
SOUND_PITCH_PHASE equ 0x16
TEXT_SCRIPT_CALL_BASE equ 0xc8
TEXT_SCRIPT_END equ 0xff
TEXT_SCRIPT_TITLE_PRESS_ANY_KEY equ 0x0
TEXT_SCRIPT_POSITION_ROW1 equ 0x7
TEXT_SCRIPT_POSITION_ROW3 equ 0x8
TEXT_SCRIPT_POSITION_ROW23 equ 0x9
TEXT_SCRIPT_TEXT_STYLE_TITLE equ 0xa
TEXT_SCRIPT_TEXT_STYLE_PREVIEW equ 0xb
TEXT_SCRIPT_POSITION_ROW22 equ 0xc
TEXT_SCRIPT_WORD_LANDSCAPE equ 0xd
TEXT_SCRIPT_WORDS_SECRET_ENTRY_CODE equ 0xe
TEXT_SCRIPT_BLANK_FIVE_CHARACTERS equ 0xf
TEXT_SCRIPT_BLANK_THREE_CHARACTERS equ 0x10
TEXT_SCRIPT_WORDS_PRESS_ANY_KEY equ 0x11
video_probe_records equ 0x5bc8
video_probe_display_combination_enabled equ 0x5bc8
video_probe_ega_configuration_enabled equ 0x5bcb
video_probe_mode8_columns_enabled equ 0x5bce
video_probe_color_crtc_enabled equ 0x5bd1
video_probe_mono_crtc_enabled equ 0x5bd4
KEY_SCAN_COUNT equ 0x80
KEY_RELEASED equ 0x0
KEY_PRESSED equ 0xff
OBJECT_SLOT_COUNT equ 0x40
OBJECT_LINK_TERRAIN equ 0x0
MESH_PASS_STORED_ORDER equ 0x0
MESH_PASS_BY_HEADING equ 0x2
MESH_PASS_BY_HEIGHT equ 0x3
FACE_ORDER_GROUP_2 equ 0x80
TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT equ 0x0
TRI_SPLIT_TOP_RIGHT_BOTTOM_LEFT equ 0x1
CGA_PALETTE_GREEN_RED_BROWN equ 0x0
CGA_PALETTE_CYAN_MAGENTA_GRAY equ 0x1
CGA_PALETTE_BRIGHT equ 0x10
cga_palette_variant_selectors equ 0x5bef
cga_background_variant_controls equ 0x5bf7
tandy_right_pixel_masks equ 0x23e
tandy_left_secondary_masks equ 0x240
tandy_right_secondary_masks equ 0x242
VIDEO_SCANLINE_COUNT equ 0xc9
PROJECTED_MODEL_CACHE_COUNT equ 0x20
CRT_ERROR_STACK equ 0x0
CRT_ERROR_INTEGER_DIVIDE equ 0x3
CRT_ERROR_ENVIRONMENT equ 0x9
CRT_ERROR_NEWLINE equ 0xfc
CRT_ERROR_GENERIC equ 0xff
CRT_ERROR_FLOATING_POINT equ 0x2
CRT_ERROR_NULL_ASSIGNMENT equ 0x1
CRT_ERROR_TABLE_END equ 0xffff
crt_error_message_table equ 0x64bb
adapter_name_pointers equ 0x606c
adapter_label_cga equ 0x6368
adapter_label_ega equ 0x636c
adapter_label_vga equ 0x6376
INPUT_DIGIT_CAPACITY equ 0x8
object_energy_values equ 0x5b43
input_key_bindings equ 0x5b5e
input_binding_actions equ 0x5b72
SCANCODE_LEFT equ 0x4b
SCANCODE_RIGHT equ 0x4d
SCANCODE_UP equ 0x48
SCANCODE_DOWN equ 0x50
SCANCODE_RAW_79 equ 0x79
SCANCODE_RAW_7A equ 0x7a
SCANCODE_RAW_7B equ 0x7b
SCANCODE_RAW_7C equ 0x7c
SCANCODE_A equ 0x1e
SCANCODE_Q equ 0x10
SCANCODE_R equ 0x13
SCANCODE_T equ 0x14
SCANCODE_B equ 0x30
SCANCODE_H equ 0x23
SCANCODE_F1 equ 0x3b
SCANCODE_F3 equ 0x3d
SCANCODE_F4 equ 0x3e
SCANCODE_U equ 0x16
VERTEX_HEIGHT_NEGATIVE equ 0x80
bios_display_combination_descriptors equ 0x5bae
menu_normal_background equ 0x605c
menu_selected_background equ 0x605e
menu_normal_foreground equ 0x6060
menu_selected_foreground equ 0x6062
video_display_enable_wait_counts equ 0x6064
video_previous_page_offset equ 0x6080
video_pending_page_offset equ 0x6082
video_transition_wait_count equ 0x608e
CRT_HANDLE_OPEN equ 0x1
crt_argv_saved_return equ 0x633a
crt_program_name_pointer equ 0x632a
crt_program_name_fallback equ 0x632e
crt_optional_service_hook equ 0x633e
crt_service_init_argument equ 0x6342
crt_service_secondary_argument equ 0x6346
view_y_end_presets equ 0x5b4f
view_y_min_presets equ 0x5b51
input_action_slots equ 0x14f8
model_selection_extents equ 0x5b3c
camera_transition_steps equ 0x5b4a
camera_transition_pitch_targets equ 0x5b5b
scene_text_color_presets_a equ 0x5b92
scene_text_color_presets_b equ 0x5b96
text_color_a_parameter equ 0x37bb
text_color_b_parameter equ 0x37c6
TEXT_COLOR_INDEX_0 equ 0x0
TEXT_COLOR_INDEX_1 equ 0x1
TEXT_COLOR_INDEX_2 equ 0x2
TEXT_COLOR_INDEX_3 equ 0x3
glyph_pair_terrain_cells equ 0x3762
TERRAIN_GLYPH_HEIGHT equ 0x2
cga_text_color_pixels equ 0x372c
cga_font_nibble_masks equ 0x373c
game_text_pending_opcode equ 0x378e
game_text_parameter_index equ 0x378f
game_text_alternate_mode equ 0x3790
game_text_transparent equ 0x3792
game_text_glyph_flags equ 0x3794
game_text_normal_foreground equ 0x3796
game_text_normal_background equ 0x3798
game_text_alternate_foreground equ 0x379a
game_text_alternate_background equ 0x379c
game_text_active_foreground equ 0x379e
game_text_active_background equ 0x37a0
game_text_column equ 0x37a2
game_text_row equ 0x37a4
game_text_input_character equ 0x37a6
cga_transition_source_mode_0 equ 0x6090
cga_transition_source_mode_1 equ 0x60b0
cga_transition_source_mode_2 equ 0x60d0
cga_transition_source_mode_3 equ 0x60e0
cga_transition_page_deltas equ 0x60f0
ega_transition_source_mode_0 equ 0x60f8
ega_transition_source_mode_1 equ 0x6118
ega_transition_source_mode_2 equ 0x6138
ega_transition_source_mode_3 equ 0x6148
ega_transition_page_deltas equ 0x6158
vga_transition_source_mode_0 equ 0x6160
vga_transition_source_mode_1 equ 0x6180
vga_transition_source_mode_2 equ 0x61a0
vga_transition_source_mode_3 equ 0x61b0
vga_transition_page_deltas equ 0x61c0
cga_sentry_warning_patterns equ 0x61cc
CGA_WARNING_COLOR equ 0x1
tandy_sentry_warning_patterns equ 0x61e4
TANDY_WARNING_COLOR equ 0x9
ega_sentry_warning_patterns equ 0x6214
vga_sentry_warning_patterns equ 0x6220
VGA_WARNING_COLOR equ 0x9
warning_bar_noise_low equ 0x61c8
warning_bar_noise_high equ 0x61ca
copy_sequence_low equ 0x6280
copy_sequence_high equ 0x6282
cga_random_copy_nibble_masks equ 0x6284
ega_random_copy_bit_masks equ 0x628c
sentry_replant_energy equ 0x149b
SENTRY_SLOT_COUNT equ 0x8
crt_optional_interrupt_enabled equ 0x6332
crt_optional_interrupt_number equ 0x6333
crt_optional_interrupt_vector equ 0x6334
landscape_zero_entry_bcd equ 0x5b56
text_inline_control_bytes_left equ 0x3778
text_character_controls_a equ 0x37bc
TEXT_CTL_GLYPH equ 0x19
TEXT_CTL_ALT_COLOR equ 0x12
TEXT_CTL_BACKSPACE equ 0x8
TEXT_CTL_DELETE equ 0x7f
TEXT_GLYPH_USE_PREVIOUS_ROW equ 0xff
video_transition_countdown equ 0x5b4e
preview_escape_restarts equ 0x5b54
occlusion_height_rows equ 0x150d
occlusion_cell_threshold_rows equ 0x152d
OCCLUSION_ROW_STRIDE equ 0x100
occlusion_ray_step_x equ 0x454
occlusion_ray_step_height equ 0x456
occlusion_ray_step_y equ 0x458
occlusion_height_row_pointer equ 0x47e
occlusion_height_row_pointer_high equ 0x47f
sound_mute_saved equ 0x1d3
ega_switch_subtypes equ 0x5ba8
cursor_step_ink_records equ 0x5f53
CURSOR_STEP_COUNT equ 0x15
CURSOR_DRAW_INK equ 0xff
cursor_step_x_deltas equ 0x5f7d
cursor_step_scanline_deltas equ 0x5f93
CURSOR_STEPS_END equ 0x80
cursor_cga_clear_pair_masks equ 0x5fa9
cursor_cga_set_pair_masks equ 0x5fab
CURSOR_CGA_INK equ 0x3
cursor_ega_pair_masks equ 0x5fad
cursor_saved_video_offsets equ 0x5dd3
cursor_saved_pixels equ 0x5e33
graphics_draw_software_cursor equ 0x65ce
graphics_restore_software_cursor equ 0x65c4
software_cursor_x_pairs equ 0x3695
software_cursor_inverted_y equ 0x3696
software_cursor_saved_last_step equ 0x14db
software_cursor_hide_flags equ 0x14e9
sound_ramp_index equ 0x1489
sentry_absorb_delays equ 0x1448
sentry_rotation_delays equ 0x1450
sentry_scan_delays equ 0x1458
sentry_countdown_phase equ 0x5b5d
sentry_countdown_bytes equ 0x1448
sentry_target_slots equ 0x14bb
sentry_target_sight equ 0x14c3
ray_position_x equ 0x1119
ray_position_height equ 0x111c
ray_position_y equ 0x111f
ray_heading_angle equ 0x468
ray_pitch_angle equ 0x46a
player_energy equ 0x1434
selection_view_total_columns equ 0x5b2e
selection_view_origin_column equ 0x5b2f
selection_saved_heading equ 0x5b30
selection_copy_iteration_budget equ 0x5b31
selection_copy_origin equ 0x5b32
selection_view_column equ 0x1478
render_pass_columns equ 0x147e
selection_view_columns_left equ 0x147f
selection_presentation_flags equ 0x1482
selection_model_draw_flags equ 0x146d
selection_extent_override equ 0x14e6
graphics_copy_selection_strip equ 0x65cc
graphics_reveal_selection_pixels equ 0x65d2
ray_contact_edge_selectors equ 0x5b34
ray_corner_height_wrap equ 0x110e
ray_contact_work equ 0x110f
RAY_CONTACT_USE_X equ 0x1
RAY_CONTACT_COMPLEMENT equ 0x2
ray_corner_heights equ 0x110a
ray_contact_fraction equ 0x43f
ray_sample_fraction equ 0x1110
ray_sample_mode_flags equ 0x1103
ray_sample_special_flags equ 0x147c
camera_transition_mode equ 0x10d0
camera_transition_request equ 0x10d1
video_transition_steps_left equ 0x14d3
video_transition_step_count equ 0x14e3
camera_cursor_x_pairs equ 0x14d8
camera_cursor_inverted_y equ 0x14d9
MODEL_ROBOT equ 0x0
MODEL_SENTRY equ 0x1
MODEL_TREE equ 0x2
MODEL_BOULDER equ 0x3
MODEL_MEANIE equ 0x4
MODEL_SENTINEL equ 0x5
MODEL_SENTINEL_PEDESTAL equ 0x6
MODEL_TEXT_BLOCK_01 equ 0x7
MODEL_TEXT_BLOCK_10 equ 0x8
MODEL_TEXT_BLOCK_11 equ 0x9
ENERGY_ROBOT equ 0x3
ENERGY_SENTRY equ 0x3
ENERGY_TREE equ 0x1
ENERGY_BOULDER equ 0x2
ENERGY_MEANIE equ 0x1
ENERGY_SENTINEL equ 0x4
ENERGY_SENTINEL_PEDESTAL equ 0x0
HUD_GOLDEN_ROBOT_ENERGY equ 0xf
ROBOT_VERTEX_START equ 0x0
ROBOT_FACE_START equ 0x0
SENTRY_VERTEX_START equ 0x1d
SENTRY_FACE_START equ 0x1b
TREE_VERTEX_START equ 0x33
TREE_FACE_START equ 0x34
BOULDER_VERTEX_START equ 0x44
BOULDER_FACE_START equ 0x43
MEANIE_VERTEX_START equ 0x4c
MEANIE_FACE_START equ 0x4d
SENTINEL_VERTEX_START equ 0x5e
SENTINEL_FACE_START equ 0x66
SENTINEL_PEDESTAL_VERTEX_START equ 0x7c
SENTINEL_PEDESTAL_FACE_START equ 0x89
TEXT_BLOCK_01_VERTEX_START equ 0x88
TEXT_BLOCK_01_FACE_START equ 0x94
TEXT_BLOCK_10_VERTEX_START equ 0x90
TEXT_BLOCK_10_FACE_START equ 0x98
TEXT_BLOCK_11_VERTEX_START equ 0x98
TEXT_BLOCK_11_FACE_START equ 0x9c
current_sentry_slot equ 0x10c9
sentry_target_sight_result equ 0x10d9
sentry_warning_pattern equ 0x142e
previous_sentry_warning_pattern equ 0x1433
warning_bar_sight equ 0x146f
sentry_test_target_slot equ 0x1474
cursor_mode_flags equ 0x1475
sentry_scan_angle_width equ 0x147d
cursor_mode_toggle_latch equ 0x5b55
graphics_draw_sentry_warning_bar equ 0x659e
CURSOR_MODE_ACTIVE equ 0x80
SIGHT_TARGET_VISIBLE equ 0x40
SIGHT_SQUARE_VISIBLE equ 0x80
ACTION_CREATE_ROBOT equ 0x0
ACTION_CREATE_TREE equ 0x2
ACTION_CREATE_BOULDER equ 0x3
ACTION_ABSORB equ 0x20
ACTION_TRANSFER equ 0x21
ACTION_HYPERSPACE equ 0x22
ACTION_TURN_AROUND equ 0x23
player_action_code equ 0x1477
hud_energy_remaining equ 0x10da
bcd_random_digit_work equ 0x3767
title_secret_screen_flags equ 0x3766
restart_requested_flags equ 0x147a
game_end_flags equ 0x146e
hyperspace_outcome_flags equ 0x14eb
game_glyph_rows equ 0x1438
terrain_text_column equ 0x1469
terrain_text_row equ 0x146a
terrain_text_character_work equ 0x1437
text_input_render_flags equ 0x1436
number_terrain_text_flags equ 0x1476
secret_code_pair_work equ 0x14f6
game_pause_flags equ 0x1487
cursor_update_guard_flags equ 0x3694
video_transition_busy_flags equ 0x3697
mouse_cursor_mode_flags equ 0x3691
mouse_activation_click_latch equ 0x3690
input_request_ready_flags equ 0x14f4
input_ready_snapshot equ 0x14ea
player_view_refresh_flags equ 0x1479
sentry_clock_gate_flags equ 0x14f5
sentry_scheduler_saved_sp equ 0x5b9d
sentry_sight_saved_source_slot equ 0x5b9c
ray_target_hit_flags equ 0x10c6
ray_intervening_hit_flags equ 0x10c7
sentry_intervening_sight_bits equ 0x10c8
ray_horizontal_scale equ 0x45e
sentry_stack_search_flags equ 0x14cb
game_timer_suspend_flags equ 0x36a9
turn_and_occlusion_flags equ 0x1471
cursor_repeat_phase equ 0x14da
held_direction_action equ 0x1445
partial_view_update_flags equ 0x1447
meanie_turn_step equ 0x1435
ray_target_test_flags equ 0x1483
trig_phase_work equ 0x49a
selection_copy_column_accumulator equ 0x14df
ega_selection_column_mask equ 0x14e1
ega_selection_column_limit equ 0x14e5
selection_copy_inner_countdown equ 0x14e4
polygon_horizontal_edge_count equ 0x10e0
scene_scatter_outer_count equ 0x10e0
placement_remaining_trees equ 0x10e0
sentry_sight_rays_left equ 0x10e0
mouse_axis_priority equ 0x3693
trig_component_a equ 0x142a
trig_component_b equ 0x142c
resource_cga_color_patterns equ 0x245
selection_copy_interrupt_flags equ 0x1446
occlusion_reference_check_flags equ 0x14e0
trig_source_angle equ 0x495
overview_camera_slot equ 0x368e
overview_pending equ 0x368d
input_poll_delay_ticks equ 0x368f
resource_read_buffer equ 0x43e
resource_read_buffer_end equ 0x3698
MELODY_TRANSFER equ 0x19
MELODY_TURN_AROUND equ 0x28
MELODY_END_EFFECT equ 0x32
MELODY_LANDSCAPE_COMPLETION equ 0x42
terrain_generation_saved_noise equ 0x378d
text_prng_compare_value equ 0x148a
text_prng_compare_limit equ 0x378d
sentry_sight_noise_limit equ 0x148a
KEY_WAIT_SCAN_COUNT equ 0x7d
generation_validation_seed equ 0x1473
generation_validation_ring equ 0x358d
terrain_checker_prng_snapshot equ 0x1473
generation_reference_window equ 0x3698
GENERATION_REFERENCE_LAST equ 0xaa
GENERATION_COMPLETION_REFERENCE_LAST equ 0xa5
GENERATION_MATCH_MARKER equ 0x7f
GENERATION_CODE_MATCH_MASK equ 0x1e
TEXT_SCRIPT_NEXT_LANDSCAPE equ 0x5
CRT_NULL_GUARD_SIZE equ 0x42
CRT_NULL_GUARD_XOR equ 0x55
keyboard_left_button_state equ 0x358b
keyboard_right_button_state equ 0x358a
MOUSE_CURSOR_X_MIN_PAIRS equ 0x8
MOUSE_CURSOR_X_MAX_PAIRS equ 0x97
MOUSE_CURSOR_Y_MIN equ 0xc
MOUSE_CURSOR_Y_MAX equ 0xbb
text_script_title_press_any_key equ 0x37d1
text_script_landscape_number_prompt equ 0x37d7
text_script_secret_entry_prompt equ 0x37ea
text_script_wrong_secret_code equ 0x37f3
text_script_landscape_preview_caption equ 0x3808
text_script_landscape_number_caption equ 0x3811
text_script_reposition_press_any_key equ 0x3817
text_script_position_row1 equ 0x381a
text_script_position_row3 equ 0x3821
text_script_position_row23 equ 0x3828
text_script_text_style_title equ 0x382f
text_script_text_style_preview equ 0x3834
text_script_position_row22 equ 0x3839
text_script_word_landscape equ 0x3840
text_script_words_secret_entry_code equ 0x384a
text_script_blank_five_characters equ 0x385c
text_script_blank_three_characters equ 0x3862
text_script_words_press_any_key equ 0x3866
model_vertex_loop_4bf7 equ 0x4bf7
model_vertex_loop_4bfc equ 0x4bfc
model_vertex_loop_4c00 equ 0x4c00
model_vertex_loop_4c04 equ 0x4c04
model_vertex_loop_4c08 equ 0x4c08
model_vertex_loop_4c0c equ 0x4c0c
model_vertex_loop_4c11 equ 0x4c11
model_vertex_loop_4c15 equ 0x4c15
model_vertex_loop_4c19 equ 0x4c19
model_vertex_loop_4c1e equ 0x4c1e
model_vertex_loop_4c22 equ 0x4c22
model_vertex_loop_4c26 equ 0x4c26
model_vertex_loop_4c2a equ 0x4c2a
model_vertex_loop_4c2e equ 0x4c2e
model_vertex_loop_4c32 equ 0x4c32
model_vertex_loop_4c36 equ 0x4c36
model_vertex_loop_4c3a equ 0x4c3a
model_vertex_loop_4c3f equ 0x4c3f
model_vertex_loop_4c43 equ 0x4c43
model_vertex_loop_4c47 equ 0x4c47
model_vertex_loop_4c4b equ 0x4c4b
model_vertex_loop_4c4f equ 0x4c4f
model_vertex_loop_4c53 equ 0x4c53
model_vertex_loop_4c57 equ 0x4c57
model_vertex_loop_4c5c equ 0x4c5c
model_vertex_loop_5379 equ 0x5379
model_vertex_loop_537d equ 0x537d
model_vertex_loop_5382 equ 0x5382
model_vertex_loop_5386 equ 0x5386
model_vertex_loop_538a equ 0x538a
model_vertex_loop_538e equ 0x538e
model_vertex_loop_5392 equ 0x5392
model_vertex_loop_5396 equ 0x5396
model_vertex_loop_539a equ 0x539a
model_vertex_loop_539f equ 0x539f
model_vertex_loop_53a3 equ 0x53a3
model_vertex_loop_53a7 equ 0x53a7
model_vertex_loop_53ab equ 0x53ab
model_vertex_loop_53af equ 0x53af
model_vertex_loop_53b4 equ 0x53b4
model_vertex_loop_53b9 equ 0x53b9
model_vertex_loop_53be equ 0x53be
model_vertex_loop_53c2 equ 0x53c2
model_vertex_loop_53c6 equ 0x53c6
model_vertex_loop_53cb equ 0x53cb
model_vertex_loop_53d0 equ 0x53d0
model_vertex_loop_53d5 equ 0x53d5
model_vertex_loop_53da equ 0x53da
model_vertex_loop_53df equ 0x53df
model_vertex_loop_53e4 equ 0x53e4
model_vertex_loop_54b9 equ 0x54b9
model_vertex_loop_54bd equ 0x54bd
model_vertex_loop_54c1 equ 0x54c1
model_vertex_loop_54c5 equ 0x54c5
model_vertex_loop_54c9 equ 0x54c9
model_vertex_loop_54f7 equ 0x54f7
model_vertex_loop_54fc equ 0x54fc
model_vertex_loop_5501 equ 0x5501
model_vertex_loop_5506 equ 0x5506
model_vertex_loop_550b equ 0x550b
model_vertex_loop_550f equ 0x550f
model_vertex_loop_5513 equ 0x5513
model_vertex_loop_5517 equ 0x5517
model_vertex_loop_551b equ 0x551b
model_vertex_loop_5520 equ 0x5520
model_vertex_loop_5525 equ 0x5525
model_vertex_loop_552a equ 0x552a
model_vertex_loop_552f equ 0x552f
model_vertex_loop_5534 equ 0x5534
model_vertex_loop_5539 equ 0x5539
model_vertex_loop_556f equ 0x556f
model_vertex_loop_5573 equ 0x5573
model_vertex_loop_5577 equ 0x5577
model_vertex_loop_557b equ 0x557b
model_vertex_loop_557f equ 0x557f
model_vertex_loop_5583 equ 0x5583
model_vertex_loop_5587 equ 0x5587
model_vertex_loop_558b equ 0x558b
model_vertex_loop_558f equ 0x558f
model_vertex_loop_5593 equ 0x5593
model_vertex_loop_5598 equ 0x5598
model_vertex_loop_559c equ 0x559c
model_vertex_loop_55a0 equ 0x55a0
model_vertex_loop_55a5 equ 0x55a5
model_vertex_loop_55aa equ 0x55aa
model_vertex_loop_55af equ 0x55af
model_vertex_loop_55b4 equ 0x55b4
model_vertex_loop_55e4 equ 0x55e4
model_vertex_loop_55e8 equ 0x55e8
model_vertex_loop_55ec equ 0x55ec
model_vertex_loop_55f1 equ 0x55f1
model_vertex_loop_55f5 equ 0x55f5
model_vertex_loop_55f9 equ 0x55f9
model_vertex_loop_55fd equ 0x55fd
model_vertex_loop_5602 equ 0x5602
model_vertex_loop_5607 equ 0x5607
model_vertex_loop_560c equ 0x560c
model_vertex_loop_5611 equ 0x5611
model_vertex_loop_5616 equ 0x5616
model_vertex_loop_561b equ 0x561b
model_vertex_loop_5620 equ 0x5620
model_vertex_loop_5625 equ 0x5625
ROBOT_VERTEX_11 equ 0x4b
ROBOT_VERTEX_10 equ 0x4a
ROBOT_VERTEX_08 equ 0x48
ROBOT_VERTEX_12 equ 0x4c
robot_face_00_vertices equ 0x553e
SENTRY_VERTEX_03 equ 0x43
SENTRY_VERTEX_08 equ 0x48
SENTRY_VERTEX_07 equ 0x47
SENTRY_VERTEX_02 equ 0x42
sentry_face_00_vertices equ 0x55b9
TREE_VERTEX_08 equ 0x48
TREE_VERTEX_15 equ 0x4f
TREE_VERTEX_14 equ 0x4e
TREE_VERTEX_09 equ 0x49
tree_face_00_vertices equ 0x548a
BOULDER_VERTEX_01 equ 0x41
BOULDER_VERTEX_00 equ 0x40
BOULDER_VERTEX_07 equ 0x47
boulder_face_00_vertices equ 0x54cd
SENTINEL_VERTEX_20 equ 0x54
SENTINEL_VERTEX_16 equ 0x50
SENTINEL_VERTEX_17 equ 0x51
SENTINEL_VERTEX_21 equ 0x55
sentinel_face_00_vertices equ 0x534a
ROBOT_VERTEX_09 equ 0x49
robot_face_01_vertices equ 0x5543
SENTRY_VERTEX_00 equ 0x40
SENTRY_VERTEX_09 equ 0x49
sentry_face_01_vertices equ 0x55be
TREE_VERTEX_13 equ 0x4d
TREE_VERTEX_10 equ 0x4a
tree_face_01_vertices equ 0x548f
BOULDER_VERTEX_03 equ 0x43
BOULDER_VERTEX_02 equ 0x42
boulder_face_01_vertices equ 0x54d1
SENTINEL_VERTEX_14 equ 0x4e
sentinel_face_01_vertices equ 0x534f
ROBOT_VERTEX_20 equ 0x54
ROBOT_VERTEX_19 equ 0x53
ROBOT_VERTEX_18 equ 0x52
ROBOT_VERTEX_21 equ 0x55
robot_face_02_vertices equ 0x5547
SENTRY_VERTEX_06 equ 0x46
SENTRY_VERTEX_01 equ 0x41
sentry_face_02_vertices equ 0x55c2
TREE_VERTEX_12 equ 0x4c
TREE_VERTEX_11 equ 0x4b
tree_face_02_vertices equ 0x5494
BOULDER_VERTEX_05 equ 0x45
BOULDER_VERTEX_04 equ 0x44
boulder_face_02_vertices equ 0x54d5
SENTINEL_VERTEX_15 equ 0x4f
sentinel_face_02_vertices equ 0x5353
ROBOT_VERTEX_00 equ 0x40
ROBOT_VERTEX_04 equ 0x44
ROBOT_VERTEX_07 equ 0x47
ROBOT_VERTEX_03 equ 0x43
robot_face_03_vertices equ 0x554c
SENTRY_VERTEX_05 equ 0x45
SENTRY_VERTEX_04 equ 0x44
sentry_face_03_vertices equ 0x55c6
TREE_VERTEX_01 equ 0x41
TREE_VERTEX_05 equ 0x45
TREE_VERTEX_04 equ 0x44
TREE_VERTEX_00 equ 0x40
tree_face_03_vertices equ 0x5499
BOULDER_VERTEX_06 equ 0x46
boulder_face_03_vertices equ 0x54d9
SENTINEL_VERTEX_19 equ 0x53
SENTINEL_VERTEX_18 equ 0x52
sentinel_face_03_vertices equ 0x5357
ROBOT_VERTEX_02 equ 0x42
ROBOT_VERTEX_06 equ 0x46
ROBOT_VERTEX_05 equ 0x45
ROBOT_VERTEX_01 equ 0x41
robot_face_04_vertices equ 0x5551
sentry_face_04_vertices equ 0x55cb
TREE_VERTEX_02 equ 0x42
TREE_VERTEX_06 equ 0x46
tree_face_04_vertices equ 0x549e
boulder_face_04_vertices equ 0x54dd
SENTINEL_VERTEX_03 equ 0x43
SENTINEL_VERTEX_07 equ 0x47
SENTINEL_VERTEX_06 equ 0x46
SENTINEL_VERTEX_02 equ 0x42
sentinel_face_04_vertices equ 0x535c
robot_face_05_vertices equ 0x5556
sentry_face_05_vertices equ 0x55cf
TREE_VERTEX_07 equ 0x47
TREE_VERTEX_03 equ 0x43
tree_face_05_vertices equ 0x54a3
boulder_face_05_vertices equ 0x54e1
SENTINEL_VERTEX_00 equ 0x40
SENTINEL_VERTEX_04 equ 0x44
sentinel_face_05_vertices equ 0x5361
robot_face_06_vertices equ 0x555b
sentry_face_06_vertices equ 0x55d3
tree_face_06_vertices equ 0x54a8
boulder_face_06_vertices equ 0x54e5
SENTINEL_VERTEX_05 equ 0x45
SENTINEL_VERTEX_01 equ 0x41
sentinel_face_06_vertices equ 0x5366
ROBOT_VERTEX_13 equ 0x4d
ROBOT_VERTEX_16 equ 0x50
robot_face_07_vertices equ 0x5560
sentry_face_07_vertices equ 0x55d7
TREE_VERTEX_16 equ 0x50
tree_face_07_vertices equ 0x54ad
boulder_face_07_vertices equ 0x54e9
sentinel_face_07_vertices equ 0x536b
ROBOT_VERTEX_15 equ 0x4f
ROBOT_VERTEX_14 equ 0x4e
robot_face_08_vertices equ 0x5565
SENTRY_VERTEX_13 equ 0x4d
sentry_face_08_vertices equ 0x55db
tree_face_08_vertices equ 0x54b1
boulder_face_08_vertices equ 0x54ed
SENTINEL_VERTEX_12 equ 0x4c
SENTINEL_VERTEX_11 equ 0x4b
sentinel_face_08_vertices equ 0x5370
robot_face_09_vertices equ 0x556a
SENTRY_VERTEX_12 equ 0x4c
sentry_face_09_vertices equ 0x55df
tree_face_09_vertices equ 0x54b5
boulder_face_09_vertices equ 0x54f2
SENTINEL_VERTEX_13 equ 0x4d
sentinel_face_09_vertices equ 0x5375

; Alternate register-register encodings: *_load selects opcode direction D=1.
; Operands retain normal destination, source order; both must have the same width.
%define encoding_reg_al 0
%define encoding_reg_cl 1
%define encoding_reg_dl 2
%define encoding_reg_bl 3
%define encoding_reg_ah 4
%define encoding_reg_ch 5
%define encoding_reg_dh 6
%define encoding_reg_bh 7
%define encoding_reg_ax 8
%define encoding_reg_cx 9
%define encoding_reg_dx 10
%define encoding_reg_bx 11
%define encoding_reg_sp 12
%define encoding_reg_bp 13
%define encoding_reg_si 14
%define encoding_reg_di 15
%macro encoding_reg_load 3
    %assign %%dst encoding_reg_%2
    %assign %%src encoding_reg_%3
    %if (%%dst >> 3) != (%%src >> 3)
        %error register widths must match
    %endif
    db %1 + (%%dst >> 3), 0xc0 | ((%%dst & 7) << 3) | (%%src & 7)
%endmacro
%macro add_load 2
    encoding_reg_load 0x02, %1, %2
%endmacro
%macro or_load 2
    encoding_reg_load 0x0a, %1, %2
%endmacro
%macro adc_load 2
    encoding_reg_load 0x12, %1, %2
%endmacro
%macro sbb_load 2
    encoding_reg_load 0x1a, %1, %2
%endmacro
%macro and_load 2
    encoding_reg_load 0x22, %1, %2
%endmacro
%macro sub_load 2
    encoding_reg_load 0x2a, %1, %2
%endmacro
%macro xor_load 2
    encoding_reg_load 0x32, %1, %2
%endmacro
%macro cmp_load 2
    encoding_reg_load 0x3a, %1, %2
%endmacro
%macro mov_load 2
    encoding_reg_load 0x8a, %1, %2
%endmacro

; Capacity tails lie beyond the established adjacent consumers, not necessarily all program use.
; Independent uses of these five tails remain unconfirmed; zero content does not prove padding.
%macro unconfirmed_capacity_tail 1
    times %1 db 0
%endmacro
; Repeated initializer chunks: names describe bytes, not a graphics/model interpretation.
; Consumers and original intent of all seven opaque trailers remain unconfirmed.
%macro opaque_init_chunk 1
    %ifidni %1, ZERO
        times 32 db 0
    %elifidni %1, ONES
        times 32 db 0xff
    %elifidni %1, ONES_WITH_A0_EF
        db 0xff, 0xa0
        times 15 db 0xff
        db 0xef
        times 14 db 0xff
    %elifidni %1, ZERO_WITH_20
        times 17 db 0
        db 0x20
        times 14 db 0
    %else
        %error unknown opaque initializer chunk
    %endif
%endmacro

; Melody bytecode: pitch < C8h; C8h..FEh set delay; FFh ends sequencing.
; The pitch table has 48 steps/octave: four steps per semitone.
; Nominal note names: divisor 3550h at the PC PIT clock gives about 87.43 Hz,
; approximately F2 (87.31 Hz at A4=440). Names describe the base pitch before effect modulation.
; Bb means B-flat; B means B-natural. Octave numbering uses middle C = C4.
NOTE_F2 equ 0
NOTE_Gb2 equ 1
NOTE_G2 equ 2
NOTE_Ab2 equ 3
NOTE_A2 equ 4
NOTE_Bb2 equ 5
NOTE_B2 equ 6
NOTE_C3 equ 7
NOTE_Db3 equ 8
NOTE_D3 equ 9
NOTE_Eb3 equ 10
NOTE_E3 equ 11
NOTE_F3 equ 12
NOTE_Gb3 equ 13
NOTE_G3 equ 14
NOTE_Ab3 equ 15
NOTE_A3 equ 16
NOTE_Bb3 equ 17
NOTE_B3 equ 18
NOTE_C4 equ 19
NOTE_Db4 equ 20
NOTE_D4 equ 21
NOTE_Eb4 equ 22
NOTE_E4 equ 23
NOTE_F4 equ 24
NOTE_Gb4 equ 25
NOTE_G4 equ 26
NOTE_Ab4 equ 27
NOTE_A4 equ 28
NOTE_Bb4 equ 29
NOTE_B4 equ 30
NOTE_C5 equ 31
NOTE_Db5 equ 32
NOTE_D5 equ 33
NOTE_Eb5 equ 34
NOTE_E5 equ 35
NOTE_F5 equ 36
NOTE_Gb5 equ 37
NOTE_G5 equ 38
NOTE_Ab5 equ 39
NOTE_A5 equ 40
NOTE_Bb5 equ 41
NOTE_B5 equ 42
NOTE_C6 equ 43
NOTE_Db6 equ 44
NOTE_D6 equ 45
NOTE_Eb6 equ 46
NOTE_E6 equ 47
NOTE_F6 equ 48
NOTE_Gb6 equ 49
; melody_note takes a NOTE_* constant, encoded as semitones above F2.
; melody_duration takes timer ticks, a multiple of four; zero retriggers next tick.
; Duration persists across notes. Each note restarts sound effect 3.
; melody_end stops the sequencer; it does not itself silence the speaker.
%macro melody_duration 1
    %if %1 < 0 || %1 > 216 || (%1 % 4) != 0
        %error melody duration must be a multiple of four in 0..216
    %endif
    db 0xc8 + (%1 / 4)
%endmacro
%macro melody_note 1
    %if %1 < 0 || %1 > 49
        %error melody semitone must be in 0..49
    %endif
    db %1 * 4
%endmacro
%macro melody_end 0
    db 0xff
%endmacro

; Status tiles: each row literal has eight hex digits, one palette index per pixel.
; Pixels run left to right; rows run top to bottom. Zero is background.
; EGA emits four plane bytes per row; packed emits four pairs of 4-bit pixels.
; CGA emits two bytes per row (2 bits/pixel); its colors are drawn separately.
STATUS_TILE_BLANK equ 0
STATUS_TILE_ROBOT equ 1
STATUS_TILE_TREE_LEFT equ 2
STATUS_TILE_TREE_RIGHT equ 3
STATUS_TILE_BOULDER_LEFT equ 4
STATUS_TILE_BOULDER_RIGHT equ 5
STATUS_TILE_GOLDEN_ROBOT equ 6
STATUS_TILE_FRAME_LEFT equ 7
STATUS_TILE_FRAME_MIDDLE equ 8
STATUS_TILE_FRAME_RIGHT equ 9
%macro status_row_packed 1
    db (%1 >> 24) & 0xff, (%1 >> 16) & 0xff, (%1 >> 8) & 0xff, %1 & 0xff
%endmacro
%macro status_row_ega 1
    %assign %%plane 0
    %rep 4
        %assign %%byte 0
        %assign %%shift 28
        %rep 8
            %assign %%byte (%%byte << 1) | ((%1 >> (%%shift + %%plane)) & 1)
            %assign %%shift %%shift - 4
        %endrep
        db %%byte
        %assign %%plane %%plane + 1
    %endrep
%endmacro
%macro status_row_cga 1
    %assign %%word 0
    %assign %%shift 28
    %rep 8
        %assign %%pixel (%1 >> %%shift) & 0xf
        %if %%pixel > 3
            %error CGA pixel must be in 0..3
        %endif
        %assign %%word (%%word << 2) | %%pixel
        %assign %%shift %%shift - 4
    %endrep
    db %%word >> 8, %%word & 0xff
%endmacro

; VGA DAC colors are red, green, blue triplets; each channel is 0..63.
; These are hardware channel values, not 8-bit RGB components.
%macro dac_rgb6 3
    %if %1 < 0 || %1 > 63 || %2 < 0 || %2 > 63 || %3 < 0 || %3 > 63
        %error VGA DAC channels must be in 0..63
    %endif
    db %1, %2, %3
%endmacro

%ifdef EXEPACK_SUPPORT
; This branch emits a 512-byte packed MZ header template and the runtime stub.
; Header fields are recomputed by the packer; reserved bytes are preserved.
    dw 0x5a4d ; magic
    dw 0x016f ; last page bytes
    dw 0x0053 ; pages
    dw 0x0000 ; relocation count
    dw 0x0020 ; header paragraphs
    dw 0x0434 ; minimum allocation
    dw 0xffff ; maximum allocation
    dw 0x0dfc ; initial SS
    dw 0x0080 ; initial SP
    dw 0x0000 ; checksum
    dw 0x0012 ; initial IP
    dw 0x0a1f ; initial CS
    dw 0x001e ; relocation table offset
    dw 0x0000 ; overlay number
    db 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; remaining original MZ header / padding
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00

; EXEPACK decompressor.
    mov ax,es
    add ax,strict word 0x10
    push cs
    pop ds
    mov [0x4],ax
    add ax,[0xc]
    mov es,ax
    mov cx,[0x6]
    mov_load di, cx
    dec di
    mov_load si, di
    std
    rep movsb
    mov dx,[0xe]
    push ax
    mov ax,0x38
    push ax
    retf
    mov bx,es
    mov ax,ds
    sub_load ax, dx
    mov ds,ax
    mov es,ax
    mov di,0xf
    mov cx,0x10
    mov al,0xff
    repe scasb
    inc di
    mov_load si, di
    mov_load ax, bx
    sub_load ax, dx
    mov es,ax
    mov di,0xf
    mov cl,0x4
    mov_load ax, si
    not ax
    shr ax,cl
    jz short $ +11
    mov dx,ds
    sub_load dx, ax
    mov ds,dx
    or si,0xfffffffffffffff0
    mov_load ax, di
    not ax
    shr ax,cl
    jz short $ +11
    mov dx,es
    sub_load dx, ax
    mov es,dx
    or di,0xfffffffffffffff0
    lodsb
    mov_load dl, al
    dec si
    lodsw
    mov_load cx, ax
    inc si
    mov_load al, dl
    and al,0xfe
    cmp al,0xb0
    jnz short $ +8
    lodsb
    rep stosb
    jmp short $ +9
    nop
    cmp al,0xb2
    jnz short $ +109
    rep movsb
    mov_load al, dl
    test al,0x1
    jz short $ -68
    mov si,0x12d
    push cs
    pop ds
    mov bx,[0x4]
    cld
    xor_load dx, dx
    lodsw
    mov_load cx, ax
    jcxz $ +21
    mov_load ax, dx
    add_load ax, bx
    mov es,ax
    lodsw
    mov_load di, ax
    cmp di,0xffffffffffffffff
    jz short $ +19
    add [es:di],bx
    loop $ -11
    cmp dx,0xf000
    jz short $ +24
    add dx,0x1000
    jmp short $ -34
    mov ax,es
    inc ax
    mov es,ax
    sub di,0x10
    add [es:di],bx
    dec ax
    mov es,ax
    jmp short $ -28
    mov_load ax, bx
    mov di,[0x8]
    mov si,[0xa]
    add_load si, ax
    add [0x2],ax
    sub ax,strict word 0x10
    mov ds,ax
    mov es,ax
    mov bx,0x0
    cli
    mov ss,si
    mov_load sp, di
    sti
    jmp word far [cs:bx]
    mov ah,0x40
    mov bx,0x2
    mov cx,0x16
    mov dx,cs
    mov ds,dx
    mov dx,0x117
    int byte 0x21
    mov ax,0x4cff
    int byte 0x21
    db 'Packed file is corrupt'

%else

; Expanded DOS MZ header.
    dw 0x5a4d ; magic
    dw (file_end-$$) % 512 ; last page bytes
    dw (file_end-$$+511) / 512 ; pages
    dw 0x0019 ; relocation count
    dw 0x0020 ; header paragraphs
    dw 0x0087 ; minimum allocation (adjusted for expanded load image)
    dw 0xffff ; maximum allocation
    dw 0x0dea ; initial SS
    dw 0x0800 ; initial SP
    dw 0x0000 ; checksum
    dw crt_entry-program_image ; initial IP
    dw 0x0000 ; initial CS
    dw 0x001c ; relocation table offset
    dw 0x0000 ; overlay number

; DOS relocation entries (offset, segment), followed by header padding.
    dw 0x49d8, 0x0000
    dw 0x4a51, 0x0000
    dw 0x4ac4, 0x0000
    dw 0x4ad4, 0x0000
    dw 0x4aed, 0x0000
    dw 0x4b2c, 0x0000
    dw 0x4bb8, 0x0000
    dw 0x4bc9, 0x0000
    dw 0x52a4, 0x0000
    dw 0x535c, 0x0000
    dw 0x5d29, 0x0000
    dw 0x5ee0, 0x0000
    dw 0x5f07, 0x0000
    dw 0x60b0, 0x0000
    dw 0x60cf, 0x0000
    dw 0x626b, 0x0000
    dw 0x62d5, 0x0000
    dw 0x6f5a, 0x0000
    dw 0x6fa3, 0x0000
    dw 0x6fe2, 0x0000
    dw 0x7024, 0x0000
    dw 0xdb48, 0x0000
    dw 0x73bd, 0x0000
    dw 0x7448, 0x0000
    dw 0xdbdc, 0x0000
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; remaining expanded MZ header
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

; Entire expanded load image; no EXEPACK records are present in this branch.
program_image:
    times 16 db 0x00 ; g data +0x0000

; AL: melody-stream offset (zero selects the five-note startup intro). Timer ticks advance sound_melody_cursor until FFh. [load+0x0010]
sound_start_melody:
    mov [sound_melody_cursor],al
    mov byte [sound_sequence_mode],0x3
    mov byte [melody_effect_flags],0x81
    ret

; Consume melody bytecode: C8h..FEh change duration; lower bytes trigger effect 3; FFh marks completion. [load+0x001e]
sound_advance_melody:
    mov bl,[sound_melody_cursor]
    or_load bl, bl
    js short $ +60
    inc byte [sound_melody_cursor]
    mov al,[bx+intro_theme_stream]
    cmp al,0xff
    jz short $ +45
    cmp al,0xc8
    jc short $ +15
    sub al,0xc8
    mov_load cl, al
    shl al,1
    shl al,1
    mov [sound_melody_duration],al
    jmp short sound_advance_melody
    mov [melody_effect_pitch],al
    mov al,[sound_melody_duration]
    mov [sound_trigger_countdown],al
    mov al,[melody_effect_flags]
    and al,0x3
    xor_load al, al
    or al,0x80
    mov [melody_effect_flags],al
    mov al,0x3
    jmp near sound_start_effect
    mov [sound_melody_cursor],al
    ret

; Clear all three software voice slots and disable speaker output. [load+0x0061]
speaker_silence_all:
    mov bx,0x2
    call near speaker_silence_voice
    dec bl
    jns short $ -5
    ret

; Reviewed instruction range 006Ch..006Dh; internal routine roles remain partial. [load+0x006c]
recovered_code_006c:
    xor_load bx, bx

; BL: voice index. Clear its active flag; disconnect speaker data. [load+0x006e]
speaker_silence_voice:
    mov_load al, bl
    and al,0x3
    mov cl,0x5
    shl al,cl
    xor_load ah, ah
    mov_load si, ax
    mov [si+sound_voice_slots],ah
    cli
    in al,byte PPI_PORT_B
    and al,0xfc
    or al,0x1
    jmp short $ +2
    out byte PPI_PORT_B,al
    sti
    ret

; Tick-driven sound-trigger logic; effect-specific game state remains unnamed. [load+0x008b]
update_sound_triggers:
    test byte [sound_trigger_countdown],0xff
    jnz short $ +65
    mov al,[sound_sequence_mode]
    cmp al,0x4
    jz short $ +62
    cmp al,0x3
    jz short $ +55
    cmp al,0x6
    jnz short $ +50
    mov dl,[sound_ramp_index]
    cmp dl,0xaa
    jnc short $ +41
    mov al,0x6
    call near sound_start_effect
    call near prng_next_byte
    and al,0x3
    inc al
    mov [sound_trigger_countdown],al
    inc byte [sound_ramp_index]
    mov al,[sound_ramp_index]
    shr al,1
    shr al,1
    inc al
    cmp al,0x20
    jc short $ +4
    mov al,0x1f
    or al,0x32
    mov [0x104],al
    ret
    jmp near sound_advance_melody
    mov byte [sound_trigger_countdown],0x36
    mov al,0x4
    call near sound_start_effect
    ret

; AL: effect index. Copy a 16-byte effect definition into a 32-byte voice slot. [load+0x00e0]
sound_start_effect:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    cbw
    mov_load si, ax
    shl si,1
    shl si,1
    shl si,1
    shl si,1
    lea si,[si+sound_effect_definitions]
    mov al,[si]
    mov_load bl, al
    and bx,0x3
    mov_load bp, bx
    mov cl,0x5
    shl bx,cl
    lea bx,[bx+sound_voice_slots]
    mov ah,[si+SOUND_LEVEL]
    xor_load al, al
    mov [bx],ax
    mov ax,[si+SOUND_PITCH]
    mov [bx+SOUND_PITCH],ax
    mov ax,[si+SOUND_ENVELOPE_STEP]
    mov [bx+SOUND_ENVELOPE_STEP],ax
    mov ax,[si+SOUND_ENVELOPE_NEXT_STEP]
    mov [bx+SOUND_ENVELOPE_NEXT_STEP],ax
    mov ax,[si+SOUND_PITCH_STEP_1]
    mov [bx+SOUND_PITCH_STEP_1],ax
    mov ax,[si+SOUND_PITCH_PERIOD]
    mov [bx+SOUND_PITCH_PERIOD],ax
    mov ax,[si+SOUND_PITCH_STEP_2]
    mov [bx+SOUND_PITCH_STEP_2],ax
    mov ax,[si+SOUND_PITCH_STEP_3]
    mov [bx+SOUND_PITCH_STEP_3],ax
    mov al,[bx+SOUND_PITCH_STEP_1]
    mov [bx+SOUND_CURRENT_PITCH_STEP],al
    mov al,[bx+SOUND_PITCH_STEPS_1]
    mov [bx+SOUND_PITCH_STEPS_LEFT],al
    mov al,[bx+SOUND_ENVELOPE_PERIOD]
    mov [bx+SOUND_ENVELOPE_COUNTDOWN],al
    mov al,[bx+SOUND_PITCH_PERIOD]
    mov [bx+SOUND_PITCH_COUNTDOWN],al
    mov byte [bx+SOUND_PITCH_PHASE],0x0
    test byte [si],0x4
    jz short $ +27
    mov ch,[si+SOUND_PITCH]
    not ch
    cli
    mov al,PIT_SPEAKER_SQUARE_WAVE
    out byte PIT_COMMAND,al
    jmp short $ +2
    mov_load ax, cx
    out byte PIT_SPEAKER_DATA,al
    jmp short $ +2
    xchg al,ah
    out byte PIT_SPEAKER_DATA,al
    sti
    jmp short $ +23
    mov al,[bx+SOUND_PITCH]
    mov cl,[si]
    shr cl,1
    shr cl,1
    shr cl,1
    and cl,0x7
    neg cl
    cli
    call near speaker_set_pitch
    sti
    test byte [sound_muted],0xff
    jnz short $ +18
    cli
    in al,byte PPI_PORT_B
    and al,0xfc
    or al,0x3
    jmp short $ +2
    out byte PPI_PORT_B,al
    sti
    mov al,[si]
    mov [bx],al
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Advance three software voices and update PIT channel 2; preserves BX. [load+0x01a6]
sound_update_voices:
    push bx
    mov si,0x40
    lea bx,[si+sound_voice_slots]
    test byte [sound_muted],0xff
    jnz short $ +73
    mov cl,[bx]
    or_load cl, cl
    jnz short $ +5
    jmp near $ +240
    test cl,0x4
    jz short $ +28
    call near prng_next_byte
    mov ch,[bx+SOUND_PITCH]
    not ch
    mov_load cl, al
    mov al,PIT_SPEAKER_SQUARE_WAVE
    out byte PIT_COMMAND,al
    jmp short $ +2
    mov_load ax, cx
    out byte PIT_SPEAKER_DATA,al
    jmp short $ +2
    xchg al,ah
    out byte PIT_SPEAKER_DATA,al
    cmp byte [bx+SOUND_ENVELOPE_COUNTDOWN],0x2
    jc short $ +7
    dec byte [bx+SOUND_ENVELOPE_COUNTDOWN]
    jmp short $ +81
    mov al,[bx+SOUND_ENVELOPE_PERIOD]
    mov [bx+SOUND_ENVELOPE_COUNTDOWN],al
    test byte [bx+SOUND_ENVELOPE_STEPS],0xff
    jnz short $ +45
    mov al,[bx+SOUND_ENVELOPE_STEP]
    cmp al,[bx+SOUND_ENVELOPE_NEXT_STEP]
    jnz short $ +16
    mov byte [bx],0x0
    in al,byte PPI_PORT_B
    and al,0xfc
    or al,0x1
    out byte PPI_PORT_B,al
    jmp near $ +164
    mov al,[bx+SOUND_ENVELOPE_NEXT_STEP]
    mov [bx+SOUND_ENVELOPE_STEP],al
    mov al,[bx+SOUND_ENVELOPE_NEXT_STEPS]
    mov [bx+SOUND_ENVELOPE_STEPS],al
    mov al,[bx+SOUND_ENVELOPE_NEXT_PERIOD]
    mov [bx+SOUND_ENVELOPE_COUNTDOWN],al
    mov [bx+SOUND_ENVELOPE_PERIOD],al
    dec byte [bx+SOUND_ENVELOPE_STEPS]
    mov al,[bx+SOUND_LEVEL]
    add al,[bx+SOUND_ENVELOPE_STEP]
    jns short $ +6
    xor_load ax, ax
    jmp short $ +8
    cmp al,0x10
    jc short $ +4
    mov al,0xf
    mov [bx+SOUND_LEVEL],al
    test byte [bx+SOUND_PITCH_COUNTDOWN],0xff
    js short $ +112
    cmp byte [bx+SOUND_PITCH_COUNTDOWN],0x2
    jc short $ +7
    dec byte [bx+SOUND_PITCH_COUNTDOWN]
    jmp short $ +101
    mov al,[bx+SOUND_PITCH_PERIOD]
    mov [bx+SOUND_PITCH_COUNTDOWN],al
    test byte [bx+SOUND_PITCH_STEPS_LEFT],0xff
    jnz short $ +61
    inc byte [bx+SOUND_PITCH_PHASE]
    cmp byte [bx+SOUND_PITCH_PHASE],0x3
    jnc short $ +36
    cmp byte [bx+SOUND_PITCH_PHASE],0x1
    jz short $ +16
    mov al,[bx+SOUND_PITCH_STEP_3]
    mov [bx+SOUND_CURRENT_PITCH_STEP],al
    mov al,[bx+SOUND_PITCH_STEPS_3]
    mov [bx+SOUND_PITCH_STEPS_LEFT],al
    jmp short $ -33
    mov al,[bx+SOUND_PITCH_STEP_2]
    mov [bx+SOUND_CURRENT_PITCH_STEP],al
    mov al,[bx+SOUND_PITCH_STEPS_2]
    mov [bx+SOUND_PITCH_STEPS_LEFT],al
    jmp short $ -47
    mov byte [bx+SOUND_PITCH_PHASE],0x0
    mov al,[bx+SOUND_PITCH_STEP_1]
    mov [bx+SOUND_CURRENT_PITCH_STEP],al
    mov al,[bx+SOUND_PITCH_STEPS_1]
    mov [bx+SOUND_PITCH_STEPS_LEFT],al
    dec byte [bx+SOUND_PITCH_STEPS_LEFT]
    mov al,[bx+SOUND_CURRENT_PITCH_STEP]
    add [bx+SOUND_PITCH],al
    mov al,[bx+SOUND_PITCH]
    mov cl,[bx]
    shr cl,1
    shr cl,1
    shr cl,1
    and cl,0x7
    neg cl
    call near speaker_set_pitch
    sub si,0x20
    js short $ +5
    jmp near $ -262
    pop bx
    ret

; AL: pitch index; CL: signed octave adjustment. Look up divisor and program channel 2. [load+0x02b5]
speaker_set_pitch:
    push bx
    cmp al,0x30
    jc short $ +8
    sub al,0x30
    inc cl
    jmp short $ -8
    shl al,1
    xor_load ah, ah
    mov_load bx, ax
    mov ax,[word bx+speaker_pitch_divisors]
    test cl,cl
    jz short $ +18
    js short $ +10
    shr ax,1
    dec cl
    jnz short $ -4
    jmp short $ +8
    shl ax,1
    inc cl
    jnz short $ -4
    mov_load cx, ax
    mov al,PIT_SPEAKER_SQUARE_WAVE
    out byte PIT_COMMAND,al
    jmp short $ +2
    mov_load ax, cx
    out byte PIT_SPEAKER_DATA,al
    jmp short $ +2
    xchg al,ah
    out byte PIT_SPEAKER_DATA,al
    pop bx
    ret

; Fill clipped left/right span buffers in the CGA work buffer; packed color nibbles supply the dither colors. [load+0x02f2]
cga_fill_polygon_spans:
    mov bl,[polygon_y_max]
    mov_load cl, bl
    mov ch,[polygon_y_min]
    add_load bl, ch
    rcr bl,1
    xor_load bh, bh
    shl bx,1
    mov ax,[bx+polygon_right_spans]
    sub ax,[bx+polygon_left_spans]
    jns short cga_fill_polygon_spans__L0311
    jmp near cga_fill_polygon_spans__L0460

cga_fill_polygon_spans__L0311:
    mov ax,[video_target_base]
    mov es,ax
    mov [raster_row_cursor],cl
    mov al,0xf7
    sub_load al, cl
    xor_load ah, ah
    mov_load di, ax
    mov al,[polygon_packed_colors]
    mov_load bl, al
    and al,0xf
    cbw
    shl ax,1
    mov [raster_color_index_a],ax
    test byte [model_dither_flags],0x80
    jnz short cga_fill_polygon_spans__L0343
    shr bl,1
    shr bl,1
    shr bl,1
    shr bl,1
    mov_load al, bl
    cbw
    shl ax,1

cga_fill_polygon_spans__L0343:
    mov [raster_color_index_b],ax
    cmp_load cl, ch
    jnc short cga_fill_polygon_spans__L034d
    jmp near cga_fill_polygon_spans__L0460

cga_fill_polygon_spans__L034d:
    xor_load ch, ch
    mov_load si, cx
    shl si,1

cga_fill_polygon_spans__L0353:
    mov cx,[si+polygon_right_spans]
    mov ax,[si+polygon_left_spans]
    cmp_load cx, ax
    jns short cga_fill_polygon_spans__L0362
    jmp near cga_fill_polygon_spans__L0460

cga_fill_polygon_spans__L0362:
    push di
    shl di,1
    mov di,[di+video_scanline_offsets]
    add di,[raster_x_word_offset]
    mov bp,[raster_color_index_a]
    mov bx,[raster_color_index_b]
    test di,0x2000
    jz short cga_fill_polygon_spans__L037d
    inc bp
    inc bx

cga_fill_polygon_spans__L037d:
    mov dl,[ds:bp+cga_dither_patterns_b]
    mov dh,[bx+cga_dither_patterns_a]
    cmp ax,[viewport_x_end]
    js short cga_fill_polygon_spans__L038f
    jmp near cga_fill_polygon_spans__L044c

cga_fill_polygon_spans__L038f:
    sub ax,[viewport_x_min]
    js short cga_fill_polygon_spans__L03a1
    mov_load bp, ax
    and bp,0x3
    mov bl,[ds:bp+cga_left_pixel_masks]
    jmp short cga_fill_polygon_spans__L03a5

cga_fill_polygon_spans__L03a1:
    xor_load bl, bl
    xor_load ax, ax

cga_fill_polygon_spans__L03a5:
    sub cx,[viewport_x_min]
    jns short cga_fill_polygon_spans__L03ae
    jmp near cga_fill_polygon_spans__L044c

cga_fill_polygon_spans__L03ae:
    cmp cx,[viewport_pixel_width]
    jns short cga_fill_polygon_spans__L03c0
    mov_load bp, cx
    and bp,0x3
    mov bh,[ds:bp+cga_right_pixel_masks]
    jmp short cga_fill_polygon_spans__L03c6

cga_fill_polygon_spans__L03c0:
    xor_load bh, bh
    mov cx,[viewport_pixel_width]

cga_fill_polygon_spans__L03c6:
    shr cx,1
    shr cx,1
    shr ax,1
    shr ax,1
    add_load di, ax
    sub_load cx, ax
    jnz short cga_fill_polygon_spans__L0402
    or_load bl, bl
    jz short cga_fill_polygon_spans__L042c
    or_load bh, bh
    jz short cga_fill_polygon_spans__L0406
    mov_load al, bl
    and_load al, bh
    mov_load ah, dh
    and_load ah, al
    not al
    and [es:di],al
    shr bl,1
    shr bl,1
    shl bh,1
    shl bh,1
    mov_load al, bl
    and_load al, bh
    and_load dl, al
    not al
    and_load ah, al
    or_load ah, dl
    or [es:di],ah
    jmp short cga_fill_polygon_spans__L044c

cga_fill_polygon_spans__L0402:
    or_load bl, bl
    jz short cga_fill_polygon_spans__L0428

cga_fill_polygon_spans__L0406:
    mov_load al, bl
    mov_load ah, dh
    and_load ah, al
    not al
    and [es:di],al
    shr bl,1
    shr bl,1
    mov_load al, dl
    and_load al, bl
    not bl
    and_load ah, bl
    or_load ah, al
    or [es:di],ah
    jcxz cga_fill_polygon_spans__L044c
    inc di
    dec cx
    jcxz cga_fill_polygon_spans__L042c

cga_fill_polygon_spans__L0428:
    mov_load al, dl
    rep stosb

cga_fill_polygon_spans__L042c:
    or_load bh, bh
    jz short cga_fill_polygon_spans__L044c
    mov_load al, bh
    mov_load ah, dh
    and_load ah, al
    not al
    and [es:di],al
    shl bh,1
    shl bh,1
    mov_load al, dl
    and_load al, bh
    not bh
    and_load ah, bh
    or_load ah, al
    or [es:di],ah

cga_fill_polygon_spans__L044c:
    pop di
    mov al,[raster_row_cursor]
    cmp al,[polygon_y_min]
    jz short cga_fill_polygon_spans__L0460
    inc di
    dec byte [raster_row_cursor]
    dec si
    dec si
    jmp near cga_fill_polygon_spans__L0353

cga_fill_polygon_spans__L0460:
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Fill polygon spans using the four-bank scanline layout installed by the Tandy backend. [load+0x0469]
tandy_fill_polygon_spans:
    mov bl,[polygon_y_max]
    mov_load cl, bl
    mov dl,[polygon_y_min]
    add_load bl, dl
    rcr bl,1
    xor_load bh, bh
    shl bx,1
    mov ax,[bx+polygon_right_spans]
    sub ax,[bx+polygon_left_spans]
    jns short tandy_fill_polygon_spans__L0488
    jmp near tandy_fill_polygon_spans__L05c5

tandy_fill_polygon_spans__L0488:
    mov [raster_row_cursor],cl
    mov al,0xf7
    sub_load al, cl
    xor_load ah, ah
    mov_load di, ax
    mov al,[polygon_packed_colors]
    mov_load bl, al
    and al,0xf
    cbw
    mov [raster_color_index_a],ax
    test byte [model_dither_flags],0x80
    jnz short tandy_fill_polygon_spans__L04b1
    shr bl,1
    shr bl,1
    shr bl,1
    shr bl,1
    mov_load al, bl
    cbw

tandy_fill_polygon_spans__L04b1:
    mov [raster_color_index_b],ax
    cmp_load cl, dl
    jnc short tandy_fill_polygon_spans__L04bb
    jmp near tandy_fill_polygon_spans__L05c5

tandy_fill_polygon_spans__L04bb:
    xor_load ch, ch
    mov_load si, cx
    shl si,1

tandy_fill_polygon_spans__L04c1:
    mov cx,[si+polygon_right_spans]
    mov ax,[si+polygon_left_spans]
    cmp_load cx, ax
    jns short tandy_fill_polygon_spans__L04d0
    jmp near tandy_fill_polygon_spans__L05c5

tandy_fill_polygon_spans__L04d0:
    push si
    push di
    mov ax,[video_target_base]
    mov es,ax
    shl di,1
    mov di,[di+video_scanline_offsets]
    mov ax,[raster_x_word_offset]
    shl ax,1
    add_load di, ax
    mov bl,[raster_color_index_a]
    mov bh,[raster_color_index_b]
    mov_load ax, bx
    shl ax,1
    shl ax,1
    shl ax,1
    shl ax,1
    or_load bx, ax
    cmp ax,[viewport_x_end]
    js short tandy_fill_polygon_spans__L0501
    jmp near tandy_fill_polygon_spans__L05a9

tandy_fill_polygon_spans__L0501:
    sub ax,[viewport_x_min]
    js short tandy_fill_polygon_spans__L050e
    mov_load si, ax
    and si,0x1
    jmp short tandy_fill_polygon_spans__L0513

tandy_fill_polygon_spans__L050e:
    mov si,0xffff
    xor_load ax, ax

tandy_fill_polygon_spans__L0513:
    sub cx,[viewport_x_min]
    jns short tandy_fill_polygon_spans__L051c
    jmp near tandy_fill_polygon_spans__L05a9

tandy_fill_polygon_spans__L051c:
    cmp cx,[viewport_pixel_width]
    jns short tandy_fill_polygon_spans__L0529
    mov_load bp, cx
    and bp,0x1
    jmp short tandy_fill_polygon_spans__L0530

tandy_fill_polygon_spans__L0529:
    mov bp,0xffff
    mov cx,[viewport_pixel_width]

tandy_fill_polygon_spans__L0530:
    shr cx,1
    shr ax,1
    add_load di, ax
    sub_load cx, ax
    jnz short tandy_fill_polygon_spans__L055d
    or_load si, si
    js short tandy_fill_polygon_spans__L058b
    or_load bp, bp
    js short tandy_fill_polygon_spans__L0561
    mov ah,[si+tandy_edge_pixel_masks]
    and ah,[ds:bp+tandy_right_pixel_masks]
    mov_load al, ah
    not al
    mov dl,[es:di]
    and_load dl, al
    and_load ah, bh
    or_load dl, ah
    mov [es:di],dl
    jmp short tandy_fill_polygon_spans__L05a9

tandy_fill_polygon_spans__L055d:
    or_load si, si
    js short tandy_fill_polygon_spans__L0583

tandy_fill_polygon_spans__L0561:
    mov ah,[si+tandy_edge_pixel_masks]
    mov_load al, ah
    not al
    mov dl,[es:di]
    and_load dl, al
    and_load ah, bh
    or_load dl, ah
    mov ah,[si+tandy_left_secondary_masks]
    and_load ah, bl
    or_load dl, ah
    mov [es:di],dl
    jcxz tandy_fill_polygon_spans__L05a9
    inc di
    dec cx
    jcxz tandy_fill_polygon_spans__L0587

tandy_fill_polygon_spans__L0583:
    mov_load al, bl
    rep stosb

tandy_fill_polygon_spans__L0587:
    or_load bp, bp
    js short tandy_fill_polygon_spans__L05a9

tandy_fill_polygon_spans__L058b:
    mov ah,[ds:bp+tandy_right_pixel_masks]
    mov_load al, ah
    not al
    mov dl,[es:di]
    and_load dl, al
    and_load ah, bh
    or_load dl, ah
    mov ah,[ds:bp+tandy_right_secondary_masks]
    and_load ah, bl
    or_load dl, ah
    mov [es:di],dl

tandy_fill_polygon_spans__L05a9:
    mov ax,ds
    mov es,ax
    pop di
    pop si
    add sp,0x4
    mov al,[raster_row_cursor]
    cmp al,[polygon_y_min]
    jz short tandy_fill_polygon_spans__L05c5
    inc di
    dec byte [raster_row_cursor]
    dec si
    dec si
    jmp near tandy_fill_polygon_spans__L04c1

tandy_fill_polygon_spans__L05c5:
    xor_load bx, bx
    mov_load dx, bx
    ret

; Fill clipped spans using EGA GC bit masks/write modes; ega_polygon_busy prevents concurrent scattered-pixel writes. [load+0x05ca]
ega_fill_polygon_spans:
    mov bl,[polygon_y_max]
    mov_load cl, bl
    mov ch,[polygon_y_min]
    add_load bl, ch
    rcr bl,1
    xor_load bh, bh
    shl bx,1
    mov ax,[bx+polygon_right_spans]
    sub ax,[bx+polygon_left_spans]
    jns short ega_fill_polygon_spans__L05e9
    jmp near ega_fill_polygon_spans__L0715

ega_fill_polygon_spans__L05e9:
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov byte [ega_polygon_busy],0xff
    mov dx,EGA_GC_INDEX
    mov ax,0xa05
    out dx,ax
    mov [raster_row_cursor],cl
    mov al,0xf7
    sub_load al, cl
    xor_load ah, ah
    mov_load di, ax
    mov bl,[polygon_packed_colors]
    mov_load al, bl
    and bl,0xf
    mov_load bh, bl
    test byte [model_dither_flags],0x80
    jnz short ega_fill_polygon_spans__L0622
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    mov_load bh, al

ega_fill_polygon_spans__L0622:
    cmp_load cl, ch
    jnc short ega_fill_polygon_spans__L0629
    jmp near ega_fill_polygon_spans__L0715

ega_fill_polygon_spans__L0629:
    xor_load ch, ch
    mov_load si, cx
    shl si,1

ega_fill_polygon_spans__L062f:
    mov bp,[si+polygon_right_spans]
    mov cx,[si+polygon_left_spans]
    cmp_load bp, cx
    jns short ega_fill_polygon_spans__L063e
    jmp near ega_fill_polygon_spans__L0715

ega_fill_polygon_spans__L063e:
    push si
    push di
    mov_load si, cx
    shl di,1
    mov di,[di+video_scanline_offsets]
    add di,[video_page_offset]
    add di,[video_target_base]
    mov ax,[raster_x_word_offset]
    shr ax,1
    add_load di, ax
    cmp si,[viewport_x_end]
    js short ega_fill_polygon_spans__L0660
    jmp near ega_fill_polygon_spans__L0700

ega_fill_polygon_spans__L0660:
    sub si,[viewport_x_min]
    js short ega_fill_polygon_spans__L066d
    mov_load ax, si
    and si,0x7
    jmp short ega_fill_polygon_spans__L0672

ega_fill_polygon_spans__L066d:
    mov si,0xffff
    xor_load ax, ax

ega_fill_polygon_spans__L0672:
    sub bp,[viewport_x_min]
    jns short ega_fill_polygon_spans__L067b
    jmp near ega_fill_polygon_spans__L0700

ega_fill_polygon_spans__L067b:
    cmp bp,[viewport_pixel_width]
    jns short ega_fill_polygon_spans__L0688
    mov_load cx, bp
    and bp,0x7
    jmp short ega_fill_polygon_spans__L068f

ega_fill_polygon_spans__L0688:
    mov bp,0xffff
    mov cx,[viewport_pixel_width]

ega_fill_polygon_spans__L068f:
    shr cx,1
    shr cx,1
    shr cx,1
    shr ax,1
    shr ax,1
    shr ax,1
    add_load di, ax
    sub_load cx, ax
    mov al,0x8
    jnz short ega_fill_polygon_spans__L06c7
    or_load si, si
    js short ega_fill_polygon_spans__L06ee
    or_load bp, bp
    js short ega_fill_polygon_spans__L06cb
    mov ah,[si+ega_span_mask_a]
    or ah,[ds:bp+ega_span_mask_a]
    out dx,ax
    and [es:di],bh
    mov ah,[si+ega_span_mask_b]
    and ah,[ds:bp+ega_span_mask_c]
    out dx,ax
    and [es:di],bl
    jmp short ega_fill_polygon_spans__L0700

ega_fill_polygon_spans__L06c7:
    or_load si, si
    js short ega_fill_polygon_spans__L06e1

ega_fill_polygon_spans__L06cb:
    mov ah,[si+ega_span_mask_a]
    out dx,ax
    and [es:di],bh
    mov ah,[si+ega_span_mask_b]
    out dx,ax
    and [es:di],bl
    jcxz ega_fill_polygon_spans__L0700
    inc di
    dec cx
    jcxz ega_fill_polygon_spans__L06ea

ega_fill_polygon_spans__L06e1:
    mov ah,0xff
    out dx,ax
    xchg al,bl
    rep stosb
    xchg al,bl

ega_fill_polygon_spans__L06ea:
    or_load bp, bp
    js short ega_fill_polygon_spans__L0700

ega_fill_polygon_spans__L06ee:
    mov ah,[ds:bp+ega_span_mask_a]
    out dx,ax
    and [es:di],bh
    mov ah,[ds:bp+ega_span_mask_c]
    out dx,ax
    and [es:di],bl

ega_fill_polygon_spans__L0700:
    pop di
    pop si
    mov al,[raster_row_cursor]
    cmp al,[polygon_y_min]
    jz short ega_fill_polygon_spans__L0715
    inc di
    dec byte [raster_row_cursor]
    dec si
    dec si
    jmp near ega_fill_polygon_spans__L062f

ega_fill_polygon_spans__L0715:
    mov byte [ega_polygon_busy],0x0
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Fill clipped spans in the linear VGA work buffer; clip X, apply packed face colors, restore ES=DS. [load+0x0723]
vga_fill_polygon_spans:
    mov bl,[polygon_y_max]
    mov_load cl, bl
    mov ch,[polygon_y_min]
    add_load bl, ch
    rcr bl,1
    xor_load bh, bh
    shl bx,1
    mov ax,[bx+polygon_right_spans]
    sub ax,[bx+polygon_left_spans]
    jns short vga_fill_polygon_spans__L0742
    jmp near vga_fill_polygon_spans__L07e9

vga_fill_polygon_spans__L0742:
    mov ax,[video_target_base]
    mov es,ax
    mov [raster_row_cursor],cl
    mov al,0xf7
    sub_load al, cl
    xor_load ah, ah
    mov_load di, ax
    mov dl,[polygon_packed_colors]
    mov_load bl, dl
    and dl,0xf
    or dl,[raster_shade_bank]
    mov_load dh, dl
    test byte [model_dither_flags],0x80
    jnz short vga_fill_polygon_spans__L0777
    shr bl,1
    shr bl,1
    shr bl,1
    shr bl,1
    or bl,[raster_shade_bank]
    mov_load dh, bl

vga_fill_polygon_spans__L0777:
    cmp_load cl, ch
    jc short vga_fill_polygon_spans__L07e9
    xor_load ch, ch
    mov_load si, cx
    shl si,1

vga_fill_polygon_spans__L0781:
    mov bp,[si+polygon_right_spans]
    mov bx,[si+polygon_left_spans]
    cmp_load bp, bx
    js short vga_fill_polygon_spans__L07e9
    push di
    shl di,1
    mov di,[di+video_scanline_offsets]
    mov ax,[raster_x_word_offset]
    shl ax,1
    shl ax,1
    add_load di, ax
    cmp bx,[viewport_x_end]
    jns short vga_fill_polygon_spans__L07d6
    sub bx,[viewport_x_min]
    mov_load al, dh
    jns short vga_fill_polygon_spans__L07af
    xor_load bx, bx
    mov_load al, dl

vga_fill_polygon_spans__L07af:
    sub bp,[viewport_x_min]
    js short vga_fill_polygon_spans__L07d6
    mov [es:bx+di],al
    inc bx
    cmp bp,[viewport_pixel_width]
    mov_load al, dh
    js short vga_fill_polygon_spans__L07c7
    mov bp,[viewport_pixel_width]
    jmp short vga_fill_polygon_spans__L07ca

vga_fill_polygon_spans__L07c7:
    mov [es:bp+di],al

vga_fill_polygon_spans__L07ca:
    mov_load cx, bp
    sub_load cx, bx
    jna short vga_fill_polygon_spans__L07d6
    add_load di, bx
    mov_load al, dl
    rep stosb

vga_fill_polygon_spans__L07d6:
    pop di
    mov al,[raster_row_cursor]
    cmp al,[polygon_y_min]
    jz short vga_fill_polygon_spans__L07e9
    inc di
    dec byte [raster_row_cursor]
    dec si
    dec si
    jmp short vga_fill_polygon_spans__L0781

vga_fill_polygon_spans__L07e9:
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Phase alias: DS:043E..3698 is the 12,890-byte resource read buffer before it becomes scene/occlusion workspace. Native reads/clears verified by scripts/check_mixed_data.py.

; BX: file handle; DX: buffered bytes remaining; SI: cursor. Refill DS:043E if empty; return word in AX. [load+0x07f2]
resource_read_word:
    or_load dx, dx
    jnz short $ +23
    push cx
    mov cx,resource_read_buffer_end
    mov dx,resource_read_buffer
    sub_load cx, dx
    and cl,0xfe
    mov_load si, dx
    mov ah,DOS_READ
    int byte DOS_SERVICES
    pop cx
    mov_load dx, ax
    lodsw
    dec dx
    dec dx
    ret

; Read four-plane resource words, form a four-bit source color per pixel, then select its even/odd-row CGA pattern and emit packed two-bit pixels. [load+0x080f]
resource_load_planar_as_cga:
    push bp
    mov_load bp, sp
    sub sp,0xe
    push si
    push di
    push es
    mov ax,[bp+0x4]
    mov es,ax
    xor_load dx, dx
    mov_load di, dx
    mov word [bp-0xc],0xc8
    mov cx,0x14
    mov bx,[bp+0x6]
    call near resource_read_word
    xchg al,ah
    mov [bp-0x2],ax
    call near resource_read_word
    xchg al,ah
    mov [bp-0x4],ax
    call near resource_read_word
    xchg al,ah
    mov [bp-0x6],ax
    call near resource_read_word
    xchg al,ah
    mov [bp-0x8],ax
    push cx
    mov cx,0x2
    mov word [bp-0xe],0x0
    mov word [bp-0xa],0xc000
    push cx
    mov cx,0x8
    shl word [bp-0x8],1
    rcl bx,1
    shl word [bp-0x6],1
    rcl bx,1
    shl word [bp-0x4],1
    rcl bx,1
    shl word [bp-0x2],1
    rcl bx,1
    and bx,0xf
    shl bx,1
    shl bx,1
    mov ax,[bp-0xc]
    and al,0x1
    add_load bx, ax
    add_load bx, ax
    mov ax,[bx+resource_cga_color_patterns]
    and ax,[bp-0xa]
    or [bp-0xe],ax
    shr word [bp-0xa],1
    shr word [bp-0xa],1
    loop $ -52
    mov ax,[bp-0xe]
    xchg al,ah
    stosw
    pop cx
    loop $ -75
    pop cx
    loop $ -117
    dec word [bp-0xc]
    jnz short $ -125
    pop es
    pop di
    pop si
    mov_load sp, bp
    pop bp
    ret
    push bp
    mov_load bp, sp
    push si
    push di
    push es
    mov ax,[bp+0x4]
    mov es,ax
    xor_load dx, dx
    mov_load di, dx
    mov bx,[bp+0x6]
    mov cx,0xfa0
    call near resource_read_word
    mov [es:di],ax
    call near resource_read_word
    mov [es:di+0x1f40],ax
    call near resource_read_word
    mov [es:di+0x3e80],ax
    call near resource_read_word
    mov [es:di+0x5dc0],ax
    inc di
    inc di
    loop $ -32
    pop es
    pop di
    pop si
    pop bp
    ret
    push bp
    mov_load bp, sp
    sub sp,0x8
    push si
    push di
    push es
    mov ax,[bp+0x4]
    mov es,ax
    xor_load dx, dx
    mov_load di, dx
    mov bx,[bp+0x6]
    mov cx,0xfa0
    call near resource_read_word
    xchg al,ah
    mov [bp-0x2],ax
    call near resource_read_word
    xchg al,ah
    mov [bp-0x4],ax
    call near resource_read_word
    xchg al,ah
    mov [bp-0x6],ax
    call near resource_read_word
    xchg al,ah
    mov [bp-0x8],ax
    push cx
    mov cx,0x4
    push cx
    mov cx,0x4
    shl word [bp-0x8],1
    rcl ax,1
    shl word [bp-0x6],1
    rcl ax,1
    shl word [bp-0x4],1
    rcl ax,1
    shl word [bp-0x2],1
    rcl ax,1
    loop $ -20
    xchg al,ah
    stosw
    pop cx
    loop $ -30
    pop cx
    loop $ -69
    pop es
    pop di
    pop si
    mov_load sp, bp
    pop bp
    ret

; Image blitter installed by the CGA backend; copy into the selected interleaved display buffer. [load+0x094e]
cga_blit_image:
    mov es,word [video_target_base]
    push ds
    shl si,1
    mov di,[si+video_scanline_offsets]
    mov_load bx, si
    shl bx,1
    shl bx,1
    shl bx,1
    mov_load si, bx
    shl bx,1
    shl bx,1
    add_load si, bx
    mov dx,0x1fb0
    mov bx,0xe000
    test di,0x2000
    jz short cga_blit_image__L0977
    xchg dx,bx

cga_blit_image__L0977:
    mov ds,ax
    mov_load ax, cx

cga_blit_image__L097b:
    mov_load cx, bp
    rep movsw
    mov cx,0x28
    sub_load cx, bp
    shl cx,1
    add_load di, cx
    add_load si, cx
    add_load di, dx
    xchg dx,bx
    dec ax
    jnz short cga_blit_image__L097b
    pop ds
    mov ax,ds
    mov es,ax
    ret

; Image blitter installed by the Tandy backend; use the four-bank video layout. [load+0x0997]
tandy_blit_image:
    mov bx,0xb000
    mov es,bx
    push ds
    mov_load dx, si
    mov_load bx, dx
    shl bx,1
    mov di,[bx+video_scanline_offsets]
    xchg dh,dl
    mov_load si, dx
    shr dx,1
    shr dx,1
    sub_load si, dx
    shr dx,1
    sub_load si, dx
    mov ds,ax
    mov_load dx, cx
    mov bx,0x8000
    mov ax,0x1f60
    shl bp,1

tandy_blit_image__L09c1:
    mov_load cx, bp
    rep movsw
    mov cx,0x50
    sub_load cx, bp
    shl cx,1
    add_load di, cx
    add_load si, cx
    add_load di, ax
    test bx,di
    jz short tandy_blit_image__L09da
    add di,0x80a0

tandy_blit_image__L09da:
    dec dx
    jnz short tandy_blit_image__L09c1
    pop ds
    mov ax,ds
    mov es,ax
    ret

; Image blitter installed by the EGA backend; transfer the planar image. [load+0x09e3]
ega_blit_image:
    mov bx,0xa000
    mov es,bx
    push ds
    shl si,1
    mov si,[si+video_scanline_offsets]
    mov_load di, si
    mov ds,ax
    mov dx,EGA_GC_INDEX
    mov ax,0x1
    out dx,ax
    mov al,0x5
    out dx,ax
    mov ax,0xff08
    out dx,ax
    mov_load bx, cx
    mov dx,EGA_SEQ_INDEX

ega_blit_image__L0a06:
    mov ax,0x102
    out dx,ax
    push di
    mov_load si, di
    mov_load cx, bp
    rep movsb
    pop di
    mov ah,0x2
    out dx,ax
    push di
    lea si,[di+0x1f40]
    mov_load cx, bp
    rep movsb
    pop di
    mov ah,0x4
    out dx,ax
    push di
    lea si,[di+0x3e80]
    mov_load cx, bp
    rep movsb
    pop di
    mov ah,0x8
    out dx,ax
    lea si,[di+0x5dc0]
    mov_load cx, bp
    rep movsb
    mov cx,0x28
    sub_load cx, bp
    add_load di, cx
    dec bx
    jnz short ega_blit_image__L0a06
    mov ah,0xf
    out dx,ax
    pop ds
    mov ax,ds
    mov es,ax
    ret

; Image blitter installed by the VGA backend; expand packed pixel pairs into the linear framebuffer. [load+0x0a4a]
vga_blit_image:
    mov bx,0xa000
    mov es,bx
    push ds
    shl si,1
    mov di,[si+video_scanline_offsets]
    mov_load si, di
    shr si,1
    mov ds,ax
    mov bl,0xf
    mov dx,0xa0
    shl bp,1
    shl bp,1

vga_blit_image__L0a65:
    push cx
    mov_load cx, bp

vga_blit_image__L0a68:
    lodsb
    mov_load ah, al
    and_load ah, bl
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    stosw
    loop vga_blit_image__L0a68
    mov_load cx, dx
    sub_load cx, bp
    add_load si, cx
    add_load di, cx
    add_load di, cx
    pop cx
    loop vga_blit_image__L0a65
    pop ds
    mov ax,ds
    mov es,ax
    ret

; Build 200 scanline offsets, alternating 8 KiB banks. [load+0x0a8b]
video_build_interleaved_rows:
    push di
    xor_load ax, ax
    mov di,video_scanline_offsets
    mov dx,0x2000
    mov cx,0xc8
    stosw
    add ax,strict word 0x50
    xchg ax,dx
    loop $ -5
    pop di
    ret

; Build 200 scanline offsets across four 8 KiB banks. [load+0x0aa0]
video_build_four_bank_rows:
    push di
    xor_load ax, ax
    mov di,video_scanline_offsets
    mov cx,0xc8
    stosw
    add ax,0x2000
    test ax,0x8000
    jz short $ +5
    add ax,0x80a0
    loop $ -12
    pop di
    ret

; Build 201 scanline offsets with a 40-byte stride; initialize EGA GC. [load+0x0ab9]
video_build_planar_rows:
    push di
    xor_load ax, ax
    mov di,video_scanline_offsets
    mov cx,0xc9
    stosw
    add ax,strict word 0x28
    loop $ -4
    mov dx,EGA_GC_INDEX
    mov ax,0x7
    out dx,ax
    pop di
    ret

; Build scanline offsets for a linear framebuffer. [load+0x0ad1]
video_build_linear_rows:
    push di
    xor_load ax, ax
    mov di,video_scanline_offsets
    mov cx,0xc9
    stosw
    add ax,0x140
    loop $ -4
    pop di
    ret

; CX: width in eight-pixel columns, DX: height in eight-pixel rows. Fill from scanline 8 using the selected scene-style pattern. [load+0x0ae2]
cga_fill_background_band:
    mov di,[video_scanline8_offset]
    mov ax,[video_target_base]
    mov es,ax
    shl dx,1
    shl dx,1
    shl dx,1
    mov bl,[scene_background_style]
    xor_load bh, bh
    shl bx,1
    shl bx,1
    mov ax,[bx+cga_background_pattern_a]
    mov bp,[bx+cga_background_pattern_b]
    mov bx,0x2000
    sub_load bx, cx
    sub_load bx, cx
    mov si,0xe050
    sub_load si, cx
    sub_load si, cx
    test di,0x2000
    jz short cga_fill_background_band__L0b19
    xchg bx,si

cga_fill_background_band__L0b19:
    push cx
    rep stosw
    pop cx
    xchg ax,bp
    add_load di, bx
    xchg bx,si
    dec dx
    jnz short cga_fill_background_band__L0b19
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Fill the scene background band in the four-bank framebuffer layout. [load+0x0b2e]
tandy_fill_background_band:
    mov di,[video_scanline8_offset]
    mov ax,[video_target_base]
    mov es,ax
    shl dx,1
    shl dx,1
    shl dx,1
    mov bl,[scene_background_style]
    xor_load bh, bh
    shl bx,1
    mov ax,[bx+tandy_background_patterns]
    shl cx,1
    mov bx,0x2000
    sub_load bx, cx
    sub_load bx, cx
    mov si,0x80a0

tandy_fill_background_band__L0b55:
    push cx
    rep stosw
    pop cx
    add_load di, bx
    test di,0x8000
    jz short tandy_fill_background_band__L0b63
    add_load di, si

tandy_fill_background_band__L0b63:
    dec dx
    jnz short tandy_fill_background_band__L0b55
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Fill the background band through GC write mode/bit masks, protecting hardware state with ega_polygon_busy. [load+0x0b6f]
ega_fill_background_band:
    mov byte [ega_polygon_busy],0xff
    mov di,[video_scanline8_offset]
    add di,[video_page_offset]
    add di,[video_target_base]
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    shl dx,1
    shl dx,1
    shl dx,1
    mov_load si, dx
    mov dx,EGA_GC_INDEX
    mov ax,0xa05
    out dx,ax
    mov ax,0xff08
    out dx,ax
    mov bl,[scene_background_style]
    xor_load bh, bh
    mov al,[bx+ega_background_patterns]
    mov bx,0x28
    sub_load bx, cx
    mov_load bp, cx

ega_fill_background_band__L0ba9:
    rep stosb
    mov_load cx, bp
    add_load di, bx
    dec si
    jnz short ega_fill_background_band__L0ba9
    mov ax,0x105
    out dx,ax
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    mov [ega_polygon_busy],bl
    ret

; Fill the scene background band in the linear target framebuffer. [load+0x0bc3]
vga_fill_background_band:
    mov di,[video_scanline8_offset]
    mov ax,[video_target_base]
    mov es,ax
    shl dx,1
    shl dx,1
    shl dx,1
    mov bl,[scene_background_style]
    xor_load bh, bh
    shl bx,1
    mov ax,[bx+vga_background_patterns]
    shl cx,1
    shl cx,1
    mov_load bx, cx
    shl bx,1
    sub bx,0x140
    neg bx
    mov_load si, cx

vga_fill_background_band__L0bee:
    rep stosw
    mov_load cx, si
    add_load di, bx
    dec dx
    jnz short vga_fill_background_band__L0bee
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Select segment B800h, reset the page offset and program the CRTC display start. [load+0x0c00]
cga_select_front_buffer:
    mov word [video_target_base],0xb800
    xor_load cx, cx
    mov [video_page_offset],cx
    mov dx,[video_crtc_port]
    mov al,0xc
    out dx,al
    inc dx
    mov_load al, ch
    out dx,al
    dec dx
    mov al,0xd
    out dx,al
    inc dx
    mov_load al, cl
    out dx,al
    xor_load dx, dx
    ret

; Select the backend front-buffer base and reset its CRTC start address. [load+0x0c22]
tandy_select_front_buffer:
    mov word [video_target_base],0xb000
    xor_load cx, cx
    mov [video_page_offset],cx
    mov dx,[video_crtc_port]
    mov al,0xc
    out dx,al
    inc dx
    mov_load al, ch
    out dx,al
    dec dx
    mov al,0xd
    out dx,al
    inc dx
    mov_load al, cl
    out dx,al
    xor_load dx, dx
    ret

; Use zero byte offset in EGA video memory and reset the CRTC display start. [load+0x0c44]
ega_select_front_buffer:
    xor_load cx, cx
    mov [video_target_base],cx
    mov [video_page_offset],cx
    mov dx,[video_crtc_port]
    mov al,0xc
    out dx,al
    inc dx
    mov_load al, ch
    out dx,al
    dec dx
    mov al,0xd
    out dx,al
    inc dx
    mov_load al, cl
    out dx,al
    xor_load dx, dx
    ret

; Select segment A000h and reset the page offset/CRTC start. [load+0x0c64]
vga_select_front_buffer:
    mov word [video_target_base],0xa000
    xor_load cx, cx
    mov [video_page_offset],cx
    mov dx,[video_crtc_port]
    mov al,0xc
    out dx,al
    inc dx
    mov_load al, ch
    out dx,al
    dec dx
    mov al,0xd
    out dx,al
    inc dx
    mov_load al, cl
    out dx,al
    xor_load dx, dx
    ret

; Set the target base to the allocated work-buffer segment (segment-based adapters). [load+0x0c86]
video_select_work_buffer_segment:
    mov ax,[video_work_buffer_segment]
    mov [video_target_base],ax
    ret

; Set the target base to byte offset 8000h in EGA video memory. [load+0x0c8d]
ega_select_work_buffer_offset:
    mov word [video_target_base],0x8000
    ret

; Adapter-specific image blit: CGA, monochrome, packed pixels, or planar EGA.
    push ds
    mov di,[0x3e9]
    mov bp,0x13
    mov ax,[graphics_adapter]
    cmp al,0x2
    jnz short $ +51
    mov ax,VIDEO_SEG_CGA
    mov es,ax
    mov si,0x37a8
    mov ax,[skybox_image_segment]
    mov ds,ax
    mov cx,0xe
    rep movsw
    sub si,0x10
    mov cx,0xe
    rep movsw
    add si,0x28
    mov ax,0x1fc8
    test di,0x2000
    jz short $ +5
    mov ax,0xe018
    add_load di, ax
    dec bp
    jnz short $ -31
    jmp near $ +277
    cmp al,0x3
    jnz short $ +51
    mov ax,VIDEO_SEG_MONO
    mov es,ax
    mov si,0x6f50
    mov ax,[skybox_image_segment]
    mov ds,ax
    mov cx,0x1c
    rep movsw
    sub si,0x20
    mov cx,0x1c
    rep movsw
    add si,0x50
    mov ax,0x1f90
    test di,0x8000
    jz short $ +5
    mov ax,0x8030
    add_load di, ax
    dec bp
    jnz short $ -31
    jmp near $ +224
    cmp al,0x5
    jnz short $ +70
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov si,0x6f50
    mov ax,[skybox_image_segment]
    mov ds,ax
    mov bl,0xf
    mov cx,0x38
    lodsb
    mov_load ah, al
    and_load ah, bl
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    stosw
    loop $ -14
    sub si,0x20
    mov cx,0x38
    lodsb
    mov_load ah, al
    and_load ah, bl
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    stosw
    loop $ -14
    add si,0x50
    add di,0x60
    dec bp
    jnz short $ -48
    jmp near $ +152
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov si,0x1bd4
    mov ax,[skybox_image_segment]
    mov ds,ax
    mov dx,EGA_GC_INDEX
    mov ax,0x1
    out dx,ax
    mov al,0x5
    out dx,ax
    mov ax,0xff08
    out dx,ax
    mov dx,EGA_SEQ_INDEX
    mov ax,0x102
    out dx,ax
    push di
    mov cx,0xe
    rep movsb
    pop di
    mov ah,0x2
    out dx,ax
    push di
    add si,0x1f32
    mov cx,0xe
    rep movsb
    pop di
    mov ah,0x4
    out dx,ax
    push di
    add si,0x1f32
    mov cx,0xe
    rep movsb
    pop di
    mov ah,0x8
    out dx,ax
    add si,0x1f32
    mov cx,0xe
    rep movsb
    sub si,0x5dc8
    mov ax,0x102
    out dx,ax
    push di
    mov cx,0xe
    rep movsb
    pop di
    mov ah,0x2
    out dx,ax
    push di
    add si,0x1f32
    mov cx,0xe
    rep movsb
    pop di
    mov ah,0x4
    out dx,ax
    push di
    add si,0x1f32
    mov cx,0xe
    rep movsb
    pop di
    mov ah,0x8
    out dx,ax
    add si,0x1f32
    mov cx,0xe
    rep movsb
    add si,0xa254
    add di,0xc
    dec bp
    jnz short $ -114
    mov ah,0xf
    out dx,ax
    pop ds
    mov ax,ds
    mov es,ax
    ret

; Reviewed instruction range 0DECh..0EC1h; internal routine roles remain partial. [load+0x0dec]
recovered_code_0dec:
    mov si,0x30
    mov ax,[graphics_adapter]
    cmp al,0x2
    jnz short recovered_code_0dec__L0e34
    mov ax,VIDEO_SEG_CGA
    mov es,ax
    mov ax,[cga_background_pattern_a]
    mov cx,[0x41b]

recovered_code_0dec__L0e02:
    mov_load di, si
    shl di,1
    mov di,[di+video_scanline_offsets]
    add di,[video_page_offset]
    test si,0x1
    jnz short recovered_code_0dec__L0e1a
    and di,0x1fff
    jmp short recovered_code_0dec__L0e22

recovered_code_0dec__L0e1a:
    and di,0x1fff
    or di,0x2000

recovered_code_0dec__L0e22:
    cmp [es:di],ax
    jz short recovered_code_0dec__L0e2a
    jmp near recovered_code_0dec__L0ebc

recovered_code_0dec__L0e2a:
    xchg ax,cx
    inc si
    cmp si,0x94
    jnz short recovered_code_0dec__L0e02
    jmp short recovered_code_0dec__L0e6b

recovered_code_0dec__L0e34:
    cmp al,0x3
    jnz short recovered_code_0dec__L0e44
    mov ax,VIDEO_SEG_MONO
    mov es,ax
    mov ax,[tandy_background_patterns]
    mov_load cx, ax
    jmp short recovered_code_0dec__L0e52

recovered_code_0dec__L0e44:
    cmp al,0x5
    jnz short recovered_code_0dec__L0e71
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov ax,[vga_background_patterns]
    mov_load cx, ax

recovered_code_0dec__L0e52:
    mov_load di, si
    shl di,1
    mov di,[di+video_scanline_offsets]
    add di,[video_page_offset]
    cmp [es:di],ax
    jnz short recovered_code_0dec__L0ebc
    xchg ax,cx
    inc si
    cmp si,0x94
    jnz short recovered_code_0dec__L0e52

recovered_code_0dec__L0e6b:
    mov ax,ds
    mov es,ax
    clc
    ret

recovered_code_0dec__L0e71:
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov byte [0x244],0xff
    mov dx,EGA_GC_INDEX
    mov ax,0xf07
    out dx,ax
    mov al,0x2
    mov ah,[ega_background_patterns]
    out dx,ax
    mov ax,0x805
    out dx,ax

recovered_code_0dec__L0e8d:
    mov_load di, si
    shl di,1
    mov di,[di+video_scanline_offsets]
    add di,[video_page_offset]
    cmp byte [es:di],0xff
    jnz short recovered_code_0dec__L0eb2
    inc si
    cmp si,0x94
    jnz short recovered_code_0dec__L0e8d
    mov ax,0x7
    out dx,ax
    xor_load dx, dx
    mov [0x244],dl
    jmp short recovered_code_0dec__L0e6b

recovered_code_0dec__L0eb2:
    mov ax,0x7
    out dx,ax
    xor_load dx, dx
    mov [0x244],dl

recovered_code_0dec__L0ebc:
    mov ax,ds
    mov es,ax
    stc
    ret

; CGA text-glyph path selected by the CGA font callback; target segment comes from video_target_base.

; CGA font callback: expand glyph bits into two-bit pixel pairs and write into the selected interleaved target buffer. [load+0x0ec2]
cga_draw_font_glyph:
    push bp
    mov_load bp, sp
    sub sp,0x2
    push di
    push si
    mov bx,cga_text_color_pixels
    mov al,[bp+0xa]
    and al,0xf
    xlatb
    mov_load ah, al
    mov [bp+0xa],ax
    mov al,[bp+0xc]
    and al,0xf
    xlatb
    mov_load ah, al
    mov [bp+0xc],ax
    mov bx,[bp+0x8]
    shl bx,1
    shl bx,1
    shl bx,1
    add bx,0x8
    shl bx,1
    mov di,[bx+video_scanline_offsets]
    mov bx,[bp+0x6]
    shl bx,1
    add_load di, bx
    mov es,word [video_target_base]
    mov dx,0x1ffc
    mov word [bp-0x2],0xe04c
    test di,0x2000
    jz short $ +5
    xchg dx,[bp-0x2]
    mov bx,[bp+0x4]
    shl bx,1
    shl bx,1
    shl bx,1
    lea si,[bx+game_font_8x8]
    mov cl,0x8
    mov bl,[bp+0x10]
    or_load bl, bl
    jz short $ +3
    dec si
    mov ch,[bp+0xe]
    lodsb
    push ax
    and ax,0xf0
    shr ax,1
    shr ax,1
    shr ax,1
    mov_load bx, ax
    mov ax,[bx+cga_font_nibble_masks]
    mov_load bx, ax
    and ax,[bp+0xa]
    not bx
    or_load ch, ch
    jnz short $ +7
    and bx,[bp+0xc]
    jmp short $ +5
    and bx,[es:di]
    or_load ax, bx
    stosw
    pop bx
    and bx,0xf
    shl bx,1
    mov ax,[bx+cga_font_nibble_masks]
    mov_load bx, ax
    and ax,[bp+0xa]
    not bx
    or_load ch, ch
    jnz short $ +7
    and bx,[bp+0xc]
    jmp short $ +5
    and bx,[es:di]
    or_load ax, bx
    stosw
    add_load di, dx
    xchg dx,[bp-0x2]
    dec cl
    jnz short $ -78
    mov ax,ds
    mov es,ax
    pop si
    pop di
    mov_load sp, bp
    pop bp
    ret

; Adapter-specific rendering path; do not infer model/terrain semantics yet.
    push bp
    mov_load bp, sp
    push di
    push si
    mov bx,[bp+0x8]
    shl bx,1
    shl bx,1
    shl bx,1
    add bx,0x8
    shl bx,1
    mov di,[bx+video_scanline_offsets]
    mov bx,[bp+0x6]
    shl bx,1
    shl bx,1
    add_load di, bx
    mov ax,VIDEO_SEG_MONO
    mov es,ax
    mov al,[bp+0xa]
    mov ah,[bp+0xc]
    and ax,0xf0f
    mov_load bx, ax
    shr ax,1
    shr ax,1
    shr ax,1
    shr ax,1
    or_load bx, ax
    mov si,[bp+0x4]
    shl si,1
    shl si,1
    shl si,1
    lea si,[si+game_font_8x8]
    mov cx,0x8
    mov dl,[bp+0x10]
    or_load dl, dl
    jz short $ +3
    dec si
    mov dl,[bp+0xe]
    mov bp,0x8000
    push cx
    mov cx,0x8
    lodsb
    mov_load al, bl
    shl ah,1
    jc short $ +11
    or_load dl, dl
    jz short $ +5
    inc di
    jmp short $ +5
    mov_load al, bh
    stosb
    loop $ -17
    add di,0x1ff0
    test bp,di
    jz short $ +6
    add di,0x80a0
    pop cx
    loop $ -36
    mov ax,ds
    mov es,ax
    pop si
    pop di
    pop bp
    ret
    push bp
    mov_load bp, sp
    push di
    push si
    mov bx,[bp+0x8]
    shl bx,1
    shl bx,1
    shl bx,1
    add bx,0x8
    shl bx,1
    mov di,[bx+video_scanline_offsets]
    mov bx,[bp+0x6]
    shl bx,1
    shl bx,1
    shl bx,1
    add_load di, bx
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov bx,[bp+0x4]
    shl bx,1
    shl bx,1
    shl bx,1
    lea si,[bx+game_font_8x8]
    mov bl,[bp+0xa]
    mov bh,[bp+0xc]
    mov ah,0x8
    mov dl,[bp+0x10]
    or_load dl, dl
    jz short $ +3
    dec si
    mov ch,[bp+0xe]
    lodsb
    xor_load dx, dx
    mov cl,0x8
    shr al,1
    rcr dx,1
    sar dx,1
    dec cl
    jnz short $ -8
    mov cl,0x10
    mov_load al, bl
    shl dx,1
    jc short $ +11
    or_load ch, ch
    jz short $ +5
    inc di
    jmp short $ +5
    mov_load al, bh
    stosb
    dec cl
    jnz short $ -18
    add di,0x130
    dec ah
    jnz short $ -43
    mov ax,ds
    mov es,ax
    pop si
    pop di
    pop bp
    ret

; Planar EGA rendering: GC mode and per-pixel bit mask writes.
    push bp
    mov_load bp, sp
    push di
    push si
    mov bx,[bp+0x8]
    shl bx,1
    shl bx,1
    shl bx,1
    add bx,0x8
    shl bx,1
    mov di,[bx+video_scanline_offsets]
    add di,[bp+0x6]
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov bx,[bp+0x4]
    shl bx,1
    shl bx,1
    shl bx,1
    lea si,[bx+game_font_8x8]
    mov cl,0x8
    mov dl,[bp+0x10]
    or_load dl, dl
    jz short $ +3
    dec si
    mov ch,[bp+0xe]
    mov dx,EGA_GC_INDEX
    mov ax,0xa05
    out dx,ax
    mov ax,0x7
    out dx,ax
    mov bl,[bp+0xa]
    mov bh,[bp+0xc]
    lodsb
    xor_load dx, dx
    mov bp,0x8
    shr al,1
    rcr dx,1
    sar dx,1
    dec bp
    jnz short $ -7
    mov_load ax, dx
    mov dx,EGA_GC_INDEX
    push ax
    mov al,0x8
    out dx,ax
    and [es:di],bl
    or_load ch, ch
    jnz short $ +8
    not ah
    out dx,ax
    and [es:di],bh
    pop ax
    mov_load ah, al
    mov al,0x8
    out dx,ax
    inc di
    and [es:di],bl
    or_load ch, ch
    jnz short $ +8
    not ah
    out dx,ax
    and [es:di],bh
    add di,0x27
    dec cl
    jnz short $ -62
    mov ax,ds
    mov es,ax
    pop si
    pop di
    pop bp
    ret

; AX: tile ID, BX: column. Copy one eight-row packed tile into the work strip. [load+0x1119]
cga_draw_status_tile:
    shl bx,1
    mov_load di, bx
    mov bx,[video_work_buffer_segment]
    mov es,bx
    mov_load bx, ax
    shl bx,1
    shl bx,1
    shl bx,1
    shl bx,1
    lea si,[bx+status_tiles_cga]
    mov ax,0x1ffe
    mov bx,0xe04e
    mov cx,0x8

cga_draw_status_tile__L113a:
    movsw
    add_load di, ax
    xchg ax,bx
    loop cga_draw_status_tile__L113a
    ret

; AX: tile ID, BX: column. Copy an eight-row tile into four-bank video memory. [load+0x1141]
tandy_draw_status_tile:
    shl bx,1
    shl bx,1
    mov_load di, bx
    mov bx,0xb000
    mov es,bx
    mov_load bx, ax
    xchg bh,bl
    shr bx,1
    shr bx,1
    shr bx,1
    lea si,[bx+status_tiles_packed]
    mov ax,0x1ffc
    mov bx,0x8000
    mov dx,0x80a0
    mov cx,0x8

tandy_draw_status_tile__L1166:
    movsw
    movsw
    add_load di, ax
    test bx,di
    jz short tandy_draw_status_tile__L1170
    add_load di, dx

tandy_draw_status_tile__L1170:
    loop tandy_draw_status_tile__L1166
    ret

; AX: tile ID, BX: column. Expand packed nibbles into an eight-row linear VGA tile. [load+0x1173]
vga_draw_status_tile:
    shl bx,1
    shl bx,1
    shl bx,1
    mov_load di, bx
    mov es,word [video_work_buffer_segment]
    mov_load bx, ax
    xchg bh,bl
    shr bx,1
    shr bx,1
    shr bx,1
    lea si,[bx+status_tiles_packed]
    mov bx,0x138
    mov dh,0xf
    mov dl,0x8

vga_draw_status_tile__L1194:
    mov cx,0x4

vga_draw_status_tile__L1197:
    lodsb
    mov_load ah, al
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    and_load ah, dh
    stosw
    loop vga_draw_status_tile__L1197
    add_load di, bx
    dec dl
    jnz short vga_draw_status_tile__L1194
    ret

; AX: tile ID, BX: column. Write the tile plane by plane into the off-screen EGA strip. [load+0x11ae]
ega_draw_status_tile:
    mov_load di, bx
    add di,[video_page_offset]
    add di,0x8000
    mov bx,0xa000
    mov es,bx
    mov_load bx, ax
    xchg bh,bl
    shr bx,1
    shr bx,1
    shr bx,1
    lea si,[bx+status_tiles_ega]
    cli
    mov dx,EGA_GC_INDEX
    mov ax,0x1
    out dx,ax
    mov al,0x5
    out dx,ax
    mov ax,0xff08
    out dx,ax
    mov dx,EGA_SEQ_INDEX
    mov cx,0x8

ega_draw_status_tile__L11e0:
    mov ax,0x102
    out dx,ax
    mov bl,[si]
    mov [es:di],bl
    inc si
    shl ah,1
    out dx,ax
    mov bl,[si]
    mov [es:di],bl
    inc si
    shl ah,1
    out dx,ax
    mov bl,[si]
    mov [es:di],bl
    inc si
    shl ah,1
    out dx,ax
    mov bl,[si]
    mov [es:di],bl
    inc si
    add di,0x28
    loop ega_draw_status_tile__L11e0
    mov ah,0xf
    out dx,ax
    sti
    ret

; Stack: BIOS mode word. Cache the CRTC port from BIOS data 0040:0063. [load+0x120f]
bios_set_video_mode:
    push bp
    mov_load bp, sp
    push es
    mov ax,[bp+0x4]
    int byte BIOS_VIDEO
    mov ax,0x40
    mov es,ax
    mov ax,[es:0x63]
    mov [video_crtc_port],ax
    pop es
    pop bp
    ret

; Return the BIOS video mode in AL; AH is cleared. [load+0x1227]
bios_get_video_mode:
    mov ah,0xf
    int byte BIOS_VIDEO
    xor_load ah, ah
    ret

; Stack: filename pointer, open mode. Return handle in AX, or zero on carry/error. [load+0x122e]
dos_open_file:
    push bp
    mov_load bp, sp
    mov dx,[bp+0x4]
    mov ax,[bp+0x6]
    mov ah,DOS_OPEN
    int byte DOS_SERVICES
    jnc short $ +4
    xor_load ax, ax
    pop bp
    ret

; Stack: file handle. DOS close; error result is not checked. [load+0x1241]
; dos_close_file
    push bp
    mov_load bp, sp
    mov bx,[bp+0x4]
    mov ah,DOS_CLOSE
    int byte DOS_SERVICES
    pop bp
    ret

; Stack: file handle. Seek to end, save DX:AX size, rewind to start, return saved size. [load+0x124d]
dos_file_size_and_rewind:
    push bp
    mov_load bp, sp
    mov bx,[bp+0x4]
    sub_load cx, cx
    sub_load dx, dx
    mov ax,DOS_SEEK_END
    int byte DOS_SERVICES
    push dx
    push ax
    sub_load dx, dx
    mov ax,DOS_SEEK_BEGIN
    int byte DOS_SERVICES
    pop ax
    pop dx
    pop bp
    ret

; Stack: writable NUL-terminated text. Replace its terminator with dollar, then DOS function 09h prints it. [load+0x1269]
dos_print_mutable_zstring:
    push bp
    mov_load bp, sp
    push di
    mov di,[bp+0x4]
    mov_load dx, di
    xor_load ax, ax
    mov_load cx, ax
    dec cx
    repne scasb
    dec di
    mov byte [di],0x24
    mov ah,0x9
    int byte DOS_SERVICES
    pop di
    pop bp
    ret

; Stack: size in 16-byte paragraphs. Return segment in AX, or zero on carry/error. [load+0x1284]
dos_alloc_paragraphs:
    push bp
    mov_load bp, sp
    mov bx,[bp+0x4]
    mov ah,DOS_ALLOCATE
    int byte DOS_SERVICES
    jnc short $ +4
    xor_load ax, ax
    pop bp
    ret

; Stack: DOS allocation segment. Free via ES; error result is not checked. [load+0x1294]
dos_free_segment:
    push bp
    mov_load bp, sp
    push es
    mov ax,[bp+0x4]
    mov es,ax
    mov ah,DOS_FREE
    int byte DOS_SERVICES
    pop es
    pop bp
    ret

; Set the BIOS cursor shape to 2000h (hidden). [load+0x12a4]
bios_hide_text_cursor:
    mov cx,0x2000
    mov ah,0x1
    int byte BIOS_VIDEO
    ret

; Stack: row, column. Cache position and set BIOS page-0 cursor. [load+0x12ac]
bios_set_text_cursor:
    push bp
    mov_load bp, sp
    xor_load bx, bx
    mov dx,[bp+0x6]
    mov [text_cursor_column],dl
    mov ax,[bp+0x4]
    mov [text_cursor_row],al
    mov_load dh, al
    mov ah,0x2
    int byte BIOS_VIDEO
    pop bp
    ret

; Stack: foreground, background. Pack two four-bit colors into the text attribute. [load+0x12c6]
text_set_attribute:
    push bp
    mov_load bp, sp
    mov ax,[bp+0x4]
    and al,0xf
    mov [text_attribute],al
    mov ax,[bp+0x6]
    shl al,1
    shl al,1
    shl al,1
    shl al,1
    or [text_attribute],al
    pop bp
    ret

; Stack: NUL-terminated string. Draw one character at a time using cached position/color. [load+0x12e2]
bios_draw_text_string:
    push bp
    mov_load bp, sp
    push si
    mov si,[bp+0x4]
    xor_load bx, bx
    mov dl,[text_cursor_column]
    mov dh,[text_cursor_row]
    mov ah,0x2
    int byte BIOS_VIDEO
    lodsb
    or_load al, al
    jz short $ +21
    mov ah,0x9
    mov cx,0x1
    xor_load bx, bx
    mov bl,[text_attribute]
    int byte BIOS_VIDEO
    inc byte [text_cursor_column]
    jmp short $ -36
    pop si
    pop bp
    ret

; Wait for BIOS key input; return ASCII in AL and scan code in AH. [load+0x1312]
bios_read_key:
    xor_load ax, ax
    int byte BIOS_KEYBOARD
    ret

; String/memory helpers below use near pointers and the caller-provided segment registers.

; Stack: ES-relative NUL-terminated string. Return length in AX (up to 65534); preserve DI. DF must be clear. [load+0x1317]
string_length:
    push bp
    mov_load bp, sp
    push di
    mov di,[bp+0x4]
    mov_load dx, di
    xor_load ax, ax
    mov_load cx, ax
    dec cx
    repne scasb
    dec di
    mov_load ax, di
    sub_load ax, dx
    pop di
    pop bp
    ret

; Stack: first DS-relative string, second DS-relative string. Return sign-extended final byte difference in AX; preserve SI/DI. DF must be clear. [load+0x132f]
; string_compare_signed_byte
    push bp
    mov_load bp, sp
    push si
    push di
    mov si,[bp+0x4]
    mov di,[bp+0x6]
    lodsb
    or_load al, al
    jz short $ +11
    cmp al,[di]
    jnz short $ +5
    inc di
    jmp short $ -10
    sub al,[di]
    cbw
    pop di
    pop si
    pop bp
    ret

; Stack: ES-relative destination, fill byte, count. REP STOSB writes AL; preserve DI. No C memset return-value guarantee; DF must be clear. [load+0x134d]
memory_fill_bytes:
    push bp
    mov_load bp, sp
    push di
    mov di,[bp+0x4]
    mov cx,[bp+0x8]
    mov ax,[bp+0x6]
    rep stosb
    pop di
    pop bp
    ret

; Stack: ES-relative destination, DS-relative source, count. REP MOVSB; preserve SI/DI. No overlap handling or C memcpy return-value guarantee; DF must be clear. [load+0x135f]
memory_copy_bytes_forward:
    push bp
    mov_load bp, sp
    push si
    push di
    mov si,[bp+0x6]
    mov di,[bp+0x4]
    mov cx,[bp+0x8]
    rep movsb
    pop di
    pop si
    pop bp
    ret

; AL: decimal digit 0..9. Convert to a character (zero becomes O); number_terrain_text_flags selects terrain glyphs versus regular screen text. [load+0x1373]
draw_decimal_digit:
    add al,0x30

; ASCII entry shared by text scripts and input repainting: zero uses the O glyph.
draw_decimal_character:
    cmp al,0x30
    jnz short $ +4
    mov al,0x4f
    test byte [number_terrain_text_flags],0x80
    jnz short game_draw_character
    jmp near $ +910

; Terrain text is geometry: each glyph consumes four terrain columns and eight rows (seven font rows plus a blank baseline). Font-bit pairs select the height-2 text-block models, which are projected and rasterized by the normal model renderer.

; AL: character or game-text control byte. Printable glyphs use the 8x8 font; controls remain partly decoded. [load+0x1385]
game_draw_character:
    push ax
    mov [terrain_text_character_work],al
    and al,0xc0
    jns short $ +23
    shl al,1
    shl al,1
    pop ax
    rcl ah,1
    and al,0x1f
    rcr ah,1
    jc short $ +6
    mov [terrain_text_column],al
    ret
    mov [terrain_text_row],al
    ret
    push cx
    push si
    mov al,[terrain_text_character_work]
    sub al,0x20
    xor_load ah, ah
    shl ax,1
    shl ax,1
    shl ax,1
    add ax,0x3876
    mov_load si, ax
    mov di,game_glyph_rows
    mov cx,0x8
    rep movsb
    mov al,[terrain_text_row]
    mov [0x10e4],al

; This comparison runs once per printable glyph with SI=7, addressing DS:148A and DS:378D; unsigned less-than advances the PRNG. The full-level recording never takes that branch.
    mov si,0x7
    mov al,[si+text_prng_compare_value-7]
    cmp al,[si+text_prng_compare_limit-7]
    jnc short $ +5
    call near prng_next_byte_carry_entry
    mov dl,[si+terrain_text_character_work]
    shl dl,1
    mov al,[terrain_text_column]
    mov [0x10e2],al
    mov cx,0x4
    shl dl,1
    rcl al,1
    shl dl,1
    rcl al,1
    and ax,strict word 0x3
    mov_load bx, ax
    mov al,[bx+glyph_pair_terrain_cells]
    push ax
    call near terrain_address_cell
    pop ax
    mov [bp+di],al
    inc byte [0x10e2]
    loop $ -28
    inc byte [0x10e4]
    dec si
    js short $ +11
    jnz short $ -52
    mov byte [terrain_text_character_work],0x0
    jmp short $ -59
    add byte [terrain_text_column],0x4
    pop si
    pop cx
    pop ax
    ret

; AL=0 builds THE SENTRY title from TFINAL.BIN and title_banner_markup; bit 7 selects the later secret-code variant. Render while the palette can still be uniformly blue. [load+0x141a]
draw_title_or_secret_code_screen:
    mov [title_secret_screen_flags],al
    mov byte [0x1229],0x80
    mov byte [0x1329],0xe0
    mov byte [0x11a9],0x2
    or byte [0x146b],0x80
    xor_load ax, ax
    call near terrain_transform_grid
    test byte [title_secret_screen_flags],0x80
    jz short $ +17
    call near completion_build_secret_code_terrain
    mov byte [scene_flags],0xa0
    mov bx,0x3
    xor_load ax, ax
    jmp short $ +24
    mov cx,0xf
    mov si,title_banner_markup
    lodsb
    call near game_draw_character
    loop $ -4
    mov byte [scene_flags],0xc0
    mov bx,0x1
    mov al,0x5
    mov dx,0x1
    call near landscape_render_scene
    and byte [0x146b],0x7f
    mov byte [scene_flags],0x0
    ret

; Copy the 200-line TFINAL.BIN image through the selected graphics backend; shared by title/final screens. [load+0x1473]
blit_title_final_screen_image:
    mov ax,[title_final_image_segment]
    xor_load si, si
    mov cx,0xc8
    mov bp,0x28
    call word near [graphics_blit_image]
    ret

; Copy the 157-line SKYBOX.BIN image portion through the selected image blitter. [load+0x1483]
blit_skybox_background_image:
    mov ax,[skybox_image_segment]
    xor_load si, si
    mov cx,0x9d
    mov bp,0x28
    call word near [graphics_blit_image]
    ret

; Copy the SKYBOX.BIN strip behind the landscape-number input area. [load+0x1493]
blit_landscape_number_panel:
    mov ax,[skybox_image_segment]
    mov si,0xb2
    mov cx,0x13
    mov bp,0x1c
    call word near [graphics_blit_image]
    ret

; Copy an eight-line strip from TFINAL.BIN for the landscape/secret-code prompt. [load+0x14a4]
blit_landscape_prompt_header:
    mov ax,[title_final_image_segment]
    mov si,0x10
    mov cx,0x8
    mov bp,0x28
    call word near [graphics_blit_image]
    ret

; Reviewed instruction range 14B5h..14C5h; internal routine roles remain partial. [load+0x14b5]
; recovered_code_14b5
    mov ax,[skybox_image_segment]
    mov si,0x18
    mov cx,0x78
    mov bp,0x20
    call word near [graphics_blit_image]
    ret

; AL: maximum digits (4 for landscape, 8 for secret code). Read digits/backspace until Enter into number_entry_buffer, least-significant digit first. [load+0x14c6]
input_decimal_digits:
    mov [0x110b],al
    call near input_clear_pending_keys
    mov cx,0x8
    mov al,0x20
    mov di,number_entry_buffer
    rep stosb
    call near $ +101
    xor_load dx, dx
    call near input_wait_key_or_click
    cmp al,0xd
    jz short $ +90
    cmp al,0x30
    jc short $ -9
    cmp al,0x7f
    jc short $ +24
    dec dx
    js short $ -18
    mov si,0x369a
    mov_load di, si
    dec di
    mov cx,0x7
    rep movsb
    mov byte [di],0x20
    call near $ +64
    jmp short $ -35
    cmp al,0x3a
    jnc short $ -39
    cmp dl,[0x110b]
    jnz short $ +28
    mov al,[sound_muted]
    mov [sound_mute_saved],al
    mov byte [sound_muted],0x0
    mov ax,0xe07
    xor_load bx, bx
    int byte BIOS_VIDEO
    mov al,[sound_mute_saved]
    mov [sound_muted],al
    jmp short $ -71
    inc dx
    mov si,0x369f
    mov_load di, si
    inc di
    mov cx,0x7
    std
    rep movsb
    cld
    mov [number_entry_buffer],al
    call near $ +6
    jmp short $ -93
    ret
    push dx
    or byte [text_input_render_flags],0x80
    mov al,[0x110b]
    cbw
    mov_load bx, ax
    dec bx
    mov al,[bx+number_entry_buffer]
    call near $ -471
    dec bx
    jns short $ -8
    mov al,[0x110b]
    cbw
    mov_load cx, ax
    mov al,0x8
    call near $ -485
    loop $ -5
    and byte [text_input_render_flags],0x7f
    pop dx
    ret

; Convert the eight reversed ASCII slots in number_entry_buffer into four packed-BCD bytes, in place; append FFh. [load+0x1566]
pack_entered_digits_to_bcd:
    xor_load ax, ax
    mov si,number_entry_buffer
    mov_load di, si
    mov cx,0x4
    call near $ +25
    mov_load bx, ax
    call near $ +20
    shl ax,1
    shl ax,1
    shl ax,1
    shl ax,1
    or_load ax, bx
    stosb
    loop $ -19
    mov byte [di],0xff
    ret
    lodsb
    cmp al,0x20
    jnz short $ +4
    mov al,0x30
    sub al,0x30
    ret

; AL: packed decimal pair. Emit high digit then low digit through draw_decimal_digit, using its current screen/terrain text mode. [load+0x1593]
draw_packed_bcd_pair:
    push ax
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    call near draw_decimal_digit
    pop ax
    and al,0xf
    jmp near draw_decimal_digit

; Advance the PRNG and form two decimal nibbles; used by secret-code and object-generation logic. [load+0x15a5]
prng_packed_decimal_byte:
    mov byte [bcd_random_digit_work],0x4
    call near prng_next_byte
    dec byte [bcd_random_digit_work]
    jnz short $ -7
    and al,0xf
    cmp al,0xa
    jc short $ +4
    sub al,0x6
    mov [bcd_random_digit_work],al
    call near prng_next_byte
    and al,0xf0
    cmp al,0xa0
    jc short $ +4
    sub al,0x60
    or al,[bcd_random_digit_work]
    ret

; The displayed code comes from the current BCD generator state. draw_decimal_digit renders numeric zero as O; the digit-entry prompt still expects 0.

; Set terrain-text mode and glyph origin (0,7), advance the BCD generator, and draw up to four pairs (eight digits) into the terrain grid; clear terrain-text mode afterward. [load+0x15ce]
completion_build_secret_code_terrain:
    mov al,0x80
    mov [number_terrain_text_flags],al
    call near game_draw_character
    mov al,0xc7
    call near game_draw_character
    shr byte [secret_code_pair_work],1
    mov bl,[secret_code_pair_work]
    call near prng_packed_decimal_byte
    cmp bl,0x4
    jnc short $ +7
    push bx
    call near draw_packed_bcd_pair
    pop bx
    dec bl
    jns short $ -15
    mov [secret_code_pair_work],bl
    call near prng_next_byte
    and byte [number_terrain_text_flags],0x7f
    ret

; Print the saved four-digit landscape number from landscape_number_bcd, high byte first. [load+0x1601]
draw_landscape_number_bcd:
    mov al,[landscape_number_bcd+1]
    call near draw_packed_bcd_pair
    mov al,[landscape_number_bcd]
    jmp short draw_packed_bcd_pair

; BX: packed-BCD landscape number. Save it for captions, seed the low PRNG word, and derive the landscape-zero flag and difficulty cap. [load+0x160c]
landscape_set_number_and_difficulty:
    mov [prng_state_low],bx
    mov [landscape_number_bcd],bx
    mov [landscape_nonzero],bh
    or_load bh, bh
    jnz short $ +21
    mov [landscape_nonzero],bl
    shr bl,1
    shr bl,1
    shr bl,1
    shr bl,1
    inc bl
    cmp bl,0x9
    jc short $ +4
    mov bl,0x8
    mov [landscape_difficulty],bl
    ret
    cmp al,0xc8
    jnc short $ +5
    jmp near $ +252
    sub al,0xc8
    cbw
    mov_load cx, ax
    push si
    call near draw_game_text_script
    pop si
    ret

; Select a count 1..8 around the leading landscape-number digit, retrying out-of-range samples. [load+0x1648]
landscape_pick_sentry_count:
    mov al,[landscape_number_bcd+1]
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    add al,0x2
    mov [math_work_lo],al

landscape_pick_sentry_count__L1658:
    call near prng_next_byte
    mov dl,0x7
    shl al,1
    pushf
    jz short landscape_pick_sentry_count__L166a
    mov dl,0xff

landscape_pick_sentry_count__L1664:
    inc dl
    shl al,1
    jnc short landscape_pick_sentry_count__L1664

landscape_pick_sentry_count__L166a:
    mov_load al, dl
    popf
    jnc short landscape_pick_sentry_count__L1671
    not al

landscape_pick_sentry_count__L1671:
    add al,[math_work_lo]
    cmp al,0x8
    jnc short landscape_pick_sentry_count__L1658
    inc al
    ret

; Return (random & 7) + ((random >> 3) & 15), a small nonuniform sample used by landscape setup. [load+0x167c]
prng_small_sum:
    call near prng_next_byte
    mov_load dl, al
    and al,0x7
    mov_load ah, al
    mov_load al, dl
    shr al,1
    shr al,1
    and al,0x1e
    shr al,1
    add_load al, ah
    ret

; Convert the byte at DS:1434 to a packed two-digit BCD counter using DAA. [load+0x1692]
player_energy_to_bcd:
    xor_load al, al
    mov cl,[player_energy]
    xor_load ch, ch
    jcxz player_energy_to_bcd__L16a1

player_energy_to_bcd__L169c:
    inc al
    daa
    loop player_energy_to_bcd__L169c

player_energy_to_bcd__L16a1:
    ret

recovered_sub_16a2:
    mov al,[input_action_slots+3]
    or_load al, al
    js short recovered_sub_16a2__L16e4
    cmp al,0x2
    jnz short recovered_sub_16a2__L16e4
    or byte [game_pause_flags],0x80
    mov byte [sentry_warning_pattern],0x8
    call word near [graphics_draw_sentry_warning_bar]
    call near speaker_silence_all
    mov al,[sound_muted]
    mov [sound_mute_saved],al

recovered_sub_16a2__L16c4:
    mov byte [sound_muted],0xff
    mov al,[input_action_slots+3]
    cmp al,0x3
    jnz short recovered_sub_16a2__L16c4
    mov al,[sound_mute_saved]
    mov [sound_muted],al
    mov byte [sentry_warning_pattern],0x0
    call word near [graphics_draw_sentry_warning_bar]
    and byte [game_pause_flags],0x7f

recovered_sub_16a2__L16e4:
    ret

; Draw tile zero across 40 columns, then publish the eight-pixel-high strip. [load+0x16e5]
video_draw_zero_tile_strip:
    xor_load ax, ax
    mov [hud_tile_column],al
    call near hud_append_tile
    cmp byte [hud_tile_column],0x28
    jc short $ -8
    jmp near video_present_hud_strip

; CX: script index. Follow a byte-offset table and process text/control/nested-script bytecode until FFh. [load+0x16f7]
draw_game_text_script:
    mov_load bx, cx
    mov bl,[bx+game_text_script_offsets]
    lea si,[bx+game_text_script_data]
    lodsb
    cmp al,0xff
    jz short $ +7
    call near $ -208
    jmp short $ -8
    xor_load bx, bx
    mov_load dx, bx
    ret
    test byte [text_input_render_flags],0x80
    jnz short $ +30
    cmp al,0x20
    jc short $ +26
    cmp al,0x7f
    jnc short $ +22
    mov [game_text_script_data],al
    mov [text_character_controls_a+9],al
    mov bx,0x16
    mov al,[bx+game_text_script_data]
    call near $ +1898
    dec bx
    jns short $ -8
    ret
    jmp near $ +1891
    cmp al,0x19
    jz short $ +5
    jmp short $ -42
    lodsb
    call near $ +1881
    dec byte [text_inline_control_bytes_left]
    jnz short $ -8
    mov byte [text_inline_control_bytes_left],0x6
    nop
    ret

; DL/DI: object slot. Compute camera-relative bearing, distance, height and model orientation before vertex projection. [load+0x174d]
model_prepare_camera_relative_origin:
    mov [drawn_object_slot],dl
    mov al,[di+object_model_id]
    mov [model_mesh_id],al
    mov bl,[view_object_slot]
    call near model_compute_planar_deltas
    call near model_compute_height_delta
    call near projection_bearing_from_deltas
    mov bl,[view_object_slot]
    mov al,[bx+object_heading]
    mov [view_heading_byte],al
    mov ax,[projection_angle]
    sub ax,[view_azimuth_reference]
    add ax,0xa00
    mov [model_azimuth_bias],ax
    mov dl,[drawn_object_slot]
    mov_load di, dx
    mov ah,[di+object_heading]
    xor_load al, al
    sub ax,[projection_angle]
    mov [model_relative_heading],ax
    call near projection_planar_distance
    mov al,[scene_view_preset]
    or_load al, al
    jnz short model_prepare_camera_relative_origin__L17b3
    cmp dl,0x3f
    jz short model_prepare_camera_relative_origin__L17b3
    mov word [model_relative_heading],0x8000
    shr word [projection_distance],1
    stc
    rcr word [model_height_delta],1
    add word [model_height_delta],0x70

model_prepare_camera_relative_origin__L17b3:
    mov ax,[projection_distance]
    mov [model_planar_distance],ax
    mov ax,[model_height_delta]
    mov [model_relative_height],ax
    ret

; Project the selected mesh vertex range into cached X/Y arrays, starting at slot 40h; input vertices use azimuth, height and radius arrays. [load+0x17c0]
model_project_vertices:
    mov bl,[model_mesh_id]
    mov byte [projected_vertex_slot],0x40
    mov al,[bx+model_vertex_range_ends]
    mov [model_element_end],al
    mov dl,[bx+model_vertex_ranges]
    mov_load di, dx
    mov [model_element_index],dl

model_project_vertices__L17da:
    mov ax,[model_relative_heading]
    mov [math_work_lo],al
    mov_load al, ah
    add al,[di+model_vertex_azimuth]
    call near projection_lookup_sine_cosine
    mov dl,[model_element_index]
    mov_load di, dx
    mov al,[di+model_vertex_radius]
    mov [math_work_hi],al
    mov al,[projection_cos]
    call near math_multiply_bytes_split_result
    mov [math_work_lo],al
    xor_load ax, ax
    test byte [projection_direction_flags],0x40
    jz short model_project_vertices__L180b
    call near math_negate_work_pair

model_project_vertices__L180b:
    mov [math_work_hi],al
    mov ax,[model_planar_distance]
    add ax,[math_work_lo]
    mov [projection_abs_delta_y],ax
    mov [projection_delta_y_sign],ah
    or_load ah, ah
    jns short model_project_vertices__L1824
    neg word [projection_abs_delta_y]

model_project_vertices__L1824:
    mov al,[di+model_vertex_radius]
    mov [math_work_hi],al
    mov al,[projection_sin]
    call near math_multiply_bytes_split_result
    xor_load ah, ah
    mov [projection_abs_delta_x],ax
    mov al,[projection_direction_flags]
    mov [projection_delta_x_sign],al
    call near projection_bearing_from_deltas
    mov dl,[projected_vertex_slot]
    mov_load di, dx
    mov ax,[projection_angle]
    add ax,[model_azimuth_bias]
    mov [di+projected_x_low],al
    mov [di+projected_x_high],ah
    call near projection_planar_distance
    mov dl,[model_element_index]
    mov_load di, dx
    mov al,[di+model_vertex_height]
    shl al,1
    mov [math_work_lo],al
    mov al,0x0
    jnc short model_project_vertices__L186d
    call near math_negate_work_pair

model_project_vertices__L186d:
    mov [math_work_hi],al
    mov ax,[math_work_lo]
    add ax,[model_relative_height]
    mov [projection_abs_delta_x],ax
    mov bl,[view_object_slot]
    call near projection_vertical_angle
    mov dl,[projected_vertex_slot]
    mov_load di, dx
    mov ax,[projection_screen_y]
    mov [di+projected_y_high],ah
    mov [di+projected_y_low],al
    inc byte [model_element_index]
    inc byte [projected_vertex_slot]
    mov dl,[model_element_index]
    mov_load di, dx
    cmp dl,[model_element_end]
    jz short model_project_vertices__L18a9
    jmp near model_project_vertices__L17da

model_project_vertices__L18a9:
    ret

; Prepare origin, project vertices, select visible face passes, scan-convert each indexed face and call the graphics span filler. [load+0x18aa]
model_draw_mesh:
    call near model_prepare_camera_relative_origin
    mov al,[projection_distance_high]
    cmp al,0xf
    cmc
    rcr byte [model_dither_flags],1
    call near model_project_vertices
    mov byte [polygon_face_flags],0x40
    xor_load ax, ax
    mov [model_face_pass],al
    mov [model_face_pass_xor],al
    mov bl,[model_mesh_id]
    mov al,[bx+model_visibility_flags]
    or_load al, al
    jz short model_draw_mesh__L18f7
    shr al,1
    jc short model_draw_mesh__L18de
    add byte [model_relative_heading_high],0xc0
    jmp short model_draw_mesh__L18f0

model_draw_mesh__L18de:
    mov al,[model_relative_height_high]
    or_load al, al
    jnz short model_draw_mesh__L18ee
    mov al,[model_relative_height]
    or_load al, al
    jz short model_draw_mesh__L18f7
    shr al,1

model_draw_mesh__L18ee:
    xor al,0x80

model_draw_mesh__L18f0:
    jns short model_draw_mesh__L18f7
    mov byte [model_face_pass],0x2

model_draw_mesh__L18f7:
    mov bl,[model_mesh_id]
    mov al,[bx+model_face_range_ends]
    mov [model_element_end],al
    mov dl,[bx+model_face_ranges]
    mov_load di, dx

model_draw_mesh__L1908:
    mov [model_element_index],dl
    mov al,[di+model_face_flags_and_counts]
    mov bl,[model_face_pass]
    or_load bl, bl
    jz short model_draw_mesh__L191e
    xor al,[model_face_pass_xor]
    js short model_draw_mesh__L193a

model_draw_mesh__L191e:
    and al,MODEL_FACE_VERTEX_COUNT_MASK
    mov [polygon_vertex_count],al
    mov al,[di+model_face_color_pairs]
    mov [polygon_packed_colors],al
    shl di,1
    mov ax,[di+model_face_index_pointers]
    mov [polygon_index_list],ax
    call near polygon_draw_face
    mov dl,[model_element_index]

model_draw_mesh__L193a:
    inc dl
    mov_load di, dx
    cmp dl,[model_element_end]
    jc short model_draw_mesh__L1908
    dec byte [model_face_pass]
    js short model_draw_mesh__L1953
    jz short model_draw_mesh__L1953
    mov byte [model_face_pass_xor],0x80
    jmp short model_draw_mesh__L18f7

model_draw_mesh__L1953:
    mov ax,terrain_face_indices
    mov [polygon_index_list],ax
    and byte [model_dither_flags],0x7f
    mov dl,[drawn_object_slot]
    mov_load di, dx
    ret

; BX: viewing object, DI: drawn object. Store signed grid-coordinate deltas and their magnitudes for bearing calculation. [load+0x1965]
model_compute_planar_deltas:
    mov byte [projection_abs_delta_x],0x0
    mov al,[di+object_grid_x]
    sub al,[bx+object_grid_x]
    sub al,[view_lateral_offset]
    mov [projection_delta_x_sign],al
    jns short model_compute_planar_deltas__L197d
    neg al

model_compute_planar_deltas__L197d:
    mov [projection_delta_x_high],al
    mov byte [projection_abs_delta_y],0x0
    mov al,[di+object_grid_y]
    sub al,[bx+object_grid_y]
    mov [projection_delta_y_sign],al
    jns short model_compute_planar_deltas__L1994
    neg al

model_compute_planar_deltas__L1994:
    mov [projection_delta_y_high],al
    ret

; BX: viewing object, DI: drawn object. Subtract the two byte-split fixed-point heights. [load+0x1998]
model_compute_height_delta:
    mov al,[di+object_height_fraction]
    mov ah,[di+object_height_integer]
    mov cl,[bx+object_height_fraction]
    mov ch,[bx+object_height_integer]
    sub_load ax, cx
    mov [model_height_delta],ax
    ret

; Discard pending keys/button state, then wait. Used by title and landscape-preview gates; Escape handling depends on the current phase. [load+0x19ae]
input_wait_fresh_key_or_click:
    call near input_clear_pending_keys
    mov byte [input_buttons],0x0

; Read a scan-code-table key or mouse click; return its translated character in AL. [load+0x19b6]
input_wait_key_or_click:
    mov cx,KEY_WAIT_SCAN_COUNT
    mov di,key_down
    mov al,KEY_PRESSED

; Title trace: 192,747 reads of scan indices 79h..7Ch were all zero while waiting for a fresh key. This scan also checks the ordinary indices; those reads were outside the selected watch ranges. Physical identities of 79h..7Ch remain unconfirmed.
    repne scasb
    jz short $ +18
    test byte [input_buttons],0x1
    jz short input_wait_key_or_click
    mov byte [input_buttons],0x0
    mov al,0x23
    jmp short $ +56
    dec di
    mov byte [di],0x0
    sub di,key_down
    cmp di,0x1
    jnz short $ +17
    cmp byte [preview_escape_restarts],0x0
    jnz short $ +5
    jmp near hardware_leave_game
    mov byte [restart_requested_flags],0x80
    mov al,[di+keyboard_ascii_by_scan_code]
    test byte [keyboard_e0_prefix],0xff
    jz short $ +17
    mov byte [keyboard_e0_prefix],0x0
    cmp al,0x30
    jc short $ +8
    cmp al,0x39
    ja short $ +4
    mov al,0x23
    ret

; Clear the 128 key states and E0 prefix flag before a new prompt/key wait. [load+0x1a09]
input_clear_pending_keys:
    mov di,key_down
    mov cx,KEY_SCAN_COUNT
    xor_load ax, ax
    rep stosb
    mov [keyboard_e0_prefix],al
    ret

; Reviewed instruction range 1A17h..1A77h; internal routine roles remain partial. [load+0x1a17]
recovered_code_1a17:
    push ax
    call near speaker_silence_all
    call near video_draw_zero_tile_strip
    mov byte [sound_sequence_mode],0x6
    mov byte [sound_ramp_index],0x1
    pop ax
    call near recovered_sub_1a5f
    xor_load dx, dx
    mov_load di, dx
    mov [software_cursor_saved_last_step],dl
    mov [cursor_mode_flags],dl
    mov al,[0x1444]
    call near model_configure_preview_viewpoint
    mov byte [0x146c],0x3
    mov byte [0x10ca],0x1
    mov al,0xc0
    mov [selection_model_draw_flags],al
    mov [selection_presentation_flags],al
    and byte [selection_copy_interrupt_flags],0x7f
    mov al,MELODY_END_EFFECT
    call near sound_start_melody
    call near recovered_code_34f2
    mov al,0x1e

recovered_sub_1a5f:
    mov [scene_scatter_outer_count],al

recovered_sub_1a5f__L1a62:
    mov byte [0x10da],0x1e

recovered_sub_1a5f__L1a67:
    call near video_scatter_clear_pixels
    dec byte [0x10da]
    jnz short recovered_sub_1a5f__L1a67
    dec byte [scene_scatter_outer_count]
    jnz short recovered_sub_1a5f__L1a62
    ret
    ret

; Terrain generation patches its outer return here. The signed completion flag branches directly into landscape_show_preview. [load+0x1a78]
landscape_preview_return_dispatch:
    js short landscape_show_preview

; Configure viewpoint object slot 1 relative to slot 2 using the preview-offset tables; make slot 2 the viewing object. [load+0x1a7a]
model_configure_preview_viewpoint:
    mov [object_model_id+0x1],al
    mov al,[di+preview_view_y_offsets]
    add al,[object_grid_y+0x2]
    mov [object_grid_y+0x1],al
    mov al,[di+preview_view_height_offsets]
    add al,[object_height_integer+0x2]
    mov [object_height_integer+0x1],al
    mov al,[object_grid_x+0x2]
    mov [object_grid_x+0x1],al
    mov al,[di+preview_view_pitch_values]
    mov [object_view_pitch+0x2],al
    mov al,[di+preview_view_companion_heading_values]
    mov [object_heading+0x2],al
    xor_load al, al
    mov [object_height_fraction+0x2],al
    mov [object_height_fraction+0x1],al
    mov al,[di+preview_view_heading_values]
    mov [object_heading+0x1],al
    mov bl,0x2
    mov [view_object_slot],bl
    ret

; Prepare landscape objects, render the generated scene, draw PRESS ANY KEY plus LANDSCAPE and its number, then finish object placement. [load+0x1abd]
landscape_show_preview:
    call near landscape_prepare_preview_objects
    mov bx,0x3
    xor_load dx, dx
    mov ax,0x80

; Render the selected landscape while blue; the later preview gate changes the palette to reveal it.
    call near landscape_render_scene
    mov cx,TEXT_SCRIPT_PREVIEW_CAPTION
    call near draw_game_text_script

; Add the saved landscape number to the preview caption.
    call near draw_landscape_number_bcd
    xor_load bx, bx
    mov_load dx, bx
    call near landscape_place_objects_and_enter_preview
    jmp near startup_wrong_code_and_retry

; Temporarily raise the lower Y clip bound to 5Bh, configure the view object and draw its mesh. [load+0x1ade]
model_draw_view_object_clipped:
    mov dx,0x1
    mov_load di, dx
    push word [viewport_y_min]
    mov byte [viewport_y_min],0x5b
    call near model_draw_configured_view_object
    pop word [viewport_y_min]
    ret

; Select preview-offset record 2 and the Sentinel pedestal model, then enter the common view-object drawing path. [load+0x1af4]
; model_draw_offset_sentinel_pedestal
    mov dl,0x2
    mov_load di, dx
    mov al,MODEL_SENTINEL_PEDESTAL

; AL: model ID, DI: viewpoint-offset record. Configure the object, then draw slot 1 through model_draw_mesh. [load+0x1afa]
model_draw_configured_view_object:
    call near model_configure_preview_viewpoint
    mov dl,0x1
    mov_load di, dx
    jmp near model_draw_mesh

; Multiply AL by math_work_hi. Store low product byte in math_work_lo; return the byte-swapped product in AX (AL=high, AH=low). [load+0x1b04]
math_multiply_bytes_split_result:
    mul byte [math_work_hi]
    mov [math_work_lo],al
    xchg ah,al
    ret

; SI <= DI: unsigned axis magnitudes. Divide SI:0000 by DI and index the arctangent table; equality returns 2000h (45 degrees). [load+0x1b0e]
projection_atan_first_octant:
    cmp_load si, di
    jz short projection_equal_axes_45_degrees
    push dx
    xor_load ax, ax
    mov_load dx, si
    div di
    mov [projection_axis_ratio],ah
    shr ax,1
    shr ax,1
    shr ax,1
    shr ax,1
    and ax,strict word 0xfffe
    mov_load si, ax
    mov ax,[si+projection_atan_lut]
    mov [projection_angle],ax
    pop dx
    ret

; The ratio is one; avoid DIV overflow and use the exact first-octant endpoint. [load+0x1b33]
projection_equal_axes_45_degrees:
    mov byte [projection_axis_ratio],0xff
    mov word [projection_angle],0x2000
    ret

; Reviewed instruction range 1B3Fh..1C2Ah; internal routine roles remain partial. [load+0x1b3f]
recovered_code_1b3f:
    push dx
    push bx
    mov [trig_source_angle],al
    mov [0x110c],al
    mov ax,[0x110b]
    shl ax,1
    shl ax,1
    mov dx,0xc900
    mul dx
    xchg ax,dx
    mov [trig_phase_work],ax
    mov byte [0x1103],0x2
    xor_load bx, bx
    test byte [trig_source_angle],0x40
    jz short recovered_code_1b3f__L1b6c
    inc bx
    inc bx
    sub byte [0x1103],0x2

recovered_code_1b3f__L1b6c:
    cmp ax,0x7af0
    jnc short recovered_code_1b3f__L1b98
    mov al,[trig_phase_work+1]
    mov [0x110c],al
    mov al,0xab
    call near math_multiply_bytes_split_result
    call near math_multiply_bytes_split_result
    mov [0x110c],al
    mov ax,[trig_phase_work]
    xor_load al, al
    mul word [0x110b]
    mov ax,[trig_phase_work]
    sub_load ax, dx
    shl ax,1
    mov [bx+trig_component_a],ax
    jmp short recovered_code_1b3f__L1bb3

recovered_code_1b3f__L1b98:
    mov cx,0xc900
    sub cx,[trig_phase_work]
    mov_load ax, cx
    xor_load al, al
    mul cx
    xchg ax,dx
    shl ax,1
    neg ax
    jnz short recovered_code_1b3f__L1baf
    mov ax,0xfffe

recovered_code_1b3f__L1baf:
    mov [bx+trig_component_a],ax

recovered_code_1b3f__L1bb3:
    cmp bl,[0x1103]
    jz short recovered_code_1b3f__L1bc9
    mov bl,[0x1103]
    mov ax,0xc900
    sub ax,[trig_phase_work]
    mov [trig_phase_work],ax
    jmp short recovered_code_1b3f__L1b6c

; Encoded component signs come from saved angle bit 7 (A) and angle bit 6 XOR bit 7 (B).

recovered_code_1b3f__L1bc9:
    test byte [trig_source_angle],0xff
    jns short recovered_code_1b3f__L1bd5
    or byte [trig_component_a],0x1

recovered_code_1b3f__L1bd5:
    mov al,[trig_source_angle]
    mov_load cl, al
    shl al,1
    xor_load al, cl
    jns short recovered_code_1b3f__L1be5
    or byte [trig_component_b],0x1

recovered_code_1b3f__L1be5:
    pop bx
    pop dx
    ret

; The title trace also sees 77h/FCh in the high scan-state slots before the fresh-key clear. These are resource-buffer contents, not key presses: only FFh is accepted by this helper.

; BX: scan-state index (callers keep BH zero). Load key_down[index] into BL and compare with FFh; ZF=1 means pressed. Does not consume or normalize the stored byte. [load+0x1be8]
input_test_scan_pressed:
    mov bl,[bx+key_down]
    cmp bl,KEY_PRESSED
    ret

recovered_sub_1bf0:
    mov al,0x80
    mov [input_action_slots],al
    mov [input_action_slots+2],al
    cmp word [mouse_x],0x12e
    jc short recovered_sub_1bf0__L1c05
    mov byte [input_action_slots],0x0

recovered_sub_1bf0__L1c05:
    cmp word [mouse_x],0x10
    jnc short recovered_sub_1bf0__L1c11
    mov byte [input_action_slots],0x1

recovered_sub_1bf0__L1c11:
    cmp word [mouse_y],0xbb
    jc short recovered_sub_1bf0__L1c1e
    mov byte [input_action_slots+2],0x2

recovered_sub_1bf0__L1c1e:
    cmp word [mouse_y],0xc
    jnc short recovered_sub_1bf0__L1c2a
    mov byte [input_action_slots+2],0x3

recovered_sub_1bf0__L1c2a:
    ret

; AL: heading, math_work_lo: fractional phase. Fold into the quarter-wave table and return two components in projection_sin/projection_cos. [load+0x1c2b]
projection_lookup_sine_cosine:
    or_load al, al
    jns short projection_lookup_sine_cosine__L1c31
    xor al,0x40

projection_lookup_sine_cosine__L1c31:
    mov [projection_direction_flags],al
    mov cl,[math_work_lo]
    shl cl,1
    rcl al,1
    and al,0x7f
    mov_load bl, al
    xor al,0x7f
    inc al
    jns short projection_lookup_sine_cosine__L1c48
    mov al,0x7f

projection_lookup_sine_cosine__L1c48:
    mov_load dl, al
    mov_load di, dx
    mov al,[bx+projection_quarter_sine_lut]
    mov bl,[di+projection_quarter_sine_lut]
    mov ah,[projection_direction_flags]
    test ah,0x80
    jnz short projection_lookup_sine_cosine__L1c6a
    test ah,0x40
    jnz short projection_lookup_sine_cosine__L1c6f

projection_lookup_sine_cosine__L1c62:
    mov [projection_sin],al
    mov [projection_cos],bl
    ret

projection_lookup_sine_cosine__L1c6a:
    test ah,0x40
    jnz short projection_lookup_sine_cosine__L1c62

projection_lookup_sine_cosine__L1c6f:
    mov [projection_cos],al
    mov [projection_sin],bl
    ret

; Multiply the encoded factor and signed work value, keeping the high product word and applying the direction flags. [load+0x1c77]
math_multiply_encoded_signed:
    push dx
    test byte [math_signed_factor_high],0xff
    jns short math_multiply_encoded_signed__L1c88
    neg word [math_signed_factor]
    xor byte [projection_direction_flags],0x80

math_multiply_encoded_signed__L1c88:
    test byte [math_encoded_factor],0x1
    jz short math_multiply_encoded_signed__L1c94
    xor byte [projection_direction_flags],0x80

math_multiply_encoded_signed__L1c94:
    mov ax,[math_signed_factor]
    mul word [math_encoded_factor]
    xchg ax,dx
    test byte [projection_direction_flags],0x80
    jz short math_multiply_encoded_signed__L1ca5
    neg ax

math_multiply_encoded_signed__L1ca5:
    pop dx
    ret

; Use the incoming sign flag to conditionally enter the shared two-byte negation helper. [load+0x1ca7]
math_negate_work_if_negative:
    jns short math_negate_work_pair__L1cb3

; Combine AL with math_work_lo as a signed word, negate it in place and return its new high byte in AL. [load+0x1ca9]
math_negate_work_pair:
    mov [math_work_hi],al
    neg word [math_work_lo]
    mov al,[math_work_hi]

math_negate_work_pair__L1cb3:
    ret

; Choose major/minor absolute axes, look up first-octant atan and restore quadrant signs; return a full-turn 16-bit bearing in projection_angle. [load+0x1cb4]
projection_bearing_from_deltas:
    mov si,[projection_abs_delta_y]
    mov di,[projection_abs_delta_x]
    cmp_load si, di
    jnc short projection_bearing_y_major
    mov [projection_major_axis],di
    mov [projection_minor_axis],si
    call near projection_atan_first_octant
    mov al,[projection_delta_x_sign]
    xor al,[projection_delta_y_sign]
    js short projection_bearing_from_deltas__L1cd8
    neg word [projection_angle]

projection_bearing_from_deltas__L1cd8:
    mov al,0x40
    test byte [projection_delta_x_sign],0x80
    jz short projection_bearing_from_deltas__L1ce3
    mov al,0xc0

projection_bearing_from_deltas__L1ce3:
    add [projection_angle_high],al
    ret

; Swap the major/minor axes and restore the alternate quadrant signs. [load+0x1ce8]
projection_bearing_y_major:
    xchg si,di
    mov [projection_major_axis],di
    mov [projection_minor_axis],si
    call near projection_atan_first_octant
    mov al,[projection_delta_x_sign]
    xor al,[projection_delta_y_sign]
    jns short projection_bearing_y_major__L1d02
    neg word [projection_angle]

projection_bearing_y_major__L1d02:
    xor_load ax, ax
    test byte [projection_delta_y_sign],0x80
    jz short projection_bearing_y_major__L1d0d
    mov al,0x80

projection_bearing_y_major__L1d0d:
    add [projection_angle_high],al
    ret

; Use height difference and planar distance to compute the vertical angle, subtract viewing pitch and scale to the cached screen Y coordinate. [load+0x1d12]
projection_vertical_angle:
    mov al,[projection_delta_x_high]
    mov [projection_delta_x_sign],al
    or_load al, al
    jns short projection_vertical_angle__L1d20
    neg word [projection_abs_delta_x]

projection_vertical_angle__L1d20:
    mov ax,[projection_distance]
    mov [projection_abs_delta_y],ax
    mov byte [projection_delta_y_sign],0x0
    call near projection_bearing_from_deltas
    mov ax,[projection_angle]
    mov cl,0x20
    mov ch,[bx+object_view_pitch]
    sub_load ax, cx
    sar ax,1
    sar ax,1
    sar ax,1
    sar ax,1
    mov [projection_screen_y],ax
    ret

; Use the saved minor/major ratio and a correction lookup to approximate Euclidean planar distance from the major-axis magnitude. [load+0x1d45]
projection_planar_distance:
    push dx
    mov al,[projection_axis_ratio]
    shr al,1
    adc al,0x0
    xor_load ah, ah
    mov_load di, ax
    mov ah,[di+projection_distance_correction_lut]
    xor_load al, al
    mul word [projection_minor_axis]
    shr dx,1
    add dx,[projection_major_axis]
    mov [projection_distance],dx
    pop dx
    ret

; Dispatch 50 highlighted scattered pixels through the adapter callback; used during scene preparation. [load+0x1d67]
video_scatter_highlight_pixels:
    mov bl,0xe
    mov cx,0x32
    jmp short video_dispatch_scattered_pixels

; Dispatch 200 clearing scattered pixels through the same callback. [load+0x1d6e]
video_scatter_clear_pixels:
    xor_load bl, bl
    mov cx,0xc8

; BL: clearing/highlight selector, CX: count. Tail-call the selected adapter pixel routine. [load+0x1d73]
video_dispatch_scattered_pixels:
    jmp word near [graphics_scatter_pixels]

; Reviewed instruction range 1D77h..1E95h; internal routine roles remain partial. [load+0x1d77]
recovered_code_1d77:
    call near object_allocate_saved_model_slot
    jnc short recovered_code_1d77__L1d7f
    jmp near recovered_code_1d77__L1e1f

recovered_code_1d77__L1d7f:
    mov [overview_camera_slot],bl
    mov dl,[player_robot_slot]
    mov_load di, dx
    mov ah,[di+0x11ea]
    xor_load al, al
    mov [ray_heading_angle],ax
    xor ah,0x80
    mov [bx+0x11ea],ah
    mov al,[di+0x606]
    add al,0xb
    js short recovered_code_1d77__L1da5
    cmp al,0x2
    jnc short recovered_code_1d77__L1da7

recovered_code_1d77__L1da5:
    mov al,0x2

recovered_code_1d77__L1da7:
    cmp al,0x2a
    jc short recovered_code_1d77__L1dad
    mov al,0x29

recovered_code_1d77__L1dad:
    mov [ray_pitch_angle+1],al
    mov_load ah, al
    neg al
    sub al,0x9
    mov [bx+0x606],al
    mov al,0x20
    mov [ray_pitch_angle],al
    mov [0x110b],al
    mov_load al, ah
    call near recovered_sub_3118
    mov bl,[overview_camera_slot]
    mov dl,[player_robot_slot]
    mov_load di, dx
    mov cl,0x6
    mov ax,[0x454]
    shr ax,cl
    add al,[di+0x112a]
    mov [bx+0x112a],al
    mov ax,[0x456]
    shr ax,cl
    add al,[di+0x116a]
    mov [bx+0x116a],al
    mov ax,[0x458]
    shr ax,cl
    add al,[di+0x11aa]
    mov [bx+0x11aa],al
    mov byte [bx+0x12ea],0xc0
    mov al,[overview_camera_slot]
    mov [0x1108],al
    mov cx,0x80
    mov al,0xff
    mov di,terrain_visibility_bits
    rep stosb
    call near scene_draw_background
    mov byte [0x5b24],0x1
    call near terrain_draw_view
    mov byte [0x5b24],0x2

recovered_code_1d77__L1e1f:
    ret

; Clamp mouse coordinates into the cursor rectangle, copy camera/software cursor positions, and request an edge view transition. mouse_axis_priority arbitrates corners; pitch endpoints can block a vertical request. [load+0x1e20]
mouse_update_cursor_and_edge_request:
    mov bl,[player_robot_slot]
    mov ch,[bx+object_view_pitch]
    mov ax,[mouse_x]
    shr ax,1
    cmp ax,MOUSE_CURSOR_X_MAX_PAIRS
    jc short mouse_update_cursor_and_edge_request__L1e3a
    mov ax,MOUSE_CURSOR_X_MAX_PAIRS
    mov byte [camera_transition_request],0x0

mouse_update_cursor_and_edge_request__L1e3a:
    cmp al,MOUSE_CURSOR_X_MIN_PAIRS
    jnc short mouse_update_cursor_and_edge_request__L1e45
    mov al,MOUSE_CURSOR_X_MIN_PAIRS
    mov byte [camera_transition_request],0x1

mouse_update_cursor_and_edge_request__L1e45:
    mov [camera_cursor_x_pairs],al
    mov [software_cursor_x_pairs],al

; Corner priority permits a vertical edge to replace an existing horizontal request. Pitch-limit checks still gate the replacement; the stored selector remains unchanged.
    mov cl,[mouse_axis_priority]
    sub cl,0x2
    mov ax,[mouse_y]
    cmp ax,MOUSE_CURSOR_Y_MAX
    jc short mouse_update_cursor_and_edge_request__L1e73
    mov ax,MOUSE_CURSOR_Y_MAX
    or_load cl, cl
    js short mouse_update_cursor_and_edge_request__L1e68
    test byte [camera_transition_request],0xff
    jns short mouse_update_cursor_and_edge_request__L1e73

mouse_update_cursor_and_edge_request__L1e68:
    cmp ch,[camera_transition_pitch_targets]
    jz short mouse_update_cursor_and_edge_request__L1e73
    mov byte [camera_transition_request],0x2

mouse_update_cursor_and_edge_request__L1e73:
    cmp al,MOUSE_CURSOR_Y_MIN
    jnc short mouse_update_cursor_and_edge_request__L1e8f
    mov al,MOUSE_CURSOR_Y_MIN
    or_load cl, cl
    js short mouse_update_cursor_and_edge_request__L1e84
    test byte [camera_transition_request],0xff
    jns short mouse_update_cursor_and_edge_request__L1e8f

mouse_update_cursor_and_edge_request__L1e84:
    cmp ch,[camera_transition_pitch_targets+1]
    jz short mouse_update_cursor_and_edge_request__L1e8f
    mov byte [camera_transition_request],0x3

mouse_update_cursor_and_edge_request__L1e8f:
    mov [camera_cursor_inverted_y],al
    mov [software_cursor_inverted_y],al
    ret
    mov [game_text_input_character],al
    push bx
    push cx
    push si
    mov al,[game_text_pending_opcode]
    or_load al, al
    jnz short $ +8
    mov al,[game_text_input_character]
    jmp near $ +146
    inc byte [game_text_parameter_index]
    cmp al,0x19
    jnz short $ +20
    mov al,[game_text_parameter_index]
    cmp al,0x5
    jnz short $ +10
    mov al,[game_text_input_character]
    mov [game_text_glyph_flags],al
    jmp short $ +114
    jmp near $ +378
    cmp al,0x1f
    jnz short $ +29
    mov al,[game_text_parameter_index]
    cmp al,0x2
    jz short $ +13
    mov al,[game_text_input_character]
    shl al,1
    cbw
    mov [game_text_column],ax
    jmp short $ +94
    mov al,[game_text_input_character]
    cbw
    mov [game_text_row],ax
    jmp short $ +80
    cmp al,0x12
    jnz short $ +52
    mov al,[game_text_parameter_index]
    cmp al,0x1
    jz short $ +74
    mov al,[game_text_input_character]
    and_load al, al
    jns short $ +21
    and al,0x7f
    cbw
    mov [game_text_alternate_background],ax
    test word [game_text_alternate_mode],0x80
    jz short $ +22
    mov [game_text_active_background],ax
    jmp short $ +43
    cbw
    mov [game_text_alternate_foreground],ax
    test word [game_text_alternate_mode],0x80
    jz short $ +5
    mov [game_text_active_foreground],ax
    jmp short $ +26
    cmp al,0x11
    jnz short $ +22
    mov al,[game_text_input_character]
    and al,0x7f
    cbw
    mov [game_text_normal_background],ax
    test word [game_text_alternate_mode],0x80
    jnz short $ +5
    mov [game_text_active_background],ax
    xor_load ax, ax
    mov [game_text_pending_opcode],al
    jmp near $ +261
    cmp al,0x1f
    jz short $ +14
    cmp al,0x12
    jz short $ +10
    cmp al,0x19
    jz short $ +6
    cmp al,0x11
    jnz short $ +13
    mov [game_text_pending_opcode],al
    mov byte [game_text_parameter_index],0x0
    jmp near $ +234
    cmp al,0x9
    jnz short $ +21
    mov ax,[game_text_column]
    add ax,strict word 0x2
    cmp ax,strict word 0x28
    jc short $ +4
    xor_load ax, ax
    mov [game_text_column],ax
    jmp near $ +211
    cmp al,0x8
    jnz short $ +30
    sub word [game_text_column],0x2
    jns short $ +20
    dec word [game_text_row]
    jns short $ +8
    mov word [game_text_row],0x18
    mov word [game_text_column],0x26
    jmp near $ +179
    cmp al,0xa
    jnz short $ +8
    call near $ +176
    jmp near $ +169
    cmp al,0xd
    jnz short $ +11
    mov word [game_text_column],0x0
    jmp near $ +156
    cmp al,0x20
    jnc short $ +54
    cmp al,0x4
    jnz short $ +24
    xor_load ax, ax
    mov [game_text_transparent],al
    mov [game_text_alternate_mode],al
    mov ax,[game_text_normal_background]
    mov [game_text_active_background],ax
    mov ax,[game_text_normal_foreground]
    mov [game_text_active_foreground],ax
    jmp short $ +125
    cmp al,0x5
    jnz short $ +22
    mov al,0xff
    mov [game_text_transparent],al
    mov [game_text_alternate_mode],al
    mov ax,[game_text_alternate_background]
    mov [game_text_active_background],ax
    mov ax,[game_text_alternate_foreground]
    mov [game_text_active_foreground],ax
    jmp short $ +99
    cmp al,0x7f
    jnz short $ +33
    mov al,0x8
    call near $ -329
    push word [game_text_transparent]
    mov word [game_text_transparent],0x0
    mov al,0x20
    call near $ -344
    pop word [game_text_transparent]
    mov al,0x8
    call near $ -353
    jmp short $ +64
    cmp al,0x80
    jc short $ +4
    jmp short $ +58
    sub al,0x20
    and al,0x7f
    cbw
    push word [game_text_glyph_flags]
    push word [game_text_transparent]
    push word [game_text_active_background]
    push word [game_text_active_foreground]
    push word [game_text_row]
    push word [game_text_column]
    push ax
    call word near [0x65ca]
    add sp,0xe
    mov ax,[game_text_column]
    add ax,strict word 0x2
    cmp ax,strict word 0x28
    jc short $ +7
    call near $ +12
    xor_load ax, ax
    mov [game_text_column],ax
    pop si
    pop cx
    pop bx
    ret
    mov ax,[game_text_row]
    inc ax
    cmp ax,strict word 0x19
    jc short $ +4
    xor_load ax, ax
    mov [game_text_row],ax
    ret

; Construct the terrain triangle vertex loop, including the closing vertex, then join the common polygon-edge setup. [load+0x204d]
polygon_build_triangle_indices:
    mov dl,[terrain_triangle_orientation]
    mov_load di, dx
    test ah,0x40
    jz short polygon_build_triangle_indices__L2061
    add al,0x21
    and al,0x3f
    add dl,0x2
    mov_load di, dx

polygon_build_triangle_indices__L2061:
    mov [terrain_face_indices],al
    mov [terrain_face_indices+0x3],al
    xor al,0x20
    mov [terrain_face_indices+0x1],al
    add al,[di+terrain_triangle_vertex_delta]
    and al,0x3f
    mov [terrain_face_indices+0x2],al
    mov bx,0x3
    jmp short polygon_build_clipped_spans__L20a7

; Select terrain quad/triangle or model face indices, convert cached coordinates, and rasterize edges into left/right span arrays; carry signals rejection. [load+0x207a]
polygon_build_clipped_spans:
    mov al,[terrain_cell_x]
    or al,[terrain_projected_row_bank]
    mov ah,[polygon_face_flags]
    test ah,0x80
    jnz short polygon_build_triangle_indices
    test ah,0x40
    jnz short polygon_project_index_loop
    mov [terrain_face_indices],al
    mov [terrain_face_indices+0x4],al
    xor al,0x20
    mov [terrain_face_indices+0x1],al
    inc al
    mov [terrain_face_indices+0x2],al
    xor al,0x20
    mov [terrain_face_indices+0x3],al
    mov bx,0x4

polygon_build_clipped_spans__L20a7:
    mov [polygon_vertex_count],bl

; Convert cached projected X coordinates for the polygon index loop. [load+0x20ab]
polygon_project_index_loop:
    mov byte [polygon_clip_work],0x0
    mov dl,[polygon_vertex_count]
    mov_load di, dx
    mov bp,[polygon_index_list]

polygon_project_index_loop__L20ba:
    mov bl,[bp+di]
    mov ah,[bx+projected_x_high]
    mov al,[bx+projected_x_low]
    add ax,[projection_x_bias]
    sar ax,1
    sar ax,1
    sar ax,1
    sar ax,1
    add_load bx, bx
    mov [bx+polygon_projected_x_words],ax
    dec di
    jns short polygon_project_index_loop__L20ba
    xor_load ax, ax
    mov [polygon_y_max],al
    mov [polygon_horizontal_x_max],ax
    mov [polygon_horizontal_edge_count],al
    dec ax
    mov [polygon_y_min],al
    mov [polygon_horizontal_x_min],ax
    mov [projection_clip_status],al

; Start walking consecutive vertices, including the closing edge. [load+0x20ee]
; polygon_begin_edges
    xor_load dx, dx
    mov_load di, dx

polygon_begin_edges__L20f2:
    mov [polygon_edge_index],dl
    mov bp,[polygon_index_list]
    mov bl,[bp+di]
    inc di
    mov dl,[bp+di]
    mov_load di, dx
    mov ax,polygon_left_spans
    mov [edge_span_buffer],ax
    mov ah,[di+projected_y_high]
    mov al,[di+projected_y_low]
    mov ch,[bx+projected_y_high]
    mov cl,[bx+projected_y_low]
    sub_load ax, cx
    jns short polygon_begin_edges__L2127
    add word [edge_span_buffer],0x200
    xchg bx,dx
    mov_load di, dx
    neg ax

polygon_begin_edges__L2127:
    mov [edge_delta_y],ax
    mov_load si, bx
    add_load si, si
    mov_load bp, di
    add_load bp, bp
    and ax,0xff00
    jz short polygon_begin_edges__L213a
    jmp near polygon_subdivide_long_edge

polygon_begin_edges__L213a:
    mov al,[edge_delta_y]
    or_load al, al
    jnz short polygon_begin_edges__L2144
    jmp near polygon_begin_edges__L21d5

polygon_begin_edges__L2144:
    mov ah,[di+projected_y_high]
    mov al,[di+projected_y_low]
    mov [edge_end_y],ax
    mov ah,[bx+projected_y_high]
    mov al,[bx+projected_y_low]
    mov [edge_start_y],ax
    mov ax,[ds:bp+polygon_projected_x_words]
    mov [edge_end_x],ax
    mov ax,[si+polygon_projected_x_words]
    mov [edge_start_x],ax
    call near polygon_rasterize_edge

polygon_begin_edges__L216c:
    mov dl,[polygon_edge_index]
    inc dl
    mov_load di, dx
    cmp dl,[polygon_vertex_count]
    jz short polygon_begin_edges__L217d
    jmp near polygon_begin_edges__L20f2

polygon_begin_edges__L217d:
    mov al,[polygon_horizontal_edge_count]
    cmp al,[polygon_vertex_count]
    jnz short polygon_begin_edges__L21c1
    mov al,[bx+projected_y_high]
    or_load al, al
    jnz short polygon_begin_edges__L21c1
    mov dl,[bx+projected_y_low]
    mov_load di, dx
    cmp dl,[viewport_y_min]
    jc short polygon_begin_edges__L21c1
    cmp dl,[viewport_y_end]
    jnc short polygon_begin_edges__L21c1
    mov [polygon_y_min],dl
    mov [polygon_y_max],dl
    mov_load bp, di
    add_load bp, bp
    mov ax,[polygon_horizontal_x_min]
    mov [ds:bp+polygon_left_spans],ax
    mov ax,[polygon_horizontal_x_max]
    mov [ds:bp+polygon_right_spans],ax
    mov byte [projection_clip_status],0x0

polygon_begin_edges__L21c1:
    test byte [projection_clip_status],0xff
    jnz short polygon_begin_edges__L21d3
    mov al,[polygon_y_max]
    cmp al,[polygon_y_min]
    jc short polygon_begin_edges__L21d3
    clc
    ret

polygon_begin_edges__L21d3:
    stc
    ret

polygon_begin_edges__L21d5:
    mov ax,[si+polygon_projected_x_words]
    cmp ax,[polygon_horizontal_x_max]
    jc short polygon_begin_edges__L21e2
    mov [polygon_horizontal_x_max],ax

polygon_begin_edges__L21e2:
    cmp ax,[polygon_horizontal_x_min]
    jnc short polygon_begin_edges__L21eb
    mov [polygon_horizontal_x_min],ax

polygon_begin_edges__L21eb:
    inc byte [polygon_horizontal_edge_count]
    jmp near polygon_begin_edges__L216c

; Shared early return for edge endpoints outside the usable Y interval. [load+0x21f2]
polygon_edge_reject_return:
    ret

; SELF-MODIFYING RASTERIZER: opcode patches below are intentional; preserve their exact encodings and code offsets.

; Clip an edge in Y and interpolate X into the selected span buffer. Patches INC/DEC opcodes and a branch displacement in its own code. [load+0x21f3]
polygon_rasterize_edge:
    mov al,[edge_start_y_high]
    or_load al, al
    js short edge_clip_start_to_viewport
    jnz short polygon_edge_reject_return
    mov al,[edge_start_y]
    cmp al,[viewport_y_end]
    jnc short polygon_edge_reject_return
    cmp al,[polygon_y_min]
    jnc short edge_clip_start_to_viewport__L2217
    cmp al,[viewport_y_min]
    jnc short edge_clip_start_to_viewport__L2214

; Clamp the start Y to the viewport minimum. [load+0x2211]
edge_clip_start_to_viewport:
    mov al,[viewport_y_min]

edge_clip_start_to_viewport__L2214:
    mov [polygon_y_min],al

edge_clip_start_to_viewport__L2217:
    mov al,[edge_end_y_high]
    or_load al, al
    js short polygon_edge_reject_return
    jnz short edge_clip_end_to_viewport
    mov al,[edge_end_y]
    cmp al,[viewport_y_min]
    jc short polygon_edge_reject_return
    cmp al,[polygon_y_max]
    jc short edge_clip_end_to_viewport__L223c
    cmp al,[viewport_y_end]
    jc short edge_clip_end_to_viewport__L2239

; Clamp the end Y to the exclusive viewport maximum minus one. [load+0x2235]
edge_clip_end_to_viewport:
    mov al,[viewport_y_end]
    dec ax

edge_clip_end_to_viewport__L2239:
    mov [polygon_y_max],al

edge_clip_end_to_viewport__L223c:
    mov ax,[edge_end_x]
    sub ax,[edge_start_x]
    jns short edge_clip_end_to_viewport__L224e
    neg ax
    mov [edge_abs_delta_x],ax
    mov cl,0x40
    jmp short edge_choose_slope_path

edge_clip_end_to_viewport__L224e:
    mov [edge_abs_delta_x],ax
    mov cl,0x48

; Choose interpolation by comparing absolute delta X and delta Y. [load+0x2253]
edge_choose_slope_path:
    mov bx,[edge_abs_delta_x]
    mov dx,[edge_delta_y]
    mov al,[edge_end_y]
    mov_load ch, al
    xor_load ah, ah
    shl ax,1
    add ax,[edge_span_buffer]
    mov_load di, ax
    cmp_load bx, dx
    jnc short edge_step_each_x

; Y-major path: one output span entry per Y step, with occasional signed X steps. [load+0x226e]
; edge_step_each_y
    std
    mov [cs:edge_y_major_step_offset],cl
    mov_load cl, dl
    mov_load bh, ch
    xor_load ch, ch
    inc cx
    mov_load dh, dl
    shr dh,1
    xor dh,0xff
    mov al,[edge_end_y_high]
    or_load al, al
    jnz short edge_skip_clipped_y_major
    mov ax,[edge_end_x]
    jmp short edge_y_major_x_step__L2295

; Accumulate the X-error term and emit a boundary X for each Y. [load+0x228e]
edge_y_major_loop:
    add_load dh, bl
    jnc short edge_y_major_x_step__L2295
    sub_load dh, dl

; Self-modified single-byte instruction: INC AX (40h) or DEC AX (48h). [load+0x2294]
; edge_y_major_x_step
    inc ax

edge_y_major_x_step__L2295:
    stosw
    dec bh
    jz short edge_restore_direction_and_return__L22a6

edge_y_major_x_step__L229a:
    loop edge_y_major_loop

; Record the interpolation completion count, clear scratch registers and restore forward string direction. [load+0x229c]
edge_finish_count:
    mov [projection_clip_status],cl

; All backward string stores must restore DF before returning. [load+0x22a0]
edge_restore_direction_and_return:
    xor_load bx, bx
    mov_load dx, bx
    cld
    ret

edge_restore_direction_and_return__L22a6:
    xor_load cx, cx
    jmp short edge_finish_count

; X-major path: advance X each iteration and move to another span entry when Y changes. [load+0x22aa]
edge_step_each_x:
    mov [cs:edge_x_major_step_offset],cl
    mov al,0x7
    mov si,polygon_left_spans
    cmp si,[edge_span_buffer]
    jz short edge_step_each_x__L22c1
    cmp cl,0x48
    jz short edge_step_each_x__L22c6
    jnz short edge_step_each_x__L22c8

edge_step_each_x__L22c1:
    cmp cl,0x48
    jz short edge_step_each_x__L22c8

edge_step_each_x__L22c6:
    mov al,0x9

; Patch the short-branch displacement to 7 or 9 according to span side and X-step direction.

edge_step_each_x__L22c8:
    mov [cs:edge_x_major_carry_displacement],al
    xchg ch,cl
    xor_load ch, ch
    mov_load si, cx
    mov_load cx, bx
    inc cx
    mov_load bp, bx
    shr bp,1
    xor bp,0xffffffffffffffff
    mov al,[edge_end_y_high]
    or_load al, al
    jnz short edge_skip_clipped_x_major
    mov ax,[edge_end_x]
    jmp short edge_x_major_x_step__L22f4

; Self-modified INC/DEC AX instruction in the X-major interpolator. [load+0x22e8]
edge_x_major_x_step:
    inc ax
    add_load bp, dx
    jnc short edge_x_major_x_step__L22f4
    sub_load bp, bx
    dec di
    dec di
    dec si
    jz short edge_restore_direction_and_return__L22a6

edge_x_major_x_step__L22f4:
    mov [di],ax

edge_x_major_x_step__L22f6:
    loop edge_x_major_x_step
    jmp short edge_finish_count

; Walk the off-screen Y-major prefix without writing span entries. [load+0x22fa]
edge_skip_clipped_y_major:
    inc bh
    mov al,[cs:edge_y_major_step_offset]
    mov [cs:edge_clipped_y_step_offset],al
    mov ax,[edge_end_x]
    jmp short edge_clipped_y_major_x_step__L2310

edge_skip_clipped_y_major__L2309:
    add_load dh, bl
    jnc short edge_clipped_y_major_x_step__L2310
    sub_load dh, dl

; Self-modified X step while skipping clipped Y-major pixels. [load+0x230f]
; edge_clipped_y_major_x_step
    inc ax

edge_clipped_y_major_x_step__L2310:
    dec bh
    jz short edge_clipped_y_major_x_step__L2318
    loop edge_skip_clipped_y_major__L2309
    jmp short edge_restore_direction_and_return

edge_clipped_y_major_x_step__L2318:
    mov di,[edge_span_buffer]
    add di,0x1fe
    dec bh
    jmp near edge_y_major_x_step__L229a

; Walk the off-screen X-major prefix without writing span entries. [load+0x2325]
edge_skip_clipped_x_major:
    inc si
    mov al,[cs:edge_x_major_step_offset]
    mov [cs:edge_clipped_x_step_offset],al
    mov ax,[edge_end_x]
    jmp short edge_clipped_x_major_x_step__L233d

; Self-modified X step while skipping clipped X-major pixels. [load+0x2333]
edge_clipped_x_major_x_step:
    inc ax
    add_load bp, dx
    jnc short edge_clipped_x_major_x_step__L233d
    sub_load bp, bx
    dec si
    jz short edge_clipped_x_major_x_step__L2342

edge_clipped_x_major_x_step__L233d:
    loop edge_clipped_x_major_x_step
    jmp near edge_restore_direction_and_return

edge_clipped_x_major_x_step__L2342:
    mov di,[edge_span_buffer]
    add di,0x1fe
    dec si
    jmp short edge_x_major_x_step__L22f6

; Scale long edge deltas into byte-sized interpolation steps, process the subdivisions, then continue the polygon edge loop. [load+0x234d]
polygon_subdivide_long_edge:
    mov [edge_saved_vertex],bl
    mov ax,[ds:bp+polygon_projected_x_words]
    sub ax,[si+polygon_projected_x_words]
    mov [edge_delta_x_sign],ah
    or_load ah, ah
    jns short polygon_subdivide_long_edge__L2364
    neg ax

polygon_subdivide_long_edge__L2364:
    xor_load cx, cx
    mov_load bx, ax
    or ah,[edge_delta_y_high]
    jz short polygon_subdivide_long_edge__L237b

polygon_subdivide_long_edge__L236e:
    shr word [edge_delta_y],1
    shr bx,1
    stc
    rcl cl,1
    shr ah,1
    jnz short polygon_subdivide_long_edge__L236e

polygon_subdivide_long_edge__L237b:
    cmp byte [edge_delta_y],0xff
    jz short polygon_subdivide_long_edge__L236e
    cmp bl,0xff
    jz short polygon_subdivide_long_edge__L236e
    test byte [edge_delta_x_sign],0x80
    jz short polygon_subdivide_long_edge__L2390
    neg bx

polygon_subdivide_long_edge__L2390:
    mov [edge_scaled_delta_x],bx
    mov ax,[ds:bp+polygon_projected_x_words]
    mov [edge_start_x],ax
    mov ah,[di+projected_y_high]
    mov al,[di+projected_y_low]
    mov [edge_start_y],ax
    xor_load bx, bx
    mov [edge_subdivision_count],cl
    or_load cl, cl
    jz short polygon_subdivide_long_edge__L23d4

polygon_subdivide_long_edge__L23b1:
    mov ax,[edge_start_y]
    mov [edge_end_y],ax
    sub ax,[edge_delta_y]
    mov [edge_start_y],ax
    mov ax,[edge_start_x]
    mov [edge_end_x],ax
    sub ax,[edge_scaled_delta_x]
    mov [edge_start_x],ax
    call near polygon_rasterize_edge
    dec byte [edge_subdivision_count]
    jnz short polygon_subdivide_long_edge__L23b1

polygon_subdivide_long_edge__L23d4:
    mov ax,[edge_start_y]
    mov [edge_end_y],ax
    mov ax,[edge_start_x]
    mov [edge_end_x],ax
    mov bl,[edge_saved_vertex]
    mov_load si, bx
    add_load si, si
    mov al,[bx+projected_y_low]
    mov ah,[bx+projected_y_high]
    mov [edge_start_y],ax
    mov ax,[si+polygon_projected_x_words]
    mov [edge_start_x],ax
    mov ax,[edge_end_y]
    sub ax,[edge_start_y]
    mov [edge_delta_y],ax
    call near polygon_rasterize_edge
    jmp near polygon_begin_edges__L216c

; Alternate PRNG entry used by the terrain-text comparison: JC +0 reaches prng_next_byte with either carry state. The purpose of this redundant branch remains unconfirmed. [load+0x240a]
prng_next_byte_carry_entry:
    jc short prng_next_byte

; Feedback is state bit 19 XOR state bit 32; eight carry-chain shifts produce one output byte. Native DOSBox recurrence check: scripts/check_mixed_data.py.

; Advance the game PRNG and return a byte in AL; used by landscape generation after number/code seeding. [load+0x240c]
prng_next_byte:
    push bx
    push dx
    mov bx,[prng_state_high]
    mov dx,[prng_state_low]
    mov ah,[prng_state_middle]
    mov cx,0x8
    mov_load al, ah
    rcr al,1
    rcr al,1
    rcr al,1
    xor_load al, bh
    rcr al,1
    rcl dl,1
    rcl dh,1
    rcl ah,1
    adc_load bx, bx
    loop $ -20
    mov [prng_state_middle],ah
    mov [prng_state_low],dx
    mov [prng_state_high],bx
    mov_load al, bh
    pop dx
    pop bx
    ret

; Rotate traversal into the viewing quadrant, project/cull row endpoints, and draw visible terrain rows from far to near. [load+0x2444]
terrain_draw_view:
    mov byte [terrain_rows_remaining],TERRAIN_SIDE
    mov bl,[view_object_slot]
    xor_load bh, bh
    mov_load ch, bh
    mov_load dh, bh
    mov byte [view_grid_x_overflow],0x0
    mov ah,[bx+object_view_pitch]
    mov al,0x20
    sar al,1
    sar al,1
    sar al,1
    sar al,1
    mov [view_pitch_reference],ax
    mov al,[bx+object_heading]
    sub al,0xa
    mov [terrain_view_rotation_work],al
    mov ax,terrain_face_indices
    mov [polygon_index_list],ax
    mov ah,[bx+object_heading]
    add ah,0x20
    mov [terrain_view_rotation],ah
    and ah,0x3f
    sub ah,0x2a
    mov al,[view_azimuth_reference]
    mov [terrain_azimuth_bias],ax
    mov al,[terrain_view_rotation]
    shl al,1
    rcl al,1
    rcl al,1
    and ax,strict word 0x3
    mov_load si, ax
    mov al,[si+terrain_quadrant_corner_bias]
    mov [terrain_corner_bias],al
    mov_load ax, si
    shl al,1
    shl al,1
    mov [terrain_face_rotation_bias],al
    mov_load ax, si
    sub al,0x2
    mov [terrain_triangle_view_bias],al
    mov al,[terrain_view_rotation]
    test al,0x80
    jnz short terrain_draw_view__L24e1
    test al,0x40
    jnz short terrain_draw_view__L24cf
    mov al,[bx+object_grid_x]
    mov [view_grid_x],al
    mov al,[bx+object_grid_y]
    mov [view_grid_y],al
    jmp short terrain_draw_view__L2509

terrain_draw_view__L24cf:
    mov al,0x1e
    sub al,[bx+object_grid_y]
    mov [view_grid_x],al
    mov al,[bx+object_grid_x]
    mov [view_grid_y],al
    jmp short terrain_draw_view__L2509

terrain_draw_view__L24e1:
    test al,0x40
    jnz short terrain_draw_view__L24f9
    mov al,0x1e
    sub al,[bx+object_grid_x]
    mov [view_grid_x],al
    mov al,0x1e
    sub al,[bx+object_grid_y]
    mov [view_grid_y],al
    jmp short terrain_draw_view__L2509

terrain_draw_view__L24f9:
    mov al,[bx+object_grid_y]
    mov [view_grid_x],al
    mov al,0x1e
    sub al,[bx+object_grid_x]
    mov [view_grid_y],al

terrain_draw_view__L2509:
    cmp byte [view_grid_x],0x20
    jc short terrain_draw_view__L252e
    mov al,[view_grid_x]
    or_load al, al
    js short terrain_draw_view__L2523
    sub al,0x1f
    mov [view_grid_x_overflow],al
    mov byte [view_grid_x],0x1f
    jmp short terrain_draw_view__L252e

terrain_draw_view__L2523:
    mov al,[view_grid_x]
    mov [view_grid_x_overflow],al
    mov byte [view_grid_x],0x0

terrain_draw_view__L252e:
    mov byte [terrain_scan_y],0x1f
    mov al,[terrain_row_left_hint]
    mov [terrain_row_left],al
    mov byte [terrain_projected_row_bank],0x0
    call near terrain_find_visible_row_interval
    mov al,[terrain_row_left]
    mov [terrain_row_left_hint],al

terrain_draw_view__L2547:
    dec byte [terrain_rows_remaining]
    xor byte [terrain_projected_row_bank],0x20
    mov al,[terrain_row_left]
    mov [terrain_previous_row_left],al
    mov al,[terrain_row_right]
    mov [terrain_previous_row_right],al
    dec byte [terrain_scan_y]
    js short terrain_draw_view__L256f
    mov dl,[terrain_scan_y]
    cmp dl,[view_grid_y]
    jnz short terrain_draw_view__L2571
    jmp near terrain_draw_view__L2606

terrain_draw_view__L256f:
    clc
    ret

terrain_draw_view__L2571:
    call near terrain_find_visible_row_interval
    mov dl,[terrain_row_left]
    cmp dl,[terrain_previous_row_left]
    jz short terrain_draw_view__L25b2
    jc short terrain_draw_view__L258d

terrain_draw_view__L2580:
    dec dl
    call near terrain_project_corner
    cmp dl,[terrain_previous_row_left]
    jnz short terrain_draw_view__L2580
    jmp short terrain_draw_view__L25b2

terrain_draw_view__L258d:
    xor byte [terrain_projected_row_bank],0x20
    inc byte [terrain_scan_y]
    mov dl,[terrain_previous_row_left]

terrain_draw_view__L259a:
    dec dl
    call near terrain_project_corner
    cmp dl,[terrain_row_left]
    jnz short terrain_draw_view__L259a
    mov [terrain_previous_row_left],dl
    dec byte [terrain_scan_y]
    xor byte [terrain_projected_row_bank],0x20

terrain_draw_view__L25b2:
    mov dl,[terrain_row_right]
    cmp dl,[terrain_previous_row_right]
    jz short terrain_draw_view__L25f0
    jnc short terrain_draw_view__L25cb

terrain_draw_view__L25be:
    inc dl
    call near terrain_project_corner
    cmp dl,[terrain_previous_row_right]
    jnz short terrain_draw_view__L25be
    jmp short terrain_draw_view__L25f0

terrain_draw_view__L25cb:
    xor byte [terrain_projected_row_bank],0x20
    inc byte [terrain_scan_y]
    mov dl,[terrain_previous_row_right]

terrain_draw_view__L25d8:
    inc dl
    call near terrain_project_corner
    cmp dl,[terrain_row_right]
    jnz short terrain_draw_view__L25d8
    mov [terrain_previous_row_right],dl
    dec byte [terrain_scan_y]
    xor byte [terrain_projected_row_bank],0x20

terrain_draw_view__L25f0:
    call near terrain_draw_visible_row
    test byte [terrain_query_flags],0x80
    jz short terrain_draw_view__L2601
    test byte [terrain_draw_stop],0xff
    jnz short terrain_draw_view__L2604

terrain_draw_view__L2601:
    jmp near terrain_draw_view__L2547

terrain_draw_view__L2604:
    stc
    ret

terrain_draw_view__L2606:
    mov dl,[terrain_previous_row_left]
    inc dl
    cmp dl,[view_grid_x]
    jnz short terrain_draw_view__L2618
    mov [terrain_previous_row_right],dl
    jmp short terrain_draw_view__L262c

terrain_draw_view__L2618:
    mov dl,[terrain_previous_row_right]
    dec dl
    dec dl
    cmp dl,[view_grid_x]
    jnz short terrain_finish_near_row
    inc dl
    mov [terrain_previous_row_left],dl

terrain_draw_view__L262c:
    mov dl,[terrain_previous_row_left]
    call near terrain_project_corner
    mov dl,[terrain_previous_row_right]
    call near terrain_project_corner
    call near terrain_draw_visible_row

; Finish the row nearest the viewing cell, including the boundary/cap face when required. [load+0x263d]
terrain_finish_near_row:
    mov byte [terrain_projected_row_bank],0x0
    inc byte [terrain_scan_y]
    mov dl,[view_grid_x]
    call near terrain_project_corner
    mov_load si, dx
    mov al,[si+projected_y_high]
    cmp al,0x2
    jnc short terrain_finish_near_row__L2691
    mov [si+projected_y_high+0x1],al
    mov al,[si+projected_y_low]
    mov [si+projected_y_low+0x1],al
    mov byte [terrain_projected_row_bank],0x20
    dec byte [terrain_scan_y]
    mov al,0xff
    mov [si+projected_y_high+0x20],al
    mov [si+projected_y_high+0x21],al
    mov [si+projected_x_high+0x20],al
    mov [si+projected_x_high],al
    mov al,0x14
    mov [si+projected_x_high+0x21],al
    mov [si+projected_x_high+0x1],al
    mov al,[view_grid_x]
    mov [terrain_cell_x],al
    call near terrain_draw_checker_quad

terrain_finish_near_row__L2691:
    clc
    ret

; Walk projected grid corners to find the row interval that intersects the current viewport. [load+0x2693]
terrain_find_visible_row_interval:
    mov dl,[terrain_row_left]
    call near terrain_project_corner
    jz short terrain_find_visible_row_interval__L26d4
    cmp al,0x80
    jz short terrain_find_visible_row_interval__L26c1

terrain_find_visible_row_interval__L26a0:
    mov al,[terrain_scan_x]
    mov [terrain_row_left],al
    call near terrain_project_next_corner
    jc short terrain_find_visible_row_interval__L26ba
    cmp al,0x81
    jz short terrain_find_visible_row_interval__L26a0
    cmp al,0x80
    jz short terrain_find_visible_row_interval__L26ba

terrain_find_visible_row_interval__L26b3:
    call near terrain_project_next_corner
    jc short terrain_find_visible_row_interval__L26ba
    jz short terrain_find_visible_row_interval__L26b3

terrain_find_visible_row_interval__L26ba:
    mov al,[terrain_scan_x]
    mov [terrain_row_right],al
    ret

terrain_find_visible_row_interval__L26c1:
    mov al,[terrain_scan_x]
    mov [terrain_row_right],al
    call near terrain_project_previous_corner
    jc short terrain_find_visible_row_interval__L26ee
    cmp al,0x80
    jz short terrain_find_visible_row_interval__L26c1
    cmp al,0x0
    jmp short terrain_find_visible_row_interval__L26ec

terrain_find_visible_row_interval__L26d4:
    call near terrain_project_next_corner
    jc short terrain_find_visible_row_interval__L26db
    jz short terrain_find_visible_row_interval__L26d4

terrain_find_visible_row_interval__L26db:
    mov al,[terrain_scan_x]
    mov [terrain_row_right],al
    mov al,[terrain_row_left]
    mov [terrain_scan_x],al

terrain_find_visible_row_interval__L26e7:
    call near terrain_project_previous_corner
    jc short terrain_find_visible_row_interval__L26ee

terrain_find_visible_row_interval__L26ec:
    jz short terrain_find_visible_row_interval__L26e7

terrain_find_visible_row_interval__L26ee:
    mov al,[terrain_scan_x]
    mov [terrain_row_left],al
    ret

; Project the corner to the left of terrain_scan_x, or return carry at the grid boundary. [load+0x26f5]
terrain_project_previous_corner:
    mov dl,[terrain_scan_x]
    or_load dl, dl
    jz short terrain_corner_out_of_grid
    dec dl
    jmp short terrain_project_corner
    nop

; Return carry at the horizontal grid boundary. [load+0x2702]
terrain_corner_out_of_grid:
    stc
    ret

; Project the corner to the right, or return carry at column 32. [load+0x2704]
terrain_project_next_corner:
    mov dl,[terrain_scan_x]
    inc dl
    cmp dl,0x20
    jz short terrain_corner_out_of_grid

; Terrain projection uses the generated grid, not a stored preview image. X/Y caches alternate between two 32-corner row banks.

; DL: corner column. Rotate to world-grid coordinates, fetch height/object indirection, compute bearing/distance/vertical angle and cache X/Y plus clip status. [load+0x270f]
terrain_project_corner:
    mov [terrain_scan_x],dl
    mov [terrain_corner_column],dl
    mov_load al, dl
    or al,[terrain_projected_row_bank]
    mov [projected_vertex_slot],al
    mov byte [projection_clip_status],0x0
    mov bl,[view_object_slot]
    mov byte [projection_abs_delta_x],0x80
    mov al,[terrain_scan_x]
    dec al
    sub al,[view_grid_x]
    sub al,[view_lateral_offset]
    sub al,[view_grid_x_overflow]
    mov [projection_delta_x_sign],al
    mov [projection_delta_x_high],al
    jns short terrain_project_corner__L274b
    neg word [projection_abs_delta_x]

terrain_project_corner__L274b:
    mov byte [projection_abs_delta_y],0x80
    mov al,[terrain_scan_y]
    dec al
    sub al,[view_grid_y]
    mov [projection_delta_y_sign],al
    mov [projection_delta_y_high],al
    jns short terrain_project_corner__L2765
    neg word [projection_abs_delta_y]

terrain_project_corner__L2765:
    call near projection_bearing_from_deltas
    mov dl,[projected_vertex_slot]
    mov_load si, dx
    mov ax,[projection_angle]
    sub ax,[terrain_azimuth_bias]
    mov [si+projected_x_low],al
    mov [si+projected_x_high],ah
    call near projection_planar_distance
    mov dl,[projected_vertex_slot]
    mov_load si, dx
    mov ax,[projection_distance]
    mov [si+terrain_corner_distance],ah
    mov al,[terrain_view_rotation]
    test al,0x80
    jnz short terrain_project_corner__L27ae
    test al,0x40
    jnz short terrain_project_corner__L27a2
    mov bl,[terrain_scan_x]
    mov dl,[terrain_scan_y]
    jmp short terrain_fetch_rotated_cell

terrain_project_corner__L27a2:
    mov bl,[terrain_scan_y]
    mov dl,0x1f
    sub dl,[terrain_scan_x]
    jmp short terrain_fetch_rotated_cell

terrain_project_corner__L27ae:
    test al,0x40
    jnz short terrain_project_corner__L27c0
    mov bl,0x1f
    sub bl,[terrain_scan_x]
    mov dl,0x1f
    sub dl,[terrain_scan_y]
    jmp short terrain_fetch_rotated_cell

terrain_project_corner__L27c0:
    mov bl,0x1f
    sub bl,[terrain_scan_y]
    mov dl,[terrain_scan_x]

; Convert rotated X/Y into the four-bank packed terrain address and fetch the cell. [load+0x27ca]
terrain_fetch_rotated_cell:
    mov_load ax, bx
    shl al,1
    shl al,1
    shl al,1
    and al,0xe0
    or_load al, dl
    mov_load dl, al
    mov_load di, dx
    mov_load ah, bl
    and ax,0x300
    mov_load cl, ah
    add ax,terrain_grid
    mov_load bp, ax
    mov al,[bp+di]
    mov bl,[projected_vertex_slot]
    mov [bx+terrain_cached_cells],al
    cmp al,OBJECT_CELL_TAG_BASE
    jc short terrain_extract_grid_height

; Special cell C0h+ follows the six-bit object stack to obtain height. [load+0x27f4]
terrain_resolve_object_height:
    and al,0x3f
    mov_load dl, al
    mov_load si, dx
    mov al,[si+object_stack_links]
    cmp al,0x40
    jnc short terrain_resolve_object_height
    mov al,[si+object_height_integer]
    mov [projection_delta_x_high],al
    jmp short terrain_project_corner_height

; Normal cell: high nibble is height; visibility bits determine whether its face remains drawable. [load+0x280b]
terrain_extract_grid_height:
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    mov [projection_delta_x_high],al
    shr dl,1
    rcl cl,1
    mov_load si, dx
    mov al,[si+terrain_visibility_bits]
    mov_load dl, cl
    mov_load si, dx
    and al,[si+terrain_visible_bit_masks]
    jnz short terrain_project_corner_height
    mov [bx+terrain_cached_cells],al

; Subtract viewing-object height and project the vertical angle into the Y cache. [load+0x282e]
terrain_project_corner_height:
    mov bl,[view_object_slot]
    mov byte [projection_abs_delta_x],0x0
    mov ah,[bx+object_height_integer]
    mov al,[bx+object_height_fraction]
    sub [projection_abs_delta_x],ax
    call near projection_vertical_angle
    mov dl,[projected_vertex_slot]
    mov_load si, dx
    mov ax,[projection_screen_y]
    mov [si+projected_y_high],ah
    mov [si+projected_y_low],al
    mov al,[si+projected_x_high]
    cmp al,[viewport_projected_x_limit]
    jc short terrain_return_corner_clip_status
    jnz short terrain_project_corner_height__L2871
    mov al,[si+projected_x_low]
    cmp al,[viewport_x_fraction_limit]
    jc short terrain_return_corner_clip_status
    mov al,[si+projected_x_high]

terrain_project_corner_height__L2871:
    or byte [projection_clip_status],0x80
    cmp al,[viewport_negative_x_threshold]
    jc short terrain_return_corner_clip_status
    inc byte [projection_clip_status]

; Return the original column in DL and cached clipping status in AL/flags. [load+0x2880]
terrain_return_corner_clip_status:
    mov dl,[terrain_corner_column]
    mov al,[projection_clip_status]
    or_load al, al
    ret

; Draw row cells in two runs around the viewing column, preserving the required visibility order. [load+0x288a]
terrain_draw_visible_row:
    mov al,[terrain_previous_row_left]
    mov [terrain_cell_x],al

terrain_draw_visible_row__L2890:
    cmp al,[terrain_previous_row_right]
    jnc short terrain_draw_row_from_right__L28c6
    cmp al,[view_grid_x]
    jnc short terrain_draw_row_from_right
    call near terrain_draw_cell
    inc byte [terrain_cell_x]
    mov al,[terrain_cell_x]
    jmp short terrain_draw_visible_row__L2890

; Draw the right side of the visible row back toward the viewing column. [load+0x28a8]
terrain_draw_row_from_right:
    mov al,[terrain_previous_row_right]

terrain_draw_row_from_right__L28ab:
    dec al
    js short terrain_draw_row_from_right__L28c6
    mov [terrain_cell_x],al
    cmp al,[terrain_previous_row_left]
    jc short terrain_draw_row_from_right__L28c6
    cmp al,[view_grid_x]
    jc short terrain_draw_row_from_right__L28c6
    call near terrain_draw_cell
    mov al,[terrain_cell_x]
    jmp short terrain_draw_row_from_right__L28ab

terrain_draw_row_from_right__L28c6:
    ret

; AL: viewport preset. Set pixel X bounds, projected X clip thresholds and projection origin bias. [load+0x28c7]
viewport_select_preset:
    mov [viewport_preset],al
    mov_load bx, ax
    mov al,[bx+viewport_x_limit_table]
    mov [viewport_projected_x_limit],al
    shr al,1
    xor al,0x80
    mov [viewport_negative_x_threshold],al
    mov al,[bx+viewport_origin_high_table]
    mov [projection_x_bias_high],al
    add_load bx, bx
    mov ax,[bx+viewport_width_table]
    mov [viewport_pixel_width],ax
    mov ax,[bx+viewport_x_min_table]
    mov [viewport_x_min],ax
    add ax,[viewport_pixel_width]
    mov [viewport_x_end],ax
    xor_load ax, ax
    mov [viewport_x_fraction_limit],al
    mov [projection_x_bias],al
    ret

; AL: width in eight-pixel units. Build a custom viewport with a zero X origin. [load+0x2901]
viewport_set_dynamic_width:
    mov_load cl, al
    shr al,1
    mov [viewport_projected_x_limit],al
    mov al,0x0
    rcr al,1
    mov [viewport_x_fraction_limit],al
    mov al,[viewport_projected_x_limit]
    shr al,1
    xor al,0x80
    mov [viewport_negative_x_threshold],al
    mov_load al, cl
    xor_load ah, ah
    shl al,1
    shl al,1
    shl al,1
    mov [viewport_pixel_width],ax
    mov [viewport_x_end],ax
    xor_load ax, ax
    mov [viewport_x_min],ax
    mov [projection_x_bias],ax
    mov al,0x2
    mov [viewport_preset],al

viewport_set_dynamic_width__L2936:
    ret

; Terrain-text rendering: font bit pairs 01/10/11 select MODEL_TEXT_BLOCK_01/10/11 in reserved slot 3Fh.

; Read the cached cell shape and draw its model representation from reserved object slot 3Fh. [load+0x2937]
terrain_draw_cell_as_model:
    mov al,[bx+terrain_cached_cells]
    and al,0xf
    mov [terrain_model_shape],al
    jz short viewport_set_dynamic_width__L2936
    mov al,[terrain_cell_x]
    mov [terrain_model_cell_x],al
    mov al,[terrain_scan_y]
    mov [terrain_model_cell_y],al
    mov dl,0x3f
    mov_load di, dx
    jmp near model_draw_mesh

; Use cached height/shape and distance shading to draw a quad, two triangles, or a linked stack of model objects. [load+0x2955]
terrain_draw_cell:
    mov al,[terrain_cell_x]
    or al,[terrain_projected_row_bank]
    add al,[terrain_corner_bias]
    and al,0x3f
    mov_load bl, al
    cmp byte [terrain_shading_mode],0x0
    jnz short terrain_draw_cell__L2971
    mov ah,[terrain_rows_remaining]
    jmp short terrain_draw_cell__L299e

terrain_draw_cell__L2971:
    cmp byte [terrain_shading_mode],0x1
    jnz short terrain_draw_cell__L2993
    mov ah,[bx+terrain_corner_distance]
    cmp ah,0x10
    ja short terrain_draw_cell__L2985
    mov ah,0x10
    jmp short terrain_draw_cell__L298c

terrain_draw_cell__L2985:
    cmp ah,0x4e
    jna short terrain_draw_cell__L298c
    mov ah,0x4e

terrain_draw_cell__L298c:
    sub ah,0x10
    shr ah,1
    jmp short terrain_draw_cell__L299e

terrain_draw_cell__L2993:
    mov ah,[bx+terrain_corner_distance]
    cmp ah,0x20
    jc short terrain_draw_cell__L299e
    mov ah,0x1f

terrain_draw_cell__L299e:
    shl ah,1
    shl ah,1
    shl ah,1
    and ah,0xf0
    mov [raster_shade_bank],ah
    test byte [terrain_model_render_flags],0x80
    jnz short terrain_draw_cell_as_model
    mov al,[bx+terrain_cached_cells]
    or_load al, al
    jnz short terrain_draw_cell__L29bd
    jmp near viewport_set_dynamic_width__L2936

terrain_draw_cell__L29bd:
    cmp al,0xc0
    jc short terrain_draw_cell__L29c9
    push ax
    call near terrain_draw_checker_quad
    pop ax
    jmp near terrain_draw_linked_object_stack

terrain_draw_cell__L29c9:
    and al,0xf
    jz short terrain_draw_checker_quad
    cmp al,0xc
    jz short terrain_draw_cell__L29d5
    cmp al,0x4
    jnz short terrain_emit_quad__L2a0a

terrain_draw_cell__L29d5:
    mov_load cl, al
    mov al,[terrain_triangle_view_bias]
    and al,0x1
    mov [terrain_triangle_orientation],al
    mov_load al, cl
    or_load al, al
    jnz short terrain_emit_two_triangles

; Choose the checkerboard face colors from cell parity and enter polygon scan conversion. [load+0x29e5]
terrain_draw_checker_quad:
    xor_load bx, bx
    mov al,[terrain_cell_x]
    xor al,[terrain_scan_y]
    and al,0x1
    jz short terrain_emit_quad
    mov bl,0x8

; BX is exactly 8 on this odd-checkerboard branch. These indexed operands address PRNG state at DS:1490 and its snapshot at DS:1473; they do not access sound or model-render flags in this phase.
    mov al,[bx+prng_state_middle-8]
    mov [bx+terrain_checker_prng_snapshot-8],al

; Use a four-corner closed loop and scan-convert the quad. [load+0x29fc]
terrain_emit_quad:
    mov al,[bx+terrain_face_color_pairs]
    mov [polygon_packed_colors],al
    mov byte [polygon_face_flags],0x0
    jmp short polygon_draw_face

terrain_emit_quad__L2a0a:
    mov_load bl, al
    sub al,[terrain_face_rotation_bias]
    and al,0xf
    mov_load dl, al
    mov_load di, dx
    and al,0x3
    cmp al,0x1
    jz short terrain_emit_quad
    mov al,[di+terrain_triangle_orientations]
    mov [terrain_triangle_orientation],al
    mov_load cl, bl
    and cl,0x4
    shr cl,1
    shr cl,1
    add cl,[terrain_triangle_view_bias]
    mov_load al, bl
    cmp cl,0x2
    jnc short terrain_emit_two_triangles
    or al,0x10

; Pick orientation-dependent colors and emit both triangles of a sloping cell. [load+0x2a39]
terrain_emit_two_triangles:
    mov [terrain_triangle_shape],al
    mov_load bl, al
    mov byte [polygon_face_flags],0x80
    mov al,[bx+terrain_face_color_pairs]
    mov [polygon_packed_colors],al
    call near polygon_draw_face
    mov bl,[terrain_triangle_shape]
    xor bl,0x10
    mov al,[bx+terrain_face_color_pairs]
    mov [polygon_packed_colors],al
    or byte [polygon_face_flags],0x40

; Renderer bridge: projected polygon -> clipped left/right boundary arrays -> selected CGA/Tandy/EGA/VGA span filler.

; Build clipped span buffers; if not rejected, call graphics_fill_polygon_spans. [load+0x2a60]
polygon_draw_face:
    call near polygon_build_clipped_spans
    jc short polygon_draw_face__L2a69
    call word near [graphics_fill_polygon_spans]

polygon_draw_face__L2a69:
    ret

; DS:10DA is phase-dependent storage: smoothing passes here, random-placement retries later, and background scatter passes in scene setup. Keep those scoped names distinct.

; Construct and smooth the 32x32 terrain and cell types. Success can redirect the outer return into landscape_preview_return_dispatch. [load+0x2a6a]
landscape_generate_terrain:
    mov si,0x50

landscape_generate_terrain__L2a6d:
    call near prng_next_byte
    mov [si+terrain_filter_work],al
    dec si
    jns short landscape_generate_terrain__L2a6d
    mov al,[landscape_nonzero]
    or_load al, al
    jnz short landscape_generate_terrain__L2a82
    mov al,0x18
    jmp short landscape_generate_terrain__L2a87

landscape_generate_terrain__L2a82:
    call near prng_small_sum
    add al,0xe

landscape_generate_terrain__L2a87:
    mov [landscape_height_scale],al
    mov al,0x80
    call near terrain_transform_grid
    xor_load ax, ax
    call near terrain_smooth_grid_axes
    mov al,0x1
    call near terrain_transform_grid
    mov al,0x40
    call near terrain_smooth_grid_axes
    mov byte [terrain_scan_y],0x1e

landscape_generate_terrain__L2aa3:
    mov byte [terrain_scan_x],0x1e

landscape_generate_terrain__L2aa8:
    call near terrain_classify_four_corners
    call near terrain_address_cell
    mov_load al, cl
    shl al,1
    shl al,1
    shl al,1
    shl al,1
    or [bp+di],al
    dec byte [terrain_scan_x]
    jns short landscape_generate_terrain__L2aa8
    dec byte [terrain_scan_y]
    jns short landscape_generate_terrain__L2aa3
    mov al,0x2
    call near terrain_transform_grid
    ret

; AL: generation mode. Clear, randomize, convert noise into bounded heights, or swap height/shape nibbles over the full grid. [load+0x2acc]
terrain_transform_grid:
    mov [terrain_generation_mode],al
    mov byte [terrain_scan_y],0x1f

terrain_transform_grid__L2ad4:
    mov byte [terrain_scan_x],0x1f

terrain_transform_grid__L2ad9:
    call near terrain_address_cell
    mov al,[terrain_generation_mode]
    or_load al, al
    jz short terrain_transform_grid__L2b2c
    js short terrain_transform_grid__L2b29
    shr al,1
    jc short terrain_transform_grid__L2b03
    mov al,[bp+di]
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    mov_load bx, ax
    mov al,[bp+di]
    shl al,1
    shl al,1
    shl al,1
    shl al,1
    or_load ax, bx
    jmp short terrain_transform_grid__L2b2c

terrain_transform_grid__L2b03:
    mov al,[bp+di]
    sub al,0x80
    pushf
    jns short terrain_transform_grid__L2b0c
    neg al

terrain_transform_grid__L2b0c:
    mov [math_work_hi],al
    mov al,[landscape_height_scale]
    call near math_multiply_bytes_split_result
    popf
    call near math_negate_work_if_negative
    add al,0x6
    jns short terrain_transform_grid__L2b1f
    xor_load al, al

terrain_transform_grid__L2b1f:
    inc al
    cmp al,0xc
    jc short terrain_transform_grid__L2b2c
    mov al,0xb
    jmp short terrain_transform_grid__L2b2c

terrain_transform_grid__L2b29:
    call near prng_next_byte

terrain_transform_grid__L2b2c:
    mov [bp+di],al
    dec byte [terrain_scan_x]
    jns short terrain_transform_grid__L2ad9
    dec byte [terrain_scan_y]
    jns short terrain_transform_grid__L2ad4
    ret

; AL: filter flags. Make two passes over all rows and columns using terrain_filter_scanline. [load+0x2b3b]
terrain_smooth_grid_axes:
    mov [terrain_smoothing_flags],al
    mov byte [terrain_smoothing_passes],0x2

terrain_smooth_grid_axes__L2b43:
    mov byte [terrain_scan_y],0x1f

terrain_smooth_grid_axes__L2b48:
    xor_load ax, ax
    call near terrain_filter_scanline
    dec byte [terrain_scan_y]
    jns short terrain_smooth_grid_axes__L2b48
    mov byte [terrain_scan_x],0x1f

terrain_smooth_grid_axes__L2b58:
    mov al,0x80
    call near terrain_filter_scanline
    dec byte [terrain_scan_x]
    jns short terrain_smooth_grid_axes__L2b58
    dec byte [terrain_smoothing_passes]
    jnz short terrain_smooth_grid_axes__L2b43
    ret

; Resolve the current X/Y into BP:DI within the 32x32 terrain array; return its packed byte in AL. [load+0x2b6a]
terrain_address_cell:
    mov al,[terrain_scan_x]
    shl ax,1
    shl ax,1
    shl ax,1
    and ax,0xe0
    or al,[terrain_scan_y]
    mov_load di, ax
    mov ah,[terrain_scan_x]
    and ax,0x300
    add ax,terrain_grid
    mov_load bp, ax
    mov al,[bp+di]
    cmp al,OBJECT_CELL_TAG_BASE
    cmc
    ret

; Filter a terrain row/column. The second smoothing pass can patch the startup return address to the landscape preview. [load+0x2b8e]
terrain_filter_scanline:
    or al,[terrain_smoothing_flags]
    mov [terrain_generation_mode],al
    mov si,0x22

terrain_filter_scanline__L2b98:
    mov_load ax, si
    and al,0x1f
    test byte [terrain_generation_mode],0x80
    jz short terrain_filter_scanline__L2ba8
    mov [terrain_scan_y],al
    jmp short terrain_filter_scanline__L2bab

terrain_filter_scanline__L2ba8:
    mov [terrain_scan_x],al

terrain_filter_scanline__L2bab:
    call near terrain_address_cell
    mov [si+terrain_filter_work],al
    dec si
    jns short terrain_filter_scanline__L2b98
    test byte [terrain_generation_mode],0x40
    jz short terrain_filter_scanline__L2c13
    mov si,0x1f

terrain_filter_scanline__L2bbf:
    mov al,[si+terrain_filter_work+0x1]
    cmp al,[si+terrain_filter_work+0x2]
    jz short terrain_filter_scanline__L2bfd
    jnc short terrain_filter_scanline__L2bdd
    cmp al,[si+terrain_filter_work]
    jz short terrain_filter_scanline__L2bfd
    jnc short terrain_filter_scanline__L2bfd
    mov al,[si+terrain_filter_work+0x2]
    cmp al,[si+terrain_filter_work]
    jmp short terrain_filter_scanline__L2bed

terrain_filter_scanline__L2bdd:
    cmp al,[si+terrain_filter_work]
    jz short terrain_filter_scanline__L2bfd
    jc short terrain_filter_scanline__L2bfd
    mov al,[si+terrain_filter_work]
    cmp al,[si+terrain_filter_work+0x2]

terrain_filter_scanline__L2bed:
    jc short terrain_filter_scanline__L2bf5
    mov al,[si+terrain_filter_work]
    jmp short terrain_filter_scanline__L2bf9

terrain_filter_scanline__L2bf5:
    mov al,[si+terrain_filter_work+0x2]

terrain_filter_scanline__L2bf9:
    mov [si+terrain_filter_work+0x1],al

terrain_filter_scanline__L2bfd:
    dec si
    jns short terrain_filter_scanline__L2bbf
    test byte [landscape_generation_flags],0x80
    jnz short terrain_filter_scanline__L2c11

; Patch the outstanding startup call return, not the innermost terrain-filter return.
    mov bx,[saved_hardware_sp]
    mov ax,(landscape_preview_return_dispatch-program_image)
    mov [bx-0x2],ax

terrain_filter_scanline__L2c11:
    jmp short terrain_filter_scanline__L2c45

terrain_filter_scanline__L2c13:
    xor_load si, si

terrain_filter_scanline__L2c15:
    xor_load ax, ax
    mov_load bx, ax
    mov al,[si+terrain_filter_work]
    add al,[si+terrain_filter_work+0x1]
    adc_load ah, bh
    add al,[si+terrain_filter_work+0x2]
    adc_load ah, bh
    add al,[si+terrain_filter_work+0x3]
    adc_load ah, bh
    shr ax,1
    shr ax,1
    mov [si+terrain_filter_work],al
    inc si
    cmp si,0x20
    jc short terrain_filter_scanline__L2c15
    mov al,[si+terrain_filter_work+0x2e]
    mov [si+terrain_generation_saved_noise-32],al

terrain_filter_scanline__L2c45:
    mov si,0x1f

terrain_filter_scanline__L2c48:
    mov_load ax, si
    test byte [terrain_generation_mode],0x80
    jz short terrain_filter_scanline__L2c56
    mov [terrain_scan_y],al
    jmp short terrain_filter_scanline__L2c59

terrain_filter_scanline__L2c56:
    mov [terrain_scan_x],al

terrain_filter_scanline__L2c59:
    call near terrain_address_cell
    mov al,[si+terrain_filter_work]
    mov [bp+di],al
    dec si
    jns short terrain_filter_scanline__L2c48
    ret

; Four-corner classification workspace: A=(x,y) at DS:110A, B=(x+1,y) at 110D, C=(x,y+1) at 110B, D=(x+1,y+1) at 110C. C/D reuse the arithmetic low/high byte pair.

; Read four neighboring heights and return the tile slope/triangle classification in CL. [load+0x2c66]
terrain_classify_four_corners:
    mov bx,0xf
    call near terrain_address_cell
    and_load al, bl
    mov [terrain_corner_height_a],al
    inc byte [terrain_scan_x]
    call near terrain_address_cell
    and_load al, bl
    mov [terrain_corner_height_b],al
    inc byte [terrain_scan_y]
    call near terrain_address_cell
    and_load al, bl
    mov [terrain_corner_height_d],al
    dec byte [terrain_scan_x]
    call near terrain_address_cell
    and_load al, bl
    mov [terrain_corner_height_c],al
    dec byte [terrain_scan_y]
    mov al,[terrain_corner_height_a]
    cmp al,[terrain_corner_height_b]
    jz short terrain_classify_four_corners__L2ced
    cmp al,[terrain_corner_height_c]
    jz short terrain_classify_four_corners__L2cc5
    mov al,[terrain_corner_height_d]
    cmp al,[terrain_corner_height_b]
    jz short terrain_classify_four_corners__L2cb4

terrain_classify_four_corners__L2cb1:
    mov cl,0xc
    ret

terrain_classify_four_corners__L2cb4:
    cmp al,[terrain_corner_height_c]
    jnz short terrain_classify_four_corners__L2cd4
    mov cl,0x2
    cmp al,[terrain_corner_height_a]
    jnc short terrain_classify_four_corners__L2cc4
    mov cl,0xb

terrain_classify_four_corners__L2cc4:
    ret

terrain_classify_four_corners__L2cc5:
    mov al,[terrain_corner_height_d]
    cmp al,[terrain_corner_height_b]
    jz short terrain_classify_four_corners__L2ce2
    cmp al,[terrain_corner_height_c]
    jz short terrain_classify_four_corners__L2cd7

terrain_classify_four_corners__L2cd4:
    mov cl,0x4
    ret

terrain_classify_four_corners__L2cd7:
    mov cl,0xe
    cmp al,[terrain_corner_height_b]
    jc short terrain_classify_four_corners__L2ce1
    mov cl,0x7

terrain_classify_four_corners__L2ce1:
    ret

terrain_classify_four_corners__L2ce2:
    mov cl,0x5
    cmp al,[terrain_corner_height_c]
    jc short terrain_classify_four_corners__L2cec
    mov cl,0xd

terrain_classify_four_corners__L2cec:
    ret

terrain_classify_four_corners__L2ced:
    cmp al,[terrain_corner_height_c]
    jz short terrain_classify_four_corners__L2d18
    mov al,[terrain_corner_height_d]
    cmp al,[terrain_corner_height_c]
    jz short terrain_classify_four_corners__L2d0d
    cmp al,[terrain_corner_height_b]
    jnz short terrain_classify_four_corners__L2cb1
    mov cl,0x6
    cmp al,[terrain_corner_height_c]
    jc short terrain_classify_four_corners__L2d0c
    mov cl,0xf

terrain_classify_four_corners__L2d0c:
    ret

terrain_classify_four_corners__L2d0d:
    mov cl,0x1
    cmp al,[terrain_corner_height_b]
    jc short terrain_classify_four_corners__L2d17
    mov cl,0x9

terrain_classify_four_corners__L2d17:
    ret

terrain_classify_four_corners__L2d18:
    cmp al,[terrain_corner_height_d]
    jz short terrain_classify_four_corners__L2d25
    mov cl,0xa
    jc short terrain_classify_four_corners__L2d24
    mov cl,0x3

terrain_classify_four_corners__L2d24:
    ret

terrain_classify_four_corners__L2d25:
    xor_load cx, cx
    ret

; Reviewed instruction range 2D28h..346Dh; internal routine roles remain partial. [load+0x2d28]
recovered_code_2d28:
    test byte [hyperspace_outcome_flags],0xff
    jns short recovered_code_2d28__L2d32
    jmp near recovered_code_2d28__L2dcd

recovered_code_2d28__L2d32:
    xor_load al, al
    mov [0x10cd],al
    mov cx,0x80
    mov di,terrain_visibility_bits
    rep stosb
    call near occlusion_build_height_rows
    mov byte [edge_end_y],0x1f
    call near occlusion_trace_grid_row
    dec byte [edge_end_y]

recovered_code_2d28__L2d4e:
    xor byte [0x10cd],0x20
    call near occlusion_trace_grid_row
    mov bl,[player_robot_slot]
    mov al,[bx+0x116a]
    mov [0x110d],al
    mov bl,0x1e
    mov bp,[occlusion_height_row_pointer]

recovered_code_2d28__L2d67:
    mov_load di, bx
    mov al,[bp+di]
    mov ch,0xff
    shr al,1
    jc short recovered_code_2d28__L2d7b
    cmp al,[0x110d]
    jc short recovered_code_2d28__L2d7b
    jz short recovered_code_2d28__L2d7b
    xor_load ch, ch

recovered_code_2d28__L2d7b:
    mov_load cl, bl
    shl cl,1
    shl cl,1
    shl cl,1
    and cl,0xe0
    or cl,[edge_end_y]
    mov_load dl, bl
    and dl,0x3
    shr cl,1
    rcl dl,1
    mov_load di, dx
    mov ah,[di+0x5b26]
    not ah
    mov al,[bx+terrain_cached_cells]
    or al,[bx+0x647]
    or al,[bx+0x666]
    or al,[bx+0x667]
    and_load al, ch
    and al,[di+0x5b26]
    mov_load si, cx
    and si,0xff
    mov ch,[si+0x36ac]
    and_load ch, ah
    or_load al, ch
    mov [si+0x36ac],al
    dec bl
    jns short recovered_code_2d28__L2d67
    dec byte [edge_end_y]
    jns short recovered_code_2d28__L2d4e

recovered_code_2d28__L2dcd:
    ret

; Test all 32 columns of one terrain row against sampled height thresholds and record FFh/00h visibility in the alternating terrain_cached_cells bank. [load+0x2dce]
occlusion_trace_grid_row:
    mov ah,[edge_end_y]
    xor_load al, al
    add ax,occlusion_height_rows
    mov [occlusion_height_row_pointer],ax
    mov byte [edge_end_x],0x1f

occlusion_trace_grid_row__L2ddf:
    mov byte [0x110b],0x0
    mov byte [0x10db],0xff
    mov dl,[edge_end_x]
    mov_load di, dx
    mov bp,[occlusion_height_row_pointer]
    mov al,[bp+di]
    shr al,1
    mov [0x10dc],al
    mov bl,[player_robot_slot]
    call near ray_initialize_from_object
    xor_load al, al
    mov ah,[edge_end_x]
    sub ax,[ray_position_x+1]
    mov [occlusion_ray_step_x],ax
    jns short occlusion_trace_grid_row__L2e12
    neg ax

occlusion_trace_grid_row__L2e12:
    cmp ah,[0x110b]
    jc short occlusion_trace_grid_row__L2e1c
    mov [0x110b],ah

occlusion_trace_grid_row__L2e1c:
    xor_load al, al
    mov ah,[0x10dc]
    sub ax,[ray_position_height+1]
    mov [occlusion_ray_step_height],ax
    jns short occlusion_trace_grid_row__L2e2d
    neg ax

occlusion_trace_grid_row__L2e2d:
    cmp ah,[0x110b]
    jc short occlusion_trace_grid_row__L2e37
    mov [0x110b],ah

occlusion_trace_grid_row__L2e37:
    xor_load al, al
    mov ah,[edge_end_y]
    sub ax,[ray_position_y+1]
    mov [occlusion_ray_step_y],ax
    jns short occlusion_trace_grid_row__L2e48
    neg ax

occlusion_trace_grid_row__L2e48:
    cmp ah,[0x110b]
    jc short occlusion_trace_grid_row__L2e52
    mov [0x110b],ah

occlusion_trace_grid_row__L2e52:
    mov bl,0xff
    mov cl,[0x10db]
    mov al,[0x110b]
    shl al,1
    shl al,1
    cmp al,0x6
    jnc short occlusion_trace_grid_row__L2e66
    jmp near occlusion_trace_grid_row__L2eea

occlusion_trace_grid_row__L2e66:
    shl word [occlusion_ray_step_x],1
    shl word [occlusion_ray_step_height],1
    shl word [occlusion_ray_step_y],1
    shr cl,1
    shl al,1
    jnc short occlusion_trace_grid_row__L2e66
    mov [0x10db],cl
    test byte [occlusion_reference_check_flags],0xff
    js short occlusion_trace_grid_row__L2eab

; BX=00FFh selects the saved seed at DS:1473. The reference check walks four secret-code match markers backward from (seed+29h)&FFh. The preceding marker corresponds to the input FFh terminator and must differ.
    mov al,[bx+generation_validation_seed-0xff]
    add al,0x29
    mov [edge_delta_y],al
    mov cx,0x4
    mov_load si, ax

occlusion_trace_grid_row__L2e91:
    and si,0xff
    cmp byte [si+generation_validation_ring],GENERATION_MATCH_MARKER
    jnz short occlusion_trace_grid_row__L2f09
    dec si
    loop occlusion_trace_grid_row__L2e91
    cmp byte [si+generation_validation_ring],GENERATION_MATCH_MARKER
    jz short occlusion_trace_grid_row__L2f09
    or byte [occlusion_reference_check_flags],0x80

occlusion_trace_grid_row__L2eab:
    mov bl,[0x10db]

occlusion_trace_grid_row__L2eaf:
    mov al,[occlusion_ray_step_x+1]
    cbw
    add [ray_position_x+1],ax
    mov al,[occlusion_ray_step_height]
    add [ray_position_height],al
    mov al,[occlusion_ray_step_height+1]
    cbw
    adc [ray_position_height+1],ax
    mov al,[occlusion_ray_step_y+1]
    cbw
    add [ray_position_y+1],ax
    mov ah,[ray_position_y+2]
    xor_load al, al
    add ax,occlusion_cell_threshold_rows
    mov_load bp, ax
    mov al,[ray_position_height+2]
    mov dl,[ray_position_x+2]
    mov_load di, dx
    cmp al,[bp+di]
    jc short occlusion_trace_grid_row__L2eee
    dec bl
    jnz short occlusion_trace_grid_row__L2eaf

occlusion_trace_grid_row__L2eea:
    mov cl,0xff
    jmp short occlusion_trace_grid_row__L2ef0

occlusion_trace_grid_row__L2eee:
    xor_load cl, cl

occlusion_trace_grid_row__L2ef0:
    mov al,[edge_end_x]
    or al,[0x10cd]
    mov_load dl, al
    mov_load di, dx
    mov [di+terrain_cached_cells],cl
    dec byte [edge_end_x]
    js short occlusion_trace_grid_row__L2f08
    jmp near occlusion_trace_grid_row__L2ddf

occlusion_trace_grid_row__L2f08:
    ret

occlusion_trace_grid_row__L2f09:
    jmp near hardware_leave_game

; Build 32 height-sample rows and 31 rows of cell thresholds for visibility ray tests; DS row stride is 256 bytes. [load+0x2f0c]
occlusion_build_height_rows:
    mov ax,occlusion_height_rows+(TERRAIN_SIDE-1)*OCCLUSION_ROW_STRIDE
    mov [occlusion_height_row_pointer],ax
    mov byte [ray_sample_mode_flags],0x0
    mov byte [0x10e4],0x1f

occlusion_build_height_rows__L2f1c:
    mov byte [0x10e2],0x1f

occlusion_build_height_rows__L2f21:
    call near recovered_sub_3302
    rcl al,1
    mov dl,[0x10e2]
    mov_load di, dx
    mov bp,[occlusion_height_row_pointer]
    mov [bp+di],al
    dec byte [0x10e2]
    jns short occlusion_build_height_rows__L2f21
    dec byte [occlusion_height_row_pointer_high]
    dec byte [0x10e4]
    jns short occlusion_build_height_rows__L2f1c
    mov bx,occlusion_height_rows+(TERRAIN_SIDE-2)*OCCLUSION_ROW_STRIDE
    mov_load bp, bx
    mov cx,0x1f

occlusion_build_height_rows__L2f4a:
    mov si,0x1e
    mov di,0x3e

occlusion_build_height_rows__L2f50:
    mov al,[bx+si]
    shr al,1
    jnc short occlusion_build_height_rows__L2f72
    rcl al,1
    inc si
    cmp al,[bx+si]
    jc short occlusion_build_height_rows__L2f5f
    mov al,[bx+si]

occlusion_build_height_rows__L2f5f:
    inc bh
    cmp al,[bx+si]
    jc short occlusion_build_height_rows__L2f67
    mov al,[bx+si]

occlusion_build_height_rows__L2f67:
    dec si
    cmp al,[bx+si]
    jc short occlusion_build_height_rows__L2f6e
    mov al,[bx+si]

occlusion_build_height_rows__L2f6e:
    dec bh
    shr al,1

occlusion_build_height_rows__L2f72:
    mov [bp+di],al
    dec di
    dec si
    jns short occlusion_build_height_rows__L2f50
    sub bp,0x100
    dec bh
    loop occlusion_build_height_rows__L2f4a
    xor_load bx, bx
    ret

; Dispatch create, absorb, transfer, hyperspace and turn-around commands. Object actions use the square hit by the cursor aiming ray. [load+0x2f83]
player_execute_action:
    mov al,[player_action_code]
    cmp al,ACTION_HYPERSPACE
    jnz short player_execute_action__L2fb4
    call near player_hyperspace
    mov byte [0x1444],0x0

player_execute_action__L2f92:
    stc
    ret

player_execute_action__L2f94:
    or byte [cursor_update_guard_flags],0x80
    call word near [graphics_restore_software_cursor]
    call near recovered_code_0dec
    jc short player_execute_action__L2fae
    call near recovered_sub_30c5
    mov byte [overview_pending],0x80
    mov al,0x19
    jmp short player_execute_action__L301d

player_execute_action__L2fae:
    call near recovered_sub_30c5
    jmp near player_execute_action__L3047

player_execute_action__L2fb4:
    cmp al,0x24
    jz short player_execute_action__L2f94
    mov bl,[0x1108]
    cmp al,ACTION_TURN_AROUND
    jnz short player_execute_action__L2fcf
    shl byte [turn_and_occlusion_flags],1
    jns short player_execute_action__L2f92
    xor byte [bx+object_heading],0x80
    mov al,MELODY_TURN_AROUND
    jmp short player_execute_action__L301d

player_execute_action__L2fcf:
    and byte [ray_target_test_flags],0x7f
    call near player_build_cursor_aiming_ray
    call near recovered_sub_3176
    jc short player_execute_action__L3047
    mov al,[player_action_code]
    and al,0x20
    jz short player_execute_action__L3058
    call near terrain_address_cell
    jnc short player_execute_action__L3047
    and al,0x3f
    mov_load bl, al
    mov al,[player_action_code]
    shr al,1
    jnc short player_execute_action__L3027

; Transfer (Q): the selected object must be a robot; change player_robot_slot to that robot.
    mov dl,[bx+object_model_id]
    mov_load di, dx
    or_load dl, dl

; MODEL_ROBOT is zero: the preceding zero test rejects transfer to any other model.
    jnz short player_execute_action__L3047
    call near recovered_sub_3e56
    mov [player_robot_slot],bl

player_execute_action__L3004:
    mov al,[bx+0x5c6]
    cmp al,0x40
    jc short player_execute_action__L301b
    and al,0x3f
    mov_load bl, al
    xor al,0x3f
    jnz short player_execute_action__L3004
    mov al,[bx+object_model_id]
    mov [secret_code_pair_work],al

player_execute_action__L301b:
    mov al,MELODY_TRANSFER

player_execute_action__L301d:
    call near sound_start_melody
    mov byte [player_view_refresh_flags],0x80
    stc
    ret

; Absorb (A): remove the selected object and credit its energy value; Meanie absorption clears its sentry link; the Sentinel pedestal is not absorbable.

player_execute_action__L3027:
    test byte [0x5c6],0xff
    js short player_execute_action__L3047
    mov al,[bx+object_model_id]
    cmp al,MODEL_MEANIE
    jz short player_absorb_meanie_clear_owner
    cmp al,MODEL_SENTINEL_PEDESTAL
    jz short player_execute_action__L3047

player_execute_action__L303a:
    call near recovered_sub_343d
    mov [0x10ca],bl
    clc
    call near player_adjust_energy_for_model
    clc
    ret

player_execute_action__L3047:
    mov byte [0xf4],0xaa
    mov al,0x5
    call near sound_start_effect
    mov byte [0xf4],0x90
    stc
    ret

; Create (R/T/B): allocate the selected robot/tree/boulder model, spend energy, and attach it to the selected square.

player_execute_action__L3058:
    call near object_allocate_saved_model_slot
    jc short player_execute_action__L3047
    stc
    call near player_adjust_energy_for_model
    jc short player_execute_action__L3047
    mov bl,[0x10ca]
    mov al,[ray_position_x+2]
    mov [0x10e2],al
    mov al,[ray_position_y+2]
    mov [0x10e4],al
    call near object_attach_to_terrain_cell
    jnc short player_execute_action__L307e
    clc
    call near player_adjust_energy_for_model
    jmp short player_execute_action__L3047

player_execute_action__L307e:
    test byte [bx+object_model_id],0xff
    jnz short player_execute_action__L3095
    mov dl,[player_robot_slot]
    mov_load di, dx
    mov al,[di+object_heading]
    xor al,0x80
    mov [bx+object_heading],al

player_execute_action__L3095:
    clc
    ret

; Inline absorption branch: find the owning active sentry/Sentinel and clear its Meanie link, then join ordinary object detachment and energy credit. Missing owners also join the common path. [load+0x3097]
player_absorb_meanie_clear_owner:
    mov dl,0x7
    mov_load di, dx

player_absorb_meanie_clear_owner__L309b:
    test byte [di+0x5c6],0xff
    js short player_absorb_meanie_clear_owner__L30bc
    mov al,[di+object_model_id]
    cmp al,MODEL_SENTRY
    jz short player_absorb_meanie_clear_owner__L30ae
    cmp al,MODEL_SENTINEL
    jnz short player_absorb_meanie_clear_owner__L30bc

player_absorb_meanie_clear_owner__L30ae:
    cmp bl,[di+sentry_meanie_slots]
    jnz short player_absorb_meanie_clear_owner__L30bc
    mov byte [di+sentry_meanie_slots],0x80
    jmp near player_execute_action__L303a

player_absorb_meanie_clear_owner__L30bc:
    dec dl
    mov_load di, dx
    jns short player_absorb_meanie_clear_owner__L309b
    jmp near player_execute_action__L303a

recovered_sub_30c5:
    test byte [cursor_mode_flags],0xff
    jns short recovered_sub_30c5__L30d0
    call word near [graphics_draw_software_cursor]

recovered_sub_30c5__L30d0:
    mov byte [cursor_update_guard_flags],0x0
    ret

; Combine the cursor position with the player robot heading/pitch to aim at a landscape square. [load+0x30d6]
player_build_cursor_aiming_ray:
    mov ah,[camera_cursor_x_pairs]
    xor_load al, al
    shr ax,1
    shr ax,1
    shr ax,1
    mov [ray_heading_angle],ax
    mov al,[bx+object_heading]
    sub al,0xa
    add [ray_heading_angle+1],al
    mov ah,[camera_cursor_inverted_y]
    xor_load al, al
    sub ax,0x500
    shr ax,1
    shr ax,1
    shr ax,1
    shr ax,1
    add ax,strict word 0x20
    mov [ray_pitch_angle],ax
    mov al,[bx+0x606]
    add al,0x3
    add [ray_pitch_angle+1],al
    mov ax,[ray_pitch_angle]
    mov [0x110b],al
    xchg ah,al

recovered_sub_3118:
    call near recovered_code_1b3f
    mov dl,0x2
    mov_load di, dx
    call near recovered_sub_3163
    mov [ray_horizontal_scale],ax
    xor_load dx, dx
    mov_load di, dx
    call near recovered_sub_3163
    mov [0x456],ax
    mov ax,[ray_heading_angle]
    mov [0x110b],al
    mov_load al, ah
    call near recovered_code_1b3f
    mov dl,0x2
    mov_load di, dx
    mov bl,0x4
    call near recovered_sub_3149
    xor_load dx, dx
    mov_load di, dx
    mov_load bx, dx

recovered_sub_3149:
    mov byte [0x1105],0x0
    mov ax,[ray_horizontal_scale]
    mov [0x47c],ax
    mov ax,[di+trig_component_a]
    mov [0x47a],ax
    call near math_multiply_encoded_signed
    mov [bx+0x454],ax
    ret

recovered_sub_3163:
    mov ax,[di+trig_component_a]
    shr ax,1
    pushf
    shr ax,1
    shr ax,1
    shr ax,1
    popf
    jnc short recovered_sub_3163__L3175
    neg ax

recovered_sub_3163__L3175:
    ret

recovered_sub_3176:
    mov bl,[0x1108]
    shr byte [ray_target_hit_flags],1
    shr byte [ray_intervening_hit_flags],1
    call near ray_initialize_from_object

recovered_sub_3176__L3185:
    xchg cx,dx
    mov ax,[0x454]
    cwd
    add [ray_position_x],ax
    adc [ray_position_x+2],dl
    mov ax,[0x456]
    cwd
    add [ray_position_height],ax
    adc [ray_position_height+2],dl
    mov ax,[0x458]
    cwd
    add [ray_position_y],ax
    adc [ray_position_y+2],dl
    xchg dx,cx
    mov al,[ray_position_x+2]
    mov [0x10e2],al
    cmp al,0x1f
    jnc short recovered_sub_3176__L3225
    mov al,[ray_position_y+2]
    mov [0x10e4],al
    cmp al,0x1f
    jnc short recovered_sub_3176__L3225
    mov al,0x80
    mov [ray_sample_mode_flags],al
    mov [edge_delta_y],al
    xor_load al, al
    mov [ray_sample_fraction],al
    mov [ray_sample_special_flags],al
    call near recovered_sub_3302
    jc short recovered_sub_3176__L3227
    mov ch,[ray_position_height+2]
    mov cl,[ray_position_height+1]
    sub [ray_sample_fraction],cl
    sbb_load al, ch
    test al,al
    js short recovered_sub_3176__L3185
    jnz short recovered_sub_3176__L3225
    mov al,[ray_sample_fraction]
    cmp al,[edge_delta_y]
    jnc short recovered_sub_3176__L3225
    test byte [ray_sample_mode_flags],0x40
    jnz short recovered_sub_3176__L3225
    mov al,[ray_target_test_flags]
    or al,[ray_sample_special_flags]
    js short recovered_sub_3176__L320a
    test byte [0x457],0xff
    jns short recovered_sub_3176__L3225

recovered_sub_3176__L320a:
    mov bl,[0x1108]
    mov al,[0x10e2]
    cmp al,[bx+object_grid_x]
    jnz short recovered_sub_3176__L3223
    mov al,[0x10e4]
    cmp al,[bx+object_grid_y]
    jnz short recovered_sub_3176__L3223
    jmp near recovered_sub_3176__L3185

recovered_sub_3176__L3223:
    clc
    ret

recovered_sub_3176__L3225:
    stc
    ret

; Ray contact heights form A,C,D,B,A at DS:110A..110E. The selector is an edge index and an X/Y/complement code. After loading edge endpoints, arithmetic reuses C/D storage for the interpolated 8.8 height; DS:043F temporarily holds the chosen fraction.

recovered_sub_3176__L3227:
    mov [ray_corner_heights],al
    mov [ray_corner_height_wrap],al
    and byte [ray_sample_mode_flags],0x7f
    inc byte [0x10e2]
    call near recovered_sub_3302
    mov [ray_corner_heights+3],al
    inc byte [0x10e4]
    call near recovered_sub_3302
    mov [ray_corner_heights+2],al
    dec byte [0x10e2]
    call near recovered_sub_3302
    mov [ray_corner_heights+1],al
    dec byte [0x10e4]
    call near terrain_address_cell
    and al,0xf
    cmp al,0x4
    jz short recovered_sub_3176__L3261
    cmp al,0xc
    jnz short recovered_sub_3176__L3281

recovered_sub_3176__L3261:
    mov al,[ray_position_height+2]
    cmp al,[ray_corner_heights]
    jnc short recovered_sub_3176__L327e
    cmp al,[ray_corner_heights+1]
    jnc short recovered_sub_3176__L327e
    cmp al,[ray_corner_heights+2]
    jnc short recovered_sub_3176__L327e
    cmp al,[ray_corner_heights+3]
    jnc short recovered_sub_3176__L327e
    jmp short recovered_sub_3176__L3225

recovered_sub_3176__L327e:
    jmp near recovered_sub_3176__L3185

recovered_sub_3176__L3281:
    shr al,1
    jnc short recovered_sub_3176__L3293
    shr al,1
    jc short recovered_sub_3176__L328d
    and al,0x1
    jmp short recovered_sub_3176__L32b2

recovered_sub_3176__L328d:
    adc al,0x1
    and al,0x3
    jmp short recovered_sub_3176__L3295

recovered_sub_3176__L3293:
    shr al,1

recovered_sub_3176__L3295:
    mov [ray_contact_work],al
    shr al,1
    mov al,[ray_position_x+1]
    jnc short recovered_sub_3176__L32a1
    not al

recovered_sub_3176__L32a1:
    cmp al,[ray_position_y+1]
    cmc
    mov dl,[ray_contact_work]
    rcl dl,1
    mov_load di, dx
    mov al,[di+ray_contact_edge_selectors]

recovered_sub_3176__L32b2:
    mov_load bl, al
    mov dl,[ray_position_x+1]
    shr al,1
    jc short recovered_sub_3176__L32c0
    mov dl,[ray_position_y+1]

recovered_sub_3176__L32c0:
    shr al,1
    mov_load al, dl
    jnc short recovered_sub_3176__L32c8
    not al

recovered_sub_3176__L32c8:
    mov [ray_contact_fraction],al
    mov al,[bx+ray_corner_heights]
    mov [ray_contact_work],al
    mov al,[bx+ray_corner_heights+1]
    sub al,[bx+ray_corner_heights]
    pushf
    jns short recovered_sub_3176__L32df
    neg al

recovered_sub_3176__L32df:
    mov [ray_corner_heights+2],al
    mov al,[ray_contact_fraction]
    call near math_multiply_bytes_split_result
    popf
    call near math_negate_work_if_negative
    add al,[ray_contact_work]
    mov [ray_corner_heights+2],al
    mov ax,[ray_position_height+1]
    sub ax,[ray_corner_heights+1]
    jns short recovered_sub_3176__L32ff
    jmp near recovered_sub_3176__L3225

recovered_sub_3176__L32ff:
    jmp near recovered_sub_3176__L3185

recovered_sub_3302:
    call near terrain_address_cell
    jc short recovered_sub_3302__L335b
    mov_load ah, al
    and ah,0xf
    mov_load dl, ah
    mov_load di, dx
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    cmp dl,0x1
    cmc
    ret

recovered_sub_3302__L331d:
    cmp dl,[sentry_test_target_slot]
    jnz short recovered_sub_3302__L3328
    stc
    rcr byte [ray_target_hit_flags],1

recovered_sub_3302__L3328:
    mov al,[di+object_model_id]
    cmp al,0x3
    jz short recovered_sub_3302__L336c
    cmp al,0x2
    jz short recovered_sub_3302__L336c
    cmp al,MODEL_SENTINEL_PEDESTAL
    jz short recovered_sub_3302__L333b
    jmp near recovered_sub_3302__L33da

recovered_sub_3302__L333b:
    call near recovered_sub_33eb
    cmp al,0x64
    jc short recovered_sub_3302__L3345
    jmp near recovered_sub_3302__L33cd

recovered_sub_3302__L3345:
    mov byte [edge_delta_y],0x10
    mov al,[di+object_height_fraction]
    add al,0x20
    mov [ray_sample_fraction],al
    mov al,[di+object_height_integer]
    adc_load al, cl
    clc
    ret

recovered_sub_3302__L335b:
    and al,0x3f
    mov_load dl, al
    mov_load di, dx
    xor_load cl, cl
    test byte [ray_sample_mode_flags],0x80
    jz short recovered_sub_3302__L33da
    jmp short recovered_sub_3302__L331d

recovered_sub_3302__L336c:
    call near recovered_sub_33eb
    cmp al,0x40
    jnc short recovered_sub_3302__L33cd
    mov al,[di+object_model_id]
    cmp al,0x2
    jz short recovered_sub_3302__L3391
    or byte [ray_sample_special_flags],0x80
    mov al,[di+object_height_fraction]
    sub al,0x60
    mov [ray_sample_fraction],al
    mov al,[di+object_height_integer]
    sbb_load al, cl
    clc
    ret

recovered_sub_3302__L3391:
    mov al,[di+object_height_fraction]
    sub al,[ray_position_height+1]
    mov [0x110c],al
    mov al,[di+object_height_integer]
    sbb al,[ray_position_height+2]
    add byte [0x110c],0xe0
    mov ch,[0x110c]
    adc_load al, cl
    js short recovered_sub_3302__L33cd
    shr al,1
    rcr ch,1
    shr al,1
    jnz short recovered_sub_3302__L33cd
    rcr ch,1
    cmp ch,[0x110b]
    jc short recovered_sub_3302__L33cd
    test byte [ray_target_hit_flags],0x80
    jnz short recovered_sub_3302__L33cd
    stc
    rcr byte [ray_intervening_hit_flags],1

recovered_sub_3302__L33cd:
    mov al,[di+object_model_id]
    cmp al,0x2
    jz short recovered_sub_3302__L33da
    mov byte [ray_sample_mode_flags],0xc0

recovered_sub_3302__L33da:
    mov al,[di+0x5c6]
    cmp al,0x40
    jc short recovered_sub_3302__L33e5
    jmp near recovered_sub_3302__L335b

recovered_sub_3302__L33e5:
    mov al,[di+object_height_integer]
    clc
    ret

recovered_sub_33eb:
    mov al,[ray_position_x+1]
    sub al,0x80
    jns short recovered_sub_33eb__L33f4
    not al

recovered_sub_33eb__L33f4:
    mov [0x110b],al
    mov al,[ray_position_y+1]
    sub al,0x80
    jns short recovered_sub_33eb__L3400
    not al

recovered_sub_33eb__L3400:
    cmp al,[0x110b]
    jnc short recovered_sub_33eb__L3409
    mov al,[0x110b]

recovered_sub_33eb__L3409:
    mov [0x110b],al
    ret

; BX: source object slot. Initialize X/Y 8.16 accumulators at cell centers and height from the object split 8.8 value, with a zero extra fractional byte. [load+0x340d]
ray_initialize_from_object:
    xor_load al, al
    mov [ray_position_x],al
    mov [ray_position_height],al
    mov [ray_position_y],al
    mov al,0x80
    mov [ray_position_x+1],al
    mov [ray_position_y+1],al
    mov al,[bx+object_height_fraction]
    mov [ray_position_height+1],al
    mov al,[bx+object_grid_x]
    mov [ray_position_x+2],al
    mov al,[bx+object_height_integer]
    mov [ray_position_height+2],al
    mov al,[bx+object_grid_y]
    mov [ray_position_y+2],al
    ret

recovered_sub_343d:
    mov al,[bx+object_grid_x]
    mov [0x10e2],al
    mov al,[bx+object_grid_y]
    mov [0x10e4],al
    call near terrain_address_cell
    mov al,[bx+0x5c6]
    cmp al,0x40
    jc short recovered_sub_343d__L345a
    or al,0xc0
    jmp short recovered_sub_343d__L3466

recovered_sub_343d__L345a:
    mov al,[bx+object_height_integer]
    shl al,1
    shl al,1
    shl al,1
    shl al,1

recovered_sub_343d__L3466:
    mov [bp+di],al
    mov byte [bx+0x5c6],0x80
    ret

; BX: object slot. Set its grid position/height, link it to a supported base object or bare terrain, then write the cell C0h+ object tag; carry rejects unsupported stacking. [load+0x346e]
object_attach_to_terrain_cell:
    mov al,[terrain_scan_x]
    mov [bx+object_grid_x],al
    mov al,[terrain_scan_y]
    mov [bx+object_grid_y],al
    call near terrain_address_cell
    jnc short object_attach_to_terrain_cell__L34c1
    and ax,strict word 0x3f
    mov_load si, ax
    mov al,[si+object_model_id]
    cmp al,0x3
    jz short object_attach_to_terrain_cell__L3492
    cmp al,0x6
    jnz short object_attach_to_terrain_cell__L34f0

object_attach_to_terrain_cell__L3492:
    mov_load ax, si
    or al,0x40
    mov [bx+object_stack_links],al
    mov al,[si+object_model_id]
    cmp al,0x6
    jnz short object_attach_to_terrain_cell__L34af
    mov al,[si+object_height_fraction]
    mov [bx+object_height_fraction],al
    mov al,0x1
    clc
    jmp short object_attach_to_terrain_cell__L34bb

object_attach_to_terrain_cell__L34af:
    mov al,[si+object_height_fraction]
    add al,0x80
    mov [bx+object_height_fraction],al
    mov al,0x0

object_attach_to_terrain_cell__L34bb:
    adc al,[si+object_height_integer]
    jmp short object_attach_to_terrain_cell__L34d3

object_attach_to_terrain_cell__L34c1:
    mov byte [bx+object_stack_links],0x0
    mov byte [bx+object_height_fraction],0xe0
    shr al,1
    shr al,1
    shr al,1
    shr al,1

object_attach_to_terrain_cell__L34d3:
    mov [bx+object_height_integer],al
    mov_load al, bl
    or al,OBJECT_CELL_TAG_BASE
    mov [bp+di],al
    mov al,0xf5
    mov [bx+object_view_pitch],al
    call near prng_next_byte
    and al,0xf8
    add al,0x60
    mov [bx+object_heading],al
    clc
    ret

object_attach_to_terrain_cell__L34f0:
    stc
    ret

; Reviewed instruction range 34F2h..367Dh; internal routine roles remain partial. [load+0x34f2]
recovered_code_34f2:
    call near recovered_sub_3603
    jnc short recovered_code_34f2__L3503

recovered_code_34f2__L34f7:
    xor_load ax, ax
    mov [selection_presentation_flags],al
    mov [selection_model_draw_flags],al
    mov [selection_copy_interrupt_flags],al
    ret

recovered_code_34f2__L3503:
    mov byte [selection_copy_iteration_budget],0x19
    test byte [selection_presentation_flags],0xff
    jns short recovered_code_34f2__L351d
    or byte [cursor_update_guard_flags],0x80
    call word near [graphics_restore_software_cursor]
    and byte [cursor_update_guard_flags],0x7f

recovered_code_34f2__L351d:
    mov ax,0x19
    mov dx,0x18
    xor_load cx, cx
    mov cl,[selection_view_total_columns]
    call word near [graphics_fill_background_band]

recovered_code_34f2__L352d:
    mov bl,[0x1108]
    mov ah,[selection_view_column]
    xor_load al, al
    shr ax,1
    mov [view_azimuth_reference],ax
    mov al,[bx+object_heading]
    mov [selection_saved_heading],al
    add al,[0x49f]
    mov [bx+object_heading],al
    xor_load dx, dx
    mov_load di, dx
    mov [camera_transition_mode],dl
    mov al,[render_pass_columns]
    call near viewport_set_dynamic_width
    test byte [selection_model_draw_flags],0x80
    jz short recovered_code_34f2__L356b
    mov dl,[0x10ca]
    mov_load di, dx
    call near model_draw_mesh
    jmp short recovered_code_34f2__L359c

recovered_code_34f2__L356b:
    call near terrain_draw_view
    mov bl,[0x1108]
    mov byte [view_azimuth_reference],0x0
    mov al,[selection_saved_heading]
    mov [bx+object_heading],al
    mov al,[render_pass_columns]
    add [selection_view_column],al
    mov al,[selection_view_columns_left]
    sub al,[render_pass_columns]
    jz short recovered_code_34f2__L359c
    mov [selection_view_columns_left],al
    mov [render_pass_columns],al
    mov word [0x1d4],0x28
    jmp short recovered_code_34f2__L352d

recovered_code_34f2__L359c:
    mov word [0x1d4],0x0
    mov al,[selection_view_origin_column]
    cbw
    mov [selection_copy_origin],ax
    test byte [selection_presentation_flags],0x40
    jz short recovered_code_34f2__L35c7
    test byte [game_end_flags],0x80
    jz short recovered_code_34f2__L35bc
    mov byte [selection_copy_iteration_budget],0x28

recovered_code_34f2__L35bc:
    call word near [graphics_reveal_selection_pixels]
    test byte [game_end_flags],0xff
    js short recovered_code_34f2__L35e6

recovered_code_34f2__L35c7:
    or byte [cursor_update_guard_flags],0x80
    call word near [graphics_restore_software_cursor]
    or byte [software_cursor_hide_flags],0x80
    and byte [cursor_update_guard_flags],0x7f
    mov al,[selection_view_total_columns]
    cbw
    mov bx,[selection_copy_origin]
    call word near [graphics_copy_selection_strip]

recovered_code_34f2__L35e6:
    and byte [software_cursor_hide_flags],0x7f
    test byte [cursor_mode_flags],0xff
    jns short recovered_code_34f2__L3600
    or byte [cursor_update_guard_flags],0x80
    call word near [graphics_draw_software_cursor]
    and byte [cursor_update_guard_flags],0x7f

recovered_code_34f2__L3600:
    jmp near recovered_code_34f2__L34f7

recovered_sub_3603:
    mov dl,[0x10ca]
    mov_load di, dx
    cmp dl,[player_robot_slot]
    jz short recovered_sub_3603__L367c
    call near model_prepare_camera_relative_origin
    mov dl,[0x10ca]
    mov_load di, dx
    mov bl,[di+object_model_id]
    mov al,[bx+model_selection_extents]
    cmp al,[selection_extent_override]
    jnc short recovered_sub_3603__L3629
    mov al,[selection_extent_override]

recovered_sub_3603__L3629:
    xor_load ah, ah
    mov [projection_abs_delta_x],ax
    mov [selection_extent_override],ah
    call near projection_vertical_angle
    mov ax,[model_azimuth_bias]
    sub ax,[projection_angle]
    jns short recovered_sub_3603__L3642
    xor_load al, al
    jmp short recovered_sub_3603__L364a

recovered_sub_3603__L3642:
    shl ax,1
    mov_load al, ah
    cmp al,0x28
    jnc short recovered_sub_3603__L367c

recovered_sub_3603__L364a:
    mov [selection_view_column],al
    mov [selection_view_origin_column],al
    mov ax,[model_azimuth_bias]
    add ax,[projection_angle]
    js short recovered_sub_3603__L367c
    shl ax,1
    mov_load al, ah
    cmp al,0x28
    jc short recovered_sub_3603__L3663
    mov al,0x27

recovered_sub_3603__L3663:
    inc al
    sub al,[selection_view_column]
    mov [selection_view_columns_left],al
    mov [selection_view_total_columns],al
    jz short recovered_sub_3603__L367c
    cmp al,0x15
    jc short recovered_sub_3603__L3677
    mov al,0x14

recovered_sub_3603__L3677:
    mov [render_pass_columns],al
    clc
    ret

recovered_sub_3603__L367c:
    stc
    ret

; AL: model ID. Save the requested model and find a free object slot, returning BX or carry on exhaustion. [load+0x367e]
object_allocate_model_slot:
    mov [pending_object_model],al

; Find a free slot using the previously saved model ID and initialize its model field. [load+0x3681]
object_allocate_saved_model_slot:
    mov bx,0x3f

object_allocate_saved_model_slot__L3684:
    mov al,[bx+object_stack_links]
    or_load al, al
    js short object_allocate_saved_model_slot__L3692
    dec bl
    jns short object_allocate_saved_model_slot__L3684
    stc
    ret

object_allocate_saved_model_slot__L3692:
    mov [allocated_object_slot],bl
    mov al,[pending_object_model]
    mov [bx+object_model_id],al
    clc
    ret

; AL: model ID. Carry set spends its energy value (failure if insufficient); carry clear credits it. Keep the energy counter within six bits. [load+0x369f]
player_adjust_energy_for_model:
    mov dl,[bx+object_model_id]
    mov_load di, dx
    mov al,[player_energy]
    jnc short player_adjust_energy_for_model__L36b2
    sub al,[di+object_energy_values]
    jns short player_adjust_energy_for_model__L36b6
    stc
    ret

player_adjust_energy_for_model__L36b2:
    add al,[di+object_energy_values]

player_adjust_energy_for_model__L36b6:
    and al,0x3f
    mov [player_energy],al
    clc
    ret

; MODEL_ROBOT is zero: XOR AX,AX requests a new robot for hyperspace.

; Allocate a robot, choose a random eligible flat square, spend its energy cost and transfer player control to it on success. [load+0x36bd]
player_hyperspace:
    xor_load ax, ax
    call near object_allocate_model_slot
    mov bl,[player_robot_slot]
    mov al,[bx+object_height_integer]
    inc al
    mov bl,[0x10ca]
    call near landscape_place_object_on_flat_cell
    jc short player_hyperspace__L3722
    stc
    call near player_adjust_energy_for_model
    jnc short player_hyperspace__L36ea
    call near recovered_sub_343d
    mov byte [0x146c],0x3
    mov byte [hyperspace_outcome_flags],0x80
    jmp short player_hyperspace__L371c

player_hyperspace__L36ea:
    xor_load ax, ax
    call near sound_start_melody
    mov bl,[player_robot_slot]
    mov al,[bx+object_grid_x]
    cmp al,[0x1441]
    jnz short player_hyperspace__L3711
    mov al,[bx+object_grid_y]
    cmp al,[0x1442]
    jnz short player_hyperspace__L3711
    mov byte [hyperspace_outcome_flags],0xc0
    mov byte [landscape_generation_flags],0x80

player_hyperspace__L3711:
    call near recovered_sub_3e56
    mov bl,[0x10ca]
    mov [player_robot_slot],bl

player_hyperspace__L371c:
    mov byte [player_view_refresh_flags],0x80
    clc

player_hyperspace__L3722:
    ret

; Follow six-bit object links for an occupied terrain cell and draw its model stack in height-dependent order. [load+0x3723]
terrain_draw_linked_object_stack:
    and al,0x3f
    mov [object_stack_head],al
    xor_load bx, bx

terrain_draw_linked_object_stack__L372a:
    inc bl
    and al,0x3f
    mov_load dl, al
    mov_load di, dx
    mov al,[di+object_stack_links]
    cmp al,0x40
    jnc short terrain_draw_linked_object_stack__L372a
    dec bl
    mov [object_stack_count],bl
    jz short terrain_draw_linked_object_stack__L3795

terrain_draw_linked_object_stack__L3742:
    mov bl,[view_object_slot]
    mov al,[bx+object_height_fraction]
    sub al,[di+object_height_fraction]
    mov [math_work_lo],al
    mov al,[bx+object_height_integer]
    sbb al,[di+object_height_integer]
    js short terrain_draw_linked_object_stack__L378f
    or al,[math_work_lo]
    jnz short terrain_draw_linked_object_stack__L3769
    mov al,[di+object_model_id]
    cmp al,0x6
    jz short terrain_draw_linked_object_stack__L378f

terrain_draw_linked_object_stack__L3769:
    call near model_draw_mesh
    mov dl,[object_stack_head]
    mov_load di, dx
    dec byte [object_stack_count]
    js short terrain_draw_linked_object_stack__L37a7
    mov bl,[object_stack_count]
    or_load bl, bl
    jns short terrain_draw_linked_object_stack__L378b

terrain_draw_linked_object_stack__L3780:
    mov dl,[di+object_stack_links]
    and dl,0x3f
    mov_load di, dx
    dec bl

terrain_draw_linked_object_stack__L378b:
    jnz short terrain_draw_linked_object_stack__L3780
    jmp short terrain_draw_linked_object_stack__L3742

terrain_draw_linked_object_stack__L378f:
    mov dl,[object_stack_head]
    mov_load di, dx

terrain_draw_linked_object_stack__L3795:
    call near model_draw_mesh
    mov dl,[di+object_stack_links]
    and dl,0x3f
    mov_load di, dx
    dec byte [object_stack_count]
    jns short terrain_draw_linked_object_stack__L3795

terrain_draw_linked_object_stack__L37a7:
    ret

; Draw player energy as golden robots (15), robots (3), then a tree (1) or boulder (2) for the remainder; pad and publish the HUD strip. [load+0x37a8]
hud_draw_player_energy:
    xor_load ax, ax
    mov [hud_tile_column],al
    call near hud_append_tile
    mov al,[player_energy]
    mov [hud_energy_remaining],al

; Emit one golden robot icon per 15 energy, followed by a spacer. [load+0x37b6]
hud_draw_golden_robot_icons:
    mov al,[hud_energy_remaining]
    cmp al,HUD_GOLDEN_ROBOT_ENERGY
    jc short hud_draw_robot_icons
    sub al,HUD_GOLDEN_ROBOT_ENERGY
    mov [hud_energy_remaining],al
    mov al,STATUS_TILE_GOLDEN_ROBOT
    call near hud_append_tile
    xor_load ax, ax
    call near hud_append_tile
    jmp short hud_draw_golden_robot_icons

; Emit one robot icon per 3 energy, followed by a spacer. [load+0x37ce]
hud_draw_robot_icons:
    mov al,[hud_energy_remaining]
    cmp al,ENERGY_ROBOT
    jc short hud_draw_tree_or_boulder_icon
    sub al,ENERGY_ROBOT
    mov [hud_energy_remaining],al
    mov al,STATUS_TILE_ROBOT
    call near hud_append_tile
    xor_load ax, ax
    call near hud_append_tile
    jmp short hud_draw_robot_icons

; Remaining energy 1 uses the two tree halves; 2 uses the two boulder halves. Doubling the remainder selects the left tile, then INC selects the right tile. [load+0x37e6]
hud_draw_tree_or_boulder_icon:
    cmp al,ENERGY_TREE
    jc short $ +14
    shl al,1

; Remainder 1 selects STATUS_TILE_TREE_LEFT/RIGHT; remainder 2 selects STATUS_TILE_BOULDER_LEFT/RIGHT. These paired tiles form 16-pixel-wide icons.
    call near hud_append_tile
    inc al
    call near hud_append_tile
    xor_load ax, ax
    call near hud_append_tile
    cmp byte [hud_tile_column],0x1d
    jc short $ -8
    mov al,STATUS_TILE_FRAME_LEFT
    call near hud_append_tile
    mov al,STATUS_TILE_FRAME_MIDDLE
    call near hud_append_tile
    cmp byte [hud_tile_column],0x26
    jc short $ -8
    mov al,STATUS_TILE_FRAME_RIGHT
    call near hud_append_tile
    xor_load ax, ax
    call near hud_append_tile
    jmp near video_present_hud_strip

; AL: tile ID. Draw at hud_tile_column, advance column, preserve AX and restore ES=DS. [load+0x381e]
hud_append_tile:
    push ax
    cbw
    mov bl,[hud_tile_column]
    xor_load bh, bh
    call word near [graphics_draw_status_tile]
    mov ax,ds
    mov es,ax
    pop ax
    inc byte [hud_tile_column]
    ret

; Update countdowns and sound, then dispatch game callbacks. Runs in timer interrupt context. [load+0x3834]
game_timer_tick:
    dec byte [video_transition_countdown]
    jns short $ +6
    inc byte [video_transition_countdown]
    dec byte [sound_trigger_countdown]
    jns short $ +6
    inc byte [sound_trigger_countdown]
    call near update_sound_triggers
    call near sound_update_voices
    mov al,0xff
    test [game_timer_suspend_flags],al
    js short $ +86
    test [game_end_flags],al
    js short $ +70
    test [game_pause_flags],al
    js short $ +46
    test byte [sentry_clock_gate_flags],0xff
    js short $ +9
    call near sentry_tick_countdowns
    call word near [graphics_draw_sentry_warning_bar]
    test byte [input_request_ready_flags],0xff
    js short $ +20
    test byte [input_poll_delay_ticks],0xff
    jz short $ +8
    dec byte [input_poll_delay_ticks]
    jnz short $ +7
    call near input_update_cursor_mode_and_view
    jmp short $ +35
    call near recovered_sub_3e65
    jmp short $ +30
    mov dx,0x11
    call near input_collect_bound_actions
    mov cx,0x3
    mov al,0x80
    mov di,0x14f8
    rep stosb
    jmp short $ +12
    test byte [selection_model_draw_flags],0xff
    jns short $ +5
    call near video_scatter_clear_pixels
    ret

; Reviewed instruction range 38ABh..38B3h; internal routine roles remain partial. [load+0x38ab]
recovered_code_38ab:
    xor_load ax, ax
    call near viewport_select_preset
    xor_load dx, dx
    mov_load di, dx
    mov al,[di+view_y_end_presets]
    mov [viewport_y_end],al
    mov al,[di+view_y_min_presets]
    mov [viewport_y_min],al
    ret

; Reviewed instruction range 38C3h..38C8h; internal routine roles remain partial. [load+0x38c3]
recovered_code_38c3:
    mov ax,0x2
    call near viewport_select_preset
    mov dx,0x1
    mov_load di, dx
    jmp short $ -26

; Move the selection cursor using horizontal and vertical input actions. [load+0x38d0]
cursor_move:
    mov al,[camera_cursor_x_pairs]
    mov [software_cursor_x_pairs],al
    mov al,[camera_cursor_inverted_y]
    mov [software_cursor_inverted_y],al
    call near cursor_move_horizontal
    mov al,[camera_transition_request]
    or_load al, al
    jns short cursor_move__L38e9
    call near cursor_move_vertical

cursor_move__L38e9:
    test byte [cursor_update_guard_flags],0xff
    js short cursor_move__L38f4
    jmp word near [graphics_draw_software_cursor]

cursor_move__L38f4:
    ret

; Apply horizontal input to the selection cursor. [load+0x38f5]
cursor_move_horizontal:
    mov bl,[input_action_slots]
    or_load bl, bl
    js short cursor_move_horizontal__L392b
    jnz short cursor_move_horizontal__L3916
    mov al,[camera_cursor_x_pairs]
    inc al
    mov [software_cursor_x_pairs],al
    cmp al,0x90
    jc short cursor_move_horizontal__L3911
    sub al,0x40
    mov [camera_transition_request],bl

cursor_move_horizontal__L3911:
    mov [camera_cursor_x_pairs],al
    jmp short cursor_move_horizontal__L392b

cursor_move_horizontal__L3916:
    mov al,[camera_cursor_x_pairs]
    dec al
    mov [software_cursor_x_pairs],al
    cmp al,0x10
    jnc short cursor_move_horizontal__L3928
    add al,0x40
    mov [camera_transition_request],bl

cursor_move_horizontal__L3928:
    mov [camera_cursor_x_pairs],al

cursor_move_horizontal__L392b:
    ret

; Apply vertical input to the selection cursor. [load+0x392c]
cursor_move_vertical:
    mov bl,[player_robot_slot]
    mov dl,[bx+0x606]
    mov_load di, dx
    mov bl,[input_action_slots+2]
    or_load bl, bl
    js short cursor_move_vertical__L397c
    cmp bl,0x2
    jnz short cursor_move_vertical__L3961
    mov al,[camera_cursor_inverted_y]
    inc al
    mov [software_cursor_inverted_y],al
    cmp al,0xa0
    jc short cursor_move_vertical__L395b
    cmp dl,[camera_transition_pitch_targets]
    jz short cursor_move_vertical__L397d
    sub al,0x40
    mov [camera_transition_request],bl

cursor_move_vertical__L395b:
    mov [camera_cursor_inverted_y],al
    jmp short cursor_move_vertical__L397c
    nop

cursor_move_vertical__L3961:
    mov al,[camera_cursor_inverted_y]
    dec al
    mov [software_cursor_inverted_y],al
    cmp al,0x20
    jnc short cursor_move_vertical__L3979
    cmp dl,[0x5b5c]
    jz short cursor_move_vertical__L397d
    add al,0x40
    mov [camera_transition_request],bl

cursor_move_vertical__L3979:
    mov [camera_cursor_inverted_y],al

cursor_move_vertical__L397c:
    ret

cursor_move_vertical__L397d:
    mov al,[camera_cursor_inverted_y]
    mov [software_cursor_inverted_y],al
    ret

; Keep B9h as an unexplained byte: the preceding instruction is RET, and the preview gate enters at 3985h. Starting a linear decode here would consume the first two bytes of that entry as a MOV immediate.
    db 0xb9 ; g data +0x3984

; Successful object placement returns here. Apply the normal game palette to reveal the rendered preview, then wait for a fresh key/click. [load+0x3985]
landscape_preview_wait_key:
    mov byte [preview_escape_restarts],0x1
    xor_load ax, ax
    call word near [graphics_set_scene_palette]
    call near input_wait_fresh_key_or_click
    and byte [game_timer_suspend_flags],0x7f
    call near speaker_silence_all
    test byte [restart_requested_flags],0xff
    jns short $ +5
    jmp near startup_title_and_landscape_flow
    test byte [game_end_flags],0xff
    jns short landscape_preview_begin_game
    jmp near recovered_code_39cf__L3a6e

; After accepting the preview, switch back to blue while preparing the playable view; Escape instead restarts the title/landscape flow. [load+0x39af]
landscape_preview_begin_game:
    mov ax,SCENE_PALETTE_BLUE
    call word near [graphics_set_scene_palette]
    cmp word [graphics_adapter],0x2
    jz short $ +6
    call word near [graphics_select_front_buffer]
    xor_load ax, ax
    mov [camera_transition_mode],al
    mov [software_cursor_saved_last_step],al
    mov [cursor_mode_flags],al
    mov [sentry_warning_pattern],al

; Reviewed instruction range 39CFh..3B3Bh; internal routine roles remain partial. [load+0x39cf]
; recovered_code_39cf
    test byte [overview_pending],0xff
    jz short recovered_code_39cf__L3a12
    or byte [game_timer_suspend_flags],0x80
    mov ax,0x2
    call word near [graphics_set_scene_palette]
    call near recovered_code_1d77
    xor_load ax, ax
    call word near [graphics_set_scene_palette]
    mov cx,0x9
    call near draw_game_text_script
    mov cx,0x11
    call near draw_game_text_script

recovered_code_39cf__L39f7:
    test byte [sound_melody_cursor],0xff
    jns short recovered_code_39cf__L39f7
    call near input_wait_fresh_key_or_click
    and byte [game_timer_suspend_flags],0x7f
    mov byte [overview_pending],0x0
    mov ax,0x2
    call word near [graphics_set_scene_palette]

recovered_code_39cf__L3a12:
    mov al,[player_robot_slot]
    mov [0x1108],al
    mov ah,[hyperspace_outcome_flags]
    test ah,0x80
    jz short recovered_code_39cf__L3a2b
    test ah,0x40
    jnz short recovered_code_39cf__L3a82
    call near scene_draw_background
    jmp short recovered_code_39cf__L3a43

recovered_code_39cf__L3a2b:
    test byte [turn_and_occlusion_flags],0xff
    js short recovered_code_39cf__L3a35
    call near recovered_code_2d28

recovered_code_39cf__L3a35:
    call near scene_draw_background
    mov byte [0x5b24],0x2
    call near terrain_draw_view
    call near hud_draw_player_energy

recovered_code_39cf__L3a43:
    mov ax,0x2
    call near viewport_select_preset

recovered_code_39cf__L3a49:
    test byte [sound_melody_cursor],0xff
    jns short recovered_code_39cf__L3a49
    xor_load ax, ax
    call word near [graphics_set_scene_palette]
    call word near [graphics_select_work_buffer]
    mov al,[hyperspace_outcome_flags]
    or_load al, al
    jns short recovered_code_39cf__L3ac1
    mov [game_end_flags],al
    mov byte [sound_sequence_mode],0x6
    mov al,0x5
    call near recovered_code_1a17

recovered_code_39cf__L3a6e:
    mov ax,0x2
    call word near [graphics_set_scene_palette]
    call near scene_reset_workspace
    mov bx,[landscape_number_bcd]
    call near landscape_set_number_and_difficulty
    jmp near $ +315

recovered_code_39cf__L3a82:
    mov ax,0x2
    call word near [graphics_set_scene_palette]
    xor_load ax, ax
    mov [sound_sequence_mode],al
    mov [prng_state_low],ax
    mov [prng_state_middle],ax
    mov [0x1492],al
    call near $ +622
    call near completion_generate_next_landscape
    mov ax,0x1
    call word near [graphics_set_scene_palette]
    mov byte [sound_trigger_countdown],0xa
    mov al,MELODY_LANDSCAPE_COMPLETION
    call near sound_start_melody

recovered_code_39cf__L3aae:
    test byte [sound_melody_cursor],0xff
    jns short recovered_code_39cf__L3aae
    mov cx,0x6
    call near draw_game_text_script
    call near input_wait_fresh_key_or_click
    jmp short startup_title_and_landscape_flow
    nop

recovered_code_39cf__L3ac1:
    call near player_poll_actions
    jnc short recovered_code_39cf__L3ac9
    jmp near $ -302

recovered_code_39cf__L3ac9:
    mov al,[camera_transition_request]
    mov [camera_transition_mode],al
    xor_load ax, ax
    mov [video_transition_step_count],al
    mov [selection_copy_interrupt_flags],al
    test byte [mouse_cursor_mode_flags],0xff
    jnz short recovered_code_39cf__L3ae5
    test byte [cursor_mode_flags],0x80
    jnz short recovered_code_39cf__L3aea

recovered_code_39cf__L3ae5:
    or byte [0x1443],0x80

recovered_code_39cf__L3aea:
    mov byte [terrain_draw_stop],0x0
    call near recovered_code_3c2b
    and byte [0x1443],0x7f
    mov al,[video_transition_step_count]
    or_load al, al
    jz short recovered_code_39cf__L3ac1
    mov [video_transition_steps_left],al

recovered_code_39cf__L3b01:
    test byte [video_transition_countdown],0xff
    jnz short recovered_code_39cf__L3b01
    mov_load ah, al
    shr al,1
    add_load al, ah
    mov [video_transition_countdown],al
    mov ax,[video_page_offset]
    mov [video_previous_page_offset],ax
    test byte [mouse_cursor_mode_flags],0xff
    jz short recovered_code_39cf__L3b22
    call word near [graphics_restore_software_cursor]

recovered_code_39cf__L3b22:
    call word near [0x659c]
    cmp word [graphics_adapter],0x4
    jnz short recovered_code_39cf__L3b30
    call near hud_draw_player_energy

recovered_code_39cf__L3b30:
    test byte [cursor_mode_flags],0xff
    jns short recovered_code_39cf__L3b3a
    call near recovered_code_4f13

recovered_code_39cf__L3b3a:
    jmp short recovered_code_39cf__L3ac1

; Startup/restart controller: blue phase -> first-run intro -> title -> key wait -> landscape/code entry -> terrain generation -> preview. [load+0x3b3c]
startup_title_and_landscape_flow:
    mov byte [preview_escape_restarts],0x0
    mov ax,SCENE_PALETTE_BLUE
    call word near [graphics_set_scene_palette]

; Clear scene workspace under the uniform-blue palette before starting the first-run intro.
    call near scene_reset_workspace
    shr byte [intro_once_flags],1
    jnc short startup_build_title_screen

; Start melody stream zero only on the first visit. The shifted intro_once_flags bit skips it on later title restarts. [load+0x3b51]
; startup_play_five_note_intro
    xor_load ax, ax
    call near sound_start_melody

; Build the title and its PRESS ANY KEY script while the intro continues on timer interrupts and the display remains blue. [load+0x3b56]
startup_build_title_screen:
    xor_load ax, ax
    call near draw_title_or_secret_code_screen

; Script 0 composes the title PRESS ANY KEY prompt, including nested text scripts.
    xor_load cx, cx
    call near draw_game_text_script

; Busy-wait until the timer-driven intro melody cursor becomes FFh; the title is revealed only afterward. [load+0x3b60]
startup_wait_intro_complete:
    test byte [sound_melody_cursor],0xff
    jns short startup_wait_intro_complete

; Select the title palette, revealing the prepared title, then require a fresh key/click. [load+0x3b67]
; startup_reveal_title_and_wait
    mov ax,SCENE_PALETTE_TITLE
    call word near [graphics_set_scene_palette]

; Title-screen key gate: the graphics menu has already returned and the intro tune has finished.
    call near input_wait_fresh_key_or_click

; Draw the LANDSCAPE NUMBER? script and read up to four digits. This is also the retry target after a wrong code. [load+0x3b71]
startup_prompt_landscape_number:
    call near scene_reset_workspace
    call near blit_landscape_prompt_header
    mov cx,TEXT_SCRIPT_LANDSCAPE_PROMPT
    call near draw_game_text_script
    call near blit_landscape_number_panel

; Landscape-number input: maximum four decimal digits; Enter accepts the current buffer.
    mov ax,0x4
    call near input_decimal_digits
    call near pack_entered_digits_to_bcd

; Use the packed-BCD number to seed the landscape; 0000 skips the secret-code prompt.
    mov bx,[number_entry_buffer]
    call near landscape_set_number_and_difficulty
    test byte [landscape_nonzero],0xff
    jnz short startup_prompt_secret_code
    mov ax,[landscape_zero_entry_bcd]
    mov [number_entry_buffer],ax
    mov ax,[landscape_zero_entry_bcd+2]
    mov [number_entry_buffer+2],ax
    jmp short $ +23

; For a nonzero landscape, draw the secret-entry prompt and read up to eight digits before generating terrain. [load+0x3ba5]
startup_prompt_secret_code:
    call near blit_landscape_prompt_header
    mov cx,TEXT_SCRIPT_SECRET_PROMPT
    call near draw_game_text_script
    call near $ -12058

; Secret-code input: maximum eight decimal digits, packed into the same working buffer.
    mov ax,0x8
    call near input_decimal_digits
    call near pack_entered_digits_to_bcd
    mov ax,SCENE_PALETTE_BLUE
    call word near [graphics_set_scene_palette]

; The preview path is NONLOCAL: terrain_filter_scanline rewrites [saved_hardware_sp-2] to load+1A78h.
    call near landscape_generate_terrain

; Ordinary generation return/failure: blue -> rebuilt title -> WRONG SECRET CODE -> key wait -> landscape prompt. Successful generation bypasses this path. [load+0x3bc4]
startup_wrong_code_and_retry:
    mov ax,SCENE_PALETTE_BLUE
    call word near [graphics_set_scene_palette]
    call near scene_reset_workspace
    xor_load ax, ax
    call near draw_title_or_secret_code_screen
    mov ax,SCENE_PALETTE_TITLE
    call word near [graphics_set_scene_palette]
    mov cx,TEXT_SCRIPT_WRONG_CODE
    call near draw_game_text_script
    call near input_wait_fresh_key_or_click
    jmp short startup_prompt_landscape_number

; Select full TFINAL/SKYBOX image background or clear/copy the render background; preview mode also scatters highlight pixels. [load+0x3be5]
scene_draw_background:
    xor_load ax, ax
    call near viewport_select_preset
    test byte [scene_flags],0x80
    jz short scene_draw_background__L3c02
    call near blit_title_final_screen_image
    test byte [scene_flags],0x20
    jz short scene_draw_background__L3c00
    call near blit_skybox_background_image
    jmp short scene_draw_background__L3c26

scene_draw_background__L3c00:
    jmp short scene_draw_background__L3c26

scene_draw_background__L3c02:
    call near video_draw_zero_tile_strip
    xor_load ax, ax
    mov dx,0x18
    mov cx,0x28
    call word near [graphics_fill_background_band]
    cmp byte [scene_background_style],0x3
    jnz short scene_draw_background__L3c26
    mov byte [scene_scatter_passes],0x3

scene_draw_background__L3c1d:
    call near video_scatter_highlight_pixels
    dec byte [scene_scatter_passes]
    jnz short scene_draw_background__L3c1d

scene_draw_background__L3c26:
    xor_load bx, bx
    mov_load dx, bx
    ret

; Reviewed instruction range 3C2Bh..3CCEh; internal routine roles remain partial. [load+0x3c2b]
recovered_code_3c2b:
    mov dl,[camera_transition_mode]
    mov_load di, dx
    mov bl,[0x1108]
    cmp dl,0x2
    jnc short recovered_code_3c2b__L3c7d
    mov al,[di+camera_transition_steps]
    add [bx+object_heading],al
    mov ax,0x19
    mov dx,0x18
    mov cx,0x10
    mov [render_pass_columns],cl
    call word near [graphics_fill_background_band]
    call near recovered_code_38c3
    call near terrain_draw_view
    mov bl,[0x1108]
    mov dl,[camera_transition_mode]
    mov_load di, dx
    jc short recovered_code_3c2b__L3c74
    or_load dl, dl
    jnz short recovered_code_3c2b__L3c6e
    sub byte [bx+object_heading],0xc

recovered_code_3c2b__L3c6e:
    mov byte [video_transition_step_count],0x10
    ret

recovered_code_3c2b__L3c74:
    mov al,[di+camera_transition_steps]
    sub [bx+object_heading],al
    ret

recovered_code_3c2b__L3c7d:
    mov al,[bx+0x606]
    cmp al,[di+camera_transition_pitch_targets-2]
    jz short recovered_code_3c2b__L3cc4
    add al,[di+camera_transition_steps]
    mov [bx+0x606],al
    mov ax,0x19
    mov dx,0x8
    mov cx,0x28
    mov [render_pass_columns],cl
    call word near [graphics_fill_background_band]
    call near recovered_code_38ab
    call near terrain_draw_view
    mov bl,[player_robot_slot]
    mov dl,[camera_transition_mode]
    mov_load di, dx
    jc short recovered_code_3c2b__L3cc5
    cmp dl,0x3
    jnz short recovered_code_3c2b__L3cbc
    add byte [bx+0x606],0x8

recovered_code_3c2b__L3cbc:
    mov byte [video_transition_step_count],0x8

recovered_code_3c2b__L3cc1:
    call near $ -1016

recovered_code_3c2b__L3cc4:
    ret

recovered_code_3c2b__L3cc5:
    mov al,[di+camera_transition_steps]
    sub [bx+0x606],al
    jmp short recovered_code_3c2b__L3cc1

; Clear the scene workspace and reset object/render flags before a title or landscape-entry screen. [load+0x3ccf]
scene_reset_workspace:
    or byte [game_timer_suspend_flags],0x80
    mov di,resource_read_buffer
    mov cx,resource_read_buffer_end
    sub_load cx, di
    xor_load ax, ax

; Phase boundary confirmed in the title trace: clear DS:043E..3698 after resource loading; this resets the aliased scene, occlusion and keyboard storage before title construction.
    rep stosb
    cmp word [graphics_adapter],0x2
    jz short $ +6
    call word near [graphics_select_front_buffer]
    mov al,0x80
    mov [input_request_ready_flags],al
    mov [sentry_clock_gate_flags],al
    mov [secret_code_pair_work],al
    mov [sound_melody_cursor],al
    mov [input_action_slots],al
    mov [input_action_slots+1],al
    mov [input_action_slots+2],al
    mov [input_action_slots+3],al
    mov al,0xff
    mov di,terrain_visibility_bits
    mov cx,0x80
    rep stosb
    mov al,0x80
    mov di,0x5c6
    mov cx,0x40
    rep stosb
    inc byte [prng_state_middle]
    call near $ -1108
    ret

; Space toggles cursor mode. Cursor mode selects a square; with cursor mode off, directional input changes the view. [load+0x3d21]
input_update_cursor_mode_and_view:
    mov byte [camera_transition_request],0x80
    mov bl,0x1
    call near input_test_scan_pressed
    jnz short input_update_cursor_mode_and_view__L3d32
    or byte [restart_requested_flags],0x80

input_update_cursor_mode_and_view__L3d32:
    mov bl,0x39
    call near input_test_scan_pressed
    jz short input_update_cursor_mode_and_view__L3d7e
    mov bl,0x77
    call near input_test_scan_pressed
    jz short input_update_cursor_mode_and_view__L3d7e
    test byte [mouse_cursor_mode_flags],0xff
    jz short input_update_cursor_mode_and_view__L3d5c
    test byte [cursor_mode_flags],0xff
    jns short input_update_cursor_mode_and_view__L3d5c
    test byte [input_buttons],0x1
    jnz short input_update_cursor_mode_and_view__L3dac
    mov byte [mouse_activation_click_latch],0x0
    jmp short input_update_cursor_mode_and_view__L3dac

input_update_cursor_mode_and_view__L3d5c:
    test byte [input_buttons],0x1
    jz short input_update_cursor_mode_and_view__L3dac
    test byte [cursor_update_guard_flags],0xff
    js short input_update_cursor_mode_and_view__L3dac
    mov al,0x80
    mov [mouse_cursor_mode_flags],al
    mov [cursor_mode_flags],al
    mov [mouse_activation_click_latch],al
    call near mouse_update_cursor_and_edge_request
    call word near [graphics_draw_software_cursor]
    jmp short input_update_cursor_mode_and_view__L3dac

input_update_cursor_mode_and_view__L3d7e:
    xor_load al, al
    mov [mouse_cursor_mode_flags],al
    mov [mouse_moved],al
    test byte [cursor_update_guard_flags],0xff
    js short input_update_cursor_mode_and_view__L3dac
    test byte [cursor_mode_toggle_latch],0xff
    jnz short input_update_cursor_mode_and_view__L3db1
    xor byte [cursor_mode_flags],CURSOR_MODE_ACTIVE
    jns short input_update_cursor_mode_and_view__L3da4
    call near recovered_sub_3fb6
    call word near [graphics_draw_software_cursor]
    jmp short input_update_cursor_mode_and_view__L3da8

input_update_cursor_mode_and_view__L3da4:
    call word near [graphics_restore_software_cursor]

input_update_cursor_mode_and_view__L3da8:
    mov al,0x80
    jmp short input_update_cursor_mode_and_view__L3dae

input_update_cursor_mode_and_view__L3dac:
    xor_load al, al

input_update_cursor_mode_and_view__L3dae:
    mov [cursor_mode_toggle_latch],al

input_update_cursor_mode_and_view__L3db1:
    mov dl,0x13
    call near input_collect_bound_actions
    jns short input_update_cursor_mode_and_view__L3e20
    test byte [mouse_cursor_mode_flags],0xff
    jz short input_update_cursor_mode_and_view__L3e08
    test byte [mouse_moved],0xff
    jz short input_update_cursor_mode_and_view__L3ddb
    call near mouse_update_cursor_and_edge_request
    test byte [cursor_update_guard_flags],0xff
    js short input_update_cursor_mode_and_view__L3dd9
    call word near [graphics_draw_software_cursor]
    mov byte [mouse_moved],0x0

input_update_cursor_mode_and_view__L3dd9:
    jmp short input_update_cursor_mode_and_view__L3e4c

input_update_cursor_mode_and_view__L3ddb:
    test byte [mouse_activation_click_latch],0xff
    jnz short input_update_cursor_mode_and_view__L3df5
    test byte [input_buttons],0x1
    jz short input_update_cursor_mode_and_view__L3df5
    mov byte [input_buttons],0x0
    mov byte [input_action_slots+1],0x20
    jmp short input_update_cursor_mode_and_view__L3e12

input_update_cursor_mode_and_view__L3df5:
    test byte [input_buttons],0x2
    jz short input_update_cursor_mode_and_view__L3e12
    mov byte [input_buttons],0x0
    mov byte [input_action_slots+1],0x21
    jmp short input_update_cursor_mode_and_view__L3e12

input_update_cursor_mode_and_view__L3e08:
    mov byte [mouse_moved],0x0
    mov byte [cursor_repeat_phase],0x6b

input_update_cursor_mode_and_view__L3e12:
    test byte [input_action_slots+1],0xff
    jns short recovered_sub_3e56
    mov byte [turn_and_occlusion_flags],0x40
    jmp short recovered_sub_3e56__L3e5e

input_update_cursor_mode_and_view__L3e20:
    xor_load al, al
    mov [mouse_cursor_mode_flags],al
    mov [mouse_moved],al
    mov bl,[cursor_mode_flags]
    or_load bl, bl
    jns short input_update_cursor_mode_and_view__L3e3b
    shl byte [cursor_repeat_phase],1
    jc short recovered_sub_3e56__L3e5e
    call near cursor_move
    jmp short input_update_cursor_mode_and_view__L3e4c

input_update_cursor_mode_and_view__L3e3b:
    mov al,[input_action_slots]
    or_load al, al
    jns short input_update_cursor_mode_and_view__L3e49
    mov al,[input_action_slots+2]
    or_load al, al
    js short recovered_sub_3e56__L3e5e

input_update_cursor_mode_and_view__L3e49:
    mov [camera_transition_request],al

input_update_cursor_mode_and_view__L3e4c:
    mov al,[camera_transition_request]
    or_load al, al
    js short recovered_sub_3e56__L3e5e
    mov [held_direction_action],al

recovered_sub_3e56:
    mov al,0x80
    mov [input_request_ready_flags],al
    mov [selection_copy_interrupt_flags],al

recovered_sub_3e56__L3e5e:
    mov al,[input_request_ready_flags]
    mov [input_ready_snapshot],al
    ret

recovered_sub_3e65:
    mov al,[input_action_slots+1]
    push ax
    mov dl,0x7
    call near input_collect_bound_actions
    pop ax
    mov [input_action_slots+1],al
    test byte [mouse_cursor_mode_flags],0xff
    jz short recovered_sub_3e65__L3e7c
    call near recovered_sub_1bf0

recovered_sub_3e65__L3e7c:
    mov al,[held_direction_action]
    cmp al,[input_action_slots]
    jz short recovered_sub_3e65__L3e8f
    cmp al,[input_action_slots+2]
    jz short recovered_sub_3e65__L3e8f
    mov al,0x80
    jmp short recovered_sub_3e65__L3e91

recovered_sub_3e65__L3e8f:
    xor_load al, al

recovered_sub_3e65__L3e91:
    mov [terrain_draw_stop],al
    ret

; AL: height threshold, BX: object slot. Sample unoccupied flat cells, relax the threshold on retries, and attach the object; carry reports exhaustion. [load+0x3e95]
landscape_place_object_on_flat_cell:
    mov [placement_height_threshold],al
    mov byte [placement_retry_counter],0x0

landscape_place_object_on_flat_cell__L3e9d:
    dec byte [placement_retry_counter]
    jnz short landscape_place_object_on_flat_cell__L3eae
    inc byte [placement_height_threshold]
    mov al,[placement_height_threshold]
    cmp al,0xc
    jnc short landscape_place_object_on_flat_cell__L3ed8

landscape_place_object_on_flat_cell__L3eae:
    call near prng_interior_grid_coordinate
    mov [terrain_scan_x],al
    call near prng_interior_grid_coordinate
    mov [terrain_scan_y],al
    call near terrain_address_cell
    jc short landscape_place_object_on_flat_cell__L3e9d
    and al,0xf
    jnz short landscape_place_object_on_flat_cell__L3e9d
    mov al,[bp+di]
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    cmp al,[placement_height_threshold]
    jnc short landscape_place_object_on_flat_cell__L3e9d
    call near object_attach_to_terrain_cell
    clc
    ret

landscape_place_object_on_flat_cell__L3ed8:
    stc
    ret

; Return a random coordinate 0..30, excluding the outermost grid row/column 31. [load+0x3eda]
prng_interior_grid_coordinate:
    call near prng_next_byte
    and al,0x1f
    cmp al,0x1f
    jnc short prng_interior_grid_coordinate
    ret

; Collect player commands; creation, absorb and transfer require cursor mode. Hyperspace and turn-around bypass that cursor-mode gate. [load+0x3ee4]
player_poll_actions:
    xor_load ax, ax
    mov [input_request_ready_flags],al
    mov [turn_and_occlusion_flags],al
    mov al,[mouse_cursor_mode_flags]
    mov [mouse_moved],al
    or_load al, al
    jz short player_poll_actions__L3efb
    mov byte [cursor_mode_flags],CURSOR_MODE_ACTIVE

player_poll_actions__L3efb:
    mov byte [input_poll_delay_ticks],0x2

player_poll_actions__L3f00:
    test byte [input_request_ready_flags],0xff
    js short player_poll_actions__L3f48
    and byte [partial_view_update_flags],0x7f
    test byte [input_ready_snapshot],0xff
    jns short player_poll_actions__L3f1f
    test byte [terrain_draw_stop],0xff
    jnz short player_poll_actions__L3f1f
    or byte [partial_view_update_flags],0x80

player_poll_actions__L3f1f:
    call near recovered_code_42fa
    test byte [game_end_flags],0xff
    jz short player_poll_actions__L3f33
    mov al,0x1e
    call near recovered_code_1a17

player_poll_actions__L3f2e:
    call near recovered_sub_3e56
    stc
    ret

player_poll_actions__L3f33:
    shl byte [player_view_refresh_flags],1
    jc short player_poll_actions__L3f2e
    test byte [restart_requested_flags],0x80
    jnz short player_poll_actions__L3f2e
    call near sentry_update_player_warning
    call near recovered_sub_16a2
    jmp short player_poll_actions__L3f00

player_poll_actions__L3f48:
    test byte [camera_transition_request],0xff
    js short player_poll_actions__L3f51
    clc
    ret

player_poll_actions__L3f51:
    mov al,[input_action_slots+1]
    or_load al, al
    js short player_poll_actions__L3f99
    cmp al,ACTION_HYPERSPACE
    jnc short player_poll_actions__L3f63
    test byte [cursor_mode_flags],CURSOR_MODE_ACTIVE
    jz short player_poll_actions__L3f99

player_poll_actions__L3f63:
    mov [player_action_code],al
    and byte [sentry_clock_gate_flags],0x7f
    call near recovered_code_006c
    call near player_execute_action
    jc short player_poll_actions__L3f8b
; The action sound lasts until this selected-view redraw returns. If no sound
; tick occurs before the explicit voice-0 silence below, it is only a transient.
    mov al,0x2
    call near sound_start_effect
    mov byte [selection_presentation_flags],0xc0
    and byte [selection_copy_interrupt_flags],0x7f
    call near recovered_code_34f2
    call near recovered_code_006c
    call near hud_draw_player_energy

player_poll_actions__L3f8b:
    mov al,[player_view_refresh_flags]
    and byte [player_view_refresh_flags],0x7f
    test al,0x80
    jz short player_poll_actions__L3f99
    stc
    ret

player_poll_actions__L3f99:
    jmp near player_poll_actions

; Advance a fractional clock; on non-carry updates, decrement all 24 sentry countdowns that are at least two. Zero and one are held for consumer-specific terminal states. [load+0x3f9c]
sentry_tick_countdowns:
    add byte [sentry_countdown_phase],0xa1
    jc short sentry_tick_countdowns__L3fb5
    mov bx,0x17

sentry_tick_countdowns__L3fa6:
    cmp byte [bx+sentry_countdown_bytes],0x2
    jc short sentry_tick_countdowns__L3fb1
    dec byte [bx+sentry_countdown_bytes]

sentry_tick_countdowns__L3fb1:
    dec bl
    jns short sentry_tick_countdowns__L3fa6

sentry_tick_countdowns__L3fb5:
    ret

recovered_sub_3fb6:
    mov al,0x50
    mov [camera_cursor_x_pairs],al
    mov [software_cursor_x_pairs],al
    mov al,0x5f
    mov [camera_cursor_inverted_y],al
    mov [software_cursor_inverted_y],al
    ret

; DX: last binding index. Reset four action bytes to 80h, then test bindings backward and unpack pressed-key actions into those slots. [load+0x3fc7]
input_collect_bound_actions:
    mov cx,0x4
    mov al,0x80
    mov di,0x14f8
    rep stosb
    mov_load di, dx

input_collect_bound_actions__L3fd3:
    mov bl,[di+input_key_bindings]
    call near input_test_scan_pressed
    jnz short input_collect_bound_actions__L3fed
    mov al,[di+input_binding_actions]
    mov_load bl, al
    and bl,0x3
    shr al,1
    shr al,1
    mov [bx+input_action_slots],al

input_collect_bound_actions__L3fed:
    dec di
    jns short input_collect_bound_actions__L3fd3
    mov al,[input_action_slots]
    and al,[input_action_slots+2]
    ret

; Build the scene background and objects for title/code screens or generated-landscape preview; scene_flags select the background path. [load+0x3ff8]
landscape_render_scene:
    mov [scene_view_model_selector],al
    mov [scene_background_style],bl
    test byte [scene_flags],0x80
    jz short landscape_render_scene__L4008
    mov bl,0x2

landscape_render_scene__L4008:
    mov al,[bx+scene_text_color_presets_a]
    mov [text_color_a_parameter],al
    mov al,[bx+scene_text_color_presets_b]
    mov [text_color_b_parameter],al
    mov [scene_view_preset],dl
    mov_load di, dx
    mov al,[di+scene_camera_height_table]
    mov [object_height_integer+0x10],al
    mov al,[di+scene_camera_pitch_table]
    mov [object_view_pitch+0x10],al
    mov al,[di+scene_camera_x_table]
    mov [object_grid_x+0x10],al
    mov al,[di+scene_camera_y_table]
    mov [object_grid_y+0x10],al
    mov al,[di+scene_camera_heading_table]
    mov [object_heading+0x10],al
    mov al,[di+scene_lateral_offset_table]
    mov [scene_lateral_offset],al
    call near scene_draw_background
    test byte [scene_flags],0x40
    jz short landscape_render_scene__L4052
    jmp short landscape_render_scene__L4071

; The preview passes a negative model selector (80h), omitting the view-object avatar from the landscape render.

landscape_render_scene__L4052:
    mov al,[scene_view_model_selector]
    or_load al, al
    js short landscape_render_scene__L405c
    call near model_draw_view_object_clipped

landscape_render_scene__L405c:
    mov al,[scene_lateral_offset]
    mov [view_lateral_offset],al
    mov bx,0x10
    mov [view_object_slot],bl
    mov byte [terrain_shading_mode],0x0
    call near terrain_draw_view

landscape_render_scene__L4071:
    mov al,[scene_view_preset]
    or_load al, al
    jnz short landscape_render_scene__L4085
    mov cx,0x7f
    mov [scene_view_preset],cl
    inc cx
    mov di,terrain_visibility_bits
    rep stosb

landscape_render_scene__L4085:
    xor_load ax, ax
    mov [view_lateral_offset],al
    mov [scene_background_style],al
    ret

; Choose the landscape object count, place the Sentinel and sentries, and select the terrain palette variant before drawing the preview. [load+0x408e]
landscape_prepare_preview_objects:
    mov al,[landscape_nonzero]
    or_load al, al
    jnz short landscape_prepare_preview_objects__L4099
    mov al,0x1
    jmp short landscape_prepare_preview_objects__L40a5

landscape_prepare_preview_objects__L4099:
    call near landscape_pick_sentry_count
    cmp al,[landscape_difficulty]
    jc short landscape_prepare_preview_objects__L40a5
    mov al,[landscape_difficulty]

landscape_prepare_preview_objects__L40a5:
    mov [landscape_sentry_count],al
    call near landscape_place_sentinel_and_sentries
    mov al,[landscape_sentry_count]
    dec al
    and al,0x7
    cbw
    call word near [graphics_set_landscape_colors]
    ret

; Placement aliases below reuse polygon scratch RAM. Several encoded-generation byte tables are named neutrally because their full code-validation role is not decoded.

; Populate the generated landscape. On success replace the return address with landscape_preview_wait_key; normal return goes to the wrong-code path. [load+0x40b8]
landscape_place_objects_and_enter_preview:
    xor_load ax, ax
    call near object_allocate_model_slot
    mov [player_robot_slot],bl
    mov byte [player_energy],0xa
    test byte [landscape_nonzero],0xff
    jnz short landscape_place_objects_and_enter_preview__L40dc
    mov byte [terrain_scan_x],0x8
    mov byte [terrain_scan_y],0x11
    call near object_attach_to_terrain_cell
    jmp short landscape_place_objects_and_enter_preview__L40ea

landscape_place_objects_and_enter_preview__L40dc:
    mov al,[landscape_object_height_limit]
    cmp al,0x6
    jc short landscape_place_objects_and_enter_preview__L40e5
    mov al,0x6

landscape_place_objects_and_enter_preview__L40e5:
    call near landscape_place_object_on_flat_cell
    jc short landscape_place_objects_and_enter_preview__L40dc

landscape_place_objects_and_enter_preview__L40ea:
    mov al,0x30
    mov ah,[landscape_sentry_count]
    sub_load al, ah
    sub_load al, ah
    sub_load al, ah
    mov [math_work_hi],al
    call near prng_small_sum
    add al,0xa
    cmp al,[math_work_hi]
    jc short landscape_place_objects_and_enter_preview__L4107
    mov al,[math_work_hi]

landscape_place_objects_and_enter_preview__L4107:
    mov [placement_remaining_trees],al

landscape_place_objects_and_enter_preview__L410a:
    mov al,0x2
    call near object_allocate_model_slot
    mov al,[landscape_object_height_limit]
    call near landscape_place_object_on_flat_cell
    jc short landscape_place_objects_and_enter_preview__L411d
    dec byte [placement_remaining_trees]
    jnz short landscape_place_objects_and_enter_preview__L410a

landscape_place_objects_and_enter_preview__L411d:
    mov bx,GENERATION_REFERENCE_LAST
    mov dl,[bx+generation_validation_seed-0xaa]
    mov_load di, dx
    test byte [landscape_generation_flags],0x80
    jz short landscape_place_objects_and_enter_preview__L412f
    mov bl,GENERATION_COMPLETION_REFERENCE_LAST

landscape_place_objects_and_enter_preview__L412f:
    call near prng_packed_decimal_byte

; The reference window overlays DS:3698..36C2: prefix, entered BCD code, zero references and existing scene state. RCL records equality; mask 1Eh ultimately selects indices 81h..84h, exactly number_entry_buffer[0..3].
    sub al,[bx+generation_reference_window-0x80]
    stc
    jz short landscape_place_objects_and_enter_preview__L413a
    cmc

landscape_place_objects_and_enter_preview__L413a:
    rcl word [landscape_validation_bits],1
    add al,[object_stack_links]
    mov [di+generation_validation_ring],al
    inc dl
    mov_load di, dx

; Continue while BL remains negative: startup visits AAh..80h (43 iterations), completion A5h..80h (38). The loop stops at 7Fh, not at zero.
    dec bl
    js short landscape_place_objects_and_enter_preview__L412f
    shl byte [landscape_generation_flags],1
    jnc short landscape_place_objects_and_enter_preview__L4155

landscape_place_objects_and_enter_preview__L4154:
    ret

landscape_place_objects_and_enter_preview__L4155:
    mov al,[landscape_validation_bits]
    and al,GENERATION_CODE_MATCH_MASK
    cmp al,GENERATION_CODE_MATCH_MASK
    jnz short landscape_place_objects_and_enter_preview__L4154

; Discard this caller return, push the preview key-gate address, and RET into it on success.
    pop ax
    mov cx,(landscape_preview_wait_key-program_image)
    push cx
    ret

; Build placement candidates from terrain height, choose cells and initialize the Sentinel and sentries. [load+0x4164]
landscape_place_sentinel_and_sentries:
    call near landscape_sample_placement_heights
    xor_load bx, bx

landscape_place_sentinel_and_sentries__L4169:
    mov [view_object_slot],bl
    mov byte [bx+object_model_id],MODEL_SENTRY

landscape_place_sentinel_and_sentries__L4172:
    call near landscape_collect_height_candidates
    jnc short landscape_place_sentinel_and_sentries__L4185
    sub byte [placement_height_threshold],0x10
    jnz short landscape_place_sentinel_and_sentries__L4172
    mov [landscape_sentry_count],bl
    jmp near landscape_place_sentinel_and_sentries__L4223

landscape_place_sentinel_and_sentries__L4185:
    call near prng_next_byte
    and al,[placement_candidate_mask]
    cmp al,[math_work_lo]
    jnc short landscape_place_sentinel_and_sentries__L4185
    xor_load bh, bh
    mov_load bl, al
    mov bl,[bx+placement_candidates]
    xor_load ax, ax
    mov [bx+placement_region_heights-0x9],al
    mov [bx+placement_region_heights-0x8],al
    mov [bx+placement_region_heights-0x7],al
    mov [bx+placement_region_heights-0x1],al
    mov [bx+placement_region_heights],al
    mov [bx+placement_region_heights+0x1],al
    mov [bx+placement_region_heights+0x7],al
    mov [bx+placement_region_heights+0x8],al
    mov [bx+placement_region_heights+0x9],al
    mov al,[bx+placement_candidate_x]
    mov [terrain_scan_x],al
    mov al,[bx+placement_candidate_y]
    mov [terrain_scan_y],al
    mov bl,[view_object_slot]
    or_load bl, bl
    jnz short landscape_place_sentinel_and_sentries__L41f7
    mov [sentinel_start_square_y],al
    mov al,[terrain_scan_x]
    mov [sentinel_start_square_x],al
    mov byte [object_model_id],MODEL_SENTINEL

; Every landscape places the Sentinel pedestal first, then attaches the Sentinel above it.
    mov ax,MODEL_SENTINEL_PEDESTAL
    call near object_allocate_model_slot
    call near object_attach_to_terrain_cell
    xor_load ax, ax
    mov [bx+object_heading],al
    mov bl,[view_object_slot]

landscape_place_sentinel_and_sentries__L41f7:
    call near object_attach_to_terrain_cell
    call near sentry_reset_scan_state
    call near prng_next_byte
    shr al,1
    rcl dh,1
    and al,0x3f
    or al,0x5
    mov [bx+sentry_scan_delays],al
    mov al,0x14
    shr dh,1
    jnc short landscape_place_sentinel_and_sentries__L4214
    mov al,0xec

landscape_place_sentinel_and_sentries__L4214:
    mov [bx+sentry_rotation_steps],al
    inc bl
    cmp bl,[landscape_sentry_count]
    jnc short landscape_place_sentinel_and_sentries__L4223
    jmp near landscape_place_sentinel_and_sentries__L4169

landscape_place_sentinel_and_sentries__L4223:
    mov al,[placement_height_threshold]
    shr al,1
    shr al,1
    shr al,1
    shr al,1
    mov [landscape_object_height_limit],al
    clc
    ret

; Collect coarse-cell indices whose sampled height matches the requested placement height; return carry if none. [load+0x4233]
landscape_collect_height_candidates:
    mov bx,0x3f
    xor_load si, si

landscape_collect_height_candidates__L4238:
    mov al,[bx+placement_region_heights]
    cmp al,[placement_height_threshold]
    jnz short landscape_collect_height_candidates__L4249
    mov_load al, bl
    mov [si+placement_candidates],al
    inc si

landscape_collect_height_candidates__L4249:
    dec bl
    jns short landscape_collect_height_candidates__L4238
    mov_load ax, si
    or_load al, al
    jz short landscape_collect_height_candidates__L4267
    mov [math_work_lo],al
    mov si,0xffff

landscape_collect_height_candidates__L4259:
    shl al,1
    inc si
    jnc short landscape_collect_height_candidates__L4259
    mov al,[si+placement_count_masks]
    mov [placement_candidate_mask],al
    clc
    ret

landscape_collect_height_candidates__L4267:
    stc
    ret

; Choose flat high points from 64 coarse regions; neighbor entries are cleared after the Sentinel or a sentry is placed.

; Sample each of 64 coarse terrain regions to build the height candidates used by Sentinel/sentry placement. [load+0x4269]
landscape_sample_placement_heights:
    xor_load bx, bx
    mov [placement_height_threshold],bl

landscape_sample_placement_heights__L426f:
    mov_load al, bl
    and al,0x7
    shl al,1
    shl al,1
    mov [placement_region_x_origin],al
    mov_load al, bl
    and al,0x38
    shr al,1
    mov [placement_region_y_origin],al
    xor_load ax, ax
    mov [bx+placement_region_heights],al
    mov byte [placement_rows_remaining],0x4
    mov al,[placement_region_y_origin]
    mov [terrain_scan_y],al
    cmp al,0x1c
    jc short landscape_sample_placement_heights__L429c
    dec byte [placement_rows_remaining]

landscape_sample_placement_heights__L429c:
    mov byte [placement_columns_remaining],0x4
    mov al,[placement_region_x_origin]
    mov [terrain_scan_x],al
    cmp al,0x1c
    jc short landscape_sample_placement_heights__L42af
    dec byte [placement_columns_remaining]

landscape_sample_placement_heights__L42af:
    call near terrain_address_cell
    and al,0xf
    jnz short landscape_sample_placement_heights__L42db
    mov al,[bp+di]
    and al,0xf0
    cmp al,[bx+placement_region_heights]
    jc short landscape_sample_placement_heights__L42db
    mov [bx+placement_region_heights],al
    cmp al,[placement_height_threshold]
    jc short landscape_sample_placement_heights__L42cd
    mov [placement_height_threshold],al

landscape_sample_placement_heights__L42cd:
    mov al,[terrain_scan_x]
    mov [bx+placement_candidate_x],al
    mov al,[terrain_scan_y]
    mov [bx+placement_candidate_y],al

landscape_sample_placement_heights__L42db:
    inc byte [terrain_scan_x]
    dec byte [placement_columns_remaining]
    jnz short landscape_sample_placement_heights__L42af
    inc byte [terrain_scan_y]
    dec byte [placement_rows_remaining]
    jnz short landscape_sample_placement_heights__L429c
    inc bl
    cmp bl,0x40
    jnc short landscape_sample_placement_heights__L42f9
    jmp near landscape_sample_placement_heights__L426f

landscape_sample_placement_heights__L42f9:
    ret

; Sentry cohort: slot zero is the Sentinel, followed by up to seven sentries. An active energy target holds rotation while scan/absorption delays run.

; Reviewed instruction range 42FAh..4650h; internal routine roles remain partial. [load+0x42fa]
recovered_code_42fa:
    mov [sentry_scheduler_saved_sp],sp
    mov bl,[current_sentry_slot]
    mov al,[bx+object_model_id]
    cmp al,MODEL_SENTRY
    jz short recovered_code_42fa__L430e
    cmp al,MODEL_SENTINEL
    jnz short recovered_code_42fa__L4321

recovered_code_42fa__L430e:
    mov [0x1444],al
    mov al,[bx+0x5c6]
    or_load al, al
    jns short recovered_code_42fa__L4336
    call near sentry_replant_absorbed_energy
    jc short recovered_code_42fa__L4321
    jmp near sentry_finish_object_update

recovered_code_42fa__L4321:
    call near prng_next_byte
    dec byte [current_sentry_slot]
    jns short recovered_code_42fa__L432f
    mov byte [current_sentry_slot],0x7

recovered_code_42fa__L432f:
    mov al,[player_robot_slot]
    mov [0x1108],al
    ret

recovered_code_42fa__L4336:
    mov al,[bx+sentry_scan_delays]
    cmp al,0x2
    jnc short recovered_code_42fa__L4321
    mov byte [bx+sentry_scan_delays],0x4
    mov byte [sentry_scan_angle_width],0x14
    mov al,[bx+sentry_meanie_slots]
    or_load al, al
    jns short sentry_update_active_meanie
    jmp near sentry_update_without_meanie

; Inline scheduler branch: use the linked Meanie as the sight origin and test the tracked robot. The Meanie session records ten visits past this sight call, all targeting robot slot 29h; it does not identify the Meanie slot. [load+0x4353]
sentry_update_active_meanie:
    mov [0x1108],al
    mov dl,[bx+sentry_target_slots]
    mov_load di, dx
    mov al,[di+0x5c6]
    or_load al, al
    js short meanie_rotate_toward_target__L43ba
    xor_load ax, ax

; Shared sight helper with AL=MODEL_ROBOT and DL/DI identifying the tracked robot; view_object_slot selects the Meanie as the ray origin. The stack return 4369h distinguishes these samples from ordinary sentry sight tests.
    call near sentry_test_target_sight
    mov al,[0x49d]
    cmp al,0x14
    jnc short meanie_choose_turn_step
    cmp dl,[player_robot_slot]
    jnz short meanie_rotate_toward_target__L43ba
    mov al,[sentry_target_sight_result]
    or_load al, al
    jz short meanie_revert_to_tree

; Inline branch reached when the Meanie faces and sees the player robot. Invoke hyperspace and select the Meanie transition model. [load+0x437d]
; meanie_force_player_hyperspace
    call near player_hyperspace
    mov byte [scene_view_model_selector],MODEL_MEANIE
    jmp short recovered_code_42fa__L4321

; Choose a signed eight-unit heading step from the target bearing. [load+0x4387]
meanie_choose_turn_step:
    mov al,0x8
    test byte [0x49d],0x80
    jz short meanie_rotate_toward_target
    mov al,0xf8

; Update the linked Meanie heading and owner scan delay, then start its sound effect. [load+0x4392]
meanie_rotate_toward_target:
    mov [meanie_turn_step],al
    mov dl,[current_sentry_slot]
    mov_load di, dx
    mov bl,[di+sentry_meanie_slots]
    mov_load ax, bx
    call near recovered_sub_480e
    mov al,[meanie_turn_step]
    add [bx+object_heading],al
    mov byte [di+sentry_scan_delays],0xa
    push bx
    mov al,0x1
    call near sound_start_effect
    pop bx
    jmp near sentry_finish_object_update__L4521

meanie_rotate_toward_target__L43ba:
    mov byte [bx+sentry_absorb_delays],0x0

; Clear the owner link and restore MODEL_TREE when the tracking/facing checks abandon the target. [load+0x43bf]
meanie_revert_to_tree:
    mov dl,[current_sentry_slot]
    mov_load di, dx
    mov bl,[di+sentry_meanie_slots]
    mov_load ax, bx
    call near recovered_sub_480e
    mov byte [di+sentry_meanie_slots],0x80
    mov byte [bx+object_model_id],MODEL_TREE
    jmp near sentry_finish_object_update

; Ordinary sentry branch: replant energy, search/recheck targets, absorb or rotate, and consider creating a Meanie. [load+0x43db]
sentry_update_without_meanie:
    mov [0x1108],bl
    call near sentry_replant_absorbed_energy
    jc short sentry_update_without_meanie__L43e7
    jmp near sentry_finish_object_update

sentry_update_without_meanie__L43e7:
    mov bl,[current_sentry_slot]
    mov al,[bx+sentry_stack_search_flags]
    or_load al, al
    jns short sentry_update_without_meanie__L4408
    call near recovered_sub_47bd
    mov bl,[current_sentry_slot]
    jc short sentry_update_without_meanie__L4403
    mov byte [bx+sentry_scan_cursor],0x40
    jmp short sentry_update_without_meanie__L447b

sentry_update_without_meanie__L4403:
    and byte [bx+sentry_stack_search_flags],0x7f

sentry_update_without_meanie__L4408:
    mov al,[bx+sentry_absorb_delays]
    or_load al, al
    jz short sentry_update_without_meanie__L4429
    mov dl,[bx+sentry_target_slots]
    mov_load di, dx
    xor_load ax, ax
    call near sentry_test_target_sight
    mov al,[sentry_target_sight_result]
    or_load al, al
    jz short sentry_update_without_meanie__L4425
    jmp near sentry_update_without_meanie__L44bd

sentry_update_without_meanie__L4425:
    mov [bx+sentry_absorb_delays],al

sentry_update_without_meanie__L4429:
    mov byte [0x10d5],0x80
    mov dl,0x3f
    mov_load di, dx

sentry_update_without_meanie__L4432:
    xor_load ax, ax
    call near sentry_test_target_sight
    test byte [sentry_intervening_sight_bits],0x40
    jnz short sentry_update_without_meanie__L4451
    test byte [sentry_target_sight_result],0xff
    jz short sentry_update_without_meanie__L4451
    js short sentry_update_without_meanie__L44bd
    cmp dl,[player_robot_slot]
    jnz short sentry_update_without_meanie__L4451
    mov [0x10d5],dl

sentry_update_without_meanie__L4451:
    dec dl
    mov_load di, dx
    jns short sentry_update_without_meanie__L4432
    mov dl,[0x10d5]
    mov_load di, dx
    or_load dl, dl
    js short sentry_update_without_meanie__L4471
    cmp dl,[bx+sentry_scan_target]
    jz short sentry_update_without_meanie__L4471
    call near sentry_reset_scan_state
    mov byte [sentry_target_sight_result],SIGHT_TARGET_VISIBLE
    jmp short sentry_update_without_meanie__L44bd

sentry_update_without_meanie__L4471:
    mov byte [bx+sentry_absorb_delays],0x0
    call near recovered_sub_47bd
    jc short sentry_update_without_meanie__L448e

sentry_update_without_meanie__L447b:
    call near sentry_absorb_energy_unit
    jc short sentry_update_without_meanie__L449a
    mov dl,[current_sentry_slot]
    mov_load di, dx
    mov byte [di+sentry_scan_delays],0x1e
    jmp near sentry_finish_object_update

sentry_update_without_meanie__L448e:
    mov bl,[current_sentry_slot]
    mov al,[bx+sentry_rotation_delays]
    cmp al,0x2
    jc short sentry_update_without_meanie__L449d

sentry_update_without_meanie__L449a:
    jmp near recovered_code_42fa__L4321

sentry_update_without_meanie__L449d:
    mov_load ax, bx
    call near recovered_sub_480e
    mov al,[bx+sentry_rotation_steps]
    add [bx+object_heading],al
    mov byte [bx+sentry_rotation_delays],0xc8
    call near sentry_reset_scan_state
    xor_load ax, ax
    call near sound_start_effect
    mov bl,[current_sentry_slot]
    jmp short sentry_finish_object_update__L4521

sentry_update_without_meanie__L44bd:
    mov [bx+sentry_target_slots],dl
    mov al,[sentry_target_sight_result]
    mov [bx+sentry_target_sight],al
    mov al,[bx+sentry_absorb_delays]
    cmp al,0x1
    jnc short sentry_update_without_meanie__L44d8
    mov byte [bx+sentry_absorb_delays],0x78

sentry_update_without_meanie__L44d5:
    jmp near recovered_code_42fa__L4321

sentry_update_without_meanie__L44d8:
    jnz short sentry_update_without_meanie__L44d5
    test byte [sentry_target_sight_result],0xff
    jns short sentry_update_without_meanie__L44f3
    call near sentry_absorb_energy_unit
    mov dl,[current_sentry_slot]
    mov_load di, dx
    mov byte [di+sentry_scan_delays],0x1e
    jc short sentry_finish_object_update__L452e
    jmp short sentry_finish_object_update

sentry_update_without_meanie__L44f3:
    call near sentry_find_tree_for_meanie
    mov dl,[current_sentry_slot]
    mov_load di, dx
    jnc short sentry_update_without_meanie__L4513
    cmp byte [di+sentry_scan_pass],0x2
    jnc short sentry_update_without_meanie__L450c
    mov byte [di+sentry_stack_search_flags],0x80
    jmp short sentry_finish_object_update__L452e

sentry_update_without_meanie__L450c:
    mov byte [di+sentry_absorb_delays],0x0
    jmp short sentry_finish_object_update__L452e

sentry_update_without_meanie__L4513:
    mov byte [di+sentry_scan_delays],0x32
    mov bl,[di+sentry_meanie_slots]

; Shared scheduler tail: mark the selection presentation and redraw from the player robot viewpoint. [load+0x451c]
sentry_finish_object_update:
    mov byte [selection_presentation_flags],0x40

sentry_finish_object_update__L4521:
    mov [0x10ca],bl
    mov al,[player_robot_slot]
    mov [0x1108],al
    call near recovered_code_34f2

sentry_finish_object_update__L452e:
    jmp near recovered_code_42fa__L4321

; SIGHT_TARGET_VISIBLE (40h) means target/body sight; SIGHT_SQUARE_VISIBLE (80h) means supporting-square sight and permission to absorb. The player warning uses half/full respectively.

; Test target-body and supporting-square sight using two rays. Result bit 6 marks target visibility; bit 7 allows energy absorption. [load+0x4531]
sentry_test_target_sight:
    mov [0x110b],al
    mov [sentry_sight_saved_source_slot],bl
    mov [sentry_test_target_slot],dl
    mov byte [sentry_target_sight_result],0x0
    mov al,[di+0x5c6]
    or_load al, al
    jns short sentry_test_target_sight__L454c
    jmp near sentry_test_target_sight__L45e3

sentry_test_target_sight__L454c:
    mov al,[di+object_model_id]
    cmp al,[0x110b]
    jz short sentry_test_target_sight__L4559
    jmp near sentry_test_target_sight__L45e3

sentry_test_target_sight__L4559:
    call near model_prepare_camera_relative_origin
    mov bl,0x7
    mov al,[bx+terrain_generation_saved_noise-7]
    mov [0x110b],al
    and al,0xf
    cmp al,0x9
    jz short sentry_test_target_sight__L456d
    mov bl,0x1

sentry_test_target_sight__L456d:
    mov_load al, bl
    add al,[0x110b]
    mov [sentry_sight_noise_limit],al
    mov al,[sentry_scan_angle_width]
    shr al,1
    mov [0x110b],al
    mov al,[0x49d]
    sub al,0xa
    add al,[0x110b]
    cmp al,[sentry_scan_angle_width]
    cmc
    jc short sentry_test_target_sight__L45e4
    mov ax,[projection_angle]
    mov [ray_heading_angle],ax
    mov byte [sentry_sight_rays_left],0x2
    test byte [0x10f7],0xff
    jnz short sentry_test_target_sight__L45cf
    or byte [ray_target_test_flags],0x80
    mov ax,[model_height_delta]
    mov [projection_abs_delta_x],ax

sentry_test_target_sight__L45ab:
    call near projection_vertical_angle
    mov ax,[projection_angle]
    mov [ray_pitch_angle],ax
    mov [0x110b],al
    mov_load al, ah
    call near recovered_sub_3118
    call near recovered_sub_3176
    rcl byte [ray_target_hit_flags],1
    rcr byte [sentry_target_sight_result],1
    rcl byte [ray_intervening_hit_flags],1
    rcr byte [sentry_intervening_sight_bits],1

sentry_test_target_sight__L45cf:
    and byte [ray_target_test_flags],0x7f
    mov ax,[model_height_delta]
    sub ax,0xe0
    mov [projection_abs_delta_x],ax
    dec byte [sentry_sight_rays_left]
    jnz short sentry_test_target_sight__L45ab

sentry_test_target_sight__L45e3:
    clc

sentry_test_target_sight__L45e4:
    mov bl,[sentry_sight_saved_source_slot]
    mov dl,[sentry_test_target_slot]
    mov_load di, dx
    ret

; Aggregate active sentry sight of the player robot: full warning takes priority over half warning; update the warning pattern and sound state. [load+0x45ef]
sentry_update_player_warning:
    xor_load dx, dx
    mov_load di, dx
    mov [0x110b],dl
    mov bl,0x7

sentry_update_player_warning__L45f9:
    test byte [bx+0x5c6],0xff
    js short sentry_update_player_warning__L462c
    mov al,[bx+object_model_id]
    cmp al,MODEL_SENTRY
    jz short sentry_update_player_warning__L460c
    cmp al,MODEL_SENTINEL
    jnz short sentry_update_player_warning__L462c

sentry_update_player_warning__L460c:
    mov al,[bx+sentry_target_slots]
    cmp al,[player_robot_slot]
    jnz short sentry_update_player_warning__L462c
    test byte [bx+sentry_absorb_delays],0xff
    jz short sentry_update_player_warning__L462c
    mov dl,0x4
    mov_load di, dx
    mov al,[bx+sentry_target_sight]
    mov [0x110b],al
    or_load al, al
    js short sentry_update_player_warning__L4630

sentry_update_player_warning__L462c:
    dec bl
    jns short sentry_update_player_warning__L45f9

sentry_update_player_warning__L4630:
    mov [sentry_warning_pattern],dl
    mov al,[0x110b]
    mov [warning_bar_sight],al
    mov al,[sound_sequence_mode]
    cmp dl,[sound_sequence_mode]
    mov [sound_sequence_mode],dl
    jz short sentry_update_player_warning__L4650
    cmp al,0x3
    jz short sentry_update_player_warning__L4650
    mov bl,0x1
    call near speaker_silence_voice

sentry_update_player_warning__L4650:
    ret

; BX: sentry slot 0..7. Reset its scan cursor to 40h, both target markers to 80h, and scan-pass state to zero. [load+0x4651]
sentry_reset_scan_state:
    mov al,0x80
    mov [bx+sentry_meanie_slots],al
    mov [bx+sentry_scan_target],al
    mov byte [bx+sentry_scan_pass],0x0
    mov byte [bx+sentry_scan_cursor],0x40
    ret

; Find a visible tree near the tracked robot, then convert it to a Meanie and save its slot. Used when the robot is visible but its square cannot be absorbed. [load+0x4666]
sentry_find_tree_for_meanie:
    mov byte [sentry_scan_angle_width],0x28
    mov bl,[current_sentry_slot]
    mov [0x1108],bl

sentry_find_tree_for_meanie__L4673:
    mov bl,[current_sentry_slot]
    mov dl,[bx+sentry_scan_cursor]
    mov_load di, dx
    or_load dl, dl
    jnz short sentry_find_tree_for_meanie__L468f
    inc byte [bx+sentry_scan_pass]
    mov al,[bx+sentry_target_slots]
    mov [bx+sentry_scan_target],al
    stc
    ret

sentry_find_tree_for_meanie__L468f:
    dec byte [bx+sentry_scan_cursor]
    dec dl
    mov_load di, dx
    test byte [di+0x5c6],0xff
    js short sentry_find_tree_for_meanie__L4673
    cmp byte [di+object_model_id],MODEL_TREE
    jnz short sentry_find_tree_for_meanie__L4673
    mov bl,[bx+sentry_target_slots]
    mov al,[bx+object_grid_x]
    sub al,[di+object_grid_x]
    jns short sentry_find_tree_for_meanie__L46b5
    neg al

sentry_find_tree_for_meanie__L46b5:
    cmp al,0xa
    jnc short sentry_find_tree_for_meanie__L4673
    mov al,[bx+object_grid_y]
    sub al,[di+object_grid_y]
    jns short sentry_find_tree_for_meanie__L46c5
    neg al

sentry_find_tree_for_meanie__L46c5:
    cmp al,0xa
    jnc short sentry_find_tree_for_meanie__L4673
    mov al,MODEL_TREE
    call near sentry_test_target_sight
    test byte [sentry_target_sight_result],0xff
    jns short sentry_find_tree_for_meanie__L4673
    mov bl,[current_sentry_slot]
    mov_load al, dl
    call near recovered_sub_481a
    jnc short sentry_meanie_candidate_failed

; Successful candidate branch: link the selected tree slot to its sentry and change its model to MODEL_MEANIE. Candidate sight checks alone do not prove this branch ran. [load+0x46e0]
; sentry_convert_tree_to_meanie
    mov [bx+sentry_meanie_slots],dl
    mov byte [di+object_model_id],MODEL_MEANIE
    mov byte [selection_extent_override],0x68
    clc
    ret

; Advance the scan cursor when the candidate cannot be converted, then unwind through the scene-update helper. [load+0x46f0]
sentry_meanie_candidate_failed:
    inc byte [bx+sentry_scan_cursor]
    jmp near recovered_sub_480e__L4813

; Set the game-end flag when the player robot has no energy left, then unwind the scene update. [load+0x46f7]
sentry_trigger_game_end:
    mov byte [game_end_flags],0x80
    jmp near recovered_sub_480e__L4813

; Absorb one energy unit from the selected target. Player energy decreases; other objects degrade robot to boulder to tree, then disappear. Credit sentry_replant_energy. [load+0x46ff]
sentry_absorb_energy_unit:
    mov bl,[sentry_test_target_slot]
    cmp bl,[player_robot_slot]
    jnz short sentry_absorb_energy_unit__L471f
    test byte [player_energy],0xff
    jz short sentry_trigger_game_end
    dec byte [player_energy]
    call near hud_draw_player_energy
    mov al,0x5
    call near sound_start_effect
    stc
    jmp short sentry_absorb_energy_unit__L4750

sentry_absorb_energy_unit__L471f:
    mov_load al, bl
    call near recovered_sub_480e
    mov al,[bx+object_model_id]

; MODEL_ROBOT is zero: a depleted unattended robot becomes a boulder.
    or_load al, al
    jnz short sentry_absorb_energy_unit__L473b
    mov dl,[current_sentry_slot]
    mov_load di, dx
    mov byte [di+sentry_absorb_delays],0x0
    mov al,MODEL_BOULDER
    jmp short sentry_absorb_energy_unit__L474b

sentry_absorb_energy_unit__L473b:
    cmp al,MODEL_TREE
    jnz short sentry_absorb_energy_unit__L4744
    call near recovered_sub_343d
    jmp short sentry_absorb_energy_unit__L474f

sentry_absorb_energy_unit__L4744:
    mov byte [selection_extent_override],0x74
    mov al,MODEL_TREE

sentry_absorb_energy_unit__L474b:
    mov [bx+object_model_id],al

sentry_absorb_energy_unit__L474f:
    clc

sentry_absorb_energy_unit__L4750:
    mov dl,[current_sentry_slot]
    mov_load di, dx
    inc byte [di+sentry_replant_energy]
    ret

; Spend accumulated absorbed energy by placing trees at randomly chosen eligible squares. [load+0x475b]
sentry_replant_absorbed_energy:
    mov bl,[current_sentry_slot]
    mov al,[bx+sentry_replant_energy]
    or_load al, al
    stc
    jz short sentry_replant_absorbed_energy__L4789
    mov al,MODEL_TREE
    call near object_allocate_model_slot
    mov al,[0x1430]
    call near landscape_place_object_on_flat_cell
    jc short sentry_replant_absorbed_energy__L4789
    mov_load al, bl
    call near recovered_sub_481a
    jnc short sentry_replant_absorbed_energy__L478a
    mov bl,[current_sentry_slot]
    dec byte [bx+sentry_replant_energy]
    mov bl,[0x10ca]
    clc

sentry_replant_absorbed_energy__L4789:
    ret

sentry_replant_absorbed_energy__L478a:
    call near recovered_sub_343d
    jmp near recovered_sub_480e__L4813

; Add the BCD energy balance to the current landscape number, generate the destination and code, build its terrain-text scene, then print the next-landscape caption and number. [load+0x4790]
completion_generate_next_landscape:
    call near player_energy_to_bcd
    add al,[landscape_number_bcd]
    daa
    mov_load bl, al
    mov al,[landscape_number_bcd+1]
    adc al,0x0
    daa
    mov_load bh, al
    call near landscape_set_number_and_difficulty
    call near landscape_generate_terrain
    call near landscape_prepare_preview_objects
    call near landscape_place_objects_and_enter_preview
    mov ax,0x80
    call near draw_title_or_secret_code_screen
    mov cx,TEXT_SCRIPT_NEXT_LANDSCAPE
    call near draw_game_text_script
    jmp near draw_landscape_number_bcd

recovered_sub_47bd:
    mov bl,0x3f

recovered_sub_47bd__L47bf:
    mov al,[bx+0x5c6]
    or_load al, al
    js short recovered_sub_47bd__L4808
    cmp al,0x40
    jnc short recovered_sub_47bd__L47d3
    mov al,[bx+object_model_id]
    cmp al,0x3
    jnz short recovered_sub_47bd__L4808

recovered_sub_47bd__L47d3:
    mov al,[bx+object_grid_x]
    mov [0x10e2],al
    mov al,[bx+object_grid_y]
    mov [0x10e4],al
    call near terrain_address_cell
    jnc short recovered_sub_47bd__L4808
    and al,0x3f
    mov_load dl, al
    mov_load di, dx
    mov al,[di+object_model_id]
    cmp al,0x2
    jz short recovered_sub_47bd__L47f8
    cmp al,0x3
    jnz short recovered_sub_47bd__L4808

recovered_sub_47bd__L47f8:
    call near sentry_test_target_sight
    test byte [sentry_target_sight_result],0xff
    jns short recovered_sub_47bd__L4808
    mov [sentry_test_target_slot],dl
    clc
    ret

recovered_sub_47bd__L4808:
    dec bl
    jns short recovered_sub_47bd__L47bf
    stc
    ret

recovered_sub_480e:
    call near recovered_sub_481a
    jc short recovered_sub_481a__L4833

recovered_sub_480e__L4813:
    mov sp,[sentry_scheduler_saved_sp]
    jmp near recovered_code_42fa__L4321

recovered_sub_481a:
    test byte [partial_view_update_flags],0x80
    jz short recovered_sub_481a__L4834
    mov [0x10ca],al
    mov al,[player_robot_slot]
    mov [0x1108],al
    push bx
    push dx
    call near recovered_sub_3603
    pop dx
    mov_load di, dx
    pop bx

recovered_sub_481a__L4833:
    ret

recovered_sub_481a__L4834:
    stc
    ret

; Stack: result pointer. Enable and run the five adapter probes in table order, then normalize the result. [load+0x4836]
video_detect_adapters:
    push bp
    mov_load bp, sp
    push si
    push di
    mov di,[bp+0x4]
    xor_load al, al
    mov [di],al
    mov [di+0x2],al
    mov al,0x1
    mov [video_probe_display_combination_enabled],al
    mov [video_probe_color_crtc_enabled],al
    mov [video_probe_mode8_columns_enabled],al
    mov [video_probe_ega_configuration_enabled],al
    mov [video_probe_mono_crtc_enabled],al
    mov cx,0x5
    nop
    mov si,video_probe_records
    lodsb
    test al,al
    lodsw
    jz short $ +8
    push si
    push cx
    call ax
    pop cx
    pop si
    loop $ -12
    call near $ +295
    pop di
    pop si
    mov_load sp, bp
    pop bp
    ret

; BIOS AX=1A00h; authoritative display-combination codes [load+0x4874]
video_probe_display_combination:
    mov ax,0x1a00
    int byte BIOS_VIDEO
    cmp al,0x1a
    jnz short $ +72
    mov_load cx, bx
    xor_load bh, bh
    or_load ch, ch
    jz short $ +17
    mov_load bl, ch
    add_load bx, bx
    mov ax,[bx+bios_display_combination_descriptors]
    mov [di+0x2],ax
    mov_load bl, cl
    xor_load bh, bh
    add_load bx, bx
    mov ax,[bx+bios_display_combination_descriptors]
    mov [di],ax
    mov al,0x0
    mov [video_probe_color_crtc_enabled],al
    mov [video_probe_mode8_columns_enabled],al
    mov [video_probe_ega_configuration_enabled],al
    mov [video_probe_mono_crtc_enabled],al
    lea bx,[di]
    cmp byte [bx],0x1
    jz short $ +10
    lea bx,[di+0x2]
    cmp byte [bx],0x1
    jnz short $ +12
    mov word [bx],0x0
    mov byte [video_probe_mono_crtc_enabled],0x1
    nop
    ret

; BIOS AH=12h/BL=10h; EGA configuration [load+0x48c4]
video_probe_ega_configuration:
    mov bl,0x10
    mov ah,0x12
    int byte BIOS_VIDEO
    cmp bl,0x10
    jz short $ +50
    and bl,0x3
    cmp bl,0x3
    jnz short $ +42
    mov_load al, cl
    shr al,1
    mov bx,ega_switch_subtypes
    xlatb
    mov_load ah, al
    mov al,0x4
    call near $ +224
    cmp ah,0x1
    jz short $ +16
    mov byte [video_probe_color_crtc_enabled],0x0
    nop
    mov byte [video_probe_mode8_columns_enabled],0x0
    nop
    jmp short $ +8
    mov byte [video_probe_mono_crtc_enabled],0x0
    nop
    ret

; try mode 8 and inspect its reported text-column count [load+0x4900]
video_probe_mode8_columns:
    mov ah,0xf
    int byte BIOS_VIDEO
    push ax
    mov ax,0x8
    int byte BIOS_VIDEO
    mov ah,0xf
    int byte BIOS_VIDEO
    cmp ah,0x14
    jnz short $ +15
    mov al,0x3
    mov ah,0x2
    call near $ +172
    mov byte [video_probe_color_crtc_enabled],0x0
    nop
    pop ax
    xor_load ah, ah
    int byte BIOS_VIDEO
    ret

; probe the color CRTC at port 3D4h [load+0x4926]
video_probe_color_crtc:
    mov dx,0x3d4
    call near $ +79
    jc short $ +9
    mov al,0x2
    mov ah,0x2
    call near $ +145
    ret

; probe the monochrome CRTC and adapter-status bits [load+0x4936]
video_probe_mono_crtc:
    mov dx,0x3b4
    call near $ +63
    jc short $ +59
    mov dl,0xba
    in al,dx
    and al,0x80
    mov_load ah, al
    mov cx,0x8000
    in al,dx
    and al,0x80
    cmp_load al, ah
    loope $ -5
    jnz short $ +11
    mov al,0x1
    mov ah,0x1
    call near $ +110
    jmp short $ +31
    in al,dx
    mov_load dl, al
    and dl,0x70
    mov ah,0x1
    mov al,0x81
    cmp dl,0x8
    jz short $ +13
    mov al,0x80
    cmp dl,0x50
    jnz short $ +6
    mov al,0x82
    mov ah,0x3
    call near $ +79
    ret
    mov al,0xf
    out dx,al
    inc dx
    in al,dx
    mov_load ah, al
    mov al,0x66
    out dx,al
    mov cx,0x100
    loop $ +0
    in al,dx
    xchg al,ah
    out dx,al
    cmp ah,0x66
    jz short $ +3
    stc
    ret
    cmp word [di+0x2],0x0
    jz short $ +44
    cmp byte [di],0x5
    jnl short $ +39
    cmp byte [di+0x2],0x5
    jnl short $ +33
    mov ah,0xf
    int byte BIOS_VIDEO
    and al,0x7
    cmp al,0x7
    jz short $ +10
    cmp byte [di+0x1],0x1
    jnz short $ +17
    jmp short $ +8
    cmp byte [di+0x1],0x1
    jz short $ +9
    mov ax,[di]
    xchg ax,[di+0x2]
    mov [di],ax
    ret
    lea bx,[di]
    cmp byte [bx],0x0
    jz short $ +5
    lea bx,[di+0x2]
    mov [bx],ax
    ret

; saved BIOS keyboard-intercept vector: code-segment storage for original INT 15h offset/segment [load+0x49d0]
    dw 0x0000, 0x0000

; IRQ1: read and acknowledge scan code, F2 toggles sound, E0 prefix is remembered; key_down holds FF/00 states. [load+0x49d4]
keyboard_irq_handler:
    push ax
    push bx
    push ds
    mov bx,PROGRAM_DATA_PARAGRAPH
    mov ds,bx
    in al,byte KEYBOARD_DATA
    cmp al,SCANCODE_F2
    jnz short $ +14
    not byte [sound_muted]
    mov bl,[sound_muted]
    mov [sound_mute_saved],bl
    mov_load bx, ax
    mov_load bh, bl
    and bh,0x80
    and bl,0x7f
    in al,byte PPI_PORT_B
    mov_load ah, al
    or al,0x80
    out byte PPI_PORT_B,al
    xchg ah,al
    out byte PPI_PORT_B,al
    cmp bh,0x80
    jnz short $ +14
    cmp bl,0x60
    jnz short $ +9
    mov byte [keyboard_e0_prefix],0xff
    jmp short $ +37
    shl bh,1
    jc short $ +9
    mov byte [bx+key_down],0xff
    jmp short $ +6
    mov [bx+key_down],bh
    mov al,0x1
    and al,[keyboard_left_button_state]
    mov_load ah, al
    mov al,0x2
    and al,[keyboard_right_button_state]
    or_load ah, al
    or [input_buttons],ah
    mov al,PIC_EOI
    out byte PIC_COMMAND,al
    pop ds
    pop bx
    pop ax
    iret

; INT 15h/AH=4Fh alternative to IRQ1; chain other functions and clear carry after updating key state. [load+0x4a40]
bios_keyboard_intercept:
    cmp ah,0x4f
    jz short $ +7
    jmp word far [cs:saved_int15_offset]
    push bp
    mov_load bp, sp
    push ax
    push bx
    push ds
    mov bx,PROGRAM_DATA_PARAGRAPH
    mov ds,bx
    cmp al,SCANCODE_F2
    jnz short $ +14
    not byte [sound_muted]
    mov bl,[sound_muted]
    mov [sound_mute_saved],bl
    mov_load bx, ax
    mov_load bh, bl
    and bh,0x80
    and bl,0x7f
    cmp bh,0x80
    jnz short $ +14
    cmp bl,0x60
    jnz short $ +9
    mov byte [keyboard_e0_prefix],0xff
    jmp short $ +37
    shl bh,1
    jc short $ +9
    mov byte [bx+key_down],0xff
    jmp short $ +6
    mov [bx+key_down],bh
    mov al,0x1
    and al,[keyboard_left_button_state]
    mov_load ah, al
    mov al,0x2
    and al,[keyboard_right_button_state]
    or_load ah, al
    or [input_buttons],ah
    pop ds
    pop bx
    pop ax
    clc
    lahf
    mov [bp+0x6],ah
    pop bp
    iret

; Use BIOS keyboard-intercept support when available; otherwise install the IRQ1 handler. [load+0x4aad]
keyboard_install_hook:
    mov ah,0xc0
    int byte 0x15
    jc short $ +30
    test ah,ah
    jnz short $ +26
    mov al,[es:bx+0x5]
    test al,0x10
    jz short $ +18
    push ds
    mov dx,(bios_keyboard_intercept-program_image)
    mov ax,0x0
    mov ds,ax
    mov ax,0x2515
    int byte DOS_SERVICES
    pop ds
    ret
    push ds
    mov dx,(keyboard_irq_handler-program_image)
    mov ax,0x0
    mov ds,ax
    mov ax,0x2509
    int byte DOS_SERVICES
    pop ds
    ret

; saved BIOS timer vector: code-segment storage for original INT 08h offset/segment [load+0x4adf]
    dw 0x0000, 0x0000

; Run the game tick at each IRQ0; chain the saved BIOS timer every third tick, otherwise acknowledge IRQ0 here. [load+0x4ae3]
timer_irq_triple_rate:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    push es
    mov ax,PROGRAM_DATA_PARAGRAPH
    mov ds,ax
    mov es,ax
    cld
    xor_load bx, bx
    mov_load dx, bx
    call near game_timer_tick
    dec byte [bios_tick_prescaler]
    jnz short $ +21
    mov byte [bios_tick_prescaler],0x3
    pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    jmp word far [cs:saved_int08_offset]
    mov al,PIC_IRQ0_EOI
    out byte PIC_COMMAND,al
    pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    iret

; Run the game tick, then always chain the original BIOS timer handler. [load+0x4b22]
timer_irq_original_rate:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    push es
    mov ax,PROGRAM_DATA_PARAGRAPH
    mov ds,ax
    mov es,ax
    cld
    xor_load bx, bx
    mov_load dx, bx
    call near game_timer_tick
    pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    jmp word far [cs:saved_int08_offset]

; Latch/read PIT channel 0 in two FFFFh-iteration loops; return largest observed counter in DX. [load+0x4b48]
pit_sample_maximum:
    xor_load dx, dx
    mov si,0x2
    mov cx,0xffff
    xor_load ax, ax
    cli
    out byte PIT_COMMAND,al
    jmp short $ +2
    in al,byte PIT_TIMER_DATA
    mov_load bx, ax
    jmp short $ +2
    in al,byte PIT_TIMER_DATA
    sti
    mov_load bh, al
    cmp_load bx, dx
    jc short $ +4
    mov_load dx, bx
    loop $ -24
    dec si
    jnz short $ -30
    ret

; Save input/timer vectors and SP; select original-rate or triple-rate IRQ0 hook, then enter the game. [load+0x4b6e]
hardware_enter_game:
    push si
    push di
    push bp
    mov [saved_hardware_sp],sp
    call near mouse_initialize
    mov ax,0x3509
    int byte DOS_SERVICES
    mov [saved_int09_offset],bx
    mov word [saved_int09_segment],es
    mov ax,0x3515
    int byte DOS_SERVICES
    mov [cs:saved_int15_offset],bx
    mov word [cs:saved_int15_segment],es
    call near keyboard_install_hook
    mov ax,0x3508
    int byte DOS_SERVICES
    mov [cs:saved_int08_offset],bx
    mov word [cs:saved_int08_segment],es
    call near pit_sample_maximum
    mov [pit_calibration_max],dx
    cmp dx,0x6000
    ja short $ +19
    push ds
    mov dx,(timer_irq_original_rate-program_image)
    mov ax,0x0
    mov ds,ax
    mov ax,0x2508
    int byte DOS_SERVICES
    pop ds
    jmp short $ +36
    push ds
    mov dx,(timer_irq_triple_rate-program_image)
    mov ax,0x0
    mov ds,ax
    mov ax,0x2508
    int byte DOS_SERVICES
    pop ds
    cli
    mov al,PIT_TIMER_SQUARE_WAVE
    out byte PIT_COMMAND,al
    jmp short $ +2
    mov ax,PIT_TRIPLE_RATE_DIVISOR
    out byte PIT_TIMER_DATA,al
    jmp short $ +2
    xchg al,ah
    out byte PIT_TIMER_DATA,al
    sti
    mov ax,ds
    mov es,ax
    jmp near startup_title_and_landscape_flow

; Silence sound, remove mouse callback, restore PIT divisor and saved vectors, then restore the entry stack. [load+0x4bed]
hardware_leave_game:
    call near speaker_silence_all
    call near mouse_remove_callback
    cmp word [pit_calibration_max],0x6000
    jna short $ +21
    cli
    mov al,PIT_TIMER_SQUARE_WAVE
    out byte PIT_COMMAND,al
    jmp short $ +2
    mov ax,0x0
    out byte PIT_TIMER_DATA,al
    jmp short $ +2
    xchg al,ah
    out byte PIT_TIMER_DATA,al
    sti
    push ds
    lds dx,word [saved_int09_offset]
    mov ax,0x2509
    int byte DOS_SERVICES
    pop ds
    push ds
    lds dx,word [cs:saved_int15_offset]
    mov ax,0x2515
    int byte DOS_SERVICES
    pop ds
    push ds
    lds dx,word [cs:saved_int08_offset]
    mov ax,0x2508
    int byte DOS_SERVICES
    pop ds
    mov sp,[saved_hardware_sp]
    pop bp
    pop di
    pop si
    ret

; AL: scene phase. Set CGA background/palette; phase 2 selects uniform blue and clears the front buffer. [load+0x4c39]
cga_set_scene_palette:
    push ax
    mov [cga_palette_phase],al
    mov_load bx, ax
    shl bx,1
    push bx
    mov bl,[bx+cga_scene_palettes]
    xor_load bh, bh
    mov ah,0xb
    int byte BIOS_VIDEO
    pop bx
    mov bl,[bx+cga_scene_palettes+1]
    mov bh,0x1
    mov ah,0xb
    int byte BIOS_VIDEO
    pop ax
    cmp al,0x2
    jnz short $ +24
    mov ax,VIDEO_SEG_CGA
    mov es,ax
    mov cx,0x2000
    xor_load ax, ax
    mov_load di, ax
    rep stosw
    mov ax,[0x65d6]
    mov [video_target_base],ax
    jmp short $ +30
    call word near [graphics_select_front_buffer]
    push ds
    mov ax,VIDEO_SEG_CGA
    mov es,ax
    mov ds,word [0x65d6]
    xor_load di, di
    mov_load si, di
    mov cx,0x2000
    rep movsw
    pop ds
    mov word [video_target_base],es
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; AL: scene phase. Load one of three 17-byte palette records. [load+0x4c97]
tandy_set_scene_palette:
    mov_load bx, ax
    mov cl,0x4
    shl bx,cl
    add_load bx, ax
    lea dx,[bx+tandy_scene_palettes]
    mov ax,0x1002
    int byte BIOS_VIDEO
    xor_load bx, bx
    mov_load dx, bx
    ret

; AL: scene phase. Load title/game colors or the uniform-blue palette. [load+0x4cad]
ega_set_scene_palette:
    mov_load bx, ax
    mov cl,0x4
    shl bx,cl
    add_load bx, ax
    lea dx,[bx+ega_scene_palettes]
    mov ax,0x1002
    int byte BIOS_VIDEO
    xor_load bx, bx
    mov_load dx, bx
    ret

; AL: scene phase. Set 16 DAC RGB entries and the EGA-compatible attribute palette. [load+0x4cc3]
vga_ega_set_scene_palette:
    mov_load bx, ax
    mov cl,0x4
    shl bx,cl
    mov_load ax, bx
    shl bx,1
    add_load bx, ax
    lea dx,[bx+vga_scene_rgb_palettes]
    xor_load bx, bx
    mov cx,0x10
    mov ax,0x1012
    int byte BIOS_VIDEO
    mov dx,vga_attribute_identity
    mov ax,0x1002
    int byte BIOS_VIDEO
    xor_load bx, bx
    mov_load dx, bx
    ret

; AL: scene phase. Repeat the 16-color RGB palette across VGA DAC banks; phase 2 maps every color to blue. [load+0x4cea]
vga_set_scene_palette:
    mov_load bx, ax
    cmp al,0x0
    jz short vga_build_intensity_banks
    mov cl,0x4
    shl bx,cl
    mov_load ax, bx
    shl bx,1
    add_load bx, ax
    lea dx,[bx+vga_scene_rgb_palettes]
    xor_load bx, bx

; Title/blue: upload the same 16 RGB colors at DAC indices 00h,10h,...,F0h. [load+0x4d00]
vga_upload_fixed_palette_bank:
    mov cx,0x10
    mov ax,0x1012
    push bx
    push dx
    int byte BIOS_VIDEO
    pop dx
    pop bx
    add bx,0x10
    cmp bx,0x100
    jnz short vga_upload_fixed_palette_bank
    xor_load bx, bx
    mov_load dx, bx
    ret

; Game phase: copy base RGB colors into the work palette and upload 16 successively darkened banks. [load+0x4d1a]
vga_build_intensity_banks:
    mov cx,0x30
    mov si,vga_scene_rgb_palettes
    mov di,vga_work_palette
    rep movsb
    xor_load bx, bx

; Upload current work palette, darken it in place, then advance the DAC start index by 16. [load+0x4d27]
vga_upload_next_intensity_bank:
    mov dx,vga_work_palette
    mov cx,0x10
    mov ax,0x1012
    push bx
    int byte BIOS_VIDEO
    call near vga_darken_work_palette
    pop bx
    add bx,0x10
    cmp bx,0x100
    jnz short vga_upload_next_intensity_bank
    xor_load bx, bx
    mov_load dx, bx
    ret

; For each RGB color, subtract floor(max(R,G,B)/13) from all three channels, clamping at zero. Colors with maximum below 13 stop changing. [load+0x4d45]
vga_darken_work_palette:
    mov si,vga_work_palette
    mov_load di, si
    xor_load dx, dx
    mov cx,0x3
    xor_load ax, ax
    mov al,[si]
    cmp al,[si+0x1]
    ja short $ +5
    mov al,[si+0x1]
    cmp al,[si+0x2]
    ja short $ +5
    mov al,[si+0x2]
    or_load al, al
    jz short $ +6
    mov bl,0xd
    div bl
    mov_load bl, al
    lodsb
    sub_load al, bl
    jns short $ +4
    xor_load al, al
    stosb
    loop $ -8
    inc dx
    cmp dx,0x10
    jnz short $ -47
    ret

; AL: landscape color variant. Select CGA background and palette selector. [load+0x4d7e]
cga_select_landscape_colors:
    mov_load bx, ax
    mov al,[bx+cga_palette_variant_selectors]
    mov [cga_scene_palettes+1],al
    mov al,[bx+cga_background_variant_controls]
    mov [cga_scene_palettes],al
    ret

; AL: variant 0..7. Replace game palette slots 3, 2, 1 with RGBI colors. [load+0x4d8f]
; tandy_select_landscape_colors
    mov_load bx, ax
    mov al,[bx+tandy_landscape_color_3_variants]
    mov [0x5c02],al
    mov al,[bx+tandy_landscape_color_2_variants]
    mov [0x5c01],al
    mov al,[bx+tandy_landscape_color_1_variants]
    mov [0x5c00],al
    ret

; AL: variant 0..7. Replace game palette slots 3, 2, 1 with EGA mode-0Dh colors. [load+0x4da7]
; ega_select_landscape_colors
    mov_load bx, ax
    mov al,[bx+ega_landscape_color_3_variants]
    mov [0x5c35],al
    mov al,[bx+ega_landscape_color_2_variants]
    mov [0x5c34],al
    mov al,[bx+ega_landscape_color_1_variants]
    mov [0x5c33],al
    ret

; AL: variant 0..7. Replace game palette RGB triplets 3, 2, 1. [load+0x4dbf]
; vga_select_landscape_colors
    mov_load bx, ax
    shl bx,1
    add_load bx, ax
    mov ax,[bx+vga_landscape_color_3_variants]
    mov [vga_scene_rgb_palettes+3*3],ax
    mov al,[bx+0x5d68]
    mov [vga_scene_rgb_palettes+3*3+2],al
    mov ax,[bx+vga_landscape_color_2_variants]
    mov [vga_scene_rgb_palettes+2*3],ax
    mov al,[bx+0x5d80]
    mov [vga_scene_rgb_palettes+2*3+2],al
    mov ax,[bx+vga_landscape_color_1_variants]
    mov [vga_scene_rgb_palettes+1*3],ax
    mov al,[bx+0x5d98]
    mov [vga_scene_rgb_palettes+1*3+2],al
    ret

; BL: clear/highlight, CX: count. Advance the shared 32-bit scatter sequence and modify packed CGA pixels. [load+0x4df0]
cga_scatter_pixels:
    mov ax,VIDEO_SEG_CGA
    cmp byte [cga_palette_phase],0x2
    jnz short cga_scatter_pixels__L4dfd
    mov ax,[video_work_buffer_segment]

cga_scatter_pixels__L4dfd:
    mov es,ax

cga_scatter_pixels__L4dff:
    mov ax,[pixel_scatter_sequence_low]
    mov bp,[pixel_scatter_sequence_high]
    shl ax,1
    rcl bp,1
    shl ax,1
    rcl bp,1
    add [pixel_scatter_sequence_low],ax
    adc [pixel_scatter_sequence_high],bp
    mov di,[pixel_scatter_sequence_high]
    mov_load si, di
    shr di,1
    shr di,1
    and si,0x3
    mov al,[es:di]
    and al,[si+cga_clear_pixel_masks]
    or_load bl, bl
    jz short cga_scatter_pixels__L4e32
    or al,[si+cga_set_pixel_masks]

cga_scatter_pixels__L4e32:
    mov [es:di],al
    loop cga_scatter_pixels__L4dff
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Advance the scatter sequence and modify pixels using the backend bit masks and B000h video segment. [load+0x4e40]
tandy_scatter_pixels:
    mov ax,VIDEO_SEG_MONO
    mov es,ax

tandy_scatter_pixels__L4e45:
    mov ax,[pixel_scatter_sequence_low]
    mov bp,[pixel_scatter_sequence_high]
    shl ax,1
    rcl bp,1
    shl ax,1
    rcl bp,1
    add [pixel_scatter_sequence_low],ax
    adc [pixel_scatter_sequence_high],bp
    mov di,[pixel_scatter_sequence_high]
    mov_load si, di
    shr di,1
    and si,0x1
    mov al,[es:di]
    and al,[si+tandy_clear_pixel_masks]
    or_load bl, bl
    jz short tandy_scatter_pixels__L4e76
    or al,[si+tandy_set_pixel_masks]

tandy_scatter_pixels__L4e76:
    mov [es:di],al
    loop tandy_scatter_pixels__L4e45
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Modify scattered EGA pixels with GC masks, skipping writes while ega_polygon_busy is set. [load+0x4e84]
ega_scatter_pixels:
    mov dx,EGA_GC_INDEX
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov bh,0xff

ega_scatter_pixels__L4e8e:
    mov ax,[pixel_scatter_sequence_low]
    mov bp,[pixel_scatter_sequence_high]
    shl ax,1
    rcl bp,1
    shl ax,1
    rcl bp,1
    add [pixel_scatter_sequence_low],ax
    adc [pixel_scatter_sequence_high],bp
    mov di,[pixel_scatter_sequence_high]
    test [ega_polygon_busy],bh
    js short ega_scatter_pixels__L4ed2
    mov_load si, di
    shr di,1
    shr di,1
    shr di,1
    add di,[video_page_offset]
    and si,0x7
    mov ah,[si+ega_single_pixel_masks]
    mov al,0x8
    cli
    out dx,ax
    mov ax,0xa05
    out dx,ax
    and [es:di],bl
    mov ax,0x105
    out dx,ax
    sti

ega_scatter_pixels__L4ed2:
    loop ega_scatter_pixels__L4e8e
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Modify scattered linear VGA pixels, using zero or color 0Eh according to BL. [load+0x4edd]
vga_scatter_pixels:
    mov ax,[pixel_scatter_sequence_low]
    mov bp,[pixel_scatter_sequence_high]
    shl ax,1
    rcl bp,1
    shl ax,1
    rcl bp,1
    add [pixel_scatter_sequence_low],ax
    adc [pixel_scatter_sequence_high],bp
    mov di,[pixel_scatter_sequence_high]
    mov bp,0xa000
    mov es,bp
    xor_load al, al
    or_load bl, bl
    jz short vga_scatter_pixels__L4f05
    mov al,0xe

vga_scatter_pixels__L4f05:
    mov [es:di],al
    loop vga_scatter_pixels
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Reviewed instruction range 4F13h..5248h; internal routine roles remain partial. [load+0x4f13]
recovered_code_4f13:
    mov bl,[camera_transition_mode]
    shl bl,1
    mov_load si, bx
    mov bl,[software_cursor_saved_last_step]
    or_load bl, bl
    jz short recovered_code_4f13__L4f31
    cmp word [graphics_adapter],0x2
    jnz short recovered_code_4f13__L4f2a

recovered_code_4f13__L4f2a:
    cmp word [graphics_adapter],0x4
    jnz short recovered_code_4f13__L4f31

recovered_code_4f13__L4f31:
    mov al,[camera_cursor_x_pairs]
    mov [software_cursor_x_pairs],al
    mov al,[camera_cursor_inverted_y]
    mov [software_cursor_inverted_y],al
    xor_load bx, bx
    ret

; Restore the previous cursor, save the bytes covered by 21 stepped pixel pairs, and draw the software crosshair in CGA memory. [load+0x4f40]
cga_draw_software_cursor:
    test byte [software_cursor_hide_flags],0xff
    jns short cga_draw_software_cursor__L4f4a
    jmp near cga_draw_software_cursor__L4fd2

cga_draw_software_cursor__L4f4a:
    call near cga_restore_software_cursor
    mov ax,VIDEO_SEG_CGA
    mov es,ax
    mov al,0xc7
    sub al,[software_cursor_inverted_y]
    xor_load ah, ah
    mov_load bp, ax
    shl bp,1
    mov cl,[software_cursor_x_pairs]
    xor_load ch, ch

cga_draw_software_cursor__L4f64:
    mov bl,[software_cursor_saved_last_step]
    mov al,[bx+cursor_step_scanline_deltas]
    or_load al, al
    js short cga_draw_software_cursor__L4fca
    cbw
    add_load bp, ax
    mov al,[bx+cursor_step_x_deltas]
    cbw
    add_load cx, ax
    mov_load ax, cx
    mov_load si, ax
    and si,0x1
    shr ax,1
    mov di,[ds:bp+video_scanline_offsets]
    add_load di, ax
    mov ax,0x2000
    test ax,di
    jnz short cga_draw_software_cursor__L4f9b
    add di,[video_page_offset]
    and di,0x1fff
    jmp short cga_draw_software_cursor__L4fa5

cga_draw_software_cursor__L4f9b:
    add di,[video_page_offset]
    and di,0x1fff
    or_load di, ax

cga_draw_software_cursor__L4fa5:
    shl bx,1
    mov [bx+cursor_saved_video_offsets],di
    mov al,[es:di]
    mov [bx+cursor_saved_pixels],al
    and al,[si+cursor_cga_clear_pair_masks]
    test byte [bx+cursor_step_ink_records],0xff
    jz short cga_draw_software_cursor__L4fc1
    or al,[si+cursor_cga_set_pair_masks]

cga_draw_software_cursor__L4fc1:
    mov [es:di],al
    inc byte [software_cursor_saved_last_step]
    jmp short cga_draw_software_cursor__L4f64

cga_draw_software_cursor__L4fca:
    dec byte [software_cursor_saved_last_step]
    mov ax,ds
    mov es,ax

cga_draw_software_cursor__L4fd2:
    ret

; Restore saved CGA bytes in reverse step order; the common tail also serves Tandy with a different video segment. [load+0x4fd3]
cga_restore_software_cursor:
    mov ax,VIDEO_SEG_CGA

cga_restore_software_cursor__L4fd6:
    mov es,ax
    mov bl,[software_cursor_saved_last_step]
    or_load bl, bl
    jz short cga_restore_software_cursor__L4ff7
    shl bx,1

cga_restore_software_cursor__L4fe2:
    mov di,[bx+cursor_saved_video_offsets]
    mov al,[bx+cursor_saved_pixels]
    mov [es:di],al
    dec bx
    dec bx
    jns short cga_restore_software_cursor__L4fe2
    xor_load bx, bx
    mov [software_cursor_saved_last_step],bl

cga_restore_software_cursor__L4ff7:
    mov ax,ds
    mov es,ax
    ret

; Save and replace one packed-color byte per two-pixel cursor step in Tandy video memory. [load+0x4ffc]
tandy_draw_software_cursor:
    test byte [software_cursor_hide_flags],0xff
    js short tandy_draw_software_cursor__L5065
    call near tandy_restore_software_cursor
    mov ax,VIDEO_SEG_MONO
    mov es,ax
    mov al,0xc7
    sub al,[software_cursor_inverted_y]
    xor_load ah, ah
    mov_load bp, ax
    shl bp,1
    mov cl,[software_cursor_x_pairs]
    xor_load ch, ch
    mov dl,0xee

tandy_draw_software_cursor__L501f:
    mov bl,[software_cursor_saved_last_step]
    mov al,[bx+cursor_step_scanline_deltas]
    or_load al, al
    js short tandy_draw_software_cursor__L505d
    cbw
    add_load bp, ax
    mov al,[bx+cursor_step_x_deltas]
    cbw
    add_load cx, ax
    mov di,[ds:bp+video_scanline_offsets]
    add_load di, cx
    shl bx,1
    mov [bx+cursor_saved_video_offsets],di
    mov al,[es:di]
    mov [bx+cursor_saved_pixels],al
    xor_load al, al
    test byte [bx+cursor_step_ink_records],0xff
    jz short tandy_draw_software_cursor__L5054
    mov_load al, dl

tandy_draw_software_cursor__L5054:
    mov [es:di],al
    inc byte [software_cursor_saved_last_step]
    jmp short tandy_draw_software_cursor__L501f

tandy_draw_software_cursor__L505d:
    dec byte [software_cursor_saved_last_step]
    mov ax,ds
    mov es,ax

tandy_draw_software_cursor__L5065:
    ret

; Select the Tandy video segment and enter the shared byte-restore loop. [load+0x5066]
tandy_restore_software_cursor:
    mov ax,VIDEO_SEG_MONO
    jmp near cga_restore_software_cursor__L4fd6

; Save all four EGA planes under each cursor pixel pair, then draw color 0 or 14 through graphics-controller write mode 2 and paired-bit masks. [load+0x506c]
ega_draw_software_cursor:
    test byte [software_cursor_hide_flags],0xff
    jns short ega_draw_software_cursor__L5076
    jmp near ega_draw_software_cursor__L5133

ega_draw_software_cursor__L5076:
    test byte [ega_polygon_busy],0xff
    jns short ega_draw_software_cursor__L5080
    jmp near ega_draw_software_cursor__L5133

ega_draw_software_cursor__L5080:
    call near ega_restore_software_cursor
    mov byte [ega_polygon_busy],0xff
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov al,0xc7
    sub al,[software_cursor_inverted_y]
    xor_load ah, ah
    mov_load bp, ax
    shl bp,1
    mov cl,[software_cursor_x_pairs]
    xor_load ch, ch
    mov dx,EGA_GC_INDEX
    mov ax,0x205
    out dx,ax

ega_draw_software_cursor__L50a6:
    mov bl,[software_cursor_saved_last_step]
    mov al,[bx+cursor_step_scanline_deltas]
    or_load al, al
    js short ega_draw_software_cursor__L5122
    cbw
    add_load bp, ax
    mov al,[bx+cursor_step_x_deltas]
    cbw
    add_load cx, ax
    mov di,[ds:bp+video_scanline_offsets]
    add di,[video_page_offset]
    mov_load ax, cx
    mov_load si, ax
    and si,0x3
    shr ax,1
    shr ax,1
    add_load di, ax
    shl bx,1
    shl bx,1
    mov ah,[si+cursor_ega_pair_masks]
    mov al,0x8
    out dx,ax
    mov_load si, bx
    mov [bx+cursor_saved_video_offsets],di
    mov ax,0x304
    out dx,ax
    mov bl,[es:di]
    mov [si+cursor_saved_pixels],bl
    dec ah
    out dx,ax
    mov bl,[es:di]
    mov [si+cursor_saved_pixels+1],bl
    dec ah
    out dx,ax
    mov bl,[es:di]
    mov [si+cursor_saved_pixels+2],bl
    dec ah
    out dx,ax
    mov bl,[es:di]
    mov [si+cursor_saved_pixels+3],bl
    xor_load al, al
    shr si,1
    test byte [si+cursor_step_ink_records],0xff
    jz short ega_draw_software_cursor__L5119
    mov al,0xe

ega_draw_software_cursor__L5119:
    mov [es:di],al
    inc byte [software_cursor_saved_last_step]
    jmp short ega_draw_software_cursor__L50a6

ega_draw_software_cursor__L5122:
    mov ax,0x105
    out dx,ax
    dec byte [software_cursor_saved_last_step]
    mov ax,ds
    mov es,ax
    mov byte [ega_polygon_busy],0x0

ega_draw_software_cursor__L5133:
    xor_load dx, dx
    ret

; Restore the four saved planes in reverse cursor-step order through sequencer map masks. [load+0x5136]
ega_restore_software_cursor:
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov bl,[software_cursor_saved_last_step]
    or_load bl, bl
    jz short ega_restore_software_cursor__L519d
    mov byte [ega_polygon_busy],0xff
    shl bx,1
    shl bx,1
    mov dx,EGA_GC_INDEX
    mov ax,0x1
    out dx,ax
    mov ax,0x5
    out dx,ax
    mov ax,0xff08
    out dx,ax
    mov dx,EGA_SEQ_INDEX

ega_restore_software_cursor__L515e:
    mov ax,0x802
    out dx,ax
    mov di,[bx+cursor_saved_video_offsets]
    mov cl,[bx+cursor_saved_pixels]
    mov [es:di],cl
    shr ah,1
    out dx,ax
    mov cl,[bx+cursor_saved_pixels+1]
    mov [es:di],cl
    shr ah,1
    out dx,ax
    mov cl,[bx+cursor_saved_pixels+2]
    mov [es:di],cl
    shr ah,1
    out dx,ax
    mov cl,[bx+cursor_saved_pixels+3]
    mov [es:di],cl
    sub bx,0x4
    jns short ega_restore_software_cursor__L515e
    mov ah,0xf
    out dx,ax
    xor_load bx, bx
    mov [software_cursor_saved_last_step],bl
    mov [ega_polygon_busy],bl

ega_restore_software_cursor__L519d:
    mov dx,EGA_GC_INDEX
    mov ax,0x105
    out dx,ax
    mov ax,ds
    mov es,ax
    xor_load dx, dx
    ret

; Save and replace a two-pixel word per cursor step in VGA memory; ink pixels use index 14. [load+0x51ab]
vga_draw_software_cursor:
    test byte [software_cursor_hide_flags],0xff
    js short vga_draw_software_cursor__L521d
    call near vga_restore_software_cursor
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov al,0xc7
    sub al,[software_cursor_inverted_y]
    xor_load ah, ah
    mov_load bp, ax
    shl bp,1
    mov cl,[software_cursor_x_pairs]
    xor_load ch, ch
    mov dx,0xe0e

vga_draw_software_cursor__L51cf:
    mov bl,[software_cursor_saved_last_step]
    mov al,[bx+cursor_step_scanline_deltas]
    or_load al, al
    js short vga_draw_software_cursor__L5215
    cbw
    add_load bp, ax
    mov al,[bx+cursor_step_x_deltas]
    cbw
    add_load cx, ax
    mov di,[ds:bp+video_scanline_offsets]
    add di,[video_page_offset]
    mov_load ax, cx
    shl ax,1
    add_load di, ax
    shl bx,1
    mov [bx+cursor_saved_video_offsets],di
    mov ax,[es:di]
    mov [bx+cursor_saved_pixels],ax
    xor_load ax, ax
    test byte [bx+cursor_step_ink_records],0xff
    jz short vga_draw_software_cursor__L520c
    mov_load ax, dx

vga_draw_software_cursor__L520c:
    mov [es:di],ax
    inc byte [software_cursor_saved_last_step]
    jmp short vga_draw_software_cursor__L51cf

vga_draw_software_cursor__L5215:
    dec byte [software_cursor_saved_last_step]
    mov ax,ds
    mov es,ax

vga_draw_software_cursor__L521d:
    xor_load dx, dx
    ret

; Restore saved two-pixel VGA words in reverse cursor-step order. [load+0x5220]
vga_restore_software_cursor:
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov bl,[software_cursor_saved_last_step]
    or_load bl, bl
    jz short vga_restore_software_cursor__L5244
    shl bx,1

vga_restore_software_cursor__L522f:
    mov di,[bx+cursor_saved_video_offsets]
    mov ax,[bx+cursor_saved_pixels]
    mov [es:di],ax
    dec bx
    dec bx
    jns short vga_restore_software_cursor__L522f
    xor_load bx, bx
    mov [software_cursor_saved_last_step],bl

vga_restore_software_cursor__L5244:
    mov ax,ds
    mov es,ax
    ret

; Reset mouse driver; constrain X to 0..319 and Y to 8..199, center it, and register movement/button callback. [load+0x5249]
mouse_initialize:
    mov ax,0x40
    mov es,ax
    mov byte [es:0x49],0x4
    xor_load ax, ax
    int byte MOUSE_SERVICES
    or_load ax, ax
    jz short $ +53
    mov byte [mouse_installed],0x80
    mov ax,MOUSE_SET_VERTICAL_RANGE
    mov cx,0x8
    mov dx,0xc7
    int byte MOUSE_SERVICES
    mov ax,MOUSE_SET_HORIZONTAL_RANGE
    mov cx,0x0
    mov dx,0x13f
    int byte MOUSE_SERVICES
    mov ax,MOUSE_SET_POSITION
    mov cx,0xa0
    mov dx,0x5f
    int byte MOUSE_SERVICES
    mov ax,MOUSE_SET_CALLBACK
    mov cx,0x1f
    push cs
    pop es
    mov dx,(mouse_event_callback-program_image)
    int byte MOUSE_SERVICES
    ret

; If a mouse was installed, unregister its event callback before shutdown. [load+0x5290]
mouse_remove_callback:
    test byte [mouse_installed],0xff
    jz short $ +9
    mov ax,MOUSE_SET_CALLBACK
    xor_load cx, cx
    int byte MOUSE_SERVICES
    ret

; This driver callback writes input_buttons directly. It does not update the keyboard_left_button_state/right aliases inside key_down; those are independent keyboard-hook inputs.

; Far callback: AL event flags, BL buttons, CX/DX coordinates. Store Y as 199-DX; mark movement. [load+0x529f]
mouse_event_callback:
    push ax
    push bx
    push si
    push ds
    mov si,PROGRAM_DATA_PARAGRAPH
    mov ds,si
    test al,0x1
    jz short $ +7
    mov byte [mouse_moved],0x80
    and bl,0x3
    mov [input_buttons],bl
    mov [mouse_x],cx
    mov word [mouse_y],0xc7
    sub [mouse_y],dx
    pop ds
    pop si
    pop bx
    pop ax
    retf

; Adapter 4 only: create sent.bin and write 32000 bytes captured from four EGA planes; free temporary DOS allocation. [load+0x52cb]
; dump_ega_screen_to_file
    cmp word [graphics_adapter],0x4
    jz short $ +3
    ret
    mov dx,filename_sent
    mov ah,0x3c
    xor_load cx, cx
    int byte DOS_SERVICES
    jc short $ -10
    mov_load bp, ax
    mov ax,0x7d0
    push ax
    call near dos_alloc_paragraphs
    add sp,0x2
    or_load ax, ax
    jz short $ -26
    mov es,ax
    mov ax,VIDEO_SEG_EGA
    mov ds,ax
    mov si,[video_page_offset]
    xor_load di, di
    mov dx,EGA_GC_INDEX
    mov ax,0x205
    out dx,ax
    mov cx,0xfa0
    mov ax,0x4
    out dx,ax
    mov_load bx, ax
    mov al,[si]
    mov ah,[si+0x1]
    stosw
    mov_load ax, bx
    inc ah
    out dx,ax
    mov_load bx, ax
    mov al,[si]
    mov ah,[si+0x1]
    stosw
    mov_load ax, bx
    inc ah
    out dx,ax
    mov_load bx, ax
    mov al,[si]
    mov ah,[si+0x1]
    stosw
    mov_load ax, bx
    inc ah
    out dx,ax
    mov al,[si]
    mov ah,[si+0x1]
    stosw
    add si,0x2
    loop $ -52
    mov ah,0x40
    mov_load bx, bp
    mov cx,0x7d00
    mov dx,es
    mov ds,dx
    xor_load dx, dx
    int byte DOS_SERVICES
    mov_load bx, bp
    mov ah,DOS_CLOSE
    int byte DOS_SERVICES
    push es
    call near dos_free_segment
    add sp,0x2
    mov ax,0x105
    out dx,ax
    mov ax,PROGRAM_DATA_PARAGRAPH
    mov ds,ax
    mov es,ax
    xor_load dx, dx
    mov_load bx, dx
    ret
    db 0x00 ; g data +0x5367

; Stack: adapter ID, display-name pointer. Append a compatible graphics-card choice. [load+0x5368]
graphics_menu_add_option:
    push bp
    mov_load bp, sp
    mov bx,[graphics_menu_count]
    shl bx,1
    mov ax,[bp+0x4]
    mov [bx+graphics_menu_ids],ax
    mov bx,[graphics_menu_count]
    shl bx,1
    mov ax,[bp+0x6]
    mov [bx+graphics_menu_names],ax
    push word [bp+0x6]
    call near string_length
    add sp,0x2
    mov bx,[graphics_menu_count]
    inc word [graphics_menu_count]
    shl bx,1
    mov [bx+graphics_menu_lengths],ax
    cmp word [graphics_menu_count],0x4
    jnz short $ +6
    sub_load ax, ax
    jmp short $ +5
    mov ax,0x1
    jmp short $ +2
    mov_load sp, bp
    pop bp
    ret

; Draw the text-mode border/shadow around the graphics-card choices. [load+0x53b0]
graphics_menu_draw_frame:
    push bp
    mov_load bp, sp
    sub sp,0x2a
    push si
    push word [menu_normal_background]
    push word [menu_normal_foreground]
    call near text_set_attribute
    add sp,0x4
    push word [bp+0x6]
    push word [bp+0x4]
    call near bios_set_text_cursor
    add sp,0x4
    mov byte [bp-0x2a],0xc9
    push word [bp+0xa]
    mov ax,0xcd
    push ax
    lea ax,[bp-0x29]
    push ax
    call near memory_fill_bytes
    add sp,0x6
    mov si,[bp+0xa]
    mov byte [bp+si-0x29],0xbb
    mov si,[bp+0xa]
    mov byte [bp+si-0x28],0x0
    lea ax,[bp-0x2a]
    push ax
    call near bios_draw_text_string
    add sp,0x2
    mov word [bp-0x2],0x1
    jmp short $ +5
    inc word [bp-0x2]
    mov ax,[bp+0x8]
    cmp [bp-0x2],ax
    jg short $ +77
    push word [bp+0x6]
    mov ax,[bp+0x4]
    add ax,[bp-0x2]
    push ax
    call near bios_set_text_cursor
    add sp,0x4
    mov byte [bp-0x2a],0xba
    mov byte [bp-0x29],0x0
    lea ax,[bp-0x2a]
    push ax
    call near bios_draw_text_string
    add sp,0x2
    mov ax,[bp+0x6]
    add ax,[bp+0xa]
    inc ax
    push ax
    mov ax,[bp+0x4]
    add ax,[bp-0x2]
    push ax
    call near bios_set_text_cursor
    add sp,0x4
    mov byte [bp-0x29],0xdb
    mov byte [bp-0x28],0x0
    lea ax,[bp-0x2a]
    push ax
    call near bios_draw_text_string
    add sp,0x2
    jmp short $ -84
    push word [bp+0x6]
    mov ax,[bp+0x4]
    add ax,[bp+0x8]
    inc ax
    push ax
    call near bios_set_text_cursor
    add sp,0x4
    mov byte [bp-0x2a],0xc8
    push word [bp+0xa]
    mov ax,0xcd
    push ax
    lea ax,[bp-0x29]
    push ax
    call near memory_fill_bytes
    add sp,0x6
    mov si,[bp+0xa]
    mov byte [bp+si-0x29],0xbc
    mov si,[bp+0xa]
    mov byte [bp+si-0x28],0xdb
    mov si,[bp+0xa]
    mov byte [bp+si-0x27],0x0
    lea ax,[bp-0x2a]
    push ax
    call near bios_draw_text_string
    add sp,0x2
    sub_load ax, ax
    push ax
    push word [menu_normal_foreground]
    call near text_set_attribute
    add sp,0x4
    mov ax,[bp+0x6]
    inc ax
    push ax
    mov ax,[bp+0x4]
    add ax,[bp+0x8]
    add ax,strict word 0x2
    push ax
    call near bios_set_text_cursor
    add sp,0x4
    mov ax,[bp+0xa]
    add ax,strict word 0x2
    push ax
    mov ax,0xdf
    push ax
    lea ax,[bp-0x2a]
    push ax
    call near memory_fill_bytes
    add sp,0x6
    mov si,[bp+0xa]
    mov byte [bp+si-0x28],0x0
    lea ax,[bp-0x2a]
    push ax
    call near bios_draw_text_string
    add sp,0x2
    pop si
    mov_load sp, bp
    pop bp
    ret

; Stack: option index. Center its adapter name inside the menu using the current text attribute. [load+0x54ed]
graphics_menu_draw_option:
    push bp
    mov_load bp, sp
    sub sp,0x2c
    push si
    mov bx,[bp+0x4]
    shl bx,1
    mov ax,[bx+graphics_menu_lengths]
    mov [bp-0x4],ax
    mov ax,[0x65a6]
    sub ax,[bp-0x4]
    cwd
    sub_load ax, dx
    sar ax,1
    mov [bp-0x2],ax
    push ax
    mov ax,0x20
    push ax
    lea ax,[bp-0x2c]
    push ax
    call near memory_fill_bytes
    add sp,0x6
    push word [bp-0x4]
    mov bx,[bp+0x4]
    shl bx,1
    push word [bx+graphics_menu_names]
    mov si,[bp-0x2]
    lea ax,[bp+si-0x2c]
    push ax
    call near memory_copy_bytes_forward
    add sp,0x6
    mov ax,[0x65a6]
    sub ax,[bp-0x2]
    sub ax,[bp-0x4]
    push ax
    mov ax,0x20
    push ax
    mov si,[bp-0x2]
    add si,[bp-0x4]
    lea ax,[bp+si-0x2c]
    push ax
    call near memory_fill_bytes
    add sp,0x6
    mov si,[0x65a6]
    mov byte [bp+si-0x2c],0x0
    mov ax,[0x65a2]
    inc ax
    push ax
    mov ax,[bp+0x4]
    add ax,[0x65a4]
    inc ax
    push ax
    call near bios_set_text_cursor
    add sp,0x4
    lea ax,[bp-0x2c]
    push ax
    call near bios_draw_text_string
    add sp,0x2
    pop si
    mov_load sp, bp
    pop bp
    ret

; Measure the registered graphics-card names, center the frame, and draw all options. [load+0x557f]
graphics_menu_draw_options:
    push bp
    mov_load bp, sp
    sub sp,0x6
    push si
    mov ax,[graphics_menu_count]
    mov [bp-0x6],ax
    push word [menu_normal_background]
    push word [menu_normal_foreground]
    call near text_set_attribute
    add sp,0x4
    mov word [bp-0x2],0x0
    mov word [bp-0x4],0x0
    jmp short $ +5
    inc word [bp-0x4]
    mov ax,[bp-0x6]
    cmp [bp-0x4],ax
    jnl short $ +21
    mov bx,[bp-0x4]
    shl bx,1
    mov si,[bx+graphics_menu_lengths]
    cmp [bp-0x2],si
    jnl short $ +5
    mov [bp-0x2],si
    jmp short $ -28
    mov ax,[bp-0x2]
    add ax,strict word 0x3
    and al,0xfe
    mov [bp-0x2],ax
    mov ax,[bp-0x6]
    cwd
    sub_load ax, dx
    sar ax,1
    sub ax,strict word 0xd
    neg ax
    mov [0x65a4],ax
    mov ax,[bp-0x2]
    add ax,strict word 0x2
    cwd
    sub_load ax, dx
    sar ax,1
    sub ax,strict word 0x14
    neg ax
    mov [0x65a2],ax
    mov ax,[bp-0x2]
    mov [0x65a6],ax
    push ax
    push word [bp-0x6]
    push word [0x65a2]
    push word [0x65a4]
    call near graphics_menu_draw_frame
    add sp,0x8
    push word [menu_normal_background]
    push word [menu_normal_foreground]
    call near text_set_attribute
    add sp,0x4
    mov word [bp-0x4],0x0
    jmp short $ +5
    inc word [bp-0x4]
    mov ax,[bp-0x6]
    cmp [bp-0x4],ax
    jnl short $ +13
    push word [bp-0x4]
    call near graphics_menu_draw_option
    add sp,0x2
    jmp short $ -20
    pop si
    mov_load sp, bp
    pop bp
    ret

; VISIBLE STARTUP STEP 1: the graphics-card menu (before sound, title, or landscape entry).

; Stack: initial selection. Show graphics choices, handle Up/Down and Enter, and return the selected driver ID in AX. [load+0x563a]
graphics_menu_select_adapter:
    push bp
    mov_load bp, sp
    cmp word [0x65c6],0x1
    sbb_load ax, ax
    neg ax
    push ax
    call near bios_set_video_mode
    add sp,0x2
    call near bios_hide_text_cursor
    mov ax,msg_menu_navigation
    push ax
    call near string_length
    add sp,0x2
    shr ax,1
    sub ax,strict word 0x14
    neg ax
    push ax
    mov ax,0x3
    push ax
    call near bios_set_text_cursor
    add sp,0x4
    mov ax,msg_menu_navigation
    push ax
    call near bios_draw_text_string
    add sp,0x2
    mov ax,msg_menu_select
    push ax
    call near string_length
    add sp,0x2
    shr ax,1
    sub ax,strict word 0x14
    neg ax
    push ax
    mov ax,0x4
    push ax
    call near bios_set_text_cursor
    add sp,0x4
    mov ax,msg_menu_select
    push ax
    call near bios_draw_text_string
    add sp,0x2
    call near graphics_menu_draw_options
    push word [menu_selected_background]
    push word [menu_selected_foreground]
    call near text_set_attribute
    add sp,0x4
    push word [bp+0x4]
    call near graphics_menu_draw_option
    add sp,0x2
    call near bios_read_key
    sub_load al, al
    mov cl,0x8
    shr ax,cl
    jmp short $ +93
    mov bx,[bp+0x4]
    shl bx,1
    mov ax,[bx+graphics_menu_ids]
    jmp short $ +125
    push word [menu_normal_background]
    push word [menu_normal_foreground]
    call near text_set_attribute
    add sp,0x4
    push word [bp+0x4]
    call near graphics_menu_draw_option
    add sp,0x2
    dec word [bp+0x4]
    jns short $ +9
    mov ax,[graphics_menu_count]
    dec ax
    mov [bp+0x4],ax
    jmp short $ +62
    push word [menu_normal_background]
    push word [menu_normal_foreground]
    call near text_set_attribute
    add sp,0x4
    push word [bp+0x4]
    call near graphics_menu_draw_option
    add sp,0x2
    inc word [bp+0x4]
    mov ax,[graphics_menu_count]
    cmp [bp+0x4],ax
    jl short $ +7
    mov word [bp+0x4],0x0
    jmp short $ +21

; Reviewed instruction range 571Ah..571Bh; internal routine roles remain partial. [load+0x571a]
; recovered_code_571a
    jmp short $ +19
    cmp ax,strict word 0x1c
    jz short $ -94
    cmp ax,strict word 0x48
    jz short $ -88
    cmp ax,strict word 0x50
    jz short $ -56
    jmp short $ +2
    push word [menu_selected_background]
    push word [menu_selected_foreground]
    call near text_set_attribute
    add sp,0x4
    push word [bp+0x4]
    call near graphics_menu_draw_option
    add sp,0x2
    jmp near $ -142
    mov_load sp, bp
    pop bp
    ret
    push bp
    mov_load bp, sp
    test byte [bp+0x4],0x80
    jz short $ +17
    mov bl,[bp+0x4]
    and bx,strict word 0x7f
    shl bx,1
    mov ax,[bx+adapter_name_pointers+6*2]
    jmp short $ +14
    mov al,[bp+0x4]
    cbw
    mov_load bx, ax
    shl bx,1
    mov ax,[bx+adapter_name_pointers]
    jmp short $ +2
    mov_load sp, bp
    pop bp
    ret
    push bp
    mov_load bp, sp
    sub sp,0x6
    sub_load ax, ax
    push ax
    push word [bp+0x4]
    call near dos_open_file
    add sp,0x4
    mov [bp-0x4],ax
    or_load ax, ax
    jnz short $ +31
    mov ax,msg_cannot_find
    push ax
    call near dos_print_mutable_zstring
    add sp,0x2
    push word [bp+0x4]
    call near dos_print_mutable_zstring
    add sp,0x2
    mov ax,0x1
    push ax
    call near crt_exit
    add sp,0x2
    push word [bp-0x4]
    call near dos_file_size_and_rewind
    add sp,0x2
    mov [bp-0x2],ax
    cmp word [graphics_adapter],0x2
    jnz short $ +7
    mov word [bp-0x2],0x3e80
    mov ax,[bp-0x2]
    mov cl,0x4
    shr ax,cl
    add ax,strict word 0x4
    push ax
    call near dos_alloc_paragraphs
    add sp,0x2
    mov [bp-0x6],ax
    or_load ax, ax
    jnz short $ +22
    mov ax,msg_no_memory_1
    push ax
    call near dos_print_mutable_zstring
    add sp,0x2
    mov ax,0x1
    push ax
    call near crt_exit
    add sp,0x2
    push word [bp-0x4]
    push word [bp-0x6]
    call word near [bp+0x6]
    add sp,0x4
    mov ax,[bp-0x6]
    jmp short $ +2
    mov_load sp, bp
    pop bp
    ret

; Install the CGA drawing/palette dispatch table, allocate buffers and set the BIOS graphics mode. [load+0x5804]
graphics_initialize_cga:
    push bp
    mov_load bp, sp
    mov word [graphics_set_scene_palette],(cga_set_scene_palette-program_image)
    mov word [graphics_set_landscape_colors],(cga_select_landscape_colors-program_image)
    mov word [0x6594],(resource_load_planar_as_cga-program_image)
    mov word [graphics_blit_image],(cga_blit_image-program_image)
    mov word [0x65ca],(cga_draw_font_glyph-program_image)
    mov word [graphics_draw_status_tile],(cga_draw_status_tile-program_image)
    mov word [graphics_fill_background_band],(cga_fill_background_band-program_image)
    mov word [graphics_scatter_pixels],(cga_scatter_pixels-program_image)
    mov word [graphics_fill_polygon_spans],(cga_fill_polygon_spans-program_image)
    mov word [graphics_draw_software_cursor],(cga_draw_software_cursor-program_image)
    mov word [graphics_restore_software_cursor],(cga_restore_software_cursor-program_image)
    mov word [graphics_select_front_buffer],(cga_select_front_buffer-program_image)
    mov word [graphics_select_work_buffer],(video_select_work_buffer_segment-program_image)
    mov ax,[0x6590]
    jmp short $ +63
    mov word [0x659c],0x5d17
    mov word [0x65e0],0x0
    jmp short $ +66
    mov word [0x659c],0x6b46
    mov ax,0x400
    push ax
    call near dos_alloc_paragraphs
    add sp,0x2
    mov [0x65e0],ax
    or_load ax, ax
    jnz short $ +22
    mov ax,msg_no_memory_2
    push ax
    call near dos_print_mutable_zstring
    add sp,0x2
    mov ax,0x1
    push ax
    call near crt_exit
    add sp,0x2
    jmp short $ +21

; Reviewed instruction range 5895h..5896h; internal routine roles remain partial. [load+0x5895]
; recovered_code_5895
    jmp short $ +19
    cmp ax,strict word 0x2
    jc short $ -50
    cmp ax,strict word 0x4
    jna short $ -69
    cmp ax,strict word 0x6
    jz short $ -74
    jmp short $ -62
    mov word [graphics_draw_sentry_warning_bar],(cga_draw_sentry_warning_bar-program_image)
    mov word [graphics_copy_selection_strip],0x6f10
    mov word [graphics_reveal_selection_pixels],(cga_reveal_selection_pixels-program_image)
    call near video_build_interleaved_rows
    mov ax,0x400
    push ax
    call near dos_alloc_paragraphs
    add sp,0x2
    mov [0x65d6],ax
    or_load ax, ax
    jnz short $ +22
    mov ax,msg_no_memory_3
    push ax
    call near dos_print_mutable_zstring
    add sp,0x2
    mov ax,0x1
    push ax
    call near crt_exit
    add sp,0x2
    cmp word [0x65c6],0x0
    jz short $ +7
    mov ax,0x5
    jmp short $ +5
    mov ax,0x4
    push ax
    call near bios_set_video_mode
    add sp,0x2
    mov_load sp, bp
    pop bp
    ret

; Install Tandy graphics callbacks, allocate buffers and set mode 9. [load+0x58fc]
graphics_initialize_tandy:
    push bp
    mov_load bp, sp
    mov word [graphics_set_scene_palette],(tandy_set_scene_palette-program_image)
    mov word [graphics_set_landscape_colors],0x4d8f
    mov word [0x6594],0x8e8
    mov word [graphics_blit_image],(tandy_blit_image-program_image)
    mov word [0x65ca],0xf85
    mov word [graphics_draw_status_tile],(tandy_draw_status_tile-program_image)
    mov word [graphics_fill_background_band],(tandy_fill_background_band-program_image)
    mov word [graphics_scatter_pixels],(tandy_scatter_pixels-program_image)
    mov word [graphics_fill_polygon_spans],(tandy_fill_polygon_spans-program_image)
    mov word [graphics_draw_software_cursor],(tandy_draw_software_cursor-program_image)
    mov word [graphics_restore_software_cursor],(tandy_restore_software_cursor-program_image)
    mov word [graphics_select_front_buffer],(tandy_select_front_buffer-program_image)
    mov word [graphics_select_work_buffer],(video_select_work_buffer_segment-program_image)
    mov word [0x659c],0x5eec
    mov word [graphics_draw_sentry_warning_bar],(tandy_draw_sentry_warning_bar-program_image)
    mov word [graphics_copy_selection_strip],0x6f65
    mov word [graphics_reveal_selection_pixels],(tandy_reveal_selection_pixels-program_image)
    call near video_build_four_bank_rows
    mov ax,0x800
    push ax
    call near dos_alloc_paragraphs
    add sp,0x2
    mov [0x65d6],ax
    or_load ax, ax
    jnz short $ +22
    mov ax,msg_no_memory_4
    push ax
    call near dos_print_mutable_zstring
    add sp,0x2
    mov ax,0x1
    push ax
    call near crt_exit
    add sp,0x2
    mov ax,0x9
    push ax
    call near bios_set_video_mode
    add sp,0x2
    mov_load sp, bp
    pop bp
    ret

; Install EGA callbacks (VGA DAC variant when available), enter mode 0Dh and build planar row offsets. [load+0x599b]
graphics_initialize_ega:
    push bp
    mov_load bp, sp
    cmp word [0x6590],0x6
    jnz short $ +16
    mov word [graphics_set_scene_palette],(vga_ega_set_scene_palette-program_image)
    mov word [graphics_set_landscape_colors],0x4dbf
    jmp short $ +14
    mov word [graphics_set_scene_palette],(ega_set_scene_palette-program_image)
    mov word [graphics_set_landscape_colors],0x4da7
    mov word [0x6594],0x8ac
    mov word [graphics_blit_image],(ega_blit_image-program_image)
    mov word [0x65ca],0x1087
    mov word [graphics_draw_status_tile],(ega_draw_status_tile-program_image)
    mov word [graphics_fill_background_band],(ega_fill_background_band-program_image)
    mov word [graphics_scatter_pixels],(ega_scatter_pixels-program_image)
    mov word [graphics_fill_polygon_spans],(ega_fill_polygon_spans-program_image)
    mov word [graphics_draw_software_cursor],(ega_draw_software_cursor-program_image)
    mov word [graphics_restore_software_cursor],(ega_restore_software_cursor-program_image)
    mov word [graphics_select_front_buffer],(ega_select_front_buffer-program_image)
    mov word [graphics_select_work_buffer],(ega_select_work_buffer_offset-program_image)
    mov word [0x659c],0x5ef1
    mov word [graphics_draw_sentry_warning_bar],(ega_draw_sentry_warning_bar-program_image)
    mov word [graphics_copy_selection_strip],0x6fae
    mov word [graphics_reveal_selection_pixels],(ega_reveal_selection_pixels-program_image)
    mov word [0x65d6],0xa000
    mov ax,0xd
    push ax
    call near bios_set_video_mode
    add sp,0x2
    call near video_build_planar_rows
    mov_load sp, bp
    pop bp
    ret

; Install VGA callbacks, allocate a framebuffer and enter mode 13h. [load+0x5a30]
graphics_initialize_vga:
    push bp
    mov_load bp, sp
    mov word [graphics_set_scene_palette],(vga_set_scene_palette-program_image)
    mov word [graphics_set_landscape_colors],0x4dbf
    mov word [0x6594],0x8e8
    mov word [graphics_blit_image],(vga_blit_image-program_image)
    mov word [0x65ca],0x100b
    mov word [graphics_draw_status_tile],(vga_draw_status_tile-program_image)
    mov word [graphics_fill_background_band],(vga_fill_background_band-program_image)
    mov word [graphics_scatter_pixels],(vga_scatter_pixels-program_image)
    mov word [graphics_fill_polygon_spans],(vga_fill_polygon_spans-program_image)
    mov word [graphics_draw_software_cursor],(vga_draw_software_cursor-program_image)
    mov word [graphics_restore_software_cursor],(vga_restore_software_cursor-program_image)
    mov word [graphics_select_front_buffer],(vga_select_front_buffer-program_image)
    mov word [graphics_select_work_buffer],(video_select_work_buffer_segment-program_image)
    mov word [graphics_draw_sentry_warning_bar],(vga_draw_sentry_warning_bar-program_image)
    mov word [0x659c],0x60bc
    mov word [graphics_copy_selection_strip],0x6fed
    mov word [graphics_reveal_selection_pixels],(vga_reveal_selection_pixels-program_image)
    call near video_build_linear_rows
    mov ax,0xffa
    push ax
    call near dos_alloc_paragraphs
    add sp,0x2
    mov [0x65d6],ax
    or_load ax, ax
    jnz short $ +22
    mov ax,msg_no_memory_5
    push ax
    call near dos_print_mutable_zstring
    add sp,0x2
    mov ax,0x1
    push ax
    call near crt_exit
    add sp,0x2
    mov ax,0x13
    push ax
    call near bios_set_video_mode
    add sp,0x2
    mov_load sp, bp
    pop bp
    ret

; Game-side C entry, called by the runtime with argc, argv and envp on the stack. [load+0x5acf]
game_main_bootstrap:
    push bp
    mov_load bp, sp
    sub sp,0x2
    mov word [graphics_menu_count],0x0
    call near bios_get_video_mode
    mov [bp-0x2],ax
    or_load ax, ax
    jz short $ +17
    cmp ax,strict word 0x2
    jz short $ +12
    cmp ax,strict word 0x5
    jz short $ +7
    cmp ax,strict word 0x7
    jnz short $ +34
    mov word [0x65c6],0x1
    mov word [menu_normal_background],0x0
    mov word [menu_normal_foreground],0x7
    mov word [menu_selected_background],0x7
    mov word [menu_selected_foreground],0x0
    jmp short $ +8
    mov word [0x65c6],0x0
    mov ax,0x65dc
    push ax
    call near video_detect_adapters
    add sp,0x2
    mov al,[0x65dc]
    sub_load ah, ah
    mov [0x6590],ax
    jmp short $ +39
    mov ax,msg_unsupported_adapter
    push ax
    call near dos_print_mutable_zstring
    add sp,0x2
    mov al,[0x65dc]
    sub_load ah, ah
    push ax
    call near $ -1011
    add sp,0x2
    push ax
    call near dos_print_mutable_zstring
    add sp,0x2
    mov ax,0x1
    jmp near $ +429

; Reviewed instruction range 5B51h..5B52h; internal routine roles remain partial. [load+0x5b51]
; recovered_code_5b51
    jmp short $ +19
    cmp ax,strict word 0x1
    jz short $ -40
    cmp ax,0x80
    jc short $ +9
    cmp ax,0x82
    jna short $ -50
    jmp short $ +2
    mov al,[0x65dc]
    sub_load ah, ah
    jmp near $ +212
    mov ax,name_cga
    push ax
    mov ax,0x2
    push ax
    call near graphics_menu_add_option
    add sp,0x4
    mov ax,name_tandy
    push ax
    mov ax,0x3
    push ax
    call near graphics_menu_add_option
    add sp,0x4
    mov ax,0x1
    push ax
    call near graphics_menu_select_adapter
    add sp,0x2
    mov [graphics_adapter],ax
    jmp near $ +204
    cmp word [0x65c6],0x0
    jnz short $ +45
    mov ax,name_cga
    push ax
    mov ax,0x2
    push ax
    call near graphics_menu_add_option
    add sp,0x4
    mov ax,name_ega
    push ax
    mov ax,0x4
    push ax
    call near graphics_menu_add_option
    add sp,0x4
    mov ax,0x1
    push ax
    call near graphics_menu_select_adapter
    add sp,0x2
    mov [graphics_adapter],ax
    jmp short $ +8
    mov word [graphics_adapter],0x2
    jmp near $ +145
    mov word [graphics_adapter],0x2
    jmp near $ +136
    mov ax,name_cga
    push ax
    mov ax,0x2
    push ax
    call near graphics_menu_add_option
    add sp,0x4
    mov ax,name_ega
    push ax
    mov ax,0x4
    push ax
    call near graphics_menu_add_option
    add sp,0x4
    mov ax,name_vga
    push ax
    mov ax,0x5
    push ax
    call near graphics_menu_add_option
    add sp,0x4
    mov ax,0x2
    push ax
    call near graphics_menu_select_adapter
    add sp,0x2
    mov [graphics_adapter],ax
    mov word [video_display_enable_wait_counts],0x20
    mov word [0x6066],0x20
    mov word [0x6068],0x14
    mov word [0x606a],0x30
    jmp short $ +54
    mov word [0x6590],0x0
    mov word [graphics_adapter],0x2
    jmp short $ +40

; Reviewed instruction range 5C3Bh..5C3Ch; internal routine roles remain partial. [load+0x5c3b]
; recovered_code_5c3b
    jmp short $ +38
    cmp ax,strict word 0x2
    jz short $ -109
    cmp ax,strict word 0x3
    jnz short $ +5
    jmp near $ -219
    cmp ax,strict word 0x4
    jnz short $ +5
    jmp near $ -183
    cmp ax,strict word 0x5
    jz short $ -34
    cmp ax,strict word 0x6
    jnz short $ +5
    jmp near $ -128
    jmp short $ -50
    mov ax,[graphics_adapter]
    jmp short $ +24
    call near graphics_initialize_tandy
    jmp short $ +41
    call near graphics_initialize_ega
    jmp short $ +36
    call near graphics_initialize_cga
    jmp short $ +31
    call near graphics_initialize_vga
    jmp short $ +26

; Reviewed instruction range 5C7Ah..5C7Bh; internal routine roles remain partial. [load+0x5c7a]
; recovered_code_5c7a
    jmp short $ +24
    cmp ax,strict word 0x2
    jz short $ -15
    cmp ax,strict word 0x3
    jz short $ -30
    cmp ax,strict word 0x4
    jz short $ -30
    cmp ax,strict word 0x5
    jz short $ -25
    jmp short $ +2

; Load SKYBOX.BIN and TFINAL.BIN for the chosen backend before installing input/timer hooks.
    push word [0x6594]
    mov ax,filename_skybox
    push ax
    call near $ -1317
    add sp,0x4
    mov [skybox_image_segment],ax
    push word [0x6594]
    mov ax,filename_tfinal
    push ax
    call near $ -1334
    add sp,0x4
    mov [title_final_image_segment],ax

; Enter hardware_enter_game, which tail-jumps into startup_title_and_landscape_flow.
    call near hardware_enter_game
    cmp word [0x65d6],0xa000
    jz short $ +12
    push word [0x65d6]
    call near dos_free_segment
    add sp,0x2
    cmp word [0x65e0],0x0
    jz short $ +12
    push word [0x65e0]
    call near dos_free_segment
    add sp,0x2
    push word [title_final_image_segment]
    call near dos_free_segment
    add sp,0x2
    push word [skybox_image_segment]
    call near dos_free_segment
    add sp,0x2
    push word [bp-0x2]
    call near bios_set_video_mode
    add sp,0x2
    sub_load ax, ax
    jmp short $ +2
    mov_load sp, bp
    pop bp
    ret

; Reviewed instruction range 5CFFh..6276h; internal routine roles remain partial. [load+0x5cff]
; recovered_code_5cff
    nop

recovered_sub_5d00:
    mov dx,[video_crtc_port]
    add dl,0x6

recovered_sub_5d00__L5d07:
    in al,dx
    test al,0x8
    jnz short recovered_sub_5d00__L5d07

recovered_sub_5d00__L5d0c:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L5d0c

recovered_sub_5d00__L5d11:
    in al,dx
    test al,0x1
    jnz short recovered_sub_5d00__L5d11
    ret
    call near recovered_sub_5d00
    mov dx,0xb800
    mov es,dx
    mov bp,0x1fff

recovered_sub_5d00__L5d22:
    dec byte [video_transition_steps_left]
    jns short recovered_sub_5d00__L5d34
    mov ax,PROGRAM_DATA_PARAGRAPH
    mov ds,ax
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

recovered_sub_5d00__L5d34:
    mov bl,[video_transition_steps_left]
    xor_load bh, bh
    shl bx,1
    mov al,[camera_transition_mode]
    cbw
    shl ax,1
    mov_load si, ax
    mov cx,[video_page_offset]
    add cx,[si+cga_transition_page_deltas]
    and_load cx, bp
    mov [video_pending_page_offset],cx
    mov ax,[si+video_display_enable_wait_counts]
    mov [video_transition_wait_count],ax
    shr cx,1
    mov_load si, cx
    mov dx,[video_crtc_port]
    add dl,0x6
    mov cx,[video_transition_wait_count]

recovered_sub_5d00__L5d68:
    in al,dx
    test al,0x1
    jz short recovered_sub_5d00__L5d68
    jmp short recovered_sub_5d00__L5d6f

recovered_sub_5d00__L5d6f:
    in al,dx
    test al,0x1
    jnz short recovered_sub_5d00__L5d6f
    loop recovered_sub_5d00__L5d68
    mov byte [video_transition_busy_flags],0xff
    sub dl,0x6
    mov_load cx, si
    cli
    mov al,0xc
    out dx,al
    jmp short recovered_sub_5d00__L5d86

recovered_sub_5d00__L5d86:
    inc dx
    mov_load al, ch
    out dx,al
    jmp short recovered_sub_5d00__L5d8c

recovered_sub_5d00__L5d8c:
    dec dx
    mov al,0xd
    out dx,al
    jmp short recovered_sub_5d00__L5d92

recovered_sub_5d00__L5d92:
    inc dx
    mov_load al, cl
    out dx,al
    sti
    cmp byte [camera_transition_mode],0x0
    jnz short recovered_sub_5d00__L5df7
    mov di,[video_pending_page_offset]
    xor_load si, si
    push ds
    push bx
    mov ds,word [0x65d6]
    call near cga_copy_status_strip
    pop bx
    pop ds
    mov dx,[video_crtc_port]
    add dl,0x6
    cli

recovered_sub_5d00__L5db7:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L5db7
    mov ax,[video_pending_page_offset]
    mov [video_page_offset],ax
    mov si,[bx+cga_transition_source_mode_0]
    mov di,0x18e
    add di,[video_pending_page_offset]
    mov ds,word [0x65d6]
    mov dx,0x4e
    mov cx,0x60

recovered_sub_5d00__L5dd7:
    mov_load ax, si
    mov_load bx, di
    and_load si, bp
    and_load di, bp
    movsw
    mov_load si, ax
    mov_load di, bx
    mov bx,0x2000
    or_load si, bx
    and_load di, bp
    or_load di, bx
    movsw
    add_load si, dx
    add_load di, dx
    loop recovered_sub_5d00__L5dd7
    jmp near recovered_sub_5d00__L5ede

recovered_sub_5d00__L5df7:
    cmp byte [camera_transition_mode],0x1
    jnz short recovered_sub_5d00__L5e57
    mov di,[video_pending_page_offset]
    xor_load si, si
    push ds
    push bx
    mov ds,word [0x65d6]
    call near cga_copy_status_strip
    pop bx
    pop ds
    mov dx,[video_crtc_port]
    add dl,0x6
    cli

recovered_sub_5d00__L5e17:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L5e17
    mov ax,[video_pending_page_offset]
    mov [video_page_offset],ax
    mov si,[bx+cga_transition_source_mode_1]
    mov di,0x140
    add di,[video_pending_page_offset]
    mov ds,word [0x65d6]
    mov dx,0x4e
    mov cx,0x60

recovered_sub_5d00__L5e37:
    mov_load ax, si
    mov_load bx, di
    and_load si, bp
    and_load di, bp
    movsw
    mov_load si, ax
    mov_load di, bx
    mov bx,0x2000
    or_load si, bx
    and_load di, bp
    or_load di, bx
    movsw
    add_load si, dx
    add_load di, dx
    loop recovered_sub_5d00__L5e37
    jmp near recovered_sub_5d00__L5ede

recovered_sub_5d00__L5e57:
    cmp byte [camera_transition_mode],0x2
    jnz short recovered_sub_5d00__L5ea8
    push ds
    push bx
    mov di,[video_pending_page_offset]
    mov ds,word [0x65d6]
    mov si,0xa0
    add_load di, si
    call near cga_copy_four_strip_rows
    pop bx
    pop ds
    mov si,[bx+cga_transition_source_mode_2]
    mov di,0x140
    add di,[video_pending_page_offset]
    push ds
    mov ds,word [0x65d6]
    call near cga_copy_status_strip
    pop ds
    mov dx,[video_crtc_port]
    add dl,0x6
    cli

recovered_sub_5d00__L5e8e:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L5e8e
    mov ax,[video_pending_page_offset]
    mov [video_page_offset],ax
    mov di,[video_pending_page_offset]
    xor_load si, si
    mov ds,word [0x65d6]
    call near cga_copy_four_strip_rows
    jmp short recovered_sub_5d00__L5ede

recovered_sub_5d00__L5ea8:
    mov di,[video_pending_page_offset]
    xor_load si, si
    push ds
    push bx
    mov ds,word [0x65d6]
    call near cga_copy_status_strip
    pop bx
    pop ds
    mov dx,[video_crtc_port]
    add dl,0x6
    cli

recovered_sub_5d00__L5ec1:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L5ec1
    mov ax,[video_pending_page_offset]
    mov [video_page_offset],ax
    mov si,[bx+cga_transition_source_mode_3]
    mov di,0x1e00
    add di,[video_pending_page_offset]
    mov ds,word [0x65d6]
    call near cga_copy_status_strip

recovered_sub_5d00__L5ede:
    sti
    mov ax,PROGRAM_DATA_PARAGRAPH
    mov ds,ax
    mov byte [video_transition_busy_flags],0x0
    jmp near recovered_sub_5d00__L5d22
    dec byte [video_transition_steps_left]
    ret
    call near recovered_sub_5d00
    mov bp,0xa000
    mov es,bp
    mov dx,EGA_GC_INDEX
    mov ax,0x105
    out dx,ax

recovered_sub_5d00__L5f00:
    dec byte [video_transition_steps_left]
    jns short recovered_sub_5d00__L5f12
    mov ax,PROGRAM_DATA_PARAGRAPH
    mov ds,ax
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

recovered_sub_5d00__L5f12:
    mov bl,[video_transition_steps_left]
    xor_load bh, bh
    shl bx,1
    mov al,[camera_transition_mode]
    cbw
    shl ax,1
    mov_load si, ax
    mov cx,[video_page_offset]
    add cx,[si+ega_transition_page_deltas]
    mov [video_pending_page_offset],cx
    mov ax,[si+video_display_enable_wait_counts]
    mov [video_transition_wait_count],ax
    mov dx,[video_crtc_port]
    add dl,0x6
    mov_load si, cx
    mov cx,[video_transition_wait_count]

recovered_sub_5d00__L5f42:
    in al,dx
    test al,0x1
    jz short recovered_sub_5d00__L5f42
    jmp short recovered_sub_5d00__L5f49

recovered_sub_5d00__L5f49:
    in al,dx
    test al,0x1
    jnz short recovered_sub_5d00__L5f49
    loop recovered_sub_5d00__L5f42
    mov byte [video_transition_busy_flags],0xff
    sub dl,0x6
    mov_load cx, si
    cli
    mov al,0xc
    out dx,al
    jmp short recovered_sub_5d00__L5f60

recovered_sub_5d00__L5f60:
    inc dx
    mov_load al, ch
    out dx,al
    jmp short recovered_sub_5d00__L5f66

recovered_sub_5d00__L5f66:
    dec dx
    mov al,0xd
    out dx,al
    jmp short recovered_sub_5d00__L5f6c

recovered_sub_5d00__L5f6c:
    inc dx
    mov_load al, cl
    out dx,al
    sti
    cmp byte [camera_transition_mode],0x0
    jnz short recovered_sub_5d00__L5fc7
    mov di,[video_pending_page_offset]
    mov si,[video_previous_page_offset]
    add si,0x8000
    mov dx,ds
    mov ds,bp
    mov cx,0x140
    rep movsb
    mov ds,dx
    mov dx,[video_crtc_port]
    add dl,0x6
    cli

recovered_sub_5d00__L5f97:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L5f97
    mov ax,[video_pending_page_offset]
    mov [video_page_offset],ax
    mov si,[bx+ega_transition_source_mode_0]
    add si,[video_previous_page_offset]
    add si,0x8000
    mov di,0x167
    add di,[video_pending_page_offset]
    mov ds,bp
    mov ax,0x27
    mov cx,0xc0

recovered_sub_5d00__L5fbd:
    movsb
    add_load di, ax
    add_load si, ax
    loop recovered_sub_5d00__L5fbd
    jmp near recovered_sub_5d00__L60ae

recovered_sub_5d00__L5fc7:
    cmp byte [camera_transition_mode],0x1
    jnz short recovered_sub_5d00__L601d
    mov di,[video_pending_page_offset]
    mov si,[video_previous_page_offset]
    add si,0x8000
    mov dx,ds
    mov ds,bp
    mov cx,0x140
    rep movsb
    mov ds,dx
    mov dx,[video_crtc_port]
    add dl,0x6
    cli

recovered_sub_5d00__L5fed:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L5fed
    mov ax,[video_pending_page_offset]
    mov [video_page_offset],ax
    mov si,[bx+ega_transition_source_mode_1]
    add si,[video_previous_page_offset]
    add si,0x8000
    mov di,0x140
    add di,[video_pending_page_offset]
    mov ds,bp
    mov ax,0x27
    mov cx,0xc0

recovered_sub_5d00__L6013:
    movsb
    add_load di, ax
    add_load si, ax
    loop recovered_sub_5d00__L6013
    jmp near recovered_sub_5d00__L60ae

recovered_sub_5d00__L601d:
    cmp byte [camera_transition_mode],0x2
    jnz short recovered_sub_5d00__L606a
    mov di,[video_pending_page_offset]
    mov si,[video_previous_page_offset]
    add si,0x8000
    mov dx,ds
    mov ds,bp
    mov cx,0x140
    rep movsb
    mov ds,dx
    mov dx,[video_crtc_port]
    add dl,0x6
    cli

recovered_sub_5d00__L6043:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L6043
    mov ax,[video_pending_page_offset]
    mov [video_page_offset],ax
    mov si,[bx+ega_transition_source_mode_2]
    add si,[video_previous_page_offset]
    add si,0x8000
    mov di,0x140
    add di,[video_pending_page_offset]
    mov ds,bp
    mov cx,0x140
    rep movsb
    jmp short recovered_sub_5d00__L60ae

recovered_sub_5d00__L606a:
    mov di,[video_pending_page_offset]
    mov si,[video_previous_page_offset]
    add si,0x8000
    mov dx,ds
    mov ds,bp
    mov cx,0x140
    rep movsb
    mov ds,dx
    mov dx,[video_crtc_port]
    add dl,0x6
    cli

recovered_sub_5d00__L6089:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L6089
    mov ax,[video_pending_page_offset]
    mov [video_page_offset],ax
    mov si,[bx+ega_transition_source_mode_3]
    add si,[video_previous_page_offset]
    add si,0x8000
    mov di,0x1e00
    add di,[video_pending_page_offset]
    mov ds,bp
    mov cx,0x140
    rep movsb

recovered_sub_5d00__L60ae:
    sti
    mov ax,PROGRAM_DATA_PARAGRAPH
    mov ds,ax
    mov byte [video_transition_busy_flags],0x0
    jmp near recovered_sub_5d00__L5f00
    call near recovered_sub_5d00
    mov dx,0xa000
    mov es,dx
    mov bp,[0x65d6]

recovered_sub_5d00__L60c8:
    dec byte [video_transition_steps_left]
    jns short recovered_sub_5d00__L60da
    mov ax,PROGRAM_DATA_PARAGRAPH
    mov ds,ax
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

recovered_sub_5d00__L60da:
    mov bl,[video_transition_steps_left]
    xor_load bh, bh
    shl bx,1
    mov cx,[video_page_offset]
    mov al,[camera_transition_mode]
    cbw
    shl ax,1
    mov_load si, ax
    add cx,[si+vga_transition_page_deltas]
    mov [video_pending_page_offset],cx
    mov ax,[si+video_display_enable_wait_counts]
    mov [video_transition_wait_count],ax
    mov dx,[video_crtc_port]
    shr cx,1
    cmp word [0x6590],0x5
    jz short recovered_sub_5d00__L610c
    shr cx,1

recovered_sub_5d00__L610c:
    mov_load si, cx
    add dl,0x6
    mov cx,[video_transition_wait_count]

recovered_sub_5d00__L6115:
    in al,dx
    test al,0x1
    jz short recovered_sub_5d00__L6115
    jmp short recovered_sub_5d00__L611c

recovered_sub_5d00__L611c:
    in al,dx
    test al,0x1
    jnz short recovered_sub_5d00__L611c
    loop recovered_sub_5d00__L6115
    mov byte [video_transition_busy_flags],0xff
    sub dl,0x6
    mov_load cx, si
    cli
    mov al,0xc
    out dx,al
    jmp short recovered_sub_5d00__L6133

recovered_sub_5d00__L6133:
    inc dx
    mov_load al, ch
    out dx,al
    jmp short recovered_sub_5d00__L6139

recovered_sub_5d00__L6139:
    dec dx
    mov al,0xd
    out dx,al
    jmp short recovered_sub_5d00__L613f

recovered_sub_5d00__L613f:
    inc dx
    mov_load al, cl
    out dx,al
    sti
    cmp byte [camera_transition_mode],0x0
    jnz short recovered_sub_5d00__L6193
    xor_load si, si
    mov di,[video_pending_page_offset]
    mov dx,ds
    mov ds,bp
    mov cx,0x500
    rep movsw
    mov ds,dx
    mov dx,[video_crtc_port]
    add dl,0x6
    cli

recovered_sub_5d00__L6164:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L6164
    mov ax,[video_pending_page_offset]
    mov [video_page_offset],ax
    mov si,[bx+vga_transition_source_mode_0]
    mov di,0xb38
    add di,[video_pending_page_offset]
    mov ds,bp
    mov ax,0x138
    mov bx,0xc0
    mov dx,0x4

recovered_sub_5d00__L6185:
    mov_load cx, dx
    rep movsw
    add_load di, ax
    add_load si, ax
    dec bx
    jnz short recovered_sub_5d00__L6185
    jmp near recovered_sub_5d00__L6269

recovered_sub_5d00__L6193:
    cmp byte [camera_transition_mode],0x1
    jnz short recovered_sub_5d00__L61e2
    xor_load si, si
    mov di,[video_pending_page_offset]
    mov dx,ds
    mov ds,bp
    mov cx,0x500
    rep movsw
    mov ds,dx
    mov dx,[video_crtc_port]
    add dl,0x6
    cli

recovered_sub_5d00__L61b3:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L61b3
    mov ax,[video_pending_page_offset]
    mov [video_page_offset],ax
    mov si,[bx+vga_transition_source_mode_1]
    mov di,0xa00
    add di,[video_pending_page_offset]
    mov ds,bp
    mov ax,0x138
    mov bx,0xc0
    mov dx,0x4

recovered_sub_5d00__L61d4:
    mov_load cx, dx
    rep movsw
    add_load di, ax
    add_load si, ax
    dec bx
    jnz short recovered_sub_5d00__L61d4
    jmp near recovered_sub_5d00__L6269

recovered_sub_5d00__L61e2:
    cmp byte [camera_transition_mode],0x2
    jnz short recovered_sub_5d00__L6233
    mov dx,ds
    mov di,[video_pending_page_offset]
    mov ds,bp
    mov si,0x400
    add_load di, si
    mov cx,0x300
    rep movsw
    mov ds,dx
    mov si,[bx+vga_transition_source_mode_2]
    mov di,0xa00
    add di,[video_pending_page_offset]
    mov ds,bp
    mov cx,0x500
    rep movsw
    mov ds,dx
    mov dx,[video_crtc_port]
    add dl,0x6
    cli

recovered_sub_5d00__L6219:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L6219
    mov ax,[video_pending_page_offset]
    mov [video_page_offset],ax
    xor_load si, si
    mov di,[video_pending_page_offset]
    mov ds,bp
    mov cx,0x280
    rep movsw
    jmp short recovered_sub_5d00__L6269

recovered_sub_5d00__L6233:
    xor_load si, si
    mov di,[video_pending_page_offset]
    mov dx,ds
    mov ds,bp
    mov cx,0x500
    rep movsw
    mov ds,dx
    mov dx,[video_crtc_port]
    add dl,0x6
    cli

recovered_sub_5d00__L624c:
    in al,dx
    test al,0x8
    jz short recovered_sub_5d00__L624c
    mov ax,[video_pending_page_offset]
    mov [video_page_offset],ax
    mov si,[bx+vga_transition_source_mode_3]
    mov di,0xf000
    add di,[video_pending_page_offset]
    mov ds,bp
    mov cx,0x500
    rep movsw

recovered_sub_5d00__L6269:
    sti
    mov ax,PROGRAM_DATA_PARAGRAPH
    mov ds,ax
    mov byte [video_transition_busy_flags],0x0
    jmp near recovered_sub_5d00__L60c8

; Publish the prepared eight-pixel tile strip with the selected adapter layout; EGA uses video-memory latch copies. [load+0x6277]
video_present_hud_strip:
    mov ax,[graphics_adapter]
    cmp al,0x4
    jnz short video_present_hud_strip__L629d
    mov dx,EGA_GC_INDEX
    mov ax,0x105
    out dx,ax
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov si,[video_page_offset]
    mov_load di, si
    add si,0x8000
    mov ds,ax
    mov cx,0x140
    rep movsb
    jmp short video_present_hud_strip__L62d4

video_present_hud_strip__L629d:
    cmp al,0x5
    jnz short video_present_hud_strip__L62b7
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    xor_load si, si
    mov di,[video_page_offset]
    mov ds,word [video_work_buffer_segment]
    mov cx,0x500
    rep movsw
    jmp short video_present_hud_strip__L62d4

video_present_hud_strip__L62b7:
    cmp al,0x2
    jnz short video_present_hud_strip__L62d4
    cmp byte [cga_palette_phase],0x2
    jz short video_present_hud_strip__L62d4
    mov ax,VIDEO_SEG_CGA
    mov es,ax
    xor_load si, si
    mov di,[video_page_offset]
    mov ds,word [video_work_buffer_segment]
    call near cga_copy_status_strip

video_present_hud_strip__L62d4:
    mov ax,PROGRAM_DATA_PARAGRAPH
    mov ds,ax
    mov es,ax
    xor_load bx, bx
    xor_load dx, dx
    ret

; Copy both four-row halves of the eight-pixel CGA status strip. [load+0x62e0]
cga_copy_status_strip:
    call near cga_copy_four_strip_rows
    and_load si, bp
    call near cga_copy_four_strip_rows
    ret

; Copy four strip rows with fully unrolled word moves and 8 KiB bank wrapping; SI: source, DI: destination. [load+0x62e9]
cga_copy_four_strip_rows:
    mov bp,0x1fff
    mov_load ax, si
    mov_load bx, di
    mov cx,0x2000
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    mov_load si, ax
    or_load si, cx
    mov_load di, bx
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    ret

; Reviewed instruction range 657Ah..73B1h; internal routine roles remain partial. [load+0x657a]
; recovered_code_657a
    call near recovered_sub_6583
    and_load si, bp
    call near recovered_sub_6583
    ret

recovered_sub_6583:
    mov bp,0x1fff
    mov_load ax, si
    mov_load bx, di
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    and_load di, bp
    movsw
    mov cx,0x2000
    mov_load si, ax
    or_load si, cx
    mov_load di, bx
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    mov cx,0x4000
    mov_load si, ax
    or_load si, cx
    mov_load di, bx
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    mov cx,0x6000
    mov_load si, ax
    or_load si, cx
    mov_load di, bx
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    and_load di, bp
    or_load di, cx
    movsw
    ret

recovered_sub_6583__L6b46:
    mov bp,ds
    dec byte [video_transition_steps_left]
    jns short recovered_sub_6583__L6b57
    mov ds,bp
    mov es,bp
    xor_load bx, bx
    xor_load dx, dx
    ret

recovered_sub_6583__L6b57:
    mov bl,[video_transition_steps_left]
    xor_load bh, bh
    shl bx,1
    mov al,[camera_transition_mode]
    cmp al,0x0
    jnz short recovered_sub_6583__L6bab
    dec byte [video_transition_steps_left]
    mov es,word [0x65e0]
    mov ax,VIDEO_SEG_CGA
    mov ds,ax
    mov si,0x144
    mov di,0x140
    mov cx,0xefe
    rep movsw
    mov si,0x2144
    mov di,0x2140
    mov cx,0xefe
    rep movsw
    mov ds,bp
    mov si,[bx+cga_transition_source_mode_0]
    mov di,0x18e
    mov ds,word [0x65d6]
    mov dx,0x1ffc
    mov bx,0xe04c
    mov cx,0xc0

recovered_sub_6583__L6b9f:
    movsw
    movsw
    add_load si, dx
    add_load di, dx
    xchg bx,dx
    loop recovered_sub_6583__L6b9f
    jmp short recovered_sub_6583__L6bfa

recovered_sub_6583__L6bab:
    cmp al,0x1
    jnz short recovered_sub_6583__L6bf4
    dec byte [video_transition_steps_left]
    mov es,word [0x65e0]
    mov ax,VIDEO_SEG_CGA
    mov ds,ax
    mov si,0x140
    mov di,0x144
    mov cx,0xefe
    rep movsw
    mov si,0x2140
    mov di,0x2144
    mov cx,0xefe
    rep movsw
    mov ds,bp
    mov si,[bx+cga_transition_source_mode_1]
    mov di,0x140
    mov ds,word [0x65d6]
    mov dx,0x1ffc
    mov bx,0xe04c
    mov cx,0xc0

recovered_sub_6583__L6be8:
    movsw
    movsw
    add_load si, dx
    add_load di, dx
    xchg bx,dx
    loop recovered_sub_6583__L6be8
    jmp short recovered_sub_6583__L6bfa

recovered_sub_6583__L6bf4:
    cmp al,0x2
    jnz short recovered_sub_6583__L6bfa
    jmp short recovered_sub_6583__L6bfa

recovered_sub_6583__L6bfa:
    mov ds,bp
    call near recovered_sub_5d00
    mov ax,VIDEO_SEG_CGA
    mov es,ax
    mov ds,word [0x65e0]
    mov si,0x140
    mov di,0x140
    mov dx,0x28
    mov ax,0x1fb0
    mov bx,0xe000
    push bp
    mov bp,0xc0

recovered_sub_6583__L6c1b:
    mov_load cx, dx
    rep movsw
    add_load si, ax
    add_load di, ax
    xchg ax,bx
    dec bp
    jnz short recovered_sub_6583__L6c1b
    pop ds
    jmp near recovered_sub_6583__L6b46

; Draw the top-right sentry warning bar; target-only sight fills half, square-visible sight fills all. [load+0x6c2b]
cga_draw_sentry_warning_bar:
    test byte [video_transition_busy_flags],0xff
    jns short cga_draw_sentry_warning_bar__L6c33
    ret

cga_draw_sentry_warning_bar__L6c33:
    mov al,[sentry_warning_pattern]
    or_load al, al
    jnz short cga_draw_sentry_warning_bar__L6c43
    cmp al,[previous_sentry_warning_pattern]
    jnz short cga_draw_sentry_warning_bar__L6c43
    jmp near cga_draw_sentry_warning_bar__L6cfa

cga_draw_sentry_warning_bar__L6c43:
    mov [previous_sentry_warning_pattern],al
    cbw
    mov_load si, ax
    mov ax,VIDEO_SEG_CGA
    mov es,ax
    mov di,0x203c
    add di,[video_page_offset]
    mov cx,0x1fff
    and_load di, cx
    or di,0x2000
    mov dl,0x4

cga_draw_sentry_warning_bar__L6c60:
    mov dh,0x4
    mov ax,[warning_bar_noise_low]
    mov bx,[warning_bar_noise_high]
    shl ax,1
    rcl bx,1
    shl ax,1
    rcl bx,1
    add [warning_bar_noise_low],ax
    adc [warning_bar_noise_high],bx
    mov bp,[warning_bar_noise_high]
    jmp short cga_draw_sentry_warning_bar__L6c83

cga_draw_sentry_warning_bar__L6c7f:
    shr bp,1
    shr bp,1

cga_draw_sentry_warning_bar__L6c83:
    mov_load bx, bp
    and bx,0x3
    or_load bx, si
    shl bx,1
    mov ax,[bx+cga_sentry_warning_patterns]
    mov [es:di],ax
    test di,0x2000
    jnz short cga_draw_sentry_warning_bar__L6c9f
    inc di
    inc di
    and_load di, cx
    jmp short cga_draw_sentry_warning_bar__L6ca7

cga_draw_sentry_warning_bar__L6c9f:
    inc di
    inc di
    and_load di, cx
    or di,0x2000

cga_draw_sentry_warning_bar__L6ca7:
    shr bp,1
    shr bp,1
    mov_load bx, bp
    and bx,0x3
    or_load bx, si
    shl bx,1
    mov ax,[bx+cga_sentry_warning_patterns]
    mov [es:di],ax
    test di,0x2000
    jnz short cga_draw_sentry_warning_bar__L6ccc
    add di,0xfffffffffffffffe
    and_load di, cx
    or di,0x2000
    jmp short cga_draw_sentry_warning_bar__L6cd1

cga_draw_sentry_warning_bar__L6ccc:
    add di,0x4e
    and_load di, cx

cga_draw_sentry_warning_bar__L6cd1:
    dec dh
    jnz short cga_draw_sentry_warning_bar__L6c7f
    sub di,0x9c
    and_load di, cx
    or di,0x2000
    dec dl
    jz short cga_draw_sentry_warning_bar__L6cfa
    cmp dl,0x2
    jz short cga_draw_sentry_warning_bar__L6ceb
    jmp near cga_draw_sentry_warning_bar__L6c60

cga_draw_sentry_warning_bar__L6ceb:
    cmp byte [warning_bar_sight],SIGHT_TARGET_VISIBLE
    jz short cga_draw_sentry_warning_bar__L6cf5
    jmp near cga_draw_sentry_warning_bar__L6c60

cga_draw_sentry_warning_bar__L6cf5:
    xor_load si, si
    jmp near cga_draw_sentry_warning_bar__L6c60

cga_draw_sentry_warning_bar__L6cfa:
    xor_load dx, dx
    mov ax,ds
    mov es,ax
    ret

; Tandy backend for the top-right sentry warning bar. [load+0x6d01]
tandy_draw_sentry_warning_bar:
    test byte [video_transition_busy_flags],0xff
    jns short tandy_draw_sentry_warning_bar__L6d09
    ret

tandy_draw_sentry_warning_bar__L6d09:
    mov al,[sentry_warning_pattern]
    or_load al, al
    jnz short tandy_draw_sentry_warning_bar__L6d19
    cmp al,[previous_sentry_warning_pattern]
    jnz short tandy_draw_sentry_warning_bar__L6d19
    jmp near tandy_draw_sentry_warning_bar__L6da2

tandy_draw_sentry_warning_bar__L6d19:
    mov [previous_sentry_warning_pattern],al
    cbw
    mov_load bx, ax
    mov ax,VIDEO_SEG_MONO
    mov es,ax
    mov di,0x2078
    xor_load cx, cx
    mov dl,0x4

tandy_draw_sentry_warning_bar__L6d2b:
    mov dh,0x4
    mov ax,[warning_bar_noise_low]
    mov bp,[warning_bar_noise_high]
    shl ax,1
    rcl bp,1
    shl ax,1
    rcl bp,1
    add [warning_bar_noise_low],ax
    adc [warning_bar_noise_high],bp
    mov bp,[warning_bar_noise_high]
    jmp short tandy_draw_sentry_warning_bar__L6d4e

tandy_draw_sentry_warning_bar__L6d4a:
    shr bp,1
    shr bp,1

tandy_draw_sentry_warning_bar__L6d4e:
    mov_load si, bp
    and si,0x3
    or_load si, bx
    shl si,1
    shl si,1
    lea si,[si+tandy_sentry_warning_patterns]
    mov cl,0x2
    rep movsw
    shr bp,1
    shr bp,1
    mov_load si, bp
    and si,0x3
    or_load si, bx
    shl si,1
    shl si,1
    lea si,[si+tandy_sentry_warning_patterns]
    mov cl,0x2
    rep movsw
    add di,0x1ff8
    test di,0x8000
    jz short tandy_draw_sentry_warning_bar__L6d86
    add di,0x80a0

tandy_draw_sentry_warning_bar__L6d86:
    dec dh
    jnz short tandy_draw_sentry_warning_bar__L6d4a
    add di,0x1f60
    dec dl
    jz short tandy_draw_sentry_warning_bar__L6da2
    cmp dl,0x2
    jnz short tandy_draw_sentry_warning_bar__L6d2b
    cmp byte [warning_bar_sight],SIGHT_TARGET_VISIBLE
    jnz short tandy_draw_sentry_warning_bar__L6d2b
    xor_load bx, bx
    jmp short tandy_draw_sentry_warning_bar__L6d2b

tandy_draw_sentry_warning_bar__L6da2:
    xor_load dx, dx
    mov ax,ds
    mov es,ax
    ret

; EGA backend for the top-right sentry warning bar. [load+0x6da9]
ega_draw_sentry_warning_bar:
    test byte [video_transition_busy_flags],0xff
    jns short ega_draw_sentry_warning_bar__L6db1
    ret

ega_draw_sentry_warning_bar__L6db1:
    test byte [0x244],0xff
    jns short ega_draw_sentry_warning_bar__L6db9
    ret

ega_draw_sentry_warning_bar__L6db9:
    mov al,[sentry_warning_pattern]
    or_load al, al
    jnz short ega_draw_sentry_warning_bar__L6dc9
    cmp al,[previous_sentry_warning_pattern]
    jnz short ega_draw_sentry_warning_bar__L6dc9
    jmp near ega_draw_sentry_warning_bar__L6e5f

ega_draw_sentry_warning_bar__L6dc9:
    mov [previous_sentry_warning_pattern],al
    cbw
    mov_load si, ax
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov di,0x46
    add di,[video_page_offset]
    mov dx,EGA_GC_INDEX
    mov ax,0xa05
    out dx,ax
    mov ax,0x7
    out dx,ax
    mov cl,0x4

ega_draw_sentry_warning_bar__L6de8:
    mov ch,0x4
    mov ax,[warning_bar_noise_low]
    mov bp,[warning_bar_noise_high]
    shl ax,1
    rcl bp,1
    shl ax,1
    rcl bp,1
    add [warning_bar_noise_low],ax
    adc [warning_bar_noise_high],bp
    mov bp,[warning_bar_noise_high]
    mov al,0x8
    jmp short ega_draw_sentry_warning_bar__L6e0d

ega_draw_sentry_warning_bar__L6e09:
    shr bp,1
    shr bp,1

ega_draw_sentry_warning_bar__L6e0d:
    mov_load bx, bp
    and bx,0x3
    or_load bx, si
    mov ah,[bx+ega_sentry_warning_patterns]
    out dx,ax
    mov byte [es:di],0x0
    not ah
    out dx,ax
    and byte [es:di],0x9
    inc di
    shr bp,1
    shr bp,1
    mov_load bx, bp
    and bx,0x3
    or_load bx, si
    mov ah,[bx+ega_sentry_warning_patterns]
    out dx,ax
    mov byte [es:di],0x0
    not ah
    out dx,ax
    and byte [es:di],0x9
    add di,0x27
    dec ch
    jnz short ega_draw_sentry_warning_bar__L6e09
    sub di,0x9e
    dec cl
    jz short ega_draw_sentry_warning_bar__L6e5f
    cmp cl,0x2
    jnz short ega_draw_sentry_warning_bar__L6de8
    cmp byte [warning_bar_sight],SIGHT_TARGET_VISIBLE
    jnz short ega_draw_sentry_warning_bar__L6de8
    xor_load si, si
    jmp short ega_draw_sentry_warning_bar__L6de8

ega_draw_sentry_warning_bar__L6e5f:
    mov ax,0x105
    out dx,ax
    xor_load dx, dx
    mov ax,ds
    mov es,ax
    ret

; VGA backend for the top-right sentry warning bar. [load+0x6e6a]
vga_draw_sentry_warning_bar:
    test byte [video_transition_busy_flags],0xff
    jns short vga_draw_sentry_warning_bar__L6e72
    ret

vga_draw_sentry_warning_bar__L6e72:
    mov al,[sentry_warning_pattern]
    or_load al, al
    jnz short vga_draw_sentry_warning_bar__L6e82
    cmp al,[previous_sentry_warning_pattern]
    jnz short vga_draw_sentry_warning_bar__L6e82
    jmp near vga_draw_sentry_warning_bar__L6f09

vga_draw_sentry_warning_bar__L6e82:
    mov [previous_sentry_warning_pattern],al
    cbw
    mov_load bx, ax
    mov ax,VIDEO_SEG_EGA
    mov es,ax
    mov di,0x230
    add di,[video_page_offset]
    xor_load cx, cx
    mov dl,0x4

vga_draw_sentry_warning_bar__L6e98:
    mov dh,0x4
    mov ax,[warning_bar_noise_low]
    mov bp,[warning_bar_noise_high]
    shl ax,1
    rcl bp,1
    shl ax,1
    rcl bp,1
    add [warning_bar_noise_low],ax
    adc [warning_bar_noise_high],bp
    mov bp,[warning_bar_noise_high]
    jmp short vga_draw_sentry_warning_bar__L6ebb

vga_draw_sentry_warning_bar__L6eb7:
    shr bp,1
    shr bp,1

vga_draw_sentry_warning_bar__L6ebb:
    mov_load si, bp
    and si,0x3
    or_load si, bx
    shl si,1
    shl si,1
    shl si,1
    lea si,[si+vga_sentry_warning_patterns]
    mov cl,0x4
    rep movsw
    shr bp,1
    shr bp,1
    mov_load si, bp
    and si,0x3
    or_load si, bx
    shl si,1
    shl si,1
    shl si,1
    lea si,[si+vga_sentry_warning_patterns]
    mov cl,0x4
    rep movsw
    add di,0x130
    dec dh
    jnz short vga_draw_sentry_warning_bar__L6eb7
    sub di,0x4f0
    dec dl
    jz short vga_draw_sentry_warning_bar__L6f09
    cmp dl,0x2
    jnz short vga_draw_sentry_warning_bar__L6e98
    cmp byte [warning_bar_sight],SIGHT_TARGET_VISIBLE
    jnz short vga_draw_sentry_warning_bar__L6e98
    xor_load bx, bx
    jmp short vga_draw_sentry_warning_bar__L6e98

vga_draw_sentry_warning_bar__L6f09:
    xor_load dx, dx
    mov ax,ds
    mov es,ax
    ret
    shl bx,1
    mov di,[0x295]
    mov_load si, di
    add_load di, bx
    add di,[video_page_offset]
    mov bx,0xb800
    mov es,bx
    mov ds,word [0x65d6]
    mov_load bx, ax
    shl ax,1
    mov bp,0x1fff
    mov dx,0x50
    sub_load dx, ax
    mov ax,0x2000
    mov cx,0x60

vga_draw_sentry_warning_bar__L6f39:
    push cx
    mov_load cx, bx
    push si
    push di

vga_draw_sentry_warning_bar__L6f3e:
    and_load di, bp
    movsw
    loop vga_draw_sentry_warning_bar__L6f3e
    pop di
    pop si
    or_load si, ax
    mov_load cx, bx

vga_draw_sentry_warning_bar__L6f49:
    and_load di, bp
    or_load di, ax
    movsw
    loop vga_draw_sentry_warning_bar__L6f49
    add_load di, dx
    add_load si, dx
    xor_load si, ax
    pop cx
    loop vga_draw_sentry_warning_bar__L6f39
    mov bx,PROGRAM_DATA_PARAGRAPH
    mov ds,bx
    mov es,bx
    xor_load bx, bx
    mov_load dx, bx
    ret
    shl bx,1
    shl bx,1
    mov di,[0x295]
    mov_load si, di
    add_load di, bx
    mov bx,0xb000
    mov es,bx
    mov ds,word [0x65d6]
    shl ax,1
    mov_load bx, ax
    shl ax,1
    mov bp,0x2000
    sub_load bp, ax
    mov dx,0x80a0
    sub_load dx, ax
    mov ax,0xc0

vga_draw_sentry_warning_bar__L6f8d:
    mov_load cx, bx
    rep movsw
    add_load di, bp
    add_load si, bp
    test di,0x8000
    jz short vga_draw_sentry_warning_bar__L6f9f
    add_load di, dx
    add_load si, dx

vga_draw_sentry_warning_bar__L6f9f:
    dec ax
    jnz short vga_draw_sentry_warning_bar__L6f8d
    mov bx,PROGRAM_DATA_PARAGRAPH
    mov ds,bx
    mov es,bx
    mov_load bx, ax
    mov_load dx, bx
    ret
    mov di,[0x295]
    add di,[video_page_offset]
    mov_load si, di
    add si,0x8000
    add_load di, bx
    mov bx,0xa000
    mov es,bx
    mov ds,bx
    mov_load bx, ax
    mov dx,EGA_GC_INDEX
    mov ax,0x105
    out dx,ax
    mov dx,0x28
    sub_load dx, bx
    mov ax,0xc0

vga_draw_sentry_warning_bar__L6fd6:
    mov_load cx, bx
    rep movsb
    add_load di, dx
    add_load si, dx
    dec ax
    jnz short vga_draw_sentry_warning_bar__L6fd6
    mov bx,PROGRAM_DATA_PARAGRAPH
    mov ds,bx
    mov es,bx
    mov_load bx, ax
    mov_load dx, bx
    ret
    shl bx,1
    shl bx,1
    shl bx,1
    mov di,[0x295]
    mov_load si, di
    add_load di, bx
    add di,[video_page_offset]
    mov bx,0xa000
    mov es,bx
    mov ds,word [0x65d6]
    shl ax,1
    shl ax,1
    mov_load bx, ax
    shl ax,1
    mov dx,0x140
    sub_load dx, ax
    mov ax,0xc0

vga_draw_sentry_warning_bar__L7018:
    mov_load cx, bx
    rep movsw
    add_load di, dx
    add_load si, dx
    dec ax
    jnz short vga_draw_sentry_warning_bar__L7018
    mov bx,PROGRAM_DATA_PARAGRAPH
    mov ds,bx
    mov es,bx
    mov_load bx, ax
    mov_load dx, bx
    ret

; Randomized masked source-to-screen copy for the selected view region. [load+0x702f]
cga_reveal_selection_pixels:
    mov cx,0xb800
    mov es,cx
    mov al,[selection_view_total_columns]
    mov_load dl, al
    dec al
    jz short cga_reveal_selection_pixels__L7049
    mov di,0xffff

cga_reveal_selection_pixels__L7040:
    inc di
    shl al,1
    jnc short cga_reveal_selection_pixels__L7040
    mov al,[di+placement_count_masks]

cga_reveal_selection_pixels__L7049:
    mov_load dh, al

cga_reveal_selection_pixels__L704b:
    mov byte [selection_copy_inner_countdown],0xff

cga_reveal_selection_pixels__L7050:
    mov ax,[copy_sequence_low]
    mov cx,[copy_sequence_high]
    shl ax,1
    rcl cx,1
    shl ax,1
    rcl cx,1
    add [copy_sequence_low],ax
    adc [copy_sequence_high],cx
    mov bp,[copy_sequence_high]
    mov_load ax, bp
    add al,[selection_copy_column_accumulator]
    and_load al, dh
    cmp_load al, dl
    jc short cga_reveal_selection_pixels__L7079
    sub_load al, dl

cga_reveal_selection_pixels__L7079:
    mov [selection_copy_column_accumulator],al
    mov_load ax, bp
    mov_load al, ah
    shr al,1
    and ah,0x3f
    add_load al, ah
    xor_load ah, ah
    add al,0x8
    mov_load di, ax
    shl di,1
    mov di,[di+video_scanline_offsets]
    mov_load si, di
    mov al,[selection_copy_column_accumulator]
    cbw
    mov_load bx, ax
    shl ax,1
    add_load si, ax
    add bl,[selection_view_origin_column]
    shl bx,1
    add_load di, bx
    add di,[video_page_offset]
    and di,0x1fff
    test si,0x2000
    jz short cga_reveal_selection_pixels__L70b9
    or di,0x2000

cga_reveal_selection_pixels__L70b9:
    mov_load bx, bp
    rol bl,1
    rol bl,1
    and bx,0x3
    shl bx,1
    mov bx,[bx+cga_random_copy_nibble_masks]
    mov bp,ds
    mov ds,word [0x65d6]
    mov ax,[si]
    and_load ax, bx
    mov cx,[es:di]
    not bx
    and_load cx, bx
    or_load ax, cx
    stosw
    mov ds,bp
    test byte [selection_copy_interrupt_flags],0x80
    jnz short cga_reveal_selection_pixels__L70f9
    dec byte [selection_copy_inner_countdown]
    jnz short cga_reveal_selection_pixels__L70f6
    push dx
    pop dx
    dec byte [selection_copy_iteration_budget]
    jz short cga_reveal_selection_pixels__L70f9
    jmp near cga_reveal_selection_pixels__L704b

cga_reveal_selection_pixels__L70f6:
    jmp near cga_reveal_selection_pixels__L7050

cga_reveal_selection_pixels__L70f9:
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Randomized masked source-to-screen copy for the selected view region. [load+0x7102]
tandy_reveal_selection_pixels:
    mov cx,0xb000
    mov es,cx
    mov al,[selection_view_total_columns]
    mov_load dl, al
    dec al
    jz short tandy_reveal_selection_pixels__L711c
    mov di,0xffff

tandy_reveal_selection_pixels__L7113:
    inc di
    shl al,1
    jnc short tandy_reveal_selection_pixels__L7113
    mov al,[di+placement_count_masks]

tandy_reveal_selection_pixels__L711c:
    mov_load dh, al

tandy_reveal_selection_pixels__L711e:
    mov byte [selection_copy_inner_countdown],0xff

tandy_reveal_selection_pixels__L7123:
    mov ax,[copy_sequence_low]
    mov cx,[copy_sequence_high]
    shl ax,1
    rcl cx,1
    shl ax,1
    rcl cx,1
    add [copy_sequence_low],ax
    adc [copy_sequence_high],cx
    mov bp,[copy_sequence_high]
    mov_load ax, bp
    add al,[selection_copy_column_accumulator]
    and_load al, dh
    cmp_load al, dl
    jc short tandy_reveal_selection_pixels__L714c
    sub_load al, dl

tandy_reveal_selection_pixels__L714c:
    mov [selection_copy_column_accumulator],al
    mov_load ax, bp
    mov_load al, ah
    shr al,1
    and ah,0x3f
    add_load al, ah
    xor_load ah, ah
    add al,0x8
    mov_load di, ax
    shl di,1
    mov di,[di+video_scanline_offsets]
    mov_load si, di
    mov_load bx, bp
    rol bl,1
    rol bl,1
    and bx,0x3
    mov al,[selection_copy_column_accumulator]
    cbw
    mov_load cx, ax
    shl ax,1
    shl ax,1
    add_load ax, bx
    add_load si, ax
    add cl,[selection_view_origin_column]
    shl cx,1
    shl cx,1
    add_load cx, bx
    add_load di, cx
    mov bp,ds
    mov ds,word [0x65d6]
    movsb
    mov ds,bp
    test byte [selection_copy_interrupt_flags],0x80
    jnz short tandy_reveal_selection_pixels__L71af
    dec byte [selection_copy_inner_countdown]
    jnz short tandy_reveal_selection_pixels__L71ac
    push dx
    pop dx
    dec byte [selection_copy_iteration_budget]
    jz short tandy_reveal_selection_pixels__L71af
    jmp near tandy_reveal_selection_pixels__L711e

tandy_reveal_selection_pixels__L71ac:
    jmp near tandy_reveal_selection_pixels__L7123

tandy_reveal_selection_pixels__L71af:
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Randomized masked source-to-screen copy for the selected view region. [load+0x71b8]
ega_reveal_selection_pixels:
    mov cx,0xa000
    mov es,cx
    mov dx,EGA_GC_INDEX
    mov al,[selection_view_total_columns]
    mov [ega_selection_column_limit],al
    dec al
    jz short ega_reveal_selection_pixels__L71d6
    mov di,0xffff

ega_reveal_selection_pixels__L71cd:
    inc di
    shl al,1
    jnc short ega_reveal_selection_pixels__L71cd
    mov al,[di+placement_count_masks]

ega_reveal_selection_pixels__L71d6:
    mov [ega_selection_column_mask],al

ega_reveal_selection_pixels__L71d9:
    mov byte [selection_copy_inner_countdown],0xc8

ega_reveal_selection_pixels__L71de:
    mov ax,[copy_sequence_low]
    mov cx,[copy_sequence_high]
    shl ax,1
    rcl cx,1
    shl ax,1
    rcl cx,1
    add [copy_sequence_low],ax
    adc [copy_sequence_high],cx
    mov bp,[copy_sequence_high]
    mov_load ax, bp
    add al,[selection_copy_column_accumulator]
    and al,[ega_selection_column_mask]
    cmp al,[ega_selection_column_limit]
    jc short ega_reveal_selection_pixels__L720d
    sub al,[ega_selection_column_limit]

ega_reveal_selection_pixels__L720d:
    mov [selection_copy_column_accumulator],al
    mov_load ax, bp
    mov_load al, ah
    shr al,1
    and ah,0x3f
    add_load al, ah
    xor_load ah, ah
    add al,0x8
    mov_load di, ax
    shl di,1
    mov di,[di+video_scanline_offsets]
    add di,[video_page_offset]
    mov_load si, di
    add si,0x8000
    mov al,[selection_copy_column_accumulator]
    cbw
    add_load si, ax
    add al,[selection_view_origin_column]
    add_load di, ax
    mov_load bx, bp
    rol bl,1
    rol bl,1
    and bx,0x3
    mov ch,[bx+ega_random_copy_bit_masks]
    mov bp,ds
    mov ds,word [0x65d6]
    cli
    mov ax,0x205
    out dx,ax
    mov_load ah, ch
    mov al,0x8
    out dx,ax
    mov ax,0x304
    out dx,ax
    mov bh,[si]
    and_load bh, ch
    neg bh
    rol bx,1
    dec ah
    out dx,ax
    mov bh,[si]
    and_load bh, ch
    neg bh
    rol bx,1
    dec ah
    out dx,ax
    mov bh,[si]
    and_load bh, ch
    neg bh
    rol bx,1
    dec ah
    out dx,ax
    mov bh,[si]
    and_load bh, ch
    neg bh
    rol bx,1
    dec ah
    mov ah,[es:di]
    mov [es:di],bl
    shr ch,1
    mov_load ah, ch
    mov al,0x8
    out dx,ax
    mov ax,0x304
    out dx,ax
    mov bh,[si]
    and_load bh, ch
    neg bh
    rol bx,1
    dec ah
    out dx,ax
    mov bh,[si]
    and_load bh, ch
    neg bh
    rol bx,1
    dec ah
    out dx,ax
    mov bh,[si]
    and_load bh, ch
    neg bh
    rol bx,1
    dec ah
    out dx,ax
    mov bh,[si]
    and_load bh, ch
    neg bh
    rol bx,1
    dec ah
    mov ah,[es:di]
    mov [es:di],bl
    sti
    mov ds,bp
    test byte [selection_copy_interrupt_flags],0x80
    jnz short ega_reveal_selection_pixels__L72e9
    dec byte [selection_copy_inner_countdown]
    jnz short ega_reveal_selection_pixels__L72e6
    push dx
    pop dx
    dec byte [selection_copy_iteration_budget]
    jz short ega_reveal_selection_pixels__L72e9
    jmp near ega_reveal_selection_pixels__L71d9

ega_reveal_selection_pixels__L72e6:
    jmp near ega_reveal_selection_pixels__L71de

ega_reveal_selection_pixels__L72e9:
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; Randomized masked source-to-screen copy for the selected view region. [load+0x72f2]
vga_reveal_selection_pixels:
    mov cx,0xa000
    mov es,cx
    mov al,[selection_view_total_columns]
    mov_load dl, al
    dec al
    jz short vga_reveal_selection_pixels__L730c
    mov di,0xffff

vga_reveal_selection_pixels__L7303:
    inc di
    shl al,1
    jnc short vga_reveal_selection_pixels__L7303
    mov al,[di+placement_count_masks]

vga_reveal_selection_pixels__L730c:
    mov_load dh, al

vga_reveal_selection_pixels__L730e:
    mov byte [selection_copy_inner_countdown],0xff

vga_reveal_selection_pixels__L7313:
    mov ax,[copy_sequence_low]
    mov cx,[copy_sequence_high]
    shl ax,1
    rcl cx,1
    shl ax,1
    rcl cx,1
    add [copy_sequence_low],ax
    adc [copy_sequence_high],cx
    mov bp,[copy_sequence_high]
    mov_load ax, bp
    add al,[selection_copy_column_accumulator]
    and_load al, dh
    cmp_load al, dl
    jc short vga_reveal_selection_pixels__L733c
    sub_load al, dl

vga_reveal_selection_pixels__L733c:
    mov [selection_copy_column_accumulator],al
    mov_load ax, bp
    mov_load al, ah
    shr al,1
    and ah,0x3f
    add_load al, ah
    xor_load ah, ah
    add al,0x8
    mov_load di, ax
    shl di,1
    mov di,[di+video_scanline_offsets]
    mov_load si, di
    add di,[video_page_offset]
    mov_load bx, bp
    rol bl,1
    rol bl,1
    and bx,0x3
    shl bx,1
    mov al,[selection_copy_column_accumulator]
    cbw
    mov_load cx, ax
    shl ax,1
    shl ax,1
    shl ax,1
    add_load ax, bx
    add_load si, ax
    add cl,[selection_view_origin_column]
    shl cx,1
    shl cx,1
    shl cx,1
    add_load cx, bx
    add_load di, cx
    mov bp,ds
    mov ds,word [0x65d6]
    movsw
    mov ds,bp
    test byte [selection_copy_interrupt_flags],0x80
    jnz short vga_reveal_selection_pixels__L73a9
    dec byte [selection_copy_inner_countdown]
    jnz short vga_reveal_selection_pixels__L73a6
    push dx
    pop dx
    dec byte [selection_copy_iteration_budget]
    jz short vga_reveal_selection_pixels__L73a9
    jmp near vga_reveal_selection_pixels__L730e

vga_reveal_selection_pixels__L73a6:
    jmp near vga_reveal_selection_pixels__L7313

vga_reveal_selection_pixels__L73a9:
    mov ax,ds
    mov es,ax
    xor_load bx, bx
    mov_load dx, bx
    ret

; DOS entry after EXEPACK: require DOS 2+, establish stack/data segments, resize memory, clear initial zero data, prepare runtime and invoke game_main_bootstrap. [load+0x73b2]
crt_entry:
    mov ah,DOS_GET_VERSION
    int byte DOS_SERVICES
    cmp al,0x2
    jnc short $ +4
    int byte DOS_TERMINATE_LEGACY
    mov di,PROGRAM_DATA_PARAGRAPH
    mov si,[0x2]
    sub_load si, di
    cmp si,0x1000
    jc short $ +5
    mov si,0x1000
    cli
    mov ss,di
    add sp,0x65ee
    sti
    jnc short $ +18
    push ss
    pop ds
    call near crt_print_error_prefix
    xor_load ax, ax
    push ax
    call near crt_write_error_message
    mov ax,0x4cff
    int byte DOS_SERVICES
    and sp,strict word 0xfffe
    mov [ss:crt_near_break],sp
    mov [ss:crt_initial_break],sp
    mov_load ax, si
    mov cl,0x4
    shl ax,cl
    dec ax
    mov [ss:crt_near_limit],ax
    add_load si, di
    mov [0x2],si
    mov bx,es
    sub_load bx, si
    neg bx
    mov ah,DOS_RESIZE_BLOCK
    int byte DOS_SERVICES
    mov word [ss:crt_psp_segment],ds
    push ss
    pop es
    cld

; Clear only the runtime initial zero-data range 658Ch..65EFh; this is not the full game-data segment.
    mov di,0x658c
    mov cx,0x65f0
    sub_load cx, di
    xor_load ax, ax
    rep stosb
    push ss
    pop ds
    call near crt_initialize_io_and_hooks
    push ss
    pop ds
    call near crt_prepare_environment
    call near crt_prepare_argv
    xor_load bp, bp

; Push envp, argv, argc for the game-side C entry; its AX result becomes the DOS exit status.
    push word [crt_envp]
    push word [crt_argv]
    push word [crt_argc]
    call near game_main_bootstrap
    push ax
    call near crt_exit

; Installed INT 00h handler: select runtime error 3 and enter the fatal-error path; does not IRET. [load+0x7447]
crt_divide_error:
    mov ax,PROGRAM_DATA_PARAGRAPH
    mov ds,ax
    mov ax,0x3
    mov word [ss:crt_exit_callback],(crt_exit-program_image)

; AX: runtime error code. Print prefix and message, then invoke the exit callback with status FFh. [load+0x7456]
crt_fatal_error:
    push ax
    call near crt_print_error_prefix
    call near crt_write_error_message
    mov ax,0xff
    push ax
    call word near [crt_exit_callback]
    db 0x00 ; g data +0x7465

; Save/hook INT 00h, recover inherited C_FILE_INFO handle flags from the environment, query DOS device flags, run initialization callbacks. [load+0x7466]
crt_initialize_io_and_hooks:
    mov ah,DOS_GET_VERSION
    int byte DOS_SERVICES
    mov [crt_dos_version],ax
    mov ax,0x3500
    int byte DOS_SERVICES
    mov [crt_saved_int00_offset],bx
    mov word [crt_saved_int00_segment],es
    push cs
    pop ds
    mov ax,0x2500
    mov dx,(crt_divide_error-program_image)
    int byte DOS_SERVICES
    push ss
    pop ds
    mov cx,[crt_optional_service_hook+2]
    jcxz $ +48
    mov es,word [crt_psp_segment]
    mov si,[es:0x2c]
    lds ax,word [crt_service_init_argument]
    mov dx,ds
    xor_load bx, bx
    call word far [ss:crt_optional_service_hook]
    jnc short $ +7
    push ss
    pop ds
    jmp near $ +336
    lds ax,word [ss:crt_service_secondary_argument]
    mov dx,ds
    mov bx,0x3
    call word far [ss:crt_optional_service_hook]
    push ss
    pop ds
    mov es,word [crt_psp_segment]
    mov cx,[es:0x2c]
    jcxz $ +56
    mov es,cx
    xor_load di, di
    cmp byte [es:di],0x0
    jz short $ +46
    mov cx,0xc
    mov si,crt_file_info_name
    repe cmpsb
    jz short $ +13
    mov cx,0x7fff
    xor_load ax, ax
    repne scasb
    jnz short $ +27
    jmp short $ -25
    push es
    push ds
    pop es
    pop ds
    mov_load si, di
    mov di,0x6310
    lodsb
    cbw
    xchg ax,cx
    lodsb
    inc al
    jz short $ +3
    dec ax
    stosb
    loop $ -7
    push ss
    pop ds
    mov bx,0x4
    and byte [bx+crt_file_flags],0xbf
    mov ax,0x4400
    int byte DOS_SERVICES
    jc short $ +12
    test dl,0x80
    jz short $ +7
    or byte [bx+crt_file_flags],0x40
    dec bx
    jns short $ -23
    mov si,crt_none
    mov di,crt_none
    call near crt_run_far_callbacks_reverse
    mov si,crt_none
    mov di,crt_none
    call near crt_run_near_callbacks_reverse
    ret

; Stack: exit status. Run cleanup callbacks, check the null region, close flagged handles 5..19, restore runtime hooks and terminate through DOS. [load+0x752a]
crt_exit:
    push bp
    mov_load bp, sp
    mov si,0x658c
    mov di,0x658c
    call near crt_run_near_callbacks_reverse
    mov si,crt_none
    mov di,crt_none
    call near crt_run_near_callbacks_reverse
    jmp short $ +5

; Alternate exit entry skips the first callback lists; shares remaining cleanup with crt_exit. [load+0x7541]
crt_exit_shared_cleanup:
    push bp
    mov_load bp, sp
    mov si,crt_none
    mov di,crt_none
    call near crt_run_near_callbacks_reverse
    mov si,crt_none
    mov di,crt_none
    call near crt_run_far_callbacks_reverse
    call near crt_check_null_assignment
    or_load ax, ax
    jz short $ +13
    cmp word [bp+0x4],0x0
    jnz short $ +7
    mov word [bp+0x4],0xff
    mov cx,0xf
    mov bx,0x5
    test byte [bx+crt_file_flags],0x1
    jz short $ +6
    mov ah,DOS_CLOSE
    int byte DOS_SERVICES
    inc bx
    loop $ -12
    call near crt_restore_runtime_hooks
    mov ax,[bp+0x4]
    mov ah,DOS_EXIT_PROCESS
    int byte DOS_SERVICES

; Run optional runtime service cleanup; restore the saved divide-error vector and any additional configured interrupt hook. [load+0x7586]
crt_restore_runtime_hooks:
    mov cx,[crt_optional_service_hook+2]
    jcxz $ +9
    mov bx,0x2
    call word far [crt_optional_service_hook]
    push ds
    lds dx,word [crt_saved_int00_offset]
    mov ax,0x2500
    int byte DOS_SERVICES
    pop ds
    cmp byte [crt_optional_interrupt_enabled],0x0
    jz short $ +15
    push ds
    mov al,[crt_optional_interrupt_number]
    lds dx,word [crt_optional_interrupt_vector]
    mov ah,0x25
    int byte DOS_SERVICES
    pop ds
    ret

; SI: start, DI: end of near callback table. Walk backward by words, skip null entries and call CX. [load+0x75b3]
crt_run_near_callbacks_reverse:
    cmp_load si, di
    jnc short $ +12
    dec di
    dec di
    mov cx,[di]
    jcxz crt_run_near_callbacks_reverse
    call cx
    jmp short crt_run_near_callbacks_reverse
    ret

; SI: start, DI: end of far callback table. Walk backward by four-byte pointers, skipping null entries. [load+0x75c2]
crt_run_far_callbacks_reverse:
    cmp_load si, di
    jnc short $ +16
    sub di,0x4
    mov ax,[di]
    or ax,[di+0x2]
    jz short crt_run_far_callbacks_reverse
    call word far [di]
    jmp short crt_run_far_callbacks_reverse
    ret
    db 0x00 ; g data +0x75d5

; Write the error-message separators/prefix, invoking the optional error callback between them. [load+0x75d6]
crt_print_error_prefix:
    push bp
    mov_load bp, sp
    mov ax,0xfc
    push ax
    call near crt_write_error_message
    cmp word [crt_error_callback],0x0
    jz short $ +6
    call word near [crt_error_callback]
    mov ax,0xff
    push ax
    call near crt_write_error_message
    mov_load sp, bp
    pop bp
    ret
    mov ax,0x2
    jmp near crt_fatal_error

; XOR the zero prefix, copyright notice and trailer; compare with CRT_NULL_GUARD_XOR. On mismatch, report runtime error 1 (null pointer assignment). [load+0x75fc]
crt_check_null_assignment:
    push si
    xor_load si, si
    mov cx,CRT_NULL_GUARD_SIZE
    xor_load ah, ah
    cld
    lodsb
    xor_load ah, al
    loop $ -3
    xor ah,CRT_NULL_GUARD_XOR
    jz short $ +15
    call near crt_print_error_prefix
    mov ax,0x1
    push ax
    call near crt_write_error_message
    mov ax,0x1
    pop si
    ret

; Parse the PSP command tail, including quotes/backslashes, into argc/argv; obtain the program name from the DOS environment. [load+0x761e]
crt_prepare_argv:
    pop word [crt_argv_saved_return]
    mov dx,0x2
    cmp [crt_dos_version],dl
    jz short $ +43
    mov es,word [crt_psp_segment]
    mov es,word [es:0x2c]
    mov word [crt_program_name_pointer+2],es
    xor_load ax, ax
    cwd
    mov cx,0x8000
    xor_load di, di
    repne scasb
    scasb
    jnz short $ -3
    inc di
    inc di
    mov [crt_program_name_pointer],di
    mov cx,0xffff
    repne scasb
    not cx
    mov_load dx, cx
    mov di,0x1
    mov si,0x81
    mov ds,word [crt_psp_segment]
    lodsb
    cmp al,0x20
    jz short $ -3
    cmp al,0x9
    jz short $ -7
    cmp al,0xd
    jz short $ +113
    or_load al, al
    jz short $ +109
    inc di
    dec si
    lodsb
    cmp al,0x20
    jz short $ -22
    cmp al,0x9
    jz short $ -26
    cmp al,0xd
    jz short $ +94
    or_load al, al
    jz short $ +90
    cmp al,0x22
    jz short $ +38
    cmp al,0x5c
    jz short $ +5
    inc dx
    jmp short $ -26
    xor_load cx, cx
    inc cx
    lodsb
    cmp al,0x5c
    jz short $ -4
    cmp al,0x22
    jz short $ +6
    add_load dx, cx
    jmp short $ -43
    mov_load ax, cx
    shr cx,1
    adc_load dx, cx
    test al,0x1
    jnz short $ -52
    jmp short $ +3
    dec si
    lodsb
    cmp al,0xd
    jz short $ +45
    or_load al, al
    jz short $ +41
    cmp al,0x22
    jz short $ -68
    cmp al,0x5c
    jz short $ +5
    inc dx
    jmp short $ -18
    xor_load cx, cx
    inc cx
    lodsb
    cmp al,0x5c
    jz short $ -4
    cmp al,0x22
    jz short $ +6
    add_load dx, cx
    jmp short $ -35
    mov_load ax, cx
    shr cx,1
    adc_load dx, cx
    test al,0x1
    jnz short $ -44
    jmp short $ -103
    push ss
    pop ds
    mov [crt_argc],di
    add_load dx, di
    inc di
    shl di,1
    add_load dx, di
    and dl,0xfe
    sub_load sp, dx
    mov_load ax, sp
    mov [crt_argv],ax
    mov_load bx, ax
    add_load di, bx
    push ss
    pop es
    mov [ss:bx],di
    inc bx
    inc bx
    lds si,word [crt_program_name_pointer]
    lodsb
    stosb
    or_load al, al
    jnz short $ -4
    mov si,0x81
    mov ds,word [ss:crt_psp_segment]
    jmp short $ +5
    xor_load ax, ax
    stosb
    lodsb
    cmp al,0x20
    jz short $ -3
    cmp al,0x9
    jz short $ -7
    cmp al,0xd
    jnz short $ +5
    jmp near $ +130
    or_load al, al
    jnz short $ +5
    jmp short $ +123

; Reviewed instruction range 7729h..7729h; internal routine roles remain partial. [load+0x7729]
; recovered_code_7729
    nop
    mov [ss:bx],di
    inc bx
    inc bx
    dec si
    lodsb
    cmp al,0x20
    jz short $ -35
    cmp al,0x9
    jz short $ -39
    cmp al,0xd
    jz short $ +100
    or_load al, al
    jz short $ +96
    cmp al,0x22
    jz short $ +41
    cmp al,0x5c
    jz short $ +5
    stosb
    jmp short $ -26
    xor_load cx, cx
    inc cx
    lodsb
    cmp al,0x5c
    jz short $ -4
    cmp al,0x22
    jz short $ +8
    mov al,0x5c
    rep stosb
    jmp short $ -45
    mov al,0x5c
    shr cx,1
    rep stosb
    jnc short $ +8
    mov al,0x22
    stosb
    jmp short $ -57
    dec si
    lodsb
    cmp al,0xd
    jz short $ +48
    or_load al, al
    jz short $ +44
    cmp al,0x22
    jz short $ -71
    cmp al,0x5c
    jz short $ +5
    stosb
    jmp short $ -18
    xor_load cx, cx
    inc cx
    lodsb
    cmp al,0x5c
    jz short $ -4
    cmp al,0x22
    jz short $ +8
    mov al,0x5c
    rep stosb
    jmp short $ -37
    mov al,0x5c
    shr cx,1
    rep stosb
    jnc short $ -104
    mov al,0x22
    stosb
    jmp short $ -49
    xor_load ax, ax
    stosb
    push ss
    pop ds
    mov word [bx],0x0
    jmp word near [crt_argv_saved_return]

; Build the near environment pointer list from the DOS environment block before argument setup. [load+0x77ac]
crt_prepare_environment:
    push bp
    mov_load bp, sp
    push bp
    mov ds,word [crt_psp_segment]
    xor_load cx, cx
    mov_load ax, cx
    mov_load bp, cx
    mov_load di, cx
    dec cx
    mov si,[0x2c]
    or_load si, si
    jz short $ +10
    mov es,si
    repne scasb
    inc bp
    scasb
    jnz short $ -4
    inc bp
    xchg ax,di
    inc ax
    and al,0xfe
    mov_load di, bp
    shl bp,1
    add_load ax, bp
    push ss
    pop ds
    push di
    mov di,0x9
    call near crt_reserve_near_bytes
    pop di
    mov_load cx, di
    mov_load di, bp
    add_load di, ax
    mov [crt_envp],bp
    push ds
    pop es
    mov ds,si
    xor_load si, si
    dec cx
    jcxz $ +21
    cmp word [si],0x433b
    jz short $ +7
    mov [bp+0x0],di
    inc bp
    inc bp
    lodsb
    stosb
    or_load al, al
    jnz short $ -4
    loop $ -17
    mov [bp+0x0],cx
    push ss
    pop ds
    pop bp
    mov_load sp, bp
    pop bp
    ret

; Stack: runtime error code (callee pops). Walk code/string records; return message pointer in AX. [load+0x7812]
crt_lookup_error_message:
    push bp
    mov_load bp, sp
    push si
    push di
    push ds
    pop es
    mov dx,[bp+0x4]
    mov si,crt_error_message_table
    lodsw
    cmp_load ax, dx
    jz short $ +18
    inc ax
    xchg ax,si
    jz short $ +14
    xchg ax,di
    xor_load ax, ax
    mov cx,0xffff
    repne scasb
    mov_load si, di
    jmp short $ -19
    xchg ax,si
    pop di
    pop si
    mov_load sp, bp
    pop bp
    ret word 0x2

; Stack: runtime error code (callee pops). Look up text and write it to DOS stderr handle 2. [load+0x783d]
crt_write_error_message:
    push bp
    mov_load bp, sp
    push di
    push word [bp+0x4]
    call near crt_lookup_error_message
    or_load ax, ax
    jz short $ +22
    xchg ax,dx
    mov_load di, dx
    xor_load ax, ax
    mov cx,0xffff
    repne scasb
    not cx
    dec cx
    mov bx,0x2
    mov ah,0x40
    int byte DOS_SERVICES
    pop di
    mov_load sp, bp
    pop bp
    ret word 0x2

; AX: requested bytes. Advance the near break, growing the DOS block if necessary; return previous break in BP and requested size in DX. [load+0x7866]
crt_reserve_near_bytes:
    mov_load dx, ax
    add ax,[crt_near_break]
    jc short $ +55
    cmp [crt_near_limit],ax
    jnc short $ +39
    add ax,strict word 0xf
    push ax
    rcr ax,1
    mov cl,0x3
    shr ax,cl
    mov cx,ds
    mov bx,[crt_psp_segment]
    sub_load cx, bx
    add_load ax, cx
    mov es,bx
    mov_load bx, ax
    mov ah,DOS_RESIZE_BLOCK
    int byte DOS_SERVICES
    pop ax
    jc short $ +18
    and al,0xf0
    dec ax
    mov [crt_near_limit],ax
    xchg ax,bp
    mov bp,[crt_near_break]
    add [crt_near_break],dx
    ret
    mov_load ax, di
    jmp near crt_fatal_error

; Linker padding before the program data segment.
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; g data +0x78a8

; crt_null_guard_zero_prefix: Eight zero bytes at the start of DS. The CRT null-assignment check reads them before the 56-byte copyright notice and its two-byte trailer; XOR over the full 42h-byte region must be 55h. The mixed-input recording captures each prefix byte once at shutdown. These bytes belong to the data guard, while the preceding eight bytes remain code-segment padding. [load+0x78b0]
    times 8 db 0

; crt_copyright: game text; original control bytes preserved [load+0x78b8]
    db 'MS Run-Time Library - Copyright (c) 1987, Microsoft Corp'

; crt_null_guard_notice_trailer: The notice ends in 1Eh,00h; both bytes participate in the null-assignment checksum. Native guard returns success with XOR 55h.  [load+0x78f0]
    db 0x1e, 0 ; copyright notice trailer; included in CRT null-assignment guard

; speaker_pitch_divisors: 48 PIT channel-2 divisors; pitch wraps at 48 [load+0x78f2]
    dw 0x3550, 0x348c, 0x33cb, 0x330d, 0x3252, 0x3199, 0x30e3, 0x3030
    dw 0x2f7f, 0x2ed1, 0x2e25, 0x2d7b, 0x2cd5, 0x2c30, 0x2b8e, 0x2aee
    dw 0x2a50, 0x29b5, 0x291c, 0x2885, 0x27f0, 0x275e, 0x26cd, 0x263f
    dw 0x25b3, 0x2528, 0x24a0, 0x2419, 0x2395, 0x2312, 0x2292, 0x2213
    dw 0x2196, 0x211a, 0x20a1, 0x2029, 0x1fb3, 0x1f3f, 0x1ecc, 0x1e5b
    dw 0x1dec, 0x1d7e, 0x1d12, 0x1ca7, 0x1c3e, 0x1bd6, 0x1b70, 0x1b0b

; sound_effect_sentry_rotation: Triggered after Sentinel/sentry heading updates at 449Dh; effect zero is requested by XOR AX,AX. Noise-mode rotation feedback. [load+0x7952]

; Effect 0: numeric identity retained.
    db SOUND_ENABLED | SOUND_VOICE_0 | SOUND_NOISE ; voice, noise mode, octave, enabled
    db 0 ; initial envelope level, clamped to 0..15 by updater
    db 191, 1 ; initial pitch/noise parameter, envelope tick period
    db 2, 7 ; first level step and step count
    db -2, 7 ; second level step and step count
    db 0, 0 ; pitch phase 1: signed step, count
    db SOUND_PITCH_MOD_DISABLED ; pitch update period; negative disables pitch modulation
    db 1 ; envelope tick period after phase transition
    db 0, 0 ; pitch phase 2: signed step, count
    db 0, 0 ; pitch phase 3: signed step, count

; sound_effect_meanie_rotation: Triggered after the Meanie heading update at 4392h. Tone-mode rotation feedback, distinct from sentry rotation. [load+0x7962]

; Effect 1: numeric identity retained.
    db SOUND_ENABLED | SOUND_VOICE_0 | 1*SOUND_OCTAVE_DOWN ; voice, noise mode, octave, enabled
    db 0 ; initial envelope level, clamped to 0..15 by updater
    db 0, 1 ; initial pitch/noise parameter, envelope tick period
    db 1, 15 ; first level step and step count
    db -2, 7 ; second level step and step count
    db 0, 16 ; pitch phase 1: signed step, count
    db 1 ; pitch update period; negative disables pitch modulation
    db 1 ; envelope tick period after phase transition
    db 0, 0 ; pitch phase 2: signed step, count
    db 0, 0 ; pitch phase 3: signed step, count

; sound_effect_object_action_success: Triggered by player_poll_actions after a successful create/absorb operation, before the selected-view redraw. [load+0x7972]

; Effect 2: numeric identity retained.
    db SOUND_ENABLED | SOUND_VOICE_0 | SOUND_NOISE ; voice, noise mode, octave, enabled
    db 0 ; initial envelope level, clamped to 0..15 by updater
    db 255, 4 ; initial pitch/noise parameter, envelope tick period
    db 1, 15 ; first level step and step count
    db 0, 255 ; second level step and step count
    db 0, 0 ; pitch phase 1: signed step, count
    db SOUND_PITCH_MOD_DISABLED ; pitch update period; negative disables pitch modulation
    db 255 ; envelope tick period after phase transition
    db 0, 0 ; pitch phase 2: signed step, count
    db 0, 0 ; pitch phase 3: signed step, count

; sound_effect_melody_note: Restarted for each melody note by sound_advance_melody. The sequencer patches its pitch and flags before copying it to a voice. [load+0x7982]

; Effect 3: melody voice; flags/pitch are patched before each note.
    db SOUND_ENABLED | SOUND_VOICE_0 ; voice, noise mode, octave, enabled
    db 0 ; initial envelope level, clamped to 0..15 by updater
    db 34, 1 ; initial pitch/noise parameter, envelope tick period
    db 15, 1 ; first level step and step count
    db -1, 4 ; second level step and step count
    db 1, 1 ; pitch phase 1: signed step, count
    db 1 ; pitch update period; negative disables pitch modulation
    db 9 ; envelope tick period after phase transition
    db -1, 1 ; pitch phase 2: signed step, count
    db 0, 8 ; pitch phase 3: signed step, count

; sound_effect_background_pulse: Scheduled every 36h trigger ticks in the default dispatcher mode. Active sentry warning selects mode 4, suppressing this automatic trigger and silencing voice 1. [load+0x7992]

; Effect 4: numeric identity retained.
    db SOUND_ENABLED | SOUND_VOICE_1 ; voice, noise mode, octave, enabled
    db 4 ; initial envelope level, clamped to 0..15 by updater
    db 34, 5 ; initial pitch/noise parameter, envelope tick period
    db 1, 6 ; first level step and step count
    db -1, 6 ; second level step and step count
    db 6, 2 ; pitch phase 1: signed step, count
    db 1 ; pitch update period; negative disables pitch modulation
    db 5 ; envelope tick period after phase transition
    db -6, 2 ; pitch phase 2: signed step, count
    db 0, 0 ; pitch phase 3: signed step, count

; sound_effect_feedback: Used both for player energy loss and rejected actions. The rejection path temporarily changes its pitch byte from 90h to AAh and restores it after starting the sound. [load+0x79a2]

; Effect 5: numeric identity retained.
    db SOUND_ENABLED | SOUND_VOICE_2 ; voice, noise mode, octave, enabled
    db 0 ; initial envelope level, clamped to 0..15 by updater
    db 144, 1 ; initial pitch/noise parameter, envelope tick period
    db 15, 1 ; first level step and step count
    db -1, 15 ; second level step and step count
    db 1, 2 ; pitch phase 1: signed step, count
    db 1 ; pitch update period; negative disables pitch modulation
    db 1 ; envelope tick period after phase transition
    db -1, 1 ; pitch phase 2: signed step, count
    db 0, 7 ; pitch phase 3: signed step, count

; sound_effect_transition_noise: Triggered by sound sequence mode 6. The dispatcher varies its noise parameter with sound_ramp_index and chooses a random 1..4 tick retrigger delay. [load+0x79b2]

; Effect 6: numeric identity retained.
    db SOUND_ENABLED | SOUND_VOICE_0 | SOUND_NOISE ; voice, noise mode, octave, enabled
    db 0 ; initial envelope level, clamped to 0..15 by updater
    db 0, 1 ; initial pitch/noise parameter, envelope tick period
    db 15, 1 ; first level step and step count
    db -15, 1 ; second level step and step count
    db 0, 0 ; pitch phase 1: signed step, count
    db SOUND_PITCH_MOD_DISABLED ; pitch update period; negative disables pitch modulation
    db 1 ; envelope tick period after phase transition
    db 0, 0 ; pitch phase 2: signed step, count
    db 0, 0 ; pitch phase 3: signed step, count

; sound_voice_action_and_melody: Voice 0 receives the rotation, action-success, transition-noise and normal melody definitions. It is a software voice sharing PIT channel 2, not a separate hardware audio channel. Reserved-byte uses remain unconfirmed. [load+0x79c2]
    times SOUND_EFFECT_BYTES db 0 ; copied effect parameters
    db 0, 0 ; current pitch step, remaining phase steps
    times 2 db 0 ; 12h..13h: no reviewed engine consumer
    db 0, 0, 0 ; envelope countdown, pitch countdown, phase selector
    times SOUND_VOICE_BYTES-23 db 0 ; 17h..1Fh: uses unconfirmed

; sound_voice_background: Voice 1 receives the periodic background-pulse definition. Sentry warning activation suppresses that trigger and explicitly silences this voice. Reserved-byte uses remain unconfirmed. [load+0x79e2]
    times SOUND_EFFECT_BYTES db 0 ; copied effect parameters
    db 0, 0 ; current pitch step, remaining phase steps
    times 2 db 0 ; 12h..13h: no reviewed engine consumer
    db 0, 0, 0 ; envelope countdown, pitch countdown, phase selector
    times SOUND_VOICE_BYTES-23 db 0 ; 17h..1Fh: uses unconfirmed

; sound_voice_feedback: Voice 2 receives energy-loss/rejected-action feedback. The sound updater starts its descending voice traversal at this slot. Reserved-byte uses remain unconfirmed. [load+0x7a02]
    times SOUND_EFFECT_BYTES db 0 ; copied effect parameters
    db 0, 0 ; current pitch step, remaining phase steps
    times 2 db 0 ; 12h..13h: no reviewed engine consumer
    db 0, 0, 0 ; envelope countdown, pitch countdown, phase selector
    times SOUND_VOICE_BYTES-23 db 0 ; 17h..1Fh: uses unconfirmed

; intro_five_note_theme: startup melody: five phrases, 15 effect-3 note triggers; G2/D3, Bb2/F3, C3/G3, F2/C3, G2/D3 [load+0x7a22]
    melody_duration 0
    melody_note NOTE_G2
    melody_note NOTE_D3
    melody_duration 28
    melody_note NOTE_D3
    melody_duration 0
    melody_note NOTE_Bb2
    melody_note NOTE_F3
    melody_duration 28
    melody_note NOTE_F3
    melody_duration 0
    melody_note NOTE_C3
    melody_note NOTE_G3
    melody_duration 28
    melody_note NOTE_G3
    melody_duration 0
    melody_note NOTE_F2
    melody_note NOTE_C3
    melody_duration 28
    melody_note NOTE_C3
    melody_duration 0
    melody_note NOTE_G2
    melody_note NOTE_D3
    melody_note NOTE_D3
    melody_end

; melody_robot_transfer: Transfer melody at start offset 25: successful robot transfer reaches CS:301B/301D. Turn-around uses the interior offset 40 at CS:2FCB, starting at the fourth phrase. The full stream has 15 triggers and a 123-tick hold budget, with A2/E3 in phrase three; the offset-40 suffix has six triggers and a 33-tick budget. Both entries are verified by the native sequencer probe. [load+0x7a3b]
    melody_duration 0
    melody_note NOTE_G2
    melody_note NOTE_D3
    melody_duration 28
    melody_note NOTE_D3
    melody_duration 0
    melody_note NOTE_Bb2
    melody_note NOTE_F3
    melody_duration 28
    melody_note NOTE_F3
    melody_duration 0
    melody_note NOTE_A2
    melody_note NOTE_E3
    melody_duration 28
    melody_note NOTE_E3
    melody_duration 0
    melody_note NOTE_F2
    melody_note NOTE_C3
    melody_duration 28
    melody_note NOTE_C3
    melody_duration 0
    melody_note NOTE_G2
    melody_note NOTE_D3
    melody_note NOTE_D3
    melody_end

; melody_end_effect: End/hyperspace-effect melody at start offset 50, selected at CS:1A55/1A57 by the effect sequence at CS:1A17. That helper is called from the end-flag path at CS:3F2B and hyperspace-outcome handling near CS:3A6B. Eleven triggers hold for 212 ticks in total. Native playback is verified; this is an effect-sequence association rather than a distinct successful camera-turn theme. [load+0x7a54]
    melody_duration 16
    melody_note NOTE_G2
    melody_note NOTE_D3
    melody_note NOTE_A3
    melody_note NOTE_Bb3
    melody_duration 60
    melody_note NOTE_D4
    melody_duration 8
    melody_note NOTE_G4
    melody_note NOTE_D4
    melody_note NOTE_Bb3
    melody_note NOTE_A3
    melody_note NOTE_D3
    melody_duration 48
    melody_note NOTE_G2
    melody_end

; melody_landscape_completion: Landscape-completion melody at start offset 66, selected at CS:3AA9/3AAB after destination/code generation and revealing the completion screen. Eighteen triggers form six phrases, holding for 260 ticks; phrase-ending durations are 48,20,24,28,36,8. The final D3 inherits 8. The controller waits for the melody cursor to become negative before the final key gate. [load+0x7a64]
    melody_duration 8
    melody_note NOTE_G2
    melody_note NOTE_D3
    melody_duration 48
    melody_note NOTE_D3
    melody_duration 8
    melody_note NOTE_F2
    melody_note NOTE_C3
    melody_duration 20
    melody_note NOTE_C3
    melody_duration 8
    melody_note NOTE_C3
    melody_note NOTE_G3
    melody_duration 24
    melody_note NOTE_G3
    melody_duration 8
    melody_note NOTE_Bb2
    melody_note NOTE_F3
    melody_duration 28
    melody_note NOTE_F3
    melody_duration 8
    melody_note NOTE_A2
    melody_note NOTE_C3
    melody_duration 36
    melody_note NOTE_C3
    melody_duration 8
    melody_note NOTE_G2
    melody_note NOTE_D3
    melody_note NOTE_D3
    melody_end

; sound_muted: Global mute latch: zero permits sound, FFh suppresses speaker updates. F2 handlers complement it; prompt helpers save, replace, and restore it. [load+0x7a82]
    db 0x00

; sound_mute_saved: Restore latch for temporary mute overrides: digit-entry beeps and pause helpers save sound_muted here and restore it afterward. F2 handlers also mirror the newly toggled mute value here. [load+0x7a83]
    db 0x00

; raster_x_word_offset: Horizontal raster offset in four-pixel units (CGA bytes): CGA adds it directly, Tandy doubles it, VGA quadruples it, and EGA halves it. The paired-view render loop selects zero or 40. [load+0x7a84]
    dw 0x0000

; raster_color_index_a: Raster workspace: first packed-color selector. CGA stores twice the low logical-color nibble as a word-table offset; Tandy stores the unscaled palette index. [load+0x7a86]
    dw 0x0000

; raster_color_index_b: Raster workspace: second packed-color selector, normally the high color nibble; CGA scales it by two and Tandy leaves it unscaled. Model dithering can select the first color for both. [load+0x7a88]
    dw 0x0000

; raster_shade_bank: VGA fog bank = (shade_input<<3)&F0h, so each pair of adjacent inputs shares a bank and input 32 wraps to bank zero. Mode 1 maps distances 16..78 monotonically onto banks 0..15. VGA span filling ORs the bank into the pixel high nibble; low-nibble object colors remain intact. [load+0x7a8a]
    db 0x00
    db 0x00 ; g data +0x7a8b

; cga_left_pixel_masks: CGA span filler: retain the selected 2-bit pixel and all pixels to its right in the first partial byte [load+0x7a8c]
    db 0xff >> 0 ; first covered pixel 0 from left
    db 0xff >> 2 ; first covered pixel 1 from left
    db 0xff >> 4 ; first covered pixel 2 from left
    db 0xff >> 6 ; first covered pixel 3 from left

; cga_right_pixel_masks: CGA span filler: retain pixels up to and including the selected pixel in the last partial byte [load+0x7a90]
    db (0xff << 6) & 0xff ; last covered pixel 0 from left
    db (0xff << 4) & 0xff ; last covered pixel 1 from left
    db (0xff << 2) & 0xff ; last covered pixel 2 from left
    db (0xff << 0) & 0xff ; last covered pixel 3 from left

; cga_dither_patterns_a: 16 logical colors, two scanline-parity patterns each; indexed by 2*color plus bank parity at 037Dh; every row is four 2-bit CGA pixels [load+0x7a94]
    db (0 << 6) | (0 << 4) | (0 << 2) | 0 ; logical color 0h, row parity 0, pixels 0000
    db (0 << 6) | (0 << 4) | (0 << 2) | 0 ; logical color 0h, row parity 1, pixels 0000
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color 1h, row parity 0, pixels 1111
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color 1h, row parity 1, pixels 1111
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 2h, row parity 0, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 2h, row parity 1, pixels 2222
    db (3 << 6) | (3 << 4) | (3 << 2) | 3 ; logical color 3h, row parity 0, pixels 3333
    db (3 << 6) | (3 << 4) | (3 << 2) | 3 ; logical color 3h, row parity 1, pixels 3333
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color 4h, row parity 0, pixels 1111
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color 4h, row parity 1, pixels 1111
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 5h, row parity 0, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 5h, row parity 1, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 6h, row parity 0, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 6h, row parity 1, pixels 2222
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color 7h, row parity 0, pixels 1111
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color 7h, row parity 1, pixels 1111
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 8h, row parity 0, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 8h, row parity 1, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 9h, row parity 0, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 9h, row parity 1, pixels 2222
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color Ah, row parity 0, pixels 1111
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color Ah, row parity 1, pixels 1111
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color Bh, row parity 0, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color Bh, row parity 1, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color Ch, row parity 0, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color Ch, row parity 1, pixels 2222
    db (0 << 6) | (0 << 4) | (0 << 2) | 0 ; logical color Dh, row parity 0, pixels 0000
    db (0 << 6) | (0 << 4) | (0 << 2) | 0 ; logical color Dh, row parity 1, pixels 0000
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color Eh, row parity 0, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color Eh, row parity 1, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color Fh, row parity 0, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color Fh, row parity 1, pixels 2222

; cga_dither_patterns_b: 16 logical colors, two scanline-parity patterns each; indexed by 2*color plus bank parity at 037Dh; every row is four 2-bit CGA pixels [load+0x7ab4]
    db (0 << 6) | (0 << 4) | (0 << 2) | 0 ; logical color 0h, row parity 0, pixels 0000
    db (0 << 6) | (0 << 4) | (0 << 2) | 0 ; logical color 0h, row parity 1, pixels 0000
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color 1h, row parity 0, pixels 1111
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color 1h, row parity 1, pixels 1111
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 2h, row parity 0, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 2h, row parity 1, pixels 2222
    db (3 << 6) | (3 << 4) | (3 << 2) | 3 ; logical color 3h, row parity 0, pixels 3333
    db (3 << 6) | (3 << 4) | (3 << 2) | 3 ; logical color 3h, row parity 1, pixels 3333
    db (0 << 6) | (1 << 4) | (0 << 2) | 1 ; logical color 4h, row parity 0, pixels 0101
    db (1 << 6) | (0 << 4) | (1 << 2) | 0 ; logical color 4h, row parity 1, pixels 1010
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color 5h, row parity 0, pixels 1111
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color 5h, row parity 1, pixels 1111
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 6h, row parity 0, pixels 2222
    db (2 << 6) | (2 << 4) | (2 << 2) | 2 ; logical color 6h, row parity 1, pixels 2222
    db (2 << 6) | (3 << 4) | (2 << 2) | 3 ; logical color 7h, row parity 0, pixels 2323
    db (3 << 6) | (2 << 4) | (3 << 2) | 2 ; logical color 7h, row parity 1, pixels 3232
    db (1 << 6) | (3 << 4) | (1 << 2) | 3 ; logical color 8h, row parity 0, pixels 1313
    db (3 << 6) | (1 << 4) | (3 << 2) | 1 ; logical color 8h, row parity 1, pixels 3131
    db (3 << 6) | (3 << 4) | (3 << 2) | 3 ; logical color 9h, row parity 0, pixels 3333
    db (3 << 6) | (3 << 4) | (3 << 2) | 3 ; logical color 9h, row parity 1, pixels 3333
    db (1 << 6) | (2 << 4) | (1 << 2) | 2 ; logical color Ah, row parity 0, pixels 1212
    db (1 << 6) | (2 << 4) | (1 << 2) | 2 ; logical color Ah, row parity 1, pixels 1212
    db (3 << 6) | (3 << 4) | (3 << 2) | 3 ; logical color Bh, row parity 0, pixels 3333
    db (3 << 6) | (3 << 4) | (3 << 2) | 3 ; logical color Bh, row parity 1, pixels 3333
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color Ch, row parity 0, pixels 1111
    db (1 << 6) | (1 << 4) | (1 << 2) | 1 ; logical color Ch, row parity 1, pixels 1111
    db (3 << 6) | (3 << 4) | (3 << 2) | 3 ; logical color Dh, row parity 0, pixels 3333
    db (3 << 6) | (3 << 4) | (3 << 2) | 3 ; logical color Dh, row parity 1, pixels 3333
    db (1 << 6) | (2 << 4) | (1 << 2) | 2 ; logical color Eh, row parity 0, pixels 1212
    db (2 << 6) | (1 << 4) | (2 << 2) | 1 ; logical color Eh, row parity 1, pixels 2121
    db (0 << 6) | (2 << 4) | (0 << 2) | 2 ; logical color Fh, row parity 0, pixels 0202
    db (2 << 6) | (0 << 4) | (2 << 2) | 0 ; logical color Fh, row parity 1, pixels 2020

; ega_span_mask_a: EGA span-edge masks: single boundary pixel, bit 7 leftmost; written to EGA GC bit-mask register 8 [load+0x7ad4]
    db 0x80 >> 0 ; boundary pixel 0 from left
    db 0x80 >> 1 ; boundary pixel 1 from left
    db 0x80 >> 2 ; boundary pixel 2 from left
    db 0x80 >> 3 ; boundary pixel 3 from left
    db 0x80 >> 4 ; boundary pixel 4 from left
    db 0x80 >> 5 ; boundary pixel 5 from left
    db 0x80 >> 6 ; boundary pixel 6 from left
    db 0x80 >> 7 ; boundary pixel 7 from left

; ega_span_mask_b: EGA span-edge masks: pixels strictly to the right of a boundary pixel [load+0x7adc]
    db 0x7f >> 0 ; boundary pixel 0 from left
    db 0x7f >> 1 ; boundary pixel 1 from left
    db 0x7f >> 2 ; boundary pixel 2 from left
    db 0x7f >> 3 ; boundary pixel 3 from left
    db 0x7f >> 4 ; boundary pixel 4 from left
    db 0x7f >> 5 ; boundary pixel 5 from left
    db 0x7f >> 6 ; boundary pixel 6 from left
    db 0x7f >> 7 ; boundary pixel 7 from left

; ega_span_mask_c: EGA span-edge masks: pixels strictly to the left of a boundary pixel [load+0x7ae4]
    db (~(0xff >> 0)) & 0xff ; boundary pixel 0 from left
    db (~(0xff >> 1)) & 0xff ; boundary pixel 1 from left
    db (~(0xff >> 2)) & 0xff ; boundary pixel 2 from left
    db (~(0xff >> 3)) & 0xff ; boundary pixel 3 from left
    db (~(0xff >> 4)) & 0xff ; boundary pixel 4 from left
    db (~(0xff >> 5)) & 0xff ; boundary pixel 5 from left
    db (~(0xff >> 6)) & 0xff ; boundary pixel 6 from left
    db (~(0xff >> 7)) & 0xff ; boundary pixel 7 from left

; tandy_edge_pixel_masks: Tandy span filler, indexed by pixel parity 0/1: first-byte primary-color masks: selected nibble and pixels to its right [load+0x7aec]
    db (0xf << 4) | 0xf ; edge pixel 0: high/low nibble masks
    db (0x0 << 4) | 0xf ; edge pixel 1: high/low nibble masks

; tandy_right_pixel_masks: Tandy span filler, indexed by pixel parity 0/1: last-byte primary-color masks: selected nibble and pixels to its left [load+0x7aee]
    db (0xf << 4) | 0x0 ; edge pixel 0: high/low nibble masks
    db (0xf << 4) | 0xf ; edge pixel 1: high/low nibble masks

; tandy_left_secondary_masks: Tandy span filler, indexed by pixel parity 0/1: first-byte secondary dither-color contributions [load+0x7af0]
    db (0x0 << 4) | 0xf ; edge pixel 0: high/low nibble masks
    db (0x0 << 4) | 0x0 ; edge pixel 1: high/low nibble masks

; tandy_right_secondary_masks: Tandy span filler, indexed by pixel parity 0/1: last-byte secondary dither-color contributions [load+0x7af2]
    db (0x0 << 4) | 0x0 ; edge pixel 0: high/low nibble masks
    db (0xf << 4) | 0x0 ; edge pixel 1: high/low nibble masks

; ega_polygon_busy: EGA graphics-controller exclusion latch: span/background writers set FFh while changing GC state and clear it on completion; scattered-pixel and sentry-warning callbacks skip drawing while it is set. [load+0x7af4]
    db 0x00

; resource_cga_color_00: Planar resource color 0: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7af5]
    dw 0x0000 ; even source rows: CGA pixel indices 0,0,0,0,0,0,0,0
    dw 0x0000 ; odd source rows: CGA pixel indices 0,0,0,0,0,0,0,0

; resource_cga_color_01: Planar resource color 1: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7af9]
    dw 0x5555 ; even source rows: CGA pixel indices 1,1,1,1,1,1,1,1
    dw 0x5555 ; odd source rows: CGA pixel indices 1,1,1,1,1,1,1,1

; resource_cga_color_02: Planar resource color 2: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7afd]
    dw 0xffff ; even source rows: CGA pixel indices 3,3,3,3,3,3,3,3
    dw 0xffff ; odd source rows: CGA pixel indices 3,3,3,3,3,3,3,3

; resource_cga_color_03: Planar resource color 3: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b01]
    dw 0x5555 ; even source rows: CGA pixel indices 1,1,1,1,1,1,1,1
    dw 0x5555 ; odd source rows: CGA pixel indices 1,1,1,1,1,1,1,1

; resource_cga_color_04: Planar resource color 4: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b05]
    dw 0xaaaa ; even source rows: CGA pixel indices 2,2,2,2,2,2,2,2
    dw 0xaaaa ; odd source rows: CGA pixel indices 2,2,2,2,2,2,2,2

; resource_cga_color_05: Planar resource color 5: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b09]
    dw 0x5555 ; even source rows: CGA pixel indices 1,1,1,1,1,1,1,1
    dw 0x5555 ; odd source rows: CGA pixel indices 1,1,1,1,1,1,1,1

; resource_cga_color_06: Planar resource color 6: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b0d]
    dw 0xaaaa ; even source rows: CGA pixel indices 2,2,2,2,2,2,2,2
    dw 0xaaaa ; odd source rows: CGA pixel indices 2,2,2,2,2,2,2,2

; resource_cga_color_07: Planar resource color 7: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b11]
    dw 0x8888 ; even source rows: CGA pixel indices 2,0,2,0,2,0,2,0
    dw 0x2222 ; odd source rows: CGA pixel indices 0,2,0,2,0,2,0,2

; resource_cga_color_08: Planar resource color 8: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b15]
    dw 0x7777 ; even source rows: CGA pixel indices 1,3,1,3,1,3,1,3
    dw 0xdddd ; odd source rows: CGA pixel indices 3,1,3,1,3,1,3,1

; resource_cga_color_09: Planar resource color 9: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b19]
    dw 0xbbbb ; even source rows: CGA pixel indices 2,3,2,3,2,3,2,3
    dw 0xeeee ; odd source rows: CGA pixel indices 3,2,3,2,3,2,3,2

; resource_cga_color_0a: Planar resource color 10: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b1d]
    dw 0x0000 ; even source rows: CGA pixel indices 0,0,0,0,0,0,0,0
    dw 0x0000 ; odd source rows: CGA pixel indices 0,0,0,0,0,0,0,0

; resource_cga_color_0b: Planar resource color 11: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b21]
    dw 0x0000 ; even source rows: CGA pixel indices 0,0,0,0,0,0,0,0
    dw 0x0000 ; odd source rows: CGA pixel indices 0,0,0,0,0,0,0,0

; resource_cga_color_0c: Planar resource color 12: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b25]
    dw 0xffff ; even source rows: CGA pixel indices 3,3,3,3,3,3,3,3
    dw 0x0000 ; odd source rows: CGA pixel indices 0,0,0,0,0,0,0,0

; resource_cga_color_0d: Planar resource color 13: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b29]
    dw 0x0000 ; even source rows: CGA pixel indices 0,0,0,0,0,0,0,0
    dw 0xffff ; odd source rows: CGA pixel indices 3,3,3,3,3,3,3,3

; resource_cga_color_0e: Planar resource color 14: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b2d]
    dw 0x5555 ; even source rows: CGA pixel indices 1,1,1,1,1,1,1,1
    dw 0x5555 ; odd source rows: CGA pixel indices 1,1,1,1,1,1,1,1

; resource_cga_color_0f: Planar resource color 15: two word-sized CGA pixel patterns, even source rows then odd rows. The loader indexes color*4 + row_parity*2, masks one two-bit output position and swaps bytes before writing. Palette indices, not fixed RGB colors. [load+0x7b31]
    dw 0xffff ; even source rows: CGA pixel indices 3,3,3,3,3,3,3,3
    dw 0xffff ; odd source rows: CGA pixel indices 3,3,3,3,3,3,3,3

; video_scanline_offsets: 201 scanline-address words. Native builders use CGA=(y&1)*8192+(y>>1)*80, Tandy=(y&3)*8192+(y>>2)*160, EGA=y*40, VGA=y*320. CS:0A8B/0AA0 write only y=0..199; CS:0AB9/0AD1 include y=200. The extra word is not a 201st visible row. Consumers index by 2*y and add their backend-specific X byte offset. [load+0x7b35]
; Runtime offsets: CGA=(y&1)*2000h+(y>>1)*80; Tandy=(y&3)*2000h+(y>>2)*160.
; EGA=y*40; VGA=y*320. These are byte offsets within the selected target buffer.
    times VIDEO_SCANLINE_COUNT dw 0

; video_target_base: Draw-target tagged union selected by the installed backend: CGA/Tandy/VGA hold a segment; EGA holds 0000h/8000h byte offsets within A000h. Front-buffer selectors CS:0C00..0C64 also clear video_page_offset and program CRTC start zero. Work selectors CS:0C86/0C8D change only the drawing target, preserving the currently displayed page origin. [load+0x7cc7]
    dw 0x0000

; cga_background_pattern_a: Scene background patterns selected by scene_background_style: four styles of two alternating scanline words; pattern B is the +2 alias in each four-byte record [load+0x7cc9]
    dw 0xeeee, 0xbbbb ; style 0: even/odd row patterns
    dw 0xeeee, 0xbbbb ; style 1: even/odd row patterns
    dw 0x0000, 0x0000 ; style 2: even/odd row patterns
    dw 0x0000, 0x0000 ; style 3: even/odd row patterns

; tandy_background_patterns: Scene background patterns selected by scene_background_style: four styles of packed 4-bit pixel words [load+0x7cd9]
    db 0x77, 0x77 ; style 0: four packed pixels
    db 0x77, 0x77 ; style 1: four packed pixels
    db 0x00, 0x00 ; style 2: four packed pixels
    db 0x00, 0x00 ; style 3: four packed pixels

; vga_background_patterns: Scene background patterns selected by scene_background_style: four styles of two linear 8-bit pixel indices [load+0x7ce1]
    db 7, 7 ; style 0: two palette indices
    db 7, 7 ; style 1: two palette indices
    db 0, 0 ; style 2: two palette indices
    db 0, 0 ; style 3: two palette indices

; ega_background_patterns: Scene background patterns selected by scene_background_style: four styles of planar fill-color selectors [load+0x7ce9]
    db 7 ; style 0: planar color selector
    db 7 ; style 1: planar color selector
    db 0 ; style 2: planar color selector
    db 0 ; style 3: planar color selector
    db 0x00 ; g data +0x7ced

; edge_span_buffer: Edge-rasterizer workspace: near write pointer into the selected span-boundary buffer [load+0x7cee]
    dw 0x0000

; edge_delta_y: Edge-rasterizer workspace: signed edge Y difference; the high-byte alias is used during clipping [load+0x7cf0]
    dw 0x0000

; edge_abs_delta_x: Edge-rasterizer workspace: absolute X difference used for interpolation and major-axis choice [load+0x7cf2]
    dw 0x0000

; projection_x_bias: Polygon rasterizer X adjustment added before arithmetic halving into polygon_projected_x_words [load+0x7cf4]
    dw 0x0000

; edge_start_y: Edge-rasterizer workspace: edge start Y word assembled from the split projection cache [load+0x7cf6]
    dw 0x0000

; edge_start_x: Edge-rasterizer workspace: edge start X word in polygon raster coordinates [load+0x7cf8]
    dw 0x0000

; edge_end_y: Edge-rasterizer workspace: edge end Y word; its low byte is reused as raster_row_cursor during span filling [load+0x7cfa]
    dw 0x0000

; edge_end_x: Edge-rasterizer workspace: edge end X word in polygon raster coordinates [load+0x7cfc]
    dw 0x0000

; terrain_azimuth_bias: Terrain projection bias: setup derives a quadrant-relative angular word from view rotation; terrain_project_corner subtracts it from projection_angle before caching X. [load+0x7cfe]
    dw 0x0000
    db 0x00, 0x00, 0x00, 0x00 ; g data +0x7d00

; occlusion_ray_step_x: Visibility-ray workspace: target-X minus ray-origin X, normalized by left shifts at 2E66h; its signed high byte advances the split X accumulator at 2EAFh. [load+0x7d04]
    dw 0x0000

; occlusion_ray_step_height: Visibility-ray workspace: target-height minus origin height, normalized with the X/Y steps; both bytes advance the split height accumulator with carry. [load+0x7d06]
    dw 0x0000

; occlusion_ray_step_y: Visibility-ray workspace: target-Y minus ray-origin Y, normalized with the X/height steps; its signed high byte advances the split Y accumulator. [load+0x7d08]
    dw 0x0000

; polygon_horizontal_x_min: Horizontal-edge extent minimum, initialized to FFFFh and updated at 21D5h. When every edge is horizontal and the row is in the viewport, the polygon path emits one span with this left boundary. [load+0x7d0a]
    dw 0x0000

; polygon_horizontal_x_max: Horizontal-edge extent maximum, initialized to zero and updated at 21D5h; paired with the minimum to emit the single-row polygon span when all edges are horizontal. [load+0x7d0c]
    dw 0x0000

; ray_horizontal_scale: Signed horizontal magnitude derived from the pitch direction table by the ray setup at 3118h. The heading helper multiplies it by the heading components to produce X/Y ray steps; height stepping uses the separately stored pitch component. [load+0x7d0e]
    dw 0x0000

; viewport_x_min: Viewport state: left pixel clip coordinate [load+0x7d10]
    dw 0x0000

; viewport_x_end: Viewport state: right clip coordinate, computed as origin plus width [load+0x7d12]
    dw 0x0000

; edge_scaled_delta_x: Edge-rasterizer workspace: scaled X increment used when long edges are subdivided [load+0x7d14]
    dw 0x0000

; polygon_index_list: Near pointer to the current closed vertex-index loop. Mesh drawing switches it to a face loop and restores terrain_face_indices; polygon_project_index_loop and edge traversal dereference it through BP. [load+0x7d16]
    dw 0x0000

; ray_heading_angle: Shared direction-angle word: cursor aiming combines horizontal cursor displacement with object heading here; direction-vector helpers and camera offset setup consume the low fractional/high angular bytes. [load+0x7d18]
    dw 0x0000

; ray_pitch_angle: Shared direction-angle word: cursor aiming combines vertical cursor displacement with object pitch; the common direction helper consumes its low fractional/high angular bytes. [load+0x7d1a]
    dw 0x0000
    db 0x00, 0x00 ; g data +0x7d1c

; model_relative_heading: Model heading minus bearing angle, in 65536 units per turn; vertex azimuths are added to its high byte [load+0x7d1e]
    dw 0x0000

; projection_minor_axis: Bearing/distance workspace: smaller absolute axis selected for first-octant ratio lookup and distance correction. [load+0x7d20]
    dw 0x0000
    db 0x00, 0x00 ; g data +0x7d22

; viewport_pixel_width: Unsigned raster width. Presets use full word values 320/112/128; CS:2901 instead shifts only AL three times and clears AH, yielding (8*column_count) modulo 256. Selected-view callers cap a render pass to 20 columns before this helper, keeping that width at or below 160 pixels. [load+0x7d24]
    dw 0x0000
    db 0x00, 0x00, 0x00, 0x00 ; g data +0x7d26

; math_encoded_factor: Shared multiplication workspace: magnitude word whose bit 0 encodes a sign toggle; math_multiply_encoded_signed combines it with projection_direction_flags and keeps the high product word. [load+0x7d2a]
    dw 0x0000

; math_signed_factor: Shared multiplication workspace: signed factor; math_multiply_encoded_signed takes its absolute value in place and folds its sign into projection_direction_flags. [load+0x7d2c]
    dw 0x0000

; occlusion_height_row_pointer: Visibility workspace: near pointer to a 256-byte-stride height row; initialized to row 31 for building and recomputed from the tested Y coordinate for ray visibility. [load+0x7d2e]
    dw 0x0000
    db 0x00, 0x00 ; g data +0x7d30

; projection_major_axis: Bearing/distance workspace: larger absolute axis; planar distance adds the ratio-based correction to this word. [load+0x7d32]
    dw 0x0000

; projection_distance: Shared projection workspace: planar distance result used by vertex and corner projection [load+0x7d34]
    dw 0x0000

; projection_abs_delta_x: Shared projection workspace: absolute X or vertical-angle input magnitude, depending on projection phase [load+0x7d36]
    dw 0x0000

; model_height_delta: Shared projection workspace: signed split-byte 8.8 height difference between drawn and viewing objects [load+0x7d38]
    dw 0x0000

; projection_abs_delta_y: Shared projection workspace: absolute Y magnitude used for bearing and distance lookup [load+0x7d3a]
    dw 0x0000

; projection_angle: Full-turn bearing word: 0000h points along positive Y, 4000h along positive X, 8000h along negative Y, C000h along negative X. CS:1CB4/1CE8 restore these quadrants after the first-octant lookup; differences wrap modulo 65536. [load+0x7d3c]
    dw 0x0000

; projection_screen_y: Signed vertical raster coordinate = arithmetic_shift_right_4(vertical_angle - ((camera_pitch<<8)|20h)), with word subtraction wrapping first (CS:1D12). One pixel is 16 angular units, or 360/4096 degrees; the low-byte 20h pitch bias contributes two pixels. [load+0x7d3e]
    dw 0x0000
    db 0x00, 0x00, 0x00, 0x00, 0x00 ; g data +0x7d40

; trig_source_angle: Saved angular byte in 1B3Fh. Bit 6 chooses the complementary-component placement; bit 7 supplies component A sign; XOR of bits 6 and 7 supplies component B sign. Sign bits are inserted into bit 0 of the two encoded words. [load+0x7d45]
    db 0x00
    db 0x00, 0x00 ; g data +0x7d46

; view_pitch_reference: Terrain-view setup stores (object_view_pitch << 8) | 2 here. The full-level trace records 811 word writes and no reads; a functional consumer remains unconfirmed. [load+0x7d48]
    dw 0x0000

; trig_phase_work: Trigonometric approximation workspace. 1B3Fh scales the angular input using C900h and stores the high product here. Complementary component evaluation replaces it with C900h minus its old value; subsequent multiply/subtract paths produce the two encoded direction components. [load+0x7d4a]
    dw 0x0000

; model_azimuth_bias: Horizontal centering angle = origin_bearing - camera_heading + 0A00h (CS:174D). The 0A00h bias becomes 160 pixels after polygon conversion divides by 16, centering a 320-pixel view. Vertex projection adds this to each vertex bearing before storing split X bytes. [load+0x7d4c]
    dw 0x0000

; view_azimuth_reference: Viewing heading reference; the object heading occupies its high byte [load+0x7d4e]
    dw 0x0000

; model_relative_height: Saved camera-relative signed split-byte 8.8 model height, added to transformed vertex heights [load+0x7d50]
    dw 0x0000

; model_planar_distance: Saved planar model-origin distance, used while projecting each mesh vertex [load+0x7d52]
    dw 0x0000

; landscape_validation_bits: Rolling equality-bit history for generated reference bytes: placement rotates each comparison result through carry into this word. Preview entry requires low-byte bits 1..4 all set (mask 1Eh). [load+0x7d54]
    dw 0x0000

; polygon_projected_x_words: 96 word-sized polygon X values indexed by projected vertex ID; 20BAh assembles split X bytes, adds projection_x_bias, arithmetic-shifts right four times, and stores here for edge rasterization [load+0x7d56]
    times TERRAIN_SIDE*2 dw 0 ; converted terrain-corner X values, indices 00h..3Fh
    times PROJECTED_MODEL_CACHE_COUNT dw 0 ; converted mesh-vertex X values, indices 40h..5Fh

; polygon_projected_x_capacity_tail: 48 additional zero words after the declared 96-word projected-X cache. Terrain uses IDs 00h..3Fh; all ten model ranges fit the 40h..5Fh cache, so validated face loops never address this trailing area. The raw extent ends at object_stack_links. [load+0x7e16]
    unconfirmed_capacity_tail 96

; object_stack_links: 64 stack-link/state bytes: negative is free, zero is terrain-supported, and 40h|slot links to a support object. CS:3681 selects the highest negative slot but does not clear its free marker; attaching at CS:346E commits it. Two allocations without attachment therefore select the same slot. Detach at CS:343D pops the cell head to its support or restores a flat height byte, then marks the removed slot 80h without clearing other object arrays. [load+0x7e76]
    times OBJECT_SLOT_COUNT db OBJECT_LINK_TERRAIN ; slots 00h..3Fh; runtime initialization follows

; object_view_pitch: 64 per-object pitch bytes, 256 angular units per turn; attachment initializes F5h (-11); camera presets can override the selected viewing slot [load+0x7eb6]
    times OBJECT_SLOT_COUNT db 0 ; slots 00h..3Fh; runtime initialization follows

; terrain_cached_cells: two 32-cell row banks, indexed by column OR row-bank bit 20h; 27CAh copies tagged world cells here, visibility processing can clear entries, and 2955h draws them [load+0x7ef6]
    times TERRAIN_SIDE db 0 ; row bank 0, cached cell tags
    times TERRAIN_SIDE db 0 ; row bank 1, cached cell tags

; terrain_cached_cells_capacity_tail: 64 zero bytes after the two 32-cell terrain row banks. Cached-cell producers and consumers mask the bank/column index to 3Fh. This is outside those banks and ends at terrain_grid; it does not declare two more active rows. [load+0x7f36]
    unconfirmed_capacity_tail 64

; terrain_grid: 32x32 cells at (x&3)*256+(x>>2)*32+y. Generation first stores random unsigned noise, box-filters it twice per axis, converts it to heights 1..11, then applies the nonlinear filter twice per axis. Before the final nibble swap height is low and slope classification is high; gameplay uses height high/shape low. C0h+ tags object stacks only after placement, so byte interpretation is phase dependent. [load+0x7f76]
; X modulo 4 = 0: columns 0, 4, 8, 12, 16, 20, 24, 28
    times (TERRAIN_SIDE/4)*TERRAIN_SIDE db 0 ; eight columns, 32 contiguous Y cells each
; X modulo 4 = 1: columns 1, 5, 9, 13, 17, 21, 25, 29
    times (TERRAIN_SIDE/4)*TERRAIN_SIDE db 0 ; eight columns, 32 contiguous Y cells each
; X modulo 4 = 2: columns 2, 6, 10, 14, 18, 22, 26, 30
    times (TERRAIN_SIDE/4)*TERRAIN_SIDE db 0 ; eight columns, 32 contiguous Y cells each
; X modulo 4 = 3: columns 3, 7, 11, 15, 19, 23, 27, 31
    times (TERRAIN_SIDE/4)*TERRAIN_SIDE db 0 ; eight columns, 32 contiguous Y cells each

; terrain_grid_capacity_tail: 512 zero bytes after the 1024-byte four-bank terrain grid. CS:2B6A constructs a base from two X bank bits and an eight-bit within-bank index, so even byte-valued input coordinates cannot reach this tail. Its next boundary is polygon_left_spans. [load+0x8376]
    unconfirmed_capacity_tail 512

; polygon_left_spans: 256 word-sized left raster boundaries; generation aliases its first 81 bytes as scratch. CS:2A6D seeds all 81 from the PRNG. CS:2B8E reloads only bytes 0..34 from a periodic 32-cell row/column. Box mode writes 32 four-sample unsigned means; nonlinear mode replaces scratch[i+1] by the median of i,i+1,i+2 for i=31..0 in place, then writes bytes 0..31 back. Scratch byte 78 retains its original random value. [load+0x8576]
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000

; polygon_right_spans: 256 word-sized right raster boundaries; placement aliases bytes +10h..4Fh as 64 coarse-region heights, +60h..9Fh as candidate X and +A0h..DFh as candidate Y. Each region covers up to 4x4 cells, excluding world row/column 31. Sampling accepts shape zero, keeps the highest height and replaces ties with the last scanned coordinate. Clearing linear neighbors +/-9,+/-8,+/-7,+/-1 around a placed principal uses the surrounding scratch halo; it does not clip coarse X/Y separately. [load+0x8776]
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    dw 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000

; sentry_ray_hit_state: Three ray/sight flag bytes. +0 tracks hits on sentry_test_target_slot; +1 records the additional object height/fraction condition at 3391h when no target hit is present; +2 collects that second condition across the two sight rays. The target-search loop rejects candidates when +2 bit 6 is set. No broader gameplay identity is assigned to the second condition. [load+0x8976]
    db 0x00, 0x00, 0x00

; current_sentry_slot: Round-robin sentry scheduler index. The update at 42FAh processes this slot, decrements it and wraps a negative value to 7. The Sentinel is slot zero; absorption and tree replanting use the same index. [load+0x8979]
    db 0x00

; allocated_object_slot: Last successful free-slot selection, not an allocation counter or permanently reserved slot. CS:3681 scans slots 63 down to 0 and changes this byte only on success. Exhaustion returns carry with this byte stale; callers must honor carry. Failed attachment also leaves it pointing at the still-free candidate; later allocations may select that same slot again. [load+0x897a]
    db 0x00

; view_grid_x: Terrain traversal camera X in the rotated viewing grid, clamped to 0..31 with excess retained separately. [load+0x897b]
    db 0x00

; polygon_y_min: Polygon rasterizer minimum row, initialized to FFh and updated by rasterized edges; the final validity test compares it with polygon_y_max. [load+0x897c]
    db 0x00

; terrain_projected_row_bank: Alternating terrain row-bank bit: zero or 20h, ORed into corner/cache indices and toggled as traversal advances rows. [load+0x897d]
    db 0x00

; polygon_y_max: Polygon rasterizer maximum row, initialized to zero and used with polygon_y_min to bound adapter span filling. [load+0x897e]
    db 0x00

; viewport_projected_x_limit: Viewport state: projected-X clipping threshold [load+0x897f]
    db 0

; camera_transition_mode: Camera/display transition mode 0..3, indexing camera_transition_steps and adapter source/delta tables; horizontal modes use heading, vertical modes use pitch. [load+0x8980]
    db 0x00

; camera_transition_request: Requested camera transition: cursor-limit handlers store modes 0..3 here; negative values indicate no pending mode in the input/controller paths. [load+0x8981]
    db 0x00

; edge_delta_x_sign: Edge workspace: sign of the original X difference, retained while interpolation uses its absolute magnitude. [load+0x8982]
    db 0x00

; player_robot_slot: Current player-controlled robot slot, used as the viewing/action origin. Transfer and successful hyperspace change this slot. [load+0x8983]
    db 0x00

; edge_saved_vertex: Long-edge subdivision scratch: polygon_subdivide_long_edge saves endpoint vertex BL before interpolation overwrites registers, then restores BL and reloads the projected endpoint after the subdivisions. This is an index into the projection caches, not a world coordinate. [load+0x8984]
    db 0x00

; terrain_corner_column: Corner-projection column saved by terrain_project_corner; sentry target search also reuses this byte for a candidate slot. [load+0x8985]
    db 0x00

; viewport_preset: Viewport state: selected predefined viewport index [load+0x8986]
    db 0

; viewport_negative_x_threshold: Coarse wrapped-negative X rejection boundary = (viewport_projected_x_limit>>1) XOR 80h (CS:28C7/2901). CS:282E first flags X at/above the positive cutoff, then increments clipping status when its high byte lies below this boundary; this is a classification threshold, not the left pixel coordinate. [load+0x8987]
    db 0
    db 0x00 ; g data +0x8988

; sentry_target_sight_result: Working sight result initialized to zero at 453Ch. The two-ray test rotates target-hit carry bits into bits 6 and 7: target/body visibility and supporting-square visibility. Target selection caches it in sentry_target_sight; bit 7 permits absorption. [load+0x8989]
    db 0x00

; shared_pass_counter: Shared pass/retry byte with scoped roles: terrain_smoothing_passes during axis filtering, placement_retry_counter during random placement, and scene_scatter_passes during background preparation. [load+0x898a]
    db 0x00

; polygon_vertex_count: Polygon workspace: count of non-repeated vertex indices, excluding the closing repeat; edge traversal stops at this count. [load+0x898b]
    db 0x00

; polygon_packed_colors: Polygon workspace: two palette indices packed into low/high nibbles; adapter span writers extract them for solid/dithered filling. [load+0x898c]
    db 0x00

; terrain_corner_bias: First-corner offset selected from terrain_quadrant_corner_bias; rotates cached quad/triangle assembly with the viewing quadrant. [load+0x898d]
    db 0x00

; terrain_view_rotation: Phase alias for view-quadrant rotation and terrain_generation_mode. CS:2ACC treats zero as clear, any negative mode as PRNG fill, positive odd modes as noise-to-height conversion, and positive even modes as nibble swap. In CS:2B8E instead, bit 7 selects column filtering and bit 6 selects nonlinear filtering. These are two separate mode protocols sharing one byte. [load+0x898e]
    db 0x00

; view_grid_y: Terrain traversal camera Y in the rotated viewing grid; corner projection subtracts it from the row coordinate. [load+0x898f]
    db 0x00

; shared_loop_work: Phase-dependent loop byte: horizontal-edge count during polygon construction, outer scatter-pass count during end effects, remaining tree count during landscape population, and a two-ray countdown during sentry sight testing. Scoped names below distinguish those uses. [load+0x8990]
    db 0x00

; projected_vertex_slot: Destination index into split X/Y projection caches: terrain uses column OR row-bank bit, meshes start at 40h. [load+0x8991]
    db 0x00

; terrain_scan_x: Shared terrain-coordinate workspace: X cell/corner currently sampled by terrain drawing, generation, placement, or ray tests. [load+0x8992]
    db 0x00

; terrain_cell_x: Terrain draw-loop cell X, retained while corner and shape helpers overwrite scan-coordinate scratch. [load+0x8993]
    db 0x00

; terrain_scan_y: Shared terrain-coordinate workspace: Y row currently sampled by terrain drawing, generation, placement, or ray tests. [load+0x8994]
    db 0x00

; placement_candidate_mask: Placement-sampling scratch: a low-bit mask chosen from placement_count_masks for the current nonzero candidate count. Principal-object placement masks each random byte and rejects values outside the actual count before indexing placement_candidates. Other phase aliases remain scoped. [load+0x8995]
    db 0x00

; viewport_x_fraction_limit: Low byte of the unsigned projected-X cutoff. Presets clear it; CS:2901 sets 80h for an odd width in eight-pixel columns and 00h for an even width. Combined with viewport_projected_x_limit, the threshold is columns*128 angular units = columns*8 pixels. [load+0x8996]
    db 0
    db 0x00 ; g data +0x8997

; terrain_row_left: Terrain traversal: current visible row interval left endpoint; terrain_find_visible_row_interval updates it and terrain_draw_view saves it before advancing rows. [load+0x8998]
    db 0x00

; terrain_row_right: Terrain traversal: current visible row interval right endpoint, paired with terrain_row_left. [load+0x8999]
    db 0x00

; terrain_triangle_shape: Shape/color-table index for a terrain triangle pair: low four bits retain the cell shape and bit 4 selects the first half color. The second half uses XOR 10h to select the opposite color. [load+0x899a]
    db 0x00

; terrain_previous_row_left: Terrain traversal: previous row left endpoint, saved before row-bank exchange and compared against the next projected interval. [load+0x899b]
    db 0x00

; terrain_previous_row_right: Terrain traversal: previous row right endpoint, saved alongside the previous left endpoint. [load+0x899c]
    db 0x00

; polygon_face_flags: Face-construction mode: 00h builds a terrain quad, 40h uses the mesh index loop, 80h builds the first terrain triangle, C0h builds its opposite half. Vertex count is stored separately. [load+0x899d]
    db 0x00

; edge_subdivision_count: Long-edge subdivision countdown; the rasterizer consumes this while splitting an edge into shorter interpolation steps. [load+0x899e]
    db 0x00
    db 0x00 ; g data +0x899f

; terrain_triangle_orientation: Per-cell split selector consumed by CS:204D: zero joins the top-left/bottom-right diagonal, one joins top-right/bottom-left. Ordinary slopes use terrain_triangle_orientations after view rotation; saddle codes 4/12 use terrain_triangle_view_bias&1. Both halves share the same selector. [load+0x89a0]
    db 0x00
    db 0x00, 0x00, 0x00, 0x00 ; g data +0x89a1

; polygon_edge_index: Current vertex-loop edge index, saved before edge helpers and incremented until polygon_vertex_count. [load+0x89a5]
    db 0x00

; terrain_triangle_view_bias: Signed quadrant bias = view_quadrant-2, hence FEh/FFh/00h/01h (CS:2444). For shape 4/12 its parity selects the diagonal. Other slopes add shape bit 2 to this byte and use the unsigned result <2 to select which color-half bank is emitted first (CS:2A0A). [load+0x89a6]
    db 0x00

; model_mesh_id: Mesh selector copied from the drawn object; indexes vertex/face range and visibility-policy tables. [load+0x89a7]
    db 0x00
    db 0x00 ; g data +0x89a8

; model_element_index: Shared mesh-loop index: current vertex while projecting, current face while drawing. [load+0x89a9]
    db 0x00

; model_element_end: Exclusive vertex/face range end loaded from the next mesh-range entry. [load+0x89aa]
    db 0x00

; viewport_y_end: Viewport state: exclusive upper Y clip coordinate [load+0x89ab]
    db 0

; viewport_y_min: Viewport state: inclusive lower Y clip coordinate [load+0x89ac]
    db 0

; model_face_pass: Mesh face-order countdown: zero selects one stored-order pass; two then one select the ordered bit-7-clear/bit-7-set passes, using model_face_pass_xor to choose the group. [load+0x89ad]
    db 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00 ; g data +0x89ae

; ray_sample_mode_flags: Ray sample mode/status: height-cache building starts at zero; detailed ray stepping sets 80h. The sampler may set C0h, whose bit 6 the tracer uses to reject a contact. [load+0x89b3]
    db 0x00

; terrain_face_rotation_bias: Phase alias: terrain rendering stores quadrant*4 here for shape rotation. Generation calls it terrain_smoothing_flags: 00h selects four-sample averaging and 40h selects the descending in-place median-of-three filter. CS:2B8E ORs in the caller direction bit 80h to choose a column rather than a row; each smoothing stage runs two row-and-column passes. [load+0x89b4]
    db 0x00

; projection_direction_flags: Shared direction workspace: sine/cosine lookup saves the angular byte here; bits 7/6 encode signs/quadrants, and signed multiplication toggles bit 7 for factor signs. [load+0x89b5]
    db 0x00

; polygon_clip_work: Polygon workspace cleared at CS:20AB before each projected index loop. The full-level trace records 94,081 zero writes and no reads of this byte; its role beyond that reset remains unconfirmed. The neighboring byte at DS:1107 remains separate unexplained storage. [load+0x89b6]
    db 0x00
    db 0x00 ; g data +0x89b7

; view_object_slot: Shared projection context: object slot used as the viewing origin for camera-relative model/terrain transforms; other phases may reuse this byte. [load+0x89b8]
    db 0x00

; drawn_object_slot: Shared projection context: drawn-object slot saved by model_prepare_camera_relative_origin before helper calls overwrite DI; recovered before returning from model drawing. [load+0x89b9]
    db 0x00

; terrain_corner_height_a: Shared terrain-generation scratch: height of the first sampled corner used by smoothing/shape comparisons; other phases reuse this byte. [load+0x89ba]
    db 0x00

; math_work_lo: Shared arithmetic scratch low byte: fractional phase input to sine/cosine lookup and low half of the signed work pair; other gameplay phases reuse this storage. [load+0x89bb]
    db 0x00

; math_work_hi: Shared arithmetic scratch high byte: byte multiplier operand and high half of the signed work pair; math_negate_work_pair negates the two-byte value in place. [load+0x89bc]
    db 0x00

; terrain_corner_height_b: Shared terrain-generation scratch: second sampled corner height used with terrain_corner_height_a to choose height/shape transitions. [load+0x89bd]
    db 0x00

; ray_corner_height_wrap: Closing copy of height A: ray contact sampling stores the first corner at both DS:110A and 110E, allowing adjacent endpoint reads for all four edges of the A,C,D,B,A height ring. [load+0x89be]
    db 0x00

; ray_contact_work: Contact interpolation scratch: first holds the shape/contact case, then the selected edge starting height while the signed height difference is multiplied by the selected fractional coordinate. [load+0x89bf]
    db 0x00

; ray_sample_fraction: Fractional height returned by the detailed terrain/object sampler: ray stepping clears it before sampling, object-specific paths fill it, and height comparisons combine it with the integer result in AL. [load+0x89c0]
    db 0x00

; projection_axis_ratio: Upper byte of q=floor(65536*minor/major), so the distance-correction index is ceil(this_byte/2), 0..128. Equal axes, including zero/zero, set FFh without DIV at CS:1B33. Thus no divide-by-zero occurs when both input deltas are zero. [load+0x89c1]
    db 0x00

; projection_clip_status: Shared projection/clipping status byte: corner projection records rejection bits/counts here; polygon edge processing clears it on success and tests it before accepting spans. [load+0x89c2]
    db 0x00

; projection_delta_x_sign: Shared projection workspace: sign of the original X/height delta before taking its absolute magnitude; quadrant restoration tests bit 7. [load+0x89c3]
    db 0x00
    db 0x00 ; g data +0x89c4

; projection_delta_y_sign: Shared projection workspace: sign of the original Y/planar delta; combined with the other sign to restore the full-turn bearing quadrant. [load+0x89c5]
    db 0x00
    db 0x00 ; g data +0x89c6

; projection_sin: Unsigned sine-like component from the quarter-wave lookup, with component exchange determined by quadrant bits in projection_direction_flags. [load+0x89c7]
    db 0x00

; projection_cos: Unsigned cosine-like component paired with projection_sin; signs are handled separately by projection_direction_flags. [load+0x89c8]
    db 0x00

; ray_position_x: 24-bit ray X accumulator: low fractional byte, high fractional byte, integer byte. Signed 16-bit steps add into the low word with sign/carry extension into the integer byte. Initialization uses the object cell center (fraction 8000h). [load+0x89c9]
    db 0, 0, 0 ; fraction low, fraction high, integer (8.16 fixed point)

; ray_position_height: 24-bit ray height accumulator: low fractional byte, high fractional byte, integer byte. Signed 16-bit steps add into the low word with sign/carry extension into the integer byte. Initialization appends a zero low fractional byte to the object 8.8 height. [load+0x89cc]
    db 0, 0, 0 ; fraction low, fraction high, integer (8.16 fixed point)

; ray_position_y: 24-bit ray Y accumulator: low fractional byte, high fractional byte, integer byte. Signed 16-bit steps add into the low word with sign/carry extension into the integer byte. Initialization uses the object cell center (fraction 8000h). [load+0x89cf]
    db 0, 0, 0 ; fraction low, fraction high, integer (8.16 fixed point)
    db 0x00, 0x00, 0x00, 0x00 ; g data +0x89d2

; terrain_view_rotation_work: Terrain setup stores object heading minus 0Ah here. The full-level trace records 811 byte writes and no reads; a functional consumer or other phase use remains unconfirmed. [load+0x89d6]
    db 0x00

; terrain_draw_stop: Held-direction cancellation flag. The timer-side helper writes zero while the stored direction matches an active directional input and 80h otherwise. When terrain_query_flags bit 7 allows interruption, terrain_draw_view tests this byte after a row and returns with carry on cancellation. The render controller clears it before a new transition. [load+0x89d7]
    db 0x00
    db 0x00, 0x00 ; g data +0x89d8

; object_grid_x: 64 byte grid-X coordinates; camera slots may lie outside the playable grid [load+0x89da]
    times OBJECT_SLOT_COUNT db 0 ; slots 00h..3Fh; runtime initialization follows

; object_height_integer: 64 integer bytes of split 8.8 object origins. Bare-terrain attachment takes the terrain height nibble. Stacking adds the support-specific offset with carry from object_height_fraction; detaching a terrain-supported head restores its integer height<<4 as a flat cell. Freeing a slot retains this byte, so allocation alone does not make it a valid new height. [load+0x8a1a]
    times OBJECT_SLOT_COUNT db 0 ; slots 00h..3Fh; runtime initialization follows

; object_grid_y: 64 byte grid-Y coordinates; coordinate differences use byte wraparound [load+0x8a5a]
    times OBJECT_SLOT_COUNT db 0 ; slots 00h..3Fh; runtime initialization follows

; object_heading: 64 azimuth bytes; 256 units per turn, used to orient camera and mesh [load+0x8a9a]
    times OBJECT_SLOT_COUNT db 0 ; slots 00h..3Fh; runtime initialization follows

; projected_y_low: 96 split-coordinate cache entries: low byte of projected Y; indices 00h..3Fh hold two terrain rows and 40h..5Fh hold projected mesh vertices [load+0x8ada]
    times TERRAIN_SIDE*2 db 0 ; two alternating 32-corner terrain rows
    times PROJECTED_MODEL_CACHE_COUNT db 0 ; mesh projection scratch, starting at index 40h

; projected_x_low: 96 split-coordinate cache entries: low byte of projected X; indices 00h..3Fh hold two terrain rows and 40h..5Fh hold projected mesh vertices [load+0x8b3a]
    times TERRAIN_SIDE*2 db 0 ; two alternating 32-corner terrain rows
    times PROJECTED_MODEL_CACHE_COUNT db 0 ; mesh projection scratch, starting at index 40h

; object_height_fraction: 64 fractional origin-height bytes. Bare terrain attaches at fraction E0h (7/8 cell). A boulder adds 80h (half a cell), carrying into the integer byte; a Sentinel pedestal preserves the fraction and adds one integer cell. These offsets depend on the support model, not the model being attached. Rejected attachment and detach leave the old fraction unchanged. [load+0x8b9a]
    times OBJECT_SLOT_COUNT db 0 ; slots 00h..3Fh; runtime initialization follows

; projected_y_high: 96 split-coordinate cache entries: high byte of projected Y; indices 00h..3Fh hold two terrain rows and 40h..5Fh hold projected mesh vertices [load+0x8bda]
    times TERRAIN_SIDE*2 db 0 ; two alternating 32-corner terrain rows
    times PROJECTED_MODEL_CACHE_COUNT db 0 ; mesh projection scratch, starting at index 40h

; projected_x_high: 96 split-coordinate cache entries: high byte of projected X; indices 00h..3Fh hold two terrain rows and 40h..5Fh hold projected mesh vertices [load+0x8c3a]
    times TERRAIN_SIDE*2 db 0 ; two alternating 32-corner terrain rows
    times PROJECTED_MODEL_CACHE_COUNT db 0 ; mesh projection scratch, starting at index 40h

; object_model_id: 64 model selectors, retained even in free slots. Allocation writes only this field and allocated_object_slot; it does not initialize position/height/link state. Attachment accepts occupied-square supports only when the current top model is BOULDER or SENTINEL_PEDESTAL. A rejected attach still leaves the requested model in the free slot, available for a later retry. [load+0x8c9a]
    times OBJECT_SLOT_COUNT db 0 ; slots 00h..3Fh; runtime initialization follows

; trig_encoded_components: Two word-sized direction components computed by the angular approximation at 1B3Fh. Bit 0 carries each component sign; the remaining bits encode magnitude. Ray setup shifts a word right four times, preserves the original sign bit through flags, and negates the result when required; heading multipliers also consume this pair. [load+0x8cda]
    dw 0x0000, 0x0000

; sentry_warning_pattern: Warning-bar pattern-group offset: 0 clears, 4 selects the animated warning group, and the pause path selects solid group 8. Sentry warning aggregation writes this byte; all four adapter backends consume it. [load+0x8cde]
    db 0x00

; hud_tile_column: Current energy-HUD tile column. hud_draw_player_energy clears it, hud_append_tile passes it to graphics_draw_status_tile and increments it, and the padding/frame loops stop at columns 29 and 38. [load+0x8cdf]
    db 0x00

; landscape_object_height_limit: Secondary-placement height limit derived from the principal placement threshold>>4. CS:3E95 accepts heights strictly below this byte, not <= it; an 8-bit retry counter relaxes the limit by one on wrap, before attempts 256,512,..., until it reaches 12. Player placement caps its initial limit at 6, while trees start from this landscape limit. Coordinates are sampled only in 0..30. [load+0x8ce0]
    db 0x00

; landscape_difficulty: Principal-count cap from valid packed BCD: landscapes 0..99 use min(8,1+floor(number/10)); every landscape >=100 uses 8. CS:160C saves the original BCD and low PRNG word before changing BL to this cap. It is a cap, not the actual sentry count: CS:408E clamps the separately sampled count and placement may reduce it further. [load+0x8ce1]
    db 0x00

; landscape_height_scale: Noise amplitude multiplier used by CS:2B03: height=clamp(7+floor((noise-128)*scale/256),1,11), with noise-128 interpreted as a signed byte. Negative products round downward through the high byte of the negated full product. Landscape zero selects 24; other landscapes select prng_small_sum+14. Smoothing occurs both before and after this conversion. [load+0x8ce2]
    db 0x00

; previous_sentry_warning_pattern: Last warning pattern accepted by the active graphics backend. Each backend skips drawing when both current and previous patterns are zero; active patterns continue animating, and a transition to zero clears the old bar. [load+0x8ce3]
    db 0x00

; player_energy: Player energy, initially 10. Create/hyperspace spend object_energy_values; absorption credits energy. The HUD displays this six-bit counter. [load+0x8ce4]
    db 0x00

; meanie_turn_step: Temporary signed heading increment for the sentry-controlled Meanie. The scheduler chooses +8 or -8 from the target bearing sign, preserves it across the nested object-update helper, adds it to the Meanie heading, and reloads its scan delay. [load+0x8ce5]
    db 0x00

; text_input_render_flags: Bit 7 selects direct text input rendering. The decimal-entry repaint helper sets it while drawing digits/backspaces and clears it afterward. The formatted-character dispatcher bypasses its normal printable-character wrapper when the bit is set. [load+0x8ce6]
    db 0x00

; terrain_text_character_work: Character/control byte saved on entry to game_draw_character. Printable characters select an eight-row font record. The terrain glyph loop later reuses this byte as a blank row by clearing it after its seven visible font rows. [load+0x8ce7]
    db 0x00

; game_glyph_rows: Eight bytes copied from the selected game_font_8x8 glyph after subtracting character 20h and multiplying by eight. game_draw_character consumes paired font bits to build terrain text cells; its preceding character/work byte supplies a cleared blank baseline. [load+0x8ce8]
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x00 ; g data +0x8cf0

; sentinel_start_square_x: Original Sentinel square X, saved when principal slot zero is placed; independent of the current player robot. [load+0x8cf1]
    db 0x00

; sentinel_start_square_y: Original Sentinel square Y; paired with sentinel_start_square_x for the return-square comparison after hyperspace. [load+0x8cf2]
    db 0x00

; terrain_query_flags: Terrain-query control byte, with bit 7 enabling early cancellation of a view render. The transition controller sets this bit for eligible cursor/view modes around rendering, then clears it. Terrain row traversal consults terrain_draw_stop only while this bit is set; other bits remain unnamed. [load+0x8cf3]
    db 0x00

; scene_view_model_selector: Scene composition model selector: nonnegative values request the view-object avatar; a negative selector omits that avatar before terrain drawing. [load+0x8cf4]
    db 0x00

; held_direction_action: Last nonnegative camera_transition_request accepted by the input callback. During rendering, the held-direction helper compares this action against horizontal and vertical input slots to decide whether terrain drawing may continue or should stop. [load+0x8cf5]
    db 0x00

; selection_copy_interrupt_flags: Bit 7 interrupts randomized selected-view copying. Input readiness and model/view setup set it; new action/transition paths clear it. All four selection-reveal backends test it after copying a mask group and return early when set. [load+0x8cf6]
    db 0x00

; partial_view_update_flags: Bit 7 permits object changes to update the selected view while the player input loop waits. That loop clears it each visit, then sets it when the saved input-ready flag is negative and terrain_draw_stop is zero. The nested object update helper tests it before preparing the partial redraw. [load+0x8cf7]
    db 0x00

; sentry_absorb_delays: Eight sentry target-acquisition warmup counters. CS:44BD reloads 120 only when zero; 119 clock updates bring it to ready value 1, which the clock holds. Absorbing a unit does not reload 120: subsequent actions are paced by the scan cooldown until sight loss clears this counter. Zero means no acquired target, not an immediately ready absorption. [load+0x8cf8]
    times SENTRY_SLOT_COUNT db 0 ; eight sentry timers; zero/one are held terminal states

; sentry_rotation_delays: Eight independent rotation cooldowns. A turn at CS:449D loads 200; 199 clock updates bring it to 1. The no-target path accepts either 0 or 1, but a retained energy target bypasses rotation even if the cooldown is ready. The clock continues decreasing it while eligible, so rotation can resume immediately after losing a long-held target. [load+0x8d00]
    times SENTRY_SLOT_COUNT db 0 ; eight sentry timers; zero/one are held terminal states

; sentry_scan_delays: Eight scheduler cooldowns; values 0/1 permit a scan and 2..255 wait. CS:4336 reloads 4 (three decrements); Meanie turning loads 10 (nine), absorption loads 30 (29), and successful Meanie creation loads 50 (49). These cooldowns pace repeated energy removal after acquisition warmup has reached its held ready value. [load+0x8d08]
    times SENTRY_SLOT_COUNT db 0 ; eight sentry timers; zero/one are held terminal states

; terrain_face_indices: five-byte terrain polygon loop; quads use four indices plus a repeat of index 0, triangles use three indices plus a repeat in slot 3. Mesh drawing restores polygon_index_list to this scratch loop afterward. [load+0x8d10]
    db 0 ; vertex 0
    db 0 ; vertex 1
    db 0 ; vertex 2
    db 0 ; quad vertex 3, or triangle closing repeat of vertex 0
    db 0 ; quad closing repeat of vertex 0
    db 0x00, 0x00 ; g data +0x8d15

; model_face_pass_xor: Mesh face-group XOR mask: initialized to zero, then changed to 80h for the second ordered face pass. [load+0x8d17]
    db 0x00

; terrain_row_left_hint: Persistent left-bound hint for the next terrain view. Setup copies it into terrain_row_left before finding the first visible interval, then stores the resulting left bound back here. Subsequent rows use separate current/previous interval fields. [load+0x8d18]
    db 0x00

; terrain_text_column: Two terrain-text coordinates: column at +0 and row at +1. game_draw_character control bytes select which coordinate to set; glyph drawing copies them into terrain_scan_x/y and advances the column by four terrain cells after a character. [load+0x8d19]
    dw 0x0000

; terrain_model_render_flags: Rendering policy byte: bit 7 switches terrain cells to reserved-slot model representations; title construction sets it temporarily, and scene/terrain paths also use indexed style aliases. [load+0x8d1b]
    db 0x00

; scene_background_style: Background/dither style index used by adapter background-pattern tables and scene text colors; style 3 also selects scattered highlight preparation. [load+0x8d1c]
    db 0x00

; selection_model_draw_flags: Partial-view drawing policy: bit 7 selects drawing only the allocated model rather than the terrain view; reset after the presentation path. [load+0x8d1d]
    db 0x00

; game_end_flags: Shared end-state flag: zero permits normal play; exhausted-player absorption writes 80h, while the render controller also copies the signed hyperspace outcome here. Timer/input paths and selection presentation test its sign to stop normal play or select the end transition. [load+0x8d1e]
    db 0x00

; warning_bar_sight: Aggregated sentry sight of the player robot. 45EFh prefers a result with bit 7 set over target-only 40h; the adapter renderers clear the second half of the bar only for 40h. Full sight permits absorption. [load+0x8d1f]
    db 0x00
    db 0x00 ; g data +0x8d20

; turn_and_occlusion_flags: Shared turn/input flag: the player wait starts at zero, idle mouse-cursor handling stores 40h, and the turn-around command shifts it left and requires the resulting sign bit before reversing heading. The render controller skips occlusion rebuilding while its sign is set. [load+0x8d21]
    db 0x00

; landscape_nonzero: Landscape-zero test byte: CS:160C stores the BCD high pair when nonzero, otherwise the low pair. Thus nonzero is not a normalized boolean: landscape 0013 stores 13h and 1200 stores 12h. Only zero selects the built-in entry, fixed scale 24, one principal and starting robot square (8,17). Other setup uses its zero/nonzero status rather than its numeric magnitude. [load+0x8d22]
    db 0x00

; generation_validation_seed: Shared seed for the generated validation ring. Odd-parity terrain quads copy the middle PRNG byte here at CS:29F8 (BX=8). CS:4120 reads it with BX=AAh to choose the first output index; CS:2E83 reads it with BX=FFh to locate the startup reference check. The full-level trace records 8,190 snapshots and seeds 38h/A5h for the startup/completion runs. [load+0x8d23]
    db 0x00

; sentry_test_target_slot: Object slot currently tested by sentry rays. 4531h copies the candidate DL here; the ray sampler marks hits against this slot, and sentry_absorb_energy_unit uses it to choose the player energy balance or an unattended object. [load+0x8d24]
    db 0x00

; cursor_mode_flags: Bit 7 selects cursor mode. Space toggles it at 3D94h; mouse selection can set it at 3D6Fh/3EF6h. Nonnegative mode allows looking around; object actions below ACTION_HYPERSPACE require bit 7 at 3F5Ch. Preview entry clears it. [load+0x8d25]
    db 0x00

; number_terrain_text_flags: Bit 7 routes decimal digits to game_draw_character for terrain-built text instead of the regular screen-text formatter. The completion code emitter sets it before printing BCD pairs and clears it at the end; numeric zero is displayed as the letter O. [load+0x8d26]
    db 0x00

; pending_object_model: Allocation request latch: CS:367E stores AL before searching; CS:3681 retries the saved model without a new AL request. The latch changes even when the pool is exhausted. Only successful search copies it into object_model_id; no object slot is reserved until the later attachment changes its free link marker. [load+0x8d27]
    db 0x00

; selection_view_column: Current partial-view start in eight-pixel columns; converted to an angular reference by shifting the word (column << 8) right once. [load+0x8d28]
    db 0x00

; player_view_refresh_flags: Pending view-refresh bit. Successful transfer/turn-around dispatch and hyperspace cleanup set 80h. The action loop tests and clears this bit to return for a redraw, or shifts it into carry while waiting to force the same exit. [load+0x8d29]
    db 0x00

; restart_requested_flags: Bit 7 requests return to the title/landscape flow. Escape sets it in the key wait or gameplay input; player_poll_actions leaves its wait loop when set, and the preview gate checks it before starting play. Workspace reset clears it. [load+0x8d2a]
    db 0x00
    db 0x00 ; g data +0x8d2b

; ray_sample_special_flags: Ray sample status byte cleared before each sample; the model-3 path sets bit 7, which the tracer combines with action flags before testing the vertical step direction. [load+0x8d2c]
    db 0x00

; sentry_scan_angle_width: Angular comparison width for the sentry sight test: normal scheduling sets 14h and the secondary-target scan sets 28h. 4531h halves the selected width for its centered heading-window comparison; values are internal angular units. [load+0x8d2d]
    db 0x00

; render_pass_columns: Current render-pass width in eight-pixel columns. Selection setup caps its first pass at 20 and draws any remainder at a 160-pixel offset; full-view camera transitions can load 40. [load+0x8d2e]
    db 0x00

; selection_view_columns_left: Selected interval width remaining: initialized with total width, reduced by the current pass, then reused as the second-pass width when nonzero. [load+0x8d2f]
    db 0x00

; object_stack_count: Linked-stack draw workspace: number of additional linked objects; drawing decrements it through zero to negative while traversing the stack. [load+0x8d30]
    db 0x00

; object_stack_head: Linked-stack draw workspace: low-six-bit head slot saved so multi-pass stack drawing can restart traversal. [load+0x8d31]
    db 0x00

; selection_presentation_flags: Selected-update presentation policy: bit 7 requests cursor restore before rendering; bit 6 requests randomized reveal afterward. With bit 6 clear the direct strip copy runs; with it set, direct copy follows only when game_end_flags is nonnegative. Before direct copy the controller always restores and hides the cursor, then optionally redraws it if cursor mode is active. Cleanup clears this byte after either path; lower bits are not tested here. [load+0x8d32]
    db 0x00

; ray_target_test_flags: Shared workspace byte: during ray targeting bit 7 enables the special target-test path. Player actions clear it; the sentry sight test sets it for the first ray and clears it for the second. Terrain text also addresses this area as indexed style/validation workspace, so ray naming is restricted to ray code. [load+0x8d33]
    db 0x00

; landscape_sentry_count: Actual principal-slot count, including Sentinel slot zero. Requested count is 1 on landscape zero, otherwise a 1..8 sample capped by landscape_difficulty. If no candidate remains as the height threshold falls to zero, CS:4172 replaces this count with the number already placed. Palette variant is (count-1)&7; initial trees are capped at 48-3*count, leaving room in the 64-slot pool. [load+0x8d34]
    db 0x00

; sound_melody_duration: Current melody duration in game-timer ticks: a C8h..FEh command supplies four times its offset from C8h; each note copies this byte into sound_trigger_countdown. [load+0x8d35]
    db 0x00

; landscape_generation_flags: Bit 7 selects completion/code-generation mode. Winning hyperspace sets 80h; terrain filtering suppresses the startup return patch in that mode, and object placement starts the BCD/reference loop at A5h rather than AAh. Placement shifts the flag out and returns instead of entering the preview. [load+0x8d36]
    db 0x00

; game_pause_flags: Pause latch: the F4 action path sets bit 7 before displaying the solid warning pattern and waiting for the continue action; it clears bit 7 on resume. game_timer_tick tests this flag after its sound updates and skips normal game processing while paused. [load+0x8d37]
    db 0x00

; sound_sequence_mode: Sound dispatcher mode: 3 advances melody bytecode, 4 suppresses automatic triggers, 6 runs the effect-6 pitch ramp; other values schedule effect 4 with countdown 36h. [load+0x8d38]
    db 0x00

; sound_ramp_index: Effect-6 ramp counter: triggers stop at AAh, delays are random 1..4 ticks, and each trigger advances the counter and patches effect-6 pitch from min(31,(counter>>2)+1) OR 32h. [load+0x8d39]
    db 0x00

; text_prng_compare_value: Shared comparison byte: terrain text calls the alternate PRNG entry only when this byte is below terrain_generation_saved_noise. Sentry sight setup stores saved_noise+1, or saved_noise+7 when its low nibble is 9, modulo 256. The full-level trace confirms 3,301 writes of 1; the eight completion digits compare 1 against 0, and both title banners compare 0 against 0. The purpose of this coupling remains unconfirmed. [load+0x8d3a]
    db 0x00

; view_lateral_offset: Signed horizontal screen offset for scene rendering. landscape_render_scene loads it from scene_lateral_offset and clears it on exit; mesh and terrain corner projection subtract this byte from the projected X coordinate. [load+0x8d3b]
    db 0x00
    db 0x00 ; g data +0x8d3c

; model_dither_flags: Mesh monochrome-distance latch: CS:18AA sets bit 7 when the planar-distance integer byte is at least 15, otherwise clears it. Span setup then chooses palette color A for both dither colors. This removes the second dither color at distance; it does not discard the mesh. CS:1953 clears the sign bit after drawing. [load+0x8d3d]
    db 0x00

; prng_state_low: 40-bit shift register stored low-word/middle-byte/high-word. Each output advances eight times: state=((state<<1)|((state>>19 XOR state>>32)&1)) modulo 2^40, then returns bits 32..39. Derived from the carry chain at 240Ch and checked against 128 native DOSBox bytes. [load+0x8d3e]
    dw 0 ; prng_state_low: bits 0..15
    db 0 ; prng_state_middle: bits 16..23
    dw 0 ; prng_state_high: bits 24..39

; sentry_scan_cursor: Eight sentry entries, indexed by the sentry scheduler slot 0..7: scan cursor; sentry_reset_scan_state resets the selected entry to 40h. The initialized image contains zeros before scene setup. [load+0x8d43]
    times SENTRY_SLOT_COUNT db 0

; sentry_replant_energy: Eight sentry energy balances: absorption credits one unit; successful tree replanting spends one unit. Slot zero belongs to the Sentinel. [load+0x8d4b]
    times SENTRY_SLOT_COUNT db 0

; sentry_scan_target: Eight sentry entries, indexed by the sentry scheduler slot 0..7: selected target object slot; reset sentinel 80h. The initialized image contains zeros before scene setup. [load+0x8d53]
    times SENTRY_SLOT_COUNT db 0

; sentry_scan_pass: Eight sentry entries, indexed by the sentry scheduler slot 0..7: scan pass state; reset value zero. The initialized image contains zeros before scene setup. [load+0x8d5b]
    times SENTRY_SLOT_COUNT db 0

; sentry_meanie_slots: Eight sentry entries, indexed by the sentry scheduler slot 0..7: active Meanie object slot; reset marker 80h. A suitable tree is converted into this model during the secondary-target scan. The initialized image contains zeros before scene setup. [load+0x8d63]
    times SENTRY_SLOT_COUNT db 0

; sentry_target_slots: Eight sentry selected object slots, stored from DL after target tests and read while an action timer is active; distinct from the scan cursor and Meanie-slot arrays. [load+0x8d6b]
    times SENTRY_SLOT_COUNT db 0

; sentry_target_sight: Eight cached sentry sight results: bit 6 is target/body visibility (half warning for the player); bit 7 is supporting-square visibility (full warning and absorption allowed). [load+0x8d73]
    times SENTRY_SLOT_COUNT db 0

; sentry_stack_search_flags: Eight per-sentry request flags. Failed secondary-target handling with scan-pass state below two sets bit 7. A later scheduler visit invokes the supported-object search at 47BDh, resets the scan cursor on success, or clears the request bit when no eligible target is found. [load+0x8d7b]
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

; video_transition_steps_left: Signed remaining transition-step counter. Adapter transition loops decrement it before lookup, returning when it becomes negative; twice its nonnegative value indexes source-offset words. [load+0x8d83]
    db 0x00
    db 0x00, 0x00, 0x00, 0x00 ; g data +0x8d84

; camera_cursor_x_pairs: Persistent horizontal aiming coordinate in two-pixel units, copied into software_cursor_x_pairs before display and updated by cursor-limit/transition logic. [load+0x8d88]
    db 0x00

; camera_cursor_inverted_y: Persistent inverted vertical aiming coordinate, copied into software_cursor_inverted_y and adjusted by cursor/pitch-transition logic. [load+0x8d89]
    db 0x00

; cursor_repeat_phase: Keyboard selection-cursor repeat phase. The input callback seeds 6Bh when leaving mouse handling, then shifts it left during cursor-mode directional input. Carry skips cursor movement on that update; the phase is separate from the Space debounce latch. [load+0x8d8a]
    db 0x00

; software_cursor_saved_last_step: Cursor restore-state sentinel and last saved step index. Zero means no saved cursor and makes restore return without touching video memory. Drawing all 21 steps increments to 21 at the Y terminator then decrements to 20; restore walks 20..0 and resets to zero. Thus this byte is not a count of saved pixels, and the initial zero deliberately does not restore record zero. [load+0x8d8b]
    db 0x00
    db 0x00, 0x00, 0x00 ; g data +0x8d8c

; selection_copy_column_accumulator: Randomized selection-copy column accumulator shared by CGA/Tandy/EGA/VGA reveal routines. Each iteration adds a PRNG-derived byte, masks to a power-of-two candidate range, and wraps candidates at the actual view width. It selects corresponding source and destination columns. [load+0x8d8f]
    db 0x00

; occlusion_reference_check_flags: Lazy reference-pattern check latch in occlusion_trace_grid_row. A clear sign performs the generated-buffer test: four bytes must be 7Fh and the preceding fifth byte must differ; failure exits the row test. Success sets bit 7 so subsequent rays bypass the check. Workspace reset clears it. [load+0x8d90]
    db 0x00

; ega_selection_column_mask: EGA selection-reveal candidate mask. The backend derives it from placement_count_masks using the requested selection width, then applies it to the accumulated random column before comparing against ega_selection_column_limit. [load+0x8d91]
    db 0x00
    db 0x00 ; g data +0x8d92

; video_transition_step_count: Number of steps requested by camera-transition setup and copied into video_transition_steps_left; pitch paths use eight steps. [load+0x8d93]
    db 0x00

; selection_copy_inner_countdown: Inner randomized-copy iteration budget. CGA/Tandy/VGA initialize FFh, EGA initializes C8h. Each copied mask/pixel group decrements it; expiry consumes one selection_copy_iteration_budget unit and starts another batch unless a redraw interruption ends the operation. [load+0x8d94]
    db 0x00

; ega_selection_column_limit: Actual EGA selection width copied from selection_view_total_columns at backend entry. A masked random candidate at or above this limit has the width subtracted to wrap it into the selected source/destination columns. [load+0x8d95]
    db 0x00

; selection_extent_override: Transient model extent override: 3603h takes the maximum of this byte and model_selection_extents, clears it, then projects that extent to an angular half-width. [load+0x8d96]
    db 0x00
    db 0x00, 0x00 ; g data +0x8d97

; software_cursor_hide_flags: Cursor drawing is skipped when this byte is negative; the paired-view render path sets bit 7 while preparing a view and clears it before restoring cursor drawing. [load+0x8d99]
    db 0x00

; input_ready_snapshot: Snapshot of input_request_ready_flags copied at 3E5Eh. The player wait loop tests its sign alongside terrain_draw_stop to decide whether to set the partial-view update flag. This is separate from the current readiness byte written by the timer input path. [load+0x8d9a]
    db 0x00

; hyperspace_outcome_flags: Outcome byte read by the gameplay render controller: failed hyperspace due to insufficient energy sets 80h, while departing from the original Sentinel square sets C0h. Bit 7 enters an end sequence and bit 6 selects landscape completion and secret-code generation. [load+0x8d9b]
    db 0x00

; sound_trigger_countdown: Interrupt-driven sound countdown. game_timer_tick decrements it and clamps underflow to zero; zero allows the sequencer/effect-trigger dispatcher to advance. [load+0x8d9c]
    db 0x00
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; g data +0x8d9d

; input_request_ready_flags: Input handshake byte: player_poll_actions clears it while waiting; the input callback writes 80h when an action or view transition is ready. The foreground loop then handles that request, while the timer uses the sign to select new-input polling versus held-direction checks. [load+0x8da4]
    db 0x00

; sentry_clock_gate_flags: Bit 7 gates sentry countdowns and the warning-bar timer callback. Scene reset initializes 80h, which suppresses those updates; accepting a player action clears bit 7. The game timer advances sentry clocks and draws the warning only when the sign is clear. [load+0x8da5]
    db 0x00

; secret_code_pair_work: Phase-dependent secret-code display workspace. Transfer records a supporting object model here; completion halves it into a BCD-pair loop index, advances the PRNG until the index is below four, then prints down to index zero. MODEL_SENTINEL_PEDESTAL (6) therefore prints four pairs (eight digits). Reset initializes 80h; the loop finishes with FFh. [load+0x8da6]
    db 0x00

; sound_melody_cursor: Melody-stream byte offset: sound_advance_melody increments it before decoding a command or pitch. FFh is the stream-end/inactive value tested by startup and scene waits. [load+0x8da7]
    db 0x00

; input_action_slots: Four action bytes reset to 80h on every binding collection, even when only a directional prefix is scanned. Entries 0/2 are horizontal/vertical values, 1 is an object/game command, and 3 is auxiliary control. CS:3FC7 returns AL=slot0 AND slot2, making its sign a both-directions-inactive test: one active direction clears the sign. Held-direction code saves/restores the object command because its directional refresh otherwise clears slot 1. [load+0x8da8]
    db 0 ; action destination 0; runtime reset uses 80h for inactive
    db 0 ; action destination 1; runtime reset uses 80h for inactive
    db 0 ; action destination 2; runtime reset uses 80h for inactive
    db 0 ; action destination 3; runtime reset uses 80h for inactive

; scene_flags: Scene composition flags: bit 7 selects the TFINAL image and forces background style 2; bit 5 adds the SKYBOX image overlay; bit 6 bypasses view-object and terrain drawing. Title/code setup uses C0h/A0h and clears this byte after rendering. [load+0x8dac]
    db 0x00

; sentry_rotation_steps: Eight sentry signed heading increments. Placement chooses +20 or -20 at 4214h, and the scheduler adds the selected byte to the sentry heading at 449Dh; angles wrap modulo 256. [load+0x8dad]
    times SENTRY_SLOT_COUNT db 0
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; g data +0x8db5

; occlusion_rows: 32 row slots sharing the resource input buffer during loading. Independent gameplay use of the row tails remains unconfirmed. [load+0x8dbd]
; Row stride: 100h. Bytes 00h..1Fh hold (sample height << 1) | sample flag.
; Rows 0..30 use bytes 20h..3Eh for cell thresholds; row 31 has no thresholds.
; Builder CS:2F0C samples via CS:3302; CS:2DCE reads heights by shifting right.
; Threshold = current height, or four-neighbor encoded minimum if flagged (CS:2F50).
; Remaining tails: 193 bytes per ordinary row, 224 bytes in the final row.
; Loading aliases this entire array into DS:043E..3698 resource input storage.
; File offset in a refill = DS offset - 043Eh; scene reset clears the storage.
    times TERRAIN_SIDE*OCCLUSION_ROW_STRIDE db 0

; key_down: 128 scan-state bytes, with exactly FFh recognized as pressed. The key-wait scan at CS:19B6 searches only indices 00h..7Ch in increasing order, consumes the first FFh by clearing it, and handles Escape specially. Binding collection reads states without clearing them. Keyboard aliases 7Eh (left) and 7Dh (right) feed input_buttons bits 0/1; those two indices are outside the key-wait scan. Their physical key identities remain unconfirmed. The mouse driver callback writes input_buttons directly, without modifying those keyboard aliases. [load+0xadbd]
    times KEY_SCAN_COUNT db KEY_RELEASED ; indexed directly by scan code 00h..7Fh

; generation_validation_ring: 256-byte wrap-indexed validation workspace. CS:4142 stores (packed_BCD_random-reference_byte+object_stack_links[0]) modulo 256 and increments DL. Startup compares reference indices AAh..80h (43 outputs); completion uses A5h..80h (38). Indices 84h..81h address the four entered secret-code bytes. Before occlusion tracing, CS:2E95 requires four 7Fh bytes at seed+29h down to seed+26h and CS:2E9F requires seed+25h to differ. The preceding mismatch corresponds to the FFh terminator after the entered BCD code; a valid match adds the Sentinel support link 7Fh (pedestal slot 3Fh plus flag 40h). Ring indexing wraps modulo 256. [load+0xae3d]
    times 256 db 0

; overview_pending: Successful F1 overview request sets 80h. The main controller suspends game updates, renders the displaced camera view, waits for a fresh key/click, clears this flag and returns to the normal view. [load+0xaf3d]
    db 0x00

; overview_camera_slot: Temporary camera object slot saved by the overview renderer at 1D7Fh. It receives a viewpoint displaced from the player using ray components and becomes view_object_slot for the overview; this is separate from player_robot_slot. [load+0xaf3e]
    db 0x00

; input_poll_delay_ticks: Initial input-poll delay in timer ticks: player_poll_actions sets 2; the timer decrements a nonzero value and polls when it reaches zero. Once zero, subsequent eligible timer calls poll immediately. [load+0xaf3f]
    db 0x00

; mouse_activation_click_latch: Suppresses absorbing on the same left-button press that enabled mouse cursor mode. Activation sets 80h; observing a left-button release clears it; only a later left press with this latch clear produces ACTION_ABSORB. Right-button transfer has its own path. [load+0xaf40]
    db 0x00

; mouse_cursor_mode_flags: Mouse-driven cursor-mode latch, separate from cursor_mode_flags. A left click enables it; keyboard cursor toggling or directional input clears it. Input collection uses it to choose mouse movement/button handling, and player_poll_actions restores cursor mode while this latch is active. [load+0xaf41]
    db 0x00
    db 0x00 ; g data +0xaf42

; mouse_axis_priority: Mouse corner-edge arbitration selector, read without modification by this helper. The helper tests the sign of uint8(value-2) in CL without changing storage. Values 00h..01h or 82h..FFh prefer vertical edges; 02h..81h preserve an existing horizontal request. Vertical requests still obey pitch limits. The CGA loss trace reads zero 1,889 times (two cursor activations and 1,887 movement updates), so the local selector is FEh. Native tests cover all 256 values at all four corners and six controls. Workspace reset clears it; an independent nonzero writer remains unconfirmed. [load+0xaf43]
    db 0x00

; cursor_update_guard_flags: Software-cursor update guard. Rendering/action paths set bit 7 around cursor restore/draw work; interrupt-driven cursor input checks the sign and defers updates while guarded. The action cleanup at 30C5h clears the guard. [load+0xaf44]
    db 0x00

; software_cursor_x_pairs: Cursor draw X coordinate in two-pixel groups. Backends divide by 2 for CGA bytes, use directly for Tandy bytes, divide by 4 for EGA bytes, or multiply by 2 for VGA bytes. [load+0xaf45]
    db 0x00

; software_cursor_inverted_y: Cursor draw vertical anchor: backends use 199 minus this byte as the initial screen row before applying the cursor path deltas. [load+0xaf46]
    db 0x00

; video_transition_busy_flags: Video transition exclusion byte. CGA/Tandy/VGA transition paths write FFh while changing display/page state and zero on completion of that guarded section. All four sentry warning-bar backends skip their drawing while the sign bit is set. [load+0xaf47]
    db 0x00

; generation_reference_prefix: Zero reference at index 80h, immediately before number_entry_buffer. The validation loop reads it last; its match becomes bit zero, outside the accepted-code mask 1Eh. It lies just beyond the shared resource/reset buffer. [load+0xaf48]
    db 0x00

; number_entry_buffer: eight reused bytes: input_decimal_digits fills spaces and stores reversed ASCII digits (ones first); pack_entered_digits_to_bcd overwrites bytes 0..3 with low-pair-first BCD and byte 4 with FFh [load+0xaf49]
    times INPUT_DIGIT_CAPACITY db 0 ; load-time storage; each input prompt initializes ASCII spaces
; Input mode: [ones,tens,hundreds,...]. Packed mode: [BCD pair 0..3,FFh,remaining bytes].

; generation_reference_zero_run: Eight initialized zero references at validation indices 89h..90h, between number_entry_buffer and game_timer_suspend_flags. Both generation runs in the full-level trace read all eight as zero. They participate in the extended BCD/reference sequence, outside the four secret-code comparisons selected by mask 1Eh. [load+0xaf51]
    times 8 db 0

; game_timer_suspend_flags: Bit 7 suspends foreground-game timer dispatch after sound updates. Workspace reset and special screens set it; the preview/return paths clear it. Distinct from game_pause_flags: this gate protects scene preparation and screen changes. [load+0xaf59]
    db 0x00

; landscape_number_bcd: Saved four-digit landscape number in packed BCD, low decimal pair first. landscape_set_number_and_difficulty writes it, draw_landscape_number_bcd prints the high pair first, and completion adds the BCD player-energy value with decimal carry to select the destination. [load+0xaf5a]
    dw 0x0000

; terrain_visibility_bits: 1024 runtime visibility bits for the 32x32 terrain, consumed at 281Ch; byte=((x>>2)<<4)|(y>>1), mask=80h>>((x&3)*2+(y&1)); zero initially, filled FFh at 3D07h, rebuilt after scene rendering [load+0xaf5c]
    times (32*32)/8 db 0 ; one visibility bit per terrain cell

; cga_text_color_pixels: 16 logical text colors translated by XLAT into a byte of four identical 2-bit CGA pixels; used by the CGA font renderer at 0EC2h [load+0xafdc]
    db 0*0x55 ; logical text color 0h -> CGA pixel index 0, repeated four times
    db 1*0x55 ; logical text color 1h -> CGA pixel index 1, repeated four times
    db 2*0x55 ; logical text color 2h -> CGA pixel index 2, repeated four times
    db 3*0x55 ; logical text color 3h -> CGA pixel index 3, repeated four times
    db 1*0x55 ; logical text color 4h -> CGA pixel index 1, repeated four times
    db 2*0x55 ; logical text color 5h -> CGA pixel index 2, repeated four times
    db 3*0x55 ; logical text color 6h -> CGA pixel index 3, repeated four times
    db 1*0x55 ; logical text color 7h -> CGA pixel index 1, repeated four times
    db 2*0x55 ; logical text color 8h -> CGA pixel index 2, repeated four times
    db 3*0x55 ; logical text color 9h -> CGA pixel index 3, repeated four times
    db 1*0x55 ; logical text color Ah -> CGA pixel index 1, repeated four times
    db 2*0x55 ; logical text color Bh -> CGA pixel index 2, repeated four times
    db 3*0x55 ; logical text color Ch -> CGA pixel index 3, repeated four times
    db 1*0x55 ; logical text color Dh -> CGA pixel index 1, repeated four times
    db 2*0x55 ; logical text color Eh -> CGA pixel index 2, repeated four times
    db 3*0x55 ; logical text color Fh -> CGA pixel index 3, repeated four times

; cga_font_nibble_masks: 16 word masks indexed by four glyph bits at 0F38h/0F58h. Each set source bit selects a nibble covering two adjacent 2-bit CGA pixels; byte order follows the two screen bytes. [load+0xafec]
    dw 0 ; glyph nibble 0000
    dw 0x0f00 ; glyph nibble 0001
    dw 0xf000 ; glyph nibble 0010
    dw 0x0f00 | 0xf000 ; glyph nibble 0011
    dw 0x000f ; glyph nibble 0100
    dw 0x0f00 | 0x000f ; glyph nibble 0101
    dw 0xf000 | 0x000f ; glyph nibble 0110
    dw 0x0f00 | 0xf000 | 0x000f ; glyph nibble 0111
    dw 0x00f0 ; glyph nibble 1000
    dw 0x0f00 | 0x00f0 ; glyph nibble 1001
    dw 0xf000 | 0x00f0 ; glyph nibble 1010
    dw 0x0f00 | 0xf000 | 0x00f0 ; glyph nibble 1011
    dw 0x000f | 0x00f0 ; glyph nibble 1100
    dw 0x0f00 | 0x000f | 0x00f0 ; glyph nibble 1101
    dw 0xf000 | 0x000f | 0x00f0 ; glyph nibble 1110
    dw 0x0f00 | 0xf000 | 0x000f | 0x00f0 ; glyph nibble 1111

; video_crtc_port: CRTC index port cached from BIOS data area 0040:0063 after a video-mode change [load+0xb00c]
    dw 0x0000

; text_attribute: BIOS text output attribute byte; distinct from the graphics formatter palette words [load+0xb00e]
    db 0x07

; text_cursor_row: Cached BIOS page-zero text cursor row [load+0xb00f]
    db 0x00

; text_cursor_column: Cached BIOS page-zero text cursor column [load+0xb010]
    db 0x00
    db 0x00 ; g data +0xb011

; glyph_pair_terrain_cells: four terrain-cell codes selected by each pair of font bits in game_draw_character; all use height 2; bit pair 00 leaves no glyph block, while 01/10/11 select the corresponding MODEL_TEXT_BLOCK model [load+0xb012]
    db (TERRAIN_GLYPH_HEIGHT << 4) | 0 ; font bit pair 00
    db (TERRAIN_GLYPH_HEIGHT << 4) | MODEL_TEXT_BLOCK_01 ; font bit pair 01
    db (TERRAIN_GLYPH_HEIGHT << 4) | MODEL_TEXT_BLOCK_10 ; font bit pair 10
    db (TERRAIN_GLYPH_HEIGHT << 4) | MODEL_TEXT_BLOCK_11 ; font bit pair 11

; title_secret_screen_flags: Saved AL argument to draw_title_or_secret_code_screen. Bit 7 selects the secret-code variant and calls the code-digit emitter at 15CEh; a clear bit draws THE SENTRY title from title_banner_markup. [load+0xb016]
    db 0x00

; bcd_random_digit_work: Reused local scratch in prng_packed_decimal_byte: initially a four-iteration PRNG countdown, then the normalized low decimal digit (0..9), finally ORed with the separately normalized high nibble. Five PRNG bytes produce each packed-BCD output. [load+0xb017]
    db 0x00

; title_banner_markup: 15 bytes of THE SENTRY title text and drawing controls [load+0xb018]
    db 0x88, 0xd5, 'THE', 0x80, 0xc7, ' SENTRY '
    db 0x00 ; g data +0xb027

; text_inline_control_bytes_left: Inline-control wrapper at 1736h forwards the incoming 19h opcode, then five parameters through LODSB; each forwarded byte decrements this counter. It resets to six after the complete opcode-plus-parameters sequence. [load+0xb028]
    db 0x06
    db 0x00 ; g data +0xb029

; preview_view_y_offsets: presets 0..2: signed grid-Y offsets added to object slot 2 to position slot 1 [load+0xb02a]
    db 5, 7, 7
    db 0x00 ; g data +0xb02d

; preview_view_pitch_values: presets 0..2: signed pitch bytes written to viewing object slot 2 [load+0xb02e]
    db -12, -5, -5

; preview_view_height_offsets: presets 0..2: signed integer heights above object slot 2; fractional heights are cleared [load+0xb031]
    db 0, 1, 0

; preview_view_heading_values: presets 0..2: slot-1 headings, 256 angular units per full turn [load+0xb034]
    db 128 ; preset 0: 180 degrees
    db 142 ; preset 1: 199.688 degrees
    db 206 ; preset 2: 289.688 degrees

; preview_view_companion_heading_values: presets 0..2: slot-2 headings, 256 angular units per full turn [load+0xb037]
    db   0 ; preset 0: 0 degrees
    db 248 ; preset 1: 348.75 degrees
    db 248 ; preset 2: 348.75 degrees
    db 0x00, 0x00, 0xa9 ; g data +0xb03a

; terrain_generation_saved_noise: Saved initial noise byte: the averaging path ends with SI=32 and copies scratch[32+2Eh] (byte 78) to [SI+376Dh], hence DS:378D. That scratch byte was PRNG-seeded at CS:2A6D and lies beyond the 35 bytes reloaded for each scanline. Nonlinear filtering leaves this destination alone. Native indexed write verified. Confirmed readers: terrain text at CS:13CB compares it with DS:148A before conditionally advancing the PRNG; sentry sight setup at CS:455E derives a comparison limit from it. The full-level trace records 256 zero writes, 3,301 zero sentry reads and 30 zero text reads; the purpose of the coupling remains unconfirmed. [load+0xb03d]
    times 1 db 0

; game_text_pending_opcode: Byte-oriented formatter state at CS:1E96. Pending opcodes 11h/12h/19h/1Fh consume 1/2/5/2 following bytes respectively. While pending, those bytes are parameters even when equal to ordinary control codes. Script-level FFh termination is checked by the outer CS:16F7 loop before calling this formatter. [load+0xb03e]
    db 0x00

; game_text_parameter_index: One-based parameter ordinal, reset to zero when a formatter opcode is accepted. Control 12h ignores parameter one and uses parameter two as a signed foreground/background selector. Control 19h ignores parameters one through four and uses parameter five as the font-row shift boolean. Completion clears the pending opcode, not this ordinal. [load+0xb03f]
    db 0x00

; game_text_alternate_mode: Word-wide flag reads use mask 80h; controls 04h/05h write only its low byte as 00h/FFh. The following initialized zero is the high byte included in those reads, not a separately interpreted control parameter. [load+0xb040]
    dw 0x0004

; game_text_transparent: Word argument to the font callback; controls 04h/05h change the low byte to 00h/FFh [load+0xb042]
    dw 0x0000

; game_text_glyph_flags: Font-row shift boolean in the low byte of a word argument. The fifth byte of control 19h sets it; the first four parameter bytes are consumed without changing formatter state. Each graphics font backend decrements the glyph source pointer once when nonzero, drawing the preceding stored row plus the first seven glyph rows. This is a one-row source shift, not a mask of independent glyph features. [load+0xb044]
    dw 0x0000

; game_text_normal_foreground: Foreground palette index selected by control 04h [load+0xb046]
    dw 0x000f

; game_text_normal_background: Background palette index selected by control 04h and updated by control 11h [load+0xb048]
    dw 0x0000

; game_text_alternate_foreground: Alternate foreground; nonnegative second parameter of control 12h updates this word [load+0xb04a]
    dw 0x000c

; game_text_alternate_background: Alternate background; negative second parameter of control 12h supplies a seven-bit palette index [load+0xb04c]
    dw 0x000e

; game_text_active_foreground: Current foreground copied from the selected normal/alternate style [load+0xb04e]
    dw 0x000c

; game_text_active_background: Current background copied from the selected normal/alternate style [load+0xb050]
    dw 0x000e

; game_text_column: Graphics text X in half-character units: even values 0..38 represent the 20 doubled-width glyph columns. A printable character advances by two; tab advances by two with X wrap but no row advance, while printable wrap also advances the row. Control 1Fh doubles its first byte before sign extension; this is not a BIOS 40-column cursor. [load+0xb052]
    dw 0x0000

; game_text_row: Graphics text row, with font callback scanline origin 8+8*row to leave the top eight pixels for the HUD. The formatter cycles 0..24; row 23 starts at screen line 192, while row 24 starts at 200 beyond the visible 200-line image. Control 1Fh sign-extends its second byte into this word; newline advances Y without resetting X. [load+0xb054]
    dw 0x0000

; game_text_input_character: Saved incoming byte while the formatter interprets control parameters [load+0xb056]
    db 0x00
    db 0x00 ; g data +0xb057

; game_text_script_offsets: 18 one-byte offsets relative to game_text_script_data; indexed by draw_game_text_script (16F7h) [load+0xb058]
    db text_script_title_press_any_key-game_text_script_data ; script 0
    db text_script_landscape_number_prompt-game_text_script_data ; script 1
    db text_script_secret_entry_prompt-game_text_script_data ; script 2
    db text_script_wrong_secret_code-game_text_script_data ; script 3
    db text_script_landscape_preview_caption-game_text_script_data ; script 4
    db text_script_landscape_number_caption-game_text_script_data ; script 5
    db text_script_reposition_press_any_key-game_text_script_data ; script 6
    db text_script_position_row1-game_text_script_data ; script 7
    db text_script_position_row3-game_text_script_data ; script 8
    db text_script_position_row23-game_text_script_data ; script 9
    db text_script_text_style_title-game_text_script_data ; script 10
    db text_script_text_style_preview-game_text_script_data ; script 11
    db text_script_position_row22-game_text_script_data ; script 12
    db text_script_word_landscape-game_text_script_data ; script 13
    db text_script_words_secret_entry_code-game_text_script_data ; script 14
    db text_script_blank_five_characters-game_text_script_data ; script 15
    db text_script_blank_three_characters-game_text_script_data ; script 16
    db text_script_words_press_any_key-game_text_script_data ; script 17

; text_character_patch_a: First printable-character patch in the mutable 23-byte reverse-played text-control buffer; the 1710h wrapper patches this byte and DS:37C5 before playing offsets 22 down to zero. [load+0xb06a]
    db 'C' ; runtime replaces this character

; text_color_a_parameter: mutable color parameter in the reverse-played per-character control buffer; filled from the matching scene text-color preset table [load+0xb06b]
    db TEXT_COLOR_INDEX_2

; text_character_controls_a: Reverse-played fragment following color A: second character patch, backspace, opcode 19h with parameters 0/0/0/4/0, then opcode 12h. The physical 04h is a parameter, not a normal-style opcode. [load+0xb06c]
    db 0, TEXT_CTL_ALT_COLOR ; reverse playback consumes the preceding color-A byte
    db 0, 4, 0, 0, 0, TEXT_CTL_GLYPH ; opcode 19h followed by five parameters when reversed
    db TEXT_CTL_BACKSPACE, 'C' ; second character patch, then backspace

; text_color_b_parameter: mutable color parameter in the reverse-played per-character control buffer; filled from the matching scene text-color preset table [load+0xb076]
    db TEXT_COLOR_INDEX_3

; text_character_controls_b: Reverse-played fragment following color B: initial space and deletion control, opcode 19h with parameters 0/0/0/FCh/FFh, then opcode 12h. FFh is consumed as a glyph-flag parameter, not a script terminator. [load+0xb077]
    db 0, TEXT_CTL_ALT_COLOR ; reverse playback consumes the preceding color-B byte
    db TEXT_GLYPH_USE_PREVIOUS_ROW, 0xfc, 0, 0, 0, TEXT_CTL_GLYPH ; five parameters in reverse order
    db TEXT_CTL_DELETE, ' ' ; playback begins with space, then deletion

; text_script_title_press_any_key: script 0; named nested calls, text and drawing controls [load+0xb081]
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_TEXT_STYLE_TITLE
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_POSITION_ROW22
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_WORDS_PRESS_ANY_KEY
    db 0x11 ; drawing control/parameter byte
    db 0x80 ; drawing control/parameter byte
    db TEXT_SCRIPT_END

; text_script_landscape_number_prompt: script 1; named nested calls, text and drawing controls [load+0xb087]
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_POSITION_ROW22
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_BLANK_FIVE_CHARACTERS
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_BLANK_FIVE_CHARACTERS
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_BLANK_THREE_CHARACTERS
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_POSITION_ROW1
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_WORD_LANDSCAPE
    db 0x09 ; drawing control/parameter byte
    db 'NUMBER?'
    db 0x04 ; drawing control/parameter byte
    db 0x1f ; drawing control/parameter byte
    db 0x05 ; drawing control/parameter byte
    db 0x16 ; drawing control/parameter byte
    db TEXT_SCRIPT_END

; text_script_secret_entry_prompt: script 2; named nested calls, text and drawing controls [load+0xb09a]
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_TEXT_STYLE_TITLE
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_POSITION_ROW1
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_WORDS_SECRET_ENTRY_CODE
    db '?'
    db 0x04 ; drawing control/parameter byte
    db 0x1f ; drawing control/parameter byte
    db 0x03 ; drawing control/parameter byte
    db 0x16 ; drawing control/parameter byte
    db TEXT_SCRIPT_END

; text_script_wrong_secret_code: script 3; named nested calls, text and drawing controls [load+0xb0a3]
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_TEXT_STYLE_TITLE
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_POSITION_ROW1
    db 'WRONG'
    db 0x09 ; drawing control/parameter byte
    db 'SECRET'
    db 0x09 ; drawing control/parameter byte
    db 'CODE'
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_TITLE_PRESS_ANY_KEY
    db TEXT_SCRIPT_END

; text_script_landscape_preview_caption: script 4; named nested calls, text and drawing controls [load+0xb0b8]
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_TEXT_STYLE_PREVIEW
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_POSITION_ROW23
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_WORDS_PRESS_ANY_KEY
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_POSITION_ROW1
    db 0x09 ; drawing control/parameter byte
    db 0x09 ; drawing control/parameter byte
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_WORD_LANDSCAPE
    db 0x09 ; drawing control/parameter byte
    db TEXT_SCRIPT_END

; text_script_landscape_number_caption: script 5; named nested calls, text and drawing controls [load+0xb0c1]
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_POSITION_ROW1
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_WORDS_SECRET_ENTRY_CODE
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_POSITION_ROW3
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_WORD_LANDSCAPE
    db 0x09 ; drawing control/parameter byte
    db TEXT_SCRIPT_END

; text_script_reposition_press_any_key: script 6; named nested calls, text and drawing controls [load+0xb0c7]
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_POSITION_ROW22
    db TEXT_SCRIPT_CALL_BASE+TEXT_SCRIPT_WORDS_PRESS_ANY_KEY
    db TEXT_SCRIPT_END

; text_script_position_row1: script 7; named nested calls, text and drawing controls [load+0xb0ca]
    db 0x1f ; drawing control/parameter byte
    db 0x01 ; drawing control/parameter byte
    db 0x01 ; drawing control/parameter byte
    db TEXT_SCRIPT_END
    db 0x00, 0x00, 0x00 ; g data +0xb0ce

; text_script_position_row3: script 8; named nested calls, text and drawing controls [load+0xb0d1]
    db 0x1f ; drawing control/parameter byte
    db 0x03 ; drawing control/parameter byte
    db 0x03 ; drawing control/parameter byte
    db TEXT_SCRIPT_END
    db 0x00, 0x00, 0x00 ; g data +0xb0d5

; text_script_position_row23: script 9; named nested calls, text and drawing controls [load+0xb0d8]
    db 0x1f ; drawing control/parameter byte
    db 0x03 ; drawing control/parameter byte
    db 0x17 ; drawing control/parameter byte
    db TEXT_SCRIPT_END
    db 0x00, 0x00, 0x00 ; g data +0xb0dc

; text_script_text_style_title: script 10; named nested calls, text and drawing controls [load+0xb0df]
    db 0x05 ; drawing control/parameter byte
    db 0x12 ; drawing control/parameter byte
    db 0x00 ; drawing control/parameter byte
    db 0x80 ; drawing control/parameter byte
    db TEXT_SCRIPT_END

; text_script_text_style_preview: script 11; named nested calls, text and drawing controls [load+0xb0e4]
    db 0x05 ; drawing control/parameter byte
    db 0x12 ; drawing control/parameter byte
    db 0x00 ; drawing control/parameter byte
    db 0x80 ; drawing control/parameter byte
    db TEXT_SCRIPT_END

; text_script_position_row22: script 12; named nested calls, text and drawing controls [load+0xb0e9]
    db 0x1f ; drawing control/parameter byte
    db 0x01 ; drawing control/parameter byte
    db 0x16 ; drawing control/parameter byte
    db TEXT_SCRIPT_END
    db 0x00, 0x00, 0x00 ; g data +0xb0ed

; text_script_word_landscape: script 13; named nested calls, text and drawing controls [load+0xb0f0]
    db 'LANDSCAPE'
    db TEXT_SCRIPT_END

; text_script_words_secret_entry_code: script 14; named nested calls, text and drawing controls [load+0xb0fa]
    db 'SECRET'
    db 0x09 ; drawing control/parameter byte
    db 'ENTRY'
    db 0x09 ; drawing control/parameter byte
    db 'CODE'
    db TEXT_SCRIPT_END

; text_script_blank_five_characters: script 15; named nested calls, text and drawing controls [load+0xb10c]
    db 0x09 ; drawing control/parameter byte
    db 0x09 ; drawing control/parameter byte
    db 0x09 ; drawing control/parameter byte
    db 0x09 ; drawing control/parameter byte
    db 0x09 ; drawing control/parameter byte
    db TEXT_SCRIPT_END

; text_script_blank_three_characters: script 16; named nested calls, text and drawing controls [load+0xb112]
    db 0x09 ; drawing control/parameter byte
    db 0x09 ; drawing control/parameter byte
    db 0x09 ; drawing control/parameter byte
    db TEXT_SCRIPT_END

; text_script_words_press_any_key: script 17; named nested calls, text and drawing controls [load+0xb116]
    db 'PRESS'
    db 0x09 ; drawing control/parameter byte
    db 'ANY'
    db 0x09 ; drawing control/parameter byte
    db 'KEY'
    db TEXT_SCRIPT_END
    db 0x00, 0x00 ; g data +0xb124

; game_font_8x8: 64 stored 8x8 glyphs for codes 20h..5Fh (uppercase, digits and punctuation). Graphics font callbacks double glyph pixels horizontally to 16x8 screen pixels. They compute 8*(code-20h) without a glyph-count bound; codes 60h..7Eh would read the following atan table as glyph rows. A nonzero row-shift argument also reads the preceding source byte. [load+0xb126]
    db 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 ; glyph 20h
    db 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b00000000, 0b00011000, 0b00000000 ; glyph 21h
    db 0b01101100, 0b01101100, 0b01101100, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 ; glyph 22h
    db 0b00110110, 0b00110110, 0b01111111, 0b00110110, 0b01111111, 0b00110110, 0b00110110, 0b00000000 ; glyph 23h
    db 0b00001100, 0b00111111, 0b01101000, 0b00111110, 0b00001011, 0b01111110, 0b00011000, 0b00000000 ; glyph 24h
    db 0b01100000, 0b01100110, 0b00001100, 0b00011000, 0b00110000, 0b01100110, 0b00000110, 0b00000000 ; glyph 25h
    db 0b00111000, 0b01101100, 0b01101100, 0b00111000, 0b01101101, 0b01100110, 0b00111011, 0b00000000 ; glyph 26h
    db 0b00001100, 0b00011000, 0b00110000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000 ; glyph 27h
    db 0b00001100, 0b00011000, 0b00110000, 0b00110000, 0b00110000, 0b00011000, 0b00001100, 0b00000000 ; glyph 28h
    db 0b00110000, 0b00011000, 0b00001100, 0b00001100, 0b00001100, 0b00011000, 0b00110000, 0b00000000 ; glyph 29h
    db 0b00000000, 0b00011000, 0b01111110, 0b00111100, 0b01111110, 0b00011000, 0b00000000, 0b00000000 ; glyph 2ah
    db 0b00000000, 0b00011000, 0b00011000, 0b01111110, 0b00011000, 0b00011000, 0b00000000, 0b00000000 ; glyph 2bh
    db 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00011000, 0b00011000, 0b00110000 ; glyph 2ch
    db 0b00000000, 0b00000000, 0b00000000, 0b01111110, 0b00000000, 0b00000000, 0b00000000, 0b00000000 ; glyph 2dh
    db 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00011000, 0b00011000, 0b00000000 ; glyph 2eh
    db 0b00000000, 0b00000110, 0b00001100, 0b00011000, 0b00110000, 0b01100000, 0b00000000, 0b00000000 ; glyph 2fh
    db 0b00111100, 0b01100110, 0b01101110, 0b01111110, 0b01110110, 0b01100110, 0b00111100, 0b00000000 ; glyph 30h
    db 0b00011000, 0b00111000, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b01111110, 0b00000000 ; glyph 31h
    db 0b00111100, 0b01100110, 0b00000110, 0b00001100, 0b00011000, 0b00110000, 0b01111110, 0b00000000 ; glyph 32h
    db 0b00111100, 0b01100110, 0b00000110, 0b00011100, 0b00000110, 0b01100110, 0b00111100, 0b00000000 ; glyph 33h
    db 0b00001100, 0b00011100, 0b00111100, 0b01101100, 0b01111110, 0b00001100, 0b00001100, 0b00000000 ; glyph 34h
    db 0b01111110, 0b01100000, 0b01111100, 0b00000110, 0b00000110, 0b01100110, 0b00111100, 0b00000000 ; glyph 35h
    db 0b00011100, 0b00110000, 0b01100000, 0b01111100, 0b01100110, 0b01100110, 0b00111100, 0b00000000 ; glyph 36h
    db 0b01111110, 0b00000110, 0b00001100, 0b00011000, 0b00110000, 0b00110000, 0b00110000, 0b00000000 ; glyph 37h
    db 0b00111100, 0b01100110, 0b01100110, 0b00111100, 0b01100110, 0b01100110, 0b00111100, 0b00000000 ; glyph 38h
    db 0b00111100, 0b01100110, 0b01100110, 0b00111110, 0b00000110, 0b00001100, 0b00111000, 0b00000000 ; glyph 39h
    db 0b00000000, 0b00000000, 0b00011000, 0b00011000, 0b00000000, 0b00011000, 0b00011000, 0b00000000 ; glyph 3ah
    db 0b00000000, 0b00000000, 0b00011000, 0b00011000, 0b00000000, 0b00011000, 0b00011000, 0b00110000 ; glyph 3bh
    db 0b00001100, 0b00011000, 0b00110000, 0b01100000, 0b00110000, 0b00011000, 0b00001100, 0b00000000 ; glyph 3ch
    db 0b00000000, 0b00000000, 0b01111110, 0b00000000, 0b01111110, 0b00000000, 0b00000000, 0b00000000 ; glyph 3dh
    db 0b00110000, 0b00011000, 0b00001100, 0b00000110, 0b00001100, 0b00011000, 0b00110000, 0b00000000 ; glyph 3eh
    db 0b00111100, 0b01100110, 0b00001100, 0b00011000, 0b00011000, 0b00000000, 0b00011000, 0b00000000 ; glyph 3fh
    db 0b00111100, 0b01100110, 0b01101110, 0b01101010, 0b01101110, 0b01100000, 0b00111100, 0b00000000 ; glyph 40h
    db 0b00111100, 0b01100110, 0b01100110, 0b01111110, 0b01100110, 0b01100110, 0b01100110, 0b00000000 ; glyph 41h
    db 0b01111100, 0b01100110, 0b01100110, 0b01111100, 0b01100110, 0b01100110, 0b01111100, 0b00000000 ; glyph 42h
    db 0b00111100, 0b01100110, 0b01100000, 0b01100000, 0b01100000, 0b01100110, 0b00111100, 0b00000000 ; glyph 43h
    db 0b01111000, 0b01101100, 0b01100110, 0b01100110, 0b01100110, 0b01101100, 0b01111000, 0b00000000 ; glyph 44h
    db 0b01111110, 0b01100000, 0b01100000, 0b01111100, 0b01100000, 0b01100000, 0b01111110, 0b00000000 ; glyph 45h
    db 0b01111110, 0b01100000, 0b01100000, 0b01111100, 0b01100000, 0b01100000, 0b01100000, 0b00000000 ; glyph 46h
    db 0b00111100, 0b01100110, 0b01100000, 0b01101110, 0b01100110, 0b01100110, 0b00111100, 0b00000000 ; glyph 47h
    db 0b01100110, 0b01100110, 0b01100110, 0b01111110, 0b01100110, 0b01100110, 0b01100110, 0b00000000 ; glyph 48h
    db 0b01111110, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b01111110, 0b00000000 ; glyph 49h
    db 0b00111110, 0b00001100, 0b00001100, 0b00001100, 0b00001100, 0b01101100, 0b00111000, 0b00000000 ; glyph 4ah
    db 0b01100110, 0b01101100, 0b01111000, 0b01110000, 0b01111000, 0b01101100, 0b01100110, 0b00000000 ; glyph 4bh
    db 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01111110, 0b00000000 ; glyph 4ch
    db 0b01100011, 0b01110111, 0b01111111, 0b01101011, 0b01101011, 0b01100011, 0b01100011, 0b00000000 ; glyph 4dh
    db 0b01100110, 0b01100110, 0b01110110, 0b01111110, 0b01101110, 0b01100110, 0b01100110, 0b00000000 ; glyph 4eh
    db 0b00111100, 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b00111100, 0b00000000 ; glyph 4fh
    db 0b01111100, 0b01100110, 0b01100110, 0b01111100, 0b01100000, 0b01100000, 0b01100000, 0b00000000 ; glyph 50h
    db 0b00111100, 0b01100110, 0b01100110, 0b01100110, 0b01101010, 0b01101100, 0b00110110, 0b00000000 ; glyph 51h
    db 0b01111100, 0b01100110, 0b01100110, 0b01111100, 0b01101100, 0b01100110, 0b01100110, 0b00000000 ; glyph 52h
    db 0b00111100, 0b01100110, 0b01100000, 0b00111100, 0b00000110, 0b01100110, 0b00111100, 0b00000000 ; glyph 53h
    db 0b01111110, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b00011000, 0b00000000 ; glyph 54h
    db 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b00111100, 0b00000000 ; glyph 55h
    db 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b01100110, 0b00111100, 0b00011000, 0b00000000 ; glyph 56h
    db 0b01100011, 0b01100011, 0b01101011, 0b01101011, 0b01111111, 0b01110111, 0b01100011, 0b00000000 ; glyph 57h
    db 0b01100110, 0b01100110, 0b00111100, 0b00011000, 0b00111100, 0b01100110, 0b01100110, 0b00000000 ; glyph 58h
    db 0b01100110, 0b01100110, 0b01100110, 0b00111100, 0b00011000, 0b00011000, 0b00011000, 0b00000000 ; glyph 59h
    db 0b01111110, 0b00000110, 0b00001100, 0b00011000, 0b00110000, 0b01100000, 0b01111110, 0b00000000 ; glyph 5ah
    db 0b01111100, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01111100, 0b00000000 ; glyph 5bh
    db 0b00000000, 0b01100000, 0b00110000, 0b00011000, 0b00001100, 0b00000110, 0b00000000, 0b00000000 ; glyph 5ch
    db 0b00111110, 0b00000110, 0b00000110, 0b00000110, 0b00000110, 0b00000110, 0b00111110, 0b00000000 ; glyph 5dh
    db 0b00011000, 0b00111100, 0b01100110, 0b01000010, 0b00000000, 0b00000000, 0b00000000, 0b00000000 ; glyph 5eh
    db 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b11111111 ; glyph 5fh

; projection_atan_lut: 2049 first-octant angles: round(atan(i/2048)*32768/pi), i=0..2048. CS:1B0E reads indices 0..2047 from floor((minor/major)*2048). The stored endpoint 2048 is 2000h (45 degrees), but equal axes use the immediate 2000h at CS:1B33 instead of reading it. This is an angular lookup, not a perspective scale table. [load+0xb326]
    dw 0x0000, 0x0005, 0x000a, 0x000f, 0x0014, 0x0019, 0x001f, 0x0024
    dw 0x0029, 0x002e, 0x0033, 0x0038, 0x003d, 0x0042, 0x0047, 0x004c
    dw 0x0051, 0x0057, 0x005c, 0x0061, 0x0066, 0x006b, 0x0070, 0x0075
    dw 0x007a, 0x007f, 0x0084, 0x008a, 0x008f, 0x0094, 0x0099, 0x009e
    dw 0x00a3, 0x00a8, 0x00ad, 0x00b2, 0x00b7, 0x00bc, 0x00c2, 0x00c7
    dw 0x00cc, 0x00d1, 0x00d6, 0x00db, 0x00e0, 0x00e5, 0x00ea, 0x00ef
    dw 0x00f4, 0x00fa, 0x00ff, 0x0104, 0x0109, 0x010e, 0x0113, 0x0118
    dw 0x011d, 0x0122, 0x0127, 0x012c, 0x0131, 0x0137, 0x013c, 0x0141
    dw 0x0146, 0x014b, 0x0150, 0x0155, 0x015a, 0x015f, 0x0164, 0x0169
    dw 0x016f, 0x0174, 0x0179, 0x017e, 0x0183, 0x0188, 0x018d, 0x0192
    dw 0x0197, 0x019c, 0x01a1, 0x01a6, 0x01ac, 0x01b1, 0x01b6, 0x01bb
    dw 0x01c0, 0x01c5, 0x01ca, 0x01cf, 0x01d4, 0x01d9, 0x01de, 0x01e3
    dw 0x01e9, 0x01ee, 0x01f3, 0x01f8, 0x01fd, 0x0202, 0x0207, 0x020c
    dw 0x0211, 0x0216, 0x021b, 0x0220, 0x0226, 0x022b, 0x0230, 0x0235
    dw 0x023a, 0x023f, 0x0244, 0x0249, 0x024e, 0x0253, 0x0258, 0x025d
    dw 0x0262, 0x0268, 0x026d, 0x0272, 0x0277, 0x027c, 0x0281, 0x0286
    dw 0x028b, 0x0290, 0x0295, 0x029a, 0x029f, 0x02a4, 0x02a9, 0x02af
    dw 0x02b4, 0x02b9, 0x02be, 0x02c3, 0x02c8, 0x02cd, 0x02d2, 0x02d7
    dw 0x02dc, 0x02e1, 0x02e6, 0x02eb, 0x02f0, 0x02f6, 0x02fb, 0x0300
    dw 0x0305, 0x030a, 0x030f, 0x0314, 0x0319, 0x031e, 0x0323, 0x0328
    dw 0x032d, 0x0332, 0x0337, 0x033c, 0x0341, 0x0347, 0x034c, 0x0351
    dw 0x0356, 0x035b, 0x0360, 0x0365, 0x036a, 0x036f, 0x0374, 0x0379
    dw 0x037e, 0x0383, 0x0388, 0x038d, 0x0392, 0x0397, 0x039c, 0x03a2
    dw 0x03a7, 0x03ac, 0x03b1, 0x03b6, 0x03bb, 0x03c0, 0x03c5, 0x03ca
    dw 0x03cf, 0x03d4, 0x03d9, 0x03de, 0x03e3, 0x03e8, 0x03ed, 0x03f2
    dw 0x03f7, 0x03fc, 0x0401, 0x0407, 0x040c, 0x0411, 0x0416, 0x041b
    dw 0x0420, 0x0425, 0x042a, 0x042f, 0x0434, 0x0439, 0x043e, 0x0443
    dw 0x0448, 0x044d, 0x0452, 0x0457, 0x045c, 0x0461, 0x0466, 0x046b
    dw 0x0470, 0x0475, 0x047a, 0x047f, 0x0484, 0x0489, 0x048e, 0x0494
    dw 0x0499, 0x049e, 0x04a3, 0x04a8, 0x04ad, 0x04b2, 0x04b7, 0x04bc
    dw 0x04c1, 0x04c6, 0x04cb, 0x04d0, 0x04d5, 0x04da, 0x04df, 0x04e4
    dw 0x04e9, 0x04ee, 0x04f3, 0x04f8, 0x04fd, 0x0502, 0x0507, 0x050c
    dw 0x0511, 0x0516, 0x051b, 0x0520, 0x0525, 0x052a, 0x052f, 0x0534
    dw 0x0539, 0x053e, 0x0543, 0x0548, 0x054d, 0x0552, 0x0557, 0x055c
    dw 0x0561, 0x0566, 0x056b, 0x0570, 0x0575, 0x057a, 0x057f, 0x0584
    dw 0x0589, 0x058e, 0x0593, 0x0598, 0x059d, 0x05a2, 0x05a7, 0x05ac
    dw 0x05b1, 0x05b6, 0x05bb, 0x05c0, 0x05c5, 0x05ca, 0x05cf, 0x05d4
    dw 0x05d9, 0x05de, 0x05e3, 0x05e8, 0x05ed, 0x05f2, 0x05f7, 0x05fc
    dw 0x0601, 0x0606, 0x060b, 0x0610, 0x0615, 0x061a, 0x061f, 0x0624
    dw 0x0629, 0x062e, 0x0633, 0x0638, 0x063d, 0x0642, 0x0647, 0x064c
    dw 0x0651, 0x0656, 0x065b, 0x0660, 0x0665, 0x066a, 0x066e, 0x0673
    dw 0x0678, 0x067d, 0x0682, 0x0687, 0x068c, 0x0691, 0x0696, 0x069b
    dw 0x06a0, 0x06a5, 0x06aa, 0x06af, 0x06b4, 0x06b9, 0x06be, 0x06c3
    dw 0x06c8, 0x06cd, 0x06d2, 0x06d7, 0x06dc, 0x06e1, 0x06e5, 0x06ea
    dw 0x06ef, 0x06f4, 0x06f9, 0x06fe, 0x0703, 0x0708, 0x070d, 0x0712
    dw 0x0717, 0x071c, 0x0721, 0x0726, 0x072b, 0x0730, 0x0735, 0x0739
    dw 0x073e, 0x0743, 0x0748, 0x074d, 0x0752, 0x0757, 0x075c, 0x0761
    dw 0x0766, 0x076b, 0x0770, 0x0775, 0x077a, 0x077e, 0x0783, 0x0788
    dw 0x078d, 0x0792, 0x0797, 0x079c, 0x07a1, 0x07a6, 0x07ab, 0x07b0
    dw 0x07b5, 0x07b9, 0x07be, 0x07c3, 0x07c8, 0x07cd, 0x07d2, 0x07d7
    dw 0x07dc, 0x07e1, 0x07e6, 0x07eb, 0x07ef, 0x07f4, 0x07f9, 0x07fe
    dw 0x0803, 0x0808, 0x080d, 0x0812, 0x0817, 0x081c, 0x0820, 0x0825
    dw 0x082a, 0x082f, 0x0834, 0x0839, 0x083e, 0x0843, 0x0848, 0x084c
    dw 0x0851, 0x0856, 0x085b, 0x0860, 0x0865, 0x086a, 0x086f, 0x0873
    dw 0x0878, 0x087d, 0x0882, 0x0887, 0x088c, 0x0891, 0x0896, 0x089a
    dw 0x089f, 0x08a4, 0x08a9, 0x08ae, 0x08b3, 0x08b8, 0x08bd, 0x08c1
    dw 0x08c6, 0x08cb, 0x08d0, 0x08d5, 0x08da, 0x08df, 0x08e3, 0x08e8
    dw 0x08ed, 0x08f2, 0x08f7, 0x08fc, 0x0901, 0x0905, 0x090a, 0x090f
    dw 0x0914, 0x0919, 0x091e, 0x0922, 0x0927, 0x092c, 0x0931, 0x0936
    dw 0x093b, 0x093f, 0x0944, 0x0949, 0x094e, 0x0953, 0x0958, 0x095c
    dw 0x0961, 0x0966, 0x096b, 0x0970, 0x0975, 0x0979, 0x097e, 0x0983
    dw 0x0988, 0x098d, 0x0992, 0x0996, 0x099b, 0x09a0, 0x09a5, 0x09aa
    dw 0x09ae, 0x09b3, 0x09b8, 0x09bd, 0x09c2, 0x09c6, 0x09cb, 0x09d0
    dw 0x09d5, 0x09da, 0x09de, 0x09e3, 0x09e8, 0x09ed, 0x09f2, 0x09f6
    dw 0x09fb, 0x0a00, 0x0a05, 0x0a0a, 0x0a0e, 0x0a13, 0x0a18, 0x0a1d
    dw 0x0a22, 0x0a26, 0x0a2b, 0x0a30, 0x0a35, 0x0a39, 0x0a3e, 0x0a43
    dw 0x0a48, 0x0a4d, 0x0a51, 0x0a56, 0x0a5b, 0x0a60, 0x0a64, 0x0a69
    dw 0x0a6e, 0x0a73, 0x0a77, 0x0a7c, 0x0a81, 0x0a86, 0x0a8b, 0x0a8f
    dw 0x0a94, 0x0a99, 0x0a9e, 0x0aa2, 0x0aa7, 0x0aac, 0x0ab1, 0x0ab5
    dw 0x0aba, 0x0abf, 0x0ac4, 0x0ac8, 0x0acd, 0x0ad2, 0x0ad7, 0x0adb
    dw 0x0ae0, 0x0ae5, 0x0ae9, 0x0aee, 0x0af3, 0x0af8, 0x0afc, 0x0b01
    dw 0x0b06, 0x0b0b, 0x0b0f, 0x0b14, 0x0b19, 0x0b1e, 0x0b22, 0x0b27
    dw 0x0b2c, 0x0b30, 0x0b35, 0x0b3a, 0x0b3f, 0x0b43, 0x0b48, 0x0b4d
    dw 0x0b51, 0x0b56, 0x0b5b, 0x0b60, 0x0b64, 0x0b69, 0x0b6e, 0x0b72
    dw 0x0b77, 0x0b7c, 0x0b80, 0x0b85, 0x0b8a, 0x0b8f, 0x0b93, 0x0b98
    dw 0x0b9d, 0x0ba1, 0x0ba6, 0x0bab, 0x0baf, 0x0bb4, 0x0bb9, 0x0bbd
    dw 0x0bc2, 0x0bc7, 0x0bcb, 0x0bd0, 0x0bd5, 0x0bd9, 0x0bde, 0x0be3
    dw 0x0be7, 0x0bec, 0x0bf1, 0x0bf5, 0x0bfa, 0x0bff, 0x0c03, 0x0c08
    dw 0x0c0d, 0x0c11, 0x0c16, 0x0c1b, 0x0c1f, 0x0c24, 0x0c29, 0x0c2d
    dw 0x0c32, 0x0c37, 0x0c3b, 0x0c40, 0x0c45, 0x0c49, 0x0c4e, 0x0c53
    dw 0x0c57, 0x0c5c, 0x0c60, 0x0c65, 0x0c6a, 0x0c6e, 0x0c73, 0x0c78
    dw 0x0c7c, 0x0c81, 0x0c86, 0x0c8a, 0x0c8f, 0x0c93, 0x0c98, 0x0c9d
    dw 0x0ca1, 0x0ca6, 0x0cab, 0x0caf, 0x0cb4, 0x0cb8, 0x0cbd, 0x0cc2
    dw 0x0cc6, 0x0ccb, 0x0ccf, 0x0cd4, 0x0cd9, 0x0cdd, 0x0ce2, 0x0ce6
    dw 0x0ceb, 0x0cf0, 0x0cf4, 0x0cf9, 0x0cfd, 0x0d02, 0x0d07, 0x0d0b
    dw 0x0d10, 0x0d14, 0x0d19, 0x0d1e, 0x0d22, 0x0d27, 0x0d2b, 0x0d30
    dw 0x0d34, 0x0d39, 0x0d3e, 0x0d42, 0x0d47, 0x0d4b, 0x0d50, 0x0d54
    dw 0x0d59, 0x0d5e, 0x0d62, 0x0d67, 0x0d6b, 0x0d70, 0x0d74, 0x0d79
    dw 0x0d7d, 0x0d82, 0x0d87, 0x0d8b, 0x0d90, 0x0d94, 0x0d99, 0x0d9d
    dw 0x0da2, 0x0da6, 0x0dab, 0x0daf, 0x0db4, 0x0db9, 0x0dbd, 0x0dc2
    dw 0x0dc6, 0x0dcb, 0x0dcf, 0x0dd4, 0x0dd8, 0x0ddd, 0x0de1, 0x0de6
    dw 0x0dea, 0x0def, 0x0df3, 0x0df8, 0x0dfc, 0x0e01, 0x0e05, 0x0e0a
    dw 0x0e0f, 0x0e13, 0x0e18, 0x0e1c, 0x0e21, 0x0e25, 0x0e2a, 0x0e2e
    dw 0x0e33, 0x0e37, 0x0e3c, 0x0e40, 0x0e45, 0x0e49, 0x0e4e, 0x0e52
    dw 0x0e56, 0x0e5b, 0x0e5f, 0x0e64, 0x0e68, 0x0e6d, 0x0e71, 0x0e76
    dw 0x0e7a, 0x0e7f, 0x0e83, 0x0e88, 0x0e8c, 0x0e91, 0x0e95, 0x0e9a
    dw 0x0e9e, 0x0ea3, 0x0ea7, 0x0eac, 0x0eb0, 0x0eb4, 0x0eb9, 0x0ebd
    dw 0x0ec2, 0x0ec6, 0x0ecb, 0x0ecf, 0x0ed4, 0x0ed8, 0x0edc, 0x0ee1
    dw 0x0ee5, 0x0eea, 0x0eee, 0x0ef3, 0x0ef7, 0x0efc, 0x0f00, 0x0f04
    dw 0x0f09, 0x0f0d, 0x0f12, 0x0f16, 0x0f1b, 0x0f1f, 0x0f23, 0x0f28
    dw 0x0f2c, 0x0f31, 0x0f35, 0x0f3a, 0x0f3e, 0x0f42, 0x0f47, 0x0f4b
    dw 0x0f50, 0x0f54, 0x0f58, 0x0f5d, 0x0f61, 0x0f66, 0x0f6a, 0x0f6e
    dw 0x0f73, 0x0f77, 0x0f7c, 0x0f80, 0x0f84, 0x0f89, 0x0f8d, 0x0f91
    dw 0x0f96, 0x0f9a, 0x0f9f, 0x0fa3, 0x0fa7, 0x0fac, 0x0fb0, 0x0fb5
    dw 0x0fb9, 0x0fbd, 0x0fc2, 0x0fc6, 0x0fca, 0x0fcf, 0x0fd3, 0x0fd7
    dw 0x0fdc, 0x0fe0, 0x0fe5, 0x0fe9, 0x0fed, 0x0ff2, 0x0ff6, 0x0ffa
    dw 0x0fff, 0x1003, 0x1007, 0x100c, 0x1010, 0x1014, 0x1019, 0x101d
    dw 0x1021, 0x1026, 0x102a, 0x102e, 0x1033, 0x1037, 0x103b, 0x1040
    dw 0x1044, 0x1048, 0x104d, 0x1051, 0x1055, 0x105a, 0x105e, 0x1062
    dw 0x1067, 0x106b, 0x106f, 0x1073, 0x1078, 0x107c, 0x1080, 0x1085
    dw 0x1089, 0x108d, 0x1092, 0x1096, 0x109a, 0x109e, 0x10a3, 0x10a7
    dw 0x10ab, 0x10b0, 0x10b4, 0x10b8, 0x10bc, 0x10c1, 0x10c5, 0x10c9
    dw 0x10ce, 0x10d2, 0x10d6, 0x10da, 0x10df, 0x10e3, 0x10e7, 0x10eb
    dw 0x10f0, 0x10f4, 0x10f8, 0x10fd, 0x1101, 0x1105, 0x1109, 0x110e
    dw 0x1112, 0x1116, 0x111a, 0x111f, 0x1123, 0x1127, 0x112b, 0x1130
    dw 0x1134, 0x1138, 0x113c, 0x1140, 0x1145, 0x1149, 0x114d, 0x1151
    dw 0x1156, 0x115a, 0x115e, 0x1162, 0x1166, 0x116b, 0x116f, 0x1173
    dw 0x1177, 0x117c, 0x1180, 0x1184, 0x1188, 0x118c, 0x1191, 0x1195
    dw 0x1199, 0x119d, 0x11a1, 0x11a6, 0x11aa, 0x11ae, 0x11b2, 0x11b6
    dw 0x11bb, 0x11bf, 0x11c3, 0x11c7, 0x11cb, 0x11cf, 0x11d4, 0x11d8
    dw 0x11dc, 0x11e0, 0x11e4, 0x11e9, 0x11ed, 0x11f1, 0x11f5, 0x11f9
    dw 0x11fd, 0x1202, 0x1206, 0x120a, 0x120e, 0x1212, 0x1216, 0x121a
    dw 0x121f, 0x1223, 0x1227, 0x122b, 0x122f, 0x1233, 0x1237, 0x123c
    dw 0x1240, 0x1244, 0x1248, 0x124c, 0x1250, 0x1254, 0x1259, 0x125d
    dw 0x1261, 0x1265, 0x1269, 0x126d, 0x1271, 0x1275, 0x127a, 0x127e
    dw 0x1282, 0x1286, 0x128a, 0x128e, 0x1292, 0x1296, 0x129a, 0x129f
    dw 0x12a3, 0x12a7, 0x12ab, 0x12af, 0x12b3, 0x12b7, 0x12bb, 0x12bf
    dw 0x12c3, 0x12c7, 0x12cc, 0x12d0, 0x12d4, 0x12d8, 0x12dc, 0x12e0
    dw 0x12e4, 0x12e8, 0x12ec, 0x12f0, 0x12f4, 0x12f8, 0x12fc, 0x1301
    dw 0x1305, 0x1309, 0x130d, 0x1311, 0x1315, 0x1319, 0x131d, 0x1321
    dw 0x1325, 0x1329, 0x132d, 0x1331, 0x1335, 0x1339, 0x133d, 0x1341
    dw 0x1345, 0x1349, 0x134d, 0x1351, 0x1355, 0x135a, 0x135e, 0x1362
    dw 0x1366, 0x136a, 0x136e, 0x1372, 0x1376, 0x137a, 0x137e, 0x1382
    dw 0x1386, 0x138a, 0x138e, 0x1392, 0x1396, 0x139a, 0x139e, 0x13a2
    dw 0x13a6, 0x13aa, 0x13ae, 0x13b2, 0x13b6, 0x13ba, 0x13be, 0x13c2
    dw 0x13c6, 0x13ca, 0x13ce, 0x13d2, 0x13d6, 0x13da, 0x13de, 0x13e2
    dw 0x13e6, 0x13e9, 0x13ed, 0x13f1, 0x13f5, 0x13f9, 0x13fd, 0x1401
    dw 0x1405, 0x1409, 0x140d, 0x1411, 0x1415, 0x1419, 0x141d, 0x1421
    dw 0x1425, 0x1429, 0x142d, 0x1431, 0x1435, 0x1439, 0x143d, 0x1440
    dw 0x1444, 0x1448, 0x144c, 0x1450, 0x1454, 0x1458, 0x145c, 0x1460
    dw 0x1464, 0x1468, 0x146c, 0x1470, 0x1473, 0x1477, 0x147b, 0x147f
    dw 0x1483, 0x1487, 0x148b, 0x148f, 0x1493, 0x1497, 0x149b, 0x149e
    dw 0x14a2, 0x14a6, 0x14aa, 0x14ae, 0x14b2, 0x14b6, 0x14ba, 0x14be
    dw 0x14c1, 0x14c5, 0x14c9, 0x14cd, 0x14d1, 0x14d5, 0x14d9, 0x14dd
    dw 0x14e0, 0x14e4, 0x14e8, 0x14ec, 0x14f0, 0x14f4, 0x14f8, 0x14fb
    dw 0x14ff, 0x1503, 0x1507, 0x150b, 0x150f, 0x1513, 0x1516, 0x151a
    dw 0x151e, 0x1522, 0x1526, 0x152a, 0x152d, 0x1531, 0x1535, 0x1539
    dw 0x153d, 0x1541, 0x1544, 0x1548, 0x154c, 0x1550, 0x1554, 0x1558
    dw 0x155b, 0x155f, 0x1563, 0x1567, 0x156b, 0x156e, 0x1572, 0x1576
    dw 0x157a, 0x157e, 0x1581, 0x1585, 0x1589, 0x158d, 0x1591, 0x1594
    dw 0x1598, 0x159c, 0x15a0, 0x15a4, 0x15a7, 0x15ab, 0x15af, 0x15b3
    dw 0x15b7, 0x15ba, 0x15be, 0x15c2, 0x15c6, 0x15c9, 0x15cd, 0x15d1
    dw 0x15d5, 0x15d8, 0x15dc, 0x15e0, 0x15e4, 0x15e8, 0x15eb, 0x15ef
    dw 0x15f3, 0x15f7, 0x15fa, 0x15fe, 0x1602, 0x1606, 0x1609, 0x160d
    dw 0x1611, 0x1614, 0x1618, 0x161c, 0x1620, 0x1623, 0x1627, 0x162b
    dw 0x162f, 0x1632, 0x1636, 0x163a, 0x163e, 0x1641, 0x1645, 0x1649
    dw 0x164c, 0x1650, 0x1654, 0x1658, 0x165b, 0x165f, 0x1663, 0x1666
    dw 0x166a, 0x166e, 0x1671, 0x1675, 0x1679, 0x167d, 0x1680, 0x1684
    dw 0x1688, 0x168b, 0x168f, 0x1693, 0x1696, 0x169a, 0x169e, 0x16a1
    dw 0x16a5, 0x16a9, 0x16ac, 0x16b0, 0x16b4, 0x16b7, 0x16bb, 0x16bf
    dw 0x16c2, 0x16c6, 0x16ca, 0x16cd, 0x16d1, 0x16d5, 0x16d8, 0x16dc
    dw 0x16e0, 0x16e3, 0x16e7, 0x16eb, 0x16ee, 0x16f2, 0x16f6, 0x16f9
    dw 0x16fd, 0x1700, 0x1704, 0x1708, 0x170b, 0x170f, 0x1713, 0x1716
    dw 0x171a, 0x171d, 0x1721, 0x1725, 0x1728, 0x172c, 0x1730, 0x1733
    dw 0x1737, 0x173a, 0x173e, 0x1742, 0x1745, 0x1749, 0x174c, 0x1750
    dw 0x1754, 0x1757, 0x175b, 0x175e, 0x1762, 0x1766, 0x1769, 0x176d
    dw 0x1770, 0x1774, 0x1778, 0x177b, 0x177f, 0x1782, 0x1786, 0x1789
    dw 0x178d, 0x1791, 0x1794, 0x1798, 0x179b, 0x179f, 0x17a2, 0x17a6
    dw 0x17aa, 0x17ad, 0x17b1, 0x17b4, 0x17b8, 0x17bb, 0x17bf, 0x17c2
    dw 0x17c6, 0x17c9, 0x17cd, 0x17d1, 0x17d4, 0x17d8, 0x17db, 0x17df
    dw 0x17e2, 0x17e6, 0x17e9, 0x17ed, 0x17f0, 0x17f4, 0x17f7, 0x17fb
    dw 0x17fe, 0x1802, 0x1806, 0x1809, 0x180d, 0x1810, 0x1814, 0x1817
    dw 0x181b, 0x181e, 0x1822, 0x1825, 0x1829, 0x182c, 0x1830, 0x1833
    dw 0x1837, 0x183a, 0x183e, 0x1841, 0x1845, 0x1848, 0x184c, 0x184f
    dw 0x1853, 0x1856, 0x185a, 0x185d, 0x1860, 0x1864, 0x1867, 0x186b
    dw 0x186e, 0x1872, 0x1875, 0x1879, 0x187c, 0x1880, 0x1883, 0x1887
    dw 0x188a, 0x188e, 0x1891, 0x1894, 0x1898, 0x189b, 0x189f, 0x18a2
    dw 0x18a6, 0x18a9, 0x18ad, 0x18b0, 0x18b3, 0x18b7, 0x18ba, 0x18be
    dw 0x18c1, 0x18c5, 0x18c8, 0x18cc, 0x18cf, 0x18d2, 0x18d6, 0x18d9
    dw 0x18dd, 0x18e0, 0x18e3, 0x18e7, 0x18ea, 0x18ee, 0x18f1, 0x18f5
    dw 0x18f8, 0x18fb, 0x18ff, 0x1902, 0x1906, 0x1909, 0x190c, 0x1910
    dw 0x1913, 0x1917, 0x191a, 0x191d, 0x1921, 0x1924, 0x1928, 0x192b
    dw 0x192e, 0x1932, 0x1935, 0x1938, 0x193c, 0x193f, 0x1943, 0x1946
    dw 0x1949, 0x194d, 0x1950, 0x1953, 0x1957, 0x195a, 0x195d, 0x1961
    dw 0x1964, 0x1968, 0x196b, 0x196e, 0x1972, 0x1975, 0x1978, 0x197c
    dw 0x197f, 0x1982, 0x1986, 0x1989, 0x198c, 0x1990, 0x1993, 0x1996
    dw 0x199a, 0x199d, 0x19a0, 0x19a4, 0x19a7, 0x19aa, 0x19ae, 0x19b1
    dw 0x19b4, 0x19b8, 0x19bb, 0x19be, 0x19c2, 0x19c5, 0x19c8, 0x19cc
    dw 0x19cf, 0x19d2, 0x19d5, 0x19d9, 0x19dc, 0x19df, 0x19e3, 0x19e6
    dw 0x19e9, 0x19ed, 0x19f0, 0x19f3, 0x19f6, 0x19fa, 0x19fd, 0x1a00
    dw 0x1a04, 0x1a07, 0x1a0a, 0x1a0d, 0x1a11, 0x1a14, 0x1a17, 0x1a1b
    dw 0x1a1e, 0x1a21, 0x1a24, 0x1a28, 0x1a2b, 0x1a2e, 0x1a31, 0x1a35
    dw 0x1a38, 0x1a3b, 0x1a3e, 0x1a42, 0x1a45, 0x1a48, 0x1a4b, 0x1a4f
    dw 0x1a52, 0x1a55, 0x1a58, 0x1a5c, 0x1a5f, 0x1a62, 0x1a65, 0x1a69
    dw 0x1a6c, 0x1a6f, 0x1a72, 0x1a76, 0x1a79, 0x1a7c, 0x1a7f, 0x1a83
    dw 0x1a86, 0x1a89, 0x1a8c, 0x1a8f, 0x1a93, 0x1a96, 0x1a99, 0x1a9c
    dw 0x1a9f, 0x1aa3, 0x1aa6, 0x1aa9, 0x1aac, 0x1ab0, 0x1ab3, 0x1ab6
    dw 0x1ab9, 0x1abc, 0x1ac0, 0x1ac3, 0x1ac6, 0x1ac9, 0x1acc, 0x1acf
    dw 0x1ad3, 0x1ad6, 0x1ad9, 0x1adc, 0x1adf, 0x1ae3, 0x1ae6, 0x1ae9
    dw 0x1aec, 0x1aef, 0x1af2, 0x1af6, 0x1af9, 0x1afc, 0x1aff, 0x1b02
    dw 0x1b05, 0x1b09, 0x1b0c, 0x1b0f, 0x1b12, 0x1b15, 0x1b18, 0x1b1c
    dw 0x1b1f, 0x1b22, 0x1b25, 0x1b28, 0x1b2b, 0x1b2e, 0x1b32, 0x1b35
    dw 0x1b38, 0x1b3b, 0x1b3e, 0x1b41, 0x1b44, 0x1b48, 0x1b4b, 0x1b4e
    dw 0x1b51, 0x1b54, 0x1b57, 0x1b5a, 0x1b5d, 0x1b61, 0x1b64, 0x1b67
    dw 0x1b6a, 0x1b6d, 0x1b70, 0x1b73, 0x1b76, 0x1b79, 0x1b7d, 0x1b80
    dw 0x1b83, 0x1b86, 0x1b89, 0x1b8c, 0x1b8f, 0x1b92, 0x1b95, 0x1b98
    dw 0x1b9c, 0x1b9f, 0x1ba2, 0x1ba5, 0x1ba8, 0x1bab, 0x1bae, 0x1bb1
    dw 0x1bb4, 0x1bb7, 0x1bba, 0x1bbd, 0x1bc1, 0x1bc4, 0x1bc7, 0x1bca
    dw 0x1bcd, 0x1bd0, 0x1bd3, 0x1bd6, 0x1bd9, 0x1bdc, 0x1bdf, 0x1be2
    dw 0x1be5, 0x1be8, 0x1beb, 0x1bee, 0x1bf2, 0x1bf5, 0x1bf8, 0x1bfb
    dw 0x1bfe, 0x1c01, 0x1c04, 0x1c07, 0x1c0a, 0x1c0d, 0x1c10, 0x1c13
    dw 0x1c16, 0x1c19, 0x1c1c, 0x1c1f, 0x1c22, 0x1c25, 0x1c28, 0x1c2b
    dw 0x1c2e, 0x1c31, 0x1c34, 0x1c37, 0x1c3a, 0x1c3d, 0x1c40, 0x1c43
    dw 0x1c46, 0x1c49, 0x1c4c, 0x1c4f, 0x1c52, 0x1c55, 0x1c58, 0x1c5b
    dw 0x1c5e, 0x1c61, 0x1c64, 0x1c67, 0x1c6a, 0x1c6d, 0x1c70, 0x1c73
    dw 0x1c76, 0x1c79, 0x1c7c, 0x1c7f, 0x1c82, 0x1c85, 0x1c88, 0x1c8b
    dw 0x1c8e, 0x1c91, 0x1c94, 0x1c97, 0x1c9a, 0x1c9d, 0x1ca0, 0x1ca3
    dw 0x1ca6, 0x1ca9, 0x1cac, 0x1caf, 0x1cb2, 0x1cb5, 0x1cb8, 0x1cbb
    dw 0x1cbe, 0x1cc1, 0x1cc3, 0x1cc6, 0x1cc9, 0x1ccc, 0x1ccf, 0x1cd2
    dw 0x1cd5, 0x1cd8, 0x1cdb, 0x1cde, 0x1ce1, 0x1ce4, 0x1ce7, 0x1cea
    dw 0x1ced, 0x1cf0, 0x1cf3, 0x1cf5, 0x1cf8, 0x1cfb, 0x1cfe, 0x1d01
    dw 0x1d04, 0x1d07, 0x1d0a, 0x1d0d, 0x1d10, 0x1d13, 0x1d16, 0x1d18
    dw 0x1d1b, 0x1d1e, 0x1d21, 0x1d24, 0x1d27, 0x1d2a, 0x1d2d, 0x1d30
    dw 0x1d33, 0x1d35, 0x1d38, 0x1d3b, 0x1d3e, 0x1d41, 0x1d44, 0x1d47
    dw 0x1d4a, 0x1d4d, 0x1d4f, 0x1d52, 0x1d55, 0x1d58, 0x1d5b, 0x1d5e
    dw 0x1d61, 0x1d64, 0x1d66, 0x1d69, 0x1d6c, 0x1d6f, 0x1d72, 0x1d75
    dw 0x1d78, 0x1d7b, 0x1d7d, 0x1d80, 0x1d83, 0x1d86, 0x1d89, 0x1d8c
    dw 0x1d8e, 0x1d91, 0x1d94, 0x1d97, 0x1d9a, 0x1d9d, 0x1da0, 0x1da2
    dw 0x1da5, 0x1da8, 0x1dab, 0x1dae, 0x1db1, 0x1db3, 0x1db6, 0x1db9
    dw 0x1dbc, 0x1dbf, 0x1dc2, 0x1dc4, 0x1dc7, 0x1dca, 0x1dcd, 0x1dd0
    dw 0x1dd3, 0x1dd5, 0x1dd8, 0x1ddb, 0x1dde, 0x1de1, 0x1de3, 0x1de6
    dw 0x1de9, 0x1dec, 0x1def, 0x1df1, 0x1df4, 0x1df7, 0x1dfa, 0x1dfd
    dw 0x1dff, 0x1e02, 0x1e05, 0x1e08, 0x1e0b, 0x1e0d, 0x1e10, 0x1e13
    dw 0x1e16, 0x1e19, 0x1e1b, 0x1e1e, 0x1e21, 0x1e24, 0x1e26, 0x1e29
    dw 0x1e2c, 0x1e2f, 0x1e32, 0x1e34, 0x1e37, 0x1e3a, 0x1e3d, 0x1e3f
    dw 0x1e42, 0x1e45, 0x1e48, 0x1e4a, 0x1e4d, 0x1e50, 0x1e53, 0x1e55
    dw 0x1e58, 0x1e5b, 0x1e5e, 0x1e60, 0x1e63, 0x1e66, 0x1e69, 0x1e6b
    dw 0x1e6e, 0x1e71, 0x1e74, 0x1e76, 0x1e79, 0x1e7c, 0x1e7f, 0x1e81
    dw 0x1e84, 0x1e87, 0x1e8a, 0x1e8c, 0x1e8f, 0x1e92, 0x1e94, 0x1e97
    dw 0x1e9a, 0x1e9d, 0x1e9f, 0x1ea2, 0x1ea5, 0x1ea8, 0x1eaa, 0x1ead
    dw 0x1eb0, 0x1eb2, 0x1eb5, 0x1eb8, 0x1eba, 0x1ebd, 0x1ec0, 0x1ec3
    dw 0x1ec5, 0x1ec8, 0x1ecb, 0x1ecd, 0x1ed0, 0x1ed3, 0x1ed5, 0x1ed8
    dw 0x1edb, 0x1ede, 0x1ee0, 0x1ee3, 0x1ee6, 0x1ee8, 0x1eeb, 0x1eee
    dw 0x1ef0, 0x1ef3, 0x1ef6, 0x1ef8, 0x1efb, 0x1efe, 0x1f00, 0x1f03
    dw 0x1f06, 0x1f08, 0x1f0b, 0x1f0e, 0x1f10, 0x1f13, 0x1f16, 0x1f18
    dw 0x1f1b, 0x1f1e, 0x1f20, 0x1f23, 0x1f26, 0x1f28, 0x1f2b, 0x1f2e
    dw 0x1f30, 0x1f33, 0x1f36, 0x1f38, 0x1f3b, 0x1f3d, 0x1f40, 0x1f43
    dw 0x1f45, 0x1f48, 0x1f4b, 0x1f4d, 0x1f50, 0x1f53, 0x1f55, 0x1f58
    dw 0x1f5a, 0x1f5d, 0x1f60, 0x1f62, 0x1f65, 0x1f68, 0x1f6a, 0x1f6d
    dw 0x1f6f, 0x1f72, 0x1f75, 0x1f77, 0x1f7a, 0x1f7c, 0x1f7f, 0x1f82
    dw 0x1f84, 0x1f87, 0x1f8a, 0x1f8c, 0x1f8f, 0x1f91, 0x1f94, 0x1f97
    dw 0x1f99, 0x1f9c, 0x1f9e, 0x1fa1, 0x1fa4, 0x1fa6, 0x1fa9, 0x1fab
    dw 0x1fae, 0x1fb0, 0x1fb3, 0x1fb6, 0x1fb8, 0x1fbb, 0x1fbd, 0x1fc0
    dw 0x1fc3, 0x1fc5, 0x1fc8, 0x1fca, 0x1fcd, 0x1fcf, 0x1fd2, 0x1fd5
    dw 0x1fd7, 0x1fda, 0x1fdc, 0x1fdf, 0x1fe1, 0x1fe4, 0x1fe6, 0x1fe9
    dw 0x1fec, 0x1fee, 0x1ff1, 0x1ff3, 0x1ff6, 0x1ff8, 0x1ffb, 0x1ffd
    dw 0x2000

; projection_atan_table_trailer: Fourteen FFh bytes after the 2049-word atan table. CS:1B0E can read only indices 0..2047; equality at CS:1B33 uses an immediate, so neither the endpoint nor this trailer is needed by that lookup. Table plus trailer occupies 4112 bytes, a multiple of 16; any independent consumer or reason for FFh fill remains unconfirmed. [load+0xc328]
    times 14 db 0xff

; projection_distance_correction_lut: 129 distance correction coefficients: round(512*r/(sqrt(1+r*r)+1)), r=i/128. CS:1D45 indexes by ceil(saved_ratio/2) and computes major + floor(coefficient*minor/512). At equal axes coefficient 212 approximates sqrt(2); zero axes add no correction. [load+0xc336]
    db 0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e
    db 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x33, 0x35, 0x37, 0x39, 0x3b, 0x3d
    db 0x3f, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x57, 0x59, 0x5b
    db 0x5d, 0x5f, 0x60, 0x62, 0x64, 0x66, 0x68, 0x69, 0x6b, 0x6d, 0x6f, 0x70, 0x72, 0x74, 0x75, 0x77
    db 0x79, 0x7b, 0x7c, 0x7e, 0x80, 0x81, 0x83, 0x84, 0x86, 0x88, 0x89, 0x8b, 0x8d, 0x8e, 0x90, 0x91
    db 0x93, 0x94, 0x96, 0x97, 0x99, 0x9b, 0x9c, 0x9e, 0x9f, 0xa1, 0xa2, 0xa3, 0xa5, 0xa6, 0xa8, 0xa9
    db 0xab, 0xac, 0xad, 0xaf, 0xb0, 0xb2, 0xb3, 0xb4, 0xb6, 0xb7, 0xb8, 0xba, 0xbb, 0xbc, 0xbe, 0xbf
    db 0xc0, 0xc2, 0xc3, 0xc4, 0xc5, 0xc7, 0xc8, 0xc9, 0xca, 0xcc, 0xcd, 0xce, 0xcf, 0xd1, 0xd2, 0xd3
    db 0xd4
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; g data +0xc3b7

; keyboard_ascii_by_scan_code: 128 prompt translations with # as the no-character result, but the key-wait path only indexes 00h..7Ch. Main-row and keypad digits map to ASCII digits; Enter and Backspace/Delete map to formatter controls. A set keyboard_e0_prefix suppresses translated digits by returning #, then clears the prefix latch. This translation is for entry prompts; gameplay bindings consume scan states directly. [load+0xc3c6]
    times 2 db KEY_NO_CHARACTER ; scan codes 00h..01h
    db '1' ; scan code 02h
    db '2' ; scan code 03h
    db '3' ; scan code 04h
    db '4' ; scan code 05h
    db '5' ; scan code 06h
    db '6' ; scan code 07h
    db '7' ; scan code 08h
    db '8' ; scan code 09h
    db '9' ; scan code 0Ah
    db '0' ; scan code 0Bh
    times 2 db KEY_NO_CHARACTER ; scan codes 0Ch..0Dh
    db KEY_DELETE ; scan code 0Eh
    times 13 db KEY_NO_CHARACTER ; scan codes 0Fh..1Bh
    db KEY_ENTER ; scan code 1Ch
    times 42 db KEY_NO_CHARACTER ; scan codes 1Dh..46h
    db '7' ; scan code 47h
    db '8' ; scan code 48h
    db '9' ; scan code 49h
    times 1 db KEY_NO_CHARACTER ; scan codes 4Ah..4Ah
    db '4' ; scan code 4Bh
    db '5' ; scan code 4Ch
    db '6' ; scan code 4Dh
    times 1 db KEY_NO_CHARACTER ; scan codes 4Eh..4Eh
    db '1' ; scan code 4Fh
    db '2' ; scan code 50h
    db '3' ; scan code 51h
    db '0' ; scan code 52h
    db KEY_DELETE ; scan code 53h
    times 32 db KEY_NO_CHARACTER ; scan codes 54h..73h
    db KEY_ENTER ; scan code 74h
    times 11 db KEY_NO_CHARACTER ; scan codes 75h..7Fh

; terrain_triangle_vertex_delta: Triangle corner-step table {+1,+33,-1,+31}: CS:204D indexes split for the first triangle and split+2 for the second. These steps join alternating rows in a 32-entry corner cache; they are cache offsets, not terrain-grid byte offsets. [load+0xc446]
    db 1 ; split 0, first half
    db TERRAIN_SIDE+1 ; split 1, first half
    db -1 ; split 0, opposite half
    db TERRAIN_SIDE-1 ; split 1, opposite half

; terrain_face_color_pairs: 32 packed two-color choices: low four index bits select terrain shape, bit 4 switches triangle half; checkerboard quads use entries 0 and 8 [load+0xc44a]
    db (3 << 4) | 3 ; shape 0h, half bank 0, palette slots 3/3
    db (1 << 4) | 0 ; shape 1h, half bank 0, palette slots 1/0
    db (1 << 4) | 0 ; shape 2h, half bank 0, palette slots 1/0
    db (1 << 4) | 2 ; shape 3h, half bank 0, palette slots 1/2
    db (1 << 4) | 2 ; shape 4h, half bank 0, palette slots 1/2
    db (1 << 4) | 2 ; shape 5h, half bank 0, palette slots 1/2
    db (1 << 4) | 0 ; shape 6h, half bank 0, palette slots 1/0
    db (1 << 4) | 2 ; shape 7h, half bank 0, palette slots 1/2
    db (1 << 4) | 1 ; shape 8h, half bank 0, palette slots 1/1
    db (1 << 4) | 0 ; shape 9h, half bank 0, palette slots 1/0
    db (1 << 4) | 2 ; shape Ah, half bank 0, palette slots 1/2
    db (1 << 4) | 0 ; shape Bh, half bank 0, palette slots 1/0
    db (1 << 4) | 0 ; shape Ch, half bank 0, palette slots 1/0
    db (1 << 4) | 2 ; shape Dh, half bank 0, palette slots 1/2
    db (1 << 4) | 2 ; shape Eh, half bank 0, palette slots 1/2
    db (1 << 4) | 0 ; shape Fh, half bank 0, palette slots 1/0
    db (1 << 4) | 1 ; shape 0h, half bank 1, palette slots 1/1
    db (1 << 4) | 1 ; shape 1h, half bank 1, palette slots 1/1
    db (1 << 4) | 2 ; shape 2h, half bank 1, palette slots 1/2
    db (1 << 4) | 0 ; shape 3h, half bank 1, palette slots 1/0
    db (1 << 4) | 2 ; shape 4h, half bank 1, palette slots 1/2
    db (1 << 4) | 1 ; shape 5h, half bank 1, palette slots 1/1
    db (1 << 4) | 2 ; shape 6h, half bank 1, palette slots 1/2
    db (1 << 4) | 0 ; shape 7h, half bank 1, palette slots 1/0
    db (1 << 4) | 1 ; shape 8h, half bank 1, palette slots 1/1
    db (1 << 4) | 1 ; shape 9h, half bank 1, palette slots 1/1
    db (1 << 4) | 0 ; shape Ah, half bank 1, palette slots 1/0
    db (1 << 4) | 2 ; shape Bh, half bank 1, palette slots 1/2
    db (1 << 4) | 0 ; shape Ch, half bank 1, palette slots 1/0
    db (1 << 4) | 1 ; shape Dh, half bank 1, palette slots 1/1
    db (1 << 4) | 0 ; shape Eh, half bank 1, palette slots 1/0
    db (1 << 4) | 2 ; shape Fh, half bank 1, palette slots 1/2

; terrain_triangle_orientations: 16 diagonal selectors with exact rule split=((rotation_adjusted_shape&6)==6). CS:2A0A subtracts terrain_face_rotation_bias modulo 16 before indexing. Shape codes 4/12 bypass this table and use view-quadrant parity; rotation-adjusted codes congruent to 1 modulo 4 are quads. [load+0xc46a]
    db TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT ; rotation-adjusted shape 0h
    db TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT ; rotation-adjusted shape 1h
    db TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT ; rotation-adjusted shape 2h
    db TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT ; rotation-adjusted shape 3h
    db TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT ; rotation-adjusted shape 4h
    db TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT ; rotation-adjusted shape 5h
    db TRI_SPLIT_TOP_RIGHT_BOTTOM_LEFT ; rotation-adjusted shape 6h
    db TRI_SPLIT_TOP_RIGHT_BOTTOM_LEFT ; rotation-adjusted shape 7h
    db TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT ; rotation-adjusted shape 8h
    db TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT ; rotation-adjusted shape 9h
    db TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT ; rotation-adjusted shape Ah
    db TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT ; rotation-adjusted shape Bh
    db TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT ; rotation-adjusted shape Ch
    db TRI_SPLIT_TOP_LEFT_BOTTOM_RIGHT ; rotation-adjusted shape Dh
    db TRI_SPLIT_TOP_RIGHT_BOTTOM_LEFT ; rotation-adjusted shape Eh
    db TRI_SPLIT_TOP_RIGHT_BOTTOM_LEFT ; rotation-adjusted shape Fh

; model_vertex_ranges: ten meshes: first vertex index per mesh plus an exclusive-end sentinel; model_project_vertices reads [mesh] and [mesh+1] [load+0xc47a]
    db ROBOT_VERTEX_START ; MODEL_ROBOT: 29 vertices, indices 0..28
    db SENTRY_VERTEX_START ; MODEL_SENTRY: 22 vertices, indices 29..50
    db TREE_VERTEX_START ; MODEL_TREE: 17 vertices, indices 51..67
    db BOULDER_VERTEX_START ; MODEL_BOULDER: 8 vertices, indices 68..75
    db MEANIE_VERTEX_START ; MODEL_MEANIE: 18 vertices, indices 76..93
    db SENTINEL_VERTEX_START ; MODEL_SENTINEL: 30 vertices, indices 94..123
    db SENTINEL_PEDESTAL_VERTEX_START ; MODEL_SENTINEL_PEDESTAL: 12 vertices, indices 124..135
    db TEXT_BLOCK_01_VERTEX_START ; MODEL_TEXT_BLOCK_01: 8 vertices, indices 136..143
    db TEXT_BLOCK_10_VERTEX_START ; MODEL_TEXT_BLOCK_10: 8 vertices, indices 144..151
    db TEXT_BLOCK_11_VERTEX_START ; MODEL_TEXT_BLOCK_11: 8 vertices, indices 152..159
    db 160 ; exclusive end of all vertices

; model_face_ranges: ten meshes: first face index per mesh plus an exclusive-end sentinel; model_draw_mesh walks each half-open face range [load+0xc485]
    db ROBOT_FACE_START ; MODEL_ROBOT: 27 faces, indices 0..26
    db SENTRY_FACE_START ; MODEL_SENTRY: 25 faces, indices 27..51
    db TREE_FACE_START ; MODEL_TREE: 15 faces, indices 52..66
    db BOULDER_FACE_START ; MODEL_BOULDER: 10 faces, indices 67..76
    db MEANIE_FACE_START ; MODEL_MEANIE: 25 faces, indices 77..101
    db SENTINEL_FACE_START ; MODEL_SENTINEL: 35 faces, indices 102..136
    db SENTINEL_PEDESTAL_FACE_START ; MODEL_SENTINEL_PEDESTAL: 11 faces, indices 137..147
    db TEXT_BLOCK_01_FACE_START ; MODEL_TEXT_BLOCK_01: 4 faces, indices 148..151
    db TEXT_BLOCK_10_FACE_START ; MODEL_TEXT_BLOCK_10: 4 faces, indices 152..155
    db TEXT_BLOCK_11_FACE_START ; MODEL_TEXT_BLOCK_11: 4 faces, indices 156..159
    db 160 ; exclusive end of all faces

; model_visibility_flags: Ten mesh ordering policies. CS:18AA uses stored order for 0. Policy 3 partitions only when model_relative_height>0; zero and negative heights use one pass. Policy 2 partitions for an original relative-heading high byte in 00h..3Fh or C0h..FFh (the C0h-biased byte is negative). Partitioning emits flag-bit-7-clear faces then flag-bit-7-set faces; it is not back-face normal culling. [load+0xc490]
    db MESH_PASS_BY_HEIGHT ; MODEL_ROBOT
    db MESH_PASS_BY_HEIGHT ; MODEL_SENTRY
    db MESH_PASS_STORED_ORDER ; MODEL_TREE
    db MESH_PASS_STORED_ORDER ; MODEL_BOULDER
    db MESH_PASS_BY_HEADING ; MODEL_MEANIE
    db MESH_PASS_BY_HEIGHT ; MODEL_SENTINEL
    db MESH_PASS_STORED_ORDER ; MODEL_SENTINEL_PEDESTAL
    db MESH_PASS_STORED_ORDER ; MODEL_TEXT_BLOCK_01
    db MESH_PASS_STORED_ORDER ; MODEL_TEXT_BLOCK_10
    db MESH_PASS_STORED_ORDER ; MODEL_TEXT_BLOCK_11
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; g data +0xc49a

; model_vertex_loop_4bf7: closed vertex loop for MODEL_MEANIE face 0 (global 77) [load+0xc4a7]
    db 0x47, 0x4c, 0x4b, 0x46, 0x47

; model_vertex_loop_4bfc: closed vertex loop for MODEL_MEANIE face 1 (global 78) [load+0xc4ac]
    db 0x40, 0x45, 0x42, 0x40

; model_vertex_loop_4c00: closed vertex loop for MODEL_MEANIE face 2 (global 79) [load+0xc4b0]
    db 0x41, 0x44, 0x40, 0x41

; model_vertex_loop_4c04: closed vertex loop for MODEL_MEANIE face 3 (global 80) [load+0xc4b4]
    db 0x40, 0x44, 0x43, 0x40

; model_vertex_loop_4c08: closed vertex loop for MODEL_MEANIE face 4 (global 81) [load+0xc4b8]
    db 0x40, 0x43, 0x45, 0x40

; model_vertex_loop_4c0c: closed vertex loop for MODEL_MEANIE face 5 (global 82) [load+0xc4bc]
    db 0x42, 0x45, 0x44, 0x41, 0x42

; model_vertex_loop_4c11: closed vertex loop for MODEL_MEANIE face 6 (global 83) [load+0xc4c1]
    db 0x45, 0x46, 0x49, 0x45

; model_vertex_loop_4c15: closed vertex loop for MODEL_MEANIE face 7 (global 84) [load+0xc4c5]
    db 0x44, 0x48, 0x47, 0x44

; model_vertex_loop_4c19: closed vertex loop for MODEL_MEANIE face 8 (global 85) [load+0xc4c9]
    db 0x45, 0x49, 0x48, 0x44, 0x45

; model_vertex_loop_4c1e: closed vertex loop for MODEL_MEANIE face 9 (global 86) [load+0xc4ce]
    db 0x43, 0x46, 0x45, 0x43

; model_vertex_loop_4c22: closed vertex loop for MODEL_MEANIE face 10 (global 87) [load+0xc4d2]
    db 0x44, 0x47, 0x43, 0x44

; model_vertex_loop_4c26: closed vertex loop for MODEL_MEANIE face 11 (global 88) [load+0xc4d6]
    db 0x43, 0x47, 0x46, 0x43

; model_vertex_loop_4c2a: closed vertex loop for MODEL_MEANIE face 12 (global 89) [load+0xc4da]
    db 0x4b, 0x4a, 0x46, 0x4b

; model_vertex_loop_4c2e: closed vertex loop for MODEL_MEANIE face 13 (global 90) [load+0xc4de]
    db 0x47, 0x4d, 0x4c, 0x47

; model_vertex_loop_4c32: closed vertex loop for MODEL_MEANIE face 14 (global 91) [load+0xc4e2]
    db 0x4b, 0x4e, 0x4a, 0x4b

; model_vertex_loop_4c36: closed vertex loop for MODEL_MEANIE face 15 (global 92) [load+0xc4e6]
    db 0x4c, 0x4d, 0x4f, 0x4c

; model_vertex_loop_4c3a: closed vertex loop for MODEL_MEANIE face 16 (global 93) [load+0xc4ea]
    db 0x4c, 0x4f, 0x4e, 0x4b, 0x4c

; model_vertex_loop_4c3f: closed vertex loop for MODEL_MEANIE face 17 (global 94) [load+0xc4ef]
    db 0x46, 0x4a, 0x49, 0x46

; model_vertex_loop_4c43: closed vertex loop for MODEL_MEANIE face 18 (global 95) [load+0xc4f3]
    db 0x47, 0x48, 0x4d, 0x47

; model_vertex_loop_4c47: closed vertex loop for MODEL_MEANIE face 19 (global 96) [load+0xc4f7]
    db 0x4a, 0x50, 0x49, 0x4a

; model_vertex_loop_4c4b: closed vertex loop for MODEL_MEANIE face 20 (global 97) [load+0xc4fb]
    db 0x48, 0x51, 0x4d, 0x48

; model_vertex_loop_4c4f: closed vertex loop for MODEL_MEANIE face 21 (global 98) [load+0xc4ff]
    db 0x4a, 0x4e, 0x50, 0x4a

; model_vertex_loop_4c53: closed vertex loop for MODEL_MEANIE face 22 (global 99) [load+0xc503]
    db 0x4d, 0x51, 0x4f, 0x4d

; model_vertex_loop_4c57: closed vertex loop for MODEL_MEANIE face 23 (global 100) [load+0xc507]
    db 0x49, 0x50, 0x51, 0x48, 0x49

; model_vertex_loop_4c5c: closed vertex loop for MODEL_MEANIE face 24 (global 101) [load+0xc50c]
    db 0x4f, 0x51, 0x50, 0x4e, 0x4f

; opaque_meanie_face_loop_trailer: Trailer after all 25 Meanie face loops: nine FFh bytes precede five 32-byte initializer chunks. No closed-loop range from the 160 face pointers enters this block; it ends at the polar azimuth array. [load+0xc511]
    times 9 db 0xff
    opaque_init_chunk ZERO
    opaque_init_chunk ONES_WITH_A0_EF
    opaque_init_chunk ZERO_WITH_20
    opaque_init_chunk ONES
    opaque_init_chunk ZERO

; model_vertex_azimuth: 160 polar azimuths in 1/256 turn. CS:17DA adds each to the high byte of model_relative_heading; its low byte supplies the extra half-sample phase to the sine/cosine lookup. Thus vertex azimuth is relative to mesh orientation, not a fixed world bearing; source indices span 0..159 across ten models. [load+0xc5ba]
    db 207 ; MODEL_ROBOT vertex 0 (global 0): 291.09375 degrees
    db 49 ; MODEL_ROBOT vertex 1 (global 1): 68.90625 degrees
    db 95 ; MODEL_ROBOT vertex 2 (global 2): 133.59375 degrees
    db 161 ; MODEL_ROBOT vertex 3 (global 3): 226.40625 degrees
    db 217 ; MODEL_ROBOT vertex 4 (global 4): 305.15625 degrees
    db 39 ; MODEL_ROBOT vertex 5 (global 5): 54.84375 degrees
    db 87 ; MODEL_ROBOT vertex 6 (global 6): 122.34375 degrees
    db 169 ; MODEL_ROBOT vertex 7 (global 7): 237.65625 degrees
    db 203 ; MODEL_ROBOT vertex 8 (global 8): 285.46875 degrees
    db 0 ; MODEL_ROBOT vertex 9 (global 9): 0 degrees
    db 53 ; MODEL_ROBOT vertex 10 (global 10): 74.53125 degrees
    db 85 ; MODEL_ROBOT vertex 11 (global 11): 119.53125 degrees
    db 171 ; MODEL_ROBOT vertex 12 (global 12): 240.46875 degrees
    db 197 ; MODEL_ROBOT vertex 13 (global 13): 277.03125 degrees
    db 59 ; MODEL_ROBOT vertex 14 (global 14): 82.96875 degrees
    db 82 ; MODEL_ROBOT vertex 15 (global 15): 115.3125 degrees
    db 174 ; MODEL_ROBOT vertex 16 (global 16): 244.6875 degrees
    db 0 ; MODEL_ROBOT vertex 17 (global 17): 0 degrees
    db 236 ; MODEL_ROBOT vertex 18 (global 18): 331.875 degrees
    db 20 ; MODEL_ROBOT vertex 19 (global 19): 28.125 degrees
    db 99 ; MODEL_ROBOT vertex 20 (global 20): 139.21875 degrees
    db 157 ; MODEL_ROBOT vertex 21 (global 21): 220.78125 degrees
    db 192 ; MODEL_ROBOT vertex 22 (global 22): 270 degrees
    db 64 ; MODEL_ROBOT vertex 23 (global 23): 90 degrees
    db 105 ; MODEL_ROBOT vertex 24 (global 24): 147.65625 degrees
    db 151 ; MODEL_ROBOT vertex 25 (global 25): 212.34375 degrees
    db 33 ; MODEL_ROBOT vertex 26 (global 26): 46.40625 degrees
    db 223 ; MODEL_ROBOT vertex 27 (global 27): 313.59375 degrees
    db 0 ; MODEL_ROBOT vertex 28 (global 28): 0 degrees
    db 218 ; MODEL_SENTRY vertex 0 (global 29): 306.5625 degrees
    db 38 ; MODEL_SENTRY vertex 1 (global 30): 53.4375 degrees
    db 90 ; MODEL_SENTRY vertex 2 (global 31): 126.5625 degrees
    db 166 ; MODEL_SENTRY vertex 3 (global 32): 233.4375 degrees
    db 245 ; MODEL_SENTRY vertex 4 (global 33): 344.53125 degrees
    db 11 ; MODEL_SENTRY vertex 5 (global 34): 15.46875 degrees
    db 64 ; MODEL_SENTRY vertex 6 (global 35): 90 degrees
    db 90 ; MODEL_SENTRY vertex 7 (global 36): 126.5625 degrees
    db 166 ; MODEL_SENTRY vertex 8 (global 37): 233.4375 degrees
    db 192 ; MODEL_SENTRY vertex 9 (global 38): 270 degrees
    db 218 ; MODEL_SENTRY vertex 10 (global 39): 306.5625 degrees
    db 38 ; MODEL_SENTRY vertex 11 (global 40): 53.4375 degrees
    db 90 ; MODEL_SENTRY vertex 12 (global 41): 126.5625 degrees
    db 166 ; MODEL_SENTRY vertex 13 (global 42): 233.4375 degrees
    db 245 ; MODEL_SENTRY vertex 14 (global 43): 344.53125 degrees
    db 11 ; MODEL_SENTRY vertex 15 (global 44): 15.46875 degrees
    db 248 ; MODEL_SENTRY vertex 16 (global 45): 348.75 degrees
    db 8 ; MODEL_SENTRY vertex 17 (global 46): 11.25 degrees
    db 226 ; MODEL_SENTRY vertex 18 (global 47): 317.8125 degrees
    db 30 ; MODEL_SENTRY vertex 19 (global 48): 42.1875 degrees
    db 92 ; MODEL_SENTRY vertex 20 (global 49): 129.375 degrees
    db 164 ; MODEL_SENTRY vertex 21 (global 50): 230.625 degrees
    db 224 ; MODEL_TREE vertex 0 (global 51): 315 degrees
    db 32 ; MODEL_TREE vertex 1 (global 52): 45 degrees
    db 96 ; MODEL_TREE vertex 2 (global 53): 135 degrees
    db 160 ; MODEL_TREE vertex 3 (global 54): 225 degrees
    db 224 ; MODEL_TREE vertex 4 (global 55): 315 degrees
    db 32 ; MODEL_TREE vertex 5 (global 56): 45 degrees
    db 96 ; MODEL_TREE vertex 6 (global 57): 135 degrees
    db 160 ; MODEL_TREE vertex 7 (global 58): 225 degrees
    db 0 ; MODEL_TREE vertex 8 (global 59): 0 degrees
    db 32 ; MODEL_TREE vertex 9 (global 60): 45 degrees
    db 64 ; MODEL_TREE vertex 10 (global 61): 90 degrees
    db 96 ; MODEL_TREE vertex 11 (global 62): 135 degrees
    db 128 ; MODEL_TREE vertex 12 (global 63): 180 degrees
    db 160 ; MODEL_TREE vertex 13 (global 64): 225 degrees
    db 192 ; MODEL_TREE vertex 14 (global 65): 270 degrees
    db 224 ; MODEL_TREE vertex 15 (global 66): 315 degrees
    db 0 ; MODEL_TREE vertex 16 (global 67): 0 degrees
    db 0 ; MODEL_BOULDER vertex 0 (global 68): 0 degrees
    db 32 ; MODEL_BOULDER vertex 1 (global 69): 45 degrees
    db 64 ; MODEL_BOULDER vertex 2 (global 70): 90 degrees
    db 96 ; MODEL_BOULDER vertex 3 (global 71): 135 degrees
    db 128 ; MODEL_BOULDER vertex 4 (global 72): 180 degrees
    db 160 ; MODEL_BOULDER vertex 5 (global 73): 225 degrees
    db 192 ; MODEL_BOULDER vertex 6 (global 74): 270 degrees
    db 224 ; MODEL_BOULDER vertex 7 (global 75): 315 degrees
    db 0 ; MODEL_MEANIE vertex 0 (global 76): 0 degrees
    db 89 ; MODEL_MEANIE vertex 1 (global 77): 125.15625 degrees
    db 167 ; MODEL_MEANIE vertex 2 (global 78): 234.84375 degrees
    db 128 ; MODEL_MEANIE vertex 3 (global 79): 180 degrees
    db 114 ; MODEL_MEANIE vertex 4 (global 80): 160.3125 degrees
    db 142 ; MODEL_MEANIE vertex 5 (global 81): 199.6875 degrees
    db 225 ; MODEL_MEANIE vertex 6 (global 82): 316.40625 degrees
    db 31 ; MODEL_MEANIE vertex 7 (global 83): 43.59375 degrees
    db 89 ; MODEL_MEANIE vertex 8 (global 84): 125.15625 degrees
    db 167 ; MODEL_MEANIE vertex 9 (global 85): 234.84375 degrees
    db 213 ; MODEL_MEANIE vertex 10 (global 86): 299.53125 degrees
    db 246 ; MODEL_MEANIE vertex 11 (global 87): 345.9375 degrees
    db 10 ; MODEL_MEANIE vertex 12 (global 88): 14.0625 degrees
    db 43 ; MODEL_MEANIE vertex 13 (global 89): 60.46875 degrees
    db 251 ; MODEL_MEANIE vertex 14 (global 90): 352.96875 degrees
    db 5 ; MODEL_MEANIE vertex 15 (global 91): 7.03125 degrees
    db 233 ; MODEL_MEANIE vertex 16 (global 92): 327.65625 degrees
    db 23 ; MODEL_MEANIE vertex 17 (global 93): 32.34375 degrees
    db 218 ; MODEL_SENTINEL vertex 0 (global 94): 306.5625 degrees
    db 38 ; MODEL_SENTINEL vertex 1 (global 95): 53.4375 degrees
    db 90 ; MODEL_SENTINEL vertex 2 (global 96): 126.5625 degrees
    db 166 ; MODEL_SENTINEL vertex 3 (global 97): 233.4375 degrees
    db 218 ; MODEL_SENTINEL vertex 4 (global 98): 306.5625 degrees
    db 38 ; MODEL_SENTINEL vertex 5 (global 99): 53.4375 degrees
    db 90 ; MODEL_SENTINEL vertex 6 (global 100): 126.5625 degrees
    db 166 ; MODEL_SENTINEL vertex 7 (global 101): 233.4375 degrees
    db 245 ; MODEL_SENTINEL vertex 8 (global 102): 344.53125 degrees
    db 11 ; MODEL_SENTINEL vertex 9 (global 103): 15.46875 degrees
    db 64 ; MODEL_SENTINEL vertex 10 (global 104): 90 degrees
    db 90 ; MODEL_SENTINEL vertex 11 (global 105): 126.5625 degrees
    db 166 ; MODEL_SENTINEL vertex 12 (global 106): 233.4375 degrees
    db 192 ; MODEL_SENTINEL vertex 13 (global 107): 270 degrees
    db 218 ; MODEL_SENTINEL vertex 14 (global 108): 306.5625 degrees
    db 38 ; MODEL_SENTINEL vertex 15 (global 109): 53.4375 degrees
    db 90 ; MODEL_SENTINEL vertex 16 (global 110): 126.5625 degrees
    db 166 ; MODEL_SENTINEL vertex 17 (global 111): 233.4375 degrees
    db 245 ; MODEL_SENTINEL vertex 18 (global 112): 344.53125 degrees
    db 11 ; MODEL_SENTINEL vertex 19 (global 113): 15.46875 degrees
    db 83 ; MODEL_SENTINEL vertex 20 (global 114): 116.71875 degrees
    db 173 ; MODEL_SENTINEL vertex 21 (global 115): 243.28125 degrees
    db 248 ; MODEL_SENTINEL vertex 22 (global 116): 348.75 degrees
    db 8 ; MODEL_SENTINEL vertex 23 (global 117): 11.25 degrees
    db 226 ; MODEL_SENTINEL vertex 24 (global 118): 317.8125 degrees
    db 30 ; MODEL_SENTINEL vertex 25 (global 119): 42.1875 degrees
    db 220 ; MODEL_SENTINEL vertex 26 (global 120): 309.375 degrees
    db 36 ; MODEL_SENTINEL vertex 27 (global 121): 50.625 degrees
    db 102 ; MODEL_SENTINEL vertex 28 (global 122): 143.4375 degrees
    db 154 ; MODEL_SENTINEL vertex 29 (global 123): 216.5625 degrees
    db 224 ; MODEL_SENTINEL_PEDESTAL vertex 0 (global 124): 315 degrees
    db 32 ; MODEL_SENTINEL_PEDESTAL vertex 1 (global 125): 45 degrees
    db 96 ; MODEL_SENTINEL_PEDESTAL vertex 2 (global 126): 135 degrees
    db 160 ; MODEL_SENTINEL_PEDESTAL vertex 3 (global 127): 225 degrees
    db 212 ; MODEL_SENTINEL_PEDESTAL vertex 4 (global 128): 298.125 degrees
    db 236 ; MODEL_SENTINEL_PEDESTAL vertex 5 (global 129): 331.875 degrees
    db 20 ; MODEL_SENTINEL_PEDESTAL vertex 6 (global 130): 28.125 degrees
    db 44 ; MODEL_SENTINEL_PEDESTAL vertex 7 (global 131): 61.875 degrees
    db 84 ; MODEL_SENTINEL_PEDESTAL vertex 8 (global 132): 118.125 degrees
    db 108 ; MODEL_SENTINEL_PEDESTAL vertex 9 (global 133): 151.875 degrees
    db 148 ; MODEL_SENTINEL_PEDESTAL vertex 10 (global 134): 208.125 degrees
    db 172 ; MODEL_SENTINEL_PEDESTAL vertex 11 (global 135): 241.875 degrees
    db 224 ; MODEL_TEXT_BLOCK_01 vertex 0 (global 136): 315 degrees
    db 253 ; MODEL_TEXT_BLOCK_01 vertex 1 (global 137): 355.78125 degrees
    db 131 ; MODEL_TEXT_BLOCK_01 vertex 2 (global 138): 184.21875 degrees
    db 160 ; MODEL_TEXT_BLOCK_01 vertex 3 (global 139): 225 degrees
    db 224 ; MODEL_TEXT_BLOCK_01 vertex 4 (global 140): 315 degrees
    db 253 ; MODEL_TEXT_BLOCK_01 vertex 5 (global 141): 355.78125 degrees
    db 131 ; MODEL_TEXT_BLOCK_01 vertex 6 (global 142): 184.21875 degrees
    db 160 ; MODEL_TEXT_BLOCK_01 vertex 7 (global 143): 225 degrees
    db 3 ; MODEL_TEXT_BLOCK_10 vertex 0 (global 144): 4.21875 degrees
    db 32 ; MODEL_TEXT_BLOCK_10 vertex 1 (global 145): 45 degrees
    db 96 ; MODEL_TEXT_BLOCK_10 vertex 2 (global 146): 135 degrees
    db 125 ; MODEL_TEXT_BLOCK_10 vertex 3 (global 147): 175.78125 degrees
    db 3 ; MODEL_TEXT_BLOCK_10 vertex 4 (global 148): 4.21875 degrees
    db 32 ; MODEL_TEXT_BLOCK_10 vertex 5 (global 149): 45 degrees
    db 96 ; MODEL_TEXT_BLOCK_10 vertex 6 (global 150): 135 degrees
    db 125 ; MODEL_TEXT_BLOCK_10 vertex 7 (global 151): 175.78125 degrees
    db 224 ; MODEL_TEXT_BLOCK_11 vertex 0 (global 152): 315 degrees
    db 32 ; MODEL_TEXT_BLOCK_11 vertex 1 (global 153): 45 degrees
    db 96 ; MODEL_TEXT_BLOCK_11 vertex 2 (global 154): 135 degrees
    db 160 ; MODEL_TEXT_BLOCK_11 vertex 3 (global 155): 225 degrees
    db 224 ; MODEL_TEXT_BLOCK_11 vertex 4 (global 156): 315 degrees
    db 32 ; MODEL_TEXT_BLOCK_11 vertex 5 (global 157): 45 degrees
    db 96 ; MODEL_TEXT_BLOCK_11 vertex 6 (global 158): 135 degrees
    db 160 ; MODEL_TEXT_BLOCK_11 vertex 7 (global 159): 225 degrees

; opaque_vertex_azimuth_trailer: Five 32-byte initializer chunks after the 160 live polar azimuths. The ten half-open model vertex ranges end at source index 160, so CS:17DA does not read these as extra vertex angles. This trailer shares its complete chunk sequence with the Sentinel-loop trailer after that trailer's leading FFh byte. [load+0xc65a]
    opaque_init_chunk ZERO_WITH_20
    opaque_init_chunk ONES
    opaque_init_chunk ZERO
    opaque_init_chunk ONES_WITH_A0_EF
    opaque_init_chunk ZERO_WITH_20

; model_vertex_height: 160 sign-magnitude heights: world-cell offset = (-1 if bit7 else +1)*(byte&7Fh)/128. CS:184F doubles the magnitude into an 8.8 word and adds model_relative_height. This is not two-complement int8: 80h represents negative zero, while FFh represents -127/128 cell. [load+0xc6fa]
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_ROBOT vertex 0 (global 0): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_ROBOT vertex 1 (global 1): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_ROBOT vertex 2 (global 2): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_ROBOT vertex 3 (global 3): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 56 ; MODEL_ROBOT vertex 4 (global 4): height delta -112/256
    db VERTEX_HEIGHT_NEGATIVE | 56 ; MODEL_ROBOT vertex 5 (global 5): height delta -112/256
    db VERTEX_HEIGHT_NEGATIVE | 56 ; MODEL_ROBOT vertex 6 (global 6): height delta -112/256
    db VERTEX_HEIGHT_NEGATIVE | 56 ; MODEL_ROBOT vertex 7 (global 7): height delta -112/256
    db VERTEX_HEIGHT_NEGATIVE | 56 ; MODEL_ROBOT vertex 8 (global 8): height delta -112/256
    db VERTEX_HEIGHT_NEGATIVE | 56 ; MODEL_ROBOT vertex 9 (global 9): height delta -112/256
    db VERTEX_HEIGHT_NEGATIVE | 56 ; MODEL_ROBOT vertex 10 (global 10): height delta -112/256
    db VERTEX_HEIGHT_NEGATIVE | 56 ; MODEL_ROBOT vertex 11 (global 11): height delta -112/256
    db VERTEX_HEIGHT_NEGATIVE | 56 ; MODEL_ROBOT vertex 12 (global 12): height delta -112/256
    db VERTEX_HEIGHT_NEGATIVE | 18 ; MODEL_ROBOT vertex 13 (global 13): height delta -36/256
    db VERTEX_HEIGHT_NEGATIVE | 18 ; MODEL_ROBOT vertex 14 (global 14): height delta -36/256
    db VERTEX_HEIGHT_NEGATIVE | 18 ; MODEL_ROBOT vertex 15 (global 15): height delta -36/256
    db VERTEX_HEIGHT_NEGATIVE | 18 ; MODEL_ROBOT vertex 16 (global 16): height delta -36/256
    db VERTEX_HEIGHT_NEGATIVE | 10 ; MODEL_ROBOT vertex 17 (global 17): height delta -20/256
    db VERTEX_HEIGHT_NEGATIVE | 11 ; MODEL_ROBOT vertex 18 (global 18): height delta -22/256
    db VERTEX_HEIGHT_NEGATIVE | 11 ; MODEL_ROBOT vertex 19 (global 19): height delta -22/256
    db VERTEX_HEIGHT_NEGATIVE | 11 ; MODEL_ROBOT vertex 20 (global 20): height delta -22/256
    db VERTEX_HEIGHT_NEGATIVE | 11 ; MODEL_ROBOT vertex 21 (global 21): height delta -22/256
    db 8 ; MODEL_ROBOT vertex 22 (global 22): height delta 16/256
    db 8 ; MODEL_ROBOT vertex 23 (global 23): height delta 16/256
    db 8 ; MODEL_ROBOT vertex 24 (global 24): height delta 16/256
    db 8 ; MODEL_ROBOT vertex 25 (global 25): height delta 16/256
    db VERTEX_HEIGHT_NEGATIVE | 3 ; MODEL_ROBOT vertex 26 (global 26): height delta -6/256
    db VERTEX_HEIGHT_NEGATIVE | 3 ; MODEL_ROBOT vertex 27 (global 27): height delta -6/256
    db VERTEX_HEIGHT_NEGATIVE | 21 ; MODEL_ROBOT vertex 28 (global 28): height delta -42/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_SENTRY vertex 0 (global 29): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_SENTRY vertex 1 (global 30): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_SENTRY vertex 2 (global 31): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_SENTRY vertex 3 (global 32): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 24 ; MODEL_SENTRY vertex 4 (global 33): height delta -48/256
    db VERTEX_HEIGHT_NEGATIVE | 24 ; MODEL_SENTRY vertex 5 (global 34): height delta -48/256
    db VERTEX_HEIGHT_NEGATIVE | 27 ; MODEL_SENTRY vertex 6 (global 35): height delta -54/256
    db VERTEX_HEIGHT_NEGATIVE | 27 ; MODEL_SENTRY vertex 7 (global 36): height delta -54/256
    db VERTEX_HEIGHT_NEGATIVE | 27 ; MODEL_SENTRY vertex 8 (global 37): height delta -54/256
    db VERTEX_HEIGHT_NEGATIVE | 27 ; MODEL_SENTRY vertex 9 (global 38): height delta -54/256
    db VERTEX_HEIGHT_NEGATIVE | 8 ; MODEL_SENTRY vertex 10 (global 39): height delta -16/256
    db VERTEX_HEIGHT_NEGATIVE | 8 ; MODEL_SENTRY vertex 11 (global 40): height delta -16/256
    db VERTEX_HEIGHT_NEGATIVE | 8 ; MODEL_SENTRY vertex 12 (global 41): height delta -16/256
    db VERTEX_HEIGHT_NEGATIVE | 8 ; MODEL_SENTRY vertex 13 (global 42): height delta -16/256
    db VERTEX_HEIGHT_NEGATIVE | 8 ; MODEL_SENTRY vertex 14 (global 43): height delta -16/256
    db VERTEX_HEIGHT_NEGATIVE | 8 ; MODEL_SENTRY vertex 15 (global 44): height delta -16/256
    db 1 ; MODEL_SENTRY vertex 16 (global 45): height delta 2/256
    db 1 ; MODEL_SENTRY vertex 17 (global 46): height delta 2/256
    db 8 ; MODEL_SENTRY vertex 18 (global 47): height delta 16/256
    db 8 ; MODEL_SENTRY vertex 19 (global 48): height delta 16/256
    db 5 ; MODEL_SENTRY vertex 20 (global 49): height delta 10/256
    db 5 ; MODEL_SENTRY vertex 21 (global 50): height delta 10/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TREE vertex 0 (global 51): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TREE vertex 1 (global 52): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TREE vertex 2 (global 53): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TREE vertex 3 (global 54): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 72 ; MODEL_TREE vertex 4 (global 55): height delta -144/256
    db VERTEX_HEIGHT_NEGATIVE | 72 ; MODEL_TREE vertex 5 (global 56): height delta -144/256
    db VERTEX_HEIGHT_NEGATIVE | 72 ; MODEL_TREE vertex 6 (global 57): height delta -144/256
    db VERTEX_HEIGHT_NEGATIVE | 72 ; MODEL_TREE vertex 7 (global 58): height delta -144/256
    db VERTEX_HEIGHT_NEGATIVE | 72 ; MODEL_TREE vertex 8 (global 59): height delta -144/256
    db VERTEX_HEIGHT_NEGATIVE | 72 ; MODEL_TREE vertex 9 (global 60): height delta -144/256
    db VERTEX_HEIGHT_NEGATIVE | 72 ; MODEL_TREE vertex 10 (global 61): height delta -144/256
    db VERTEX_HEIGHT_NEGATIVE | 72 ; MODEL_TREE vertex 11 (global 62): height delta -144/256
    db VERTEX_HEIGHT_NEGATIVE | 72 ; MODEL_TREE vertex 12 (global 63): height delta -144/256
    db VERTEX_HEIGHT_NEGATIVE | 72 ; MODEL_TREE vertex 13 (global 64): height delta -144/256
    db VERTEX_HEIGHT_NEGATIVE | 72 ; MODEL_TREE vertex 14 (global 65): height delta -144/256
    db VERTEX_HEIGHT_NEGATIVE | 72 ; MODEL_TREE vertex 15 (global 66): height delta -144/256
    db 120 ; MODEL_TREE vertex 16 (global 67): height delta 240/256
    db VERTEX_HEIGHT_NEGATIVE | 48 ; MODEL_BOULDER vertex 0 (global 68): height delta -96/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_BOULDER vertex 1 (global 69): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 48 ; MODEL_BOULDER vertex 2 (global 70): height delta -96/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_BOULDER vertex 3 (global 71): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 48 ; MODEL_BOULDER vertex 4 (global 72): height delta -96/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_BOULDER vertex 5 (global 73): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 48 ; MODEL_BOULDER vertex 6 (global 74): height delta -96/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_BOULDER vertex 7 (global 75): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_MEANIE vertex 0 (global 76): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_MEANIE vertex 1 (global 77): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_MEANIE vertex 2 (global 78): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 60 ; MODEL_MEANIE vertex 3 (global 79): height delta -120/256
    db VERTEX_HEIGHT_NEGATIVE | 60 ; MODEL_MEANIE vertex 4 (global 80): height delta -120/256
    db VERTEX_HEIGHT_NEGATIVE | 60 ; MODEL_MEANIE vertex 5 (global 81): height delta -120/256
    db VERTEX_HEIGHT_NEGATIVE | 10 ; MODEL_MEANIE vertex 6 (global 82): height delta -20/256
    db VERTEX_HEIGHT_NEGATIVE | 10 ; MODEL_MEANIE vertex 7 (global 83): height delta -20/256
    db 1 ; MODEL_MEANIE vertex 8 (global 84): height delta 2/256
    db 1 ; MODEL_MEANIE vertex 9 (global 85): height delta 2/256
    db VERTEX_HEIGHT_NEGATIVE | 8 ; MODEL_MEANIE vertex 10 (global 86): height delta -16/256
    db VERTEX_HEIGHT_NEGATIVE | 14 ; MODEL_MEANIE vertex 11 (global 87): height delta -28/256
    db VERTEX_HEIGHT_NEGATIVE | 14 ; MODEL_MEANIE vertex 12 (global 88): height delta -28/256
    db VERTEX_HEIGHT_NEGATIVE | 8 ; MODEL_MEANIE vertex 13 (global 89): height delta -16/256
    db VERTEX_HEIGHT_NEGATIVE | 5 ; MODEL_MEANIE vertex 14 (global 90): height delta -10/256
    db VERTEX_HEIGHT_NEGATIVE | 5 ; MODEL_MEANIE vertex 15 (global 91): height delta -10/256
    db 6 ; MODEL_MEANIE vertex 16 (global 92): height delta 12/256
    db 6 ; MODEL_MEANIE vertex 17 (global 93): height delta 12/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_SENTINEL vertex 0 (global 94): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_SENTINEL vertex 1 (global 95): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_SENTINEL vertex 2 (global 96): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_SENTINEL vertex 3 (global 97): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 96 ; MODEL_SENTINEL vertex 4 (global 98): height delta -192/256
    db VERTEX_HEIGHT_NEGATIVE | 96 ; MODEL_SENTINEL vertex 5 (global 99): height delta -192/256
    db VERTEX_HEIGHT_NEGATIVE | 96 ; MODEL_SENTINEL vertex 6 (global 100): height delta -192/256
    db VERTEX_HEIGHT_NEGATIVE | 96 ; MODEL_SENTINEL vertex 7 (global 101): height delta -192/256
    db VERTEX_HEIGHT_NEGATIVE | 8 ; MODEL_SENTINEL vertex 8 (global 102): height delta -16/256
    db VERTEX_HEIGHT_NEGATIVE | 8 ; MODEL_SENTINEL vertex 9 (global 103): height delta -16/256
    db VERTEX_HEIGHT_NEGATIVE | 11 ; MODEL_SENTINEL vertex 10 (global 104): height delta -22/256
    db VERTEX_HEIGHT_NEGATIVE | 11 ; MODEL_SENTINEL vertex 11 (global 105): height delta -22/256
    db VERTEX_HEIGHT_NEGATIVE | 11 ; MODEL_SENTINEL vertex 12 (global 106): height delta -22/256
    db VERTEX_HEIGHT_NEGATIVE | 11 ; MODEL_SENTINEL vertex 13 (global 107): height delta -22/256
    db 8 ; MODEL_SENTINEL vertex 14 (global 108): height delta 16/256
    db 8 ; MODEL_SENTINEL vertex 15 (global 109): height delta 16/256
    db 5 ; MODEL_SENTINEL vertex 16 (global 110): height delta 10/256
    db 5 ; MODEL_SENTINEL vertex 17 (global 111): height delta 10/256
    db 8 ; MODEL_SENTINEL vertex 18 (global 112): height delta 16/256
    db 8 ; MODEL_SENTINEL vertex 19 (global 113): height delta 16/256
    db 2 ; MODEL_SENTINEL vertex 20 (global 114): height delta 4/256
    db 2 ; MODEL_SENTINEL vertex 21 (global 115): height delta 4/256
    db 17 ; MODEL_SENTINEL vertex 22 (global 116): height delta 34/256
    db 17 ; MODEL_SENTINEL vertex 23 (global 117): height delta 34/256
    db 24 ; MODEL_SENTINEL vertex 24 (global 118): height delta 48/256
    db 24 ; MODEL_SENTINEL vertex 25 (global 119): height delta 48/256
    db 39 ; MODEL_SENTINEL vertex 26 (global 120): height delta 78/256
    db 39 ; MODEL_SENTINEL vertex 27 (global 121): height delta 78/256
    db 39 ; MODEL_SENTINEL vertex 28 (global 122): height delta 78/256
    db 39 ; MODEL_SENTINEL vertex 29 (global 123): height delta 78/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_SENTINEL_PEDESTAL vertex 0 (global 124): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_SENTINEL_PEDESTAL vertex 1 (global 125): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_SENTINEL_PEDESTAL vertex 2 (global 126): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_SENTINEL_PEDESTAL vertex 3 (global 127): height delta -224/256
    db 16 ; MODEL_SENTINEL_PEDESTAL vertex 4 (global 128): height delta 32/256
    db 16 ; MODEL_SENTINEL_PEDESTAL vertex 5 (global 129): height delta 32/256
    db 16 ; MODEL_SENTINEL_PEDESTAL vertex 6 (global 130): height delta 32/256
    db 16 ; MODEL_SENTINEL_PEDESTAL vertex 7 (global 131): height delta 32/256
    db 16 ; MODEL_SENTINEL_PEDESTAL vertex 8 (global 132): height delta 32/256
    db 16 ; MODEL_SENTINEL_PEDESTAL vertex 9 (global 133): height delta 32/256
    db 16 ; MODEL_SENTINEL_PEDESTAL vertex 10 (global 134): height delta 32/256
    db 16 ; MODEL_SENTINEL_PEDESTAL vertex 11 (global 135): height delta 32/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TEXT_BLOCK_01 vertex 0 (global 136): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TEXT_BLOCK_01 vertex 1 (global 137): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TEXT_BLOCK_01 vertex 2 (global 138): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TEXT_BLOCK_01 vertex 3 (global 139): height delta -224/256
    db 112 ; MODEL_TEXT_BLOCK_01 vertex 4 (global 140): height delta 224/256
    db 112 ; MODEL_TEXT_BLOCK_01 vertex 5 (global 141): height delta 224/256
    db 112 ; MODEL_TEXT_BLOCK_01 vertex 6 (global 142): height delta 224/256
    db 112 ; MODEL_TEXT_BLOCK_01 vertex 7 (global 143): height delta 224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TEXT_BLOCK_10 vertex 0 (global 144): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TEXT_BLOCK_10 vertex 1 (global 145): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TEXT_BLOCK_10 vertex 2 (global 146): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TEXT_BLOCK_10 vertex 3 (global 147): height delta -224/256
    db 112 ; MODEL_TEXT_BLOCK_10 vertex 4 (global 148): height delta 224/256
    db 112 ; MODEL_TEXT_BLOCK_10 vertex 5 (global 149): height delta 224/256
    db 112 ; MODEL_TEXT_BLOCK_10 vertex 6 (global 150): height delta 224/256
    db 112 ; MODEL_TEXT_BLOCK_10 vertex 7 (global 151): height delta 224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TEXT_BLOCK_11 vertex 0 (global 152): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TEXT_BLOCK_11 vertex 1 (global 153): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TEXT_BLOCK_11 vertex 2 (global 154): height delta -224/256
    db VERTEX_HEIGHT_NEGATIVE | 112 ; MODEL_TEXT_BLOCK_11 vertex 3 (global 155): height delta -224/256
    db 112 ; MODEL_TEXT_BLOCK_11 vertex 4 (global 156): height delta 224/256
    db 112 ; MODEL_TEXT_BLOCK_11 vertex 5 (global 157): height delta 224/256
    db 112 ; MODEL_TEXT_BLOCK_11 vertex 6 (global 158): height delta 224/256
    db 112 ; MODEL_TEXT_BLOCK_11 vertex 7 (global 159): height delta 224/256

; opaque_vertex_height_trailer: Five 32-byte initializer chunks after the 160 live sign-magnitude heights. These bytes are outside the source-vertex ranges and are not decoded as additional model heights. Unlike the azimuth trailer, the middle zero chunk has no 20h marker. [load+0xc79a]
    opaque_init_chunk ZERO
    opaque_init_chunk ONES_WITH_A0_EF
    opaque_init_chunk ZERO
    opaque_init_chunk ONES
    opaque_init_chunk ZERO

; model_vertex_radius: 160 unsigned polar radii in 1/256 terrain-cell units. CS:17DA multiplies radius by a 0..255 component, discards the low product byte, then interprets the signed component as an 8.8 offset. Quantization occurs before bearing projection; height uses a separate /128 scale. [load+0xc83a]
    db 20 ; MODEL_ROBOT vertex 0 (global 0): unsigned radial factor
    db 20 ; MODEL_ROBOT vertex 1 (global 1): unsigned radial factor
    db 27 ; MODEL_ROBOT vertex 2 (global 2): unsigned radial factor
    db 27 ; MODEL_ROBOT vertex 3 (global 3): unsigned radial factor
    db 31 ; MODEL_ROBOT vertex 4 (global 4): unsigned radial factor
    db 31 ; MODEL_ROBOT vertex 5 (global 5): unsigned radial factor
    db 35 ; MODEL_ROBOT vertex 6 (global 6): unsigned radial factor
    db 35 ; MODEL_ROBOT vertex 7 (global 7): unsigned radial factor
    db 42 ; MODEL_ROBOT vertex 8 (global 8): unsigned radial factor
    db 32 ; MODEL_ROBOT vertex 9 (global 9): unsigned radial factor
    db 42 ; MODEL_ROBOT vertex 10 (global 10): unsigned radial factor
    db 48 ; MODEL_ROBOT vertex 11 (global 11): unsigned radial factor
    db 48 ; MODEL_ROBOT vertex 12 (global 12): unsigned radial factor
    db 52 ; MODEL_ROBOT vertex 13 (global 13): unsigned radial factor
    db 52 ; MODEL_ROBOT vertex 14 (global 14): unsigned radial factor
    db 56 ; MODEL_ROBOT vertex 15 (global 15): unsigned radial factor
    db 56 ; MODEL_ROBOT vertex 16 (global 16): unsigned radial factor
    db 0 ; MODEL_ROBOT vertex 17 (global 17): unsigned radial factor
    db 19 ; MODEL_ROBOT vertex 18 (global 18): unsigned radial factor
    db 19 ; MODEL_ROBOT vertex 19 (global 19): unsigned radial factor
    db 29 ; MODEL_ROBOT vertex 20 (global 20): unsigned radial factor
    db 29 ; MODEL_ROBOT vertex 21 (global 21): unsigned radial factor
    db 12 ; MODEL_ROBOT vertex 22 (global 22): unsigned radial factor
    db 12 ; MODEL_ROBOT vertex 23 (global 23): unsigned radial factor
    db 19 ; MODEL_ROBOT vertex 24 (global 24): unsigned radial factor
    db 19 ; MODEL_ROBOT vertex 25 (global 25): unsigned radial factor
    db 28 ; MODEL_ROBOT vertex 26 (global 26): unsigned radial factor
    db 28 ; MODEL_ROBOT vertex 27 (global 27): unsigned radial factor
    db 32 ; MODEL_ROBOT vertex 28 (global 28): unsigned radial factor
    db 42 ; MODEL_SENTRY vertex 0 (global 29): unsigned radial factor
    db 42 ; MODEL_SENTRY vertex 1 (global 30): unsigned radial factor
    db 42 ; MODEL_SENTRY vertex 2 (global 31): unsigned radial factor
    db 42 ; MODEL_SENTRY vertex 3 (global 32): unsigned radial factor
    db 50 ; MODEL_SENTRY vertex 4 (global 33): unsigned radial factor
    db 50 ; MODEL_SENTRY vertex 5 (global 34): unsigned radial factor
    db 54 ; MODEL_SENTRY vertex 6 (global 35): unsigned radial factor
    db 42 ; MODEL_SENTRY vertex 7 (global 36): unsigned radial factor
    db 42 ; MODEL_SENTRY vertex 8 (global 37): unsigned radial factor
    db 54 ; MODEL_SENTRY vertex 9 (global 38): unsigned radial factor
    db 34 ; MODEL_SENTRY vertex 10 (global 39): unsigned radial factor
    db 34 ; MODEL_SENTRY vertex 11 (global 40): unsigned radial factor
    db 34 ; MODEL_SENTRY vertex 12 (global 41): unsigned radial factor
    db 34 ; MODEL_SENTRY vertex 13 (global 42): unsigned radial factor
    db 64 ; MODEL_SENTRY vertex 14 (global 43): unsigned radial factor
    db 64 ; MODEL_SENTRY vertex 15 (global 44): unsigned radial factor
    db 63 ; MODEL_SENTRY vertex 16 (global 45): unsigned radial factor
    db 63 ; MODEL_SENTRY vertex 17 (global 46): unsigned radial factor
    db 34 ; MODEL_SENTRY vertex 18 (global 47): unsigned radial factor
    db 34 ; MODEL_SENTRY vertex 19 (global 48): unsigned radial factor
    db 31 ; MODEL_SENTRY vertex 20 (global 49): unsigned radial factor
    db 31 ; MODEL_SENTRY vertex 21 (global 50): unsigned radial factor
    db 24 ; MODEL_TREE vertex 0 (global 51): unsigned radial factor
    db 24 ; MODEL_TREE vertex 1 (global 52): unsigned radial factor
    db 24 ; MODEL_TREE vertex 2 (global 53): unsigned radial factor
    db 24 ; MODEL_TREE vertex 3 (global 54): unsigned radial factor
    db 24 ; MODEL_TREE vertex 4 (global 55): unsigned radial factor
    db 24 ; MODEL_TREE vertex 5 (global 56): unsigned radial factor
    db 24 ; MODEL_TREE vertex 6 (global 57): unsigned radial factor
    db 24 ; MODEL_TREE vertex 7 (global 58): unsigned radial factor
    db 104 ; MODEL_TREE vertex 8 (global 59): unsigned radial factor
    db 104 ; MODEL_TREE vertex 9 (global 60): unsigned radial factor
    db 104 ; MODEL_TREE vertex 10 (global 61): unsigned radial factor
    db 104 ; MODEL_TREE vertex 11 (global 62): unsigned radial factor
    db 104 ; MODEL_TREE vertex 12 (global 63): unsigned radial factor
    db 104 ; MODEL_TREE vertex 13 (global 64): unsigned radial factor
    db 104 ; MODEL_TREE vertex 14 (global 65): unsigned radial factor
    db 104 ; MODEL_TREE vertex 15 (global 66): unsigned radial factor
    db 0 ; MODEL_TREE vertex 16 (global 67): unsigned radial factor
    db 112 ; MODEL_BOULDER vertex 0 (global 68): unsigned radial factor
    db 112 ; MODEL_BOULDER vertex 1 (global 69): unsigned radial factor
    db 112 ; MODEL_BOULDER vertex 2 (global 70): unsigned radial factor
    db 112 ; MODEL_BOULDER vertex 3 (global 71): unsigned radial factor
    db 112 ; MODEL_BOULDER vertex 4 (global 72): unsigned radial factor
    db 112 ; MODEL_BOULDER vertex 5 (global 73): unsigned radial factor
    db 112 ; MODEL_BOULDER vertex 6 (global 74): unsigned radial factor
    db 112 ; MODEL_BOULDER vertex 7 (global 75): unsigned radial factor
    db 52 ; MODEL_MEANIE vertex 0 (global 76): unsigned radial factor
    db 68 ; MODEL_MEANIE vertex 1 (global 77): unsigned radial factor
    db 68 ; MODEL_MEANIE vertex 2 (global 78): unsigned radial factor
    db 26 ; MODEL_MEANIE vertex 3 (global 79): unsigned radial factor
    db 41 ; MODEL_MEANIE vertex 4 (global 80): unsigned radial factor
    db 41 ; MODEL_MEANIE vertex 5 (global 81): unsigned radial factor
    db 12 ; MODEL_MEANIE vertex 6 (global 82): unsigned radial factor
    db 12 ; MODEL_MEANIE vertex 7 (global 83): unsigned radial factor
    db 23 ; MODEL_MEANIE vertex 8 (global 84): unsigned radial factor
    db 23 ; MODEL_MEANIE vertex 9 (global 85): unsigned radial factor
    db 46 ; MODEL_MEANIE vertex 10 (global 86): unsigned radial factor
    db 54 ; MODEL_MEANIE vertex 11 (global 87): unsigned radial factor
    db 54 ; MODEL_MEANIE vertex 12 (global 88): unsigned radial factor
    db 46 ; MODEL_MEANIE vertex 13 (global 89): unsigned radial factor
    db 58 ; MODEL_MEANIE vertex 14 (global 90): unsigned radial factor
    db 58 ; MODEL_MEANIE vertex 15 (global 91): unsigned radial factor
    db 34 ; MODEL_MEANIE vertex 16 (global 92): unsigned radial factor
    db 34 ; MODEL_MEANIE vertex 17 (global 93): unsigned radial factor
    db 56 ; MODEL_SENTINEL vertex 0 (global 94): unsigned radial factor
    db 56 ; MODEL_SENTINEL vertex 1 (global 95): unsigned radial factor
    db 56 ; MODEL_SENTINEL vertex 2 (global 96): unsigned radial factor
    db 56 ; MODEL_SENTINEL vertex 3 (global 97): unsigned radial factor
    db 42 ; MODEL_SENTINEL vertex 4 (global 98): unsigned radial factor
    db 42 ; MODEL_SENTINEL vertex 5 (global 99): unsigned radial factor
    db 42 ; MODEL_SENTINEL vertex 6 (global 100): unsigned radial factor
    db 42 ; MODEL_SENTINEL vertex 7 (global 101): unsigned radial factor
    db 50 ; MODEL_SENTINEL vertex 8 (global 102): unsigned radial factor
    db 50 ; MODEL_SENTINEL vertex 9 (global 103): unsigned radial factor
    db 54 ; MODEL_SENTINEL vertex 10 (global 104): unsigned radial factor
    db 42 ; MODEL_SENTINEL vertex 11 (global 105): unsigned radial factor
    db 42 ; MODEL_SENTINEL vertex 12 (global 106): unsigned radial factor
    db 54 ; MODEL_SENTINEL vertex 13 (global 107): unsigned radial factor
    db 34 ; MODEL_SENTINEL vertex 14 (global 108): unsigned radial factor
    db 34 ; MODEL_SENTINEL vertex 15 (global 109): unsigned radial factor
    db 34 ; MODEL_SENTINEL vertex 16 (global 110): unsigned radial factor
    db 34 ; MODEL_SENTINEL vertex 17 (global 111): unsigned radial factor
    db 64 ; MODEL_SENTINEL vertex 18 (global 112): unsigned radial factor
    db 64 ; MODEL_SENTINEL vertex 19 (global 113): unsigned radial factor
    db 70 ; MODEL_SENTINEL vertex 20 (global 114): unsigned radial factor
    db 70 ; MODEL_SENTINEL vertex 21 (global 115): unsigned radial factor
    db 63 ; MODEL_SENTINEL vertex 22 (global 116): unsigned radial factor
    db 63 ; MODEL_SENTINEL vertex 23 (global 117): unsigned radial factor
    db 34 ; MODEL_SENTINEL vertex 24 (global 118): unsigned radial factor
    db 34 ; MODEL_SENTINEL vertex 25 (global 119): unsigned radial factor
    db 19 ; MODEL_SENTINEL vertex 26 (global 120): unsigned radial factor
    db 19 ; MODEL_SENTINEL vertex 27 (global 121): unsigned radial factor
    db 24 ; MODEL_SENTINEL vertex 28 (global 122): unsigned radial factor
    db 24 ; MODEL_SENTINEL vertex 29 (global 123): unsigned radial factor
    db 181 ; MODEL_SENTINEL_PEDESTAL vertex 0 (global 124): unsigned radial factor
    db 181 ; MODEL_SENTINEL_PEDESTAL vertex 1 (global 125): unsigned radial factor
    db 181 ; MODEL_SENTINEL_PEDESTAL vertex 2 (global 126): unsigned radial factor
    db 181 ; MODEL_SENTINEL_PEDESTAL vertex 3 (global 127): unsigned radial factor
    db 145 ; MODEL_SENTINEL_PEDESTAL vertex 4 (global 128): unsigned radial factor
    db 145 ; MODEL_SENTINEL_PEDESTAL vertex 5 (global 129): unsigned radial factor
    db 145 ; MODEL_SENTINEL_PEDESTAL vertex 6 (global 130): unsigned radial factor
    db 145 ; MODEL_SENTINEL_PEDESTAL vertex 7 (global 131): unsigned radial factor
    db 145 ; MODEL_SENTINEL_PEDESTAL vertex 8 (global 132): unsigned radial factor
    db 145 ; MODEL_SENTINEL_PEDESTAL vertex 9 (global 133): unsigned radial factor
    db 145 ; MODEL_SENTINEL_PEDESTAL vertex 10 (global 134): unsigned radial factor
    db 145 ; MODEL_SENTINEL_PEDESTAL vertex 11 (global 135): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_01 vertex 0 (global 136): unsigned radial factor
    db 136 ; MODEL_TEXT_BLOCK_01 vertex 1 (global 137): unsigned radial factor
    db 136 ; MODEL_TEXT_BLOCK_01 vertex 2 (global 138): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_01 vertex 3 (global 139): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_01 vertex 4 (global 140): unsigned radial factor
    db 136 ; MODEL_TEXT_BLOCK_01 vertex 5 (global 141): unsigned radial factor
    db 136 ; MODEL_TEXT_BLOCK_01 vertex 6 (global 142): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_01 vertex 7 (global 143): unsigned radial factor
    db 136 ; MODEL_TEXT_BLOCK_10 vertex 0 (global 144): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_10 vertex 1 (global 145): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_10 vertex 2 (global 146): unsigned radial factor
    db 136 ; MODEL_TEXT_BLOCK_10 vertex 3 (global 147): unsigned radial factor
    db 136 ; MODEL_TEXT_BLOCK_10 vertex 4 (global 148): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_10 vertex 5 (global 149): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_10 vertex 6 (global 150): unsigned radial factor
    db 136 ; MODEL_TEXT_BLOCK_10 vertex 7 (global 151): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_11 vertex 0 (global 152): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_11 vertex 1 (global 153): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_11 vertex 2 (global 154): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_11 vertex 3 (global 155): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_11 vertex 4 (global 156): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_11 vertex 5 (global 157): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_11 vertex 6 (global 158): unsigned radial factor
    db 192 ; MODEL_TEXT_BLOCK_11 vertex 7 (global 159): unsigned radial factor

; opaque_vertex_radius_trailer: Five 32-byte initializer chunks after the 160 live polar radii. The ONES_WITH_A0_EF chunk occurs fourth here, whereas it occurs second in the height trailer. The renderer's vertex-range endpoints bound radius reads before this block. [load+0xc8da]
    opaque_init_chunk ZERO
    opaque_init_chunk ONES
    opaque_init_chunk ZERO
    opaque_init_chunk ONES_WITH_A0_EF
    opaque_init_chunk ZERO

; model_face_color_pairs: 160 face-indexed two-color selections; high/low nibbles name palette slots used by adapter span/dither filling, not fixed RGB colors [load+0xc97a]
    db (0 << 4) | 0 ; MODEL_ROBOT face 0 (global 0): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_ROBOT face 1 (global 1): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_ROBOT face 2 (global 2): palette slots 0/0
    db (0 << 4) | 2 ; MODEL_ROBOT face 3 (global 3): palette slots 0/2
    db (0 << 4) | 2 ; MODEL_ROBOT face 4 (global 4): palette slots 0/2
    db (2 << 4) | 0 ; MODEL_ROBOT face 5 (global 5): palette slots 2/0
    db (2 << 4) | 0 ; MODEL_ROBOT face 6 (global 6): palette slots 2/0
    db (9 << 4) | 9 ; MODEL_ROBOT face 7 (global 7): palette slots 9/9
    db (9 << 4) | 9 ; MODEL_ROBOT face 8 (global 8): palette slots 9/9
    db (12 << 4) | 12 ; MODEL_ROBOT face 9 (global 9): palette slots 12/12
    db (0 << 4) | 12 ; MODEL_ROBOT face 10 (global 10): palette slots 0/12
    db (0 << 4) | 12 ; MODEL_ROBOT face 11 (global 11): palette slots 0/12
    db (0 << 4) | 0 ; MODEL_ROBOT face 12 (global 12): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_ROBOT face 13 (global 13): palette slots 0/0
    db (0 << 4) | 12 ; MODEL_ROBOT face 14 (global 14): palette slots 0/12
    db (0 << 4) | 12 ; MODEL_ROBOT face 15 (global 15): palette slots 0/12
    db (9 << 4) | 9 ; MODEL_ROBOT face 16 (global 16): palette slots 9/9
    db (9 << 4) | 9 ; MODEL_ROBOT face 17 (global 17): palette slots 9/9
    db (0 << 4) | 12 ; MODEL_ROBOT face 18 (global 18): palette slots 0/12
    db (0 << 4) | 0 ; MODEL_ROBOT face 19 (global 19): palette slots 0/0
    db (6 << 4) | 6 ; MODEL_ROBOT face 20 (global 20): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_ROBOT face 21 (global 21): palette slots 6/6
    db (9 << 4) | 9 ; MODEL_ROBOT face 22 (global 22): palette slots 9/9
    db (9 << 4) | 9 ; MODEL_ROBOT face 23 (global 23): palette slots 9/9
    db (0 << 4) | 0 ; MODEL_ROBOT face 24 (global 24): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_ROBOT face 25 (global 25): palette slots 0/0
    db (9 << 4) | 9 ; MODEL_ROBOT face 26 (global 26): palette slots 9/9
    db (0 << 4) | 0 ; MODEL_SENTRY face 0 (global 27): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTRY face 1 (global 28): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTRY face 2 (global 29): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTRY face 3 (global 30): palette slots 0/0
    db (6 << 4) | 6 ; MODEL_SENTRY face 4 (global 31): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTRY face 5 (global 32): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTRY face 6 (global 33): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTRY face 7 (global 34): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTRY face 8 (global 35): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTRY face 9 (global 36): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTRY face 10 (global 37): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTRY face 11 (global 38): palette slots 6/6
    db (0 << 4) | 0 ; MODEL_SENTRY face 12 (global 39): palette slots 0/0
    db (6 << 4) | 6 ; MODEL_SENTRY face 13 (global 40): palette slots 6/6
    db (0 << 4) | 0 ; MODEL_SENTRY face 14 (global 41): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTRY face 15 (global 42): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTRY face 16 (global 43): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTRY face 17 (global 44): palette slots 0/0
    db (6 << 4) | 6 ; MODEL_SENTRY face 18 (global 45): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTRY face 19 (global 46): palette slots 6/6
    db (0 << 4) | 0 ; MODEL_SENTRY face 20 (global 47): palette slots 0/0
    db (0 << 4) | 9 ; MODEL_SENTRY face 21 (global 48): palette slots 0/9
    db (0 << 4) | 9 ; MODEL_SENTRY face 22 (global 49): palette slots 0/9
    db (0 << 4) | 9 ; MODEL_SENTRY face 23 (global 50): palette slots 0/9
    db (0 << 4) | 9 ; MODEL_SENTRY face 24 (global 51): palette slots 0/9
    db (0 << 4) | 0 ; MODEL_TREE face 0 (global 52): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_TREE face 1 (global 53): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_TREE face 2 (global 54): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_TREE face 3 (global 55): palette slots 0/0
    db (13 << 4) | 13 ; MODEL_TREE face 4 (global 56): palette slots 13/13
    db (13 << 4) | 13 ; MODEL_TREE face 5 (global 57): palette slots 13/13
    db (0 << 4) | 0 ; MODEL_TREE face 6 (global 58): palette slots 0/0
    db (4 << 4) | 4 ; MODEL_TREE face 7 (global 59): palette slots 4/4
    db (5 << 4) | 5 ; MODEL_TREE face 8 (global 60): palette slots 5/5
    db (4 << 4) | 4 ; MODEL_TREE face 9 (global 61): palette slots 4/4
    db (5 << 4) | 5 ; MODEL_TREE face 10 (global 62): palette slots 5/5
    db (4 << 4) | 4 ; MODEL_TREE face 11 (global 63): palette slots 4/4
    db (5 << 4) | 5 ; MODEL_TREE face 12 (global 64): palette slots 5/5
    db (4 << 4) | 4 ; MODEL_TREE face 13 (global 65): palette slots 4/4
    db (5 << 4) | 5 ; MODEL_TREE face 14 (global 66): palette slots 5/5
    db (15 << 4) | 15 ; MODEL_BOULDER face 0 (global 67): palette slots 15/15
    db (15 << 4) | 15 ; MODEL_BOULDER face 1 (global 68): palette slots 15/15
    db (15 << 4) | 15 ; MODEL_BOULDER face 2 (global 69): palette slots 15/15
    db (15 << 4) | 15 ; MODEL_BOULDER face 3 (global 70): palette slots 15/15
    db (11 << 4) | 11 ; MODEL_BOULDER face 4 (global 71): palette slots 11/11
    db (11 << 4) | 11 ; MODEL_BOULDER face 5 (global 72): palette slots 11/11
    db (11 << 4) | 11 ; MODEL_BOULDER face 6 (global 73): palette slots 11/11
    db (11 << 4) | 11 ; MODEL_BOULDER face 7 (global 74): palette slots 11/11
    db (0 << 4) | 0 ; MODEL_BOULDER face 8 (global 75): palette slots 0/0
    db (15 << 4) | 15 ; MODEL_BOULDER face 9 (global 76): palette slots 15/15
    db (0 << 4) | 0 ; MODEL_MEANIE face 0 (global 77): palette slots 0/0
    db (13 << 4) | 13 ; MODEL_MEANIE face 1 (global 78): palette slots 13/13
    db (13 << 4) | 13 ; MODEL_MEANIE face 2 (global 79): palette slots 13/13
    db (0 << 4) | 3 ; MODEL_MEANIE face 3 (global 80): palette slots 0/3
    db (0 << 4) | 3 ; MODEL_MEANIE face 4 (global 81): palette slots 0/3
    db (0 << 4) | 0 ; MODEL_MEANIE face 5 (global 82): palette slots 0/0
    db (5 << 4) | 5 ; MODEL_MEANIE face 6 (global 83): palette slots 5/5
    db (5 << 4) | 5 ; MODEL_MEANIE face 7 (global 84): palette slots 5/5
    db (4 << 4) | 4 ; MODEL_MEANIE face 8 (global 85): palette slots 4/4
    db (0 << 4) | 0 ; MODEL_MEANIE face 9 (global 86): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_MEANIE face 10 (global 87): palette slots 0/0
    db (0 << 4) | 2 ; MODEL_MEANIE face 11 (global 88): palette slots 0/2
    db (0 << 4) | 2 ; MODEL_MEANIE face 12 (global 89): palette slots 0/2
    db (0 << 4) | 2 ; MODEL_MEANIE face 13 (global 90): palette slots 0/2
    db (0 << 4) | 0 ; MODEL_MEANIE face 14 (global 91): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_MEANIE face 15 (global 92): palette slots 0/0
    db (9 << 4) | 9 ; MODEL_MEANIE face 16 (global 93): palette slots 9/9
    db (0 << 4) | 2 ; MODEL_MEANIE face 17 (global 94): palette slots 0/2
    db (0 << 4) | 2 ; MODEL_MEANIE face 18 (global 95): palette slots 0/2
    db (0 << 4) | 0 ; MODEL_MEANIE face 19 (global 96): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_MEANIE face 20 (global 97): palette slots 0/0
    db (9 << 4) | 9 ; MODEL_MEANIE face 21 (global 98): palette slots 9/9
    db (9 << 4) | 9 ; MODEL_MEANIE face 22 (global 99): palette slots 9/9
    db (0 << 4) | 0 ; MODEL_MEANIE face 23 (global 100): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_MEANIE face 24 (global 101): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 0 (global 102): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 1 (global 103): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 2 (global 104): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 3 (global 105): palette slots 0/0
    db (6 << 4) | 6 ; MODEL_SENTINEL face 4 (global 106): palette slots 6/6
    db (0 << 4) | 0 ; MODEL_SENTINEL face 5 (global 107): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 6 (global 108): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 7 (global 109): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 8 (global 110): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 9 (global 111): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 10 (global 112): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 11 (global 113): palette slots 0/0
    db (6 << 4) | 6 ; MODEL_SENTINEL face 12 (global 114): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTINEL face 13 (global 115): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTINEL face 14 (global 116): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTINEL face 15 (global 117): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTINEL face 16 (global 118): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTINEL face 17 (global 119): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTINEL face 18 (global 120): palette slots 6/6
    db (0 << 4) | 0 ; MODEL_SENTINEL face 19 (global 121): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 20 (global 122): palette slots 0/0
    db (3 << 4) | 3 ; MODEL_SENTINEL face 21 (global 123): palette slots 3/3
    db (3 << 4) | 3 ; MODEL_SENTINEL face 22 (global 124): palette slots 3/3
    db (0 << 4) | 0 ; MODEL_SENTINEL face 23 (global 125): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 24 (global 126): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 25 (global 127): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 26 (global 128): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_SENTINEL face 27 (global 129): palette slots 0/0
    db (0 << 4) | 9 ; MODEL_SENTINEL face 28 (global 130): palette slots 0/9
    db (0 << 4) | 9 ; MODEL_SENTINEL face 29 (global 131): palette slots 0/9
    db (0 << 4) | 9 ; MODEL_SENTINEL face 30 (global 132): palette slots 0/9
    db (0 << 4) | 9 ; MODEL_SENTINEL face 31 (global 133): palette slots 0/9
    db (0 << 4) | 0 ; MODEL_SENTINEL face 32 (global 134): palette slots 0/0
    db (6 << 4) | 6 ; MODEL_SENTINEL face 33 (global 135): palette slots 6/6
    db (6 << 4) | 6 ; MODEL_SENTINEL face 34 (global 136): palette slots 6/6
    db (0 << 4) | 0 ; MODEL_SENTINEL_PEDESTAL face 0 (global 137): palette slots 0/0
    db (2 << 4) | 2 ; MODEL_SENTINEL_PEDESTAL face 1 (global 138): palette slots 2/2
    db (0 << 4) | 0 ; MODEL_SENTINEL_PEDESTAL face 2 (global 139): palette slots 0/0
    db (2 << 4) | 2 ; MODEL_SENTINEL_PEDESTAL face 3 (global 140): palette slots 2/2
    db (9 << 4) | 9 ; MODEL_SENTINEL_PEDESTAL face 4 (global 141): palette slots 9/9
    db (9 << 4) | 9 ; MODEL_SENTINEL_PEDESTAL face 5 (global 142): palette slots 9/9
    db (9 << 4) | 9 ; MODEL_SENTINEL_PEDESTAL face 6 (global 143): palette slots 9/9
    db (9 << 4) | 9 ; MODEL_SENTINEL_PEDESTAL face 7 (global 144): palette slots 9/9
    db (3 << 4) | 3 ; MODEL_SENTINEL_PEDESTAL face 8 (global 145): palette slots 3/3
    db (3 << 4) | 3 ; MODEL_SENTINEL_PEDESTAL face 9 (global 146): palette slots 3/3
    db (3 << 4) | 3 ; MODEL_SENTINEL_PEDESTAL face 10 (global 147): palette slots 3/3
    db (0 << 4) | 0 ; MODEL_TEXT_BLOCK_01 face 0 (global 148): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_TEXT_BLOCK_01 face 1 (global 149): palette slots 0/0
    db (2 << 4) | 2 ; MODEL_TEXT_BLOCK_01 face 2 (global 150): palette slots 2/2
    db (3 << 4) | 3 ; MODEL_TEXT_BLOCK_01 face 3 (global 151): palette slots 3/3
    db (0 << 4) | 0 ; MODEL_TEXT_BLOCK_10 face 0 (global 152): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_TEXT_BLOCK_10 face 1 (global 153): palette slots 0/0
    db (2 << 4) | 2 ; MODEL_TEXT_BLOCK_10 face 2 (global 154): palette slots 2/2
    db (3 << 4) | 3 ; MODEL_TEXT_BLOCK_10 face 3 (global 155): palette slots 3/3
    db (0 << 4) | 0 ; MODEL_TEXT_BLOCK_11 face 0 (global 156): palette slots 0/0
    db (0 << 4) | 0 ; MODEL_TEXT_BLOCK_11 face 1 (global 157): palette slots 0/0
    db (2 << 4) | 2 ; MODEL_TEXT_BLOCK_11 face 2 (global 158): palette slots 2/2
    db (3 << 4) | 3 ; MODEL_TEXT_BLOCK_11 face 3 (global 159): palette slots 3/3

; model_face_flags_and_counts: 160 face records, each exactly 03h/04h/83h/84h: bits 0..2 give triangle/quad vertex count, bit 7 gives the ordering partition, and bits 3..6 are zero throughout the original data. CS:1908 uses the count plus a closing repeated vertex through model_face_index_pointers. The partition bit does not independently hide a face. [load+0xca1a]
    db 4 ; MODEL_ROBOT face 0 (global 0): quad, ordering group 1
    db 3 ; MODEL_ROBOT face 1 (global 1): triangle, ordering group 1
    db 4 ; MODEL_ROBOT face 2 (global 2): quad, ordering group 1
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 3 (global 3): quad, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 4 (global 4): quad, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 5 (global 5): quad, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 6 (global 6): quad, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 7 (global 7): quad, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 8 (global 8): quad, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 9 (global 9): quad, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 10 (global 10): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 11 (global 11): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 12 (global 12): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 13 (global 13): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 14 (global 14): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 15 (global 15): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 16 (global 16): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 17 (global 17): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_ROBOT face 18 (global 18): triangle, ordering group 2
    db 4 ; MODEL_ROBOT face 19 (global 19): quad, ordering group 1
    db 3 ; MODEL_ROBOT face 20 (global 20): triangle, ordering group 1
    db 3 ; MODEL_ROBOT face 21 (global 21): triangle, ordering group 1
    db 4 ; MODEL_ROBOT face 22 (global 22): quad, ordering group 1
    db 4 ; MODEL_ROBOT face 23 (global 23): quad, ordering group 1
    db 4 ; MODEL_ROBOT face 24 (global 24): quad, ordering group 1
    db 4 ; MODEL_ROBOT face 25 (global 25): quad, ordering group 1
    db 4 ; MODEL_ROBOT face 26 (global 26): quad, ordering group 1
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 0 (global 27): quad, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 1 (global 28): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 2 (global 29): triangle, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 3 (global 30): quad, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 4 (global 31): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 5 (global 32): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 6 (global 33): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 7 (global 34): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 8 (global 35): triangle, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 9 (global 36): quad, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 10 (global 37): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 11 (global 38): triangle, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 12 (global 39): quad, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 13 (global 40): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 14 (global 41): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTRY face 15 (global 42): triangle, ordering group 2
    db 4 ; MODEL_SENTRY face 16 (global 43): quad, ordering group 1
    db 4 ; MODEL_SENTRY face 17 (global 44): quad, ordering group 1
    db 4 ; MODEL_SENTRY face 18 (global 45): quad, ordering group 1
    db 4 ; MODEL_SENTRY face 19 (global 46): quad, ordering group 1
    db 4 ; MODEL_SENTRY face 20 (global 47): quad, ordering group 1
    db 4 ; MODEL_SENTRY face 21 (global 48): quad, ordering group 1
    db 4 ; MODEL_SENTRY face 22 (global 49): quad, ordering group 1
    db 4 ; MODEL_SENTRY face 23 (global 50): quad, ordering group 1
    db 4 ; MODEL_SENTRY face 24 (global 51): quad, ordering group 1
    db 4 ; MODEL_TREE face 0 (global 52): quad, ordering group 1
    db 4 ; MODEL_TREE face 1 (global 53): quad, ordering group 1
    db 4 ; MODEL_TREE face 2 (global 54): quad, ordering group 1
    db 4 ; MODEL_TREE face 3 (global 55): quad, ordering group 1
    db 4 ; MODEL_TREE face 4 (global 56): quad, ordering group 1
    db 4 ; MODEL_TREE face 5 (global 57): quad, ordering group 1
    db 4 ; MODEL_TREE face 6 (global 58): quad, ordering group 1
    db 3 ; MODEL_TREE face 7 (global 59): triangle, ordering group 1
    db 3 ; MODEL_TREE face 8 (global 60): triangle, ordering group 1
    db 3 ; MODEL_TREE face 9 (global 61): triangle, ordering group 1
    db 3 ; MODEL_TREE face 10 (global 62): triangle, ordering group 1
    db 3 ; MODEL_TREE face 11 (global 63): triangle, ordering group 1
    db 3 ; MODEL_TREE face 12 (global 64): triangle, ordering group 1
    db 3 ; MODEL_TREE face 13 (global 65): triangle, ordering group 1
    db 3 ; MODEL_TREE face 14 (global 66): triangle, ordering group 1
    db 3 ; MODEL_BOULDER face 0 (global 67): triangle, ordering group 1
    db 3 ; MODEL_BOULDER face 1 (global 68): triangle, ordering group 1
    db 3 ; MODEL_BOULDER face 2 (global 69): triangle, ordering group 1
    db 3 ; MODEL_BOULDER face 3 (global 70): triangle, ordering group 1
    db 3 ; MODEL_BOULDER face 4 (global 71): triangle, ordering group 1
    db 3 ; MODEL_BOULDER face 5 (global 72): triangle, ordering group 1
    db 3 ; MODEL_BOULDER face 6 (global 73): triangle, ordering group 1
    db 3 ; MODEL_BOULDER face 7 (global 74): triangle, ordering group 1
    db 4 ; MODEL_BOULDER face 8 (global 75): quad, ordering group 1
    db 4 ; MODEL_BOULDER face 9 (global 76): quad, ordering group 1
    db 4 ; MODEL_MEANIE face 0 (global 77): quad, ordering group 1
    db 3 ; MODEL_MEANIE face 1 (global 78): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 2 (global 79): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 3 (global 80): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 4 (global 81): triangle, ordering group 1
    db 4 ; MODEL_MEANIE face 5 (global 82): quad, ordering group 1
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_MEANIE face 6 (global 83): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_MEANIE face 7 (global 84): triangle, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_MEANIE face 8 (global 85): quad, ordering group 2
    db 3 ; MODEL_MEANIE face 9 (global 86): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 10 (global 87): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 11 (global 88): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 12 (global 89): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 13 (global 90): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 14 (global 91): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 15 (global 92): triangle, ordering group 1
    db 4 ; MODEL_MEANIE face 16 (global 93): quad, ordering group 1
    db 3 ; MODEL_MEANIE face 17 (global 94): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 18 (global 95): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 19 (global 96): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 20 (global 97): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 21 (global 98): triangle, ordering group 1
    db 3 ; MODEL_MEANIE face 22 (global 99): triangle, ordering group 1
    db 4 ; MODEL_MEANIE face 23 (global 100): quad, ordering group 1
    db 4 ; MODEL_MEANIE face 24 (global 101): quad, ordering group 1
    db 4 ; MODEL_SENTINEL face 0 (global 102): quad, ordering group 1
    db 3 ; MODEL_SENTINEL face 1 (global 103): triangle, ordering group 1
    db 3 ; MODEL_SENTINEL face 2 (global 104): triangle, ordering group 1
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 3 (global 105): quad, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 4 (global 106): quad, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 5 (global 107): quad, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 6 (global 108): quad, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 7 (global 109): quad, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 8 (global 110): quad, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 9 (global 111): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 10 (global 112): triangle, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 11 (global 113): quad, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 12 (global 114): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 13 (global 115): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 14 (global 116): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 15 (global 117): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 16 (global 118): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 17 (global 119): triangle, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 18 (global 120): quad, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 19 (global 121): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 20 (global 122): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 21 (global 123): triangle, ordering group 2
    db 3 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 22 (global 124): triangle, ordering group 2
    db 4 | FACE_ORDER_GROUP_2 ; MODEL_SENTINEL face 23 (global 125): quad, ordering group 2
    db 4 ; MODEL_SENTINEL face 24 (global 126): quad, ordering group 1
    db 4 ; MODEL_SENTINEL face 25 (global 127): quad, ordering group 1
    db 3 ; MODEL_SENTINEL face 26 (global 128): triangle, ordering group 1
    db 3 ; MODEL_SENTINEL face 27 (global 129): triangle, ordering group 1
    db 4 ; MODEL_SENTINEL face 28 (global 130): quad, ordering group 1
    db 4 ; MODEL_SENTINEL face 29 (global 131): quad, ordering group 1
    db 4 ; MODEL_SENTINEL face 30 (global 132): quad, ordering group 1
    db 4 ; MODEL_SENTINEL face 31 (global 133): quad, ordering group 1
    db 4 ; MODEL_SENTINEL face 32 (global 134): quad, ordering group 1
    db 4 ; MODEL_SENTINEL face 33 (global 135): quad, ordering group 1
    db 4 ; MODEL_SENTINEL face 34 (global 136): quad, ordering group 1
    db 4 ; MODEL_SENTINEL_PEDESTAL face 0 (global 137): quad, ordering group 1
    db 4 ; MODEL_SENTINEL_PEDESTAL face 1 (global 138): quad, ordering group 1
    db 4 ; MODEL_SENTINEL_PEDESTAL face 2 (global 139): quad, ordering group 1
    db 4 ; MODEL_SENTINEL_PEDESTAL face 3 (global 140): quad, ordering group 1
    db 3 ; MODEL_SENTINEL_PEDESTAL face 4 (global 141): triangle, ordering group 1
    db 3 ; MODEL_SENTINEL_PEDESTAL face 5 (global 142): triangle, ordering group 1
    db 3 ; MODEL_SENTINEL_PEDESTAL face 6 (global 143): triangle, ordering group 1
    db 3 ; MODEL_SENTINEL_PEDESTAL face 7 (global 144): triangle, ordering group 1
    db 4 ; MODEL_SENTINEL_PEDESTAL face 8 (global 145): quad, ordering group 1
    db 4 ; MODEL_SENTINEL_PEDESTAL face 9 (global 146): quad, ordering group 1
    db 4 ; MODEL_SENTINEL_PEDESTAL face 10 (global 147): quad, ordering group 1
    db 4 ; MODEL_TEXT_BLOCK_01 face 0 (global 148): quad, ordering group 1
    db 4 ; MODEL_TEXT_BLOCK_01 face 1 (global 149): quad, ordering group 1
    db 4 ; MODEL_TEXT_BLOCK_01 face 2 (global 150): quad, ordering group 1
    db 4 ; MODEL_TEXT_BLOCK_01 face 3 (global 151): quad, ordering group 1
    db 4 ; MODEL_TEXT_BLOCK_10 face 0 (global 152): quad, ordering group 1
    db 4 ; MODEL_TEXT_BLOCK_10 face 1 (global 153): quad, ordering group 1
    db 4 ; MODEL_TEXT_BLOCK_10 face 2 (global 154): quad, ordering group 1
    db 4 ; MODEL_TEXT_BLOCK_10 face 3 (global 155): quad, ordering group 1
    db 4 ; MODEL_TEXT_BLOCK_11 face 0 (global 156): quad, ordering group 1
    db 4 ; MODEL_TEXT_BLOCK_11 face 1 (global 157): quad, ordering group 1
    db 4 ; MODEL_TEXT_BLOCK_11 face 2 (global 158): quad, ordering group 1
    db 4 ; MODEL_TEXT_BLOCK_11 face 3 (global 159): quad, ordering group 1

; model_face_index_pointers: 160 DS-relative pointers to closed face vertex loops below [load+0xcaba]
    dw robot_face_00_vertices, robot_face_01_vertices, robot_face_02_vertices, robot_face_03_vertices, robot_face_04_vertices, robot_face_05_vertices, robot_face_06_vertices, robot_face_07_vertices
    dw robot_face_08_vertices, robot_face_09_vertices, model_vertex_loop_556f, model_vertex_loop_5573, model_vertex_loop_5577, model_vertex_loop_557b, model_vertex_loop_557f, model_vertex_loop_5583
    dw model_vertex_loop_5587, model_vertex_loop_558b, model_vertex_loop_558f, model_vertex_loop_5593, model_vertex_loop_5598, model_vertex_loop_559c, model_vertex_loop_55a0, model_vertex_loop_55a5
    dw model_vertex_loop_55aa, model_vertex_loop_55af, model_vertex_loop_55b4, sentry_face_00_vertices, sentry_face_01_vertices, sentry_face_02_vertices, sentry_face_03_vertices, sentry_face_04_vertices
    dw sentry_face_05_vertices, sentry_face_06_vertices, sentry_face_07_vertices, sentry_face_08_vertices, sentry_face_09_vertices, model_vertex_loop_55e4, model_vertex_loop_55e8, model_vertex_loop_55ec
    dw model_vertex_loop_55f1, model_vertex_loop_55f5, model_vertex_loop_55f9, model_vertex_loop_55fd, model_vertex_loop_5602, model_vertex_loop_5607, model_vertex_loop_560c, model_vertex_loop_5611
    dw model_vertex_loop_5616, model_vertex_loop_561b, model_vertex_loop_5620, model_vertex_loop_5625, tree_face_00_vertices, tree_face_01_vertices, tree_face_02_vertices, tree_face_03_vertices
    dw tree_face_04_vertices, tree_face_05_vertices, tree_face_06_vertices, tree_face_07_vertices, tree_face_08_vertices, tree_face_09_vertices, model_vertex_loop_54b9, model_vertex_loop_54bd
    dw model_vertex_loop_54c1, model_vertex_loop_54c5, model_vertex_loop_54c9, boulder_face_00_vertices, boulder_face_01_vertices, boulder_face_02_vertices, boulder_face_03_vertices, boulder_face_04_vertices
    dw boulder_face_05_vertices, boulder_face_06_vertices, boulder_face_07_vertices, boulder_face_08_vertices, boulder_face_09_vertices, model_vertex_loop_4bf7, model_vertex_loop_4bfc, model_vertex_loop_4c00
    dw model_vertex_loop_4c04, model_vertex_loop_4c08, model_vertex_loop_4c0c, model_vertex_loop_4c11, model_vertex_loop_4c15, model_vertex_loop_4c19, model_vertex_loop_4c1e, model_vertex_loop_4c22
    dw model_vertex_loop_4c26, model_vertex_loop_4c2a, model_vertex_loop_4c2e, model_vertex_loop_4c32, model_vertex_loop_4c36, model_vertex_loop_4c3a, model_vertex_loop_4c3f, model_vertex_loop_4c43
    dw model_vertex_loop_4c47, model_vertex_loop_4c4b, model_vertex_loop_4c4f, model_vertex_loop_4c53, model_vertex_loop_4c57, model_vertex_loop_4c5c, sentinel_face_00_vertices, sentinel_face_01_vertices
    dw sentinel_face_02_vertices, sentinel_face_03_vertices, sentinel_face_04_vertices, sentinel_face_05_vertices, sentinel_face_06_vertices, sentinel_face_07_vertices, sentinel_face_08_vertices, sentinel_face_09_vertices
    dw model_vertex_loop_5379, model_vertex_loop_537d, model_vertex_loop_5382, model_vertex_loop_5386, model_vertex_loop_538a, model_vertex_loop_538e, model_vertex_loop_5392, model_vertex_loop_5396
    dw model_vertex_loop_539a, model_vertex_loop_539f, model_vertex_loop_53a3, model_vertex_loop_53a7, model_vertex_loop_53ab, model_vertex_loop_53af, model_vertex_loop_53b4, model_vertex_loop_53b9
    dw model_vertex_loop_53be, model_vertex_loop_53c2, model_vertex_loop_53c6, model_vertex_loop_53cb, model_vertex_loop_53d0, model_vertex_loop_53d5, model_vertex_loop_53da, model_vertex_loop_53df
    dw model_vertex_loop_53e4, model_vertex_loop_54f7, model_vertex_loop_54fc, model_vertex_loop_5501, model_vertex_loop_5506, model_vertex_loop_550b, model_vertex_loop_550f, model_vertex_loop_5513
    dw model_vertex_loop_5517, model_vertex_loop_551b, model_vertex_loop_5520, model_vertex_loop_5525, model_vertex_loop_552a, model_vertex_loop_552f, model_vertex_loop_5534, model_vertex_loop_5539
    dw model_vertex_loop_552a, model_vertex_loop_552f, model_vertex_loop_5534, model_vertex_loop_5539, model_vertex_loop_552a, model_vertex_loop_552f, model_vertex_loop_5534, model_vertex_loop_5539

; sentinel_face_00_vertices: MODEL_SENTINEL face 0 (global face 102): closed quad, stored heights 4/256..10/256; palette slots 0/0, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcbfa]
; SENTINEL_VERTEX_20: cache 54h, source vertex 114; azimuth 83/256 turn, height 4/256, radial factor 70.
; SENTINEL_VERTEX_16: cache 50h, source vertex 110; azimuth 90/256 turn, height 10/256, radial factor 34.
; SENTINEL_VERTEX_17: cache 51h, source vertex 111; azimuth 166/256 turn, height 10/256, radial factor 34.
; SENTINEL_VERTEX_21: cache 55h, source vertex 115; azimuth 173/256 turn, height 4/256, radial factor 70.
    db SENTINEL_VERTEX_20, SENTINEL_VERTEX_16, SENTINEL_VERTEX_17, SENTINEL_VERTEX_21, SENTINEL_VERTEX_20 ; final vertex closes the polygon

; sentinel_face_01_vertices: MODEL_SENTINEL face 1 (global face 103): closed triangle, stored heights 4/256..16/256; palette slots 0/0, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcbff]
; SENTINEL_VERTEX_17: cache 51h, source vertex 111; azimuth 166/256 turn, height 10/256, radial factor 34.
; SENTINEL_VERTEX_14: cache 4Eh, source vertex 108; azimuth 218/256 turn, height 16/256, radial factor 34.
; SENTINEL_VERTEX_21: cache 55h, source vertex 115; azimuth 173/256 turn, height 4/256, radial factor 70.
    db SENTINEL_VERTEX_17, SENTINEL_VERTEX_14, SENTINEL_VERTEX_21, SENTINEL_VERTEX_17 ; final vertex closes the polygon

; sentinel_face_02_vertices: MODEL_SENTINEL face 2 (global face 104): closed triangle, stored heights 4/256..16/256; palette slots 0/0, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcc03]
; SENTINEL_VERTEX_20: cache 54h, source vertex 114; azimuth 83/256 turn, height 4/256, radial factor 70.
; SENTINEL_VERTEX_15: cache 4Fh, source vertex 109; azimuth 38/256 turn, height 16/256, radial factor 34.
; SENTINEL_VERTEX_16: cache 50h, source vertex 110; azimuth 90/256 turn, height 10/256, radial factor 34.
    db SENTINEL_VERTEX_20, SENTINEL_VERTEX_15, SENTINEL_VERTEX_16, SENTINEL_VERTEX_20 ; final vertex closes the polygon

; sentinel_face_03_vertices: MODEL_SENTINEL face 3 (global face 105): closed quad, constant stored height 16/256; palette slots 0/0, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcc07]
; SENTINEL_VERTEX_15: cache 4Fh, source vertex 109; azimuth 38/256 turn, height 16/256, radial factor 34.
; SENTINEL_VERTEX_19: cache 53h, source vertex 113; azimuth 11/256 turn, height 16/256, radial factor 64.
; SENTINEL_VERTEX_18: cache 52h, source vertex 112; azimuth 245/256 turn, height 16/256, radial factor 64.
; SENTINEL_VERTEX_14: cache 4Eh, source vertex 108; azimuth 218/256 turn, height 16/256, radial factor 34.
    db SENTINEL_VERTEX_15, SENTINEL_VERTEX_19, SENTINEL_VERTEX_18, SENTINEL_VERTEX_14, SENTINEL_VERTEX_15 ; final vertex closes the polygon

; sentinel_face_04_vertices: MODEL_SENTINEL face 4 (global face 106): closed quad, stored heights -224/256..-192/256; palette slots 6/6, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcc0c]
; SENTINEL_VERTEX_03: cache 43h, source vertex 97; azimuth 166/256 turn, height -224/256, radial factor 56.
; SENTINEL_VERTEX_07: cache 47h, source vertex 101; azimuth 166/256 turn, height -192/256, radial factor 42.
; SENTINEL_VERTEX_06: cache 46h, source vertex 100; azimuth 90/256 turn, height -192/256, radial factor 42.
; SENTINEL_VERTEX_02: cache 42h, source vertex 96; azimuth 90/256 turn, height -224/256, radial factor 56.
    db SENTINEL_VERTEX_03, SENTINEL_VERTEX_07, SENTINEL_VERTEX_06, SENTINEL_VERTEX_02, SENTINEL_VERTEX_03 ; final vertex closes the polygon

; sentinel_face_05_vertices: MODEL_SENTINEL face 5 (global face 107): closed quad, stored heights -224/256..-192/256; palette slots 0/0, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcc11]
; SENTINEL_VERTEX_00: cache 40h, source vertex 94; azimuth 218/256 turn, height -224/256, radial factor 56.
; SENTINEL_VERTEX_04: cache 44h, source vertex 98; azimuth 218/256 turn, height -192/256, radial factor 42.
; SENTINEL_VERTEX_07: cache 47h, source vertex 101; azimuth 166/256 turn, height -192/256, radial factor 42.
; SENTINEL_VERTEX_03: cache 43h, source vertex 97; azimuth 166/256 turn, height -224/256, radial factor 56.
    db SENTINEL_VERTEX_00, SENTINEL_VERTEX_04, SENTINEL_VERTEX_07, SENTINEL_VERTEX_03, SENTINEL_VERTEX_00 ; final vertex closes the polygon

; sentinel_face_06_vertices: MODEL_SENTINEL face 6 (global face 108): closed quad, stored heights -224/256..-192/256; palette slots 0/0, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcc16]
; SENTINEL_VERTEX_02: cache 42h, source vertex 96; azimuth 90/256 turn, height -224/256, radial factor 56.
; SENTINEL_VERTEX_06: cache 46h, source vertex 100; azimuth 90/256 turn, height -192/256, radial factor 42.
; SENTINEL_VERTEX_05: cache 45h, source vertex 99; azimuth 38/256 turn, height -192/256, radial factor 42.
; SENTINEL_VERTEX_01: cache 41h, source vertex 95; azimuth 38/256 turn, height -224/256, radial factor 56.
    db SENTINEL_VERTEX_02, SENTINEL_VERTEX_06, SENTINEL_VERTEX_05, SENTINEL_VERTEX_01, SENTINEL_VERTEX_02 ; final vertex closes the polygon

; sentinel_face_07_vertices: MODEL_SENTINEL face 7 (global face 109): closed quad, stored heights -224/256..-192/256; palette slots 0/0, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcc1b]
; SENTINEL_VERTEX_01: cache 41h, source vertex 95; azimuth 38/256 turn, height -224/256, radial factor 56.
; SENTINEL_VERTEX_05: cache 45h, source vertex 99; azimuth 38/256 turn, height -192/256, radial factor 42.
; SENTINEL_VERTEX_04: cache 44h, source vertex 98; azimuth 218/256 turn, height -192/256, radial factor 42.
; SENTINEL_VERTEX_00: cache 40h, source vertex 94; azimuth 218/256 turn, height -224/256, radial factor 56.
    db SENTINEL_VERTEX_01, SENTINEL_VERTEX_05, SENTINEL_VERTEX_04, SENTINEL_VERTEX_00, SENTINEL_VERTEX_01 ; final vertex closes the polygon

; sentinel_face_08_vertices: MODEL_SENTINEL face 8 (global face 110): closed quad, stored heights -192/256..-22/256; palette slots 0/0, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcc20]
; SENTINEL_VERTEX_07: cache 47h, source vertex 101; azimuth 166/256 turn, height -192/256, radial factor 42.
; SENTINEL_VERTEX_12: cache 4Ch, source vertex 106; azimuth 166/256 turn, height -22/256, radial factor 42.
; SENTINEL_VERTEX_11: cache 4Bh, source vertex 105; azimuth 90/256 turn, height -22/256, radial factor 42.
; SENTINEL_VERTEX_06: cache 46h, source vertex 100; azimuth 90/256 turn, height -192/256, radial factor 42.
    db SENTINEL_VERTEX_07, SENTINEL_VERTEX_12, SENTINEL_VERTEX_11, SENTINEL_VERTEX_06, SENTINEL_VERTEX_07 ; final vertex closes the polygon

; sentinel_face_09_vertices: MODEL_SENTINEL face 9 (global face 111): closed triangle, stored heights -192/256..-22/256; palette slots 0/0, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcc25]
; SENTINEL_VERTEX_04: cache 44h, source vertex 98; azimuth 218/256 turn, height -192/256, radial factor 42.
; SENTINEL_VERTEX_13: cache 4Dh, source vertex 107; azimuth 192/256 turn, height -22/256, radial factor 54.
; SENTINEL_VERTEX_07: cache 47h, source vertex 101; azimuth 166/256 turn, height -192/256, radial factor 42.
    db SENTINEL_VERTEX_04, SENTINEL_VERTEX_13, SENTINEL_VERTEX_07, SENTINEL_VERTEX_04 ; final vertex closes the polygon

; model_vertex_loop_5379: closed vertex loop for MODEL_SENTINEL face 10 (global 112) [load+0xcc29]
    db 0x46, 0x4a, 0x45, 0x46

; model_vertex_loop_537d: closed vertex loop for MODEL_SENTINEL face 11 (global 113) [load+0xcc2d]
    db 0x45, 0x49, 0x48, 0x44, 0x45

; model_vertex_loop_5382: closed vertex loop for MODEL_SENTINEL face 12 (global 114) [load+0xcc32]
    db 0x47, 0x4d, 0x4c, 0x47

; model_vertex_loop_5386: closed vertex loop for MODEL_SENTINEL face 13 (global 115) [load+0xcc36]
    db 0x46, 0x4b, 0x4a, 0x46

; model_vertex_loop_538a: closed vertex loop for MODEL_SENTINEL face 14 (global 116) [load+0xcc3a]
    db 0x44, 0x48, 0x4d, 0x44

; model_vertex_loop_538e: closed vertex loop for MODEL_SENTINEL face 15 (global 117) [load+0xcc3e]
    db 0x45, 0x4a, 0x49, 0x45

; model_vertex_loop_5392: closed vertex loop for MODEL_SENTINEL face 16 (global 118) [load+0xcc42]
    db 0x4d, 0x51, 0x4c, 0x4d

; model_vertex_loop_5396: closed vertex loop for MODEL_SENTINEL face 17 (global 119) [load+0xcc46]
    db 0x4b, 0x50, 0x4a, 0x4b

; model_vertex_loop_539a: closed vertex loop for MODEL_SENTINEL face 18 (global 120) [load+0xcc4a]
    db 0x4c, 0x51, 0x50, 0x4b, 0x4c

; model_vertex_loop_539f: closed vertex loop for MODEL_SENTINEL face 19 (global 121) [load+0xcc4f]
    db 0x4d, 0x4e, 0x51, 0x4d

; model_vertex_loop_53a3: closed vertex loop for MODEL_SENTINEL face 20 (global 122) [load+0xcc53]
    db 0x4a, 0x50, 0x4f, 0x4a

; model_vertex_loop_53a7: closed vertex loop for MODEL_SENTINEL face 21 (global 123) [load+0xcc57]
    db 0x48, 0x4e, 0x4d, 0x48

; model_vertex_loop_53ab: closed vertex loop for MODEL_SENTINEL face 22 (global 124) [load+0xcc5b]
    db 0x49, 0x4a, 0x4f, 0x49

; model_vertex_loop_53af: closed vertex loop for MODEL_SENTINEL face 23 (global 125) [load+0xcc5f]
    db 0x49, 0x4f, 0x4e, 0x48, 0x49

; model_vertex_loop_53b4: closed vertex loop for MODEL_SENTINEL face 24 (global 126) [load+0xcc64]
    db 0x59, 0x5b, 0x5a, 0x58, 0x59

; model_vertex_loop_53b9: closed vertex loop for MODEL_SENTINEL face 25 (global 127) [load+0xcc69]
    db 0x55, 0x5d, 0x5c, 0x54, 0x55

; model_vertex_loop_53be: closed vertex loop for MODEL_SENTINEL face 26 (global 128) [load+0xcc6e]
    db 0x4e, 0x58, 0x55, 0x4e

; model_vertex_loop_53c2: closed vertex loop for MODEL_SENTINEL face 27 (global 129) [load+0xcc72]
    db 0x54, 0x59, 0x4f, 0x54

; model_vertex_loop_53c6: closed vertex loop for MODEL_SENTINEL face 28 (global 130) [load+0xcc76]
    db 0x52, 0x56, 0x58, 0x4e, 0x52

; model_vertex_loop_53cb: closed vertex loop for MODEL_SENTINEL face 29 (global 131) [load+0xcc7b]
    db 0x4f, 0x59, 0x57, 0x53, 0x4f

; model_vertex_loop_53d0: closed vertex loop for MODEL_SENTINEL face 30 (global 132) [load+0xcc80]
    db 0x57, 0x59, 0x58, 0x56, 0x57

; model_vertex_loop_53d5: closed vertex loop for MODEL_SENTINEL face 31 (global 133) [load+0xcc85]
    db 0x53, 0x57, 0x56, 0x52, 0x53

; model_vertex_loop_53da: closed vertex loop for MODEL_SENTINEL face 32 (global 134) [load+0xcc8a]
    db 0x5b, 0x5c, 0x5d, 0x5a, 0x5b

; model_vertex_loop_53df: closed vertex loop for MODEL_SENTINEL face 33 (global 135) [load+0xcc8f]
    db 0x58, 0x5a, 0x5d, 0x55, 0x58

; model_vertex_loop_53e4: closed vertex loop for MODEL_SENTINEL face 34 (global 136) [load+0xcc94]
    db 0x54, 0x5c, 0x5b, 0x59, 0x54

; opaque_sentinel_face_loop_trailer: One FFh byte followed by the same five-chunk sequence as the azimuth trailer. It follows the complete 35-face Sentinel loop set and precedes the tree loops; no face-pointer/count pair references any byte in it. [load+0xcc99]
    times 1 db 0xff
    opaque_init_chunk ZERO_WITH_20
    opaque_init_chunk ONES
    opaque_init_chunk ZERO
    opaque_init_chunk ONES_WITH_A0_EF
    opaque_init_chunk ZERO_WITH_20

; tree_face_00_vertices: MODEL_TREE face 0 (global face 52): closed quad, constant stored height -144/256; palette slots 0/0, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd3a]
; TREE_VERTEX_08: cache 48h, source vertex 59; azimuth 0/256 turn, height -144/256, radial factor 104.
; TREE_VERTEX_15: cache 4Fh, source vertex 66; azimuth 224/256 turn, height -144/256, radial factor 104.
; TREE_VERTEX_14: cache 4Eh, source vertex 65; azimuth 192/256 turn, height -144/256, radial factor 104.
; TREE_VERTEX_09: cache 49h, source vertex 60; azimuth 32/256 turn, height -144/256, radial factor 104.
    db TREE_VERTEX_08, TREE_VERTEX_15, TREE_VERTEX_14, TREE_VERTEX_09, TREE_VERTEX_08 ; final vertex closes the polygon

; tree_face_01_vertices: MODEL_TREE face 1 (global face 53): closed quad, constant stored height -144/256; palette slots 0/0, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd3f]
; TREE_VERTEX_09: cache 49h, source vertex 60; azimuth 32/256 turn, height -144/256, radial factor 104.
; TREE_VERTEX_14: cache 4Eh, source vertex 65; azimuth 192/256 turn, height -144/256, radial factor 104.
; TREE_VERTEX_13: cache 4Dh, source vertex 64; azimuth 160/256 turn, height -144/256, radial factor 104.
; TREE_VERTEX_10: cache 4Ah, source vertex 61; azimuth 64/256 turn, height -144/256, radial factor 104.
    db TREE_VERTEX_09, TREE_VERTEX_14, TREE_VERTEX_13, TREE_VERTEX_10, TREE_VERTEX_09 ; final vertex closes the polygon

; tree_face_02_vertices: MODEL_TREE face 2 (global face 54): closed quad, constant stored height -144/256; palette slots 0/0, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd44]
; TREE_VERTEX_10: cache 4Ah, source vertex 61; azimuth 64/256 turn, height -144/256, radial factor 104.
; TREE_VERTEX_13: cache 4Dh, source vertex 64; azimuth 160/256 turn, height -144/256, radial factor 104.
; TREE_VERTEX_12: cache 4Ch, source vertex 63; azimuth 128/256 turn, height -144/256, radial factor 104.
; TREE_VERTEX_11: cache 4Bh, source vertex 62; azimuth 96/256 turn, height -144/256, radial factor 104.
    db TREE_VERTEX_10, TREE_VERTEX_13, TREE_VERTEX_12, TREE_VERTEX_11, TREE_VERTEX_10 ; final vertex closes the polygon

; tree_face_03_vertices: MODEL_TREE face 3 (global face 55): closed quad, stored heights -224/256..-144/256; palette slots 0/0, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd49]
; TREE_VERTEX_01: cache 41h, source vertex 52; azimuth 32/256 turn, height -224/256, radial factor 24.
; TREE_VERTEX_05: cache 45h, source vertex 56; azimuth 32/256 turn, height -144/256, radial factor 24.
; TREE_VERTEX_04: cache 44h, source vertex 55; azimuth 224/256 turn, height -144/256, radial factor 24.
; TREE_VERTEX_00: cache 40h, source vertex 51; azimuth 224/256 turn, height -224/256, radial factor 24.
    db TREE_VERTEX_01, TREE_VERTEX_05, TREE_VERTEX_04, TREE_VERTEX_00, TREE_VERTEX_01 ; final vertex closes the polygon

; tree_face_04_vertices: MODEL_TREE face 4 (global face 56): closed quad, stored heights -224/256..-144/256; palette slots 13/13, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd4e]
; TREE_VERTEX_02: cache 42h, source vertex 53; azimuth 96/256 turn, height -224/256, radial factor 24.
; TREE_VERTEX_06: cache 46h, source vertex 57; azimuth 96/256 turn, height -144/256, radial factor 24.
; TREE_VERTEX_05: cache 45h, source vertex 56; azimuth 32/256 turn, height -144/256, radial factor 24.
; TREE_VERTEX_01: cache 41h, source vertex 52; azimuth 32/256 turn, height -224/256, radial factor 24.
    db TREE_VERTEX_02, TREE_VERTEX_06, TREE_VERTEX_05, TREE_VERTEX_01, TREE_VERTEX_02 ; final vertex closes the polygon

; tree_face_05_vertices: MODEL_TREE face 5 (global face 57): closed quad, stored heights -224/256..-144/256; palette slots 13/13, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd53]
; TREE_VERTEX_00: cache 40h, source vertex 51; azimuth 224/256 turn, height -224/256, radial factor 24.
; TREE_VERTEX_04: cache 44h, source vertex 55; azimuth 224/256 turn, height -144/256, radial factor 24.
; TREE_VERTEX_07: cache 47h, source vertex 58; azimuth 160/256 turn, height -144/256, radial factor 24.
; TREE_VERTEX_03: cache 43h, source vertex 54; azimuth 160/256 turn, height -224/256, radial factor 24.
    db TREE_VERTEX_00, TREE_VERTEX_04, TREE_VERTEX_07, TREE_VERTEX_03, TREE_VERTEX_00 ; final vertex closes the polygon

; tree_face_06_vertices: MODEL_TREE face 6 (global face 58): closed quad, stored heights -224/256..-144/256; palette slots 0/0, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd58]
; TREE_VERTEX_03: cache 43h, source vertex 54; azimuth 160/256 turn, height -224/256, radial factor 24.
; TREE_VERTEX_07: cache 47h, source vertex 58; azimuth 160/256 turn, height -144/256, radial factor 24.
; TREE_VERTEX_06: cache 46h, source vertex 57; azimuth 96/256 turn, height -144/256, radial factor 24.
; TREE_VERTEX_02: cache 42h, source vertex 53; azimuth 96/256 turn, height -224/256, radial factor 24.
    db TREE_VERTEX_03, TREE_VERTEX_07, TREE_VERTEX_06, TREE_VERTEX_02, TREE_VERTEX_03 ; final vertex closes the polygon

; tree_face_07_vertices: MODEL_TREE face 7 (global face 59): closed triangle, stored heights -144/256..240/256; palette slots 4/4, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd5d]
; TREE_VERTEX_08: cache 48h, source vertex 59; azimuth 0/256 turn, height -144/256, radial factor 104.
; TREE_VERTEX_16: cache 50h, source vertex 67; azimuth 0/256 turn, height 240/256, radial factor 0.
; TREE_VERTEX_15: cache 4Fh, source vertex 66; azimuth 224/256 turn, height -144/256, radial factor 104.
    db TREE_VERTEX_08, TREE_VERTEX_16, TREE_VERTEX_15, TREE_VERTEX_08 ; final vertex closes the polygon

; tree_face_08_vertices: MODEL_TREE face 8 (global face 60): closed triangle, stored heights -144/256..240/256; palette slots 5/5, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd61]
; TREE_VERTEX_09: cache 49h, source vertex 60; azimuth 32/256 turn, height -144/256, radial factor 104.
; TREE_VERTEX_16: cache 50h, source vertex 67; azimuth 0/256 turn, height 240/256, radial factor 0.
; TREE_VERTEX_08: cache 48h, source vertex 59; azimuth 0/256 turn, height -144/256, radial factor 104.
    db TREE_VERTEX_09, TREE_VERTEX_16, TREE_VERTEX_08, TREE_VERTEX_09 ; final vertex closes the polygon

; tree_face_09_vertices: MODEL_TREE face 9 (global face 61): closed triangle, stored heights -144/256..240/256; palette slots 4/4, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd65]
; TREE_VERTEX_10: cache 4Ah, source vertex 61; azimuth 64/256 turn, height -144/256, radial factor 104.
; TREE_VERTEX_16: cache 50h, source vertex 67; azimuth 0/256 turn, height 240/256, radial factor 0.
; TREE_VERTEX_09: cache 49h, source vertex 60; azimuth 32/256 turn, height -144/256, radial factor 104.
    db TREE_VERTEX_10, TREE_VERTEX_16, TREE_VERTEX_09, TREE_VERTEX_10 ; final vertex closes the polygon

; model_vertex_loop_54b9: closed vertex loop for MODEL_TREE face 10 (global 62) [load+0xcd69]
    db 0x4b, 0x50, 0x4a, 0x4b

; model_vertex_loop_54bd: closed vertex loop for MODEL_TREE face 11 (global 63) [load+0xcd6d]
    db 0x4c, 0x50, 0x4b, 0x4c

; model_vertex_loop_54c1: closed vertex loop for MODEL_TREE face 12 (global 64) [load+0xcd71]
    db 0x4d, 0x50, 0x4c, 0x4d

; model_vertex_loop_54c5: closed vertex loop for MODEL_TREE face 13 (global 65) [load+0xcd75]
    db 0x4e, 0x50, 0x4d, 0x4e

; model_vertex_loop_54c9: closed vertex loop for MODEL_TREE face 14 (global 66) [load+0xcd79]
    db 0x4f, 0x50, 0x4e, 0x4f

; boulder_face_00_vertices: MODEL_BOULDER face 0 (global face 67): closed triangle, stored heights -224/256..-96/256; palette slots 15/15, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd7d]
; BOULDER_VERTEX_01: cache 41h, source vertex 69; azimuth 32/256 turn, height -224/256, radial factor 112.
; BOULDER_VERTEX_00: cache 40h, source vertex 68; azimuth 0/256 turn, height -96/256, radial factor 112.
; BOULDER_VERTEX_07: cache 47h, source vertex 75; azimuth 224/256 turn, height -224/256, radial factor 112.
    db BOULDER_VERTEX_01, BOULDER_VERTEX_00, BOULDER_VERTEX_07, BOULDER_VERTEX_01 ; final vertex closes the polygon

; boulder_face_01_vertices: MODEL_BOULDER face 1 (global face 68): closed triangle, stored heights -224/256..-96/256; palette slots 15/15, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd81]
; BOULDER_VERTEX_03: cache 43h, source vertex 71; azimuth 96/256 turn, height -224/256, radial factor 112.
; BOULDER_VERTEX_02: cache 42h, source vertex 70; azimuth 64/256 turn, height -96/256, radial factor 112.
; BOULDER_VERTEX_01: cache 41h, source vertex 69; azimuth 32/256 turn, height -224/256, radial factor 112.
    db BOULDER_VERTEX_03, BOULDER_VERTEX_02, BOULDER_VERTEX_01, BOULDER_VERTEX_03 ; final vertex closes the polygon

; boulder_face_02_vertices: MODEL_BOULDER face 2 (global face 69): closed triangle, stored heights -224/256..-96/256; palette slots 15/15, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd85]
; BOULDER_VERTEX_05: cache 45h, source vertex 73; azimuth 160/256 turn, height -224/256, radial factor 112.
; BOULDER_VERTEX_04: cache 44h, source vertex 72; azimuth 128/256 turn, height -96/256, radial factor 112.
; BOULDER_VERTEX_03: cache 43h, source vertex 71; azimuth 96/256 turn, height -224/256, radial factor 112.
    db BOULDER_VERTEX_05, BOULDER_VERTEX_04, BOULDER_VERTEX_03, BOULDER_VERTEX_05 ; final vertex closes the polygon

; boulder_face_03_vertices: MODEL_BOULDER face 3 (global face 70): closed triangle, stored heights -224/256..-96/256; palette slots 15/15, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd89]
; BOULDER_VERTEX_07: cache 47h, source vertex 75; azimuth 224/256 turn, height -224/256, radial factor 112.
; BOULDER_VERTEX_06: cache 46h, source vertex 74; azimuth 192/256 turn, height -96/256, radial factor 112.
; BOULDER_VERTEX_05: cache 45h, source vertex 73; azimuth 160/256 turn, height -224/256, radial factor 112.
    db BOULDER_VERTEX_07, BOULDER_VERTEX_06, BOULDER_VERTEX_05, BOULDER_VERTEX_07 ; final vertex closes the polygon

; boulder_face_04_vertices: MODEL_BOULDER face 4 (global face 71): closed triangle, stored heights -224/256..-96/256; palette slots 11/11, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd8d]
; BOULDER_VERTEX_01: cache 41h, source vertex 69; azimuth 32/256 turn, height -224/256, radial factor 112.
; BOULDER_VERTEX_02: cache 42h, source vertex 70; azimuth 64/256 turn, height -96/256, radial factor 112.
; BOULDER_VERTEX_00: cache 40h, source vertex 68; azimuth 0/256 turn, height -96/256, radial factor 112.
    db BOULDER_VERTEX_01, BOULDER_VERTEX_02, BOULDER_VERTEX_00, BOULDER_VERTEX_01 ; final vertex closes the polygon

; boulder_face_05_vertices: MODEL_BOULDER face 5 (global face 72): closed triangle, stored heights -224/256..-96/256; palette slots 11/11, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd91]
; BOULDER_VERTEX_03: cache 43h, source vertex 71; azimuth 96/256 turn, height -224/256, radial factor 112.
; BOULDER_VERTEX_04: cache 44h, source vertex 72; azimuth 128/256 turn, height -96/256, radial factor 112.
; BOULDER_VERTEX_02: cache 42h, source vertex 70; azimuth 64/256 turn, height -96/256, radial factor 112.
    db BOULDER_VERTEX_03, BOULDER_VERTEX_04, BOULDER_VERTEX_02, BOULDER_VERTEX_03 ; final vertex closes the polygon

; boulder_face_06_vertices: MODEL_BOULDER face 6 (global face 73): closed triangle, stored heights -224/256..-96/256; palette slots 11/11, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd95]
; BOULDER_VERTEX_05: cache 45h, source vertex 73; azimuth 160/256 turn, height -224/256, radial factor 112.
; BOULDER_VERTEX_06: cache 46h, source vertex 74; azimuth 192/256 turn, height -96/256, radial factor 112.
; BOULDER_VERTEX_04: cache 44h, source vertex 72; azimuth 128/256 turn, height -96/256, radial factor 112.
    db BOULDER_VERTEX_05, BOULDER_VERTEX_06, BOULDER_VERTEX_04, BOULDER_VERTEX_05 ; final vertex closes the polygon

; boulder_face_07_vertices: MODEL_BOULDER face 7 (global face 74): closed triangle, stored heights -224/256..-96/256; palette slots 11/11, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd99]
; BOULDER_VERTEX_07: cache 47h, source vertex 75; azimuth 224/256 turn, height -224/256, radial factor 112.
; BOULDER_VERTEX_00: cache 40h, source vertex 68; azimuth 0/256 turn, height -96/256, radial factor 112.
; BOULDER_VERTEX_06: cache 46h, source vertex 74; azimuth 192/256 turn, height -96/256, radial factor 112.
    db BOULDER_VERTEX_07, BOULDER_VERTEX_00, BOULDER_VERTEX_06, BOULDER_VERTEX_07 ; final vertex closes the polygon

; boulder_face_08_vertices: MODEL_BOULDER face 8 (global face 75): closed quad, constant stored height -224/256; palette slots 0/0, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcd9d]
; BOULDER_VERTEX_01: cache 41h, source vertex 69; azimuth 32/256 turn, height -224/256, radial factor 112.
; BOULDER_VERTEX_07: cache 47h, source vertex 75; azimuth 224/256 turn, height -224/256, radial factor 112.
; BOULDER_VERTEX_05: cache 45h, source vertex 73; azimuth 160/256 turn, height -224/256, radial factor 112.
; BOULDER_VERTEX_03: cache 43h, source vertex 71; azimuth 96/256 turn, height -224/256, radial factor 112.
    db BOULDER_VERTEX_01, BOULDER_VERTEX_07, BOULDER_VERTEX_05, BOULDER_VERTEX_03, BOULDER_VERTEX_01 ; final vertex closes the polygon

; boulder_face_09_vertices: MODEL_BOULDER face 9 (global face 76): closed quad, constant stored height -96/256; palette slots 15/15, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcda2]
; BOULDER_VERTEX_00: cache 40h, source vertex 68; azimuth 0/256 turn, height -96/256, radial factor 112.
; BOULDER_VERTEX_02: cache 42h, source vertex 70; azimuth 64/256 turn, height -96/256, radial factor 112.
; BOULDER_VERTEX_04: cache 44h, source vertex 72; azimuth 128/256 turn, height -96/256, radial factor 112.
; BOULDER_VERTEX_06: cache 46h, source vertex 74; azimuth 192/256 turn, height -96/256, radial factor 112.
    db BOULDER_VERTEX_00, BOULDER_VERTEX_02, BOULDER_VERTEX_04, BOULDER_VERTEX_06, BOULDER_VERTEX_00 ; final vertex closes the polygon

; model_vertex_loop_54f7: closed vertex loop for MODEL_SENTINEL_PEDESTAL face 0 (global 137) [load+0xcda7]
    db 0x41, 0x46, 0x45, 0x40, 0x41

; model_vertex_loop_54fc: closed vertex loop for MODEL_SENTINEL_PEDESTAL face 1 (global 138) [load+0xcdac]
    db 0x42, 0x48, 0x47, 0x41, 0x42

; model_vertex_loop_5501: closed vertex loop for MODEL_SENTINEL_PEDESTAL face 2 (global 139) [load+0xcdb1]
    db 0x43, 0x4a, 0x49, 0x42, 0x43

; model_vertex_loop_5506: closed vertex loop for MODEL_SENTINEL_PEDESTAL face 3 (global 140) [load+0xcdb6]
    db 0x40, 0x44, 0x4b, 0x43, 0x40

; model_vertex_loop_550b: closed vertex loop for MODEL_SENTINEL_PEDESTAL face 4 (global 141) [load+0xcdbb]
    db 0x40, 0x45, 0x44, 0x40

; model_vertex_loop_550f: closed vertex loop for MODEL_SENTINEL_PEDESTAL face 5 (global 142) [load+0xcdbf]
    db 0x41, 0x47, 0x46, 0x41

; model_vertex_loop_5513: closed vertex loop for MODEL_SENTINEL_PEDESTAL face 6 (global 143) [load+0xcdc3]
    db 0x42, 0x49, 0x48, 0x42

; model_vertex_loop_5517: closed vertex loop for MODEL_SENTINEL_PEDESTAL face 7 (global 144) [load+0xcdc7]
    db 0x43, 0x4b, 0x4a, 0x43

; model_vertex_loop_551b: closed vertex loop for MODEL_SENTINEL_PEDESTAL face 8 (global 145) [load+0xcdcb]
    db 0x46, 0x47, 0x48, 0x49, 0x46

; model_vertex_loop_5520: closed vertex loop for MODEL_SENTINEL_PEDESTAL face 9 (global 146) [load+0xcdd0]
    db 0x46, 0x49, 0x4a, 0x45, 0x46

; model_vertex_loop_5525: closed vertex loop for MODEL_SENTINEL_PEDESTAL face 10 (global 147) [load+0xcdd5]
    db 0x45, 0x4a, 0x4b, 0x44, 0x45

; model_vertex_loop_552a: closed vertex loop for MODEL_TEXT_BLOCK_01 face 0 (global 148), MODEL_TEXT_BLOCK_10 face 0 (global 152), MODEL_TEXT_BLOCK_11 face 0 (global 156) [load+0xcdda]
    db 0x40, 0x44, 0x47, 0x43, 0x40

; model_vertex_loop_552f: closed vertex loop for MODEL_TEXT_BLOCK_01 face 1 (global 149), MODEL_TEXT_BLOCK_10 face 1 (global 153), MODEL_TEXT_BLOCK_11 face 1 (global 157) [load+0xcddf]
    db 0x42, 0x46, 0x45, 0x41, 0x42

; model_vertex_loop_5534: closed vertex loop for MODEL_TEXT_BLOCK_01 face 2 (global 150), MODEL_TEXT_BLOCK_10 face 2 (global 154), MODEL_TEXT_BLOCK_11 face 2 (global 158) [load+0xcde4]
    db 0x41, 0x45, 0x44, 0x40, 0x41

; model_vertex_loop_5539: closed vertex loop for MODEL_TEXT_BLOCK_01 face 3 (global 151), MODEL_TEXT_BLOCK_10 face 3 (global 155), MODEL_TEXT_BLOCK_11 face 3 (global 159) [load+0xcde9]
    db 0x45, 0x46, 0x47, 0x44, 0x45

; robot_face_00_vertices: MODEL_ROBOT face 0 (global face 0): closed quad, constant stored height -112/256; palette slots 0/0, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcdee]
; ROBOT_VERTEX_11: cache 4Bh, source vertex 11; azimuth 85/256 turn, height -112/256, radial factor 48.
; ROBOT_VERTEX_10: cache 4Ah, source vertex 10; azimuth 53/256 turn, height -112/256, radial factor 42.
; ROBOT_VERTEX_08: cache 48h, source vertex 8; azimuth 203/256 turn, height -112/256, radial factor 42.
; ROBOT_VERTEX_12: cache 4Ch, source vertex 12; azimuth 171/256 turn, height -112/256, radial factor 48.
    db ROBOT_VERTEX_11, ROBOT_VERTEX_10, ROBOT_VERTEX_08, ROBOT_VERTEX_12, ROBOT_VERTEX_11 ; final vertex closes the polygon

; robot_face_01_vertices: MODEL_ROBOT face 1 (global face 1): closed triangle, constant stored height -112/256; palette slots 0/0, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcdf3]
; ROBOT_VERTEX_10: cache 4Ah, source vertex 10; azimuth 53/256 turn, height -112/256, radial factor 42.
; ROBOT_VERTEX_09: cache 49h, source vertex 9; azimuth 0/256 turn, height -112/256, radial factor 32.
; ROBOT_VERTEX_08: cache 48h, source vertex 8; azimuth 203/256 turn, height -112/256, radial factor 42.
    db ROBOT_VERTEX_10, ROBOT_VERTEX_09, ROBOT_VERTEX_08, ROBOT_VERTEX_10 ; final vertex closes the polygon

; robot_face_02_vertices: MODEL_ROBOT face 2 (global face 2): closed quad, constant stored height -22/256; palette slots 0/0, ordering group 1. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcdf7]
; ROBOT_VERTEX_20: cache 54h, source vertex 20; azimuth 99/256 turn, height -22/256, radial factor 29.
; ROBOT_VERTEX_19: cache 53h, source vertex 19; azimuth 20/256 turn, height -22/256, radial factor 19.
; ROBOT_VERTEX_18: cache 52h, source vertex 18; azimuth 236/256 turn, height -22/256, radial factor 19.
; ROBOT_VERTEX_21: cache 55h, source vertex 21; azimuth 157/256 turn, height -22/256, radial factor 29.
    db ROBOT_VERTEX_20, ROBOT_VERTEX_19, ROBOT_VERTEX_18, ROBOT_VERTEX_21, ROBOT_VERTEX_20 ; final vertex closes the polygon

; robot_face_03_vertices: MODEL_ROBOT face 3 (global face 3): closed quad, stored heights -224/256..-112/256; palette slots 0/2, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xcdfc]
; ROBOT_VERTEX_00: cache 40h, source vertex 0; azimuth 207/256 turn, height -224/256, radial factor 20.
; ROBOT_VERTEX_04: cache 44h, source vertex 4; azimuth 217/256 turn, height -112/256, radial factor 31.
; ROBOT_VERTEX_07: cache 47h, source vertex 7; azimuth 169/256 turn, height -112/256, radial factor 35.
; ROBOT_VERTEX_03: cache 43h, source vertex 3; azimuth 161/256 turn, height -224/256, radial factor 27.
    db ROBOT_VERTEX_00, ROBOT_VERTEX_04, ROBOT_VERTEX_07, ROBOT_VERTEX_03, ROBOT_VERTEX_00 ; final vertex closes the polygon

; robot_face_04_vertices: MODEL_ROBOT face 4 (global face 4): closed quad, stored heights -224/256..-112/256; palette slots 0/2, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce01]
; ROBOT_VERTEX_02: cache 42h, source vertex 2; azimuth 95/256 turn, height -224/256, radial factor 27.
; ROBOT_VERTEX_06: cache 46h, source vertex 6; azimuth 87/256 turn, height -112/256, radial factor 35.
; ROBOT_VERTEX_05: cache 45h, source vertex 5; azimuth 39/256 turn, height -112/256, radial factor 31.
; ROBOT_VERTEX_01: cache 41h, source vertex 1; azimuth 49/256 turn, height -224/256, radial factor 20.
    db ROBOT_VERTEX_02, ROBOT_VERTEX_06, ROBOT_VERTEX_05, ROBOT_VERTEX_01, ROBOT_VERTEX_02 ; final vertex closes the polygon

; robot_face_05_vertices: MODEL_ROBOT face 5 (global face 5): closed quad, stored heights -224/256..-112/256; palette slots 2/0, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce06]
; ROBOT_VERTEX_03: cache 43h, source vertex 3; azimuth 161/256 turn, height -224/256, radial factor 27.
; ROBOT_VERTEX_07: cache 47h, source vertex 7; azimuth 169/256 turn, height -112/256, radial factor 35.
; ROBOT_VERTEX_06: cache 46h, source vertex 6; azimuth 87/256 turn, height -112/256, radial factor 35.
; ROBOT_VERTEX_02: cache 42h, source vertex 2; azimuth 95/256 turn, height -224/256, radial factor 27.
    db ROBOT_VERTEX_03, ROBOT_VERTEX_07, ROBOT_VERTEX_06, ROBOT_VERTEX_02, ROBOT_VERTEX_03 ; final vertex closes the polygon

; robot_face_06_vertices: MODEL_ROBOT face 6 (global face 6): closed quad, stored heights -224/256..-112/256; palette slots 2/0, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce0b]
; ROBOT_VERTEX_01: cache 41h, source vertex 1; azimuth 49/256 turn, height -224/256, radial factor 20.
; ROBOT_VERTEX_05: cache 45h, source vertex 5; azimuth 39/256 turn, height -112/256, radial factor 31.
; ROBOT_VERTEX_04: cache 44h, source vertex 4; azimuth 217/256 turn, height -112/256, radial factor 31.
; ROBOT_VERTEX_00: cache 40h, source vertex 0; azimuth 207/256 turn, height -224/256, radial factor 20.
    db ROBOT_VERTEX_01, ROBOT_VERTEX_05, ROBOT_VERTEX_04, ROBOT_VERTEX_00, ROBOT_VERTEX_01 ; final vertex closes the polygon

; robot_face_07_vertices: MODEL_ROBOT face 7 (global face 7): closed quad, stored heights -112/256..-36/256; palette slots 9/9, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce10]
; ROBOT_VERTEX_08: cache 48h, source vertex 8; azimuth 203/256 turn, height -112/256, radial factor 42.
; ROBOT_VERTEX_13: cache 4Dh, source vertex 13; azimuth 197/256 turn, height -36/256, radial factor 52.
; ROBOT_VERTEX_16: cache 50h, source vertex 16; azimuth 174/256 turn, height -36/256, radial factor 56.
; ROBOT_VERTEX_12: cache 4Ch, source vertex 12; azimuth 171/256 turn, height -112/256, radial factor 48.
    db ROBOT_VERTEX_08, ROBOT_VERTEX_13, ROBOT_VERTEX_16, ROBOT_VERTEX_12, ROBOT_VERTEX_08 ; final vertex closes the polygon

; robot_face_08_vertices: MODEL_ROBOT face 8 (global face 8): closed quad, stored heights -112/256..-36/256; palette slots 9/9, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce15]
; ROBOT_VERTEX_11: cache 4Bh, source vertex 11; azimuth 85/256 turn, height -112/256, radial factor 48.
; ROBOT_VERTEX_15: cache 4Fh, source vertex 15; azimuth 82/256 turn, height -36/256, radial factor 56.
; ROBOT_VERTEX_14: cache 4Eh, source vertex 14; azimuth 59/256 turn, height -36/256, radial factor 52.
; ROBOT_VERTEX_10: cache 4Ah, source vertex 10; azimuth 53/256 turn, height -112/256, radial factor 42.
    db ROBOT_VERTEX_11, ROBOT_VERTEX_15, ROBOT_VERTEX_14, ROBOT_VERTEX_10, ROBOT_VERTEX_11 ; final vertex closes the polygon

; robot_face_09_vertices: MODEL_ROBOT face 9 (global face 9): closed quad, stored heights -112/256..-36/256; palette slots 12/12, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce1a]
; ROBOT_VERTEX_12: cache 4Ch, source vertex 12; azimuth 171/256 turn, height -112/256, radial factor 48.
; ROBOT_VERTEX_16: cache 50h, source vertex 16; azimuth 174/256 turn, height -36/256, radial factor 56.
; ROBOT_VERTEX_15: cache 4Fh, source vertex 15; azimuth 82/256 turn, height -36/256, radial factor 56.
; ROBOT_VERTEX_11: cache 4Bh, source vertex 11; azimuth 85/256 turn, height -112/256, radial factor 48.
    db ROBOT_VERTEX_12, ROBOT_VERTEX_16, ROBOT_VERTEX_15, ROBOT_VERTEX_11, ROBOT_VERTEX_12 ; final vertex closes the polygon

; model_vertex_loop_556f: closed vertex loop for MODEL_ROBOT face 10 (global 10) [load+0xce1f]
    db 0x4a, 0x5c, 0x49, 0x4a

; model_vertex_loop_5573: closed vertex loop for MODEL_ROBOT face 11 (global 11) [load+0xce23]
    db 0x49, 0x5c, 0x48, 0x49

; model_vertex_loop_5577: closed vertex loop for MODEL_ROBOT face 12 (global 12) [load+0xce27]
    db 0x4a, 0x4e, 0x5c, 0x4a

; model_vertex_loop_557b: closed vertex loop for MODEL_ROBOT face 13 (global 13) [load+0xce2b]
    db 0x48, 0x5c, 0x4d, 0x48

; model_vertex_loop_557f: closed vertex loop for MODEL_ROBOT face 14 (global 14) [load+0xce2f]
    db 0x4e, 0x51, 0x5c, 0x4e

; model_vertex_loop_5583: closed vertex loop for MODEL_ROBOT face 15 (global 15) [load+0xce33]
    db 0x5c, 0x51, 0x4d, 0x5c

; model_vertex_loop_5587: closed vertex loop for MODEL_ROBOT face 16 (global 16) [load+0xce37]
    db 0x4f, 0x51, 0x4e, 0x4f

; model_vertex_loop_558b: closed vertex loop for MODEL_ROBOT face 17 (global 17) [load+0xce3b]
    db 0x4d, 0x51, 0x50, 0x4d

; model_vertex_loop_558f: closed vertex loop for MODEL_ROBOT face 18 (global 18) [load+0xce3f]
    db 0x50, 0x51, 0x4f, 0x50

; model_vertex_loop_5593: closed vertex loop for MODEL_ROBOT face 19 (global 19) [load+0xce43]
    db 0x53, 0x5a, 0x5b, 0x52, 0x53

; model_vertex_loop_5598: closed vertex loop for MODEL_ROBOT face 20 (global 20) [load+0xce48]
    db 0x52, 0x5b, 0x55, 0x52

; model_vertex_loop_559c: closed vertex loop for MODEL_ROBOT face 21 (global 21) [load+0xce4c]
    db 0x54, 0x5a, 0x53, 0x54

; model_vertex_loop_55a0: closed vertex loop for MODEL_ROBOT face 22 (global 22) [load+0xce50]
    db 0x5b, 0x56, 0x59, 0x55, 0x5b

; model_vertex_loop_55a5: closed vertex loop for MODEL_ROBOT face 23 (global 23) [load+0xce55]
    db 0x54, 0x58, 0x57, 0x5a, 0x54

; model_vertex_loop_55aa: closed vertex loop for MODEL_ROBOT face 24 (global 24) [load+0xce5a]
    db 0x55, 0x59, 0x58, 0x54, 0x55

; model_vertex_loop_55af: closed vertex loop for MODEL_ROBOT face 25 (global 25) [load+0xce5f]
    db 0x57, 0x58, 0x59, 0x56, 0x57

; model_vertex_loop_55b4: closed vertex loop for MODEL_ROBOT face 26 (global 26) [load+0xce64]
    db 0x5a, 0x57, 0x56, 0x5b, 0x5a

; sentry_face_00_vertices: MODEL_SENTRY face 0 (global face 27): closed quad, stored heights -224/256..-54/256; palette slots 0/0, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce69]
; SENTRY_VERTEX_03: cache 43h, source vertex 32; azimuth 166/256 turn, height -224/256, radial factor 42.
; SENTRY_VERTEX_08: cache 48h, source vertex 37; azimuth 166/256 turn, height -54/256, radial factor 42.
; SENTRY_VERTEX_07: cache 47h, source vertex 36; azimuth 90/256 turn, height -54/256, radial factor 42.
; SENTRY_VERTEX_02: cache 42h, source vertex 31; azimuth 90/256 turn, height -224/256, radial factor 42.
    db SENTRY_VERTEX_03, SENTRY_VERTEX_08, SENTRY_VERTEX_07, SENTRY_VERTEX_02, SENTRY_VERTEX_03 ; final vertex closes the polygon

; sentry_face_01_vertices: MODEL_SENTRY face 1 (global face 28): closed triangle, stored heights -224/256..-54/256; palette slots 0/0, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce6e]
; SENTRY_VERTEX_00: cache 40h, source vertex 29; azimuth 218/256 turn, height -224/256, radial factor 42.
; SENTRY_VERTEX_09: cache 49h, source vertex 38; azimuth 192/256 turn, height -54/256, radial factor 54.
; SENTRY_VERTEX_03: cache 43h, source vertex 32; azimuth 166/256 turn, height -224/256, radial factor 42.
    db SENTRY_VERTEX_00, SENTRY_VERTEX_09, SENTRY_VERTEX_03, SENTRY_VERTEX_00 ; final vertex closes the polygon

; sentry_face_02_vertices: MODEL_SENTRY face 2 (global face 29): closed triangle, stored heights -224/256..-54/256; palette slots 0/0, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce72]
; SENTRY_VERTEX_02: cache 42h, source vertex 31; azimuth 90/256 turn, height -224/256, radial factor 42.
; SENTRY_VERTEX_06: cache 46h, source vertex 35; azimuth 64/256 turn, height -54/256, radial factor 54.
; SENTRY_VERTEX_01: cache 41h, source vertex 30; azimuth 38/256 turn, height -224/256, radial factor 42.
    db SENTRY_VERTEX_02, SENTRY_VERTEX_06, SENTRY_VERTEX_01, SENTRY_VERTEX_02 ; final vertex closes the polygon

; sentry_face_03_vertices: MODEL_SENTRY face 3 (global face 30): closed quad, stored heights -224/256..-48/256; palette slots 0/0, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce76]
; SENTRY_VERTEX_01: cache 41h, source vertex 30; azimuth 38/256 turn, height -224/256, radial factor 42.
; SENTRY_VERTEX_05: cache 45h, source vertex 34; azimuth 11/256 turn, height -48/256, radial factor 50.
; SENTRY_VERTEX_04: cache 44h, source vertex 33; azimuth 245/256 turn, height -48/256, radial factor 50.
; SENTRY_VERTEX_00: cache 40h, source vertex 29; azimuth 218/256 turn, height -224/256, radial factor 42.
    db SENTRY_VERTEX_01, SENTRY_VERTEX_05, SENTRY_VERTEX_04, SENTRY_VERTEX_00, SENTRY_VERTEX_01 ; final vertex closes the polygon

; sentry_face_04_vertices: MODEL_SENTRY face 4 (global face 31): closed triangle, stored heights -224/256..-54/256; palette slots 6/6, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce7b]
; SENTRY_VERTEX_03: cache 43h, source vertex 32; azimuth 166/256 turn, height -224/256, radial factor 42.
; SENTRY_VERTEX_09: cache 49h, source vertex 38; azimuth 192/256 turn, height -54/256, radial factor 54.
; SENTRY_VERTEX_08: cache 48h, source vertex 37; azimuth 166/256 turn, height -54/256, radial factor 42.
    db SENTRY_VERTEX_03, SENTRY_VERTEX_09, SENTRY_VERTEX_08, SENTRY_VERTEX_03 ; final vertex closes the polygon

; sentry_face_05_vertices: MODEL_SENTRY face 5 (global face 32): closed triangle, stored heights -224/256..-54/256; palette slots 6/6, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce7f]
; SENTRY_VERTEX_02: cache 42h, source vertex 31; azimuth 90/256 turn, height -224/256, radial factor 42.
; SENTRY_VERTEX_07: cache 47h, source vertex 36; azimuth 90/256 turn, height -54/256, radial factor 42.
; SENTRY_VERTEX_06: cache 46h, source vertex 35; azimuth 64/256 turn, height -54/256, radial factor 54.
    db SENTRY_VERTEX_02, SENTRY_VERTEX_07, SENTRY_VERTEX_06, SENTRY_VERTEX_02 ; final vertex closes the polygon

; sentry_face_06_vertices: MODEL_SENTRY face 6 (global face 33): closed triangle, stored heights -224/256..-48/256; palette slots 6/6, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce83]
; SENTRY_VERTEX_01: cache 41h, source vertex 30; azimuth 38/256 turn, height -224/256, radial factor 42.
; SENTRY_VERTEX_06: cache 46h, source vertex 35; azimuth 64/256 turn, height -54/256, radial factor 54.
; SENTRY_VERTEX_05: cache 45h, source vertex 34; azimuth 11/256 turn, height -48/256, radial factor 50.
    db SENTRY_VERTEX_01, SENTRY_VERTEX_06, SENTRY_VERTEX_05, SENTRY_VERTEX_01 ; final vertex closes the polygon

; sentry_face_07_vertices: MODEL_SENTRY face 7 (global face 34): closed triangle, stored heights -224/256..-48/256; palette slots 6/6, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce87]
; SENTRY_VERTEX_00: cache 40h, source vertex 29; azimuth 218/256 turn, height -224/256, radial factor 42.
; SENTRY_VERTEX_04: cache 44h, source vertex 33; azimuth 245/256 turn, height -48/256, radial factor 50.
; SENTRY_VERTEX_09: cache 49h, source vertex 38; azimuth 192/256 turn, height -54/256, radial factor 54.
    db SENTRY_VERTEX_00, SENTRY_VERTEX_04, SENTRY_VERTEX_09, SENTRY_VERTEX_00 ; final vertex closes the polygon

; sentry_face_08_vertices: MODEL_SENTRY face 8 (global face 35): closed triangle, stored heights -54/256..-16/256; palette slots 6/6, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce8b]
; SENTRY_VERTEX_09: cache 49h, source vertex 38; azimuth 192/256 turn, height -54/256, radial factor 54.
; SENTRY_VERTEX_13: cache 4Dh, source vertex 42; azimuth 166/256 turn, height -16/256, radial factor 34.
; SENTRY_VERTEX_08: cache 48h, source vertex 37; azimuth 166/256 turn, height -54/256, radial factor 42.
    db SENTRY_VERTEX_09, SENTRY_VERTEX_13, SENTRY_VERTEX_08, SENTRY_VERTEX_09 ; final vertex closes the polygon

; sentry_face_09_vertices: MODEL_SENTRY face 9 (global face 36): closed quad, stored heights -54/256..-16/256; palette slots 6/6, ordering group 2. Named IDs address the model-local projection cache; polar components below use global source vertex indices. Closure repeats the first vertex. [load+0xce8f]
; SENTRY_VERTEX_08: cache 48h, source vertex 37; azimuth 166/256 turn, height -54/256, radial factor 42.
; SENTRY_VERTEX_13: cache 4Dh, source vertex 42; azimuth 166/256 turn, height -16/256, radial factor 34.
; SENTRY_VERTEX_12: cache 4Ch, source vertex 41; azimuth 90/256 turn, height -16/256, radial factor 34.
; SENTRY_VERTEX_07: cache 47h, source vertex 36; azimuth 90/256 turn, height -54/256, radial factor 42.
    db SENTRY_VERTEX_08, SENTRY_VERTEX_13, SENTRY_VERTEX_12, SENTRY_VERTEX_07, SENTRY_VERTEX_08 ; final vertex closes the polygon

; model_vertex_loop_55e4: closed vertex loop for MODEL_SENTRY face 10 (global 37) [load+0xce94]
    db 0x47, 0x4c, 0x46, 0x47

; model_vertex_loop_55e8: closed vertex loop for MODEL_SENTRY face 11 (global 38) [load+0xce98]
    db 0x46, 0x4b, 0x45, 0x46

; model_vertex_loop_55ec: closed vertex loop for MODEL_SENTRY face 12 (global 39) [load+0xce9c]
    db 0x45, 0x4b, 0x4a, 0x44, 0x45

; model_vertex_loop_55f1: closed vertex loop for MODEL_SENTRY face 13 (global 40) [load+0xcea1]
    db 0x44, 0x4a, 0x49, 0x44

; model_vertex_loop_55f5: closed vertex loop for MODEL_SENTRY face 14 (global 41) [load+0xcea5]
    db 0x49, 0x4a, 0x4d, 0x49

; model_vertex_loop_55f9: closed vertex loop for MODEL_SENTRY face 15 (global 42) [load+0xcea9]
    db 0x46, 0x4c, 0x4b, 0x46

; model_vertex_loop_55fd: closed vertex loop for MODEL_SENTRY face 16 (global 43) [load+0xcead]
    db 0x4b, 0x4f, 0x4e, 0x4a, 0x4b

; model_vertex_loop_5602: closed vertex loop for MODEL_SENTRY face 17 (global 44) [load+0xceb2]
    db 0x4d, 0x55, 0x54, 0x4c, 0x4d

; model_vertex_loop_5607: closed vertex loop for MODEL_SENTRY face 18 (global 45) [load+0xceb7]
    db 0x4a, 0x52, 0x55, 0x4d, 0x4a

; model_vertex_loop_560c: closed vertex loop for MODEL_SENTRY face 19 (global 46) [load+0xcebc]
    db 0x4c, 0x54, 0x53, 0x4b, 0x4c

; model_vertex_loop_5611: closed vertex loop for MODEL_SENTRY face 20 (global 47) [load+0xcec1]
    db 0x53, 0x54, 0x55, 0x52, 0x53

; model_vertex_loop_5616: closed vertex loop for MODEL_SENTRY face 21 (global 48) [load+0xcec6]
    db 0x51, 0x53, 0x52, 0x50, 0x51

; model_vertex_loop_561b: closed vertex loop for MODEL_SENTRY face 22 (global 49) [load+0xcecb]
    db 0x4e, 0x50, 0x52, 0x4a, 0x4e

; model_vertex_loop_5620: closed vertex loop for MODEL_SENTRY face 23 (global 50) [load+0xced0]
    db 0x4b, 0x53, 0x51, 0x4f, 0x4b

; model_vertex_loop_5625: closed vertex loop for MODEL_SENTRY face 24 (global 51) [load+0xced5]
    db 0x4f, 0x51, 0x50, 0x4e, 0x4f

; opaque_sentry_face_loop_trailer: Three 32-byte chunks between the last of the 25 sentry face loops and the sine table: ZERO, ONES_WITH_A0_EF, ZERO_WITH_20. It is excluded from every declared face-pointer/count range. [load+0xceda]
    opaque_init_chunk ZERO
    opaque_init_chunk ONES_WITH_A0_EF
    opaque_init_chunk ZERO_WITH_20

; projection_quarter_sine_lut: 128 quarter-wave components: min(255,round(256*sin(i*pi/256))), i=0..127. CS:1C2B folds the 256-unit heading into two complementary indices and swaps components by quadrant. Values saturate at 255 rather than wrapping the mathematical endpoint 256 to zero. [load+0xcf3a]
    db 0x00, 0x03, 0x06, 0x09, 0x0d, 0x10, 0x13, 0x16, 0x19, 0x1c, 0x1f, 0x22, 0x26, 0x29, 0x2c, 0x2f
    db 0x32, 0x35, 0x38, 0x3b, 0x3e, 0x41, 0x44, 0x47, 0x4a, 0x4d, 0x50, 0x53, 0x56, 0x59, 0x5c, 0x5f
    db 0x62, 0x65, 0x68, 0x6b, 0x6d, 0x70, 0x73, 0x76, 0x79, 0x7b, 0x7e, 0x81, 0x84, 0x86, 0x89, 0x8c
    db 0x8e, 0x91, 0x93, 0x96, 0x98, 0x9b, 0x9d, 0xa0, 0xa2, 0xa5, 0xa7, 0xaa, 0xac, 0xae, 0xb1, 0xb3
    db 0xb5, 0xb7, 0xb9, 0xbc, 0xbe, 0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xcf, 0xd1, 0xd3
    db 0xd5, 0xd7, 0xd8, 0xda, 0xdc, 0xdd, 0xdf, 0xe0, 0xe2, 0xe3, 0xe5, 0xe6, 0xe7, 0xe9, 0xea, 0xeb
    db 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf8, 0xf9, 0xfa, 0xfa
    db 0xfb, 0xfc, 0xfc, 0xfd, 0xfd, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff

; opaque_quarter_sine_trailer: Four 32-byte initializer chunks after the 128-entry quarter-wave sine table. CS:1C2B masks/folds both indices to 0..127, so it cannot consume this block as additional sine samples. The next boundary is the EGA energy-icon tile bank. [load+0xcfba]
    opaque_init_chunk ONES
    opaque_init_chunk ZERO
    opaque_init_chunk ONES_WITH_A0_EF
    opaque_init_chunk ZERO_WITH_20

; status_tiles_ega: 10 8x8 tiles; each row stores plane 0/1/2/3 bytes, bit 7 is leftmost [load+0xd03a]

; Tile 0: STATUS_TILE_BLANK — blank/spacer.
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000

; Tile 1: STATUS_TILE_ROBOT — robot energy icon: 3 energy.
    status_row_ega 0x00999900
    status_row_ega 0x00000000
    status_row_ega 0xcccccccc
    status_row_ega 0xcc0000cc
    status_row_ega 0x00cccc00
    status_row_ega 0x00cccc00
    status_row_ega 0x00000000
    status_row_ega 0x00000000

; Tile 2: STATUS_TILE_TREE_LEFT — tree energy icon: 1 energy, left half.
    status_row_ega 0x00005500
    status_row_ega 0x00005500
    status_row_ega 0x00445544
    status_row_ega 0x00445544
    status_row_ega 0x55445544
    status_row_ega 0x00005500
    status_row_ega 0x00000000
    status_row_ega 0x00000000

; Tile 3: STATUS_TILE_TREE_RIGHT — tree energy icon: 1 energy, right half.
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x55000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000

; Tile 4: STATUS_TILE_BOULDER_LEFT — boulder energy icon: 2 energy, left half.
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00bbbbbb
    status_row_ega 0xbbbbffbb
    status_row_ega 0xbbffffff
    status_row_ega 0xbbffffff
    status_row_ega 0x00000000
    status_row_ega 0x00000000

; Tile 5: STATUS_TILE_BOULDER_RIGHT — boulder energy icon: 2 energy, right half.
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0xbb000000
    status_row_ega 0xbb000000
    status_row_ega 0xbb000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000

; Tile 6: STATUS_TILE_GOLDEN_ROBOT — golden robot energy icon: 15 energy; a HUD denomination, not a separate creatable model.
    status_row_ega 0x00999900
    status_row_ega 0x00000000
    status_row_ega 0x99999999
    status_row_ega 0x99000099
    status_row_ega 0x00999900
    status_row_ega 0x00999900
    status_row_ega 0x00000000
    status_row_ega 0x00000000

; Tile 7: STATUS_TILE_FRAME_LEFT — left end of the outlined status field.
    status_row_ega 0x00000066
    status_row_ega 0x00000066
    status_row_ega 0x00000066
    status_row_ega 0x00000066
    status_row_ega 0x00000066
    status_row_ega 0x00000066
    status_row_ega 0x00000000
    status_row_ega 0x00000000

; Tile 8: STATUS_TILE_FRAME_MIDDLE — top and bottom edges of the status field.
    status_row_ega 0x66666666
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000
    status_row_ega 0x66666666
    status_row_ega 0x00000000
    status_row_ega 0x00000000

; Tile 9: STATUS_TILE_FRAME_RIGHT — right end of the outlined status field.
    status_row_ega 0x66000000
    status_row_ega 0x66000000
    status_row_ega 0x66000000
    status_row_ega 0x66000000
    status_row_ega 0x66000000
    status_row_ega 0x66000000
    status_row_ega 0x00000000
    status_row_ega 0x00000000

; status_tiles_packed: same pixels as EGA, packed high nibble first; used by Tandy and VGA [load+0xd17a]

; Tile 0: STATUS_TILE_BLANK — blank/spacer.
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000

; Tile 1: STATUS_TILE_ROBOT — robot energy icon: 3 energy.
    status_row_packed 0x00999900
    status_row_packed 0x00000000
    status_row_packed 0xcccccccc
    status_row_packed 0xcc0000cc
    status_row_packed 0x00cccc00
    status_row_packed 0x00cccc00
    status_row_packed 0x00000000
    status_row_packed 0x00000000

; Tile 2: STATUS_TILE_TREE_LEFT — tree energy icon: 1 energy, left half.
    status_row_packed 0x00005500
    status_row_packed 0x00005500
    status_row_packed 0x00445544
    status_row_packed 0x00445544
    status_row_packed 0x55445544
    status_row_packed 0x00005500
    status_row_packed 0x00000000
    status_row_packed 0x00000000

; Tile 3: STATUS_TILE_TREE_RIGHT — tree energy icon: 1 energy, right half.
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x55000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000

; Tile 4: STATUS_TILE_BOULDER_LEFT — boulder energy icon: 2 energy, left half.
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00bbbbbb
    status_row_packed 0xbbbbffbb
    status_row_packed 0xbbffffff
    status_row_packed 0xbbffffff
    status_row_packed 0x00000000
    status_row_packed 0x00000000

; Tile 5: STATUS_TILE_BOULDER_RIGHT — boulder energy icon: 2 energy, right half.
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0xbb000000
    status_row_packed 0xbb000000
    status_row_packed 0xbb000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000

; Tile 6: STATUS_TILE_GOLDEN_ROBOT — golden robot energy icon: 15 energy; a HUD denomination, not a separate creatable model.
    status_row_packed 0x00999900
    status_row_packed 0x00000000
    status_row_packed 0x99999999
    status_row_packed 0x99000099
    status_row_packed 0x00999900
    status_row_packed 0x00999900
    status_row_packed 0x00000000
    status_row_packed 0x00000000

; Tile 7: STATUS_TILE_FRAME_LEFT — left end of the outlined status field.
    status_row_packed 0x00000066
    status_row_packed 0x00000066
    status_row_packed 0x00000066
    status_row_packed 0x00000066
    status_row_packed 0x00000066
    status_row_packed 0x00000066
    status_row_packed 0x00000000
    status_row_packed 0x00000000

; Tile 8: STATUS_TILE_FRAME_MIDDLE — top and bottom edges of the status field.
    status_row_packed 0x66666666
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000
    status_row_packed 0x66666666
    status_row_packed 0x00000000
    status_row_packed 0x00000000

; Tile 9: STATUS_TILE_FRAME_RIGHT — right end of the outlined status field.
    status_row_packed 0x66000000
    status_row_packed 0x66000000
    status_row_packed 0x66000000
    status_row_packed 0x66000000
    status_row_packed 0x66000000
    status_row_packed 0x66000000
    status_row_packed 0x00000000
    status_row_packed 0x00000000

; status_tiles_cga: 10 8x8 tiles with independent CGA colors, high bit pair first [load+0xd2ba]

; Tile 0: STATUS_TILE_BLANK — blank/spacer.
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000

; Tile 1: STATUS_TILE_ROBOT — robot energy icon: 3 energy.
    status_row_cga 0x00111100
    status_row_cga 0x00000000
    status_row_cga 0x22222222
    status_row_cga 0x22000022
    status_row_cga 0x00222200
    status_row_cga 0x00222200
    status_row_cga 0x00000000
    status_row_cga 0x00000000

; Tile 2: STATUS_TILE_TREE_LEFT — tree energy icon: 1 energy, left half.
    status_row_cga 0x00001100
    status_row_cga 0x00001100
    status_row_cga 0x00111111
    status_row_cga 0x00111111
    status_row_cga 0x11111111
    status_row_cga 0x00001100
    status_row_cga 0x00000000
    status_row_cga 0x00000000

; Tile 3: STATUS_TILE_TREE_RIGHT — tree energy icon: 1 energy, right half.
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x11000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000

; Tile 4: STATUS_TILE_BOULDER_LEFT — boulder energy icon: 2 energy, left half.
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00111111
    status_row_cga 0x11113311
    status_row_cga 0x11333333
    status_row_cga 0x11333333
    status_row_cga 0x00000000
    status_row_cga 0x00000000

; Tile 5: STATUS_TILE_BOULDER_RIGHT — boulder energy icon: 2 energy, right half.
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x11000000
    status_row_cga 0x11000000
    status_row_cga 0x11000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000

; Tile 6: STATUS_TILE_GOLDEN_ROBOT — golden robot energy icon: 15 energy; a HUD denomination, not a separate creatable model.
    status_row_cga 0x00333300
    status_row_cga 0x00000000
    status_row_cga 0x33333333
    status_row_cga 0x33000033
    status_row_cga 0x00333300
    status_row_cga 0x00333300
    status_row_cga 0x00000000
    status_row_cga 0x00000000

; Tile 7: STATUS_TILE_FRAME_LEFT — left end of the outlined status field.
    status_row_cga 0x00000022
    status_row_cga 0x00000022
    status_row_cga 0x00000022
    status_row_cga 0x00000022
    status_row_cga 0x00000022
    status_row_cga 0x00000022
    status_row_cga 0x00000000
    status_row_cga 0x00000000

; Tile 8: STATUS_TILE_FRAME_MIDDLE — top and bottom edges of the status field.
    status_row_cga 0x22222222
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000
    status_row_cga 0x22222222
    status_row_cga 0x00000000
    status_row_cga 0x00000000

; Tile 9: STATUS_TILE_FRAME_RIGHT — right end of the outlined status field.
    status_row_cga 0x22000000
    status_row_cga 0x22000000
    status_row_cga 0x22000000
    status_row_cga 0x22000000
    status_row_cga 0x22000000
    status_row_cga 0x22000000
    status_row_cga 0x00000000
    status_row_cga 0x00000000

; terrain_quadrant_corner_bias: first quad-corner offset for the four terrain view rotations; corners follow top-left, top-right, bottom-right, bottom-left [load+0xd35a]
    db 0 ; top-left
    db 1 ; top-right
    db TERRAIN_SIDE+1 ; bottom-right
    db TERRAIN_SIDE ; bottom-left

; viewport_origin_high_table: high byte of the 16-bit projection-X bias; viewport_select_preset clears its low byte [load+0xd35e]
    db 0x0000 >> 8 ; preset 0: projection bias 0
    db 0x0200 >> 8 ; preset 1: projection bias 512
    db 0x0000 >> 8 ; preset 2: projection bias 0
    db 0x00 ; g data +0xd361

; viewport_x_min_table: pixel X origins for presets 0..2: full screen, inset, and narrow view [load+0xd362]
    dw 0 ; preset 0
    dw 64 ; preset 1
    dw 0 ; preset 2

; viewport_width_table: pixel widths for presets 0..2; right edge is X origin plus width [load+0xd368]
    dw 320 ; preset 0
    dw 112 ; preset 1
    dw 128 ; preset 2

; viewport_x_limit_table: projected-X clip thresholds for presets 0..2; negative threshold is (value>>1) XOR 80h [load+0xd36e]
    db 20 ; preset 0
    db 20 ; preset 1
    db 8 ; preset 2
    db 0x00 ; g data +0xd371

; terrain_corner_distance: Coarse 8.8 planar-distance cache for two alternating 32-corner rows. CS:2789 stores only distance>>8. Consequently all fractional precision is lost before fog selection at CS:2971; this stores viewpoint distance, not world-cell height or depth-buffer values. [load+0xd372]
    times TERRAIN_SIDE db 0 ; bank 0: coarse projected distances
    times TERRAIN_SIDE db 0 ; bank 1: coarse projected distances

; terrain_corner_distance_tail: 32 zero initializer bytes following the two 32-corner distance banks. CS:2789 writes by projected_vertex_slot and terrain drawing addresses slots 00h..3Fh; CS:2971/2993 read the same corner slots. These bytes are beyond that 64-byte footprint; an independent consumer remains unconfirmed. They are not additional terrain rows. [load+0xd3b2]
    times TERRAIN_SIDE db 0

; terrain_rows_remaining: Far-to-near row budget: starts at 32 at CS:2444 and decreases per traversed row. In shading mode zero it also supplies fog input, producing bank zero at 32 then banks 15..0 for inputs 31..0; this initial wrap follows byte arithmetic rather than a clamp. [load+0xd3d2]
    db 0x00

; view_grid_x_overflow: Signed horizontal camera excess outside the clamped 0..31 view_grid_x range; corner projection subtracts it to retain the unclamped camera position. [load+0xd3d3]
    db 0x00

; terrain_shading_mode: Fog input selector at CS:2971: 0 uses terrain_rows_remaining; 1 uses (clamp(corner_distance,16,78)-16)>>1; other values use min(corner_distance,31). The chosen input is then mapped to a DAC-bank nibble, not a direct object/text palette index. [load+0xd3d4]
    db 0x00
    db 0x00 ; g data +0xd3d5

; terrain_visible_bit_masks: eight masks indexed by two times X modulo four plus Y parity; paired with terrain_visibility_bits [load+0xd3d6]
    db 0x80 >> 0 ; X modulo 4 = 0, Y parity 0
    db 0x80 >> 1 ; X modulo 4 = 0, Y parity 1
    db 0x80 >> 2 ; X modulo 4 = 1, Y parity 0
    db 0x80 >> 3 ; X modulo 4 = 1, Y parity 1
    db 0x80 >> 4 ; X modulo 4 = 2, Y parity 0
    db 0x80 >> 5 ; X modulo 4 = 2, Y parity 1
    db 0x80 >> 6 ; X modulo 4 = 3, Y parity 0
    db 0x80 >> 7 ; X modulo 4 = 3, Y parity 1

; selection_view_total_columns: Inclusive selected redraw width in eight-pixel columns: min(39,floor((center+half_angle)/128))-max(0,floor((center-half_angle)/128))+1, after the native signed/offscreen tests. Range is 1..40 on success. Both boundary columns are included even when a bound lies exactly on a column boundary; the first rendering pass is capped to 20 and any remainder uses the second 160-pixel half-buffer. [load+0xd3de]
    db 0x00

; selection_view_origin_column: Clipped left redraw column 0..39 saved by CS:3603. selection_view_column advances during the two render passes, but this original column remains fixed for presentation. A negative signed left angular bound clips to zero; a left bound already at column 40 or beyond rejects the update. Matching the player slot returns carry before consuming an extent override, whereas an offscreen model consumes it first. [load+0xd3df]
    db 0x00

; selection_saved_heading: Object heading saved before each temporary camera-angle adjustment and restored after terrain rendering. [load+0xd3e0]
    db 0x00

; selection_copy_iteration_budget: Copy-iteration budget: presentation setup loads 25, or 40 for one signed render mode. Randomized copy loops decrement it after groups of pixel operations; this is not a framebuffer row count. [load+0xd3e1]
    db 0x00

; selection_copy_origin: Presentation origin as a count of eight-pixel columns, not a near pointer or video-memory address. CS:359C sign-extends selection_view_origin_column with CBW; valid origins 0..39 therefore produce an ordinary nonnegative word. The adapter strip/reveal callbacks convert this column count into their own byte/pixel addressing units. [load+0xd3e2]
    dw 0x0000

; ray_contact_edge_selectors: Eight contact cases map to edge indices 0..3 in the closed height ring A,C,D,B,A. The index also encodes the interpolation coordinate: bit 0 selects X fraction instead of Y; bit 1 complements that fraction. [load+0xd3e4]
    db 0 ; contact case 0: edge A-C
    db RAY_CONTACT_USE_X | RAY_CONTACT_COMPLEMENT ; contact case 1: edge B-A
    db RAY_CONTACT_USE_X ; contact case 2: edge C-D
    db 0 ; contact case 3: edge A-C
    db RAY_CONTACT_USE_X ; contact case 4: edge C-D
    db RAY_CONTACT_COMPLEMENT ; contact case 5: edge D-B
    db RAY_CONTACT_COMPLEMENT ; contact case 6: edge D-B
    db RAY_CONTACT_USE_X | RAY_CONTACT_COMPLEMENT ; contact case 7: edge B-A

; model_selection_extents: Seven unsigned 8.8 world-cell half-extents for gameplay models, not pixel widths. CS:3603 takes max(table[model],selection_extent_override), clears the override, and projects that positive extent against model distance to obtain an angular half-width. The redraw interval centers on model_azimuth_bias. Text-block models have no entries; redraw callers must use a gameplay model ID. [load+0xd3ec]
    db 62 ; MODEL_ROBOT: horizontal selection extent parameter
    db 70 ; MODEL_SENTRY: horizontal selection extent parameter
    db 114 ; MODEL_TREE: horizontal selection extent parameter
    db 122 ; MODEL_BOULDER: horizontal selection extent parameter
    db 74 ; MODEL_MEANIE: horizontal selection extent parameter
    db 78 ; MODEL_SENTINEL: horizontal selection extent parameter
    db 193 ; MODEL_SENTINEL_PEDESTAL: horizontal selection extent parameter

; object_energy_values: Energy costs/refunds indexed by model: robot 3, sentry 3, tree 1, boulder 2, Meanie 1, Sentinel 4, Sentinel pedestal 0. Text-block models are rendering primitives and have no energy entries. [load+0xd3f3]
    db ENERGY_ROBOT ; MODEL_ROBOT: 3 energy spent/absorbed
    db ENERGY_SENTRY ; MODEL_SENTRY: 3 energy spent/absorbed
    db ENERGY_TREE ; MODEL_TREE: 1 energy spent/absorbed
    db ENERGY_BOULDER ; MODEL_BOULDER: 2 energy spent/absorbed
    db ENERGY_MEANIE ; MODEL_MEANIE: 1 energy spent/absorbed
    db ENERGY_SENTINEL ; MODEL_SENTINEL: 4 energy spent/absorbed
    db ENERGY_SENTINEL_PEDESTAL ; MODEL_SENTINEL_PEDESTAL: 0 energy spent/absorbed

; camera_transition_steps: signed angular-byte increments for transition modes 0..3; 3C2Bh changes heading for modes 0/1 and pitch for modes 2/3; one full turn is 256 units [load+0xd3fa]
    db 20 ; mode 0: heading increment
    db -8 ; mode 1: heading increment
    db 4 ; mode 2: pitch increment
    db -12 ; mode 3: pitch increment

; video_transition_countdown: Signed saturating countdown: timer tick 3834h decrements it but clamps underflow at zero; the transition loop waits for zero, then reloads floor(3*AL/2) before presenting the next step. [load+0xd3fe]
    db 0x00

; view_y_end_presets: two Y-end clip presets read by the common 38B4h setup path [load+0xd3ff]
    db 240 ; preset 0
    db 240 ; preset 1

; view_y_min_presets: two Y-min clip presets read by the common 38B4h setup path; these are unsigned coordinates [load+0xd401]
    db 176 ; preset 0
    db 48 ; preset 1
    db 0x00 ; g data +0xd403

; preview_escape_restarts: Startup clears this byte; accepting generated landscape placement sets it to one. The key-wait handler uses it to select Escape behavior: zero exits through hardware cleanup, nonzero returns to the startup/preview flow. [load+0xd404]
    db 0x00

; cursor_mode_toggle_latch: Space/alternate-key debounce byte. 3D8Dh skips another toggle while nonzero; the input path stores 80h after a toggle and zero after release at 3DAEh. Mouse cursor mode has separate state. [load+0xd405]
    db 0x00

; landscape_zero_entry_bcd: Built-in packed-BCD entry copied into number_entry_buffer at 3B97h when landscape zero skips secret-code input; pairs are stored least-significant first, representing 63589488 [load+0xd406]
    db (8 << 4) | 8 ; decimal pair 88, digits 0..1 from the right
    db (9 << 4) | 4 ; decimal pair 94, digits 2..3 from the right
    db (5 << 4) | 8 ; decimal pair 58, digits 4..5 from the right
    db (6 << 4) | 3 ; decimal pair 63, digits 6..7 from the right

; intro_once_flags: One-shot startup latch, initialized to bit 0. SHR at 3B4Bh puts that bit into carry; carry set plays the five-note intro, later visits skip it. [load+0xd40a]
    db 0x01

; camera_transition_pitch_targets: pitch stop values for transition modes 2/3; the original lookup uses a base two bytes before this table, so DI=2/3 selects these two bytes [load+0xd40b]
    db 53 ; mode 2: pitch target byte
    db -51 ; mode 3: pitch target byte

; sentry_countdown_phase: Fractional-clock accumulator: CS:3F9C adds 161 modulo 256 and updates all 24 delays only when that addition does not carry. Every complete 256 eligible calls therefore yields 95 decrements, independent of initial phase. Pause, scene suspension, game end and sentry_clock_gate_flags can suppress calls; these are not raw IRQ counts or fixed seconds. [load+0xd40d]
    db 0x00

; input_key_bindings: 20 parallel binding entries. CS:3FC7 visits the requested prefix in reverse order, so the lowest binding index wins when multiple pressed keys target one action slot. Physical arrow keys 0..3 therefore override raw alternatives 4..7. Duplicate F2 entries 14/15 target the same slot; entry 14 wins, so entry 15 payload 1 cannot win while both share that scan ID. Scan IDs 79h..7Ch remain unconfirmed. [load+0xd40e]
    db SCANCODE_LEFT ; binding 0
    db SCANCODE_RIGHT ; binding 1
    db SCANCODE_UP ; binding 2
    db SCANCODE_DOWN ; binding 3
    db SCANCODE_RAW_7A ; binding 4
    db SCANCODE_RAW_79 ; binding 5
    db SCANCODE_RAW_7C ; binding 6
    db SCANCODE_RAW_7B ; binding 7
    db SCANCODE_A ; binding 8
    db SCANCODE_Q ; binding 9
    db SCANCODE_R ; binding 10
    db SCANCODE_T ; binding 11
    db SCANCODE_B ; binding 12
    db SCANCODE_H ; binding 13
    db SCANCODE_F2 ; binding 14
    db SCANCODE_F2 ; binding 15
    db SCANCODE_F4 ; binding 16
    db SCANCODE_F3 ; binding 17
    db SCANCODE_U ; binding 18
    db SCANCODE_F1 ; binding 19

; input_binding_actions: 20 packed slot/value actions. Low two bits select horizontal, object-action, vertical or auxiliary slot; the upper six bits are copied without key-edge detection. Active input visits entries 0..19; held-direction checking visits only 0..7; suspended/end handling visits 0..17. Thus turn-around (18) and overview (19) are excluded from that last prefix, while F2/F4/F3 auxiliary controls remain included. [load+0xd422]
    db (1 << 2) | 0 ; binding 0, destination slot 0
    db (0 << 2) | 0 ; binding 1, destination slot 0
    db (2 << 2) | 2 ; binding 2, destination slot 2
    db (3 << 2) | 2 ; binding 3, destination slot 2
    db (1 << 2) | 0 ; binding 4, destination slot 0
    db (0 << 2) | 0 ; binding 5, destination slot 0
    db (2 << 2) | 2 ; binding 6, destination slot 2
    db (3 << 2) | 2 ; binding 7, destination slot 2
    db (ACTION_ABSORB << 2) | 1 ; binding 8, destination slot 1
    db (ACTION_TRANSFER << 2) | 1 ; binding 9, destination slot 1
    db (ACTION_CREATE_ROBOT << 2) | 1 ; binding 10, destination slot 1
    db (ACTION_CREATE_TREE << 2) | 1 ; binding 11, destination slot 1
    db (ACTION_CREATE_BOULDER << 2) | 1 ; binding 12, destination slot 1
    db (ACTION_HYPERSPACE << 2) | 1 ; binding 13, destination slot 1
    db (0 << 2) | 3 ; binding 14, destination slot 3
    db (1 << 2) | 3 ; binding 15, destination slot 3
    db (2 << 2) | 3 ; binding 16, destination slot 3
    db (3 << 2) | 3 ; binding 17, destination slot 3
    db (ACTION_TURN_AROUND << 2) | 1 ; binding 18, destination slot 1
    db (36 << 2) | 1 ; binding 19, destination slot 1

; scene_camera_height_table: two scene presets: integer height for camera object slot 16 [load+0xd436]
    db 33 ; preset 0
    db 75 ; preset 1

; scene_camera_pitch_table: two scene presets: signed camera pitch bytes [load+0xd438]
    db -22, -39

; scene_camera_x_table: two scene presets: byte grid-X coordinate for camera slot 16 [load+0xd43a]
    db 15 ; preset 0
    db 0 ; preset 1

; scene_lateral_offset_table: two scene presets: signed lateral projection offset [load+0xd43c]
    db 0, -17

; scene_camera_y_table: two scene presets: byte grid-Y coordinate for camera slot 16 [load+0xd43e]
    db 194 ; preset 0
    db 191 ; preset 1

; scene_camera_heading_table: two scene presets: camera heading, 256 units per turn [load+0xd440]
    db   0 ; preset 0: 0 degrees
    db  18 ; preset 1: 25.3125 degrees

; scene_text_color_presets_a: four scene-background-style color presets, patched into glyph formatter parameter A; positive opcode-12h parameters select the palette index stored at DS:379A [load+0xd442]
    db TEXT_COLOR_INDEX_0 ; background style 0
    db TEXT_COLOR_INDEX_2 ; background style 1
    db TEXT_COLOR_INDEX_1 ; background style 2
    db TEXT_COLOR_INDEX_1 ; background style 3

; scene_text_color_presets_b: four scene-background-style color presets, patched into glyph formatter parameter B; positive opcode-12h parameters select the palette index stored at DS:379A [load+0xd446]
    db TEXT_COLOR_INDEX_0 ; background style 0
    db TEXT_COLOR_INDEX_3 ; background style 1
    db TEXT_COLOR_INDEX_2 ; background style 2
    db TEXT_COLOR_INDEX_2 ; background style 3

; scene_view_preset: Scene camera-preset selector saved from DL. Model drawing tests zero for the alternate scale/height treatment; scene completion changes preset zero to 7Fh after clearing visibility bits. [load+0xd44a]
    db 0x01

; scene_lateral_offset: Signed lateral camera offset loaded from the selected scene preset, copied to view_lateral_offset before terrain drawing. [load+0xd44b]
    db 0x00

; sentry_sight_saved_source_slot: Source sentry slot BL saved by sentry_test_target_sight before projection and ray helpers reuse BX. The sight routine restores BL from this byte on every return through 45E4h; the candidate target DL is restored separately. [load+0xd44c]
    db 0x00

; sentry_scheduler_saved_sp: Stack pointer saved at entry to the sentry scheduler (42FAh). A nested object-update/presentation path restores it at 4813h and jumps directly to scheduler completion at 4321h, discarding intermediate return addresses. [load+0xd44d]
    dw 0x0000

; placement_count_masks: Nine masks 2^(8-index)-1. CS:4259 chooses mask=2^bit_length(candidate_count)-1 for a nonzero count, then placement rejects random values >=count. Even a power-of-two count uses twice that many candidates, retaining an unbiased rejection sampler at half acceptance. Principal placement has at most 64 candidates and uses masks 1..127; the zero-candidate path returns carry before looking up a mask. [load+0xd44f]
    db (1 << 8)-1 ; mask width 8 bits, lookup index 0
    db (1 << 7)-1 ; mask width 7 bits, lookup index 1
    db (1 << 6)-1 ; mask width 6 bits, lookup index 2
    db (1 << 5)-1 ; mask width 5 bits, lookup index 3
    db (1 << 4)-1 ; mask width 4 bits, lookup index 4
    db (1 << 3)-1 ; mask width 3 bits, lookup index 5
    db (1 << 2)-1 ; mask width 2 bits, lookup index 6
    db (1 << 1)-1 ; mask width 1 bits, lookup index 7
    db (1 << 0)-1 ; mask width 0 bits, lookup index 8

; ega_switch_subtypes: Six-entry XLAT table used by video_probe_ega_configuration: index is BIOS AH=12h/BL=10h returned CL shifted right once; AL=4 identifies EGA and this table supplies its internal AH subtype. [load+0xd458]
    db 2 ; BIOS switch CL=0 or 1: internal EGA subtype
    db 3 ; BIOS switch CL=2 or 3: internal EGA subtype
    db 1 ; BIOS switch CL=4 or 5: internal EGA subtype
    db 2 ; BIOS switch CL=6 or 7: internal EGA subtype
    db 3 ; BIOS switch CL=8 or 9: internal EGA subtype
    db 1 ; BIOS switch CL=10 or 11: internal EGA subtype

; bios_display_combination_descriptors: 13 word descriptors indexed by BIOS INT 10h/AX=1A00h display-combination code; 4874h translates both active and alternate codes. Low byte is internal adapter class, high byte its subtype; zero marks an unsupported combination. [load+0xd45e]
    dw 0 | (0 << 8) ; BIOS combination 00h: adapter class 0, subtype 0
    dw 1 | (1 << 8) ; BIOS combination 01h: adapter class 1, subtype 1
    dw 2 | (2 << 8) ; BIOS combination 02h: adapter class 2, subtype 2
    dw 0 | (0 << 8) ; BIOS combination 03h: adapter class 0, subtype 0
    dw 4 | (3 << 8) ; BIOS combination 04h: adapter class 4, subtype 3
    dw 4 | (1 << 8) ; BIOS combination 05h: adapter class 4, subtype 1
    dw 0 | (0 << 8) ; BIOS combination 06h: adapter class 0, subtype 0
    dw 6 | (4 << 8) ; BIOS combination 07h: adapter class 6, subtype 4
    dw 6 | (5 << 8) ; BIOS combination 08h: adapter class 6, subtype 5
    dw 0 | (0 << 8) ; BIOS combination 09h: adapter class 0, subtype 0
    dw 5 | (3 << 8) ; BIOS combination 0Ah: adapter class 5, subtype 3
    dw 5 | (4 << 8) ; BIOS combination 0Bh: adapter class 5, subtype 4
    dw 5 | (5 << 8) ; BIOS combination 0Ch: adapter class 5, subtype 5

; video_probe_records: five packed records: one mutable enable byte and a near code pointer; 4836h enables all probes, then each successful probe can disable later probes [load+0xd478]

; BIOS AX=1A00h; authoritative display-combination codes
    db 0 ; enable flag; video_detect_adapters sets this to 1 before probing
    dw video_probe_display_combination-program_image

; BIOS AH=12h/BL=10h; EGA configuration
    db 0 ; enable flag; video_detect_adapters sets this to 1 before probing
    dw video_probe_ega_configuration-program_image

; try mode 8 and inspect its reported text-column count
    db 0 ; enable flag; video_detect_adapters sets this to 1 before probing
    dw video_probe_mode8_columns-program_image

; probe the color CRTC at port 3D4h
    db 0 ; enable flag; video_detect_adapters sets this to 1 before probing
    dw video_probe_color_crtc-program_image

; probe the monochrome CRTC and adapter-status bits
    db 0 ; enable flag; video_detect_adapters sets this to 1 before probing
    dw video_probe_mono_crtc-program_image
    db 0x00 ; g data +0xd487

; saved_hardware_sp: saved game-entry SP, restored on exit and used to patch the outer preview return [load+0xd488]
    dw 0 ; filled at runtime

; saved_int09_offset: saved IRQ1 vector: offset word then segment word, captured with DOS AH=35h and restored with AH=25h [load+0xd48a]
    dw 0 ; saved offset
    dw 0 ; saved segment

; bios_tick_prescaler: triple-rate IRQ0 countdown; chain the original BIOS handler every third game timer tick [load+0xd48e]
    db 3 ; chain the original IRQ0 handler once per three game ticks
    db 0x00 ; g data +0xd48f

; pit_calibration_max: largest sampled PIT counter; threshold 6000h selects original versus triple-rate IRQ0 setup [load+0xd490]
    dw 0 ; filled at runtime

; keyboard_e0_prefix: E0 prefix latch kept separately from key_down; FFh marks a pending extended key [load+0xd492]
    db 0
    db 0x00 ; g data +0xd493

; cga_palette_phase: current CGA phase, updated by cga_set_scene_palette; scatter drawing uses it to choose the visible or work buffer [load+0xd494]
    db SCENE_PALETTE_GAME

; pixel_scatter_sequence_low: Pixel-scatter sequence: state=5*state modulo 2^32. Its odd initializer gives a 2^30-update period with constant low two bits. CGA uses high_word>>2 as the byte offset and high_word&3 as the pixel selector; one scatter request advances once per pixel. All scatter backends share this state, which is separate from both warning animation and landscape generation. [load+0xd495]
    dd 0x75843239 ; initial state; pixel_scatter_sequence_high aliases the upper word

; cga_scene_palettes: three phase records: background/intensity control (BIOS 0Bh/BH=0), then 3-color palette selector (BH=1); blue phase also clears video memory [load+0xd499]
; game phase
    db RGBI4_BLACK ; background and intensity
    db CGA_PALETTE_CYAN_MAGENTA_GRAY ; three foreground colors
; title phase
    db RGBI4_BLACK ; background and intensity
    db CGA_PALETTE_CYAN_MAGENTA_GRAY ; three foreground colors
; blue concealment phase
    db RGBI4_LIGHT_BLUE ; background and intensity
    db CGA_PALETTE_GREEN_RED_BROWN ; three foreground colors

; cga_palette_variant_selectors: eight landscape variants selecting green/red/brown or cyan/magenta/gray [load+0xd49f]
    db CGA_PALETTE_GREEN_RED_BROWN ; landscape variant 0
    db CGA_PALETTE_CYAN_MAGENTA_GRAY ; landscape variant 1
    db CGA_PALETTE_GREEN_RED_BROWN ; landscape variant 2
    db CGA_PALETTE_CYAN_MAGENTA_GRAY ; landscape variant 3
    db CGA_PALETTE_GREEN_RED_BROWN ; landscape variant 4
    db CGA_PALETTE_CYAN_MAGENTA_GRAY ; landscape variant 5
    db CGA_PALETTE_GREEN_RED_BROWN ; landscape variant 6
    db CGA_PALETTE_GREEN_RED_BROWN ; landscape variant 7

; cga_background_variant_controls: eight landscape variants: background in low four bits, palette high-intensity flag in bit 4 [load+0xd4a7]
    db RGBI4_BLACK | CGA_PALETTE_BRIGHT ; landscape variant 0
    db RGBI4_BLACK ; landscape variant 1
    db RGBI4_BLACK ; landscape variant 2
    db RGBI4_BLACK ; landscape variant 3
    db RGBI4_BLACK | CGA_PALETTE_BRIGHT ; landscape variant 4
    db RGBI4_BLACK ; landscape variant 5
    db RGBI4_BLACK ; landscape variant 6
    db RGBI4_BLACK | CGA_PALETTE_BRIGHT ; landscape variant 7

; tandy_palette_phase_0: Tandy RGBI: 16 logical pixel-color entries followed by the border color; game colors; slots 1..3 can be changed by landscape selection [load+0xd4af]
    db RGBI4_BLACK ; pixel color slot 0h
    db RGBI4_LIGHT_BLUE ; pixel color slot 1h
    db RGBI4_WHITE ; pixel color slot 2h
    db RGBI4_LIGHT_GREEN ; pixel color slot 3h
    db RGBI4_GREEN ; pixel color slot 4h
    db RGBI4_LIGHT_CYAN ; pixel color slot 5h
    db RGBI4_LIGHT_RED ; pixel color slot 6h
    db RGBI4_BLUE ; pixel color slot 7h
    db RGBI4_LIGHT_GREEN ; pixel color slot 8h
    db RGBI4_YELLOW ; pixel color slot 9h
    db RGBI4_LIGHT_MAGENTA ; pixel color slot Ah
    db RGBI4_CYAN ; pixel color slot Bh
    db RGBI4_LIGHT_BLUE ; pixel color slot Ch
    db RGBI4_BROWN ; pixel color slot Dh
    db RGBI4_WHITE ; pixel color slot Eh
    db RGBI4_BLUE ; pixel color slot Fh
    db RGBI4_BLACK ; border/overscan color

; tandy_palette_phase_1: Tandy RGBI: 16 logical pixel-color entries followed by the border color; title colors [load+0xd4c0]
    db RGBI4_BLACK ; pixel color slot 0h
    db RGBI4_GREEN ; pixel color slot 1h
    db RGBI4_LIGHT_CYAN ; pixel color slot 2h
    db RGBI4_LIGHT_BLUE ; pixel color slot 3h
    db RGBI4_RED ; pixel color slot 4h
    db RGBI4_BLUE ; pixel color slot 5h
    db RGBI4_LIGHT_RED ; pixel color slot 6h
    db RGBI4_BLUE ; pixel color slot 7h
    db RGBI4_BLUE ; pixel color slot 8h
    db RGBI4_BROWN ; pixel color slot 9h
    db RGBI4_BLACK ; pixel color slot Ah
    db RGBI4_BLACK ; pixel color slot Bh
    db RGBI4_BLUE ; pixel color slot Ch
    db RGBI4_BLACK ; pixel color slot Dh
    db RGBI4_LIGHT_GRAY ; pixel color slot Eh
    db RGBI4_YELLOW ; pixel color slot Fh
    db RGBI4_BLACK ; border/overscan color

; tandy_palette_phase_2: Tandy RGBI: 16 logical pixel-color entries followed by the border color; uniform light blue, including the border [load+0xd4d1]
    db RGBI4_LIGHT_BLUE ; pixel color slot 0h
    db RGBI4_LIGHT_BLUE ; pixel color slot 1h
    db RGBI4_LIGHT_BLUE ; pixel color slot 2h
    db RGBI4_LIGHT_BLUE ; pixel color slot 3h
    db RGBI4_LIGHT_BLUE ; pixel color slot 4h
    db RGBI4_LIGHT_BLUE ; pixel color slot 5h
    db RGBI4_LIGHT_BLUE ; pixel color slot 6h
    db RGBI4_LIGHT_BLUE ; pixel color slot 7h
    db RGBI4_LIGHT_BLUE ; pixel color slot 8h
    db RGBI4_LIGHT_BLUE ; pixel color slot 9h
    db RGBI4_LIGHT_BLUE ; pixel color slot Ah
    db RGBI4_LIGHT_BLUE ; pixel color slot Bh
    db RGBI4_LIGHT_BLUE ; pixel color slot Ch
    db RGBI4_LIGHT_BLUE ; pixel color slot Dh
    db RGBI4_LIGHT_BLUE ; pixel color slot Eh
    db RGBI4_LIGHT_BLUE ; pixel color slot Fh
    db RGBI4_LIGHT_BLUE ; border/overscan color

; ega_palette_phase_0: EGA 200-line color selectors: low three RGB bits and bit-4 intensity; 16 slots plus border; game colors; slots 1..3 are mutable [load+0xd4e2]
    db RGBI4_BLACK ; pixel color slot 0h
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot 1h
    db RGBI4_LIGHT_GRAY | EGA_200_INTENSITY ; pixel color slot 2h
    db RGBI4_GREEN | EGA_200_INTENSITY ; pixel color slot 3h
    db RGBI4_GREEN ; pixel color slot 4h
    db RGBI4_GREEN | EGA_200_INTENSITY ; pixel color slot 5h
    db RGBI4_RED | EGA_200_INTENSITY ; pixel color slot 6h
    db RGBI4_BLUE ; pixel color slot 7h
    db RGBI4_GREEN | EGA_200_INTENSITY ; pixel color slot 8h
    db RGBI4_BROWN | EGA_200_INTENSITY ; pixel color slot 9h
    db RGBI4_MAGENTA | EGA_200_INTENSITY ; pixel color slot Ah
    db RGBI4_CYAN | EGA_200_INTENSITY ; pixel color slot Bh
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot Ch
    db RGBI4_BROWN ; pixel color slot Dh
    db RGBI4_LIGHT_GRAY | EGA_200_INTENSITY ; pixel color slot Eh
    db RGBI4_BLUE ; pixel color slot Fh
    db RGBI4_BLACK ; border/overscan color

; ega_palette_phase_1: EGA 200-line color selectors: low three RGB bits and bit-4 intensity; 16 slots plus border; title colors [load+0xd4f3]
    db RGBI4_BLACK ; pixel color slot 0h
    db RGBI4_GREEN ; pixel color slot 1h
    db RGBI4_CYAN | EGA_200_INTENSITY ; pixel color slot 2h
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot 3h
    db RGBI4_RED ; pixel color slot 4h
    db RGBI4_BLUE ; pixel color slot 5h
    db RGBI4_RED | EGA_200_INTENSITY ; pixel color slot 6h
    db RGBI4_BLUE ; pixel color slot 7h
    db RGBI4_BLUE ; pixel color slot 8h
    db RGBI4_BROWN ; pixel color slot 9h
    db RGBI4_BLACK ; pixel color slot Ah
    db RGBI4_BLACK ; pixel color slot Bh
    db RGBI4_BLUE ; pixel color slot Ch
    db RGBI4_BLACK ; pixel color slot Dh
    db RGBI4_LIGHT_GRAY ; pixel color slot Eh
    db RGBI4_BROWN | EGA_200_INTENSITY ; pixel color slot Fh
    db RGBI4_BLACK ; border/overscan color

; ega_palette_phase_2: EGA 200-line color selectors: low three RGB bits and bit-4 intensity; 16 slots plus border; uniform blue, including the border [load+0xd504]
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot 0h
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot 1h
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot 2h
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot 3h
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot 4h
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot 5h
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot 6h
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot 7h
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot 8h
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot 9h
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot Ah
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot Bh
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot Ch
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot Dh
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot Eh
    db RGBI4_BLUE | EGA_200_INTENSITY ; pixel color slot Fh
    db RGBI4_BLUE | EGA_200_INTENSITY ; border/overscan color

; vga_attribute_identity: 16 logical-color to DAC-index mappings plus border; used by VGA with the planar EGA drawing path [load+0xd515]
    db 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 ; attribute slots 0..15 map directly to DAC entries 0..15
    db 0 ; overscan/border selects DAC entry 0

; vga_rgb_palette_phase_0: game RGB colors; entries 1..3 can be changed by the landscape-color hook [load+0xd526]
    dac_rgb6  0,  0,  0 ; color index 00h
    dac_rgb6 39, 39, 63 ; color index 01h
    dac_rgb6 63, 63, 63 ; color index 02h
    dac_rgb6  0, 63,  0 ; color index 03h
    dac_rgb6  0, 31,  0 ; color index 04h
    dac_rgb6 23, 55, 23 ; color index 05h
    dac_rgb6 63,  0,  0 ; color index 06h
    dac_rgb6  0, 15, 39 ; color index 07h
    dac_rgb6  0, 63,  0 ; color index 08h
    dac_rgb6 63, 63,  0 ; color index 09h
    dac_rgb6 39,  0, 39 ; color index 0Ah
    dac_rgb6 23, 47, 47 ; color index 0Bh
    dac_rgb6 39, 39, 63 ; color index 0Ch
    dac_rgb6 31, 15,  0 ; color index 0Dh
    dac_rgb6 63, 63, 63 ; color index 0Eh
    dac_rgb6  0,  0, 23 ; color index 0Fh

; vga_rgb_palette_phase_1: title RGB colors; copied unchanged into all 16 DAC banks [load+0xd556]
    dac_rgb6  0,  0,  0 ; color index 00h
    dac_rgb6  0, 39, 23 ; color index 01h
    dac_rgb6 23, 63, 63 ; color index 02h
    dac_rgb6 31, 31, 55 ; color index 03h
    dac_rgb6 39, 15, 15 ; color index 04h
    dac_rgb6 15, 15, 39 ; color index 05h
    dac_rgb6 55, 31, 31 ; color index 06h
    dac_rgb6  0,  0, 31 ; color index 07h
    dac_rgb6 15, 15, 39 ; color index 08h
    dac_rgb6 39, 39, 15 ; color index 09h
    dac_rgb6  0,  0, 23 ; color index 0Ah
    dac_rgb6  0,  0, 15 ; color index 0Bh
    dac_rgb6 15, 15, 55 ; color index 0Ch
    dac_rgb6  0,  0, 15 ; color index 0Dh
    dac_rgb6 47, 47, 47 ; color index 0Eh
    dac_rgb6 55, 55, 23 ; color index 0Fh

; vga_rgb_palette_phase_2: all 16 colors become full blue, concealing already-rendered pixels [load+0xd586]
    %rep 16
        dac_rgb6 0, 0, 63
    %endrep

; vga_work_palette: 16 mutable RGB triplets; copied from game palette, then darkened between DAC bank uploads [load+0xd5b6]
    %rep 16
        dac_rgb6 0, 0, 0
    %endrep

; tandy_landscape_color_3_variants: eight landscape variants for game palette slot 3; index=(principal object count-1)&7 [load+0xd5e6]
    db RGBI4_LIGHT_GREEN ; variant 0
    db RGBI4_YELLOW ; variant 1
    db RGBI4_LIGHT_CYAN ; variant 2
    db RGBI4_YELLOW ; variant 3
    db RGBI4_WHITE ; variant 4
    db RGBI4_LIGHT_MAGENTA ; variant 5
    db RGBI4_WHITE ; variant 6
    db RGBI4_YELLOW ; variant 7

; tandy_landscape_color_2_variants: eight landscape variants for game palette slot 2; index=(principal object count-1)&7 [load+0xd5ee]
    db RGBI4_WHITE ; variant 0
    db RGBI4_LIGHT_CYAN ; variant 1
    db RGBI4_LIGHT_MAGENTA ; variant 2
    db RGBI4_WHITE ; variant 3
    db RGBI4_LIGHT_CYAN ; variant 4
    db RGBI4_YELLOW ; variant 5
    db RGBI4_LIGHT_MAGENTA ; variant 6
    db RGBI4_LIGHT_RED ; variant 7

; tandy_landscape_color_1_variants: eight landscape variants for game palette slot 1; index=(principal object count-1)&7 [load+0xd5f6]
    db RGBI4_CYAN ; variant 0
    db RGBI4_MAGENTA ; variant 1
    db RGBI4_CYAN ; variant 2
    db RGBI4_BROWN ; variant 3
    db RGBI4_LIGHT_BLUE ; variant 4
    db RGBI4_LIGHT_RED ; variant 5
    db RGBI4_MAGENTA ; variant 6
    db RGBI4_LIGHT_BLUE ; variant 7

; ega_landscape_color_3_variants: eight landscape variants for game palette slot 3; index=(principal object count-1)&7 [load+0xd5fe]
    db RGBI4_GREEN | EGA_200_INTENSITY ; variant 0
    db RGBI4_BROWN | EGA_200_INTENSITY ; variant 1
    db RGBI4_CYAN | EGA_200_INTENSITY ; variant 2
    db RGBI4_BROWN | EGA_200_INTENSITY ; variant 3
    db RGBI4_LIGHT_GRAY | EGA_200_INTENSITY ; variant 4
    db RGBI4_MAGENTA | EGA_200_INTENSITY ; variant 5
    db RGBI4_LIGHT_GRAY | EGA_200_INTENSITY ; variant 6
    db RGBI4_BROWN | EGA_200_INTENSITY ; variant 7

; ega_landscape_color_2_variants: eight landscape variants for game palette slot 2; index=(principal object count-1)&7 [load+0xd606]
    db RGBI4_LIGHT_GRAY | EGA_200_INTENSITY ; variant 0
    db RGBI4_CYAN | EGA_200_INTENSITY ; variant 1
    db RGBI4_MAGENTA | EGA_200_INTENSITY ; variant 2
    db RGBI4_LIGHT_GRAY | EGA_200_INTENSITY ; variant 3
    db RGBI4_CYAN | EGA_200_INTENSITY ; variant 4
    db RGBI4_BROWN | EGA_200_INTENSITY ; variant 5
    db RGBI4_MAGENTA | EGA_200_INTENSITY ; variant 6
    db RGBI4_RED | EGA_200_INTENSITY ; variant 7

; ega_landscape_color_1_variants: eight landscape variants for game palette slot 1; index=(principal object count-1)&7 [load+0xd60e]
    db RGBI4_CYAN ; variant 0
    db RGBI4_MAGENTA ; variant 1
    db RGBI4_CYAN ; variant 2
    db RGBI4_BROWN ; variant 3
    db RGBI4_BLUE | EGA_200_INTENSITY ; variant 4
    db RGBI4_RED | EGA_200_INTENSITY ; variant 5
    db RGBI4_MAGENTA ; variant 6
    db RGBI4_BLUE | EGA_200_INTENSITY ; variant 7

; vga_landscape_color_3_variants: eight landscape variants for game palette slot 3; index=(principal object count-1)&7 [load+0xd616]
    dac_rgb6 0, 63, 0 ; variant 0
    dac_rgb6 63, 63, 39 ; variant 1
    dac_rgb6 39, 63, 63 ; variant 2
    dac_rgb6 63, 63, 39 ; variant 3
    dac_rgb6 63, 63, 63 ; variant 4
    dac_rgb6 63, 47, 47 ; variant 5
    dac_rgb6 63, 63, 63 ; variant 6
    dac_rgb6 63, 63, 0 ; variant 7

; vga_landscape_color_2_variants: eight landscape variants for game palette slot 2; index=(principal object count-1)&7 [load+0xd62e]
    dac_rgb6 63, 63, 63 ; variant 0
    dac_rgb6 39, 63, 63 ; variant 1
    dac_rgb6 63, 47, 47 ; variant 2
    dac_rgb6 63, 63, 63 ; variant 3
    dac_rgb6 39, 63, 63 ; variant 4
    dac_rgb6 63, 63, 39 ; variant 5
    dac_rgb6 63, 39, 63 ; variant 6
    dac_rgb6 63, 0, 0 ; variant 7

; vga_landscape_color_1_variants: eight landscape variants for game palette slot 1; index=(principal object count-1)&7 [load+0xd646]
    dac_rgb6 0, 39, 39 ; variant 0
    dac_rgb6 39, 0, 39 ; variant 1
    dac_rgb6 0, 39, 39 ; variant 2
    dac_rgb6 55, 23, 0 ; variant 3
    dac_rgb6 23, 23, 63 ; variant 4
    dac_rgb6 63, 0, 0 ; variant 5
    dac_rgb6 39, 0, 39 ; variant 6
    dac_rgb6 23, 23, 63 ; variant 7

; cga_clear_pixel_masks: four 2-bit pixels per byte; AND clears the selected pixel, leftmost first [load+0xd65e]
    db (~(0xc0 >> 0)) & 0xff ; pixel 0 from left
    db (~(0xc0 >> 2)) & 0xff ; pixel 1 from left
    db (~(0xc0 >> 4)) & 0xff ; pixel 2 from left
    db (~(0xc0 >> 6)) & 0xff ; pixel 3 from left

; cga_set_pixel_masks: OR selects CGA palette index 1 at the selected pixel [load+0xd662]
    db 0x40 >> 0 ; pixel 0 from left
    db 0x40 >> 2 ; pixel 1 from left
    db 0x40 >> 4 ; pixel 2 from left
    db 0x40 >> 6 ; pixel 3 from left

; tandy_clear_pixel_masks: two 4-bit pixels per byte; clear upper/lower nibble [load+0xd666]
    db 0x0f ; pixel 0 from left
    db 0xf0 ; pixel 1 from left

; tandy_set_pixel_masks: OR selects Tandy palette index 14 in the upper/lower nibble [load+0xd668]
    db 14 << 4 ; pixel 0 from left
    db 14 ; pixel 1 from left

; ega_single_pixel_masks: eight planar pixel positions, bit 7 leftmost [load+0xd66a]
    db 0x80 >> 0 ; pixel 0 from left
    db 0x80 >> 1 ; pixel 1 from left
    db 0x80 >> 2 ; pixel 2 from left
    db 0x80 >> 3 ; pixel 3 from left
    db 0x80 >> 4 ; pixel 4 from left
    db 0x80 >> 5 ; pixel 5 from left
    db 0x80 >> 6 ; pixel 6 from left
    db 0x80 >> 7 ; pixel 7 from left

; mouse_input_state: Seven-byte mouse callback record. CX is stored unchanged as X; Y is 199-DX. Every callback replaces both coordinates and masks BL to two button bits, even when AL lacks the movement-event bit. That bit sets mouse_moved=80h but nonmovement events do not clear it: input processing acknowledges it separately. mouse_installed is an initialization latch and is untouched by the callback. This is a driver mouse record, not the keyboard cursor position. [load+0xd672]
    dw 160 ; mouse_x, initial stored X position
    dw 104 ; mouse_y, initial inverted Y position
    db 0 ; input_buttons: bit 0 left, bit 1 right
    db 0 ; mouse_moved: callback sets 80h on a movement event
    db 0 ; mouse_installed: initialization sets 80h if INT 33h succeeds

; filename_sent: NUL-terminated text; padding remains separate [load+0xd679]
    db 'sent.bin', 0x00
    db 0x00 ; g data +0xd682

; cursor_saved_video_offsets: Observed address-save footprint for 21 cursor steps: CGA/Tandy/VGA use 21 contiguous offset words (stride 2); EGA stores offset words at stride 4, ending at byte 81. Remaining surrounding zeros stay unclassified. [load+0xd683]
    times CURSOR_STEP_COUNT-1 dw 0, 0 ; EGA stride-four address slots; packed backends also use intervening words
    dw 0 ; last EGA offset word: exact observed footprint ends here

; cursor_saved_offsets_capacity_tail: 14 zero bytes after the exact 82-byte EGA cursor-offset footprint. Step 20 writes a word at byte 80; stride-two backends need only the first 42 bytes. The next declared storage is cursor_saved_pixels. [load+0xd6d5]
    unconfirmed_capacity_tail 14

; cursor_saved_pixels: 21 four-byte save records for EGA, read/written in plane order 3,2,1,0. CGA/Tandy save one byte at stride 2; VGA saves one word at stride 2. These original pixels are restored before the next cursor draw. [load+0xd6e3]
    times CURSOR_STEP_COUNT db 0, 0, 0, 0 ; EGA planes 3,2,1,0; packed backends use stride two

; cursor_saved_pixels_capacity_tail: 204 zero bytes after 21 four-byte EGA pixel-save records. Packed backends use at most the first 42 bytes of the preceding save array. The next declared storage is cursor_step_ink_records; the native VGA probe preserves marker bytes throughout this tail. [load+0xd737]
    unconfirmed_capacity_tail 204

; cursor_step_ink_records: 21 duplicated ink/black style pairs for the stepped crosshair. The shape paints 42 pixels: a two-pixel-wide 11-row vertical stem crossing a 22-pixel horizontal arm. Black styles actively replace the old pixels with zero; they are not transparent gaps. CGA ink uses palette index 3, while Tandy/EGA/VGA use 14. Only the first byte of each duplicated pair is tested. [load+0xd803]
    dw CURSOR_DRAW_INK*0x101, 0, CURSOR_DRAW_INK*0x101, 0, CURSOR_DRAW_INK*0x101, CURSOR_DRAW_INK*0x101, 0
    dw CURSOR_DRAW_INK*0x101, 0, CURSOR_DRAW_INK*0x101, 0, CURSOR_DRAW_INK*0x101, 0, CURSOR_DRAW_INK*0x101
    dw 0, CURSOR_DRAW_INK*0x101, CURSOR_DRAW_INK*0x101, 0, CURSOR_DRAW_INK*0x101, 0, CURSOR_DRAW_INK*0x101

; cursor_step_x_deltas: Signed X deltas for an 11-position horizontal sweep in two-pixel groups: five initial vertical steps stay at X=0; the center row jumps to -5 then visits -4..+5; the final vertical run returns to X=0. These are successive path deltas, not absolute bitmap columns. Entry 21 is an unread companion of the Y terminator. [load+0xd82d]
    db 0, 0, 0, 0, 0, -5, 1, 1, 1, 1, 1
    db 1, 1, 1, 1, 1, -5, 0, 0, 0, 0, 0

; cursor_step_scanline_deltas: Scanline-table byte deltas yielding relative rows 0..10. The path visits vertical rows 0..4, all 11 horizontal pairs at row 5, then vertical rows 6..10. Each +2 advances one word-sized scanline-table entry. Final 80h terminates before any X/style/save-buffer access, so only 21 steps are drawn. [load+0xd843]
    db 0, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0
    db 0, 0, 0, 0, 0, 2, 2, 2, 2, 2
    db CURSOR_STEPS_END

; cursor_cga_clear_pair_masks: CGA cursor: X-pair parity selects high or low nibble; AND preserves the other two pixels in the destination byte. [load+0xd859]
    db (~(0x0f << 4)) & 0xff ; clear first pixel pair
    db (~0x0f) & 0xff ; clear second pixel pair

; cursor_cga_set_pair_masks: CGA cursor: OR repeats logical palette index 3 across the selected adjacent pixel pair; applied only for ink records after clearing that pair. [load+0xd85b]
    db CURSOR_CGA_INK*0x50 ; same ink in both high-nibble pixels
    db CURSOR_CGA_INK*0x05 ; same ink in both low-nibble pixels

; cursor_ega_pair_masks: EGA cursor: (X pair coordinate & 3) selects one adjacent pixel pair in an eight-pixel byte. Written to graphics-controller bit-mask register 8 before writing color 0 or 14. [load+0xd85d]
    db 0xc0 >> 0 ; pixel pair 0, bits 7/6
    db 0xc0 >> 2 ; pixel pair 1, bits 5/4
    db 0xc0 >> 4 ; pixel pair 2, bits 3/2
    db 0xc0 >> 6 ; pixel pair 3, bits 1/0

; untraced_signed_step_vectors: Inferred signed-word layout: two vectors {-32,+32,+2560,-2560} and {-16,+16,+2560,-2560}, followed by a zero byte. Both pair opposite steps, halve the horizontal magnitude, and retain 320*8 vertical magnitude. This resembles the live transition-delta tables but their dispatch paths use DS:60F0/6158/61C0 instead; a consumer and adapter assignment for these bytes remain unconfirmed. [load+0xd861]
    dw -32, 32, 320*8, -320*8
    dw -16, 16, 320*8, -320*8
    db 0

; msg_menu_navigation: NUL-terminated text; padding remains separate [load+0xd872]
    db 'Move to menu selection with cursor keys.', 0x00
    db 0x00 ; g data +0xd89b

; msg_menu_select: NUL-terminated text; padding remains separate [load+0xd89c]
    db 'Press ENTER to select.', 0x00
    db 0x00 ; g data +0xd8b3

; name_tandy: NUL-terminated text; padding remains separate [load+0xd8b4]
    db 'Tandy Graphics', 0x00
    db 0x00 ; g data +0xd8c3

; name_ega: NUL-terminated text; padding remains separate [load+0xd8c4]
    db 'Enhanced Graphics Adaptor', 0x00

; name_vga: NUL-terminated text; padding remains separate [load+0xd8de]
    db 'Video Graphics Array', 0x00
    db 0x00 ; g data +0xd8f3

; name_cga: NUL-terminated text; padding remains separate [load+0xd8f4]
    db 'Colour Graphics Adaptor', 0x00

; menu_normal_background: background used for ordinary graphics-menu options; word-sized argument to text_set_attribute [load+0xd90c]
    dw RGBI4_BLUE

; menu_selected_background: background used for the highlighted graphics-menu option; word-sized argument to text_set_attribute [load+0xd90e]
    dw RGBI4_RED

; menu_normal_foreground: foreground used for ordinary graphics-menu options; word-sized argument to text_set_attribute [load+0xd910]
    dw RGBI4_CYAN

; menu_selected_foreground: foreground used for the highlighted graphics-menu option; word-sized argument to text_set_attribute [load+0xd912]
    dw RGBI4_LIGHT_GRAY

; video_display_enable_wait_counts: Four mode-indexed counts of status-port bit-0 high-to-low cycles, not BIOS ticks or whole frames. Default values are 16,16,16,32. The menu selection path at CS:5C13 replaces all four with 32,32,20,48 for its alternate adapter selection. Transition code separately waits for bit-3 vertical-retrace boundaries before committing the page origin. [load+0xd914]
    dw 16 ; transition mode 0: status cycles
    dw 16 ; transition mode 1: status cycles
    dw 16 ; transition mode 2: status cycles
    dw 32 ; transition mode 3: status cycles

; adapter_name_pointers: nine DS-relative string pointers: normal adapter names occupy entries 0..5; the high-bit adapter-name lookup uses entries 6..8 through the +12-byte alias [load+0xd91c]
    dw crt_none ; table entry 0
    dw name_mda ; table entry 1
    dw adapter_label_cga ; table entry 2
    dw adapter_label_ega ; table entry 3
    dw name_mcga ; table entry 4
    dw adapter_label_vga ; table entry 5
    dw name_hercules ; table entry 6, special adapter 80h
    dw name_hercules_plus ; table entry 7, special adapter 81h
    dw name_incolor ; table entry 8, special adapter 82h

; video_page_offset: Committed display-start offset in video-memory bytes. CGA transitions at CS:5D34 mask it to 1FFFh and divide by two before programming CRTC start registers; EGA transitions at CS:5F12 use the word byte offset directly. The same numeric value therefore has different display-start register units across backends. Work-buffer selection does not reset it. [load+0xd92e]
    dw 0x0000

; video_previous_page_offset: page origin saved before a display transition and used as the copy source [load+0xd930]
    dw 0x0000

; video_pending_page_offset: new page origin computed from the current origin and a per-mode delta [load+0xd932]
    dw 0x0000
    db 0x00, 0x00, 0x11, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00 ; g data +0xd934

; video_transition_wait_count: scratch copy of the selected status polling count; initial value 7 is overwritten before polling [load+0xd93e]
    dw 0x0007

; cga_transition_source_mode_0: CGA transition source byte offsets for mode 0; indexed by twice DS:14D3 (the remaining-step counter). Formula: 320+2*(15-1*slot). [load+0xd940]
    dw 0x140 + 2*15 ; stored step 0
    dw 0x140 + 2*14 ; stored step 1
    dw 0x140 + 2*13 ; stored step 2
    dw 0x140 + 2*12 ; stored step 3
    dw 0x140 + 2*11 ; stored step 4
    dw 0x140 + 2*10 ; stored step 5
    dw 0x140 + 2*9 ; stored step 6
    dw 0x140 + 2*8 ; stored step 7
    dw 0x140 + 2*7 ; stored step 8
    dw 0x140 + 2*6 ; stored step 9
    dw 0x140 + 2*5 ; stored step 10
    dw 0x140 + 2*4 ; stored step 11
    dw 0x140 + 2*3 ; stored step 12
    dw 0x140 + 2*2 ; stored step 13
    dw 0x140 + 2*1 ; stored step 14
    dw 0x140 + 2*0 ; stored step 15

; cga_transition_source_mode_1: CGA transition source byte offsets for mode 1; indexed by twice DS:14D3 (the remaining-step counter). Formula: 320+2*(0+1*slot). [load+0xd960]
    dw 0x140 + 2*0 ; stored step 0
    dw 0x140 + 2*1 ; stored step 1
    dw 0x140 + 2*2 ; stored step 2
    dw 0x140 + 2*3 ; stored step 3
    dw 0x140 + 2*4 ; stored step 4
    dw 0x140 + 2*5 ; stored step 5
    dw 0x140 + 2*6 ; stored step 6
    dw 0x140 + 2*7 ; stored step 7
    dw 0x140 + 2*8 ; stored step 8
    dw 0x140 + 2*9 ; stored step 9
    dw 0x140 + 2*10 ; stored step 10
    dw 0x140 + 2*11 ; stored step 11
    dw 0x140 + 2*12 ; stored step 12
    dw 0x140 + 2*13 ; stored step 13
    dw 0x140 + 2*14 ; stored step 14
    dw 0x140 + 2*15 ; stored step 15

; cga_transition_source_mode_2: CGA transition source byte offsets for mode 2; indexed by twice DS:14D3 (the remaining-step counter). Formula: 0+320*(1+1*slot). [load+0xd980]
    dw 320*1 ; stored step 0
    dw 320*2 ; stored step 1
    dw 320*3 ; stored step 2
    dw 320*4 ; stored step 3
    dw 320*5 ; stored step 4
    dw 320*6 ; stored step 5
    dw 320*7 ; stored step 6
    dw 320*8 ; stored step 7

; cga_transition_source_mode_3: CGA transition source byte offsets for mode 3; indexed by twice DS:14D3 (the remaining-step counter). Formula: 0+320*(8-1*slot). [load+0xd990]
    dw 320*8 ; stored step 0
    dw 320*7 ; stored step 1
    dw 320*6 ; stored step 2
    dw 320*5 ; stored step 3
    dw 320*4 ; stored step 4
    dw 320*3 ; stored step 5
    dw 320*2 ; stored step 6
    dw 320*1 ; stored step 7

; cga_transition_page_deltas: CGA signed byte-offset changes for transition modes 0..3; added to video_page_offset to form video_pending_page_offset. Horizontal step 2 byte(s), vertical step 320 bytes. [load+0xd9a0]
    dw 2 ; transition mode 0
    dw -2 ; transition mode 1
    dw -320 ; transition mode 2
    dw 320 ; transition mode 3

; ega_transition_source_mode_0: EGA transition source byte offsets for mode 0; indexed by twice DS:14D3 (the remaining-step counter). Formula: 320+1*(15-1*slot). [load+0xd9a8]
    dw 0x140 + 1*15 ; stored step 0
    dw 0x140 + 1*14 ; stored step 1
    dw 0x140 + 1*13 ; stored step 2
    dw 0x140 + 1*12 ; stored step 3
    dw 0x140 + 1*11 ; stored step 4
    dw 0x140 + 1*10 ; stored step 5
    dw 0x140 + 1*9 ; stored step 6
    dw 0x140 + 1*8 ; stored step 7
    dw 0x140 + 1*7 ; stored step 8
    dw 0x140 + 1*6 ; stored step 9
    dw 0x140 + 1*5 ; stored step 10
    dw 0x140 + 1*4 ; stored step 11
    dw 0x140 + 1*3 ; stored step 12
    dw 0x140 + 1*2 ; stored step 13
    dw 0x140 + 1*1 ; stored step 14
    dw 0x140 + 1*0 ; stored step 15

; ega_transition_source_mode_1: EGA transition source byte offsets for mode 1; indexed by twice DS:14D3 (the remaining-step counter). Formula: 320+1*(0+1*slot). [load+0xd9c8]
    dw 0x140 + 1*0 ; stored step 0
    dw 0x140 + 1*1 ; stored step 1
    dw 0x140 + 1*2 ; stored step 2
    dw 0x140 + 1*3 ; stored step 3
    dw 0x140 + 1*4 ; stored step 4
    dw 0x140 + 1*5 ; stored step 5
    dw 0x140 + 1*6 ; stored step 6
    dw 0x140 + 1*7 ; stored step 7
    dw 0x140 + 1*8 ; stored step 8
    dw 0x140 + 1*9 ; stored step 9
    dw 0x140 + 1*10 ; stored step 10
    dw 0x140 + 1*11 ; stored step 11
    dw 0x140 + 1*12 ; stored step 12
    dw 0x140 + 1*13 ; stored step 13
    dw 0x140 + 1*14 ; stored step 14
    dw 0x140 + 1*15 ; stored step 15

; ega_transition_source_mode_2: EGA transition source byte offsets for mode 2; indexed by twice DS:14D3 (the remaining-step counter). Formula: 0+320*(1+1*slot). [load+0xd9e8]
    dw 320*1 ; stored step 0
    dw 320*2 ; stored step 1
    dw 320*3 ; stored step 2
    dw 320*4 ; stored step 3
    dw 320*5 ; stored step 4
    dw 320*6 ; stored step 5
    dw 320*7 ; stored step 6
    dw 320*8 ; stored step 7

; ega_transition_source_mode_3: EGA transition source byte offsets for mode 3; indexed by twice DS:14D3 (the remaining-step counter). Formula: 0+320*(8-1*slot). [load+0xd9f8]
    dw 320*8 ; stored step 0
    dw 320*7 ; stored step 1
    dw 320*6 ; stored step 2
    dw 320*5 ; stored step 3
    dw 320*4 ; stored step 4
    dw 320*3 ; stored step 5
    dw 320*2 ; stored step 6
    dw 320*1 ; stored step 7

; ega_transition_page_deltas: EGA signed byte-offset changes for transition modes 0..3; added to video_page_offset to form video_pending_page_offset. Horizontal step 1 byte(s), vertical step 320 bytes. [load+0xda08]
    dw 1 ; transition mode 0
    dw -1 ; transition mode 1
    dw -320 ; transition mode 2
    dw 320 ; transition mode 3

; vga_transition_source_mode_0: VGA transition source byte offsets for mode 0; indexed by twice DS:14D3 (the remaining-step counter). Formula: 2560+8*(15-1*slot). [load+0xda10]
    dw 0xa00 + 8*15 ; stored step 0
    dw 0xa00 + 8*14 ; stored step 1
    dw 0xa00 + 8*13 ; stored step 2
    dw 0xa00 + 8*12 ; stored step 3
    dw 0xa00 + 8*11 ; stored step 4
    dw 0xa00 + 8*10 ; stored step 5
    dw 0xa00 + 8*9 ; stored step 6
    dw 0xa00 + 8*8 ; stored step 7
    dw 0xa00 + 8*7 ; stored step 8
    dw 0xa00 + 8*6 ; stored step 9
    dw 0xa00 + 8*5 ; stored step 10
    dw 0xa00 + 8*4 ; stored step 11
    dw 0xa00 + 8*3 ; stored step 12
    dw 0xa00 + 8*2 ; stored step 13
    dw 0xa00 + 8*1 ; stored step 14
    dw 0xa00 + 8*0 ; stored step 15

; vga_transition_source_mode_1: VGA transition source byte offsets for mode 1; indexed by twice DS:14D3 (the remaining-step counter). Formula: 2560+8*(0+1*slot). [load+0xda30]
    dw 0xa00 + 8*0 ; stored step 0
    dw 0xa00 + 8*1 ; stored step 1
    dw 0xa00 + 8*2 ; stored step 2
    dw 0xa00 + 8*3 ; stored step 3
    dw 0xa00 + 8*4 ; stored step 4
    dw 0xa00 + 8*5 ; stored step 5
    dw 0xa00 + 8*6 ; stored step 6
    dw 0xa00 + 8*7 ; stored step 7
    dw 0xa00 + 8*8 ; stored step 8
    dw 0xa00 + 8*9 ; stored step 9
    dw 0xa00 + 8*10 ; stored step 10
    dw 0xa00 + 8*11 ; stored step 11
    dw 0xa00 + 8*12 ; stored step 12
    dw 0xa00 + 8*13 ; stored step 13
    dw 0xa00 + 8*14 ; stored step 14
    dw 0xa00 + 8*15 ; stored step 15

; vga_transition_source_mode_2: VGA transition source byte offsets for mode 2; indexed by twice DS:14D3 (the remaining-step counter). Formula: 0+2560*(1+1*slot). [load+0xda50]
    dw 2560*1 ; stored step 0
    dw 2560*2 ; stored step 1
    dw 2560*3 ; stored step 2
    dw 2560*4 ; stored step 3
    dw 2560*5 ; stored step 4
    dw 2560*6 ; stored step 5
    dw 2560*7 ; stored step 6
    dw 2560*8 ; stored step 7

; vga_transition_source_mode_3: VGA transition source byte offsets for mode 3; indexed by twice DS:14D3 (the remaining-step counter). Formula: 0+2560*(8-1*slot). [load+0xda60]
    dw 2560*8 ; stored step 0
    dw 2560*7 ; stored step 1
    dw 2560*6 ; stored step 2
    dw 2560*5 ; stored step 3
    dw 2560*4 ; stored step 4
    dw 2560*3 ; stored step 5
    dw 2560*2 ; stored step 6
    dw 2560*1 ; stored step 7

; vga_transition_page_deltas: VGA signed byte-offset changes for transition modes 0..3; added to video_page_offset to form video_pending_page_offset. Horizontal step 8 byte(s), vertical step 2560 bytes. [load+0xda70]
    dw 8 ; transition mode 0
    dw -8 ; transition mode 1
    dw -2560 ; transition mode 2
    dw 2560 ; transition mode 3

; warning_bar_noise_low: Independent warning-pattern sequence, seed 1: state=5*state modulo 2^32. Odd state preserves its residue modulo 4 and repeats after 2^30 updates. Each backend extracts successive two-bit variants from the high word to choose warning-pattern tiles; the state is advanced per tile group, not once per game frame. No landscape PRNG state is consumed. [load+0xda78]
    dw 0x0001, 0x0000 ; low word, high word

; cga_sentry_warning_patterns: CGA top-right sentry warning-bar patterns: three groups of four variants for clear, animated pixel-pair and solid fills. warning_bar_sight selects half or full height. [load+0xda7c]
    db 0, 0 ; stage 0, variant 0
    db 0, 0 ; stage 0, variant 1
    db 0, 0 ; stage 0, variant 2
    db 0, 0 ; stage 0, variant 3
    db 0, CGA_WARNING_COLOR*0x50 ; stage 1, variant 0
    db 0, CGA_WARNING_COLOR*0x05 ; stage 1, variant 1
    db CGA_WARNING_COLOR*0x50, 0 ; stage 1, variant 2
    db CGA_WARNING_COLOR*0x05, 0 ; stage 1, variant 3
    db CGA_WARNING_COLOR*0x55, CGA_WARNING_COLOR*0x55 ; stage 2, variant 0
    db CGA_WARNING_COLOR*0x55, CGA_WARNING_COLOR*0x55 ; stage 2, variant 1
    db CGA_WARNING_COLOR*0x55, CGA_WARNING_COLOR*0x55 ; stage 2, variant 2
    db CGA_WARNING_COLOR*0x55, CGA_WARNING_COLOR*0x55 ; stage 2, variant 3

; tandy_sentry_warning_patterns: Tandy top-right sentry warning-bar patterns: three groups of four variants for clear, animated pixel-pair and solid fills. warning_bar_sight selects half or full height. [load+0xda94]
    db 0, 0, 0, 0 ; stage 0, variant 0
    db 0, 0, 0, 0 ; stage 0, variant 1
    db 0, 0, 0, 0 ; stage 0, variant 2
    db 0, 0, 0, 0 ; stage 0, variant 3
    db TANDY_WARNING_COLOR*0x11, 0, 0, 0 ; stage 1, variant 0
    db 0, TANDY_WARNING_COLOR*0x11, 0, 0 ; stage 1, variant 1
    db 0, 0, TANDY_WARNING_COLOR*0x11, 0 ; stage 1, variant 2
    db 0, 0, 0, TANDY_WARNING_COLOR*0x11 ; stage 1, variant 3
    db TANDY_WARNING_COLOR*0x11, TANDY_WARNING_COLOR*0x11, TANDY_WARNING_COLOR*0x11, TANDY_WARNING_COLOR*0x11 ; stage 2, variant 0
    db TANDY_WARNING_COLOR*0x11, TANDY_WARNING_COLOR*0x11, TANDY_WARNING_COLOR*0x11, TANDY_WARNING_COLOR*0x11 ; stage 2, variant 1
    db TANDY_WARNING_COLOR*0x11, TANDY_WARNING_COLOR*0x11, TANDY_WARNING_COLOR*0x11, TANDY_WARNING_COLOR*0x11 ; stage 2, variant 2
    db TANDY_WARNING_COLOR*0x11, TANDY_WARNING_COLOR*0x11, TANDY_WARNING_COLOR*0x11, TANDY_WARNING_COLOR*0x11 ; stage 2, variant 3

; ega_sentry_warning_patterns: EGA top-right sentry warning-bar patterns: three groups of four variants for clear, animated pixel-pair and solid fills. warning_bar_sight selects half or full height. [load+0xdac4]
    db 0xff ; stage 0, variant 0
    db 0xff ; stage 0, variant 1
    db 0xff ; stage 0, variant 2
    db 0xff ; stage 0, variant 3
    db 0xff ^ (0x03 << 6) ; stage 1, variant 0
    db 0xff ^ (0x03 << 4) ; stage 1, variant 1
    db 0xff ^ (0x03 << 2) ; stage 1, variant 2
    db 0xff ^ (0x03 << 0) ; stage 1, variant 3
    db 0 ; stage 2, variant 0
    db 0 ; stage 2, variant 1
    db 0 ; stage 2, variant 2
    db 0 ; stage 2, variant 3

; vga_sentry_warning_patterns: VGA top-right sentry warning-bar patterns: three groups of four variants for clear, animated pixel-pair and solid fills. warning_bar_sight selects half or full height. [load+0xdad0]
    db 0, 0, 0, 0, 0, 0, 0, 0 ; stage 0, variant 0
    db 0, 0, 0, 0, 0, 0, 0, 0 ; stage 0, variant 1
    db 0, 0, 0, 0, 0, 0, 0, 0 ; stage 0, variant 2
    db 0, 0, 0, 0, 0, 0, 0, 0 ; stage 0, variant 3
    db VGA_WARNING_COLOR, VGA_WARNING_COLOR, 0, 0, 0, 0, 0, 0 ; stage 1, variant 0
    db 0, 0, VGA_WARNING_COLOR, VGA_WARNING_COLOR, 0, 0, 0, 0 ; stage 1, variant 1
    db 0, 0, 0, 0, VGA_WARNING_COLOR, VGA_WARNING_COLOR, 0, 0 ; stage 1, variant 2
    db 0, 0, 0, 0, 0, 0, VGA_WARNING_COLOR, VGA_WARNING_COLOR ; stage 1, variant 3
    db VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR ; stage 2, variant 0
    db VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR ; stage 2, variant 1
    db VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR ; stage 2, variant 2
    db VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR, VGA_WARNING_COLOR ; stage 2, variant 3

; copy_sequence_low: Randomized source-to-screen masked-copy sequence: little-endian 32-bit state multiplied by five modulo 2^32; the high word supplies position and variant bits. Independent from the terrain pixel-scatter sequence. [load+0xdb30]
    dw 0x4261, 0x3987 ; low word, high word

; cga_random_copy_nibble_masks: CGA random copy at 70B9h chooses one nibble from a word, merges those two pixels from the work buffer, and preserves the destination bits outside the mask [load+0xdb34]
    dw 0x000f << 12 ; random variant 0
    dw 0x000f << 8 ; random variant 1
    dw 0x000f << 4 ; random variant 2
    dw 0x000f << 0 ; random variant 3

; ega_random_copy_bit_masks: EGA random copy at 71DEh selects the first pixel of a pair at bit 7, 5, 3 or 1. The mask selects a source bit in each plane and programs graphics-controller register 8; a second pass uses mask >> 1 for the adjacent pixel. [load+0xdb3c]
    db 1 << 7 ; variant 0: first bit 7, then bit 6
    db 1 << 5 ; variant 1: first bit 5, then bit 4
    db 1 << 3 ; variant 2: first bit 3, then bit 2
    db 1 << 1 ; variant 3: first bit 1, then bit 0

; crt_near_limit: highest currently usable near-heap offset, updated after DOS block growth [load+0xdb40]
    dw 0x0000

; crt_initial_break: near-heap break established by startup before environment/argument allocations [load+0xdb42]
    dw 0x0000

; crt_exit_callback: near code pointer used by crt_fatal_error; initially the shared cleanup exit entry, replaced by crt_exit after INT 00h failure [load+0xdb44]
    dw crt_exit_shared_cleanup-program_image

; crt_near_break: current near-heap break; crt_reserve_near_bytes returns the old value and advances it [load+0xdb46]
    dw 0x0000

; crt_loader_data_segment_reference: The 078Bh initializer is a loader-relocated program-data-segment reference, confirmed equal to DS after loading a diagnostic MZ copy. Why CRT stores this segment reference here remains unconfirmed. [load+0xdb48]
    dw PROGRAM_DATA_PARAGRAPH ; MZ loader relocates this word to the loaded DS segment
    times 76 db 0x00 ; g data +0xdb4a

; crt_near_break_address: Initialized near data pointer to crt_near_break (DS:6296), rather than an executable code pointer or a heap size. Its value needs no MZ segment relocation. The target word is used by crt_reserve_near_bytes at CS:7866; a reader of this pointer itself remains unconfirmed. [load+0xdb96]
    dw crt_near_break

; crt_file_info_name: NUL-terminated text; padding remains separate [load+0xdb98]
    db ';C_FILE_INFO', 0x00

; crt_saved_int00_offset: saved divide-error vector: offset then segment; crt_initialize_io_and_hooks captures it and crt_restore_runtime_hooks restores it through DOS [load+0xdba5]
    dw 0x0000 ; offset
    dw 0x0000 ; segment
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ; g data +0xdba9

; crt_psp_segment: DOS Program Segment Prefix segment saved during startup; used for command tail, environment lookup, and DOS memory resizing [load+0xdbb7]
    dw 0x0000

; crt_dos_version: DOS AH=30h result: low byte major version, high byte minor version; argument setup distinguishes DOS 2 from later environment formats [load+0xdbb9]
    dw 0x0000
    db 0x00, 0x00, 0x00 ; g data +0xdbbb

; crt_handle_flag_capacity: configured capacity of the following 20-byte per-DOS-handle flag table [load+0xdbbe]
    dw 0x0014

; crt_file_flags: 20 per-handle flags; bit 0 marks an open handle, bit 6 is refreshed from DOS IOCTL device status. Inherited C_FILE_INFO flags can replace the initial values; exit closes open handles 5..19. [load+0xdbc0]
    db CRT_HANDLE_OPEN | 0x80 ; DOS handle 0: stdin
    db CRT_HANDLE_OPEN | 0x80 ; DOS handle 1: stdout
    db CRT_HANDLE_OPEN | 0x80 ; DOS handle 2: stderr
    db CRT_HANDLE_OPEN ; DOS handle 3: auxiliary
    db CRT_HANDLE_OPEN ; DOS handle 4: printer
    db 0 ; DOS handle 5
    db 0 ; DOS handle 6
    db 0 ; DOS handle 7
    db 0 ; DOS handle 8
    db 0 ; DOS handle 9
    db 0 ; DOS handle 10
    db 0 ; DOS handle 11
    db 0 ; DOS handle 12
    db 0 ; DOS handle 13
    db 0 ; DOS handle 14
    db 0 ; DOS handle 15
    db 0 ; DOS handle 16
    db 0 ; DOS handle 17
    db 0 ; DOS handle 18
    db 0 ; DOS handle 19

; crt_argc: C argument count including argv[0], initialized to one before parsing the PSP command tail at CS:761E. Space/tab separate arguments outside quotes; quoted spaces and an empty quoted string form arguments. Runs of backslashes before a quote follow parity: pairs yield literal backslashes, an odd remainder yields a literal quote. Native synthetic-PSP examples verified by check_crt_arguments.py. [load+0xdbd4]
    dw 0x0000

; crt_argv: Near pointer to a zero-word-terminated vector of near string pointers. CS:761E allocates the vector and copied argument strings together below SP, using SS=DS; argv[0] is copied first from crt_program_name_pointer (DOS 2 fallback C). This storage is stack-resident for game lifetime, unlike envp allocated at the near break. argc gives the vector length before its null entry. [load+0xdbd6]
    dw 0x0000

; crt_envp: Near pointer to a zero-word-terminated environment vector, allocated with copied strings by crt_reserve_near_bytes at CS:77AC. Any DOS environment string beginning with bytes ;C is omitted from the pointer vector, including inherited ;C_FILE_INFO handle metadata. Its bytes are still copied into the allocation. Ordinary strings retain DOS environment order; native probe verifies filtering separately from argv parsing. [load+0xdbd8]
    dw 0x0000

; crt_program_name_pointer: far source pointer for argv[0]; default points to the C fallback string, replaced by the executable name from the DOS environment when available [load+0xdbda]
    dw crt_program_name_fallback, PROGRAM_DATA_PARAGRAPH ; far pointer, segment word is DOS-relocated

; crt_program_name_fallback: default argv[0] string used when the environment executable-name path is unavailable, including the DOS 2 path [load+0xdbde]
    db 'C', 0
    db 0x00, 0x00 ; g data +0xdbe0

; crt_optional_interrupt_enabled: Byte-valued enable for the optional interrupt-vector restore. Cleanup tests nonzero, so value 1 enables it despite having no sign bit; zero disables it. The operation restores the pointer/interrupt number in the neighboring fields and leaves this byte set, rather than consuming a one-shot request. Its shipped zero initializer makes this additional restoration dormant. [load+0xdbe2]
    db 0x00

; crt_optional_interrupt_number: Interrupt number passed in AL to DOS AH=25h during optional-hook cleanup [load+0xdbe3]
    db 0x00

; crt_optional_interrupt_vector: Saved far vector for an optional DOS interrupt restoration. Cleanup reads it with LDS DX and uses the separate byte at DS:6333 as AL for DOS AH=25h; it never calls through this pointer. A synthetic INT 60h hook is restored to this exact offset/segment by the native probe. This is separate from the divide-error vector saved at DS:62F5. [load+0xdbe4]
    dw 0x0000, 0x0000 ; offset, segment

; crt_error_callback: optional near callback invoked while printing a fatal runtime error; zero disables it [load+0xdbe8]
    dw 0x0000

; crt_argv_saved_return: return address temporarily popped by crt_prepare_argv while rearranging the stack; restored by an indirect near jump [load+0xdbea]
    dw 0x0000
    db 0x00, 0x00 ; g data +0xdbec

; crt_optional_service_hook: Optional far service callback, dormant in the shipped image because its segment word is zero. CS:7466 tests the segment alone, then calls BX=0 with AX=offset and DX=segment from the first far argument and SI=PSP environment segment; carry clear permits the BX=3 call with the second argument. CS:7586 calls BX=2 during cleanup. DS is temporarily loaded from each argument and restored from SS. The native diagnostic enables a callback and verifies this ABI; the original service provider remains unconfirmed. [load+0xdbee]
    dw 0x0000 ; offset
    dw 0x0000 ; segment

; crt_service_init_argument: First optional-service far argument, initialized 0000:0000. LDS AX loads its offset into AX and its segment into DS; DX then receives that segment for the BX=0 initialization call. Thus the service receives the same segment both in DS and DX, not a single 32-bit integer in DX:AX that the CRT dereferences itself. The original callback is disabled unless its segment is configured. [load+0xdbf2]
    dw 0x0000 ; offset
    dw 0x0000 ; segment

; crt_service_secondary_argument: Second optional-service far argument, initialized 0000:0000. Reached only after the BX=0 service call returns carry clear; LDS AX reads this record through SS even though DS may still name the first argument segment. BX=3 receives AX=offset and DX=DS=segment. Its return carry is ignored, and the CRT restores DS to SS afterward. [load+0xdbf6]
    dw 0x0000 ; offset
    dw 0x0000 ; segment

; crt_none: NUL-terminated text; padding remains separate [load+0xdbfa]
    db '(none)', 0x00
    db 0x00 ; g data +0xdc01

; name_mda: NUL-terminated text; padding remains separate [load+0xdc02]
    db 'Mono Display Adaptor', 0x00
    db 0x00 ; g data +0xdc17

; adapter_label_cga: NUL-terminated text; padding remains separate [load+0xdc18]
    db 'CGA', 0x00

; adapter_label_ega: NUL-terminated text; padding remains separate [load+0xdc1c]
    db 'EGA', 0x00

; name_mcga: NUL-terminated text; padding remains separate [load+0xdc20]
    db 'MCGA', 0x00
    db 0x00 ; g data +0xdc25

; adapter_label_vga: NUL-terminated text; padding remains separate [load+0xdc26]
    db 'VGA', 0x00

; name_hercules: NUL-terminated text; padding remains separate [load+0xdc2a]
    db 'Hercules Graphics Card', 0x00
    db 0x00 ; g data +0xdc41

; name_hercules_plus: NUL-terminated text; padding remains separate [load+0xdc42]
    db 'Hercules Graphics Card+', 0x00

; name_incolor: NUL-terminated text; padding remains separate [load+0xdc5a]
    db 'InColor Card', 0x00
    db 0x00 ; g data +0xdc67

; msg_cannot_find: NUL-terminated text; padding remains separate [load+0xdc68]
    db 'Cannot find ', 0x00
    db 0x00 ; g data +0xdc75

; msg_no_memory_1: NUL-terminated text; padding remains separate [load+0xdc76]
    db 'Not enough RAM to run this program.', 0x00

; msg_no_memory_2: NUL-terminated text; padding remains separate [load+0xdc9a]
    db 'Not enough RAM to run this program.', 0x00

; msg_no_memory_3: NUL-terminated text; padding remains separate [load+0xdcbe]
    db 'Not enough RAM to run this program.', 0x00

; msg_no_memory_4: NUL-terminated text; padding remains separate [load+0xdce2]
    db 'Not enough RAM to run this program.', 0x00

; msg_no_memory_5: NUL-terminated text; padding remains separate [load+0xdd06]
    db 'Not enough RAM to run this program.', 0x00

; msg_unsupported_adapter: NUL-terminated text; padding remains separate [load+0xdd2a]
    db 'This program will not run with a ', 0x00

; filename_skybox: NUL-terminated text; padding remains separate [load+0xdd4c]
    db 'SKYBOX.BIN', 0x00
    db 0x00 ; g data +0xdd57

; filename_tfinal: NUL-terminated text; padding remains separate [load+0xdd58]
    db 'TFINAL.BIN', 0x00

; crt_missing_message_marker: eight marker characters; the following error-code-zero low byte also supplies its NUL terminator [load+0xdd63]
    db '<<NMSG>>' ; terminating zero aliases the first error tag

; crt_error_message_table: error tag followed immediately by its NUL-terminated message; consumed by crt_lookup_error_message [load+0xdd6b]
    dw CRT_ERROR_STACK

; crt_msg_stack_overflow: NUL-terminated text; padding remains separate [load+0xdd6d]
    db 'R6000', 0x0d, 0x0a, '- stack overflow', 0x0d, 0x0a, 0x00

; crt_error_tag_3: error tag followed immediately by its NUL-terminated message; consumed by crt_lookup_error_message [load+0xdd87]
    dw CRT_ERROR_INTEGER_DIVIDE

; crt_msg_divide_by_zero: NUL-terminated text; padding remains separate [load+0xdd89]
    db 'R6003', 0x0d, 0x0a, '- integer divide by 0', 0x0d, 0x0a, 0x00

; crt_error_tag_9: error tag followed immediately by its NUL-terminated message; consumed by crt_lookup_error_message [load+0xdda8]
    dw CRT_ERROR_ENVIRONMENT

; crt_msg_environment: NUL-terminated text; padding remains separate [load+0xddaa]
    db 'R6009', 0x0d, 0x0a, '- not enough space for environment', 0x0d, 0x0a, 0x00

; crt_error_tag_252: error tag followed immediately by its NUL-terminated message; consumed by crt_lookup_error_message [load+0xddd6]
    dw CRT_ERROR_NEWLINE

; crt_msg_newline: NUL-terminated text; padding remains separate [load+0xddd8]
    db 0x0d, 0x0a, 0x00

; crt_error_tag_255: error tag followed immediately by its NUL-terminated message; consumed by crt_lookup_error_message [load+0xdddb]
    dw CRT_ERROR_GENERIC

; crt_msg_runtime_error: NUL-terminated text; padding remains separate [load+0xdddd]
    db 'run-time error ', 0x00

; crt_error_tag_2: error tag followed immediately by its NUL-terminated message; consumed by crt_lookup_error_message [load+0xdded]
    dw CRT_ERROR_FLOATING_POINT

; crt_msg_floating_point: NUL-terminated text; padding remains separate [load+0xddef]
    db 'R6002', 0x0d, 0x0a, '- floating point not loaded', 0x0d, 0x0a, 0x00

; crt_error_tag_1: error tag followed immediately by its NUL-terminated message; consumed by crt_lookup_error_message [load+0xde14]
    dw CRT_ERROR_NULL_ASSIGNMENT

; crt_msg_null_assignment: NUL-terminated text; padding remains separate [load+0xde16]
    db 'R6001', 0x0d, 0x0a, '- null pointer assignment', 0x0d, 0x0a, 0x00

; crt_error_tag_65535: end of variable-length error registry [load+0xde39]
    dw CRT_ERROR_TABLE_END
    db 0xff, 0x00, 0x00, 0x00, 0x00 ; g data +0xde3b

file_end:

%endif
