#ifndef _ALTERATIONS_H
#define _ALTERATIONS_H

#include "parser.h"

#define OCTAVE_STEP (12)

typedef uint8_t remapping_t[0x100];

//  Single-event alterations
typedef int (*event_func_t)(event_t *, void *);
int change_event_octave(event_t *, int *);
int change_event_time(event_t *, float *);
int change_event_instrument(event_t *, remapping_t);
int change_event_note(event_t *, remapping_t);

//  Helpers
int apply_to_events(song_data_t *, event_func_t, void *);

//  Wrappers (song-level event alterations)
int change_octave(song_data_t *, int);
int warp_time(song_data_t *, float);
int remap_instruments(song_data_t *, remapping_t);
int remap_notes(song_data_t *song, remapping_t instrument_table);

//	Major functions
void add_round(song_data_t *, int, int, unsigned int, uint8_t);

//  Remapping tables
remapping_t I_BRASS_BAND;
remapping_t I_HELICOPTER;
remapping_t N_LOWER;

void __attribute__((constructor)) build_mapping_tables();

#endif // _ALTERATIONS_H
