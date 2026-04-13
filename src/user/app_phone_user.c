#include "app_phone_user.h"
#include "../ui/app_phone_ui.h"
#include "gui_img.h"
#include "gui_text.h"
#include "gui_view.h"

#include <stdio.h>
#include <string.h>

#define PHONE_MAX_DIGITS 12
#define INCOMING_FRAME_COUNT 30

static char phone_number[PHONE_MAX_DIGITS + 1] = "";
static char call_timer_text[16] = "00:00";
static char phone_volume_text[4] = "5";
static int phone_number_len = 0;
static char current_call_number[32] = "Unknown";
static bool phone_muted = false;
static int phone_volume = 5;
static int elapsed_seconds = 0;
static int incoming_frame_index = 0;

static const char *incoming_default_number = "+1 (555) 123-4567";

static bool is_current_view(const char *view_name)
{
    gui_view_t *current_view = gui_view_get_current();

    return current_view != NULL && current_view->base.name != NULL && strcmp(current_view->base.name, view_name) == 0;
}

static void set_text_content(gui_text_t *text_obj, const char *text)
{
    if (!text_obj || !text)
    {
        return;
    }

    gui_text_content_set(text_obj, (char *)text, strlen(text));
}

static void set_current_call_number(const char *value)
{
    if (!value || value[0] == '\0')
    {
        value = "Unknown";
    }

    strncpy(current_call_number, value, sizeof(current_call_number) - 1);
    current_call_number[sizeof(current_call_number) - 1] = '\0';
}

static void refresh_current_call_number_from_dialer(void)
{
    if (phone_number_len > 0)
    {
        set_current_call_number(phone_number);
    }
    else
    {
        set_current_call_number("Unknown");
    }
}

static void clear_dialed_number(void)
{
    phone_number[0] = '\0';
    phone_number_len = 0;
}

static void update_dialer_number_display(void)
{
    const char *display_text = phone_number_len > 0 ? phone_number : " ";

    set_text_content((gui_text_t *)number_display_label, display_text);
}

static void update_calling_number_display(void)
{
    set_text_content((gui_text_t *)calling_number_label, current_call_number);
}

static void update_call_timer_display(void)
{
    snprintf(call_timer_text, sizeof(call_timer_text), "%02d:%02d", elapsed_seconds / 60, elapsed_seconds % 60);
    set_text_content((gui_text_t *)call_timer_label, call_timer_text);
}

static void update_mute_button_display(void)
{
    if (!phone_call_mute_btn)
    {
        return;
    }

    gui_img_set_src((gui_img_t *)phone_call_mute_btn,
                    phone_muted ? "/app_phone/mute_btn_active.bin" : "/app_phone/mute_btn_normal.bin",
                    IMG_SRC_FILESYS);
}

static void update_volume_display(void)
{
    snprintf(phone_volume_text, sizeof(phone_volume_text), "%d", phone_volume);
    set_text_content((gui_text_t *)volume_value_label, phone_volume_text);
}

static void update_incoming_ring_frame(void)
{
    char frame_path[80];

    if (!incoming_ring_animation_img)
    {
        return;
    }

    snprintf(frame_path,
             sizeof(frame_path),
             "/app_phone/incoming_ring_animation/frame_%02d.bin",
             incoming_frame_index);
    gui_img_set_src((gui_img_t *)incoming_ring_animation_img, frame_path, IMG_SRC_FILESYS);
}

static void sync_calling_view(void)
{
    update_calling_number_display();
    update_call_timer_display();
    update_mute_button_display();
    update_volume_display();
}

static void append_phone_digit(char key)
{
    if (phone_number_len >= PHONE_MAX_DIGITS)
    {
        return;
    }

    phone_number[phone_number_len++] = key;
    phone_number[phone_number_len] = '\0';
    refresh_current_call_number_from_dialer();
    update_dialer_number_display();
    gui_fb_change();
}

void dialer_view_init_cb_impl(void)
{
    clear_dialed_number();
    set_current_call_number("Unknown");
    update_dialer_number_display();
    gui_fb_change();
}

void calling_view_init_cb_impl(void)
{
    phone_muted = false;
    phone_volume = 5;
    elapsed_seconds = 0;
    sync_calling_view();
    gui_fb_change();
}

void incoming_view_init_cb_impl(void)
{
    clear_dialed_number();
    set_current_call_number(incoming_default_number);
    incoming_frame_index = 0;
    update_incoming_ring_frame();
    gui_fb_change();
}

void call_timer_tick_impl(void)
{
    if (!is_current_view("app_phoneCallingView"))
    {
        return;
    }

    elapsed_seconds++;
    update_call_timer_display();
    gui_fb_change();
}

void incoming_ring_timer_cb_impl(void)
{
    if (!is_current_view("app_phoneIncomingView"))
    {
        return;
    }

    incoming_frame_index = (incoming_frame_index + 1) % INCOMING_FRAME_COUNT;
    update_incoming_ring_frame();
    gui_fb_change();
}

void dial_key_0_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    append_phone_digit('0');
}

void dial_key_1_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    append_phone_digit('1');
}

void dial_key_2_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    append_phone_digit('2');
}

void dial_key_3_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    append_phone_digit('3');
}

void dial_key_4_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    append_phone_digit('4');
}

void dial_key_5_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    append_phone_digit('5');
}

void dial_key_6_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    append_phone_digit('6');
}

void dial_key_7_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    append_phone_digit('7');
}

void dial_key_8_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    append_phone_digit('8');
}

void dial_key_9_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    append_phone_digit('9');
}

void dial_key_star_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    append_phone_digit('*');
}

void dial_key_hash_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    append_phone_digit('#');
}

void delete_key_pressed(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    if (phone_number_len <= 0)
    {
        return;
    }

    phone_number[--phone_number_len] = '\0';
    refresh_current_call_number_from_dialer();
    update_dialer_number_display();
    gui_fb_change();
}

void mute_toggle_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    phone_muted = !phone_muted;
    update_mute_button_display();
    gui_fb_change();
}

void volume_up_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    if (phone_volume >= 10)
    {
        return;
    }

    phone_volume++;
    update_volume_display();
    gui_fb_change();
}

void volume_down_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);

    if (phone_volume <= 0)
    {
        return;
    }

    phone_volume--;
    update_volume_display();
    gui_fb_change();
}