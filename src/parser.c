#include "parser.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define HEADER_CHUNK_TYPE  "MThd"
#define TRACK_CHUNK_TYPE   "MTrk"

uint8_t g_status = 0;

/* parse_file function will first read the MIDI file from the input file path,
 * then it will save the data into a song data struct and return it.
 */

song_data_t *parse_file(const char *input_file_path) {

  /* Open file */

  assert(input_file_path != NULL);
  FILE *fp = NULL;
  fp = fopen(input_file_path, "rb");
  assert(fp != NULL);

  /* Create new song data and save the data that readed from file*/

  song_data_t *result_data = malloc(sizeof(song_data_t));
  assert(result_data);
  result_data->path = malloc((sizeof(char) * strlen(input_file_path) + 1));
  assert(result_data->path);
  strcpy(result_data->path, input_file_path);
  result_data->track_list = NULL;

  /* Parser Header and Track */

  parse_header(fp, result_data);
  uint16_t read_size = result_data->num_tracks;
  int counter = 0;
  while (counter < read_size) {
    parse_track(fp, result_data);
    counter++;
  }

  /* Finish Parsing */

  int end_file = getc(fp);
  assert(end_file == EOF);
  fclose(fp);
  fp = NULL;
  return result_data;
} /* parse_file() */

/* Parser header will parse the header chunk from MIDI file,
 * then return the song data.
 */

void parse_header(FILE *fp, song_data_t *song_data) {

  assert(fp != NULL);

  /* Header Chunk format:
   * <Header Chunk> = <chunk type> <length> <format> <ntrks> <division>
   */

  /* Parse header chunk type */

  char input_chunk_type[5] = "";
  int read_len = 0;
  read_len = fread(input_chunk_type, sizeof(char), 4, fp);
  assert(read_len == 4);
  assert(strcmp(input_chunk_type, HEADER_CHUNK_TYPE) == 0);

  /* Parse lenth data and convert endian*/

  uint32_t input_length = 0;
  uint8_t input_arr[4] = {};
  for (int i = 0; i < 4; i++) {
    read_len = fread(&input_arr[i], sizeof(uint8_t), 1, fp);
    assert(read_len == 1);
  }
  input_length = end_swap_32(input_arr);
  assert(input_length == 6);

  /* Parse format data and convert endian*/

  uint16_t input_format = 0;
  read_len = fread(&input_format, sizeof(uint16_t), 1, fp);
  input_format = (input_format >> 8) | (input_format << 8);
  assert(read_len == 1);
  assert((input_format == 0) | (input_format == 1) | (input_format == 2));
  song_data->format = input_format;

  /* Parse number of track data and convert endian*/

  uint16_t input_ntrks = 0;
  read_len = fread(&input_ntrks, sizeof(uint16_t), 1, fp);
  assert(read_len == 1);
  input_ntrks = (input_ntrks >> 8) | (input_ntrks << 8);
  song_data->num_tracks = input_ntrks;

  /* Parse division data and convert endian*/

  uint16_t input_division = 0;
  read_len = fread(&input_division, sizeof(uint16_t), 1, fp);
  assert(read_len == 1);
  input_division = (input_division >> 8) | (input_division << 8);
  if (!(input_division & (0b0000000000000001))) {
    song_data->division.uses_tpq = true;
    song_data->division.ticks_per_qtr = input_division;
  } else {
    song_data->division.uses_tpq = false;
    song_data->division.ticks_per_frame = (uint8_t)((input_division >> 8)
                                                     & (0b11111111100000000));
    song_data->division.frames_per_sec = input_division & (0b0000000011111110);
  }
} /* parse_header() */

/* Parser track will parse the track chunk from MIDI file,
 * then return the song data.
 */

