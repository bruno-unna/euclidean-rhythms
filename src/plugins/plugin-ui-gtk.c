#include "lv2_uris.h"

#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/ui/ui.h"
#include "lv2/urid/urid.h"

#include <cairo.h>
#include <gdk/gdk.h>
#include <glib-object.h>
#include <glib.h>
#include <gobject/gclosure.h>
#include <gtk/gtk.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLER_UI_URI "https://github.com/bruno-unna/euclidean-rhythms#ui"

#define MIN_CANVAS_W 128
#define MIN_CANVAS_H 80

typedef struct {
    LV2_Atom_Forge       forge;
    LV2_URID_Map*        map;
    LV2UI_Request_Value* request_value;
    LV2_Log_Logger       logger;
    Euclidean_URIs         uris;

    LV2UI_Write_Function write;
    LV2UI_Controller     controller;

    GtkWidget* box;
    GtkWidget* play_button;
    GtkWidget* file_button;
    GtkWidget* request_file_button;
    GtkWidget* button_box;
    GtkWidget* canvas;

    uint32_t width;
    uint32_t requested_n_peaks;
    char*    filename;

    uint8_t forge_buf[1024];

    // Optional show/hide interface
    GtkWidget* window;
    bool       did_init;
} SamplerUI;

static inline LV2_Atom_Forge_Ref
write_set_file(LV2_Atom_Forge*    forge,
               const Euclidean_URIs * uris,
               const char*        filename,
               const uint32_t     filename_len)
{
    LV2_Atom_Forge_Frame frame;
    LV2_Atom_Forge_Ref   set =
            lv2_atom_forge_object(forge, &frame, 0, uris->patch_Set);

    lv2_atom_forge_key(forge, uris->patch_property);
    lv2_atom_forge_key(forge, uris->patch_value);
    lv2_atom_forge_path(forge, filename, filename_len);

    lv2_atom_forge_pop(forge, &frame);
    return set;
}

//static void
//on_play_clicked(GtkFileChooserButton* widget, void* handle)
//{
//    SamplerUI* ui = (SamplerUI*)handle;
//    struct {
//        LV2_Atom atom;
//        uint8_t  msg[3];
//    } note_on;
//
//    note_on.atom.type = ui->uris.midi_Event;
//    note_on.atom.size = 3;
//    note_on.msg[0]    = LV2_MIDI_MSG_NOTE_ON;
//    note_on.msg[1]    = 60;
//    note_on.msg[2]    = 60;
//    ui->write(ui->controller,
//              0,
//              sizeof(LV2_Atom) + 3,
//              ui->uris.atom_event_transfer,
//              &note_on);
//}

//static void
//request_peaks(SamplerUI* ui, uint32_t n_peaks)
//{
//    if (n_peaks == ui->requested_n_peaks) {
//        return;
//    }
//
//    lv2_atom_forge_set_buffer(&ui->forge, ui->forge_buf, sizeof(ui->forge_buf));
//
//    LV2_Atom_Forge_Frame frame;
//    lv2_atom_forge_object(&ui->forge, &frame, 0, ui->uris.patch_Get);
//    lv2_atom_forge_key(&ui->forge, ui->uris.patch_accept);
//    lv2_atom_forge_urid(&ui->forge, ui->precv.uris.peaks_PeakUpdate);
//    lv2_atom_forge_key(&ui->forge, ui->precv.uris.peaks_total);
//    lv2_atom_forge_int(&ui->forge, n_peaks);
//    lv2_atom_forge_pop(&ui->forge, &frame);
//
//    LV2_Atom* msg = lv2_atom_forge_deref(&ui->forge, frame.ref);
//    ui->write(ui->controller,
//              0,
//              lv2_atom_total_size(msg),
//              ui->uris.atom_eventTransfer,
//              msg);
//
//    ui->requested_n_peaks = n_peaks;
//}
