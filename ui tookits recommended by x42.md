Responding to an explicit question from me about frameworks/libraries/toolkits for plugin UIs, Robin
Gareus [shared his opinion](https://discourse.ardour.org/t/eq10q-not-ardour-8-4/110046/19):

> > *Bruno Unna:*
> >
> > What would you recommend to me for a very simple interface (8 identical rows, each with 7 knobs and one on/off
> > button)?
>
> *Robin Gareus:*
> 
> I highly recommend to use some plugin specific toolkit.
>
> - [GitHub - DISTRHO/DPF](https://github.com/DISTRHO/DPF): DISTRHO Plugin Framework
> - [GitHub - brummer10/XUiDesigner](https://github.com/brummer10/XUiDesigner): A WYSIWYG LV2 GUI/plugin creator tool
> - [GitHub - juce-framework/JUCE](https://github.com/juce-framework/JUCE): JUCE is an open-source cross-platform C++
    application framework for desktop and mobile applications, including VST, VST3, AU, AUv3, LV2 and AAX audio
    plug-ins.
> - [GitHub - sjaehn/BWidgets](https://github.com/sjaehn/BWidgets): Widget toolkit based on Cairo and Pugl
>
> For LV2 plugins most UIs use pugl under the hood: [GitHub - lv2/pugl](https://github.com/lv2/pugl): A minimal portable
> API for embeddable GUIs
