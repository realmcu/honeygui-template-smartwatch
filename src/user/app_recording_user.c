#include "app_recording_user.h"
#include "app_recording_ui.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "gui_obj.h"
#include "gui_view.h"
#include "gui_text.h"
#include "gui_arc.h"
#include "gui_list.h"


#define MAX_RECORDING_FILES 5
#define FILES_VIEW_REFRESH_INTERVAL_MS 20
#define FILES_VIEW_REFRESH_RETRY_COUNT 8
#define WAVEFORM_FRAME_COUNT 30

typedef struct recording_file_entry
{
	char name[32];
	int duration_seconds;
} recording_file_entry_t;

static recording_file_entry_t recording_files[MAX_RECORDING_FILES];
static int recording_file_count = 0;
static int next_recording_number = 1;
static int selected_recording_index = -1;
static char recording_timer_text[8] = "00:00";
static char recording_file_duration_texts[MAX_RECORDING_FILES][8];
static char playback_current_time_text[8] = "00:00";
static char playback_total_time_text[32] = "/ 00:00";

static bool recording_active = false;
static int recording_elapsed_seconds = 0;
static char saved_recording_message[48] = "";
static int waveform_frame_index = 0;
static int waveform_tick_divider = 0;
static int recording_breath_phase = 0;
static uint8_t recording_status_opacity = 255;

static bool playback_active = false;
static int playback_elapsed_ticks = 0;
static int files_view_refresh_retries = 0;

static void set_text_content(gui_text_t *text_obj, const char *text)
{
	if (!text_obj || !text)
	{
		return;
	}

	gui_text_content_set(text_obj, (char *)text, strlen(text));
}

static void format_time_text(int seconds, char *buffer, size_t buffer_size)
{
	if (!buffer || buffer_size == 0)
	{
		return;
	}

	snprintf(buffer, buffer_size, "%02d:%02d", seconds / 60, seconds % 60);
}

static gui_text_t *get_recording_file_name_label(int index)
{
	switch (index)
	{
	case 0:
		return recording_file_name_0;
	case 1:
		return recording_file_name_1;
	case 2:
		return recording_file_name_2;
	case 3:
		return recording_file_name_3;
	case 4:
		return recording_file_name_4;
	default:
		return NULL;
	}
}

static gui_text_t *get_recording_file_duration_label(int index)
{
	switch (index)
	{
	case 0:
		return recording_file_duration_0;
	case 1:
		return recording_file_duration_1;
	case 2:
		return recording_file_duration_2;
	case 3:
		return recording_file_duration_3;
	case 4:
		return recording_file_duration_4;
	default:
		return NULL;
	}
}

static void reset_recording_files_view_handles(void)
{
	recording_file_name_0 = NULL;
	recording_file_name_1 = NULL;
	recording_file_name_2 = NULL;
	recording_file_name_3 = NULL;
	recording_file_name_4 = NULL;
	recording_file_duration_0 = NULL;
	recording_file_duration_1 = NULL;
	recording_file_duration_2 = NULL;
	recording_file_duration_3 = NULL;
	recording_file_duration_4 = NULL;
}

static const recording_file_entry_t *get_selected_recording(void)
{
	if (selected_recording_index < 0 || selected_recording_index >= recording_file_count)
	{
		return NULL;
	}

	return &recording_files[selected_recording_index];
}

static bool is_current_view(const char *view_name)
{
	gui_view_t *current_view = gui_view_get_current();

	return current_view != NULL && current_view->base.name != NULL && strcmp(current_view->base.name, view_name) == 0;
}

static void refresh_recording_files_list_layout(void)
{
	if (!recording_files_list || !recording_files_empty_label)
	{
		return;
	}

	gui_list_set_note_num((gui_list_t *)recording_files_list, 0);
	gui_list_set_note_num((gui_list_t *)recording_files_list, (uint16_t)recording_file_count);
	gui_list_set_offset((gui_list_t *)recording_files_list, 0);
	gui_obj_hidden((gui_obj_t *)recording_files_list, recording_file_count == 0);
	gui_obj_hidden((gui_obj_t *)recording_files_empty_label, recording_file_count != 0);
}

