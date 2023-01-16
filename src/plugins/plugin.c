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
    struct {
        double rate; // Sample rate
        float bpm;   // Beats per minute (tempo)
        float speed; // Transport speed (usually 0=stop, 1=play)
        int last_bar_beat;
    } state;
} Euclidean;

static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
    Euclidean *self = (Euclidean *) instance;

    switch (port) {
        case EUCLIDEAN_CONTROL:
            self->ports.control = (LV2_Atom_Sequence *) data;
            break;
        case EUCLIDEAN_BEATS:
            self->ports.beats = (uint8_t *) data;
            break;
        case EUCLIDEAN_ONSETS:
            self->ports.onsets = (uint8_t *) data;
            break;
        case EUCLIDEAN_ROTATION:
            self->ports.rotation = (uint8_t *) data;
            break;
        case EUCLIDEAN_CHANNEL:
            self->ports.channel = (uint8_t *) data;
            break;
        case EUCLIDEAN_NOTE:
            self->ports.note = (uint8_t *) data;
            break;
        case EUCLIDEAN_VELOCITY:
            self->ports.velocity = (uint8_t *) data;
            break;
        case EUCLIDEAN_MIDI_OUT:
            self->ports.midiout = (LV2_Atom_Sequence *) data;
            break;
        default:
            break;
    }
}

static LV2_Handle instantiate(const LV2_Descriptor *descriptor, double rate,
                              const char *path,
                              const LV2_Feature *const *features) {
    Euclidean *self = (Euclidean *) calloc(1, sizeof(Euclidean));
    if (!self) {
        return NULL;
    }

    self->logger.log = NULL;
    self->map = NULL;
    // clang-format off
    const char *missing = lv2_features_query(features,
                                             LV2_LOG__log, &self->logger.log, false,
                                             LV2_URID__map, &self->map, true,
                                             NULL);
    // clang-format on

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
    self->state.rate = rate;
    self->state.bpm = 120.0f;
    self->state.speed = 1.0f;
    self->state.last_bar_beat = -1;

    return (LV2_Handle) self;
}

static void cleanup(LV2_Handle instance) {
    free(instance);
}

/**
   Update the current position based on a host message. This is called by
   run() when a time:Position is received.
*/
static void update_position(Euclidean *self, const LV2_Atom_Object *obj) {
    lv2_log_trace(&self->logger, "entering update_position()\n");

    EuclideanURIs *const uris = &self->uris;

    // Received new transport position/speed_atom
    LV2_Atom *beat_atom = NULL;
    LV2_Atom *bpm_atom = NULL;
    LV2_Atom *speed_atom = NULL;
    // clang-format off
    lv2_atom_object_get(obj,
                        uris->time_barBeat, &beat_atom,
                        uris->time_beatsPerMinute, &bpm_atom,
                        uris->time_speed, &speed_atom,
                        NULL);
    // clang-format on

    if (bpm_atom && bpm_atom->type == uris->atom_Float) {
        // Tempo changed, update BPM
        self->state.bpm = ((LV2_Atom_Float *) bpm_atom)->body;
        lv2_log_trace(&self->logger, "BPM set to %f\n", self->state.bpm);
    }
    if (speed_atom && speed_atom->type == uris->atom_Float) {
        // Speed changed, e.g. 0 (stop) to 1 (play)
        self->state.speed = ((LV2_Atom_Float *) speed_atom)->body;
        lv2_log_trace(&self->logger, "speed set to %f\n", self->state.speed);
    }
    if (beat_atom && beat_atom->type == uris->atom_Float) {
        // Received a beat_atom position, synchronise
        // const float frames_per_beat = (float)(60.0 / self->bpm_atom * self->rate);
        const int bar_beat = ((LV2_Atom_Float *) beat_atom)->body;
        // const float beat_beats = bar_beat - floorf(bar_beat);
        if (bar_beat != self->state.last_bar_beat) {
            self->state.last_bar_beat = bar_beat;
            lv2_log_trace(&self->logger, "beat set to %d\n", bar_beat);
        }
    }
}

static void run(LV2_Handle instance, uint32_t sample_count) {
    Euclidean *self = (Euclidean *) instance;

    const EuclideanURIs *uris = &self->uris;
    const LV2_Atom_Sequence *in = self->ports.control;

    for (const LV2_Atom_Event *ev = lv2_atom_sequence_begin(&in->body);
         !lv2_atom_sequence_is_end(&in->body, in->atom.size, ev);
         ev = lv2_atom_sequence_next(ev)) {

        if (ev->body.type == uris->atom_Object || ev->body.type == uris->atom_Blank) {
            const LV2_Atom_Object *obj = (const LV2_Atom_Object *) &ev->body;
            if (obj->body.otype == uris->time_Position) {
                update_position(self, obj);
            }
        }
    }
}

// clang-format off
static const LV2_Descriptor descriptor = {
        EUCLIDEAN_URI,
        instantiate,
        connect_port,
        NULL,   // activate
        run,
        NULL, // deactivate,
        cleanup,
        NULL, // extension_data
};
// clang-format on

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index) {
    return index == 0 ? &descriptor : NULL;
}
