#include <stdarg.h>
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

// xwidgets.h includes xputty.h and all defined widgets from Xputty
#include "xwidgets.h"

#include "./euclidean.h"

#define PLUGIN_UI_URI "https://github.com/bruno-unna/euclidean-rhythms#ui"

typedef enum
{
    EFFECTS_OUTPUT,
    EFFECTS_INPUT,
    BYPASS,
    GAIN,
    METER,
} PortIndex;

/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                define controller numbers
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

#define CONTROLS 3

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
    LV2UI_Resize* resize;
} X11_UI;

// draw the window
static void draw_window(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    set_pattern(w,&w->app->color_scheme->selected,&w->app->color_scheme->normal,BACKGROUND_);
    cairo_paint (w->crb);
}

// if controller value changed send notify to host
static void value_changed(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    X11_UI* ui = (X11_UI*)w->parent_struct;
    if (ui->block_event != w->data)
        ui->write_function(ui->controller,w->data,sizeof(float),0,&w->adj->value);
    ui->block_event = -1;
}

// init the xwindow and return the LV2UI handle
static LV2UI_Handle instantiate(const LV2UI_Descriptor * descriptor,
            const char * plugin_uri, const char * bundle_path,
            LV2UI_Write_Function write_function,
            LV2UI_Controller controller, LV2UI_Widget * widget,
            const LV2_Feature * const * features) {

    X11_UI* ui = (X11_UI*)malloc(sizeof(X11_UI));

    if (!ui) {
        fprintf(stderr,"ERROR: failed to instantiate plugin with URI %s\n", plugin_uri);
        return NULL;
    }

    ui->parentXwindow = 0;
    LV2UI_Resize* resize = NULL;
    ui->block_event = -1;

    int i = 0;
    for (; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_UI__parent)) {
            ui->parentXwindow = features[i]->data;
        } else if (!strcmp(features[i]->URI, LV2_UI__resize)) {
            resize = (LV2UI_Resize*)features[i]->data;
        }
    }

    if (ui->parentXwindow == NULL)  {
        fprintf(stderr, "ERROR: Failed to open parentXwindow for %s\n", plugin_uri);
        free(ui);
        return NULL;
    }
    // init Xputty
    main_init(&ui->main);
    // create the toplevel Window on the parentXwindow provided by the host
    ui->win = create_window(&ui->main, (Window)ui->parentXwindow, 0, 0, 100, 500);
    // connect the expose func
    ui->win->func.expose_callback = draw_window;
    // create a toggle button
    ui->widget[0] = add_on_off_button(ui->win, "Power", 20, 20, 60, 60);
    // set resize mode for the toggle button to Aspect ratio
    ui->widget[0]->scale.gravity = ASPECT;
    // store the Port Index in the Widget_t data field
    ui->widget[0]->data = BYPASS;
    // store a pointer to the X11_UI struct in the parent_struct Widget_t field
    ui->widget[0]->parent_struct = ui;
    // connect the value changed callback with the write_function
    ui->widget[0]->func.value_changed_callback = value_changed;
    // create a knob widget
    ui->widget[1] = add_vslider(ui->win, "Gain", 5, 90, 40, 400);
    // store the port index in the Widget_t data field
    ui->widget[1]->data = GAIN;
    // store a pointer to the X11_UI struct in the parent_struct Widget_t field
    ui->widget[1]->parent_struct = ui;
    // set the knob adjustment to the needed range
    set_adjustment(ui->widget[1]->adj,0.0, 0.0, -40.0, 40.0, 0.1, CL_CONTINUOS);
    // connect the value changed callback with the write_function
    ui->widget[1]->func.value_changed_callback = value_changed;
    // create a meter widget
    ui->widget[2] = add_vmeter(ui->win, "Meter", true, 50, 90, 20, 400);
    // store the port index in the Widget_t data field
    ui->widget[2]->data = METER;
    // finally map all Widgets on screen
    widget_show_all(ui->win);
    // set the widget pointer to the X11 Window from the toplevel Widget_t
    *widget = (void*)ui->win->widget;
    // request to resize the parentXwindow to the size of the toplevel Widget_t
    if (resize){
        ui->resize = resize;
        resize->ui_resize(resize->handle, 100, 500);
    }
    // store pointer to the host controller
    ui->controller = controller;
    // store pointer to the host write function
    ui->write_function = write_function;

    return (LV2UI_Handle)ui;
}

// cleanup after usage
static void cleanup(LV2UI_Handle handle) {
    X11_UI* ui = (X11_UI*)handle;
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
                        const void * buffer) {
    X11_UI* ui = (X11_UI*)handle;
    float value = *(float*)buffer;
    int i=0;
    for (;i<CONTROLS;i++) {
        if (port_index == (uint32_t)ui->widget[i]->data) {
            // prevent event loop between host and plugin
            ui->block_event = (int)port_index;
            // case port is METER, convert value to meter deflection
            if (port_index == METER) value = power2db(ui->widget[i], value);
            // Xputty check if the new value differs from the old one
            // and set new one, when needed
            check_value_changed(ui->widget[i]->adj, &value);
        }
    }
}

// LV2 idle interface to host
static int ui_idle(LV2UI_Handle handle) {
    X11_UI* ui = (X11_UI*)handle;
    // Xputty event loop setup to run one cycle when called
    run_embedded(&ui->main);
    return 0;
}

// LV2 resize interface to host
static int ui_resize(LV2UI_Feature_Handle handle, int w, int h) {
    X11_UI* ui = (X11_UI*)handle;
    // Xputty sends configure event to the toplevel widget to resize itself
    if (ui) send_configure_event(ui->win,0, 0, w, h);
    return 0;
}

// connect idle and resize functions to host
static const void* extension_data(const char* uri) {
    static const LV2UI_Idle_Interface idle = { ui_idle };
    static const LV2UI_Resize resize = { 0 ,ui_resize };
    if (!strcmp(uri, LV2_UI__idleInterface)) {
        return &idle;
    }
    if (!strcmp(uri, LV2_UI__resize)) {
        return &resize;
    }
    return NULL;
}

static const LV2UI_Descriptor descriptor = {
    PLUGIN_UI_URI,
    instantiate,
    cleanup,
    port_event,
    extension_data
};


LV2_SYMBOL_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index) {
    switch (index) {
        case 0:
            return &descriptor;
        default:
        return NULL;
    }
}