#ifndef PH_SIGNAL_GENERATOR_HPP
#define PH_SIGNAL_GENERATOR_HPP

#include <rack.hpp>
#include "plugin.hpp"

using namespace rack;

namespace ph {

inline float triangle(float x) {
    return 4.f * std::abs(std::round(x) - x) - 1.f;
}
inline float saw(float x) {
    return 2.f * (x - std::round(x));
}
inline float square(float x) {
    return (x < 0.5f) ? 1.f : -1.f;
}

// Normalise phase to [0,1] for sin2pi
inline float normalisePhase(float phase) {
    return (phase < 0.f) ? phase + 1.f : phase;
}

struct SpiderSignalGenerator {
    float generate(float sampleTime, float freq, float wavePos, float phaseMod = 0) {
        phase += freq * sampleTime;

        if (phase > 1.f)
            phase -= 1.f;
        if (phase < -1.f)
            phase += 1.f;

        float normalisedPM = normalisePhase(phase + (phaseMod / (2 * M_PI)));
        float PM = phase + phaseMod / (2 * M_PI);
        float blend = 0.f;
        if (wavePos <= 0.25f) {
            blend = crossfade(sin2pi(normalisedPM), triangle(phase), wavePos / 0.25f);
        } else if (wavePos <= 0.5f) {
            blend = crossfade(triangle(PM), saw(PM), (wavePos - 0.25f) / 0.25f);
        } else if (wavePos <= 0.75f) {
            blend = crossfade(saw(PM), square(PM), (wavePos - 0.5f) / 0.25f);
        } else {
            blend = crossfade(square(PM), sin2pi(normalisedPM), (wavePos - 0.75f) / 0.25f);
        }

        return blend;
    }

    void reset() { phase = 0.f; }

    float phase = 0.f;
};

} // namespace ph

#endif // PH_SIGNAL_GENERATOR_HPP
