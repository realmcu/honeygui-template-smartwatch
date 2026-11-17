/**
 * USER-OWNED FILE - GENERATED ONCE BY HONEYGUI AUTOGEN.
 * This file is not overwritten after creation; edit it freely.
 */
#ifndef APP_MUSIC_PLAYER_USER_H
#define APP_MUSIC_PLAYER_USER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gui_api.h"

void music_toggle_play(void *obj, gui_event_t *e);
void music_next(void *obj, gui_event_t *e);
void music_prev(void *obj, gui_event_t *e);
void music_show_volume(void *obj, gui_event_t *e);
void music_close_volume(void *obj, gui_event_t *e);
void music_volume_up(void *obj, gui_event_t *e);
void music_volume_down(void *obj, gui_event_t *e);
void playlist_select_song_0(void *obj, gui_event_t *e);
void playlist_select_song_1(void *obj, gui_event_t *e);
void playlist_select_song_2(void *obj, gui_event_t *e);
void playlist_select_song_3(void *obj, gui_event_t *e);
void playlist_select_song_4(void *obj, gui_event_t *e);
void playlist_select_song_5(void *obj, gui_event_t *e);
void music_progress_timer_cb_impl(void);
void player_view_init_cb_impl(void);
void playlist_view_init_cb_impl(void);

#ifdef __cplusplus
}
#endif

#endif