void parse_track(FILE *fp, song_data_t *song_data) {

  /* Track Chunk format:
   * <Track Chunk> = <chunk type> <length> <event>
   */

  /* Parse track chunk type */

  char input_chunk_type[5] = "";
  int read_len = 0;
  read_len = fread(input_chunk_type, sizeof(char), 4, fp);
  assert(read_len == 4);
  assert(strcmp(input_chunk_type, TRACK_CHUNK_TYPE) == 0);
  track_t *input_track = malloc(sizeof(track_t));
  assert(input_track != NULL);

  /* Parse length data and convert endian */

  uint32_t input_length = 0;
  uint8_t input_arr[4] = {};
  for (int i = 0; i < 4; i++) {
    read_len = fread(&input_arr[i], sizeof(uint8_t), 1, fp);
    assert(read_len == 1);
  }
  input_length = end_swap_32(input_arr);
  input_track->length = input_length;

  /* Parse event list of new track  */

  input_track->event_list = malloc(sizeof(event_node_t));
  assert(input_track->event_list);
  int read_size = ftell(fp) + input_length;
  input_track->event_list->event = parse_event(fp);
  input_track->event_list->next_event = NULL;
  do {
    if (ftell(fp) >= read_size) {
      break;
    }
    event_node_t *current_event = input_track->event_list;
    while (current_event->next_event != NULL) {
      current_event = current_event->next_event;
    }
    current_event->next_event = malloc(sizeof(event_t));
    assert(current_event->next_event);
    current_event->next_event->event = parse_event(fp);
    current_event->next_event->next_event = NULL;
  } while (1);

  /* Update new track into the track list of song data */

  track_node_t *input_track_node = malloc(sizeof(track_node_t));
  assert(input_track_node);
  input_track_node->next_track = NULL;
  input_track_node->track = input_track;

  if (song_data->track_list == NULL) {
    song_data->track_list = input_track_node;
  } else {
    track_node_t *current_track = song_data->track_list;
    while (current_track->next_track) {
      current_track = current_track->next_track;
    }
    current_track->next_track = input_track_node;
  }
} /* parse_track() */

/* Parse_sys_event function will parse event that has type as sys_event
 * from the given MIDI file
 */

sys_event_t parse_sys_event(FILE *fp) {
  sys_event_t result = {};
  result.data_len = parse_var_len(fp);
  uint8_t read_size = result.data_len;
  result.data = malloc(result.data_len);
  int read_len = fread(result.data, sizeof(uint8_t), read_size, fp);
  assert(read_len == read_size);
  return result;
} /* parse_sys_event() */

/* Parse_meta_event function will parse event that has type as meta_event
 * from the given MIDI file
 */

meta_event_t parse_meta_event(FILE *fp) {
  meta_event_t result = {};
  uint8_t event_type = 0;
  int read_len = fread(&event_type, sizeof(uint8_t), 1, fp);

  assert(read_len == 1);
  result.name = META_TABLE[event_type].name;
  assert(result.name != NULL);
  result.data_len = META_TABLE[event_type].data_len;

  /* Check the read data length with meta table */

  if (result.data_len != 0) {
    uint32_t input_len = 0;
    read_len = fread(&input_len, sizeof(uint8_t), 1, fp);
    assert(input_len == result.data_len);
  } else {
    result.data_len = parse_var_len(fp);
  }
  if (result.data_len != 0) {
    uint8_t read_size = result.data_len;
    result.data = malloc(result.data_len);
    assert(result.data != NULL);
    read_len = fread(result.data, sizeof(uint8_t), read_size, fp);
    assert(read_len == read_size);
  }
  return result;
} /* parse_meta_event() */

/* Parse_midi_event function will parse event that has type as midi_event
 * from the given MIDI file
 */

midi_event_t parse_midi_event(FILE *fp, uint8_t input_status) {

  /* Read event type and check which type of midi event it is */

  midi_event_t result = {};
  result.status = input_status;
  int counter = 0;
  if (input_status & (0b10000000)) {
    g_status = result.status;
  } else {
    result.status = g_status;
    counter++;
  }
  result.name = MIDI_TABLE[g_status].name;
  assert(result.name != NULL);
  result.data_len = MIDI_TABLE[g_status].data_len;

  /* Parse data of midi event */

  if (result.data_len > 0) {
    result.data = malloc(result.data_len);
    assert(result.data);

    int read_len = 0;
    if (counter != 0) {
      result.data[0] = input_status;
      read_len = fread(&result.data[1], sizeof(uint8_t),
                       result.data_len - 1, fp);
      assert(read_len == result.data_len - 1);
    }
    if (counter == 0) {
      read_len = fread(result.data, sizeof(uint8_t), result.data_len, fp);
      assert(read_len == result.data_len);
    }
  }
  return result;
} /* parse_midi_event() */

/* Parse_event function will parse event from the given MIDI file.
 * There're three type of events: Sys_event, meta_event.midi_event
 */

