/* Panels are named slices of the two predecoded resources. */

static unsigned blit_panel(enum RenderPanel panel, int backwards, struct ImageCopyEnd *end)
{
    struct ImageSlice slice;
    switch (panel) {
    case RENDER_TITLE:
        slice = (struct ImageSlice){title_final_image_segment, 0, 200, 40, backwards};
        break;
    case RENDER_SKYBOX:
        slice = (struct ImageSlice){skybox_image_segment, 0, 157, 40, backwards};
        break;
    case RENDER_LANDSCAPE_PANEL:
        slice = (struct ImageSlice){skybox_image_segment, 0xb2, 19, 28, backwards};
        break;
    case RENDER_PROMPT_HEADER:
        slice = (struct ImageSlice){title_final_image_segment, 0x10, 8, 40, backwards};
        break;
    default:
        slice = (struct ImageSlice){skybox_image_segment, 0x18, 120, 32, backwards};
        break;
    }
    BlitCallback callback = graphics_blit_image;
    unsigned mode = 0;
    if (end)
        mode = callback == video_blit_cga  ? 0
            : callback == video_blit_tandy ? 1
            : callback == video_blit_ega   ? 2
                                           : 3;
    call_video_blit(callback ? callback : video_blit_vga, &slice, end);
    return mode;
}

void __far draw_panel(enum RenderPanel panel, int backwards)
{
    blit_panel(panel, backwards, 0);
}

unsigned __far c_blit_panel(enum RenderPanel panel, int backwards, struct ImageCopyEnd *end)
{
    return blit_panel(panel, backwards, end);
}
