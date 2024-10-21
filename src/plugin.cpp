#include "plugin.hpp"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;

    // Add modules here
    p->addModel(modelPostHumanFM);

    // Any other plugin initialization may go here.
    // As an alternative, consider lazy-loading assets and lookup tables when
    // your module is created to reduce startup times of Rack.
}
