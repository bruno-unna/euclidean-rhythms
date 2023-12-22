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

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

// xwidgets.h includes xputty.h and all defined widgets from Xputty
#include "xwidgets.h"

#include "plugin.h"

/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                define controller numbers
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

#define CONTROLS 2

/*
 * Various definitions
 */
#define KNOB_H_OFFSET 50
#define KNOB_H_SPACE 50
#define KNOB_WIDTH 40
#define KNOB_HEIGHT 60

/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                the main LV2 handle->XWindow
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

// main window struct
typedef struct {
    void *parentXwindow;
    Xputty main;
    Widget_t *win;
    Widget_t *widget[CONTROLS];
    int block_event;

    void *controller;
    LV2UI_Write_Function write_function;
    LV2UI_Resize *resize;
} X11_UI;

// draw the window
static void draw_window(void *w_, void *user_data) {
    Widget_t *w = (Widget_t *) w_;
    set_pattern(w, &w->app->color_scheme->selected, &w->app->color_scheme->normal, BACKGROUND_);
    cairo_paint(w->crb);
}

// if controller value changed send notify to host
static void value_changed(void *w_, void *user_data) {
    Widget_t *w = (Widget_t *) w_;
    X11_UI *ui = (X11_UI *) w->parent_struct;
    ui->write_function(ui->controller, w->data, sizeof(float), 0, &w->adj->value);
    ui->block_event = -1;
}

static void
create_knob(X11_UI *ui, short widget_index, short port_index,
            char *label, int pos_x, int pos_y,
            float std_value, float value, float min_value, float max_value) {
    ui->widget[widget_index] = add_knob(ui->win, label, pos_x, pos_y, KNOB_WIDTH, KNOB_HEIGHT);
    // store the port index in the Widget_t data field
    ui->widget[widget_index]->data = port_index;
    // store a pointer to the X11_UI struct in the parent_struct Widget_t field
    ui->widget[widget_index]->parent_struct = ui;
    // set the knob adjustment to the needed range
    set_adjustment(ui->widget[widget_index]->adj, std_value, value, min_value, max_value, 1.0f, CL_CONTINUOS);
    // connect the value changed callback with the write_function
    ui->widget[widget_index]->func.value_changed_callback = value_changed;
}

// init the xwindow and return the LV2UI handle
static LV2UI_Handle instantiate(const LV2UI_Descriptor *descriptor,
                                const char *plugin_uri, const char *bundle_path,
                                LV2UI_Write_Function write_function,
                                LV2UI_Controller controller, LV2UI_Widget *widget,
                                const LV2_Feature *const *features) {

    X11_UI *ui = (X11_UI *) malloc(sizeof(X11_UI));

    if (!ui) {
        fprintf(stderr, "ERROR: failed to instantiate plugin with URI %s\n", plugin_uri);
        return NULL;
    }

    ui->parentXwindow = 0;
    LV2UI_Resize *resize = NULL;
    ui->block_event = -1;

    for (int i = 0; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_UI__parent)) {
            ui->parentXwindow = features[i]->data;
        } else if (!strcmp(features[i]->URI, LV2_UI__resize)) {
            resize = (LV2UI_Resize *) features[i]->data;
        }
    }

    if (ui->parentXwindow == NULL) {
        fprintf(stderr, "ERROR: Failed to open parentXwindow for %s\n", plugin_uri);
        free(ui);
        return NULL;
    }
    // init Xputty
    main_init(&ui->main);
    // create the toplevel Window on the parentXwindow provided by the host
    ui->win = create_window(&ui->main, (Window) ui->parentXwindow, 0, 0, 500, 100);
    // connect the expose func
    ui->win->func.expose_callback = draw_window;

    // add the widgets
    add_label(ui->win, "0", 5, 10, 40, 40);
    create_knob(ui, 0, EUCLIDEAN_BEATS, "Beats", KNOB_H_OFFSET + 0 * KNOB_H_SPACE, 10, 8.0f, 8.0f, 2.0f, 64.0f);
    create_knob(ui, 1, EUCLIDEAN_ONSETS, "Onsets", KNOB_H_OFFSET + 1 * KNOB_H_SPACE, 10, 5.0f, 5.0f, 0.0f, 64.0f);
    create_knob(ui, 2, EUCLIDEAN_ROTATION, "Rot", KNOB_H_OFFSET + 2 * KNOB_H_SPACE, 10, 0.0f, 0.0f, -32.0f, 31.0f);
    create_knob(ui, 3, EUCLIDEAN_BARS, "Bars", KNOB_H_OFFSET + 3 * KNOB_H_SPACE, 10, 1.0f, 1.0f, 1.0f, 8.0f);
    create_knob(ui, 4, EUCLIDEAN_CHANNEL, "Chan", KNOB_H_OFFSET + 4 * KNOB_H_SPACE, 10, 10.0f, 10.0f, 1.0f, 16.0f);
    create_knob(ui, 5, EUCLIDEAN_NOTE, "Note", KNOB_H_OFFSET + 5 * KNOB_H_SPACE, 10, 48.0f, 48.0f, 0.0f, 127.0f);
    create_knob(ui, 6, EUCLIDEAN_VELOCITY, "Vel", KNOB_H_OFFSET + 6 * KNOB_H_SPACE, 10, 64.0f, 64.0f, 0.0f, 127.0f);

    // finally map all Widgets on screen
    widget_show_all(ui->win);
    // set the widget pointer to the X11 Window from the toplevel Widget_t
    *widget = (void *) ui->win->widget;
    // request to resize the parentXwindow to the size of the toplevel Widget_t
    if (resize) {
        ui->resize = resize;
        resize->ui_resize(resize->handle, 500, 100);
    }
    // store pointer to the host controller
    ui->controller = controller;
    // store pointer to the host write function
    ui->write_function = write_function;

    return (LV2UI_Handle) ui;
}

