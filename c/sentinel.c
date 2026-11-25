/* Sentinel C reconstruction: named data, DOS header and inline ABI boundaries.
 * All game and runtime algorithms are C. Inline assembly handles hardware,
 * interrupt/stack boundaries and the register adapters.
 * Build: make -C c. No NASM source, reference EXE or packer is used.
 * Code, callbacks and subsystem-owned data are resolved by the linker.
 * Only the DOS null-assignment guard requires a fixed DS address.
 */
#include <stdint.h>
#include <stddef.h>
#include "text_bytecode.h"
#include "legacy_targets.h"
#include "typed_entries.h"
#include "typed_api.h"
#include "owned_state.h"
extern char __game_data_paragraph, __initial_ss, __last_page_bytes, __file_pages,
    __minimum_paragraphs;

extern char reloc_keyboard, reloc_bios_keyboard, reloc_timer_triple, reloc_timer_original,
    reloc_mouse, reloc_startup, reloc_divide;

struct __attribute__((packed)) DosRelocation {
    uint16_t offset, segment;
};

struct __attribute__((packed)) DosHeader {
    uint16_t magic, last_page_bytes, pages, relocations, header_paragraphs;
    uint16_t minimum_paragraphs, maximum_paragraphs, ss, sp, checksum;
    uint16_t ip, cs, relocation_offset, overlay;
    struct DosRelocation fixup[117];
    uint8_t padding[8], signature[4];
    uint16_t data_paragraph, abi_version;
};
const struct DosHeader sentinel_dos_header
    __attribute__((used, section(".sentinel.mz"), aligned(1))) = {0x5a4d,
        (uint16_t)&__last_page_bytes,
        (uint16_t)&__file_pages,
        9,
        32,
        (uint16_t)&__minimum_paragraphs,
        0xffff,
        (uint16_t)&__initial_ss,
        0x1000,
        0,
        (uint16_t)crt_entry,
        0,
        28,
        0,
        {
            {(uint16_t)&reloc_keyboard, 0},
            {(uint16_t)&reloc_bios_keyboard, 0},
            {(uint16_t)&reloc_timer_triple, 0},
            {(uint16_t)&reloc_timer_original, 0},
            {(uint16_t)&reloc_mouse, 0},
            {(uint16_t)&reloc_startup, 0},
            {(uint16_t)&reloc_divide, 0},
            {(uint16_t)crt_loader_data_segment_reference, (uint16_t)&__game_data_paragraph},
            {(uint16_t)&crt_program_name_pointer[1], (uint16_t)&__game_data_paragraph},
        },
        {0},
        {'C', 'A', 'P', 'I'},
        (uint16_t)&__game_data_paragraph,
        4};
_Static_assert(sizeof(struct DosHeader) == 512, "DOS header size");

#include "data_aliases.h"

