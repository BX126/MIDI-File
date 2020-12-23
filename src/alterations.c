
#include "alterations.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


#define SUCCESS (1)
#define FAIL (0)
#define NOTE_ON "Note On"
#define NOTE_OFF "Note Off"
#define POLY_KEY "Polyphonic Key"
#define PROGRAM_CHANGE "Program Change"

/* Prototype of helper function for the add_round function */

event_t *duplicate_event(event_t *input_event, int input_channel);

/* change_event_octave will change the event octave with the given
 * ocatave number. Return 1 if success and return 0 otherwise.
 */

int change_event_octave(event_t *input_event, int *input_octave_number) {

  /* Ensure the input arguments are valid */

  assert(input_event != NULL);
  assert(input_octave_number != NULL);

  /* if the type of input event is MIDI event and it's a note event,
   * then change it
   */

  if ((input_event->type != SYS_EVENT_1)
      && (input_event->type != SYS_EVENT_2)
      && (input_event->type != META_EVENT)) {

    if ((strcmp(input_event->midi_event.name, NOTE_ON) == 0)
        || (strcmp(input_event->midi_event.name, NOTE_OFF) == 0)
        || (strcmp(input_event->midi_event.name, POLY_KEY) == 0)) {

      int new_data = input_event->midi_event.data[0]
                    + (*input_octave_number) * OCTAVE_STEP;

      if ((new_data >= 0) && (new_data <= 127)) {
        input_event->midi_event.data[0] = new_data;
        return SUCCESS;
      }
    }
  }

  return FAIL;
} /* change_event_octave() */

/* change_event_time function will mutiply the event time
 * with the given mutiplier, and return the difference byte.
 */

int change_event_time(event_t *input_event, float *input_multiplier) {

  /* Ensure the input arguments are valid */

  assert(input_event != NULL);
  assert(input_multiplier != NULL);

  /* Check the byte number of origin delta time and the byte number of delta
   * time after changing
   */

  uint32_t old_delta_time = input_event->delta_time;
  int old_byte = 4;
  if (old_delta_time <= 0x7F) {
    old_byte = 1;
  } else if (old_delta_time <= 0x3FFF) {
    old_byte = 2;
  } else if (old_delta_time <= 0x1FFFFF) {
    old_byte = 3;
  }
  input_event->delta_time = input_event->delta_time * (*input_multiplier);
  int new_byte = 4;
  if (input_event->delta_time <= 0x7F) {
    new_byte = 1;
  } else if (input_event->delta_time <= 0x3FFF) {
    new_byte = 2;
  } else if (input_event->delta_time <= 0x1FFFFF) {
    new_byte = 3;
  }
  return new_byte - old_byte;
} /* change_event_time() */

/* change_event_instrument will change the instrument of event with given
 * input remapping table
 */

int change_event_instrument(event_t *event, remapping_t input_table) {

  assert(event != NULL);

  if ((event->type != SYS_EVENT_1)
      && (event->type != SYS_EVENT_2)
      && (event->type != META_EVENT)) {
    if (strcmp(event->midi_event.name, PROGRAM_CHANGE) == 0) {
      event->midi_event.data[0] = input_table[event->midi_event.data[0]];
      return SUCCESS;
    }
  }

  return FAIL;
} /* change_event_instrument() */

/* change_event_note will change the note of the given event with
 * the given remapping table
 */

int change_event_note(event_t *event, remapping_t input_table) {

  assert(event != NULL);

  /* if the type of input event is MIDI event and it's a note event,
  * then change it
  */

  if ((event->type != SYS_EVENT_1)
      && (event->type != SYS_EVENT_2)
      && (event->type != META_EVENT)) {
    if ((strcmp(event->midi_event.name, NOTE_ON) == 0)
        || (strcmp(event->midi_event.name, NOTE_OFF) == 0)
        || (strcmp(event->midi_event.name, POLY_KEY) == 0)) {
      event->midi_event.data[0] = input_table[event->midi_event.data[0]];
      return SUCCESS;
    }
  }

  return FAIL;
} /* change_event_note() */

/* apply_to_events function will apply the given function and given
 * data to each event of each track in the given song
 */

int apply_to_events(song_data_t *input_song, event_func_t input_func,
                    void *input_data) {
  int func_return_sum = 0;
  track_node_t *current_track = NULL;
  current_track = input_song->track_list;

  /* Traversal each event of each track in the given song */

  while (current_track != NULL) {
    event_node_t *current_event = NULL;
    current_event = current_track->track->event_list;
    while (current_event != NULL) {
      int func_return = 0;
      func_return = input_func(current_event->event, input_data);
      func_return_sum += func_return;
      current_event = current_event->next_event;
    }
    current_track = current_track->next_track;
  }
  return func_return_sum;
} /* apply_to_events() */

/* change_octave function will change all the octave of every
 * event with given octave number in the given song
 */

