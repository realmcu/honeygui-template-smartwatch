#include "app_camera_user.h"

/**
 * 用户自定义实现
 * 此文件只生成一次，可自由修改
 */

// 在此添加自定义实现

/***
 * Template function
 * Distinguish development environments
 */
void app_camera_enter_video_camera(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    char *ip_addr = "192.168.39.1";
    void wifi_gui_msg_handler(uint32_t type, void *ip_addr);
    wifi_gui_msg_handler(0, ip_addr);
#endif
}

void app_camera_exit_video_camera(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    void wifi_gui_msg_handler(uint32_t type, void *ip_addr);
    wifi_gui_msg_handler(1, NULL);
#endif
}

void app_camera_update_video_image(gui_obj_t *obj, const char *topic, void *data, uint16_t len)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(topic);
    GUI_UNUSED(data);
    GUI_UNUSED(len);
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    gui_img_set_src((gui_img_t *)obj, data, IMG_SRC_MEMADDR);
    gui_img_set_pos((gui_img_t *)obj, 0, 0);

    gui_fb_change();
#endif
}