event_t *parse_event(FILE *fp) {
  event_t *result = malloc(sizeof(event_t));
  assert(result != NULL);
  result->delta_time = parse_var_len(fp);
  int read_len = fread(&result->type, sizeof(uint8_t), 1, fp);
  if (read_len != 1) {
    return NULL;
  }

  /* Parser event by type */

  if ((result->type == SYS_EVENT_1) || (result->type == SYS_EVENT_2)) {
    result->sys_event = parse_sys_event(fp);
  } else if (result->type == META_EVENT) {
    result->meta_event = parse_meta_event(fp);
  } else {
    result->midi_event = parse_midi_event(fp, result->type);
  }
  return result;
} /* parse_event() */

/* Parse_var_len will convert the MIDI VLQ into hex number
 * to help us to save read data
 */

uint32_t parse_var_len(FILE *fp) {
  uint32_t result = 0;
  while (1) {
    uint8_t input_vlq = 0;
    int read_len = fread(&input_vlq, sizeof(uint8_t), 1, fp);
    assert(read_len == 1);
    if ((input_vlq & 0b10000000) == 0) {
      input_vlq = input_vlq & 0b1111111;
      result = (result << 7) | input_vlq;
      break;
    }
    input_vlq = input_vlq & 0b1111111;
    result = (result << 7) | input_vlq;
  }
  return result;
} /* parse_var_len() */

/* event_type will parse the event type
 * for interpreting data internally
 */

uint8_t event_type(event_t *input_event) {
  if (input_event->type == META_EVENT) {
    return META_EVENT_T;
  }
  if (input_event->type == SYS_EVENT_1) {
    return SYS_EVENT_T;
  }
  if (input_event->type == SYS_EVENT_2) {
    return SYS_EVENT_T;
  }
  return MIDI_EVENT_T;
} /* event_type() */

/* free_song will free the given song and all memory
 * of this song.
 */

void free_song(song_data_t *input_song) {
  free(input_song->path);
  input_song->path = NULL;
  while (input_song->track_list != NULL) {
    track_node_t *old_track = input_song->track_list;
    input_song->track_list = input_song->track_list->next_track;
    free_track_node(old_track);
    old_track = NULL;
  }
  free(input_song->track_list);
  input_song->track_list = NULL;
  free(input_song);
  input_song = NULL;
} /* free_song() */

/* free_track_node will free the given track node
 * and it's memory
 */

void free_track_node(track_node_t *input_track) {
  while (input_track->track->event_list != NULL) {
    event_node_t *old_event = input_track->track->event_list;
    input_track->track->event_list = input_track->track->event_list->next_event;
    free_event_node(old_event);
    old_event = NULL;
  }
  free(input_track->track->event_list);
  input_track->track->event_list = NULL;
  free(input_track->track);
  input_track->track = NULL;
  free(input_track);
  input_track = NULL;
} /* free_track_node() */

/* free_event_node will free the given
 * event node
 */

void free_event_node(event_node_t *input_event) {
  if (event_type(input_event->event) == SYS_EVENT_T) {
    free(input_event->event->sys_event.data);
    input_event->event->sys_event.data = NULL;
  }
  if (event_type(input_event->event) == META_EVENT_T) {
    free(input_event->event->meta_event.data);
    input_event->event->meta_event.data = NULL;
  }
  if (event_type(input_event->event) == MIDI_EVENT_T) {
    free(input_event->event->midi_event.data);
    input_event->event->midi_event.data = NULL;
  }
  free(input_event->event);
  input_event->event = NULL;
  free(input_event);
  input_event = NULL;
} /* free_event_node() */

/* end_swap_16 function will convert the endianness of 16 byte number.
 * if the input is little endian, it will return the big endian.
 * if the input is big endian, it will return the little endian.
 */

uint16_t end_swap_16(uint8_t input[2]) {
  uint16_t swap = 0;
  swap = input[0] << 8 | input[1];
  return swap;
} /* end_swap_16() */

/* end_swap_16 function will convert the endianness of 32 byte number.
 * if the input is little endian, it will return the big endian.
 * if the input is big endian, it will return the little endian.
 */

uint32_t end_swap_32(uint8_t input[4]) {
  uint32_t swap = 0;
  swap = input[0] << 24 | input[1] << 16 | input[2] << 8 | input[3];
  return swap;
} /* end_swap_32() */