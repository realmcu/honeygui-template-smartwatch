/**
 * USER-OWNED FILE - GENERATED ONCE BY HONEYGUI AUTOGEN.
 * This file is not overwritten after creation; edit it freely.
 */
#include "app_music_player_user.h"
#include "../ui/app_music_player_ui.h"
#include "gui_rect.h"
#include "gui_arc.h"
#include "gui_img.h"
#include "gui_view.h"
#include "gui_text.h"
#include <string.h>
#include <stdio.h>

/* === State Variables === */
static bool is_playing = false;
static int current_song_index = 0;
static int volume = 70;          /* Default 70% */
static float progress = 0.0f;    /* 0~100 */

#define SONG_COUNT 6

static const char *song_titles[SONG_COUNT] = {
    "Let Me Know", "Midnight City", "Golden Hour",
    "Ocean Drive", "Starlight", "Vinyl Dreams"
};
static const char *song_artists[SONG_COUNT] = {
    "No Wyld", "M83", "JVKE",
    "Duke Dumont", "Muse", "Retro Wave"
};

/* Cover image paths (filesystem paths after conversion) */
static const char *cover_paths[SONG_COUNT] = {
    "/app_music_player/cover_1.bin",
    "/app_music_player/cover_2.bin",
    "/app_music_player/cover_3.bin",
    "/app_music_player/cover_4.bin",
    "/app_music_player/cover_5.bin",
    "/app_music_player/cover_6.bin"
};

/* === Helper: Populate playlist widget arrays (always refresh, views are recreated) === */
static gui_rounded_rect_t *pl_song_bgs[SONG_COUNT];
static gui_text_t *pl_song_titles[SONG_COUNT];

static void refresh_pl_arrays(void)
{
    pl_song_bgs[0] = pl_song_1_bg;
    pl_song_bgs[1] = pl_song_2_bg;
    pl_song_bgs[2] = pl_song_3_bg;
    pl_song_bgs[3] = pl_song_4_bg;
    pl_song_bgs[4] = pl_song_5_bg;
    pl_song_bgs[5] = pl_song_6_bg;

    pl_song_titles[0] = pl_song_1_title;
    pl_song_titles[1] = pl_song_2_title;
    pl_song_titles[2] = pl_song_3_title;
    pl_song_titles[3] = pl_song_4_title;
    pl_song_titles[4] = pl_song_5_title;
    pl_song_titles[5] = pl_song_6_title;
}

/* === Helper: Update PlayerView song display === */
static void update_song_display(void)
{
    gui_log("len %d %d", strlen(song_titles[current_song_index]), strlen(song_artists[current_song_index]));
    /* Full re-init: gui_text_set + gui_text_type_set + gui_text_mode_set */
    gui_text_set(song_title_label, (char *)song_titles[current_song_index],
                 GUI_FONT_SRC_BMP, gui_rgb(242, 242, 242),
                 strlen(song_titles[current_song_index]), 22);
    gui_text_type_set(song_title_label,
                      "/font/Inter_24pt_Regular_size22_bits4_bitmap.bin", FONT_SRC_FILESYS);
    gui_text_mode_set(song_title_label, MID_CENTER);

    gui_text_set(artist_label, (char *)song_artists[current_song_index],
                 GUI_FONT_SRC_BMP, gui_rgb(102, 102, 102),
                 strlen(song_artists[current_song_index]), 14);
    gui_text_type_set(artist_label,
                      "/font/Inter_24pt_Regular_size14_bits4_bitmap.bin", FONT_SRC_FILESYS);
    gui_text_mode_set(artist_label, MID_CENTER);

    gui_img_set_src((gui_img_t *)album_cover,
                    cover_paths[current_song_index], IMG_SRC_FILESYS);
}

/* === Helper: Update volume bar and percentage === */
static void update_volume_display(void)
{
    int bar_width = 240 * volume / 100;
    /* Update volume_bar_fill width by setting rect size */
    gui_rect_set_size(volume_bar_fill, bar_width, 10);

    /* Update volume_percent_label text */
    static char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", volume);
    gui_text_set(volume_percent_label, buf, GUI_FONT_SRC_BMP, gui_rgb(249, 211, 66),
                 strlen(buf), 28);
    gui_text_type_set(volume_percent_label,
                      "/font/Inter_24pt_Regular_size28_bits4_bitmap.bin", FONT_SRC_FILESYS);
    gui_text_mode_set(volume_percent_label, MID_CENTER);
    gui_fb_change();
}

/* === Helper: VolumeOverlay visibility === */
static void set_volume_overlay_visible(bool visible)
{
    gui_obj_show((gui_obj_t *)volume_overlay_window, visible);
    gui_fb_change();
}

