#pragma once
#include <rack.hpp>

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelPostHumanSpider;

namespace ph {
    const NVGcolor OPERATOR_COLOURS[] = {nvgRGB(255, 31, 57), nvgRGB(255, 75, 31), nvgRGB(255, 184, 51),
                                         nvgRGB(44, 252, 168), nvgRGB(0, 147, 240), nvgRGB(246, 73, 239)};


    constexpr int TABLE_SIZE = 256;
    extern std::vector<float> sinTable;

    void initSinTable(); 
    float sin2pi(float x);
}