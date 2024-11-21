#pragma once

#include <rack.hpp>
#include "Wavetable.hpp"

using namespace rack;

namespace ph {
class WavetableDisplay : public OpaqueWidget {
public:
    WavetableDisplay();

    void onButton(const ButtonEvent& e) override;
    void createContextMenu();

    void draw(const DrawArgs& args) override;
    void drawLayer(const DrawArgs& args, int layer) override;

    Wavetable* wavetable = nullptr;
    float* wavePos = nullptr;

    int op = 0;
};

} // namespace ph