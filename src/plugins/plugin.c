#define EUCLIDEAN_URI "https://github.com/bruno-unna/euclidean-rhythms"

#define _GNU_SOURCE

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/time/time.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2_util.h>

typedef struct {
  LV2_URID atom_Blank;
  LV2_URID atom_Float;
  LV2_URID atom_Object;
  LV2_URID atom_Path;
  LV2_URID atom_Resource;
  LV2_URID atom_Sequence;
  LV2_URID time_Position;
  LV2_URID time_barBeat;
  LV2_URID time_beatsPerMinute;
  LV2_URID time_speed;
} EuclideanURIs;

typedef enum {
  EUCLIDEAN_CONTROL = 0,
  EUCLIDEAN_BEATS = 1,
  EUCLIDEAN_ONSETS = 2,
  EUCLIDEAN_ROTATION = 3,
  EUCLIDEAN_CHANNEL = 4,
  EUCLIDEAN_NOTE = 5,
  EUCLIDEAN_VELOCITY = 6,
  EUCLIDEAN_MIDI_OUT = 7
} PortIndex;

typedef struct {
  LV2_URID_Map *map;     // URID map feature
  LV2_Log_Logger logger; // Logger API
  EuclideanURIs uris;    // Cache of mapped URIDs

  struct {
    LV2_Atom_Sequence *control;
    uint8_t *beats;
    uint8_t *onsets;
    uint8_t *rotation;
    uint8_t *channel;
    uint8_t *note;
    uint8_t *velocity;
    LV2_Atom_Sequence *midiout;
  } ports;

  // Variables to keep track of the tempo information sent by the host
  double rate; // Sample rate
  float bpm;   // Beats per minute (tempo)
  float speed; // Transport speed (usually 0=stop, 1=play)
} Euclidean;

static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
  Euclidean *self = (Euclidean *)instance;

  switch (port) {
  case EUCLIDEAN_CONTROL:
    self->ports.control = (LV2_Atom_Sequence *)data;
    break;
  case EUCLIDEAN_BEATS:
    self->ports.beats = (uint8_t *)data;
    break;
  case EUCLIDEAN_ONSETS:
    self->ports.onsets = (uint8_t *)data;
    break;
  case EUCLIDEAN_ROTATION:
    self->ports.rotation = (uint8_t *)data;
    break;
  case EUCLIDEAN_CHANNEL:
    self->ports.channel = (uint8_t *)data;
    break;
  case EUCLIDEAN_NOTE:
    self->ports.note = (uint8_t *)data;
    break;
  case EUCLIDEAN_VELOCITY:
    self->ports.velocity = (uint8_t *)data;
    break;
  case EUCLIDEAN_MIDI_OUT:
    self->ports.midiout = (LV2_Atom_Sequence *)data;
    break;
  default:
    break;
  }
}

/**
   The activate() method resets the state completely, so the wave offset is
   zero and the envelope is off.
*/
static void activate(LV2_Handle instance) {
  Euclidean *self = (Euclidean *)instance;
}

/**
   This plugin does a bit more work in instantiate() than the previous
   examples.  The tempo updates from the host contain several URIs, so those
   are mapped.
*/
static LV2_Handle instantiate(const LV2_Descriptor *descriptor, double rate,
                              const char *path,
                              const LV2_Feature *const *features) {
  Euclidean *self = (Euclidean *)calloc(1, sizeof(Euclidean));
  if (!self) {
    return NULL;
  }

  // Scan host features for URID map
  const char *missing =
      lv2_features_query(features, LV2_LOG__log, &self->logger.log, false,
                         LV2_URID__map, &self->map, true, NULL);

  lv2_log_logger_set_map(&self->logger, self->map);
  if (missing) {
    lv2_log_error(&self->logger, "Missing feature <%s>\n", missing);
    free(self);
    return NULL;
  }

  // Map URIS
  EuclideanURIs *const uris = &self->uris;
  LV2_URID_Map *const map = self->map;
  uris->atom_Blank = map->map(map->handle, LV2_ATOM__Blank);
  uris->atom_Float = map->map(map->handle, LV2_ATOM__Float);
  uris->atom_Object = map->map(map->handle, LV2_ATOM__Object);
  uris->atom_Path = map->map(map->handle, LV2_ATOM__Path);
  uris->atom_Resource = map->map(map->handle, LV2_ATOM__Resource);
  uris->atom_Sequence = map->map(map->handle, LV2_ATOM__Sequence);
  uris->time_Position = map->map(map->handle, LV2_TIME__Position);
  uris->time_barBeat = map->map(map->handle, LV2_TIME__barBeat);
  uris->time_beatsPerMinute = map->map(map->handle, LV2_TIME__beatsPerMinute);
  uris->time_speed = map->map(map->handle, LV2_TIME__speed);

  // Initialise instance fields
  self->rate = rate;
  self->bpm = 120.0f;

  return (LV2_Handle)self;
}

