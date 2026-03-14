#include "app_heart_callbacks.h"
#include "../ui/app_heart_ui.h"
#include "../user/app_heart_user.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// 时间字符串全局变量（在 UI 文件中定义）
extern char hg_time_label_heart_time_str[10];

// 定时动画计数器
uint16_t hg_image_1769146380658_kvde_timer_cnt = 0;
uint16_t app_heart_circel0_timer_cnt = 0;

// 事件回调函数实现

void app_heart_window_key_0_cb(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    // 检查按键名
    if (strcmp(e->indev_name, "Home") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "SmartWatchTemplateMainView", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_ANIMATION_FADE);
    }
    else if (strcmp(e->indev_name, "Menu") == 0)
    {
        gui_view_switch_direct(gui_view_get_current(), "app_menu_view", SWITCH_OUT_NONE_ANIMATION, SWITCH_IN_ANIMATION_FADE);
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
    
    sprintf(hg_time_label_heart_time_str, "%02d:%02d", t->tm_hour, t->tm_min);
    
    gui_text_content_set((gui_text_t *)hg_time_label_heart, hg_time_label_heart_time_str, strlen(hg_time_label_heart_time_str));
}

// 预设定时器回调函数

/**
 * 定时动画 1
 * 组件: hg_image_1769146380658_kvde
 * 模式: 预设动作（多段动画）
 * 段数: 1
 */
void hg_image_1769146380658_kvde_timer_0_cb(void *obj)
{
    gui_obj_t *target = (gui_obj_t *)obj;
    const uint16_t total_cnt_max = 10;
    
    const uint16_t seg0_start = 0;
    const uint16_t seg0_end = 10;
    
    hg_image_1769146380658_kvde_timer_cnt++;
    
    // 段 1: 1000ms, 1 个动作
    if (hg_image_1769146380658_kvde_timer_cnt > seg0_start && hg_image_1769146380658_kvde_timer_cnt <= seg0_end) {
        uint16_t seg_cnt = hg_image_1769146380658_kvde_timer_cnt - seg0_start;
        const uint16_t seg_cnt_max = seg0_end - seg0_start;
        
            // 调整缩放: (1, 1) -> (1.3, 1.3)
            const float zoom_x_origin = 1;
            const float zoom_x_target = 1.3;
            const float zoom_y_origin = 1;
            const float zoom_y_target = 1.3;
            float zoom_x_cur = zoom_x_origin + (zoom_x_target - zoom_x_origin) * seg_cnt / seg_cnt_max;
            float zoom_y_cur = zoom_y_origin + (zoom_y_target - zoom_y_origin) * seg_cnt / seg_cnt_max;
            gui_img_scale((gui_img_t *)target, zoom_x_cur, zoom_y_cur);
            
    }
    
    if (hg_image_1769146380658_kvde_timer_cnt >= total_cnt_max) {
        hg_image_1769146380658_kvde_timer_cnt = 0; // 重置计数器，继续循环
    }
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
