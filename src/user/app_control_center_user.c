#include "app_control_center_user.h"
#include "../ui/app_control_center_ui.h"
#include "../callbacks/app_control_center_callbacks.h"
#include "gui_list.h"
#include "gui_text.h"
#include "gui_rect.h"
#include "gui_img.h"
#include "gui_fb.h"
#include "gui_obj_tree.h"
#include <string.h>
#include <stdio.h>

#ifndef _HONEYGUI_SIMULATOR_
#include "app_bond.h"
#include "app_cfg.h"
#include "app_msg.h"
#include "app_task.h"
#include "app_main.h"
#include "app_gap.h"
#include "app_link_util.h"
#include "gap.h"
#include "app_common_event.h"
#include "event_bus.h"
#else
/* Mock types for simulator */
typedef struct
{
    uint8_t exist_addr_flag;
    uint8_t used;
    uint8_t device_name_len;
    uint8_t device_name[32];
    uint8_t bd_addr[6];
} T_APP_BOND_DEVICE;
#endif

/* Search device info (for bt_search_list) */
#define MAX_SEARCH_DEVICES 2

/* Pending device index for remove operation (0xFF = none) */
static uint8_t pending_remove_device_index = 0xFF;

/* Pending headphone to connect after current device disconnects */
#ifndef _HONEYGUI_SIMULATOR_
static bool pending_headphone_connect_after_disconnect = false;
static uint8_t pending_headphone_connect_addr[6] = {0};
static char search_addr_buffer[MAX_SEARCH_DEVICES][18];  /* For bt_search_list address display */
#endif

/* Bluetooth address buffer for settings view */
static char bt_addr_buffer[18];

static uint8_t found_device_count = 0;

/* T_SEARCH_RESULT - must match definition in app_gap.c */
typedef struct
{
    uint8_t bd_addr[6];
    uint8_t nam_len;
    uint16_t device_name[25];  // UTF-16 encoded device name
    uint32_t cod;
} T_SEARCH_RESULT;

static T_SEARCH_RESULT found_devices[MAX_SEARCH_DEVICES];

/* ==================== Simulator Mock Data ==================== */
#ifdef _HONEYGUI_SIMULATOR_

/* Mock phone: iPhone 15 Pro, Not Connected */
static const char *sim_phone_name = "iPhone 15 Pro";

/* Mock 7 headphones (match screenshot) */
typedef struct
{
    const char *name;
    bool connected;
} sim_headphone_t;

#define SIM_HEADPHONE_COUNT 7
static const sim_headphone_t sim_headphones[SIM_HEADPHONE_COUNT] =
{
    { "AirPods Pro",        true  },  /* index 0: Connected */
    { "Sony WH-1000",       false },
    { "Galaxy Buds2",       false },
    { "Beats Studio3",      false },
    { "Jabra Elite 85h",    false },
    { "Bose QC35 II",       false },
    { "Sennheiser HD 450B", false },
};

/* Mock 2 search results: JBL Tune 510BT / Bose QC45 */
static const char *sim_search_names[MAX_SEARCH_DEVICES] =
{
    "JBL Tune 510BT",
    "Bose QC45",
};

/* Mock MAC addresses (non-zero, required for has_addr check) */
static const uint8_t sim_search_addrs[MAX_SEARCH_DEVICES][6] =
{
    { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 },
    { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF },
};

#endif /* _HONEYGUI_SIMULATOR_ */

/*
 * Helper: show/hide list notes by index range.
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

/*
 * Helper: get list note by index. Returns NULL if not found.
 */
static gui_list_note_t *get_list_note_by_index(gui_obj_t *list, uint16_t index)
{
    gui_node_list_t *node;
    gui_list_for_each(node, &list->child_list)
    {
        gui_list_note_t *note = (gui_list_note_t *)gui_list_entry(node, gui_obj_t, brother_list);
        if (note->index == index)
        {
            return note;
        }
    }
    return NULL;
}

/* Send BT toggle message to app task */
static void send_bt_toggle_msg_to_app(bool enable)
{
#ifndef _HONEYGUI_SIMULATOR_
    T_IO_MSG msg;
    msg.type = IO_MSG_TYPE_WRISTBNAD;
    msg.subtype = IO_MSG_BT_TOGGLE;
    msg.u.param = enable ? 1 : 0;
    app_send_msg_to_apptask(&msg);

    /* If BT enable and have bonded phone, reconnect to phone */
    if (enable && app_db.bond_device[0].exist_addr_flag)
    {
        T_IO_MSG con_msg;
        con_msg.type = IO_MSG_TYPE_WRISTBNAD;
        con_msg.subtype = IO_MSG_CONNECT_PHONE;
        con_msg.u.buf = app_db.bond_device[0].bd_addr;
        app_send_msg_to_apptask(&con_msg);
    }
#else
    GUI_UNUSED(enable);
#endif
}

#ifndef _HONEYGUI_SIMULATOR_
/**
 * @brief Try to flush pending headphone connect request.
 *
 * Called when receiving a headphone disconnect event.
 * If a pending connect request exists and all BR links are down,
 * send IO_MSG_CONNECT_BREDR_DEVICE to the app task to connect the
 * pending headphone, then clear the flag.
 *
 * @note Must be called from GUI thread (single-thread access to flag).
 * @note This function is idempotent — if no pending request exists,
 *       it returns immediately. Safe to call multiple times.
 */
static void try_flush_pending_headphone_connect(void)
{
    if (!pending_headphone_connect_after_disconnect)
    {
        return;
    }

    /* Safety: verify the pending address is valid (non-zero) */
    bool has_addr = false;
    for (int i = 0; i < 6; i++)
    {
        if (pending_headphone_connect_addr[i] != 0) { has_addr = true; break; }
    }
    if (!has_addr)
    {
        pending_headphone_connect_after_disconnect = false;
        return;
    }

    /* Defensive check: any other active BR link still up? */
    for (uint8_t i = 1; i <= 7; i++)
    {
        T_APP_BOND_DEVICE *b = &app_db.bond_device[i];
        if (b->exist_addr_flag)
        {
            /* Skip pending target itself */
            if (memcmp(b->bd_addr, pending_headphone_connect_addr, 6) == 0)
            {
                continue;
            }
            if (app_find_br_link(b->bd_addr) != NULL)
            {
                /* Some other link still alive, defer (keep pending flag). */
                return;
            }
        }
    }

    T_IO_MSG msg;
    msg.type = IO_MSG_TYPE_WRISTBNAD;
    msg.subtype = IO_MSG_CONNECT_BREDR_DEVICE;
    msg.u.buf = pending_headphone_connect_addr;
    app_send_msg_to_apptask(&msg);

    pending_headphone_connect_after_disconnect = false;
}

/**
 * @brief Clear the pending headphone connect state.
 */
static void clear_pending_headphone_connect(void)
{
    pending_headphone_connect_after_disconnect = false;
    memset(pending_headphone_connect_addr, 0, 6);
}
#endif


/**
 * @brief Helper function to connect or disconnect a headphone (for bond list)
 * @param index Headphone index (1-7), corresponds to bond_device[1-7]
 */
