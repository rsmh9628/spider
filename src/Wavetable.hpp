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

    simd::float_4 sampleAt(simd::int32_4 wave, simd::int32_4 sample) const {
        return simd::float_4(sampleAt(wave[0], sample[0]), sampleAt(wave[1], sample[1]), sampleAt(wave[2], sample[2]),
                             sampleAt(wave[3], sample[3]));
        // todo: probably a better way of doing this but it's fine for now
    }

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