/* Hardware and stack boundaries use linker-resolved symbols. */
__asm__(".intel_syntax noprefix\n"
        ".code16\n"
        "\n"
        ".section .text.hardware_boundaries,\"ax\",@progbits\n"
        "# Each C entry here exists only for a native stack/callback boundary.\n"
        ".macro c_entry name, function\n"
        "\\name:\n"
        "    call c_enter_literal\n"
        "    .word target_\\function\n"
        ".endm\n"
        "saved_int15_vector: .word 0,0\n"
        "saved_int08_vector: .word 0,0\n"
        "# Generation enters preview preparation before population validates the code.\n"
        "landscape_preview_return_dispatch:\n"
        "    js abi_c_preview_caption\n"
        "    c_entry abi_c_configure_preview_object, c_configure_preview_object\n"
        "    c_entry abi_c_preview_caption, preview_caption_flow\n"
        "preview_population_call:\n"
        "    call abi_c_populate_preview\n"
        "# Accepted codes rewrite this CALL return; failures take the ordinary retry.\n"
        "    jmp abi_c_wrong_secret\n"
        "generation_call:\n"
        "    call abi_c_generate_landscape\n"
        "    c_entry abi_c_wrong_secret, wrong_secret_flow\n"
        "keyboard_irq_handler:\n"
        "    push ax\n"
        "    push bx\n"
        "    push ds\n"
        "    mov bx, OFFSET FLAT:PROGRAM_DATA_PARAGRAPH\n"
        "reloc_keyboard = .-2\n"
        "    mov ds,bx\n"
        "    c_entry abi_c_keyboard_irq_scan, c_keyboard_irq_scan\n"
        "keyboard_irq_return:\n"
        "    mov al,0x20\n"
        "    out 0x20,al\n"
        "    pop ds\n"
        "    pop bx\n"
        "    pop ax\n"
        "    iret\n"
        "bios_keyboard_intercept:\n"
        "    cmp ah,0x4f\n"
        "    je bios_keyboard_scan\n"
        "    jmp far ptr cs:[saved_int15_vector]\n"
        "bios_keyboard_scan:\n"
        "    push bp\n"
        "    mov bp,sp\n"
        "    push ax\n"
        "    push bx\n"
        "    push ds\n"
        "    mov bx, OFFSET FLAT:PROGRAM_DATA_PARAGRAPH\n"
        "reloc_bios_keyboard = .-2\n"
        "    mov ds,bx\n"
        "    c_entry abi_c_keyboard_bios_scan, c_keyboard_bios_scan\n"
        "bios_keyboard_return:\n"
        "    pop ds\n"
        "    pop bx\n"
        "    pop ax\n"
        "    clc\n"
        "    lahf\n"
        "    mov [bp+6],ah\n"
        "    pop bp\n"
        "    iret\n"
        "timer_irq_triple_rate:\n"
        "    push ax\n"
        "    push bx\n"
        "    push cx\n"
        "    push dx\n"
        "    push si\n"
        "    push di\n"
        "    push bp\n"
        "    push ds\n"
        "    push es\n"
        "    mov ax, OFFSET FLAT:PROGRAM_DATA_PARAGRAPH\n"
        "reloc_timer_triple = .-2\n"
        "    mov ds,ax\n"
        "    mov es,ax\n"
        "    c_entry abi_c_timer_triple_body, c_timer_triple_body\n"
        "timer_chain_return:\n"
        "    pop es\n"
        "    pop ds\n"
        "    pop bp\n"
        "    pop di\n"
        "    pop si\n"
        "    pop dx\n"
        "    pop cx\n"
        "    pop bx\n"
        "    pop ax\n"
        "    jmp far ptr cs:[saved_int08_vector]\n"
        "timer_eoi_return:\n"
        "    mov al,0x20\n"
        "    out 0x20,al\n"
        "    pop es\n"
        "    pop ds\n"
        "    pop bp\n"
        "    pop di\n"
        "    pop si\n"
        "    pop dx\n"
        "    pop cx\n"
        "    pop bx\n"
        "    pop ax\n"
        "    iret\n"
        "timer_irq_original_rate:\n"
        "    push ax\n"
        "    push bx\n"
        "    push cx\n"
        "    push dx\n"
        "    push si\n"
        "    push di\n"
        "    push bp\n"
        "    push ds\n"
        "    push es\n"
        "    mov ax, OFFSET FLAT:PROGRAM_DATA_PARAGRAPH\n"
        "reloc_timer_original = .-2\n"
        "    mov ds,ax\n"
        "    mov es,ax\n"
        "    c_entry abi_c_timer_original_body, c_timer_original_body\n"
        "timer_original_return:\n"
        "    pop es\n"
        "    pop ds\n"
        "    pop bp\n"
        "    pop di\n"
        "    pop si\n"
        "    pop dx\n"
        "    pop cx\n"
        "    pop bx\n"
        "    pop ax\n"
        "    jmp far ptr cs:[saved_int08_vector]\n"
        "hardware_enter_game:\n"
        "    push si\n"
        "    push di\n"
        "    push bp\n"
        "    c_entry abi_c_hardware_enter, c_hardware_enter\n"
        "hardware_stack_return:\n"
        "    mov sp,[saved_hardware_sp]\n"
        "    pop bp\n"
        "    pop di\n"
        "    pop si\n"
        "    ret\n"
        "    c_entry hardware_leave_game, c_hardware_leave\n"
        "mouse_event_callback:\n"
        "    push ax\n"
        "    push bx\n"
        "    push si\n"
        "    push ds\n"
        "    mov si, OFFSET FLAT:PROGRAM_DATA_PARAGRAPH\n"
        "reloc_mouse = .-2\n"
        "    mov ds,si\n"
        "    c_entry abi_c_mouse_callback_body, c_mouse_callback_body\n"
        "mouse_callback_return:\n"
        "    pop ds\n"
        "    pop si\n"
        "    pop bx\n"
        "    pop ax\n"
        "    retf\n"
        "crt_entry:\n"
        "    mov ah,0x30\n"
        "    int 0x21\n"
        "    cmp al,2\n"
        "    jae crt_version_ok\n"
        "    int 0x20\n"
        "crt_version_ok:\n"
        "    mov di, OFFSET FLAT:PROGRAM_DATA_PARAGRAPH\n"
        "reloc_startup = .-2\n"
        "    mov si,[2]\n"
        "    sub si,di\n"
        "    cmp si,0x1000\n"
        "    jb crt_memory_checked\n"
        "    mov si,0x1000\n"
        "crt_memory_checked:\n"
        "    cli\n"
        "    mov ss,di\n"
        "    add sp, OFFSET FLAT:__c_irq_stack_top\n"
        "    sti\n"
        "    jnc crt_memory_ok\n"
        "    push ss\n"
        "    pop ds\n"
        "    call abi_c_crt_error_prefix\n"
        "    xor ax,ax\n"
        "    push ax\n"
        "    call abi_c_crt_write_message\n"
        "    mov ax,0x4cff\n"
        "    int 0x21\n"
        "crt_memory_ok:\n"
        "    c_entry abi_c_crt_initialize, c_crt_initialize\n"
        "argv_call:\n"
        "    call abi_c_crt_prepare_argv\n"
        "    c_entry abi_c_crt_run_game, c_crt_run_game\n"
        "crt_divide_error:\n"
        "    mov ax, OFFSET FLAT:PROGRAM_DATA_PARAGRAPH\n"
        "reloc_divide = .-2\n"
        "    mov ds,ax\n"
        "    c_entry abi_c_crt_divide_error, c_crt_divide_error\n"
        "    c_entry abi_c_crt_exit, c_crt_exit\n"
        "    c_entry abi_c_crt_exit_common, c_crt_exit_common\n"
        "    c_entry abi_c_crt_error_prefix, c_crt_error_prefix\n"
        "    c_entry abi_c_crt_fatal, c_crt_fatal\n"
        "    c_entry abi_c_crt_write_message, c_crt_write_message\n"
        "    c_entry abi_c_crt_prepare_argv, c_crt_prepare_argv\n"
        "    c_entry abi_c_populate_preview, populate_preview_flow\n"
        "    c_entry abi_c_generate_landscape, generate_landscape_flow\n"
        "    c_entry abi_c_sentry_schedule_finish, c_sentry_schedule_finish\n"
        "    c_entry abi_c_preview_key_gate, preview_key_flow\n"
        ".globl reloc_bios_keyboard\n"
        ".globl mouse_callback_return\n"
        ".globl bios_keyboard_return\n"
        ".globl hardware_leave_game\n"
        ".globl hardware_enter_game\n"
        ".globl hardware_stack_return\n"
        ".globl timer_eoi_return\n"
        ".globl timer_original_return\n"
        ".globl reloc_timer_triple\n"
        ".globl reloc_keyboard\n"
        ".globl landscape_preview_return_dispatch\n"
        ".globl crt_divide_error\n"
        ".globl keyboard_irq_return\n"
        ".globl abi_c_crt_exit\n"
        ".globl saved_int08_vector\n"
        ".globl timer_chain_return\n"
        ".globl keyboard_irq_handler\n"
        ".globl argv_call\n"
        ".globl abi_c_preview_key_gate\n"
        ".globl reloc_mouse\n"
        ".globl preview_population_call\n"
        ".globl reloc_timer_original\n"
        ".globl reloc_divide\n"
        ".globl timer_irq_triple_rate\n"
        ".globl generation_call\n"
        ".globl crt_argument_handoff\n"
        ".globl crt_entry\n"
        ".globl saved_int15_vector\n"
        ".globl timer_irq_original_rate\n"
        ".globl abi_c_sentry_schedule_finish\n"
        ".globl abi_c_crt_exit_common\n"
        ".globl mouse_event_callback\n"
        ".globl legacy_resume\n"
        ".globl bios_keyboard_intercept\n"
        ".globl reloc_startup\n"
        ".att_syntax prefix\n"
        ".text\n");

