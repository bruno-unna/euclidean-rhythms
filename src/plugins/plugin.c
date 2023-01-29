#define EUCLIDEAN_URI "https://github.com/bruno-unna/euclidean-rhythms"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/time/time.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2_util.h>
#include "lv2/patch/patch.h"
#include "lv2/urid/urid.h"

typedef struct {
    LV2_URID atom_Float;
    LV2_URID atom_Long;
    LV2_URID atom_Object;
    LV2_URID atom_Path;
    LV2_URID atom_Sequence;
    LV2_URID atom_URID;
    LV2_URID midi_Event;
    LV2_URID patch_Set;
    LV2_URID patch_property;
    LV2_URID patch_value;
    LV2_URID time_Position;
    LV2_URID time_speed;
    LV2_URID time_bar_beat;
    LV2_URID time_beats_per_bar;
} Euclidean_URIs;

typedef enum {
    EUCLIDEAN_CONTROL = 0,
    EUCLIDEAN_BEATS = 1,
    EUCLIDEAN_ONSETS = 2,
    EUCLIDEAN_ROTATION = 3,
    EUCLIDEAN_CHANNEL = 4,
    EUCLIDEAN_NOTE = 5,
    EUCLIDEAN_VELOCITY = 6,
    EUCLIDEAN_MIDI_OUT = 7
} Port_index;

typedef struct {
    LV2_URID_Map *map;     // URID map feature
    LV2_Log_Logger logger; // Logger API
    Euclidean_URIs uris;    // Cache of mapped URIDs

    struct {
        const LV2_Atom_Sequence *control;
        const float *beats;
        const float *onsets;
        const float *rotation;
        const float *channel;
        const float *note;
        const float *velocity;
        LV2_Atom_Sequence *midi_out;
    } ports;

    struct {
        float speed; // Transport speed (usually 0=stop, 1=play)
        float host_beats_per_bar;
        int beats_per_bar;
        float *positions_vector;
    } state;
} Euclidean;

static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
    Euclidean *self = (Euclidean *) instance;

    switch (port) {
        case EUCLIDEAN_CONTROL:
            self->ports.control = (const LV2_Atom_Sequence *) data;
            break;
        case EUCLIDEAN_BEATS:
            self->ports.beats = (float *) data;
            break;
        case EUCLIDEAN_ONSETS:
            self->ports.onsets = (float *) data;
            break;
        case EUCLIDEAN_ROTATION:
            self->ports.rotation = (float *) data;
            break;
        case EUCLIDEAN_CHANNEL:
            self->ports.channel = (float *) data;
            break;
        case EUCLIDEAN_NOTE:
            self->ports.note = (float *) data;
            break;
        case EUCLIDEAN_VELOCITY:
            self->ports.velocity = (float *) data;
            break;
        case EUCLIDEAN_MIDI_OUT:
            self->ports.midi_out = (LV2_Atom_Sequence *) data;
            break;
        default:
            break;
    }
}

static inline void map_uris(LV2_URID_Map *map, Euclidean_URIs *uris) {
    uris->atom_Float = map->map(map->handle, LV2_ATOM__Float);
    uris->atom_Object = map->map(map->handle, LV2_ATOM__Object);
    uris->atom_Path = map->map(map->handle, LV2_ATOM__Path);
    uris->atom_Sequence = map->map(map->handle, LV2_ATOM__Sequence);
    uris->atom_URID = map->map(map->handle, LV2_ATOM__URID);
    uris->midi_Event = map->map(map->handle, LV2_MIDI__MidiEvent);
    uris->patch_Set = map->map(map->handle, LV2_PATCH__Set);
    uris->patch_property = map->map(map->handle, LV2_PATCH__property);
    uris->patch_value = map->map(map->handle, LV2_PATCH__value);
    uris->time_Position = map->map(map->handle, LV2_TIME__Position);
    uris->time_speed = map->map(map->handle, LV2_TIME__speed);
    uris->time_bar_beat = map->map(map->handle, LV2_TIME__barBeat);
    uris->time_beats_per_bar = map->map(map->handle, LV2_TIME__beatsPerBar);
}

static LV2_Handle instantiate(const LV2_Descriptor *descriptor,
                              double rate,
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

    map_uris(self->map, &self->uris);

    // Initialise instance fields
    self->state.speed = 0;
    self->state.host_beats_per_bar = 0;
    self->state.positions_vector = NULL;
    self->state.beats_per_bar = 0;  // to force initialisation of position vector

    return (LV2_Handle) self;
}

