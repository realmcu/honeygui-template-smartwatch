#include "app_control_center_user.h"
#include "../ui/app_control_center_ui.h"
#include "gui_list.h"

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

void bluetooth_toggle_on(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    /* Update scroll range: items 0~7 visible (8 items) */
    gui_list_set_note_num(bt_list, 12);

    /* Show bt_list items index 1~7:
     * 1: phone section, 2: phone item, 3: headphones section,
     * 4~10: headphones 1-7, 11: search button */
    set_list_items_visible((gui_obj_t *)bt_list, 1, 11, true);

    gui_fb_change();
}

void bluetooth_toggle_off(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    /* Hide bt_list items index 1~10: all device items + found devices */
    set_list_items_visible((gui_obj_t *)bt_list, 1, 10, false);

    /* Shrink scroll range to only toggle item, reset scroll position */
    gui_list_set_note_num(bt_list, 1);
    gui_list_set_offset(bt_list, 0);

    gui_fb_change();
}

void bluetooth_search_devices(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    gui_fb_change();
}


void wifi_toggle_on(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    /* Expand scroll range to 3 items */
    gui_list_set_note_num(wifi_list, 3);

    /* Show wifi_list items index 1~2: saved networks section + network item */
    set_list_items_visible((gui_obj_t *)wifi_list, 1, 2, true);

    gui_fb_change();
}

void wifi_toggle_off(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    /* Hide wifi_list items index 1~2 */
    set_list_items_visible((gui_obj_t *)wifi_list, 1, 2, false);

    /* Shrink scroll range to only toggle item, reset scroll position */
    gui_list_set_note_num(wifi_list, 1);
    gui_list_set_offset(wifi_list, 0);

    gui_fb_change();
}

void phone_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* send msg to app */
    gui_fb_change();
}
void headphone1_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* send msg to app */
    gui_fb_change();
}
void headphone2_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* send msg to app */
    gui_fb_change();
}
void headphone3_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* send msg to app */
    gui_fb_change();
}
void headphone4_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* send msg to app */
    gui_fb_change();
}
void headphone5_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* send msg to app */
    gui_fb_change();
}
void headphone6_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* send msg to app */
    gui_fb_change();
}
void headphone7_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* send msg to app */
    gui_fb_change();
}
void phone_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* remember the index */
    gui_fb_change();
}
void headphone1_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* remember the index */
    gui_fb_change();
}
void headphone2_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* remember the index */
    gui_fb_change();
}
void headphone3_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* remember the index */
    gui_fb_change();
}
void headphone4_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* remember the index */
    gui_fb_change();
}
void headphone5_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* remember the index */
    gui_fb_change();
}
void headphone6_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* remember the index */
    gui_fb_change();
}
void headphone7_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* remember the index */
    gui_fb_change();
}
void remove_paired_device_confirm(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* send msg to app */
    gui_fb_change();
}

void remove_paired_device_cancel(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* send msg to app */
    gui_fb_change();
}

void headphone1_connect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* send msg to app */
    gui_fb_change();
}
void headphone2_connect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    /* send msg to app */
    gui_fb_change();
}