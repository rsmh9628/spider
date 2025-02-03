#pragma once

#include <rack.hpp>

using namespace rack;

namespace ph {
class WaveDisplay : public OpaqueWidget {
public:
    WaveDisplay();

    void draw(const DrawArgs& args) override;
    void drawLayer(const DrawArgs& args, int layer) override;

    int op = 0;
};

} // namespace ph