int change_octave(song_data_t *input_song, int input_octave_number) {
  int counter = 0;
  track_node_t *current_track = NULL;
  current_track = input_song->track_list;

  /* Traversal each event of each track in the given song */

  while (current_track != NULL) {
    event_node_t *current_event = NULL;
    current_event = current_track->track->event_list;
    while (current_event != NULL) {
      int func_return = 0;
      func_return = change_event_octave(current_event->event,
                                        &input_octave_number);
      current_event = current_event->next_event;
      if (func_return == SUCCESS) {
        counter++;
      }
    }
    current_track = current_track->next_track;
  }
  return counter;
} /* change_octave() */

/* warp_time will change the speed of the song with the given multiplier
 * and return the difference of the number of byte before
 * and after the change
 */

int warp_time(song_data_t *input_song, float input_multiplier) {

  assert(input_song != NULL);

  int length_changed = 0;
  track_node_t *current_track = NULL;
  current_track = input_song->track_list;

  /* Traversal each event of each track in the given song */

  while (current_track != NULL) {
    event_node_t *current_event = NULL;
    current_event = current_track->track->event_list;
    int func_return = 0;
    while (current_event != NULL) {
      func_return += change_event_time(current_event->event, &input_multiplier);
      current_event = current_event->next_event;
    }
    current_track->track->length += func_return;
    current_track = current_track->next_track;
    length_changed += func_return;
  }
  return length_changed;
} /* warp_time() */

/* apply_instruments is the helper function of
 * remapping instrument
 */

int apply_instruments(event_t *input_event, void *input_table) {
  return change_event_instrument(input_event, input_table);
} /* apply_instruments() */

/* remap_instruments will change all the instruments of event
 * in every track of the given song with given remapping table
 */

int remap_instruments(song_data_t *input_song, remapping_t input_table) {
  return apply_to_events(input_song, apply_instruments, input_table);
} /* remap_instruments() */

/* apply_note is the helper function of
 * remapping note
 */

int apply_note(event_t *input_event, void *input_table) {
  return change_event_note(input_event, input_table);
} /* apply_note() */

/* remap_note will change all the note of event
 * in every track of the given song with given remapping table
 */

int remap_notes(song_data_t *input_song, remapping_t input_table) {
  return apply_to_events(input_song, apply_note, input_table);
} /* remap_notes() */

/* add_round will takes in a song, a track index (int),
 * an octave differential (int), a time delay (unsigned int), and an instrument
 * (represented as a uint8 t per appendix 1.4 of the website),
 * and uses them to turn the song into a round of sorts.
 */

void add_round(song_data_t *input_song, int input_index,
               int input_octave, unsigned int time_delay,
               uint8_t input_instrument) {

  /* Ensure the input arguments are valid */

  assert(input_index < input_song->num_tracks);
  assert(input_song != NULL);
  assert(input_song->format != 2);

  int channel_array[16];
  for (int i = 0; i < 16; i++) {
    channel_array[i] = i;
  }

  /* Check the channels of all event and find the lowest channel */

  int counter = 0;
  track_node_t *current_track = NULL;
  current_track = input_song->track_list;
  track_t *target_track = NULL;

  track_node_t *duplicate = malloc(sizeof(track_node_t));
  assert(duplicate != NULL);
  duplicate->track = malloc(sizeof(track_t));
  duplicate->next_track = NULL;
  assert(duplicate->track != NULL);

  while (current_track != NULL) {
    event_node_t *current_event = NULL;
    current_event = current_track->track->event_list;
    while (current_event != NULL) {
      if ((current_event->event->type != SYS_EVENT_1)
          && (current_event->event->type != SYS_EVENT_2)
          && (current_event->event->type != META_EVENT)) {
        int channel = current_event->event->midi_event.status & (0b00001111);
        for (int i = 0; i < 16; i++) {
          if (channel_array[i] == channel) {
            channel_array[i] = 16;
          }
        }
      }
      current_event = current_event->next_event;
    }

    /* Find the target track */

    if (counter == input_index) {
      target_track = current_track->track;
    }
    current_track = current_track->next_track;
    counter++;
  }

  int lowest_channel = 16;
  for (int i = 0; i < 16; i++) {
    if (channel_array[i] < lowest_channel) {
      lowest_channel = channel_array[i];
    }
  }
  assert(lowest_channel != 16);

  /* Duplicate the first event of the target track and change the delta time */

  duplicate->track->length = target_track->length;
  duplicate->track->event_list = malloc(sizeof(event_node_t));
  assert(duplicate->track->event_list != NULL);
  event_node_t *target_events = target_track->event_list;
  duplicate->track->event_list->event = duplicate_event(target_events->event,
                                                        lowest_channel);
  uint32_t org_delta_time = target_track->event_list->event->delta_time;
  duplicate->track->event_list->event->delta_time = org_delta_time;
  uint32_t org_type = target_track->event_list->event->type;
  duplicate->track->event_list->event->type = org_type;
  uint32_t old_delta_time = duplicate->track->event_list->event->delta_time;

  int old_byte = 4;
  if (old_delta_time <= 0x7F) {
    old_byte = 1;
  } else if (old_delta_time <= 0x3FFF) {
    old_byte = 2;
  } else if (old_delta_time <= 0x1FFFFF) {
    old_byte = 3;
  }
  duplicate->track->event_list->event->delta_time += time_delay;
  uint32_t new_delta_time = duplicate->track->event_list->event->delta_time;
  int new_byte = 4;
  if (new_delta_time <= 0x7F) {
    new_byte = 1;
  } else if (new_delta_time <= 0x3FFF) {
    new_byte = 2;
  } else if (new_delta_time <= 0x1FFFFF) {
    new_byte = 3;
  }
  duplicate->track->length += (new_byte - old_byte);

  /* Duplicate all the events of the target track */

  event_node_t *head = NULL;
  head = duplicate->track->event_list;
  head->next_event = NULL;
  target_events = target_events->next_event;
  event_node_t *current_event = head;

  while (target_events != NULL) {
    event_node_t *temp_event = malloc(sizeof(event_node_t));
    assert(temp_event != NULL);
    temp_event->event = duplicate_event(target_events->event, lowest_channel);
    temp_event->next_event = NULL;
    current_event->next_event = temp_event;
    current_event = current_event->next_event;
    target_events = target_events->next_event;
  }
  current_event = head;

  /* Change all the events of the target track */

  remapping_t new_table = {0};
  for (int i = 0; i <= 0xFF; i++) {
    new_table[i] = input_instrument;
  }

  track_node_t *last_track = NULL;
  last_track = input_song->track_list;
  while (last_track->next_track != NULL) {
    last_track = last_track->next_track;
  }

  /* Add all duplicate track in the given song and update the number of track */

  last_track->next_track = duplicate;
  while (current_event != NULL) {
    change_event_instrument(current_event->event, new_table);
    change_event_octave(current_event->event, &input_octave);
    current_event = current_event->next_event;
  }
  input_song->num_tracks++;
} /* add_round() */

