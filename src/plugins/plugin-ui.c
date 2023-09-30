#include <xputty/xputty.h>
#include <cairo/cairo.h>

/** your own expose function */
static void draw_window(void *w_, void *user_data) {
    Widget_t *w = (Widget_t *) w_;
    cairo_set_source_rgb(w->crb, 1, 1, 1);
    cairo_paint(w->crb);
}

int main(int argc, char **argv) {
    /** acces the main struct */
    Xputty app;
    /** init the main struct */
    main_init(&app);
    /** create a Window on default root window */
    Widget_t *w = create_window(&app, DefaultRootWindow(app.dpy), 0, 0, 300, 200);
    /** acces Xlib function */
    XStoreName(app.dpy, w->widget, "Hello world");
    /** overwrite event handler with your own */
    w->func.expose_callback = draw_window;
    /** map the Window to display */
    widget_show_all(w);
    /** run the event loop */
    main_run(&app);
    /** clean up after the event loop is finished */
    main_quit(&app);
    return 0;
}
