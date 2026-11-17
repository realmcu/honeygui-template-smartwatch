/**
 * USER-OWNED FILE - GENERATED ONCE BY HONEYGUI AUTOGEN.
 * This file is not overwritten after creation; edit it freely.
 */
#ifndef APP_PHONE_USER_H
#define APP_PHONE_USER_H

#include "../callbacks/app_phone_callbacks.h"
#include "../ui/app_phone_ui.h"
#ifndef _HONEYGUI_SIMULATOR_
#include "app_common_event.h"
#endif
/* Phone app events for GUI communication */
typedef enum
{
    PHONE_APP_INCOMING_CALL,     // Incoming call detected
    PHONE_APP_CALL_ANSWERED,    // Call answered (active)
    PHONE_APP_CALLER_ID_UPDATED, // Caller ID received from PBAP
    PHONE_APP_CALL_ENDED,       // Call ended (idle)
    PHONE_APP_VOLUME_CHANGED,   // Volume changed
} PHONE_APP_EVENT;

/**
 * @brief Unified entry point for phone app to GUI communication
 * @param event the phone event type
 * @param data optional event data
 * @param len data length
 */
void phone_app_to_gui(PHONE_APP_EVENT event, void *data, uint16_t len);

/* Dial key callbacks */
void dial_key_0_cb(void *obj, gui_event_t *e);
void dial_key_1_cb(void *obj, gui_event_t *e);
void dial_key_2_cb(void *obj, gui_event_t *e);
void dial_key_3_cb(void *obj, gui_event_t *e);
void dial_key_4_cb(void *obj, gui_event_t *e);
void dial_key_5_cb(void *obj, gui_event_t *e);
void dial_key_6_cb(void *obj, gui_event_t *e);
void dial_key_7_cb(void *obj, gui_event_t *e);
void dial_key_8_cb(void *obj, gui_event_t *e);
void dial_key_9_cb(void *obj, gui_event_t *e);
void dial_key_star_cb(void *obj, gui_event_t *e);
void dial_key_hash_cb(void *obj, gui_event_t *e);

/* Control callbacks */
void delete_key_pressed(void *obj, gui_event_t *e);
void mute_toggle_cb(void *obj, gui_event_t *e);
void volume_up_cb(void *obj, gui_event_t *e);
void volume_down_cb(void *obj, gui_event_t *e);

/* Timer/init callbacks */
void incoming_view_init_cb_impl(void);
void call_timer_tick_impl(void);
void incoming_ring_timer_cb_impl(void);

/* Message subscription callbacks */
void phone_update_incoming_caller_id(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void phone_update_incoming_number(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void phone_update_calling_label(gui_obj_t *obj, const char *topic, void *data, uint16_t len);

/* Call control callbacks */
void phone_outgoing_call_cb(void *obj, gui_event_t *e);
void phone_end_call_cb(void *obj, gui_event_t *e);
void phone_answer_call_cb(void *obj, gui_event_t *e);

/**
 * @brief Get the current dialed phone number
 * @return pointer to the phone number string
 */
const char *get_dialed_number(void);

/**
 * @brief Switch to dialer view and clear dialed number
 * This should be called when entering dialer view to ensure clean state
 */
void phone_switch_to_dialer_view(void);

/**
 * @brief Set current call number before switching to calling view
 * @param number the phone number to display
 */
void phone_set_current_call_number(const char *number);

#endif // APP_PHONE_USER_H