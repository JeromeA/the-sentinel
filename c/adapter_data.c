/* Adapter names and resource/allocation messages belong to the menu layer.
 * Pointer entries are resolved by the linker, not encoded DS offsets. */

struct UnalignedWord adapter_name_pointers[9] = {{(uint16_t)crt_none},
    {(uint16_t)name_mda},
    {(uint16_t)adapter_label_cga},
    {(uint16_t)adapter_label_ega},
    {(uint16_t)name_mcga},
    {(uint16_t)adapter_label_vga},
    {(uint16_t)name_hercules},
    {(uint16_t)name_hercules_plus},
    {(uint16_t)name_incolor}};
uint8_t name_mda[22] = "Mono Display Adaptor";
uint8_t adapter_label_cga[4] = "CGA";
uint8_t adapter_label_ega[4] = "EGA";
uint8_t name_mcga[6] = "MCGA";
uint8_t adapter_label_vga[4] = "VGA";
uint8_t name_hercules[24] = "Hercules Graphics Card";
uint8_t name_hercules_plus[24] = "Hercules Graphics Card+";
uint8_t name_incolor[14] = "InColor Card";
uint8_t msg_cannot_find[14] = "Cannot find ";
uint8_t msg_no_memory_1[36] = "Not enough RAM to run this program.";
uint8_t msg_no_memory_2[36] = "Not enough RAM to run this program.";
uint8_t msg_no_memory_3[36] = "Not enough RAM to run this program.";
uint8_t msg_no_memory_4[36] = "Not enough RAM to run this program.";
uint8_t msg_no_memory_5[36] = "Not enough RAM to run this program.";
uint8_t msg_unsupported_adapter[34] = "This program will not run with a ";
uint8_t filename_skybox[12] = "SKYBOX.BIN";
uint8_t filename_tfinal[11] = "TFINAL.BIN";