static void headphone_connect_or_disconnect(uint8_t index)
{
#ifndef _HONEYGUI_SIMULATOR_
    if (index < 1 || index > 7)
    {
        return;
    }

    T_APP_BOND_DEVICE *bond = &app_db.bond_device[index];
    if (!bond->exist_addr_flag)
    {
        return;
    }

    T_IO_MSG msg;
    msg.type = IO_MSG_TYPE_WRISTBNAD;

    T_APP_BR_LINK *p_link = app_find_br_link(bond->bd_addr);
    if (p_link != NULL)
    {
        /* Currently connected → user wants to disconnect it */
        msg.subtype = IO_MSG_DISCONNECT_BREDR_DEVICE;
        msg.u.buf = bond->bd_addr;

        /* Clear any stale pending request — user does NOT want auto-reconnect */
        clear_pending_headphone_connect();
    }
    else
    {
        /* Check if another headphone is connected */
        bool another_connected = false;
        for (uint8_t i = 1; i <= 7; i++)
        {
            if (i == index) { continue; }
            T_APP_BOND_DEVICE *other_bond = &app_db.bond_device[i];
            if (other_bond->exist_addr_flag &&
                app_find_br_link(other_bond->bd_addr) != NULL)
            {
                another_connected = true;
                break;
            }
        }

        if (another_connected)
        {
            /* app_task will auto-disconnect the active one; we must resend
             * CONNECT once that disconnect completes. Save pending address. */
            pending_headphone_connect_after_disconnect = true;
            memcpy(pending_headphone_connect_addr, bond->bd_addr, 6);
        }
        else
        {
            /* No pending needed — app_task will connect directly */
            clear_pending_headphone_connect();
        }

        msg.subtype = IO_MSG_CONNECT_BREDR_DEVICE;
        msg.u.buf = bond->bd_addr;
    }

    app_send_msg_to_apptask(&msg);
#else
    GUI_UNUSED(index);
#endif
}

#ifndef _HONEYGUI_SIMULATOR_
/**
 * @brief Common connect logic for headphones found via search.
 * @param index Index into found_devices[] (0 ~ MAX_SEARCH_DEVICES-1)
 */
static void headphone_search_connect_common(uint8_t index)
{
    if (index >= MAX_SEARCH_DEVICES) { return; }

    uint8_t *addr = found_devices[index].bd_addr;

    /* Validate address (non-zero) */
    bool has_addr = false;
    for (int i = 0; i < 6; i++)
    {
        if (addr[i] != 0) { has_addr = true; break; }
    }
    if (!has_addr) { return; }

    /* Stop inquiry first if running */
    if (get_search_status() == SEARCH_START)
    {
        app_bt_bond_temp_cache_save_to_search();

        T_IO_MSG dis_inquiry_msg;
        dis_inquiry_msg.type = IO_MSG_TYPE_WRISTBNAD;
        dis_inquiry_msg.subtype = IO_MSG_INQUIRY_STOP;
        app_send_msg_to_apptask(&dis_inquiry_msg);
    }

    /* Check if another headphone is already connected */
    bool another_connected = false;
    for (uint8_t i = 1; i <= 7; i++)
    {
        T_APP_BOND_DEVICE *other_bond = &app_db.bond_device[i];
        if (other_bond->exist_addr_flag &&
            app_find_br_link(other_bond->bd_addr) != NULL)
        {
            another_connected = true;
            break;
        }
    }

    if (another_connected)
    {
        /* app_task will auto-disconnect the active one; we must resend
         * CONNECT once that disconnect completes via pending flush. */
        pending_headphone_connect_after_disconnect = true;
        memcpy(pending_headphone_connect_addr, addr, 6);
    }
    else
    {
        clear_pending_headphone_connect();
    }

    T_IO_MSG msg;
    msg.type = IO_MSG_TYPE_WRISTBNAD;
    msg.subtype = IO_MSG_CONNECT_BREDR_DEVICE;
    msg.u.buf = addr;
    app_send_msg_to_apptask(&msg);
}
#endif

void bluetooth_toggle_on(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    /* Show headphones entry and phone section widgets */
    if (bt_headphones_entry_bg != NULL)
    {
        gui_obj_show((gui_obj_t *)bt_headphones_entry_bg, true);
    }
    if (headphones_entry_icon != NULL)
    {
        gui_obj_show((gui_obj_t *)headphones_entry_icon, true);
    }
    if (headphones_entry_label != NULL)
    {
        gui_obj_show((gui_obj_t *)headphones_entry_label, true);
    }
    if (phone_section_label != NULL)
    {
        gui_obj_show((gui_obj_t *)phone_section_label, true);
    }

    /* bt_list note structure:
     * index 0: phone item (show if phone bonded) */
    uint16_t note_count = 0;

#ifndef _HONEYGUI_SIMULATOR_
    if (app_db.bond_device[0].exist_addr_flag)
    {
        note_count = 1;  /* Show phone item */
    }
#else
    /* Simulator: always show 1 phone (iPhone 15 Pro) */
    note_count = 1;
#endif

    /* Show bt_list first, then set note count */
    gui_obj_show((gui_obj_t *)bt_list, true);
    gui_list_set_note_num(bt_list, note_count);

    /* Send BT toggle on message to app task */
    send_bt_toggle_msg_to_app(true);

    gui_fb_change();
}

void bluetooth_toggle_off(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    /* Send BT toggle off message to app task */
    send_bt_toggle_msg_to_app(false);

#ifndef _HONEYGUI_SIMULATOR_
    /* Clear any pending headphone connect request — BT is going off */
    clear_pending_headphone_connect();
#endif

    /* Hide headphones entry and phone section widgets */
    if (bt_headphones_entry_bg != NULL)
    {
        gui_obj_show((gui_obj_t *)bt_headphones_entry_bg, false);
    }
    if (headphones_entry_icon != NULL)
    {
        gui_obj_show((gui_obj_t *)headphones_entry_icon, false);
    }
    if (headphones_entry_label != NULL)
    {
        gui_obj_show((gui_obj_t *)headphones_entry_label, false);
    }
    if (phone_section_label != NULL)
    {
        gui_obj_show((gui_obj_t *)phone_section_label, false);
    }

    /* Hide bt_list and set note count to 0 */
    gui_obj_show((gui_obj_t *)bt_list, false);
    gui_list_set_note_num(bt_list, 0);
    gui_list_set_offset(bt_list, 0);

    gui_fb_change();
}

