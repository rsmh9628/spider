#pragma once

#include <rack.hpp>

using namespace rack;

namespace ph {

class Wavetable {
public:
    Wavetable();

    void init();
    void open(const std::string& filename);
    void openDialog();

    // TODO: Not sure if this is gonna work, we'll try it
    float sampleAt(size_t wave, size_t sample) const { return interpolatedSamples[(wavelength * wave) + sample]; }

    size_t waveCount() const {
        if (wavelength == 0) {
            return 0;
        }
        return samples.size() / wavelength;
    }
    int wavelength;

private:
    // TODO: probably don't need both
    std::vector<float> samples;
    std::vector<float> interpolatedSamples;

    void interpolate();
};

} // namespace ph