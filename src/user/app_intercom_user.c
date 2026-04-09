#include "app_intercom_user.h"
#include "../ui/app_intercom_ui.h"
#include "gui_list.h"

/* Intercom state flags */
static bool intercom_toggle_state = false;
static bool intercom_muted = false;
static bool intercom_talking = false;
static bool intercom_receiving = false;
static int receive_timer_count = 0;
static int waveform_frame_index = 0;

/*
 * Helper: show/hide list notes by index range.
 * Iterates children of the given list, casts each to gui_list_note_t,
 * and sets visibility for notes whose index is within [start_idx, end_idx].
 */
static void set_list_items_visible(gui_obj_t *list, int start_idx, int end_idx, bool visible)
{
    gui_node_list_t *node;
    gui_list_for_each(node, &list->child_list)
    {
        gui_list_note_t *note = (gui_list_note_t *)gui_list_entry(node, gui_obj_t, brother_list);
        if (note->index >= start_idx && note->index <= end_idx)
        {
            gui_obj_show((gui_obj_t *)note, visible);
        }
    }
}

/**
 * intercom_toggle_on: Show device list (toggle_list items index 1~4)
 * Called by hg_button onCallback when toggled ON.
 */
void intercom_toggle_on(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    intercom_toggle_state = true;

    /* Expand scroll range to show all 5 items */
    gui_list_set_note_num(toggle_list, 3);

    /* Show items 0~2 */
    set_list_items_visible((gui_obj_t *)toggle_list, 0, 2, true);

    gui_obj_show((gui_obj_t *)toggle_list, true);

    gui_obj_show((gui_obj_t *)available_devices_label, true);

    gui_fb_change();
}

/**
 * intercom_toggle_off: Hide device list (toggle_list items index 1~4)
 * Called by hg_button offCallback when toggled OFF.
 */
void intercom_toggle_off(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    intercom_toggle_state = false;

    gui_obj_show((gui_obj_t *)toggle_list, false);

    gui_obj_show((gui_obj_t *)available_devices_label, false);

    gui_fb_change();
}

/**
 * talk_btn_press: Press-to-hold start talking.
 * Lazily creates waveform animation timer on first call.
 * TK-1~TK-3: Switch to transmitting state.
 */
void talk_btn_press(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    /* Lazy-create waveform animation timer (50ms recurring) */
    if (waveform_image != NULL)
    {
        gui_obj_create_timer((gui_obj_t *)waveform_image, 50, -1, receive_sim_timer_cb);
        gui_obj_start_timer((gui_obj_t *)waveform_image);
    }

    intercom_talking = true;
    intercom_receiving = false;

    /* TK-1: Talk button active (cyan) */
    gui_img_set_src((gui_img_t *)talk_btn, "/app_intercom/talk_btn_active.bin", IMG_SRC_FILESYS);
    gui_img_refresh_size((gui_img_t *)talk_btn);

    /* TK-2: Switch waveform to transmitting frame 0 */
    waveform_frame_index = 0;
    gui_img_set_src((gui_img_t *)waveform_image, "/app_intercom/waveform/transmitting/transmitting_frame_00.bin",
                    IMG_SRC_FILESYS);
    gui_img_refresh_size((gui_img_t *)waveform_image);

    /* TK-3: Update status text */
    gui_text_content_set((gui_text_t *)status_text_label, "Transmitting...", 15);

    gui_fb_change();
}

/**
 * talk_btn_release: Release to stop talking.
 * TK-4~TK-6: Switch to idle state.
 */
void talk_btn_release(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    intercom_talking = false;

    /* TK-4: Talk button normal (idle) */
    gui_img_set_src((gui_img_t *)talk_btn, "/app_intercom/talk_btn_normal.bin", IMG_SRC_FILESYS);
    gui_img_refresh_size((gui_img_t *)talk_btn);

    /* TK-5: Switch waveform to idle frame 0 */
    waveform_frame_index = 0;
    gui_img_set_src((gui_img_t *)waveform_image, "/app_intercom/waveform/idle/idle_frame_00.bin",
                    IMG_SRC_FILESYS);
    gui_img_refresh_size((gui_img_t *)waveform_image);

    /* TK-6: Update status text */
    gui_text_content_set((gui_text_t *)status_text_label, "Hold to Talk", 12);

    gui_fb_change();
}