#include "runtime_support.c"
#include "game_logic.c"
#include "video_copy.c"
#include "legacy_abi.c"
#include "input_state.h"
#include "render_state.h"
#include "resource_decode.c"
#include "video_core.c"
#include "font_render.c"
#include "platform_io.c"
#include "graphics_setup.c"
#include "image_draw.c"
#include "terrain_generation.c"
#include "pixel_scatter.c"
#include "sound.c"
#include "projection_math.c"
#include "input_state.c"
#include "scene_image.c"
#include "selection_view.c"
#include "mouse_view.c"
#include "terrain_text.c"
#include "digit_input.c"
#include "video_transition.c"
#include "terrain_view.c"
#include "hud.c"
#include "occlusion.c"
#define SOFTWARE_CURSOR_DATA_ONLY
#include "software_cursor.c"
#undef SOFTWARE_CURSOR_DATA_ONLY
#include "selection_effects.c"
#include "model_data.c"
#include "model_render.c"
#include "landscape_placement.c"
#include "polygon_spans.c"
#include "object_world.c"
#include "text_format.c"
#include "adapter_data.c"
#include "graphics_menu.c"
#include "selection_copy.c"
#include "ray_math.c"
/* SampleWork is reset independently of other subsystems and data placement. */
struct SampleWork sample_work = {0};

