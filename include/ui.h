#ifndef _UI_H
#define _UI_H

#include <gtk/gtk.h>

#include "library.h"

extern tree_node_t* g_current_node;
extern song_data_t* g_current_song;
extern song_data_t* g_modified_song;

GdkRGBA* COLOR_PALETTE[128];

void __attribute__ ((constructor)) build_color_palette();

//	Helpers
void update_song_list();
void update_drawing_area();
void update_info();
void update_song();
void range_of_song(song_data_t *, int *, int *, int *);

//	UI Functions
void activate(GtkApplication *, gpointer);
void add_song_cb(GtkButton *, gpointer);
void load_songs_cb(GtkButton *, gpointer);
void song_selected_cb(GtkListBox *, GtkListBoxRow *);
void search_bar_cb(GtkSearchBar *, gpointer);
void time_scale_cb(GtkSpinButton *, gpointer);
gboolean draw_cb(GtkDrawingArea *, cairo_t *, gpointer);
void warp_time_cb(GtkSpinButton *, gpointer);
void song_octave_cb(GtkSpinButton *, gpointer);
void instrument_map_cb(GtkComboBoxText *, gpointer);
void note_map_cb(GtkComboBoxText *, gpointer);
void save_song_cb(GtkButton *, gpointer);
void remove_song_cb(GtkButton *, gpointer);

#endif // _UI_H
