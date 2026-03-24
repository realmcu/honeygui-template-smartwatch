#ifndef APP_MUSIC_USER_H
#define APP_MUSIC_USER_H

#include "../callbacks/app_music_callbacks.h"
#include "../ui/app_music_ui.h"

/**
 * 用户自定义头文件
 * 此文件只生成一次，可自由修改
 */

// 在此添加自定义声明
void app_music_play_next(void *obj, gui_event_t *e);
void app_music_play_prev(void *obj, gui_event_t *e);
void app_music_play(void *obj, gui_event_t *e);
void app_music_pause(void *obj, gui_event_t *e);


#endif // APP_MUSIC_USER_H
