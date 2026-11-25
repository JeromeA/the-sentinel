#include "owned_state.h"
#include "adapter_data.h"
/* DOS segment references remain linker symbols at hardware boundaries. */
__asm__(".equ PROGRAM_DATA_PARAGRAPH, __game_data_paragraph\n");