static bool sync_recording_files_view_labels(void)
{
	int visible_count;
	int index;

	if (!is_current_view("app_recordingFilesView"))
	{
		return false;
	}

	if (!recording_files_list || !recording_files_empty_label)
	{
		return false;
	}

	if (recording_file_count <= 0)
	{
		return true;
	}

	visible_count = recording_file_count < MAX_RECORDING_FILES ? recording_file_count : MAX_RECORDING_FILES;
	for (index = 0; index < visible_count; ++index)
	{
		gui_text_t *name_label = get_recording_file_name_label(index);
		gui_text_t *duration_label = get_recording_file_duration_label(index);

		if (!name_label || !duration_label)
		{
			return false;
		}

		format_time_text(recording_files[index].duration_seconds,
						 recording_file_duration_texts[index],
						 sizeof(recording_file_duration_texts[index]));
		set_text_content(name_label, recording_files[index].name);
		set_text_content(duration_label, recording_file_duration_texts[index]);
	}

	return true;
}

static void recording_files_refresh_timer_cb(void *obj)
{
	bool synced = sync_recording_files_view_labels();

	if (synced || files_view_refresh_retries <= 1 || !is_current_view("app_recordingFilesView"))
	{
		gui_obj_stop_timer((gui_obj_t *)obj);
		files_view_refresh_retries = 0;
		gui_fb_change();
		return;
	}

	files_view_refresh_retries--;
	gui_fb_change();
}

static void schedule_recording_files_view_refresh(void)
{
	
}

static void update_recording_timer_display(void)
{
	format_time_text(recording_elapsed_seconds, recording_timer_text, sizeof(recording_timer_text));
	set_text_content((gui_text_t *)recording_timer_label, recording_timer_text);
}

static void update_recording_status_display(void)
{
	if (!recording_status_label)
	{
		return;
	}

	if (recording_active)
	{
		set_text_content((gui_text_t *)recording_status_label, "Recording...");
		((gui_obj_t *)recording_status_label)->opacity_value = recording_status_opacity;
		return;
	}

	((gui_obj_t *)recording_status_label)->opacity_value = 255;
	set_text_content((gui_text_t *)recording_status_label,
					 saved_recording_message[0] != '\0' ? saved_recording_message : "Ready");
}

static void update_waveform_frame_display(void)
{
	char frame_path[96];

	if (!recording_waveform_image)
	{
		return;
	}

	snprintf(frame_path,
			 sizeof(frame_path),
			 "/app_recording/waveform/%s/frame_%02d.bin",
			 recording_active ? "active" : "inactive",
			 waveform_frame_index);
	gui_img_set_src((gui_img_t *)recording_waveform_image, frame_path, IMG_SRC_FILESYS);
}

static void update_files_view_display(void)
{
	if (!is_current_view("app_recordingFilesView"))
	{
		return;
	}

	refresh_recording_files_list_layout();
	(void)sync_recording_files_view_labels();
}

static void update_playback_progress_display(void)
{
	const recording_file_entry_t *selected_recording = get_selected_recording();
	int playback_current_seconds = playback_elapsed_ticks / 10;
	float progress = 0.0f;

	if (!selected_recording)
	{
		set_text_content((gui_text_t *)playback_file_name_label, "");
		strcpy(playback_current_time_text, "00:00");
		strcpy(playback_total_time_text, "/ 00:00");
		set_text_content((gui_text_t *)playback_current_time_label, playback_current_time_text);
		set_text_content((gui_text_t *)playback_total_time_label, playback_total_time_text);
		if (playback_progress_fg)
		{
			gui_arc_set_end_angle((gui_arc_t *)playback_progress_fg, -90.0f);
		}
		return;
	}

	if (selected_recording->duration_seconds > 0)
	{
		progress = (float)playback_elapsed_ticks / ((float)selected_recording->duration_seconds * 10.0f);
		if (progress < 0.0f)
		{
			progress = 0.0f;
		}
		if (progress > 1.0f)
		{
			progress = 1.0f;
		}
	}

	format_time_text(playback_current_seconds, playback_current_time_text, sizeof(playback_current_time_text));
	snprintf(playback_total_time_text,
			 sizeof(playback_total_time_text),
			 "/ %02d:%02d",
			 selected_recording->duration_seconds / 60,
			 selected_recording->duration_seconds % 60);

	set_text_content((gui_text_t *)playback_file_name_label, selected_recording->name);
	set_text_content((gui_text_t *)playback_current_time_label, playback_current_time_text);
	set_text_content((gui_text_t *)playback_total_time_label, playback_total_time_text);

	if (playback_progress_fg)
	{
		gui_arc_set_end_angle((gui_arc_t *)playback_progress_fg, -90.0f + 360.0f * progress);
	}
}

