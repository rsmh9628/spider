#include "plugin.hpp"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;

    // Add modules here
    p->addModel(modelPostHumanSpider);

    // Any other plugin initialization may go here.
    // As an alternative, consider lazy-loading assets and lookup tables when
    // your module is created to reduce startup times of Rack.

    ph::initSinTable();
}

namespace ph {
    std::vector<float> sinTable(TABLE_SIZE);

    void initSinTable() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            sinTable[i] = std::sin(2.0f * M_PI * i / TABLE_SIZE);
        }
    }

    float sin2pi(float x) {
        x *= TABLE_SIZE;
        int i = (int)x;
        float f = x - i;
        return (1.0f - f) * sinTable[i] + f * sinTable[(i + 1) % TABLE_SIZE];
    }
}