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
#include "BWidgets/BWidgets/CheckBox.hpp"
#include "BWidgets/BWidgets/Text.hpp"
#include "euclidean.h"
#include <iostream>
#include <cstring>

class Euclidean_GUI : public BWidgets::Window {
public:
    explicit Euclidean_GUI(PuglNativeView parentWindow);

    void portEvent(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void *buffer);

    static void valueChangedCallback(BEvents::Event *event);

    LV2UI_Write_Function write_function;
    LV2UI_Controller controller;
    BWidgets::Text beatsLabel;
    BWidgets::Text onsetsLabel;
    BWidgets::Text rotationLabel;
    BWidgets::Text barsLabel;
    BWidgets::Text channelLabel;
    BWidgets::Text noteLabel;
    BWidgets::Text velocityLabel;
    BWidgets::Text generatorLabels[N_GENERATORS];
    BWidgets::CheckBox enabledCheckboxes[N_GENERATORS];
    BWidgets::ValueDial beatsDials[N_GENERATORS];
    BWidgets::ValueDial onsetsDials[N_GENERATORS];
    BWidgets::ValueDial rotationDials[N_GENERATORS];
    BWidgets::ValueDial barsDials[N_GENERATORS];
    BWidgets::ValueDial channelDials[N_GENERATORS];
    BWidgets::ValueDial noteDials[N_GENERATORS];
    BWidgets::ValueDial velocityDials[N_GENERATORS];
};