/* === Helper: Update playlist highlight === */
static void update_playlist_highlight(int old_index, int new_index)
{
    refresh_pl_arrays();

    if (old_index >= 0 && old_index < SONG_COUNT)
    {
        /* Deselect old: transparent bg, white text */
        gui_rect_set_color(pl_song_bgs[old_index], gui_rgb(0, 0, 0));
        gui_rect_set_opacity(pl_song_bgs[old_index], 0);
        gui_text_set(pl_song_titles[old_index],
                     (char *)song_titles[old_index],
                     GUI_FONT_SRC_BMP, gui_rgb(242, 242, 242),
                     strlen(song_titles[old_index]), 18);
        gui_text_type_set(pl_song_titles[old_index],
                          "/font/Inter_24pt_Regular_size18_bits4_bitmap.bin", FONT_SRC_FILESYS);
        gui_text_mode_set(pl_song_titles[old_index], MID_LEFT);
    }

    if (new_index >= 0 && new_index < SONG_COUNT)
    {
        /* Select new: gold bg, gold text */
        gui_rect_set_color(pl_song_bgs[new_index], gui_rgb(61, 53, 32));
        gui_rect_set_opacity(pl_song_bgs[new_index], 255);
        gui_text_set(pl_song_titles[new_index],
                     (char *)song_titles[new_index],
                     GUI_FONT_SRC_BMP, gui_rgb(249, 211, 66),
                     strlen(song_titles[new_index]), 18);
        gui_text_type_set(pl_song_titles[new_index],
                          "/font/Inter_24pt_Regular_size18_bits4_bitmap.bin", FONT_SRC_FILESYS);
        gui_text_mode_set(pl_song_titles[new_index], MID_LEFT);
    }
}

/* === Helper: Select song from playlist and switch to PlayerView === */
static void select_song(int index)
{
    int old_index = current_song_index;
    current_song_index = index;
    is_playing = true;
    progress = 0.0f;

    /* Update PlaylistView highlight before switching */
    update_playlist_highlight(old_index, current_song_index);
    gui_fb_change();

    /* Switch to PlayerView */
    gui_view_switch_direct(gui_view_get_current(), "app_music_playerPlayerView",
                           SWITCH_OUT_ANIMATION_FADE, SWITCH_IN_ANIMATION_FADE);
}

/* === View Init Callbacks (weak impl, called by generated _callbacks.c) === */

/* Called once when PlayerView is created */
void player_view_init_cb_impl(void)
{
    update_song_display();
    if (is_playing)
    {
        gui_img_set_src((gui_img_t *)play_pause_btn,
                        "/app_music_player/pause_btn.bin", IMG_SRC_FILESYS);
    }
    float angle = -90.0f + (progress / 100.0f) * 360.0f;
    gui_arc_set_end_angle(progress_ring, angle);
    gui_fb_change();
}

/* Called once when PlaylistView is created */
void playlist_view_init_cb_impl(void)
{
    update_playlist_highlight(-1, current_song_index);
    gui_fb_change();
}

/* === Callback Implementations === */

/* 1. Play/Pause toggle */
void music_toggle_play(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    is_playing = !is_playing;
    if (is_playing)
    {
        gui_img_set_src((gui_img_t *)play_pause_btn,
                        "/app_music_player/pause_btn.bin", IMG_SRC_FILESYS);
    }
    else
    {
        gui_img_set_src((gui_img_t *)play_pause_btn,
                        "/app_music_player/play_btn.bin", IMG_SRC_FILESYS);
    }
    gui_fb_change();
}

/* 2. Next song (PlayerView only) */
void music_next(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    current_song_index = (current_song_index + 1) % SONG_COUNT;
    is_playing = true;
    progress = 0.0f;
    gui_img_set_src((gui_img_t *)play_pause_btn,
                    "/app_music_player/pause_btn.bin", IMG_SRC_FILESYS);
    update_song_display();
    gui_arc_set_end_angle(progress_ring, -90.0f);
    gui_fb_change();
}

/* 3. Previous song (PlayerView only) */
void music_prev(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    current_song_index = (current_song_index - 1 + SONG_COUNT) % SONG_COUNT;
    is_playing = true;
    progress = 0.0f;
    gui_img_set_src((gui_img_t *)play_pause_btn,
                    "/app_music_player/pause_btn.bin", IMG_SRC_FILESYS);
    update_song_display();
    gui_arc_set_end_angle(progress_ring, -90.0f);
    gui_fb_change();
}

/* 4. Show volume overlay */
void music_show_volume(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    set_volume_overlay_visible(true);
}

/* 5. Close volume overlay */
void music_close_volume(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    set_volume_overlay_visible(false);
}

/* 6. Volume down */
void music_volume_down(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    volume = (volume - 10 < 0) ? 0 : volume - 10;
    update_volume_display();
}

/* 7. Volume up */
void music_volume_up(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    volume = (volume + 10 > 100) ? 100 : volume + 10;
    update_volume_display();
}

/* 8. Playlist song selection (0~5) */
void playlist_select_song_0(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    select_song(0);
}

void playlist_select_song_1(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    select_song(1);
}

void playlist_select_song_2(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    select_song(2);
}

void playlist_select_song_3(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    select_song(3);
}

void playlist_select_song_4(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    select_song(4);
}

void playlist_select_song_5(void *obj, gui_event_t *e)
{
    GUI_UNUSED(obj);
    GUI_UNUSED(e);
    select_song(5);
}

/* 9. Progress ring timer callback (weak impl, called by generated _callbacks.c) */
void music_progress_timer_cb_impl(void)
{
    if (!is_playing)
    {
        return;
    }
    progress += 0.5f;
    if (progress >= 100.0f)
    {
        /* Auto next song */
        music_next(NULL, NULL);
        return;
    }
    /* Update progress ring endAngle: -90 + (progress/100)*360 */
    float angle = -90.0f + (progress / 100.0f) * 360.0f;
    gui_arc_set_end_angle(progress_ring, angle);
    gui_fb_change();
}