void bluetooth_search_devices(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

#ifndef _HONEYGUI_SIMULATOR_
    /* Start Bluetooth search */
    T_IO_MSG inquiry_msg;
    inquiry_msg.type = IO_MSG_TYPE_WRISTBNAD;
    inquiry_msg.subtype = IO_MSG_INQUIRY_START;
    app_send_msg_to_apptask(&inquiry_msg);
#else
    /* Simulator: populate 2 mock found devices */
    found_device_count = MAX_SEARCH_DEVICES;
    for (uint8_t i = 0; i < MAX_SEARCH_DEVICES; i++)
    {
        memset(&found_devices[i], 0, sizeof(T_SEARCH_RESULT));
        memcpy(found_devices[i].bd_addr, sim_search_addrs[i], 6);
        found_devices[i].nam_len = 0;  /* UTF-16 name not used; sim name handled in note_design */
    }
    if (bt_search_list != NULL)
    {
        gui_obj_show((gui_obj_t *)bt_search_list, true);
        gui_list_set_note_num(bt_search_list, found_device_count);
    }
#endif

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

#ifndef _HONEYGUI_SIMULATOR_
    T_APP_BOND_DEVICE *phone_bond = &app_db.bond_device[0];
    if (!phone_bond->exist_addr_flag)
    {
        return;
    }

    T_IO_MSG msg;
    msg.type = IO_MSG_TYPE_WRISTBNAD;

    if (phone_bond->used)
    {
        msg.subtype = IO_MSG_DISCONNECT_BREDR_DEVICE;
    }
    else
    {
        msg.subtype = IO_MSG_CONNECT_PHONE;
    }

    msg.u.buf = phone_bond->bd_addr;
    app_send_msg_to_apptask(&msg);
#endif

    gui_fb_change();
}

void headphone1_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    headphone_connect_or_disconnect(1);
    gui_fb_change();
}
void headphone2_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    headphone_connect_or_disconnect(2);
    gui_fb_change();
}
void headphone3_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    headphone_connect_or_disconnect(3);
    gui_fb_change();
}
void headphone4_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    headphone_connect_or_disconnect(4);
    gui_fb_change();
}
void headphone5_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    headphone_connect_or_disconnect(5);
    gui_fb_change();
}
void headphone6_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    headphone_connect_or_disconnect(6);
    gui_fb_change();
}
void headphone7_linkback_and_disconnect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    headphone_connect_or_disconnect(7);
    gui_fb_change();
}

void phone_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    pending_remove_device_index = 0;
    if (unbind_confirm_bg != NULL) { gui_obj_show((gui_obj_t *)unbind_confirm_bg, true); }
    if (unbind_cancel_bg != NULL)  { gui_obj_show((gui_obj_t *)unbind_cancel_bg,  true); }
    gui_fb_change();
}
void headphone1_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    pending_remove_device_index = 1;
    if (unbind_confirm_bg != NULL) { gui_obj_show((gui_obj_t *)unbind_confirm_bg, true); }
    if (unbind_cancel_bg != NULL)  { gui_obj_show((gui_obj_t *)unbind_cancel_bg,  true); }
    gui_fb_change();
}
void headphone2_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    pending_remove_device_index = 2;
    if (unbind_confirm_bg != NULL) { gui_obj_show((gui_obj_t *)unbind_confirm_bg, true); }
    if (unbind_cancel_bg != NULL)  { gui_obj_show((gui_obj_t *)unbind_cancel_bg,  true); }
    gui_fb_change();
}
void headphone3_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    pending_remove_device_index = 3;
    if (unbind_confirm_bg != NULL) { gui_obj_show((gui_obj_t *)unbind_confirm_bg, true); }
    if (unbind_cancel_bg != NULL)  { gui_obj_show((gui_obj_t *)unbind_cancel_bg,  true); }
    gui_fb_change();
}
void headphone4_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    pending_remove_device_index = 4;
    if (unbind_confirm_bg != NULL) { gui_obj_show((gui_obj_t *)unbind_confirm_bg, true); }
    if (unbind_cancel_bg != NULL)  { gui_obj_show((gui_obj_t *)unbind_cancel_bg,  true); }
    gui_fb_change();
}
void headphone5_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    pending_remove_device_index = 5;
    if (unbind_confirm_bg != NULL) { gui_obj_show((gui_obj_t *)unbind_confirm_bg, true); }
    if (unbind_cancel_bg != NULL)  { gui_obj_show((gui_obj_t *)unbind_cancel_bg,  true); }
    gui_fb_change();
}
void headphone6_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    pending_remove_device_index = 6;
    if (unbind_confirm_bg != NULL) { gui_obj_show((gui_obj_t *)unbind_confirm_bg, true); }
    if (unbind_cancel_bg != NULL)  { gui_obj_show((gui_obj_t *)unbind_cancel_bg,  true); }
    gui_fb_change();
}
void headphone7_remove_paired_device(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    pending_remove_device_index = 7;
    if (unbind_confirm_bg != NULL) { gui_obj_show((gui_obj_t *)unbind_confirm_bg, true); }
    if (unbind_cancel_bg != NULL)  { gui_obj_show((gui_obj_t *)unbind_cancel_bg,  true); }
    gui_fb_change();
}

void remove_paired_device_confirm(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);

#ifndef _HONEYGUI_SIMULATOR_
    if (pending_remove_device_index <= 7)
    {
        /* If the device being removed is the pending connect target,
         * clear pending state to avoid ghost re-connect after removal. */
        T_APP_BOND_DEVICE *b = &app_db.bond_device[pending_remove_device_index];
        if (pending_headphone_connect_after_disconnect &&
            b->exist_addr_flag &&
            memcmp(b->bd_addr, pending_headphone_connect_addr, 6) == 0)
        {
            clear_pending_headphone_connect();
        }

        T_IO_MSG msg;
        msg.type = IO_MSG_TYPE_WRISTBNAD;
        msg.subtype = IO_MSG_REMOVE_BOND_DEVICE;
        msg.u.param = pending_remove_device_index;
        app_send_msg_to_apptask(&msg);
    }
#endif

    pending_remove_device_index = 0xFF;
    gui_fb_change();
}

void remove_paired_device_cancel(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
    pending_remove_device_index = 0xFF;
    gui_fb_change();
}

void headphone1_connect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
#ifndef _HONEYGUI_SIMULATOR_
    headphone_search_connect_common(0);
#endif
    gui_fb_change();
}

void headphone2_connect(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj); GUI_UNUSED(e);
#ifndef _HONEYGUI_SIMULATOR_
    headphone_search_connect_common(1);
#endif
    gui_fb_change();
}

void update_phone_list(gui_obj_t *obj, const char *topic, void *data, uint16_t len)
{
    GUI_UNUSED(obj); GUI_UNUSED(topic); GUI_UNUSED(data); GUI_UNUSED(len);

#ifndef _HONEYGUI_SIMULATOR_
    if (app_db.bond_device[0].exist_addr_flag)
    {
        gui_obj_show((gui_obj_t *)bt_list, true);
        gui_list_set_note_num(bt_list, 1);
    }
    else
    {
        gui_list_set_note_num(bt_list, 0);
    }
#else
    /* Simulator: always show 1 phone item (iPhone 15 Pro) */
    gui_obj_show((gui_obj_t *)bt_list, true);
    gui_list_set_note_num(bt_list, 1);
#endif

    gui_fb_change();
}

