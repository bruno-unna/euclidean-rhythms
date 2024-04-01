/*
 * Copyright 2023 by Bruno Unna.
 *
 * This file is part of Euclidean Rhythms.
 *
 * Euclidean Rhythms is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Euclidean Rhythms is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Euclidean Rhythms.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "euclidean.h"
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

#include <pugl/pugl.h>
#include <pugl/cairo.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MIN_CANVAS_W 128
#define MIN_CANVAS_H 80

typedef struct {
    LV2_Atom_Forge forge;
    LV2_URID_Map *map;
    LV2UI_Request_Value *request_value;
    LV2_Log_Logger logger;
    Euclidean_URIs uris;    // Cache of mapped URIDs

    LV2UI_Write_Function write;
    LV2UI_Controller controller;

    PuglWorld *world;
    PuglView *view;

    uint32_t width;
    uint32_t requested_n_peaks;
    char *filename;

    uint8_t forge_buf[1024];

    // Optional show/hide interface
    bool did_init;
} EuclideanUI;

static PuglStatus
onEvent(PuglView *view, const PuglEvent *event) {
    return PUGL_SUCCESS;
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor,
            const char *plugin_uri,
            const char *bundle_path,
            LV2UI_Write_Function write_function,
            LV2UI_Controller controller,
            LV2UI_Widget *widget,
            const LV2_Feature *const *features) {
    EuclideanUI *ui = (EuclideanUI *) calloc(1, sizeof(EuclideanUI));
    if (!ui) {
        return NULL;
    }

    ui->logger.log = NULL;
    ui->write = write_function;
    ui->controller = controller;
    ui->width = MIN_CANVAS_W;
    *widget = NULL;
    ui->did_init = false;

    // Get host features
    // clang-format off
    const char *missing = lv2_features_query(features,
                                             LV2_LOG__log, &ui->logger.log, false,
                                             LV2_URID__map, &ui->map, true,
                                             LV2_UI__requestValue, &ui->request_value, false,
                                             NULL);
    // clang-format on

    lv2_log_logger_set_map(&ui->logger, ui->map);

    if (missing) {
        lv2_log_error(&ui->logger, "Missing feature <%s>\n", missing);
        free(ui);
        return NULL;
    }

    map_uris(ui->map, &ui->uris);
    lv2_atom_forge_init(&ui->forge, ui->map);

    // Construct a basic UI
    ui->world = puglNewWorld(PUGL_PROGRAM, 0);
    puglSetWorldString(ui->world, PUGL_CLASS_NAME, "Euclidean Rhythms");

    ui->view = puglNewView(ui->world);


    puglSetViewString(ui->view, PUGL_WINDOW_TITLE, "Pugl Cairo Demo");
    puglSetSizeHint(ui->view, PUGL_DEFAULT_SIZE, 512, 512);
    puglSetSizeHint(ui->view, PUGL_MIN_SIZE, 256, 256);
    puglSetSizeHint(ui->view, PUGL_MAX_SIZE, 2048, 2048);
    puglSetViewHint(ui->view, PUGL_RESIZABLE, true);
    puglSetHandle(ui->view, &ui);
    puglSetBackend(ui->view, puglCairoBackend());
    puglSetEventFunc(ui->view, onEvent);

    PuglStatus st = puglRealize(ui->view);

    if (st) {
        lv2_log_error(&ui->logger, "Failed to create window (%s)\n", puglStrerror(st));
        free(ui);
        return NULL;
    }

    puglShow(ui->view, PUGL_SHOW_RAISE);

    return ui;
}

static void
cleanup(LV2UI_Handle handle) {
    EuclideanUI *ui = (EuclideanUI *) handle;

    puglFreeView(ui->view);
    puglFreeWorld(ui->world);


    free(ui);
}

static void
port_event(LV2UI_Handle handle,
           uint32_t port_index,
           uint32_t buffer_size,
           uint32_t format,
           const void *buffer) {
    EuclideanUI *ui = (EuclideanUI *) handle;
}

/* Optional non-embedded UI show interface. */
static int
ui_show(LV2UI_Handle handle) {
    EuclideanUI *ui = (EuclideanUI *) handle;


    return 0;
}

/* Optional non-embedded UI hide interface. */
static int
ui_hide(LV2UI_Handle handle) {
    EuclideanUI *ui = (EuclideanUI *) handle;


    return 0;
}

/* Idle interface for optional non-embedded UI. */
static int
ui_idle(LV2UI_Handle handle) {
    EuclideanUI *ui = (EuclideanUI *) handle;


    return 0;
}

static const void *
extension_data(const char *uri) {
    static const LV2UI_Show_Interface show = {ui_show, ui_hide};
    static const LV2UI_Idle_Interface idle = {ui_idle};

    if (!strcmp(uri, LV2_UI__showInterface)) {
        return &show;
    }

    if (!strcmp(uri, LV2_UI__idleInterface)) {
        return &idle;
    }

    return NULL;
}

static const LV2UI_Descriptor descriptor = {EUCLIDEAN_UI_URI,
                                            instantiate,
                                            cleanup,
                                            port_event,
                                            extension_data};

LV2_SYMBOL_EXPORT
const LV2UI_Descriptor *lv2ui_descriptor(uint32_t index) {
    return index == 0 ? &descriptor : NULL;
}
