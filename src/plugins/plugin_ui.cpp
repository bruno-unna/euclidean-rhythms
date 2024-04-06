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

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include "BWidgets/BEvents/ExposeEvent.hpp"
#include "BWidgets/BWidgets/ValueDial.hpp"
#include "euclidean.h"
#include <iostream>
#include <cstring>

class Euclidean_GUI : public BWidgets::Window {
public:
    explicit Euclidean_GUI(PuglNativeView parentWindow);

    void portEvent(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void *buffer);

    void onConfigureRequest(BEvents::Event *event) override;

    static void valueChangedCallback(BEvents::Event *event);

    LV2UI_Write_Function write_function;
    LV2UI_Controller controller;
    BWidgets::ValueDial dial[1];
};

Euclidean_GUI::Euclidean_GUI(PuglNativeView parentWindow) :
        BWidgets::Window(100, 100, parentWindow, BUtilities::Urid::urid(EUCLIDEAN_UI_URI), "Euclidean Rhythms", true,
                         PUGL_MODULE, 0),
        write_function(nullptr), controller(nullptr),
        dial{{BWidgets::ValueDial(10, 10, 80, 80, 8, 2, 64.0, 1.0)}} {
    dial[0].setClickable(false);
    add(&dial[0]);
    dial[0].setCallbackFunction(BEvents::Event::EventType::valueChangedEvent, Euclidean_GUI::valueChangedCallback);
}

void Euclidean_GUI::portEvent(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void *buffer) {
    if ((format == 0) && (port_index == 3)) {
        auto *pval = (float *) buffer;
        dial[0].setValue(*pval);
    }
}

void Euclidean_GUI::onConfigureRequest(BEvents::Event *event) {
    Window::onConfigureRequest(event);

    double sz = (getWidth() > getHeight() ? getHeight() : getWidth()) / 100;
    dial[0].label.setFont(BStyles::Font("Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL, 12.0 * sz));
    dial[0].moveTo(10 * sz, 10 * sz);
    dial[0].resize(80 * sz, 80 * sz);
}

void Euclidean_GUI::valueChangedCallback(BEvents::Event *event) {
    if ((event) && (event->getWidget())) {
        BWidgets::Widget *widget = event->getWidget();
        if (!widget) return;
        auto *vd = dynamic_cast<BWidgets::ValueableTyped<double> *>(widget);
        if (!vd) return;
        float value = vd->getValue();

        if (widget->getMainWindow()) {
            auto *ui = (Euclidean_GUI *) widget->getMainWindow();

            if (widget == (BWidgets::Widget *) &ui->dial) {
                ui->write_function(ui->controller, 3, sizeof(float), 0, &value);
            }
        }
    }
}

static LV2UI_Handle instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri, const char *bundle_path,
                                LV2UI_Write_Function write_function, LV2UI_Controller controller, LV2UI_Widget *widget,
                                const LV2_Feature *const *features) {
    PuglNativeView parentWindow = 0;

    if (strcmp(plugin_uri, EUCLIDEAN_URI) != 0) {
        std::cerr << "Euclidean_GUI: This GUI does not support plugin with URI " << plugin_uri << std::endl;
        return nullptr;
    }

    for (int i = 0; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_UI__parent)) parentWindow = (PuglNativeView) features[i]->data;
    }
    if (parentWindow == 0) std::cerr << "Euclidean_GUI: No parent window.\n";

    auto *ui = new Euclidean_GUI(parentWindow);

    ui->controller = controller;
    ui->write_function = write_function;
    *widget = (LV2UI_Widget) ui->getNativeView();
    return (LV2UI_Handle) ui;
}

static void cleanup(LV2UI_Handle ui) {
    auto *pluginGui = (Euclidean_GUI *) ui;
    delete pluginGui;
}

static void portEvent(LV2UI_Handle ui, uint32_t port_index, uint32_t buffer_size, uint32_t format, const void *buffer) {
    auto *pluginGui = (Euclidean_GUI *) ui;
    pluginGui->portEvent(port_index, buffer_size, format, buffer);
}

static int callIdle(LV2UI_Handle ui) {
    auto *pluginGui = (Euclidean_GUI *) ui;
    pluginGui->handleEvents();
    return 0;
}

static const LV2UI_Idle_Interface idle = {callIdle};

static const void *extensionData(const char *uri) {
    if (!strcmp(uri, LV2_UI__idleInterface)) return &idle;
    else return nullptr;
}


static const LV2UI_Descriptor guiDescriptor = {
        EUCLIDEAN_UI_URI,
        instantiate,
        cleanup,
        portEvent,
        extensionData
};

// LV2 Symbol Export
LV2_SYMBOL_EXPORT const LV2UI_Descriptor *lv2ui_descriptor(uint32_t index) {
    if (index == 0)
        return &guiDescriptor;
    return nullptr;
}