static void append_recording_file(int duration_seconds, char *saved_name_buffer, size_t buffer_size)
{
	int target_index = recording_file_count;
	recording_file_entry_t *entry = NULL;

	if (duration_seconds <= 0)
	{
		if (saved_name_buffer && buffer_size > 0)
		{
			saved_name_buffer[0] = '\0';
		}
		return;
	}

	if (recording_file_count >= MAX_RECORDING_FILES)
	{
		memmove(&recording_files[0], &recording_files[1], sizeof(recording_files[0]) * (MAX_RECORDING_FILES - 1));
		target_index = MAX_RECORDING_FILES - 1;
		recording_file_count = MAX_RECORDING_FILES;
	}
	else
	{
		recording_file_count++;
	}

	entry = &recording_files[target_index];
	snprintf(entry->name, sizeof(entry->name), "Recording %03d", next_recording_number++);
	entry->duration_seconds = duration_seconds;

	if (saved_name_buffer && buffer_size > 0)
	{
		strncpy(saved_name_buffer, entry->name, buffer_size - 1);
		saved_name_buffer[buffer_size - 1] = '\0';
	}
}

uint16_t recording_list_count(void)
{
	return (uint16_t)recording_file_count;
}

const char *recording_list_name_at(int index)
{
	if (index < 0 || index >= recording_file_count)
	{
		return "";
	}

	return recording_files[index].name;
}

const char *recording_list_duration_at(int index)
{
	if (index < 0 || index >= recording_file_count)
	{
		return "";
	}

	format_time_text(recording_files[index].duration_seconds,
					 recording_file_duration_texts[index],
					 sizeof(recording_file_duration_texts[index]));
	return recording_file_duration_texts[index];
}

static void select_recording_and_open_playback(int index)
{
	GUI_UNUSED(index);

	if (index < 0 || index >= recording_file_count)
	{
		return;
	}

	selected_recording_index = index;
	gui_view_switch_direct(gui_view_get_current(), "app_recordingPlaybackView", SWITCH_OUT_ANIMATION_FADE, SWITCH_IN_ANIMATION_FADE);
	gui_fb_change();
}

void recording_main_init_cb_impl(void)
{
	saved_recording_message[0] = '\0';
	recording_elapsed_seconds = 0;
	waveform_frame_index = 0;
	waveform_tick_divider = 0;
	recording_breath_phase = 0;
	recording_status_opacity = 255;

	if (recording_record_btn_get_state())
	{
		recording_record_btn_set_state(false);
	}
	else
	{
		recording_active = false;
	}

	update_recording_timer_display();
	update_waveform_frame_display();
	update_recording_status_display();
	gui_fb_change();
}

void recording_files_init_cb_impl(void)
{
	recording_active = false;
	playback_active = false;
	reset_recording_files_view_handles();
	refresh_recording_files_list_layout();
	if (!sync_recording_files_view_labels() && recording_file_count > 0)
	{
		schedule_recording_files_view_refresh();
	}
	gui_fb_change();
}

void recording_playback_init_cb_impl(void)
{
	recording_active = false;
	if (playback_play_btn_get_state())
	{
		playback_play_btn_set_state(false);
	}
	else
	{
		playback_active = false;
	}

	playback_elapsed_ticks = 0;
	update_playback_progress_display();
	gui_fb_change();
}

void recording_timer_tick_impl(void)
{
	if (!recording_active)
	{
		return;
	}

	recording_elapsed_seconds++;
	update_recording_timer_display();
	gui_fb_change();
}

