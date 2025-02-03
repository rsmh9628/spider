#pragma once
#include <rack.hpp>

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelPostHumanSpider;

namespace ph {
    constexpr int TABLE_SIZE = 256;
    extern std::vector<float> sinTable;

    void initSinTable(); 
    float sin2pi(float x);
}