// cleanup after usage
static void cleanup(LV2UI_Handle handle) {
    X11_UI *ui = (X11_UI *) handle;
    // Xputty free all memory used
    main_quit(&ui->main);
    free(ui);
}

/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                        LV2 interface
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

// port value change message from host
static void port_event(LV2UI_Handle handle, uint32_t port_index,
                       uint32_t buffer_size, uint32_t format,
                       const void *buffer) {
    X11_UI *ui = (X11_UI *) handle;
    float value = *(float *) buffer;
    for (int i = 0; i < CONTROLS; i++) {
        if (port_index == (uint32_t) ui->widget[i]->data) {
            // prevent event loop between host and plugin
            ui->block_event = (int) port_index;
            // case port is METER, convert value to meter deflection
//            if (port_index == METER) value = power2db(ui->widget[i], value);
            // Xputty check if the new value differs from the old one
            // and set new one, when needed
            check_value_changed(ui->widget[i]->adj, &value);
        }
    }
}

// LV2 idle interface to host
static int ui_idle(LV2UI_Handle handle) {
    X11_UI *ui = (X11_UI *) handle;
    // Xputty event loop setup to run one cycle when called
    run_embedded(&ui->main);
    return 0;
}

// LV2 resize interface to host
static int ui_resize(LV2UI_Feature_Handle handle, int w, int h) {
    X11_UI *ui = (X11_UI *) handle;
    // Xputty sends configure event to the toplevel widget to resize itself
    if (ui) send_configure_event(ui->win, 0, 0, w, h);
    return 0;
}

// connect idle and resize functions to host
static const void *extension_data(const char *uri) {
    static const LV2UI_Idle_Interface idle = {ui_idle};
    static const LV2UI_Resize resize = {0, ui_resize};
    if (!strcmp(uri, LV2_UI__idleInterface)) {
        return &idle;
    }
    if (!strcmp(uri, LV2_UI__resize)) {
        return &resize;
    }
    return NULL;
}

static const LV2UI_Descriptor descriptor = {
        EUCLIDEAN_UI_URI,
        instantiate,
        cleanup,
        port_event,
        extension_data
};


LV2_SYMBOL_EXPORT
const LV2UI_Descriptor *lv2ui_descriptor(uint32_t index) {
    if (index == 0) return &descriptor;
    return NULL;
}
