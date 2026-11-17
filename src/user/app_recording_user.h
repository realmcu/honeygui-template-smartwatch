/**
 * USER-OWNED FILE - GENERATED ONCE BY HONEYGUI AUTOGEN.
 * This file is not overwritten after creation; edit it freely.
 */
#ifndef APP_RECORDING_USER_H
#define APP_RECORDING_USER_H

#include <stdbool.h>
#include <stdint.h>
#include "def_event.h"


void recording_start(void *obj, gui_event_t *e);
void recording_stop(void *obj, gui_event_t *e);
void playback_play(void *obj, gui_event_t *e);
void playback_pause(void *obj, gui_event_t *e);

void recording_file_0_selected(void *obj, gui_event_t *e);
void recording_file_1_selected(void *obj, gui_event_t *e);
void recording_file_2_selected(void *obj, gui_event_t *e);
void recording_file_3_selected(void *obj, gui_event_t *e);
void recording_file_4_selected(void *obj, gui_event_t *e);

uint16_t recording_list_count(void);
const char *recording_list_name_at(int index);
const char *recording_list_duration_at(int index);

void recording_main_init_cb_impl(void);
void recording_files_init_cb_impl(void);
void recording_playback_init_cb_impl(void);
void recording_timer_tick_impl(void);
void recording_waveform_timer_cb_impl(void);
void playback_timer_tick_impl(void);

#endif // APP_RECORDING_USER_H