void recording_waveform_timer_cb_impl(void)
{
	if (recording_active)
	{
		waveform_frame_index = (waveform_frame_index + 1) % WAVEFORM_FRAME_COUNT;
		recording_breath_phase = (recording_breath_phase + 1) % 60;
		if (recording_breath_phase < 30)
		{
			recording_status_opacity = (uint8_t)(110 + (recording_breath_phase * 145) / 29);
		}
		else
		{
			recording_status_opacity = (uint8_t)(110 + ((59 - recording_breath_phase) * 145) / 29);
		}
	}
	else
	{
		waveform_tick_divider = (waveform_tick_divider + 1) % 2;
		if (waveform_tick_divider == 0)
		{
			waveform_frame_index = (waveform_frame_index + 1) % WAVEFORM_FRAME_COUNT;
		}
		recording_status_opacity = 255;
	}

	update_waveform_frame_display();
	update_recording_status_display();
	gui_fb_change();
}

void playback_timer_tick_impl(void)
{
	const recording_file_entry_t *selected_recording = get_selected_recording();

	if (!playback_active || !selected_recording)
	{
		return;
	}

	if (playback_elapsed_ticks + 1 >= selected_recording->duration_seconds * 10)
	{
		playback_elapsed_ticks = selected_recording->duration_seconds * 10;
		update_playback_progress_display();
		playback_play_btn_set_state(false);
		gui_fb_change();
		return;
	}

	playback_elapsed_ticks++;
	update_playback_progress_display();
	gui_fb_change();
}

void recording_start(void *obj, gui_event_t *e)
{
	GUI_UNUSED(obj);
	GUI_UNUSED(e);

	recording_active = true;
	recording_elapsed_seconds = 0;
	saved_recording_message[0] = '\0';
	waveform_frame_index = 0;
	waveform_tick_divider = 0;
	recording_breath_phase = 0;
	recording_status_opacity = 255;

	update_recording_timer_display();
	update_waveform_frame_display();
	update_recording_status_display();
	gui_fb_change();
}

void recording_stop(void *obj, gui_event_t *e)
{
	char saved_name[32] = "";

	GUI_UNUSED(obj);
	GUI_UNUSED(e);

	recording_active = false;
	waveform_frame_index = 0;
	waveform_tick_divider = 0;
	recording_status_opacity = 255;

	if (recording_elapsed_seconds > 0)
	{
		append_recording_file(recording_elapsed_seconds, saved_name, sizeof(saved_name));
		if (saved_name[0] != '\0')
		{
			snprintf(saved_recording_message, sizeof(saved_recording_message), "Saved: %s", saved_name);
		}
	}
	else
	{
		saved_recording_message[0] = '\0';
	}

	update_files_view_display();
	update_waveform_frame_display();
	update_recording_status_display();
	gui_fb_change();
}

void playback_play(void *obj, gui_event_t *e)
{
	const recording_file_entry_t *selected_recording = get_selected_recording();

	GUI_UNUSED(obj);
	GUI_UNUSED(e);

	if (!selected_recording)
	{
		playback_active = false;
		return;
	}

	if (playback_elapsed_ticks >= selected_recording->duration_seconds * 10)
	{
		playback_elapsed_ticks = 0;
	}

	playback_active = true;
	update_playback_progress_display();
	gui_fb_change();
}

void playback_pause(void *obj, gui_event_t *e)
{
	GUI_UNUSED(obj);
	GUI_UNUSED(e);

	playback_active = false;
	update_playback_progress_display();
	gui_fb_change();
}

void recording_file_0_selected(void *obj, gui_event_t *e)
{
	GUI_UNUSED(obj);
	GUI_UNUSED(e);
	select_recording_and_open_playback(0);
}

void recording_file_1_selected(void *obj, gui_event_t *e)
{
	GUI_UNUSED(obj);
	GUI_UNUSED(e);
	select_recording_and_open_playback(1);
}

void recording_file_2_selected(void *obj, gui_event_t *e)
{
	GUI_UNUSED(obj);
	GUI_UNUSED(e);
	select_recording_and_open_playback(2);
}

void recording_file_3_selected(void *obj, gui_event_t *e)
{
	GUI_UNUSED(obj);
	GUI_UNUSED(e);
	select_recording_and_open_playback(3);
}

void recording_file_4_selected(void *obj, gui_event_t *e)
{
	GUI_UNUSED(obj);
	GUI_UNUSED(e);
	select_recording_and_open_playback(4);
}