void update_headphone_list(gui_obj_t *obj, const char *topic, void *data, uint16_t len)
{
    GUI_UNUSED(obj); GUI_UNUSED(topic); GUI_UNUSED(data); GUI_UNUSED(len);

    if (headphone_list != NULL)
    {
#ifndef _HONEYGUI_SIMULATOR_
        uint8_t headphone_count = 0;
        for (int i = 1; i <= 7; i++)
        {
            if (app_db.bond_device[i].exist_addr_flag) { headphone_count++; }
        }
        gui_obj_show((gui_obj_t *)headphone_list, true);
        gui_list_set_note_num(headphone_list, headphone_count);
#else
        gui_obj_show((gui_obj_t *)headphone_list, true);
        gui_list_set_note_num(headphone_list, SIM_HEADPHONE_COUNT);
#endif
    }

    gui_fb_change();
}

void update_search_list(gui_obj_t *obj, const char *topic, void *data, uint16_t len)
{
    GUI_UNUSED(obj); GUI_UNUSED(data); GUI_UNUSED(len);

    if (strcmp(topic, "bt/inquiry_result") == 0)
    {
        T_SEARCH_RESULT *result = (T_SEARCH_RESULT *)data;

        if (result != NULL && found_device_count < MAX_SEARCH_DEVICES)
        {
            bool already_exists = false;
            for (uint8_t i = 0; i < found_device_count; i++)
            {
                if (memcmp(found_devices[i].bd_addr, result->bd_addr, 6) == 0)
                {
                    already_exists = true;
                    break;
                }
            }

            if (!already_exists)
            {
                memcpy(&found_devices[found_device_count], result, sizeof(T_SEARCH_RESULT));
                found_device_count++;
            }

            gui_obj_show((gui_obj_t *)bt_search_list, true);
            gui_list_set_note_num(bt_search_list, found_device_count);
        }
    }
    else if (strcmp(topic, "bt/inquiry_cmpl") == 0)
    {
        gui_obj_show((gui_obj_t *)bt_search_list, false);
        gui_list_set_note_num(bt_search_list, 0);
        found_device_count = 0;
    }

    gui_fb_change();
}

/**
 * @brief Event bus callback for BT connection events (strong override)
 * @note Overrides weak symbol in app_common_event.c
 */
#ifndef _HONEYGUI_SIMULATOR_
int32_t bt_event_bus_callback(T_EVENT_BUS_EVENT_DATA *event_data)
{
    if (event_data == NULL || event_data->topic == NULL)
    {
        return EVENT_BUS_ERR_INVALID_PARAM;
    }

    if (strcmp(event_data->topic, EVENT_BUS_TOPIC_BT_PHONE_CONN) == 0)
    {
        gui_msg_publish("bt/phone_conn", event_data->data, event_data->data_len);
    }
    else if (strcmp(event_data->topic, EVENT_BUS_TOPIC_BT_PHONE_DISCONN) == 0)
    {
        gui_msg_publish("bt/phone_disconn", event_data->data, event_data->data_len);
    }
    else if (strcmp(event_data->topic, EVENT_BUS_TOPIC_BT_HEADPHONE_CONN) == 0)
    {
        gui_msg_publish("bt/headphone_conn", event_data->data, event_data->data_len);
    }
    else if (strcmp(event_data->topic, EVENT_BUS_TOPIC_BT_HEADPHONE_DISCONN) == 0)
    {
        gui_msg_publish("bt/headphone_disconn", event_data->data, event_data->data_len);

        /* Always try to flush pending headphone connect request, regardless of
         * which UI page is currently active. This ensures that after the old
         * earphone disconnects, the pending connection to the new earphone
         * (triggered from search page or headphone list page) proceeds. */
        try_flush_pending_headphone_connect();
    }
    else if (strcmp(event_data->topic, EVENT_BUS_TOPIC_BT_INQUIRY_RESULT) == 0)
    {
        gui_msg_publish("bt/inquiry_result", event_data->data, event_data->data_len);
    }
    else if (strcmp(event_data->topic, EVENT_BUS_TOPIC_BT_INQUIRY_CMPL) == 0)
    {
        gui_msg_publish("bt/inquiry_cmpl", event_data->data, event_data->data_len);
    }

    return EVENT_BUS_OK;
}

#endif

void bt_phone_list_note_design(gui_obj_t *obj, void *param)
{
    GUI_UNUSED(param);

    gui_list_note_t *note = (gui_list_note_t *)obj;
    uint16_t index = note->index;

    if (index != 0) { return; }

    /* Resolve data source */
    const char *phone_name_ascii = NULL;
    bool phone_used = false;

#ifndef _HONEYGUI_SIMULATOR_
    T_APP_BOND_DEVICE *phone_bond = &app_db.bond_device[0];
    if (!phone_bond->exist_addr_flag) { return; }
    phone_used = phone_bond->used;
#else
    phone_name_ascii = sim_phone_name;  /* "iPhone 15 Pro" */
    phone_used = false;                 /* Not Connected */
#endif

    /* Check if widget already exists by name */
    gui_obj_t *existing_status = gui_obj_get_handle((gui_obj_t *)note, "phone_status_label");
    gui_obj_t *existing_name = gui_obj_get_handle((gui_obj_t *)note, "phone_name_label");

    if (existing_status != NULL || existing_name != NULL)
    {
        if (existing_status != NULL)
        {
            phone_status_label = (gui_text_t *)existing_status;
            if (phone_used)
            {
                gui_text_content_set(phone_status_label, "Connected", 9);
                gui_text_color_set(phone_status_label, gui_rgb(76, 217, 100));
            }
            else
            {
                gui_text_content_set(phone_status_label, "Not Connected", 13);
                gui_text_color_set(phone_status_label, gui_rgb(102, 102, 102));
            }
        }
        if (existing_name != NULL) { phone_name_label = (gui_text_t *)existing_name; }
        return;
    }

    /* Create phone_item_bg */
    phone_item_bg = gui_rect_create((gui_obj_t *)note, "phone_item_bg", 24, 0, 362, 84, 12,
                                    gui_rgb(44, 44, 46));
    gui_obj_add_event_cb(phone_item_bg, (gui_event_cb_t)phone_item_bg_clicked_cb,
                         GUI_EVENT_TOUCH_CLICKED, NULL);
    gui_obj_add_event_cb(phone_item_bg, (gui_event_cb_t)phone_item_bg_long_pressed_cb,
                         GUI_EVENT_TOUCH_LONG, NULL);

    /* Create phone icon */
    phone_icon = gui_img_create_from_fs((gui_obj_t *)note, "phone_icon",
                                        "/app_control_center/smartphone_icon.bin",
                                        352, 28, 28, 28);

    /* Create phone_name_label */
    phone_name_label = gui_text_create((gui_obj_t *)note, "phone_name_label", 40, 19, 260, 40);
#ifndef _HONEYGUI_SIMULATOR_
    if (phone_bond->device_name_len > 0)
    {
        gui_text_encoding_set(phone_name_label, UTF_16);
        gui_text_set(phone_name_label, (char *)phone_bond->device_name,
                     GUI_FONT_SRC_BMP, gui_rgb(255, 255, 255),
                     phone_bond->device_name_len * 2, 40);
    }
    else
    {
        gui_text_set(phone_name_label, "Unknown Phone", GUI_FONT_SRC_BMP,
                     gui_rgb(255, 255, 255), 13, 40);
    }
#else
    gui_text_set(phone_name_label, (char *)phone_name_ascii, GUI_FONT_SRC_BMP,
                 gui_rgb(255, 255, 255), strlen(phone_name_ascii), 40);
#endif
    gui_text_type_set(phone_name_label,
                      "/font/NotoSansSC_Regular_size40_bits4_bitmap.bin", FONT_SRC_FILESYS);
    gui_text_mode_set(phone_name_label, LEFT);
    gui_text_extra_letter_spacing_set(phone_name_label, 0);
    gui_text_extra_line_spacing_set(phone_name_label, 0);
    gui_obj_show((gui_obj_t *)phone_name_label, true);

    /* Create phone_status_label */
    phone_status_label = gui_text_create((gui_obj_t *)note, "phone_status_label", 40, 52, 200, 32);
    if (phone_used)
    {
        gui_text_set(phone_status_label, "Connected", GUI_FONT_SRC_BMP,
                     gui_rgb(76, 217, 100), 9, 28);
    }
    else
    {
        gui_text_set(phone_status_label, "Not Connected", GUI_FONT_SRC_BMP,
                     gui_rgb(102, 102, 102), 13, 28);
    }
    gui_text_type_set(phone_status_label,
                      "/font/Inter_24pt_Regular_size28_bits4_bitmap.bin", FONT_SRC_FILESYS);
    gui_text_mode_set(phone_status_label, LEFT);
    gui_text_extra_letter_spacing_set(phone_status_label, 0);
    gui_text_extra_line_spacing_set(phone_status_label, 0);
    gui_obj_show((gui_obj_t *)phone_status_label, true);
}