static void cleanup(LV2_Handle instance) {
    Euclidean *self = (Euclidean *) instance;
    if (self->state.positions_vector != NULL) free(self->state.positions_vector);
    free(instance);
}

static void run(LV2_Handle instance, uint32_t sample_count) {
    Euclidean *self = (Euclidean *) instance;
    Euclidean_URIs *uris = &self->uris;

    typedef struct {
        LV2_Atom_Event event;
        uint8_t msg[3];
    } MIDI_note_event;

    const uint32_t out_capacity = self->ports.midi_out->atom.size;

    // Write an empty Sequence header to the output
    lv2_atom_sequence_clear(self->ports.midi_out);
    self->ports.midi_out->atom.type = uris->atom_Sequence;

    // Analyse the parameters
    int portBeats = (int) *self->ports.beats;
    if (portBeats != self->state.beats_per_bar) {
        self->state.beats_per_bar = portBeats;
        if (self->state.positions_vector != NULL) free(self->state.positions_vector);
        self->state.positions_vector = calloc(portBeats, sizeof(float));
        float delta = 1.0f / (float) portBeats;
        for (int i = 0; i < portBeats; ++i) {
            self->state.positions_vector[i] = (float) i * delta;
        }
    }

    LV2_ATOM_SEQUENCE_FOREACH(self->ports.control, ev) {
        if (ev->body.type == uris->atom_Object) {
            const LV2_Atom_Object *obj = (const LV2_Atom_Object *) &ev->body;
            if (obj->body.otype == uris->time_Position) {
                // Received new transport position/host_speed_atom
                LV2_Atom const *host_speed_atom = NULL;
                LV2_Atom const *host_bar_beat_atom = NULL;
                LV2_Atom const *host_beats_per_bar_atom = NULL;
                // clang-format off
                lv2_atom_object_get(obj,
                                    uris->time_speed, &host_speed_atom,
                                    uris->time_bar_beat, &host_bar_beat_atom,
                                    uris->time_beats_per_bar, &host_beats_per_bar_atom,
                                    NULL);
                // clang-format on

                if (host_speed_atom != 0) {
                    const float speed = (float) ((LV2_Atom_Float *) host_speed_atom)->body;
                    if (speed != self->state.speed) {
                        // Speed changed, e.g. 0 (stop) to 1 (play)
                        self->state.speed = speed;
                        lv2_log_note(&self->logger, "speed set to %f\n", self->state.speed);
                    }
                }
                if (host_beats_per_bar_atom != 0) {
                    const float beats_per_bar = (float) ((LV2_Atom_Float *) host_beats_per_bar_atom)->body;
                    if (beats_per_bar != self->state.host_beats_per_bar) {
                        // host_beats_per_bar changed
                        self->state.host_beats_per_bar = beats_per_bar;
                        lv2_log_note(&self->logger, "host_beats_per_bar set to %f\n", self->state.host_beats_per_bar);
                    }
                }
                if (host_bar_beat_atom != 0) {
                    const float barBeat = (float) ((LV2_Atom_Float *) host_bar_beat_atom)->body;
                    if (self->state.speed > 0) {
                        const float bar_progress = barBeat / self->state.host_beats_per_bar;

                        if (barBeat == 0) {
                            lv2_log_trace(&self->logger, "trying to produce a note\n");
                            MIDI_note_event note;
                            note.event.time.frames = ev->time.frames;
                            note.event.body.type = uris->midi_Event;
                            note.event.body.size = 3;
                            note.msg[0] = LV2_MIDI_MSG_NOTE_ON;
                            note.msg[1] = (int) *self->ports.note;
                            note.msg[2] = (int) *self->ports.velocity;
                            lv2_atom_sequence_append_event(self->ports.midi_out, out_capacity, &note.event);
                        } else if (barBeat == 1) {
                            lv2_log_trace(&self->logger, "trying to stop a note\n");
                            MIDI_note_event note;
                            note.event.time.frames = ev->time.frames;
                            note.event.body.type = uris->midi_Event;
                            note.event.body.size = 3;
                            note.msg[0] = LV2_MIDI_MSG_NOTE_OFF;
                            note.msg[1] = (int) *self->ports.note;
                            note.msg[2] = 0x00;
                            lv2_atom_sequence_append_event(self->ports.midi_out, out_capacity, &note.event);
                        }
                    }
                }
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
