#ifndef SENTINEL_ADAPTER_DATA_H
#define SENTINEL_ADAPTER_DATA_H
#include "register_compat.h"
/* Owned adapter names, resource filenames and allocation diagnostics. */
extern struct UnalignedWord adapter_name_pointers[9];
extern uint8_t name_mda[22];
extern uint8_t adapter_label_cga[4];
extern uint8_t adapter_label_ega[4];
extern uint8_t name_mcga[6];
extern uint8_t adapter_label_vga[4];
extern uint8_t name_hercules[24];
extern uint8_t name_hercules_plus[24];
extern uint8_t name_incolor[14];
extern uint8_t msg_cannot_find[14];
extern uint8_t msg_no_memory_1[36];
extern uint8_t msg_no_memory_2[36];
extern uint8_t msg_no_memory_3[36];
extern uint8_t msg_no_memory_4[36];
extern uint8_t msg_no_memory_5[36];
extern uint8_t msg_unsupported_adapter[34];
extern uint8_t filename_skybox[12];
extern uint8_t filename_tfinal[11];
#endif