void bt_headphone_list_note_design(gui_obj_t *obj, void *param)
{
    GUI_UNUSED(param);

    gui_list_note_t *note = (gui_list_note_t *)obj;
    uint16_t index = note->index;

    if (index >= 7) { return; }

    /* Resolve data source */
    const char *hp_name_ascii = NULL;
    bool hp_used = false;

#ifndef _HONEYGUI_SIMULATOR_
    T_APP_BOND_DEVICE *headphone_bond = &app_db.bond_device[index + 1];

    if (!headphone_bond->exist_addr_flag)
    {
        if (headphone_list != NULL)
        {
            uint8_t headphone_count = 0;
            for (int i = 1; i <= 7; i++)
            {
                if (app_db.bond_device[i].exist_addr_flag) { headphone_count++; }
            }
            gui_obj_show((gui_obj_t *)headphone_list, true);
            gui_list_set_note_num(headphone_list, headphone_count);
        }
        return;
    }
    hp_used = headphone_bond->used;
#else
    if (index >= SIM_HEADPHONE_COUNT) { return; }
    hp_name_ascii = sim_headphones[index].name;
    hp_used = sim_headphones[index].connected;
#endif

    /* Check if widget already exists */
    
    char name_buf[32];
    snprintf(name_buf, sizeof(name_buf), "headphones%d_status_label", index + 1);
    gui_obj_t *existing_status = gui_obj_get_handle((gui_obj_t *)note, name_buf);
    snprintf(name_buf, sizeof(name_buf), "headphones%d_name_label", index + 1);
    gui_obj_t *existing_name = gui_obj_get_handle((gui_obj_t *)note, name_buf);

    if (existing_status != NULL || existing_name != NULL)
    {
        if (existing_status != NULL)
        {
            if (hp_used)
            {
                gui_text_content_set((gui_text_t *)existing_status, "Connected", 9);
                gui_text_color_set((gui_text_t *)existing_status, gui_rgb(76, 217, 100));
            }
            else
            {
                gui_text_content_set((gui_text_t *)existing_status, "Not Connected", 13);
                gui_text_color_set((gui_text_t *)existing_status, gui_rgb(102, 102, 102));
            }
        }

        snprintf(name_buf, sizeof(name_buf), "headphones%d_icon", index + 1);
        gui_obj_t *existing_icon = gui_obj_get_handle((gui_obj_t *)note, name_buf);
        if (existing_icon != NULL)
        {
            gui_img_set_src((gui_img_t *)existing_icon,
                            hp_used ? "/app_control_center/headphones_icon_connected.bin"
                            : "/app_control_center/headphones_icon_disconnected.bin",
                            IMG_SRC_FILESYS);
        }
        return;
    }

    /* Create background */
    char bg_name[32];
    snprintf(bg_name, sizeof(bg_name), "headphones_item%d_bg", index + 1);
    gui_rounded_rect_t *headphone_item_bg = gui_rect_create((gui_obj_t *)note, bg_name,
                                                            24, 0, 362, 84, 12,
                                                            gui_rgb(44, 44, 46));

    /* Add click/long press events based on index */
    gui_event_cb_t click_cb = NULL;
    gui_event_cb_t long_cb = NULL;
    switch (index)
    {
    case 0: click_cb = (gui_event_cb_t)headphones_item1_bg_clicked_cb;
        long_cb = (gui_event_cb_t)headphones_item1_bg_long_pressed_cb; break;
    case 1: click_cb = (gui_event_cb_t)headphones_item2_bg_clicked_cb;
        long_cb = (gui_event_cb_t)headphones_item2_bg_long_pressed_cb; break;
    case 2: click_cb = (gui_event_cb_t)headphones_item3_bg_clicked_cb;
        long_cb = (gui_event_cb_t)headphones_item3_bg_long_pressed_cb; break;
    case 3: click_cb = (gui_event_cb_t)headphones_item4_bg_clicked_cb;
        long_cb = (gui_event_cb_t)headphones_item4_bg_long_pressed_cb; break;
    case 4: click_cb = (gui_event_cb_t)headphones_item5_bg_clicked_cb;
        long_cb = (gui_event_cb_t)headphones_item5_bg_long_pressed_cb; break;
    case 5: click_cb = (gui_event_cb_t)headphones_item6_bg_clicked_cb;
        long_cb = (gui_event_cb_t)headphones_item6_bg_long_pressed_cb; break;
    case 6: click_cb = (gui_event_cb_t)headphones_item7_bg_clicked_cb;
        long_cb = (gui_event_cb_t)headphones_item7_bg_long_pressed_cb; break;
    default: break;
    }
    if (click_cb) { gui_obj_add_event_cb(headphone_item_bg, click_cb, GUI_EVENT_TOUCH_CLICKED, NULL); }
    if (long_cb)  { gui_obj_add_event_cb(headphone_item_bg, long_cb,  GUI_EVENT_TOUCH_LONG,    NULL); }

    /* Create icon */
    char icon_name[32];
    snprintf(icon_name, sizeof(icon_name), "headphones%d_icon", index + 1);
    gui_img_create_from_fs((gui_obj_t *)note, icon_name,
                           hp_used ? "/app_control_center/headphones_icon_connected.bin"
                           : "/app_control_center/headphones_icon_disconnected.bin",
                           352, 28, 28, 28);

    /* Create name label */
    char name_label_name[32];
    snprintf(name_label_name, sizeof(name_label_name), "headphones%d_name_label", index + 1);
    gui_text_t *name_label = gui_text_create((gui_obj_t *)note, name_label_name, 40, 19, 260, 40);
#ifndef _HONEYGUI_SIMULATOR_
    if (headphone_bond->device_name_len > 0)
    {
        gui_text_encoding_set(name_label, UTF_16);
        gui_text_set(name_label, (char *)headphone_bond->device_name,
                     GUI_FONT_SRC_BMP, gui_rgb(255, 255, 255),
                     headphone_bond->device_name_len * 2, 40);
    }
    else
    {
        gui_text_set(name_label, "Unknown Headphone", GUI_FONT_SRC_BMP,
                     gui_rgb(255, 255, 255), 17, 40);
    }
#else
    gui_text_set(name_label, (char *)hp_name_ascii, GUI_FONT_SRC_BMP,
                 gui_rgb(255, 255, 255), strlen(hp_name_ascii), 40);
#endif
    gui_text_type_set(name_label,
                      "/font/NotoSansSC_Regular_size40_bits4_bitmap.bin", FONT_SRC_FILESYS);
    gui_text_mode_set(name_label, LEFT);
    gui_text_extra_letter_spacing_set(name_label, 0);
    gui_text_extra_line_spacing_set(name_label, 0);
    gui_obj_show((gui_obj_t *)name_label, true);

    /* Create status label */
    char status_label_name[32];
    snprintf(status_label_name, sizeof(status_label_name), "headphones%d_status_label", index + 1);
    gui_text_t *status_label = gui_text_create((gui_obj_t *)note, status_label_name, 40, 52, 200, 32);
    if (hp_used)
    {
        gui_text_set(status_label, "Connected", GUI_FONT_SRC_BMP,
                     gui_rgb(76, 217, 100), 9, 28);
    }
    else
    {
        gui_text_set(status_label, "Not Connected", GUI_FONT_SRC_BMP,
                     gui_rgb(102, 102, 102), 13, 28);
    }
    gui_text_type_set(status_label,
                      "/font/Inter_24pt_Regular_size28_bits4_bitmap.bin", FONT_SRC_FILESYS);
    gui_text_mode_set(status_label, LEFT);
    gui_text_extra_letter_spacing_set(status_label, 0);
    gui_text_extra_line_spacing_set(status_label, 0);
    gui_obj_show((gui_obj_t *)status_label, true);
}