void __far reset_sample_work(SegmentBytes output)
{
    c_memory_fill(output + ((uint16_t)&sample_work), 0, sizeof(sample_work), 0);
}

#include "video_special.c"
#include "keyboard_wait.c"
#include "game_effects.c"
#include "adapter_detect.c"
#include "platform_events.c"
#include "bootstrap.c"
#include "polygon_setup.c"
#include "terrain_cell.c"
/* Player action algorithms are a separate far C translation unit. */
#include "view_transition.c"
#include "sentry_sight.c"
#define SENTRY_ACTIONS_DATA_ONLY
#include "sentry_actions.c"
#undef SENTRY_ACTIONS_DATA_ONLY

#include "tandy_spans.c"
#include "play_flow.c"
#include "crt_runtime.c"
#include "entry_aliases.c"
#include "video_callbacks.c"

const struct LegacyTarget target_c_configure_preview_object = {
    typed_legacy_entry, TYPED_c_configure_preview_object};
const struct LegacyTarget target_c_crt_divide_error = {
    typed_legacy_entry, TYPED_c_crt_divide_error};
const struct LegacyTarget target_c_crt_error_prefix = {c_crt_error_prefix, 0};
const struct LegacyTarget target_c_crt_exit = {c_crt_exit, 0};
const struct LegacyTarget target_c_crt_exit_common = {c_crt_exit_common, 0};
const struct LegacyTarget target_c_crt_fatal = {c_crt_fatal, 0};
const struct LegacyTarget target_c_crt_initialize = {c_crt_initialize, 0};
const struct LegacyTarget target_c_crt_prepare_argv = {
    typed_legacy_entry, TYPED_c_crt_prepare_argv};
const struct LegacyTarget target_c_crt_run_game = {c_crt_run_game, 0};
const struct LegacyTarget target_c_crt_write_message = {
    typed_legacy_entry, TYPED_c_crt_write_message};
const struct LegacyTarget target_generate_landscape_flow = {
    typed_legacy_entry, TYPED_generate_landscape_flow};