static void cleanup(LV2_Handle instance) { free(instance); }

/**
   Update the current position based on a host message.  This is called by
   run() when a time:Position is received.
*/
static void update_position(Euclidean *self, const LV2_Atom_Object *obj) {
  const EuclideanURIs *uris = &self->uris;

  // Received new transport position/speed
  LV2_Atom *beat = NULL;
  LV2_Atom *bpm = NULL;
  LV2_Atom *speed = NULL;
  lv2_atom_object_get(obj, uris->time_barBeat, &beat, uris->time_beatsPerMinute,
                      &bpm, uris->time_speed, &speed, NULL);

  if (bpm && bpm->type == uris->atom_Float) {
    // Tempo changed, update BPM
    self->bpm = ((LV2_Atom_Float *)bpm)->body;
    lv2_log_note(&self->logger, "setting bpm to %f\n", self->bpm);
  }
  if (speed && speed->type == uris->atom_Float) {
    // Speed changed, e.g. 0 (stop) to 1 (play)
    self->speed = ((LV2_Atom_Float *)speed)->body;
    lv2_log_note(&self->logger, "setting speed to %f\n", self->speed);
  }
  if (beat && beat->type == uris->atom_Float) {
    // Received a beat position, synchronise
    // This hard sync may cause clicks, a real plugin would be more graceful
    const float frames_per_beat = (float)(60.0 / self->bpm * self->rate);
    const float bar_beats = ((LV2_Atom_Float *)beat)->body;
    const float beat_beats = bar_beats - floorf(bar_beats);
    lv2_log_note(&self->logger, "setting frames/beat to %f\n", frames_per_beat);
    lv2_log_note(&self->logger, "setting bar beats to %f\n", bar_beats);
    lv2_log_note(&self->logger, "setting beat beats to %f\n", beat_beats);
  }
}

static void run(LV2_Handle instance, uint32_t sample_count) {
  Euclidean *self = (Euclidean *)instance;
  const EuclideanURIs *uris = &self->uris;

  // Work forwards in time frame by frame, handling events as we go
  const LV2_Atom_Sequence *in = self->ports.control;
  uint32_t last_t = 0;
  for (const LV2_Atom_Event *ev = lv2_atom_sequence_begin(&in->body);
       !lv2_atom_sequence_is_end(&in->body, in->atom.size, ev);
       ev = lv2_atom_sequence_next(ev)) {
    // Play the click for the time slice from last_t until now
    // play(self, last_t, (uint32_t)ev->time.frames);
    // TODO instead of play(), perhaps generate a MIDI event

    // Check if this event is an Object
    // (or deprecated Blank to tolerate old hosts)
    if (ev->body.type == uris->atom_Object ||
        ev->body.type == uris->atom_Blank) {
      const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;
      if (obj->body.otype == uris->time_Position) {
        // Received position information, update
        update_position(self, obj);
      }
    }

    // Update time for next iteration and move to next event
    last_t = (uint32_t)ev->time.frames;
  }

  // Play for remainder of cycle
  // play(self, last_t, sample_count);
  // TODO investigate if something like this is needed for MIDI events
}

static const LV2_Descriptor descriptor = {
    EUCLIDEAN_URI,
    instantiate,
    connect_port,
    activate,
    run,
    NULL, // deactivate,
    cleanup,
    NULL, // extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index) {
  return index == 0 ? &descriptor : NULL;
}
