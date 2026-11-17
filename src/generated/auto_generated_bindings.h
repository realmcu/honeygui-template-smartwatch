/**
 * AUTO-GENERATED FILE. DO NOT EDIT MANUALLY.
 * Generated from HoneyGUI HML source files by autogen.
 */
#ifndef AUTO_GENERATED_BINDINGS_H
#define AUTO_GENERATED_BINDINGS_H

#include <stdint.h>
#include "gui_api.h"
#include "gui_text.h"
#include "gui_obj_focus.h"

void app_music_play_next(void *obj, gui_event_t *e);
void app_music_play_prev(void *obj, gui_event_t *e);
void bluetooth_search_devices(void *obj, gui_event_t *e);
void click_button_l(void *obj, gui_event_t *e);
void click_button_page_all_timers(void *obj, gui_event_t *e);
void click_button_play_stop(void *obj, gui_event_t *e);
void click_button_r(void *obj, gui_event_t *e);
void delete_key_pressed(void *obj, gui_event_t *e);
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
void dial_key_hash_cb(void *obj, gui_event_t *e);
void dial_key_star_cb(void *obj, gui_event_t *e);
void file_download_back(void *obj, gui_event_t *e);
void file_download_start(void *obj, gui_event_t *e);
void file_key_confirm(void *obj, gui_event_t *e);
void file_key_delete(void *obj, gui_event_t *e);
void file_key_dot_press(void *obj, gui_event_t *e);
void file_key_num(void *obj, gui_event_t *e);
void file_upload_back(void *obj, gui_event_t *e);
void file_upload_start(void *obj, gui_event_t *e);
void hangup_reset(void *obj, gui_event_t *e);
void headphone1_connect(void *obj, gui_event_t *e);
void headphone1_linkback_and_disconnect(void *obj, gui_event_t *e);
void headphone1_remove_paired_device(void *obj, gui_event_t *e);
void headphone2_connect(void *obj, gui_event_t *e);
void headphone2_linkback_and_disconnect(void *obj, gui_event_t *e);
void headphone2_remove_paired_device(void *obj, gui_event_t *e);
void headphone3_linkback_and_disconnect(void *obj, gui_event_t *e);
void headphone3_remove_paired_device(void *obj, gui_event_t *e);
void headphone4_linkback_and_disconnect(void *obj, gui_event_t *e);
void headphone4_remove_paired_device(void *obj, gui_event_t *e);
void headphone5_linkback_and_disconnect(void *obj, gui_event_t *e);
void headphone5_remove_paired_device(void *obj, gui_event_t *e);
void headphone6_linkback_and_disconnect(void *obj, gui_event_t *e);
void headphone6_remove_paired_device(void *obj, gui_event_t *e);
void headphone7_linkback_and_disconnect(void *obj, gui_event_t *e);
void headphone7_remove_paired_device(void *obj, gui_event_t *e);
void intercom_connect_dev(void *obj, gui_event_t *e);
void intercom_disconnect(void *obj, gui_event_t *e);
void intercom_update_connect_status(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void intercom_update_receive_status(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void intercom_update_scan_result(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void intercom_update_user_name(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void iperf_download_back(void *obj, gui_event_t *e);
void iperf_download_start(void *obj, gui_event_t *e);
void iperf_key_confirm(void *obj, gui_event_t *e);
void iperf_key_delete(void *obj, gui_event_t *e);
void iperf_key_dot_press(void *obj, gui_event_t *e);
void iperf_key_num(void *obj, gui_event_t *e);
void iperf_upload_back(void *obj, gui_event_t *e);
void iperf_upload_start(void *obj, gui_event_t *e);
void music_close_volume(void *obj, gui_event_t *e);
void music_next(void *obj, gui_event_t *e);
void music_prev(void *obj, gui_event_t *e);
void music_show_volume(void *obj, gui_event_t *e);
void music_toggle_play(void *obj, gui_event_t *e);
void music_volume_down(void *obj, gui_event_t *e);
void music_volume_up(void *obj, gui_event_t *e);
void mute_btn_off(void *obj, gui_event_t *e);
void mute_btn_on(void *obj, gui_event_t *e);
void mute_toggle_cb(void *obj, gui_event_t *e);
void ota_reset_to_ready(void *obj, gui_event_t *e);
void ota_retry(void *obj, gui_event_t *e);
void phone_answer_call_cb(void *obj, gui_event_t *e);
void phone_end_call_cb(void *obj, gui_event_t *e);
void phone_linkback_and_disconnect(void *obj, gui_event_t *e);
void phone_outgoing_call_cb(void *obj, gui_event_t *e);
void phone_remove_paired_device(void *obj, gui_event_t *e);
void phone_update_calling_label(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void phone_update_incoming_caller_id(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void phone_update_incoming_number(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void playlist_select_song_0(void *obj, gui_event_t *e);
void playlist_select_song_1(void *obj, gui_event_t *e);
void playlist_select_song_2(void *obj, gui_event_t *e);
void playlist_select_song_3(void *obj, gui_event_t *e);
void playlist_select_song_4(void *obj, gui_event_t *e);
void playlist_select_song_5(void *obj, gui_event_t *e);
void recording_file_0_selected(void *obj, gui_event_t *e);
void recording_file_1_selected(void *obj, gui_event_t *e);
void recording_file_2_selected(void *obj, gui_event_t *e);
void recording_file_3_selected(void *obj, gui_event_t *e);
void recording_file_4_selected(void *obj, gui_event_t *e);
void remove_paired_device_cancel(void *obj, gui_event_t *e);
void remove_paired_device_confirm(void *obj, gui_event_t *e);
void talk_btn_press(void *obj, gui_event_t *e);
void talk_btn_release(void *obj, gui_event_t *e);
void update_headphone_list(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void update_phone_list(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void update_search_list(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void video_call_start(void *obj, gui_event_t *e);
void video_call_update_stream(gui_obj_t *obj, const char *topic, void *data, uint16_t len);
void volume_down_cb(void *obj, gui_event_t *e);
void volume_up_cb(void *obj, gui_event_t *e);

#endif // AUTO_GENERATED_BINDINGS_H
