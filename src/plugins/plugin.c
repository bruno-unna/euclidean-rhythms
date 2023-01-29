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
    LV2_URID atom_Double;
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
    LV2_URID time_barBeat;
    LV2_URID time_beatsPerBar;
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
        const LV2_Atom_Sequence *control;
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
        float speed; // Transport speed (usually 0=stop, 1=play)
        long bar;    // Global running bar number
        float beatsPerBar;
    } state;
} Euclidean;

static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
    Euclidean *self = (Euclidean *) instance;

    switch (port) {
        case EUCLIDEAN_CONTROL:
            self->ports.control = (const LV2_Atom_Sequence *) data;
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

static inline void map_uris(LV2_URID_Map *map, EuclideanURIs *uris) {
    uris->atom_Float = map->map(map->handle, LV2_ATOM__Float);
    uris->atom_Double = map->map(map->handle, LV2_ATOM__Double);
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
    uris->time_barBeat = map->map(map->handle, LV2_TIME__barBeat);
    uris->time_beatsPerBar = map->map(map->handle, LV2_TIME__beatsPerBar);
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
    self->state.speed = 1.0f;

    return (LV2_Handle) self;
}

static void cleanup(LV2_Handle instance) {
    free(instance);
}

static void run(LV2_Handle instance, uint32_t sample_count) {
    Euclidean *self = (Euclidean *) instance;
    EuclideanURIs *uris = &self->uris;

    typedef struct {
        LV2_Atom_Event event;
        uint8_t msg[3];
    } MIDINoteEvent;

    const uint32_t out_capacity = self->ports.midiout->atom.size;

    // Write an empty Sequence header to the output
    lv2_atom_sequence_clear(self->ports.midiout);
    self->ports.midiout->atom.type = uris->atom_Sequence;

    LV2_ATOM_SEQUENCE_FOREACH(self->ports.control, ev) {
        if (ev->body.type == uris->atom_Object) {
            const LV2_Atom_Object *obj = (const LV2_Atom_Object *) &ev->body;
            if (obj->body.otype == uris->time_Position) {
                // Received new transport position/speed_atom
                LV2_Atom const *speedAtom = NULL;
                LV2_Atom const *barBeatAtom = NULL;
                LV2_Atom const *beatsPerBarAtom = NULL;
                // clang-format off
                lv2_atom_object_get(obj,
                                    uris->time_speed, &speedAtom,
                                    uris->time_barBeat, &barBeatAtom,
                                    uris->time_beatsPerBar, &beatsPerBarAtom,
                                    NULL);
                // clang-format on

                if (speedAtom != 0) {
                    const float speed = (float) ((LV2_Atom_Float *) speedAtom)->body;
                    if (speed != self->state.speed) {
                        // Speed changed, e.g. 0 (stop) to 1 (play)
                        self->state.speed = speed;
                        lv2_log_note(&self->logger, "speed set to %f\n", self->state.speed);
                    }
                }
                if (beatsPerBarAtom != 0) {
                    const float beatsPerBar = (float) ((LV2_Atom_Float *) beatsPerBarAtom)->body;
                    if (beatsPerBar != self->state.beatsPerBar) {
                        // beatsPerBar changed
                        self->state.beatsPerBar = beatsPerBar;
                        lv2_log_note(&self->logger, "beatsPerBar set to %f\n", self->state.beatsPerBar);
                    }
                }
                if (barBeatAtom != 0) {
                    const float beat = (float) ((LV2_Atom_Float *) barBeatAtom)->body;
                    if (self->state.speed > 0) {
                        lv2_log_note(&self->logger, "bar beat is now %f\n", beat);
                        if (beat == 0) {
                            lv2_log_trace(&self->logger, "trying to produce a note\n");
                            MIDINoteEvent note;
                            note.event.time.frames = ev->time.frames;
                            note.event.body.type = uris->midi_Event;
                            note.event.body.size = 3;
                            note.msg[0] = LV2_MIDI_MSG_NOTE_ON;
                            note.msg[1] = *self->ports.note;
                            note.msg[2] = *self->ports.velocity;
                            lv2_atom_sequence_append_event(self->ports.midiout, out_capacity, &note.event);
                        } else if (beat == 1) {
                            lv2_log_trace(&self->logger, "trying to stop a note\n");
                            MIDINoteEvent note;
                            note.event.time.frames = ev->time.frames;
                            note.event.body.type = uris->midi_Event;
                            note.event.body.size = 3;
                            note.msg[0] = LV2_MIDI_MSG_NOTE_OFF;
                            note.msg[1] = *self->ports.note;
                            note.msg[2] = 0x00;
                            lv2_atom_sequence_append_event(self->ports.midiout, out_capacity, &note.event);
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
