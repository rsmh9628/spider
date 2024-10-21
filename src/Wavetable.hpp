#pragma once

#include <rack.hpp>

using namespace rack;

namespace ph {

class Wavetable {
public:
    Wavetable();
    void load(const std::string& filename);

    // TODO: Not sure if this is gonna work, we'll try it
    float sampleAt(size_t wave, size_t sample) const {
        return interpolatedSamples[samples.size() + (waveLength * wave) + sample];
    }

    size_t waveCount() const {
        if (waveLength == 0) {
            return 0;
        }
        return samples.size() / waveLength;
    }

    int getWaveLength() const { return waveLength; }

private:
    int waveLength;

    // TODO: probably don't need both
    std::vector<float> samples;
    std::vector<float> interpolatedSamples;

    void interpolate();
};

} // namespace ph