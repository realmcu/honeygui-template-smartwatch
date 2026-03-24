#include "app_music_user.h"
#ifndef _HONEYGUI_SIMULATOR_
#include "app_task.h"
#include "app_mmi.h"
#include "module_playlist.h"
#endif
/**
 * 用户自定义实现
 * 此文件只生成一次，可自由修改
 */

// 在此添加自定义实现

void app_music_play_next(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    T_IO_MSG play_msg;
    play_msg.type = IO_MSG_TYPE_WRISTBNAD;
    play_msg.subtype = IO_MSG_MMI;
    play_msg.u.param = MMI_AV_FWD;
    app_send_msg_to_apptask(&play_msg);
#endif
}

void app_music_play_prev(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    T_IO_MSG play_msg;
    play_msg.type = IO_MSG_TYPE_WRISTBNAD;
    play_msg.subtype = IO_MSG_MMI;
    play_msg.u.param = MMI_AV_BWD;
    app_send_msg_to_apptask(&play_msg);
#endif
}

void app_music_play(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    T_IO_MSG play_msg;
    play_msg.type = IO_MSG_TYPE_WRISTBNAD;
    play_msg.subtype = IO_MSG_PLAY_BY_NAME;
    play_msg.u.param = (uint32_t)(get_cur_play_header_info());
    app_send_msg_to_apptask(&play_msg);
#endif
}

void app_music_pause(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
#ifdef _HONEYGUI_SIMULATOR_
    // TODO
#else
    T_IO_MSG play_msg;
    play_msg.type = IO_MSG_TYPE_WRISTBNAD;
    play_msg.subtype = IO_MSG_MMI;
    play_msg.u.param = MMI_AV_PLAY_PAUSE;
    app_send_msg_to_apptask(&play_msg);
#endif
}