/* duplicate_event() is the helper function for the add_round
 * function
 */

event_t *duplicate_event(event_t *input_event, int input_channel) {

  event_t *new_event = malloc(sizeof(event_t));
  assert(new_event != NULL);

  if (input_event->type == META_EVENT) {
    new_event->type = input_event->type;
    new_event->delta_time = input_event->delta_time;
    new_event->meta_event.name = input_event->meta_event.name;
    new_event->meta_event.data_len = input_event->meta_event.data_len;
    if (new_event->meta_event.data_len) {
      new_event->meta_event.data = malloc(new_event->meta_event.data_len);
      assert(new_event->meta_event.data);
      for (int i = 0; i < new_event->meta_event.data_len; i++) {
        new_event->meta_event.data[i] = input_event->meta_event.data[i];
      }
    }
  }

  else if ((input_event->type == SYS_EVENT_1)
           || (input_event->type == SYS_EVENT_2)) {
    new_event->type = input_event->type;
    new_event->delta_time = input_event->delta_time;
    new_event->sys_event.data_len = input_event->sys_event.data_len;
    if (new_event->sys_event.data_len) {
      new_event->sys_event.data = malloc(new_event->sys_event.data_len);
      assert(new_event->sys_event.data);
      for (int i = 0; i < new_event->sys_event.data_len; i++) {
        new_event->sys_event.data[i] = input_event->sys_event.data[i];
      }
    }
  } else {
    new_event->delta_time = input_event->delta_time;
    new_event->midi_event.name = input_event->midi_event.name;
    new_event->midi_event.status = ((input_event->midi_event.status
                                     & (0b11110000)) | input_channel);
    new_event->type = new_event->midi_event.status;
    new_event->midi_event.data_len = input_event->midi_event.data_len;
    if (new_event->midi_event.data_len) {
      new_event->midi_event.data = malloc(new_event->midi_event.data_len);
      assert(new_event->midi_event.data);
      for (int i = 0; i < new_event->midi_event.data_len; i++) {
        new_event->midi_event.data[i] = input_event->midi_event.data[i];
      }
    }
  }

  return new_event;
} /* duplicate_event() */

/*
 * Function called prior to main that sets up random mapping tables
 */

void build_mapping_tables() {
  for (int i = 0; i <= 0xFF; i++) {
    I_BRASS_BAND[i] = 61;
  }

  for (int i = 0; i <= 0xFF; i++) {
    I_HELICOPTER[i] = 125;
  }

  for (int i = 0; i <= 0xFF; i++) {
    N_LOWER[i] = i;
  }
  //  Swap C# for C
  for (int i = 1; i <= 0xFF; i += 12) {
    N_LOWER[i] = i - 1;
  }
  //  Swap F# for G
  for (int i = 6; i <= 0xFF; i += 12) {
    N_LOWER[i] = i + 1;
  }
} /* build_mapping_tables() */