void bt_search_list_note_design(gui_obj_t *obj, void *param)
{
    GUI_UNUSED(param);

    gui_list_note_t *note = (gui_list_note_t *)obj;
    uint16_t index = note->index;

    if (index >= MAX_SEARCH_DEVICES) { return; }

    T_SEARCH_RESULT *dev = &found_devices[index];

    /* Check if device has valid address */
    bool has_addr = false;
    for (int i = 0; i < 6; i++)
    {
        if (dev->bd_addr[i] != 0) { has_addr = true; break; }
    }
    if (!has_addr) { return; }

    // Create found_device_bg (hg_rect)
    gui_rounded_rect_t *device_bg = NULL;
    gui_text_t *name_label = NULL;
    gui_text_t *status_label = NULL;
    gui_img_t *icon = NULL;

    switch (index)
    {
    case 0:
        device_bg = gui_rect_create((gui_obj_t *)note, "found_device1_bg", 24, 0, 362, 84, 12,
                                    gui_rgb(44, 44, 46));
        found_device1_name = gui_text_create((gui_obj_t *)note, "found_device1_name", 40, 19, 260, 40);
        found_device1_status = gui_text_create((gui_obj_t *)note, "found_device1_status", 40, 52, 200, 32);
        icon = gui_img_create_from_fs((gui_obj_t *)note, "found_device1_icon",
                                      "/app_control_center/headphones_icon_disconnected.bin",
                                      352, 19, 18, 18);
        name_label = found_device1_name;
        status_label = found_device1_status;
        gui_obj_add_event_cb(device_bg, (gui_event_cb_t)found_device1_bg_clicked_cb,
                             GUI_EVENT_TOUCH_CLICKED, NULL);
        break;
    case 1:
        device_bg = gui_rect_create((gui_obj_t *)note, "found_device2_bg", 24, 0, 362, 84, 12,
                                    gui_rgb(44, 44, 46));
        found_device2_name = gui_text_create((gui_obj_t *)note, "found_device2_name", 40, 19, 260, 40);
        found_device2_status = gui_text_create((gui_obj_t *)note, "found_device2_status", 40, 52, 200, 32);
        icon = gui_img_create_from_fs((gui_obj_t *)note, "found_device2_icon",
                                      "/app_control_center/headphones_icon_disconnected.bin",
                                      352, 19, 18, 18);
        name_label = found_device2_name;
        status_label = found_device2_status;
        gui_obj_add_event_cb(device_bg, (gui_event_cb_t)found_device2_bg_clicked_cb,
                             GUI_EVENT_TOUCH_CLICKED, NULL);
        break;
    default:
        break;
    }

    // Set device name
    if (name_label != NULL)
    {
#ifdef _HONEYGUI_SIMULATOR_
        /* Simulator: use mock device name (JBL Tune 510BT / Bose QC45) */
        const char *sim_name = sim_search_names[index];
        gui_text_set(name_label, (char *)sim_name, GUI_FONT_SRC_BMP,
                     gui_rgb(255, 255, 255), strlen(sim_name), 40);
        gui_text_type_set(name_label,
                          "/font/Inter_24pt_Regular_size40_bits4_bitmap.bin",
                          FONT_SRC_FILESYS);
#else
        if (dev->nam_len > 0)
        {
            gui_text_encoding_set(name_label, UTF_16);
            gui_text_set(name_label, (char *)dev->device_name,
                         GUI_FONT_SRC_BMP, gui_rgb(255, 255, 255), dev->nam_len * 2, 40);
        }
        else
        {
            /* Show BD address if no device name */
            char *addr_str = search_addr_buffer[index];
            snprintf(addr_str, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
                     dev->bd_addr[5], dev->bd_addr[4], dev->bd_addr[3],
                     dev->bd_addr[2], dev->bd_addr[1], dev->bd_addr[0]);
            gui_text_set(name_label, addr_str, GUI_FONT_SRC_BMP,
                         gui_rgb(255, 255, 255), strlen(addr_str), 40);
            gui_text_type_set(name_label,
                              "/font/Inter_24pt_Regular_size40_bits4_bitmap.bin",
                              FONT_SRC_FILESYS);
        }
#endif
        gui_text_mode_set(name_label, LEFT);
        gui_text_extra_letter_spacing_set(name_label, 0);
        gui_text_extra_line_spacing_set(name_label, 0);
        gui_obj_show((gui_obj_t *)name_label, true);
    }

    // Set device status: Available
    if (status_label != NULL)
    {
        gui_text_set(status_label, "Available", GUI_FONT_SRC_BMP,
                     gui_rgb(102, 102, 102), 9, 28);
        gui_text_type_set(status_label,
                          "/font/Inter_24pt_Regular_size28_bits4_bitmap.bin",
                          FONT_SRC_FILESYS);
        gui_text_mode_set(status_label, LEFT);
        gui_text_extra_letter_spacing_set(status_label, 0);
        gui_text_extra_line_spacing_set(status_label, 0);
        gui_obj_show((gui_obj_t *)status_label, true);
    }

    // Set device icon
    if (icon != NULL)
    {
        gui_obj_show((gui_obj_t *)icon, true);
    }
}

