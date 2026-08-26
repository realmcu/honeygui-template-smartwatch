#define APP_HEART_CALLBACKS_H_IMPLEMENTATION
#include "app_heart_callbacks.h"
#undef APP_HEART_CALLBACKS_H_IMPLEMENTATION
#include "../ui/app_heart_ui.h"
#include "../user/app_heart_user.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Time string global variables (defined in UI file)
extern char hg_time_label_heart_time_str[10];

// Internal preset animation clock state (real elapsed time, not callback counts)
static uint32_t hg_image_1769146380658_kvde_timer_start_ms = 0;
static bool hg_image_1769146380658_kvde_timer_started = false;
static uint32_t hg_image_1769146380658_kvde_timer_prev_elapsed_ms = 0;

void hg_image_1769146380658_kvde_preset_animation_reset(void)
{
    hg_image_1769146380658_kvde_timer_start_ms = 0;
    hg_image_1769146380658_kvde_timer_started = false;
    hg_image_1769146380658_kvde_timer_prev_elapsed_ms = 0;
}

// Deprecated animation counter (restart request flag, not a frame counter)
uint16_t hg_image_1769146380658_kvde_timer_cnt = 0;
uint16_t app_heart_circel0_timer_cnt = 0;

// Event callback function implementations

void app_heart_window_key_0_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    // Check key name
    if (strcmp(e->indev_name, "Home") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "SmartWatchTemplateMainView",
                               SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_ANIMATION_FADE);
    }
    else if (strcmp(e->indev_name, "Menu") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "app_menu_view", SWITCH_OUT_NONE_ANIMATION,
                               SWITCH_IN_ANIMATION_FADE);
    }
}

void hg_time_label_heart_time_update_cb(void *p)
{
    GUI_UNUSED(p);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t == NULL)
    {
        return;
    }

    snprintf(hg_time_label_heart_time_str, sizeof(hg_time_label_heart_time_str), "%02d:%02d",
             t->tm_hour, t->tm_min);

    gui_text_content_set((gui_text_t *)hg_time_label_heart, hg_time_label_heart_time_str,
                         strlen(hg_time_label_heart_time_str));
}

// Preset timer callback functions

/**
 * 定时动画 1
 * Component: hg_image_1769146380658_kvde
 * Mode: Preset actions, driven by real elapsed time (gui_ms_get)
 * Timeline: 1 segment(s), 1000 ms total, looping
 */
void hg_image_1769146380658_kvde_timer_0_cb(void *obj)
{
    gui_obj_t *target = (gui_obj_t *)obj;

    // Timeline boundaries in real milliseconds
    const uint32_t total_duration_ms = 1000u;
    const uint32_t seg0_start_ms = 0u;
    const uint32_t seg0_end_ms = 1000u;

    // Deprecated: <id>_timer_cnt = 0 from user code requests a restart
    if (hg_image_1769146380658_kvde_timer_cnt == 0)
    {
        hg_image_1769146380658_kvde_preset_animation_reset();
    }
    hg_image_1769146380658_kvde_timer_cnt = 1;

    uint32_t now_ms = gui_ms_get();

    // The time origin is established by the first callback of a run
    if (!hg_image_1769146380658_kvde_timer_started)
    {
        hg_image_1769146380658_kvde_timer_started = true;
        hg_image_1769146380658_kvde_timer_start_ms = now_ms;
        hg_image_1769146380658_kvde_timer_prev_elapsed_ms = 0;
    }

    // Unsigned subtraction stays correct across uint32_t clock wrap
    uint32_t total_elapsed_ms = now_ms - hg_image_1769146380658_kvde_timer_start_ms;
    hg_image_1769146380658_kvde_timer_prev_elapsed_ms = total_elapsed_ms;

    // Loop against the original origin so frame error cannot accumulate
    uint32_t timeline_ms = total_elapsed_ms % total_duration_ms;

    // Sampled state of the segment the timeline is currently inside
    // Segment 1: 1000 ms
    float progress = (float)(timeline_ms - seg0_start_ms) / (float)(seg0_end_ms - seg0_start_ms);
    if (progress > 1.0f)
    {
        progress = 1.0f;
    }
    // Adjust scale: (1, 1) -> (1.3, 1.3)
    const float zoom_x_origin = 1;
    const float zoom_x_target = 1.3;
    const float zoom_y_origin = 1;
    const float zoom_y_target = 1.3;
    float zoom_x_cur = zoom_x_origin + (zoom_x_target - zoom_x_origin) * progress;
    float zoom_y_cur = zoom_y_origin + (zoom_y_target - zoom_y_origin) * progress;
    gui_img_scale((gui_img_t *)target, zoom_x_cur, zoom_y_cur);


}


/* @protected start custom_functions */
// 自定义函数

void app_heart_circel0_timer_0_cb(void *obj)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(app_heart_circel0_timer_cnt);
    if (list_heart->offset > -251)
    {
        gui_circle_set_color(app_heart_circel0, gui_rgb(255, 255, 255));
        gui_circle_set_color(app_heart_circel1, gui_rgb(66, 62, 62));
    }
    else
    {
        gui_circle_set_color(app_heart_circel1, gui_rgb(255, 255, 255));
        gui_circle_set_color(app_heart_circel0, gui_rgb(66, 62, 62));
    }
}
/* @protected end custom_functions */
