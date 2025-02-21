#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    ph::initSinTable();
    pluginInstance = p;
    
    p->addModel(modelPostHumanSpider);
}

namespace ph {
    std::vector<float> sinTable(TABLE_SIZE);

    void initSinTable() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            sinTable[i] = std::sin(2.0f * M_PI * i / TABLE_SIZE);
        }
    }

    float sin2pi(float x) {
        if (x < 0.f)
            x = 0;
        if (x > 1.f)
            x = 1;

        x *= TABLE_SIZE;
        int i = (int)x;
        float f = x - i;
        return (1.0f - f) * sinTable[i] + f * sinTable[(i + 1) % TABLE_SIZE];
    }
}