Euclidean_GUI::Euclidean_GUI(PuglNativeView parentWindow) :
        BWidgets::Window(800, 800, parentWindow, BUtilities::Urid::urid(EUCLIDEAN_UI_URI), "Euclidean Rhythms", true,
                         PUGL_MODULE, 0),
        write_function(nullptr), controller(nullptr),
        beatsLabel(BWidgets::Text("beats")),
        onsetsLabel(BWidgets::Text("onsets")),
        rotationLabel(BWidgets::Text("rotation")),
        barsLabel(BWidgets::Text("size in bars")),
        channelLabel(BWidgets::Text("MIDI channel")),
        noteLabel(BWidgets::Text("MIDI note")),
        velocityLabel(BWidgets::Text("MIDI velocity")),
        generatorLabels{
                {BWidgets::Text("gen 0")},
                {BWidgets::Text("gen 1")},
                {BWidgets::Text("gen 2")},
                {BWidgets::Text("gen 3")},
                {BWidgets::Text("gen 4")},
                {BWidgets::Text("gen 5")},
                {BWidgets::Text("gen 6")},
                {BWidgets::Text("gen 7")},
        },
        enabledCheckboxes{
                {BWidgets::CheckBox(true)},
                {BWidgets::CheckBox(true)},
                {BWidgets::CheckBox(true)},
                {BWidgets::CheckBox(true)},
                {BWidgets::CheckBox(true)},
                {BWidgets::CheckBox(true)},
                {BWidgets::CheckBox(true)},
                {BWidgets::CheckBox(true)},
        },
        beatsDials{
                {BWidgets::ValueDial(8, 2, 64, 1)},
                {BWidgets::ValueDial(8, 2, 64, 1)},
                {BWidgets::ValueDial(8, 2, 64, 1)},
                {BWidgets::ValueDial(8, 2, 64, 1)},
                {BWidgets::ValueDial(8, 2, 64, 1)},
                {BWidgets::ValueDial(8, 2, 64, 1)},
                {BWidgets::ValueDial(8, 2, 64, 1)},
                {BWidgets::ValueDial(8, 2, 64, 1)},
        },
        onsetsDials{
                {BWidgets::ValueDial(5, 0, 64, 1)},
                {BWidgets::ValueDial(5, 0, 64, 1)},
                {BWidgets::ValueDial(5, 0, 64, 1)},
                {BWidgets::ValueDial(5, 0, 64, 1)},
                {BWidgets::ValueDial(5, 0, 64, 1)},
                {BWidgets::ValueDial(5, 0, 64, 1)},
                {BWidgets::ValueDial(5, 0, 64, 1)},
                {BWidgets::ValueDial(5, 0, 64, 1)},
        },
        rotationDials{
                {BWidgets::ValueDial(0, -32, 31, 1)},
                {BWidgets::ValueDial(0, -32, 31, 1)},
                {BWidgets::ValueDial(0, -32, 31, 1)},
                {BWidgets::ValueDial(0, -32, 31, 1)},
                {BWidgets::ValueDial(0, -32, 31, 1)},
                {BWidgets::ValueDial(0, -32, 31, 1)},
                {BWidgets::ValueDial(0, -32, 31, 1)},
                {BWidgets::ValueDial(0, -32, 31, 1)},
        },
        barsDials{
                {BWidgets::ValueDial(1, 1, 8, 1)},
                {BWidgets::ValueDial(1, 1, 8, 1)},
                {BWidgets::ValueDial(1, 1, 8, 1)},
                {BWidgets::ValueDial(1, 1, 8, 1)},
                {BWidgets::ValueDial(1, 1, 8, 1)},
                {BWidgets::ValueDial(1, 1, 8, 1)},
                {BWidgets::ValueDial(1, 1, 8, 1)},
                {BWidgets::ValueDial(1, 1, 8, 1)},
        },
        channelDials{
                {BWidgets::ValueDial(10, 1, 16, 1)},
                {BWidgets::ValueDial(10, 1, 16, 1)},
                {BWidgets::ValueDial(10, 1, 16, 1)},
                {BWidgets::ValueDial(10, 1, 16, 1)},
                {BWidgets::ValueDial(10, 1, 16, 1)},
                {BWidgets::ValueDial(10, 1, 16, 1)},
                {BWidgets::ValueDial(10, 1, 16, 1)},
                {BWidgets::ValueDial(10, 1, 16, 1)},
        },
        noteDials{
                {BWidgets::ValueDial(48, 0, 127, 1)},
                {BWidgets::ValueDial(48, 0, 127, 1)},
                {BWidgets::ValueDial(48, 0, 127, 1)},
                {BWidgets::ValueDial(48, 0, 127, 1)},
                {BWidgets::ValueDial(48, 0, 127, 1)},
                {BWidgets::ValueDial(48, 0, 127, 1)},
                {BWidgets::ValueDial(48, 0, 127, 1)},
                {BWidgets::ValueDial(48, 0, 127, 1)},
        },
        velocityDials{
                {BWidgets::ValueDial(64, 0, 127, 1)},
                {BWidgets::ValueDial(64, 0, 127, 1)},
                {BWidgets::ValueDial(64, 0, 127, 1)},
                {BWidgets::ValueDial(64, 0, 127, 1)},
                {BWidgets::ValueDial(64, 0, 127, 1)},
                {BWidgets::ValueDial(64, 0, 127, 1)},
                {BWidgets::ValueDial(64, 0, 127, 1)},
                {BWidgets::ValueDial(64, 0, 127, 1)},
        } {
    beatsLabel.moveTo(50 + 90 * 1, 40);
    add(&beatsLabel);
    onsetsLabel.moveTo(50 + 90 * 2, 40);
    add(&onsetsLabel);
    rotationLabel.moveTo(36 + 90 * 3, 40);
    add(&rotationLabel);
    barsLabel.moveTo(30 + 90 * 4, 40);
    add(&barsLabel);
    channelLabel.moveTo(30 + 90 * 5, 40);
    add(&channelLabel);
    noteLabel.moveTo(30 + 90 * 6, 40);
    add(&noteLabel);
    velocityLabel.moveTo(30 + 90 * 7, 40);
    add(&velocityLabel);
    for (int i = 0; i < N_GENERATORS; ++i) {
        generatorLabels[i].moveTo(20, 70 + 24 + 90 * i);
        add(&generatorLabels[i]);

        enabledCheckboxes[i].setValue(i == 0);
        enabledCheckboxes[i].moveTo(72, 70 + 20 + 90 * i);
        enabledCheckboxes[i].setWidth(16);
        enabledCheckboxes[i].setHeight(16);
        add(&enabledCheckboxes[i]);
        enabledCheckboxes[i].setCallbackFunction(BEvents::Event::EventType::valueChangedEvent,
                                                 Euclidean_GUI::valueChangedCallback);

        beatsDials[i].moveTo(30 + 90 * 1, 70 + 90 * i);
        beatsDials[i].setWidth(80);
        beatsDials[i].setHeight(80);
        beatsDials[i].setClickable(false);
        add(&beatsDials[i]);
        beatsDials[i].setCallbackFunction(BEvents::Event::EventType::valueChangedEvent,
                                          Euclidean_GUI::valueChangedCallback);

        onsetsDials[i].moveTo(30 + 90 * 2, 70 + 90 * i);
        onsetsDials[i].setWidth(80);
        onsetsDials[i].setHeight(80);
        onsetsDials[i].setClickable(false);
        add(&onsetsDials[i]);
        onsetsDials[i].setCallbackFunction(BEvents::Event::EventType::valueChangedEvent,
                                           Euclidean_GUI::valueChangedCallback);

        rotationDials[i].moveTo(30 + 90 * 3, 70 + 90 * i);
        rotationDials[i].setWidth(80);
        rotationDials[i].setHeight(80);
        rotationDials[i].setClickable(false);
        add(&rotationDials[i]);
        rotationDials[i].setCallbackFunction(BEvents::Event::EventType::valueChangedEvent,
                                             Euclidean_GUI::valueChangedCallback);

        barsDials[i].moveTo(30 + 90 * 4, 70 + 90 * i);
        barsDials[i].setWidth(80);
        barsDials[i].setHeight(80);
        barsDials[i].setClickable(false);
        add(&barsDials[i]);
        barsDials[i].setCallbackFunction(BEvents::Event::EventType::valueChangedEvent,
                                         Euclidean_GUI::valueChangedCallback);

        channelDials[i].moveTo(30 + 90 * 5, 70 + 90 * i);
        channelDials[i].setWidth(80);
        channelDials[i].setHeight(80);
        channelDials[i].setClickable(false);
        add(&channelDials[i]);
        channelDials[i].setCallbackFunction(BEvents::Event::EventType::valueChangedEvent,
                                            Euclidean_GUI::valueChangedCallback);

        noteDials[i].moveTo(30 + 90 * 6, 70 + 90 * i);
        noteDials[i].setWidth(80);
        noteDials[i].setHeight(80);
        noteDials[i].setClickable(false);
        add(&noteDials[i]);
        noteDials[i].setCallbackFunction(BEvents::Event::EventType::valueChangedEvent,
                                         Euclidean_GUI::valueChangedCallback);

        velocityDials[i].moveTo(30 + 90 * 7, 70 + 90 * i);
        velocityDials[i].setWidth(80);
        velocityDials[i].setHeight(80);
        velocityDials[i].setClickable(false);
        add(&velocityDials[i]);
        velocityDials[i].setCallbackFunction(BEvents::Event::EventType::valueChangedEvent,
                                             Euclidean_GUI::valueChangedCallback);
    }
}

void Euclidean_GUI::portEvent(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void *buffer) {
    if (format == 0) {
        auto *pval = (float *) buffer;
        unsigned short generator = (port_index - 2) / N_PARAMETERS;
        unsigned short widget_offset = (port_index - 2) % N_PARAMETERS;
        switch (widget_offset) {
            case ENABLED_IDX:
                enabledCheckboxes[generator].setValue(*pval > 0);
                break;
            case BEATS_IDX:
                beatsDials[generator].setValue(*pval);
                break;
            case ONSETS_IDX:
                onsetsDials[generator].setValue(*pval);
                break;
            case ROTATION_IDX:
                rotationDials[generator].setValue(*pval);
                break;
            case BARS_IDX:
                barsDials[generator].setValue(*pval);
                break;
            case CHANNEL_IDX:
                channelDials[generator].setValue(*pval);
                break;
            case NOTE_IDX:
                noteDials[generator].setValue(*pval);
                break;
            case VELOCITY_IDX:
                velocityDials[generator].setValue(*pval);
                break;
            default:
                std::cout << "received a non-understood port event for port_index " << port_index << "\n";
                break;
        }
    }
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

            if (widget == (BWidgets::Widget *) &ui->beatsDials) {
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