const struct LegacyTarget target_c_hardware_enter = {c_hardware_enter, 0};
const struct LegacyTarget target_c_hardware_leave = {c_hardware_leave, 0};
const struct LegacyTarget target_c_keyboard_bios_scan = {
    typed_legacy_entry, TYPED_c_keyboard_bios_scan};
const struct LegacyTarget target_c_keyboard_irq_scan = {
    typed_legacy_entry, TYPED_c_keyboard_irq_scan};
const struct LegacyTarget target_c_mouse_callback_body = {
    typed_legacy_entry, TYPED_c_mouse_callback_body};
const struct LegacyTarget target_populate_preview_flow = {
    typed_legacy_entry, TYPED_populate_preview_flow};
const struct LegacyTarget target_preview_caption_flow = {
    typed_legacy_entry, TYPED_preview_caption_flow};
const struct LegacyTarget target_preview_key_flow = {typed_legacy_entry, TYPED_preview_key_flow};
const struct LegacyTarget target_c_sentry_schedule_finish = {
    typed_legacy_entry, TYPED_c_sentry_schedule_finish};
const struct LegacyTarget target_c_timer_original_body = {c_timer_original_body, 0};
const struct LegacyTarget target_c_timer_triple_body = {c_timer_triple_body, 0};
const struct LegacyTarget target_wrong_secret_flow = {typed_legacy_entry, TYPED_wrong_secret_flow};
const struct LegacyTarget target_legacy_resume = {0, (uint16_t)legacy_resume};
const struct LegacyTarget target_crt_argument_handoff = {0, (uint16_t)crt_argument_handoff};
const struct LegacyTarget target_preview_population_call = {0, (uint16_t)preview_population_call};
const struct LegacyTarget target_generation_call = {0, (uint16_t)generation_call};
const struct LegacyTarget target_keyboard_irq_return = {0, (uint16_t)keyboard_irq_return};
const struct LegacyTarget target_bios_keyboard_return = {0, (uint16_t)bios_keyboard_return};
const struct LegacyTarget target_timer_chain_return = {0, (uint16_t)timer_chain_return};
const struct LegacyTarget target_timer_eoi_return = {0, (uint16_t)timer_eoi_return};
const struct LegacyTarget target_timer_original_return = {0, (uint16_t)timer_original_return};
const struct LegacyTarget target_hardware_stack_return = {0, (uint16_t)hardware_stack_return};
const struct LegacyTarget target_mouse_callback_return = {0, (uint16_t)mouse_callback_return};
const struct LegacyTarget target_argv_call = {0, (uint16_t)argv_call};

void c_dispatch_entry(struct LegacyRegisters *r, uint32_t frame, uint16_t target)
{
    while (target) {
        const struct LegacyTarget *entry = (const struct LegacyTarget *)target;
        if (!entry->function) {
            ((uint16_t *)r)[-1] = entry->native_ip;
            return;
        }
        ((uint16_t *)r)[-1] = 0;
        entry->function(r, frame, entry->native_ip);
        target = ((uint16_t *)r)[-1];
    }
}