/**
 * mute_btn_on: Mute enabled (hg_button handles image switching).
 * MT-1: Set muted flag, suppress receiving waveform.
 */
void mute_btn_on(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    intercom_muted = true;
    gui_fb_change();
}

/**
 * mute_btn_off: Mute disabled (hg_button handles image switching).
 * MT-3: Clear muted flag, allow receiving waveform.
 */
void mute_btn_off(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    intercom_muted = false;
    gui_fb_change();
}

/**
 * receive_sim_timer_cb: Simulate incoming voice (called every 100ms by timer)
 * - Count to 60 (6s): start receiving → RX-1~RX-3
 * - Count to 80 (8s = 6+2): stop receiving → RX-4~RX-6, reset counter
 * - Also handles waveform frame animation cycling
 */
void receive_sim_timer_cb(void *obj)
{
    GUI_UNUSED(obj);

    char frame_path[64];

    /* If user is talking, skip receive simulation and just animate transmitting waveform */
    if (intercom_talking)
    {
        waveform_frame_index = (waveform_frame_index + 1) % 30;
        sprintf(frame_path, "/app_intercom/waveform/transmitting/transmitting_frame_%02d.bin", waveform_frame_index);
        gui_img_set_src((gui_img_t *)waveform_image, frame_path, IMG_SRC_FILESYS);
        gui_img_refresh_size((gui_img_t *)waveform_image);
        receive_timer_count = 0; /* Reset receive counter while talking */
        gui_fb_change();
        return;
    }

    receive_timer_count++;

    if (!intercom_receiving && receive_timer_count >= 60)
    {
        /* 6s elapsed: start receiving */
        intercom_receiving = true;
        receive_timer_count = 60; /* Anchor at 60 for the 2s duration */
        waveform_frame_index = 0;

        /* RX-1: Talk button receiving (green) */
        gui_img_set_src((gui_img_t *)talk_btn, "/app_intercom/talk_btn_receiving.bin", IMG_SRC_FILESYS);
        gui_img_refresh_size((gui_img_t *)talk_btn);

        /* RX-3: Status text */
        gui_text_content_set((gui_text_t *)status_text_label, "Receiving...", 12);
    }

    if (intercom_receiving)
    {
        /* Animate receiving waveform (if not muted: MT-2) */
        if (!intercom_muted)
        {
            waveform_frame_index = (waveform_frame_index + 1) % 30;
            sprintf(frame_path, "/app_intercom/waveform/receiving/receiving_frame_%02d.bin", waveform_frame_index);
            gui_img_set_src((gui_img_t *)waveform_image, frame_path, IMG_SRC_FILESYS);
            gui_img_refresh_size((gui_img_t *)waveform_image);
        }

        if (receive_timer_count >= 80)
        {
            /* 8s (6+2) elapsed: stop receiving → RX-4~RX-6 */
            intercom_receiving = false;
            receive_timer_count = 0;
            waveform_frame_index = 0;

            /* RX-4: Talk button back to normal */
            gui_img_set_src((gui_img_t *)talk_btn, "/app_intercom/talk_btn_normal.bin", IMG_SRC_FILESYS);
            gui_img_refresh_size((gui_img_t *)talk_btn);

            /* RX-5: Waveform back to idle */
            gui_img_set_src((gui_img_t *)waveform_image, "/app_intercom/waveform/idle/idle_frame_00.bin",
                            IMG_SRC_FILESYS);
            gui_img_refresh_size((gui_img_t *)waveform_image);

            /* RX-6: Status text */
            gui_text_content_set((gui_text_t *)status_text_label, "Hold to Talk", 12);
        }
    }
    else
    {
        /* Idle state: animate idle waveform at slower rate (every other tick) */
        if (receive_timer_count % 2 == 0)
        {
            waveform_frame_index = (waveform_frame_index + 1) % 30;
            sprintf(frame_path, "/app_intercom/waveform/idle/idle_frame_%02d.bin", waveform_frame_index);
            gui_img_set_src((gui_img_t *)waveform_image, frame_path, IMG_SRC_FILESYS);
            gui_img_refresh_size((gui_img_t *)waveform_image);
        }
    }

    gui_fb_change();
}
