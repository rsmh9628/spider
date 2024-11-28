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
    json_t* toJson() const;
    void fromJson(json_t* rootJ);

    float sampleAt(size_t wave, size_t sample) const {
        if (sample >= samples.size()) {
            return 0.f;
        }
        return samples[(wavelength * wave) + sample];
    }

    size_t waveCount() const {
        if (wavelength == 0) {
            return 0;
        }
        return samples.size() / wavelength;
    }

    size_t sampleCount() const { return samples.size(); }

    int wavelength;

private:
    std::string filename = "";
    std::vector<float> samples;
};

} // namespace ph