__asm__(".intel_syntax noprefix\n"
        ".code16\n"
        ".section .text.c_enter_bridge,\"ax\"\n"
        "# Native boundaries CALL this helper followed by a symbolic descriptor.\n"
        "c_enter_literal:\n"
        "pushf\n"
        "push ax\n"
        "push bx\n"
        "push cx\n"
        "push dx\n"
        "push si\n"
        "push di\n"
        "push bp\n"
        "push ds\n"
        "push es\n"
        "mov bp,sp\n"
        "mov bx,word ptr ss:[bp+20]\n"
        "mov ax,word ptr cs:[bx]\n"
        "jmp .Lremove_entry_word\n"
        ".globl c_enter_entry\n"
        "c_enter_entry:\n"
        "pushf\n"
        "push ax\n"
        "push bx\n"
        "push cx\n"
        "push dx\n"
        "push si\n"
        "push di\n"
        "push bp\n"
        "push ds\n"
        "push es\n"
        "mov bp,sp\n"
        "mov ax,word ptr ss:[bp+20]\n"
        ".Lremove_entry_word:\n"
        "push ss\n"
        "pop ds\n"
        "push ss\n"
        "pop es\n"
        "# Remove the literal return word, preserving the full register frame.\n"
        "lea si,[bp+18]\n"
        "lea di,[bp+20]\n"
        "mov cx,10\n"
        "std\n"
        "rep movsw\n"
        "cld\n"
        "add sp,2\n"
        "jmp .Lprepared_c_frame\n"
        "# Shared register-to-C bridge; normal calls and foreign-stack IRQs.\n"
        ".Lprepared_c_frame:\n"
        "mov di,ax\n"
        "mov ax,cs\n"
        "add ax, OFFSET FLAT:PROGRAM_DATA_PARAGRAPH\n"
        "mov dx,ss\n"
        "cmp ax,dx\n"
        "jnz .Lforeign_c_stack\n"
        "push ss\n"
        "pop ds\n"
        "push ss\n"
        "pop es\n"
        "mov ax,sp\n"
        "mov bp,ax\n"
        "xor cx,cx\n"
        "push cx\n"
        "push di\n"
        "push ss\n"
        "push ax\n"
        "push ax\n"
        "cld\n"
        "call c_dispatch_entry\n"
        "lea sp,[bp-2]\n"
        "pop ax\n"
        "jmp .Lrestore_c_frame\n"
        ".Lforeign_c_stack:\n"
        "# IRQ during DOS: copy the saved frame to the reserved game-data stack.\n"
        "mov bx,sp\n"
        "cli\n"
        "mov ss,ax\n"
        "mov sp, OFFSET FLAT:__c_irq_stack_top\n"
        "push dx\n"
        "push bx\n"
        "push di\n"
        "sub sp,20\n"
        "mov ds,dx\n"
        "mov es,ax\n"
        "mov si,bx\n"
        "mov di,sp\n"
        "mov cx,10\n"
        "cld\n"
        "rep movsw\n"
        "mov ds,ax\n"
        "mov es,ax\n"
        "mov bp,sp\n"
        "mov ax,sp\n"
        "mov bp,ax\n"
        "xor cx,cx\n"
        "push cx\n"
        "push word ptr ss:[bp+20]\n"
        "push word ptr ss:[bp+24]\n"
        "push word ptr ss:[bp+22]\n"
        "push ax\n"
        "mov di,word ptr ss:[bp+20]\n"
        "test word ptr ss:[bp+18],0x200\n"
        "jz .Lcall_foreign_c\n"
        "sti\n"
        ".Lcall_foreign_c:\n"
        "call c_dispatch_entry\n"
        "lea sp,[bp-2]\n"
        "pop ax\n"
        "cli\n"
        "mov bx,word ptr ss:[bp+22]\n"
        "mov dx,word ptr ss:[bp+24]\n"
        "mov es,dx\n"
        "mov si,bp\n"
        "mov di,bx\n"
        "mov cx,10\n"
        "cld\n"
        "rep movsw\n"
        "mov ss,dx\n"
        "mov sp,bx\n"
        ".Lrestore_c_frame:\n"
        "test ax,ax\n"
        "jnz .Ltail_c_frame\n"
        "pop es\n"
        "pop ds\n"
        "pop bp\n"
        "pop di\n"
        "pop si\n"
        "pop dx\n"
        "pop cx\n"
        "pop bx\n"
        "pop ax\n"
        "popf\n"
        "ret\n"
        ".Ltail_c_frame:\n"
        "mov bp,sp\n"
        "push word ptr ss:[bp+18]\n"
        "popf\n"
        "mov word ptr ss:[bp+18],ax\n"
        "pop es\n"
        "pop ds\n"
        "pop bp\n"
        "pop di\n"
        "pop si\n"
        "pop dx\n"
        "pop cx\n"
        "pop bx\n"
        "pop ax\n"
        "ret\n"
        ".att_syntax prefix\n");
