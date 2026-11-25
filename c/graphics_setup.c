/* Allocation failure leaves through the primary-segment CRT entry.
 * Adapter bindings and initialization live in graphics_ui.c. */

void __far graphics_memory_error(SegmentBytes data, uint16_t message)
{
    uint16_t end = message;
    while (data[end])
        ++end;
    SegmentBytes text = data + message;
    SegmentBytes display_start = segment_at_offset(data, message);
    c_dos_print(text, text, display_start, 0, 0x202);
    data[end] = 0;
    /* CRT exit restores installed interrupt hooks before returning to DOS. */
    abi_c_crt_exit(1);
}
