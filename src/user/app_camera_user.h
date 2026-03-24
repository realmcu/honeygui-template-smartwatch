#ifndef APP_CAMERA_USER_H
#define APP_CAMERA_USER_H

#include "../callbacks/app_camera_callbacks.h"
#include "../ui/app_camera_ui.h"

/**
 * 用户自定义头文件
 * 此文件只生成一次，可自由修改
 */

// 在此添加自定义声明
void app_camera_enter_video_camera(void *obj, gui_event_t *e);
void app_camera_exit_video_camera(void *obj, gui_event_t *e);
void app_camera_update_video_image(gui_obj_t *obj, const char *topic, void *data, uint16_t len);

#endif // APP_CAMERA_USER_H