void setting_list_note_design(gui_obj_t *obj, void *param)
{
    GUI_UNUSED(param);

    gui_list_note_t *note = (gui_list_note_t *)obj;
    uint16_t index = note->index;

    switch (index)
    {
    case 0:
        {
            // Create device_name_bg
            device_name_bg = gui_rect_create((gui_obj_t *)note, "device_name_bg", 24, 0, 362, 84, 12,
                                             gui_rgb(44, 44, 46));
            // Create device_name_label
            device_name_label = gui_text_create((gui_obj_t *)note, "device_name_label", 40, 6, 300, 28);
            gui_text_set((gui_text_t *)device_name_label, "Device Name", GUI_FONT_SRC_BMP,
                         gui_rgb(102, 102, 102), 11, 28);
            gui_text_type_set((gui_text_t *)device_name_label,
                              "/font/Inter_24pt_Regular_size28_bits4_bitmap.bin", FONT_SRC_FILESYS);
            gui_text_mode_set((gui_text_t *)device_name_label, LEFT);
            gui_text_extra_letter_spacing_set((gui_text_t *)device_name_label, 0);
            gui_text_extra_line_spacing_set((gui_text_t *)device_name_label, 0);
            gui_obj_show((gui_obj_t *)device_name_label, true);

            // Create device_name_value
            device_name_value = gui_text_create((gui_obj_t *)note, "device_name_value", 40, 30, 300, 40);
#ifndef _HONEYGUI_SIMULATOR_
            uint8_t name_len = strlen((char *)app_cfg_nv.device_name_legacy);
            if (name_len > 0)
            {
                gui_text_set((gui_text_t *)device_name_value, (char *)app_cfg_nv.device_name_legacy,
                             GUI_FONT_SRC_BMP, gui_rgb(255, 255, 255), name_len, 40);
            }
            else
#endif
            {
                gui_text_set((gui_text_t *)device_name_value, "Apple Watch S9", GUI_FONT_SRC_BMP,
                             gui_rgb(255, 255, 255), 14, 40);
            }
            gui_text_type_set((gui_text_t *)device_name_value,
                              "/font/Inter_24pt_Regular_size40_bits4_bitmap.bin", FONT_SRC_FILESYS);
            gui_text_mode_set((gui_text_t *)device_name_value, LEFT);
            gui_text_extra_letter_spacing_set((gui_text_t *)device_name_value, 0);
            gui_text_extra_line_spacing_set((gui_text_t *)device_name_value, 0);
            gui_obj_show((gui_obj_t *)device_name_value, true);
            break;
        }
    case 1:
        {
            // Create bt_address_bg
            bt_address_bg = gui_rect_create((gui_obj_t *)note, "bt_address_bg", 24, 0, 362, 84, 12,
                                            gui_rgb(44, 44, 46));
            // Create bt_address_label
            bt_address_label = gui_text_create((gui_obj_t *)note, "bt_address_label", 40, 6, 300, 28);
            gui_text_set((gui_text_t *)bt_address_label, "BT Address", GUI_FONT_SRC_BMP,
                         gui_rgb(102, 102, 102), 10, 28);
            gui_text_type_set((gui_text_t *)bt_address_label,
                              "/font/Inter_24pt_Regular_size28_bits4_bitmap.bin", FONT_SRC_FILESYS);
            gui_text_mode_set((gui_text_t *)bt_address_label, LEFT);
            gui_text_extra_letter_spacing_set((gui_text_t *)bt_address_label, 0);
            gui_text_extra_line_spacing_set((gui_text_t *)bt_address_label, 0);
            gui_obj_show((gui_obj_t *)bt_address_label, true);

            // Create bt_address_value
#ifndef _HONEYGUI_SIMULATOR_
            uint8_t bd_addr_local[6];
            gap_get_param(GAP_PARAM_BD_ADDR, bd_addr_local);
            snprintf(bt_addr_buffer, sizeof(bt_addr_buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
                     bd_addr_local[5], bd_addr_local[4], bd_addr_local[3],
                     bd_addr_local[2], bd_addr_local[1], bd_addr_local[0]);
#else
            snprintf(bt_addr_buffer, sizeof(bt_addr_buffer), "A4:B1:C2:D3:E4:F5");
#endif

            bt_address_value = gui_text_create((gui_obj_t *)note, "bt_address_value", 40, 30, 300, 40);
            gui_text_set((gui_text_t *)bt_address_value, bt_addr_buffer, GUI_FONT_SRC_BMP,
                         gui_rgb(255, 255, 255), strlen(bt_addr_buffer), 40);
            gui_text_type_set((gui_text_t *)bt_address_value,
                              "/font/Inter_24pt_Regular_size40_bits4_bitmap.bin", FONT_SRC_FILESYS);
            gui_text_mode_set((gui_text_t *)bt_address_value, LEFT);
            gui_text_extra_letter_spacing_set((gui_text_t *)bt_address_value, 0);
            gui_text_extra_line_spacing_set((gui_text_t *)bt_address_value, 0);
            gui_obj_show((gui_obj_t *)bt_address_value, true);
            break;
        }
    case 2:
        {
            // Create bt_version_bg
            gui_rect_create((gui_obj_t *)note, "bt_version_bg", 24, 0, 362, 84, 12,
                            gui_rgb(44, 44, 46));
            // Create bt_version_label
            gui_text_t *bt_version_label = gui_text_create((gui_obj_t *)note, "bt_version_label",
                                                           40, 6, 300, 28);
            gui_text_set((gui_text_t *)bt_version_label, "BT Version", GUI_FONT_SRC_BMP,
                         gui_rgb(102, 102, 102), 10, 28);
            gui_text_type_set((gui_text_t *)bt_version_label,
                              "/font/Inter_24pt_Regular_size28_bits4_bitmap.bin", FONT_SRC_FILESYS);
            gui_text_mode_set((gui_text_t *)bt_version_label, LEFT);
            gui_text_extra_letter_spacing_set((gui_text_t *)bt_version_label, 0);
            gui_text_extra_line_spacing_set((gui_text_t *)bt_version_label, 0);
            gui_obj_show((gui_obj_t *)bt_version_label, true);

            // Create bt_version_value
            gui_text_t *bt_version_value = gui_text_create((gui_obj_t *)note, "bt_version_value",
                                                           40, 30, 300, 40);
            gui_text_set((gui_text_t *)bt_version_value, "Bluetooth 5.3", GUI_FONT_SRC_BMP,
                         gui_rgb(255, 255, 255), 13, 40);
            gui_text_type_set((gui_text_t *)bt_version_value,
                              "/font/Inter_24pt_Regular_size40_bits4_bitmap.bin", FONT_SRC_FILESYS);
            gui_text_mode_set((gui_text_t *)bt_version_value, LEFT);
            gui_text_extra_letter_spacing_set((gui_text_t *)bt_version_value, 0);
            gui_text_extra_line_spacing_set((gui_text_t *)bt_version_value, 0);
            gui_obj_show((gui_obj_t *)bt_version_value, true);
            break;
        }
    case 3:
        {
            // Create wifi_ip_bg
            gui_rect_create((gui_obj_t *)note, "wifi_ip_bg", 24, 0, 362, 84, 12,
                            gui_rgb(44, 44, 46));
            // Create wifi_ip_label
            gui_text_t *wifi_ip_label = gui_text_create((gui_obj_t *)note, "wifi_ip_label",
                                                        40, 6, 300, 28);
            gui_text_set((gui_text_t *)wifi_ip_label, "Wi-Fi IP", GUI_FONT_SRC_BMP,
                         gui_rgb(102, 102, 102), 8, 28);
            gui_text_type_set((gui_text_t *)wifi_ip_label,
                              "/font/Inter_24pt_Regular_size28_bits4_bitmap.bin", FONT_SRC_FILESYS);
            gui_text_mode_set((gui_text_t *)wifi_ip_label, LEFT);
            gui_text_extra_letter_spacing_set((gui_text_t *)wifi_ip_label, 0);
            gui_text_extra_line_spacing_set((gui_text_t *)wifi_ip_label, 0);
            gui_obj_show((gui_obj_t *)wifi_ip_label, true);

            // Create wifi_ip_value
            gui_text_t *wifi_ip_value = gui_text_create((gui_obj_t *)note, "wifi_ip_value",
                                                        40, 30, 300, 40);
            gui_text_set((gui_text_t *)wifi_ip_value, "192.168.1.42", GUI_FONT_SRC_BMP,
                         gui_rgb(255, 255, 255), 12, 40);
            gui_text_type_set((gui_text_t *)wifi_ip_value,
                              "/font/Inter_24pt_Regular_size40_bits4_bitmap.bin", FONT_SRC_FILESYS);
            gui_text_mode_set((gui_text_t *)wifi_ip_value, LEFT);
            gui_text_extra_letter_spacing_set((gui_text_t *)wifi_ip_value, 0);
            gui_text_extra_line_spacing_set((gui_text_t *)wifi_ip_value, 0);
            gui_obj_show((gui_obj_t *)wifi_ip_value, true);
            break;
        }
    case 4:
        {
            // Create wifi_version_bg
            gui_rect_create((gui_obj_t *)note, "wifi_version_bg", 24, 0, 362, 84, 12,
                            gui_rgb(44, 44, 46));
            // Create wifi_version_label
            gui_text_t *wifi_version_label = gui_text_create((gui_obj_t *)note, "wifi_version_label",
                                                             40, 6, 300, 28);
            gui_text_set((gui_text_t *)wifi_version_label, "Wi-Fi Version", GUI_FONT_SRC_BMP,
                         gui_rgb(102, 102, 102), 13, 28);
            gui_text_type_set((gui_text_t *)wifi_version_label,
                              "/font/Inter_24pt_Regular_size28_bits4_bitmap.bin", FONT_SRC_FILESYS);
            gui_text_mode_set((gui_text_t *)wifi_version_label, LEFT);
            gui_text_extra_letter_spacing_set((gui_text_t *)wifi_version_label, 0);
            gui_text_extra_line_spacing_set((gui_text_t *)wifi_version_label, 0);
            gui_obj_show((gui_obj_t *)wifi_version_label, true);

            // Create wifi_version_value
            gui_text_t *wifi_version_value = gui_text_create((gui_obj_t *)note, "wifi_version_value",
                                                             40, 30, 300, 40);
            gui_text_set((gui_text_t *)wifi_version_value, "802.11ac (Wi-Fi 5)", GUI_FONT_SRC_BMP,
                         gui_rgb(255, 255, 255), 18, 40);
            gui_text_type_set((gui_text_t *)wifi_version_value,
                              "/font/Inter_24pt_Regular_size40_bits4_bitmap.bin", FONT_SRC_FILESYS);
            gui_text_mode_set((gui_text_t *)wifi_version_value, LEFT);
            gui_text_extra_letter_spacing_set((gui_text_t *)wifi_version_value, 0);
            gui_text_extra_line_spacing_set((gui_text_t *)wifi_version_value, 0);
            gui_obj_show((gui_obj_t *)wifi_version_value, true);
            break;
        }
    default:
        break;
    }
}

/**
 * @brief Timer callback for Bluetooth view - sync UI with BT toggle state
 * Called when entering app_control_centerBluetoothView
 */
void app_control_centerBluetoothView_timer_0_cb_impl(void)
{
    bool bt_enabled = bt_toggle_btn_get_state();

    if (bt_enabled)
    {
        /* BT is ON - show headphones entry and phone section */
        if (bt_headphones_entry_bg != NULL)
        {
            gui_obj_show((gui_obj_t *)bt_headphones_entry_bg, true);
        }
        if (headphones_entry_icon != NULL)
        {
            gui_obj_show((gui_obj_t *)headphones_entry_icon, true);
        }
        if (headphones_entry_label != NULL)
        {
            gui_obj_show((gui_obj_t *)headphones_entry_label, true);
        }
        if (phone_section_label != NULL)
        {
            gui_obj_show((gui_obj_t *)phone_section_label, true);
        }

        /* bt_list note structure:
         * index 0: phone item (show if phone bonded) */
        uint16_t note_count = 0;
#ifndef _HONEYGUI_SIMULATOR_
        if (app_db.bond_device[0].exist_addr_flag)
        {
            note_count = 1;
        }
#else
        /* Simulator: always show 1 phone item (iPhone 15 Pro) */
        note_count = 1;
#endif

        gui_obj_show((gui_obj_t *)bt_list, true);
        gui_list_set_note_num(bt_list, note_count);
    }
    else
    {
        /* BT is OFF - hide headphones entry and phone section */
        if (bt_headphones_entry_bg != NULL)
        {
            gui_obj_show((gui_obj_t *)bt_headphones_entry_bg, false);
        }
        if (headphones_entry_icon != NULL)
        {
            gui_obj_show((gui_obj_t *)headphones_entry_icon, false);
        }
        if (headphones_entry_label != NULL)
        {
            gui_obj_show((gui_obj_t *)headphones_entry_label, false);
        }
        if (phone_section_label != NULL)
        {
            gui_obj_show((gui_obj_t *)phone_section_label, false);
        }

        gui_obj_show((gui_obj_t *)bt_list, false);
        gui_list_set_note_num(bt_list, 0);
        gui_list_set_offset(bt_list, 0);
    }

    gui_fb_change();
}
