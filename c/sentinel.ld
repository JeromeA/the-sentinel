/* Reconstructed C program with register interfaces and inline hardware boundaries. */
OUTPUT_FORMAT(elf32-i386)
SECTIONS
{
    .mz 0x100000 : AT(0x100000) { KEEP(*(.sentinel.mz)) }
    /* Leave six bytes for the diagnostic far-return trampoline. No production
       entry or function is assigned an original address. */
    .prefix 0 : AT(0) { BYTE(0) BYTE(0) BYTE(0) BYTE(0) BYTE(0) BYTE(0) }
    .text 6 : AT(6) {
        *(EXCLUDE_FILE (*software_cursor.o *ray_trace.o *polygon_edges.o *legacy_typed.o *text_interpreter.o *render_geometry.o *graphics_ui.o *palette.o *render_world.o *scene_flow.o *input_controller.o *player_actions.o *sentry_actions.o) .text*)
        *(EXCLUDE_FILE (*software_cursor.o *ray_trace.o *polygon_edges.o *legacy_typed.o *text_interpreter.o *render_geometry.o *graphics_ui.o *palette.o *render_world.o *scene_flow.o *input_controller.o *player_actions.o *sentry_actions.o) .fartext*)
    }
    __typed_code_linear = ALIGN(ADDR(.text) + SIZEOF(.text),16);
    __typed_code_paragraph = __typed_code_linear / 16;
    .typed_code 0 : AT(__typed_code_linear) { *software_cursor.o(.text* .fartext*) *ray_trace.o(.text* .fartext*) *polygon_edges.o(.text* .fartext*) *legacy_typed.o(.text* .fartext*) *text_interpreter.o(.text* .fartext*) *render_geometry.o(.text* .fartext*) *graphics_ui.o(.text* .fartext*) *palette.o(.text* .fartext*) *render_world.o(.text* .fartext*) *scene_flow.o(.text* .fartext*) *input_controller.o(.text* .fartext*) }
    __gameplay_code_linear = ALIGN(__typed_code_linear + SIZEOF(.typed_code), 16);
    __gameplay_code_paragraph = __gameplay_code_linear / 16;
    .gameplay_code 0 : AT(__gameplay_code_linear) {
        *player_actions.o(.text* .fartext*) *sentry_actions.o(.text* .fartext*)
    }
    __game_data_linear = ALIGN(__gameplay_code_linear + SIZEOF(.gameplay_code), 16);
    __game_data_paragraph = __game_data_linear / 16;
    /* Only the 66-byte null-assignment guard requires DS-zero placement. */
    __c_data_base = 0x50;
    __c_irq_stack_bottom = ALIGN(__c_data_base + SIZEOF(.c_data), 16);
    __c_irq_stack_top = __c_irq_stack_bottom + 0x800;
    __c_main_stack_top = __c_irq_stack_top + 0x1000;
    __initial_ss = __game_data_paragraph + (__c_main_stack_top - 0x1000) / 16;
    .guard 0 : AT(__game_data_linear) { KEEP(*(.crt.guard)) }
    /* Native C globals use DS-relative virtual addresses, while AT gives
       their actual load-image position after the code segments. */
    .c_data __c_data_base : AT(__game_data_linear + __c_data_base) {
        *(.rodata*) *(.data*) *(.bss*) BYTE(0)
    }
    __minimum_paragraphs = (__c_main_stack_top + 16 - (__c_data_base + SIZEOF(.c_data)) + 15) / 16;
    __file_size = 512 + __game_data_linear + __c_data_base + SIZEOF(.c_data);
    __last_page_bytes = __file_size % 512;
    __file_pages = (__file_size + 511) / 512;
    /DISCARD/ : { *(.comment*) *(.note*) *(.eh_frame*) *(.msdos_mz_reloc*) }
    ASSERT(SIZEOF(.mz) == 512, "MZ header size changed")
    ASSERT(SIZEOF(.guard) == 0x42, "null-assignment guard size changed")
    ASSERT(SIZEOF(.guard) <= __c_data_base, "C globals overlap null-assignment guard")
    ASSERT(__c_main_stack_top <= 0x10000, "C stacks exceed the data segment")
    ASSERT(ADDR(.text)+SIZEOF(.text) <= 0x10000, "main code segment limit reached")
    ASSERT(SIZEOF(.typed_code) <= 0x10000, "secondary code segment limit reached")
    ASSERT(SIZEOF(.gameplay_code) <= 0x10000, "gameplay code segment limit reached")
}
