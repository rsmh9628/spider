#include "Wavetable.hpp"
#include "dr_wav.h"

namespace ph {

Wavetable::Wavetable() {
    waveLength = 1024;
    samples.resize(waveLength * 2);
    for (int i = 0; i < waveLength; i++) {
        float p = i / static_cast<float>(waveLength);
        samples[i] = std::sin(2 * M_PI * p);
    }
    for (int i = 0; i < waveLength; i++) {
        float p = i / static_cast<float>(waveLength);
        samples[waveLength + i] = (p < 0.5f) ? 1 : -1;
    }
    interpolate();
}

void Wavetable::load(const std::string& filename) {
    drwav wav;
    if (!drwav_init_file(&wav, filename.c_str(), nullptr)) {
        throw std::runtime_error("Failed to load wavetable"); // TODO: error handling
    }

    samples.clear();
    samples.resize(wav.totalPCMFrameCount);
    drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, samples.data());
    drwav_uninit(&wav);
}

void Wavetable::interpolate() {
    size_t length = samples.size();

    interpolatedSamples.clear();
    interpolatedSamples.resize(samples.size());

    std::vector<float> timeDomainSamples(waveLength);
    std::vector<float> frequencyDomainSamples(2 * waveLength);
    std::vector<float> interpolatedFrequencyDomainSamples(2 * waveLength);

    dsp::RealFFT fft(waveLength);

    float waveCnt = waveCount();
    for (size_t i = 0; i < waveCnt; i++) {
        // Compute FFT of wave
        for (size_t j = 0; j < waveLength; j++) {
            timeDomainSamples[j] = samples[waveLength * i + j] / waveLength;
        }

        fft.rfft(timeDomainSamples.data(), frequencyDomainSamples.data());

        // TODO: this is taken directly from vcv rack, change it before you push

        // Compute FFT-filtered version of the wave
        for (size_t j = 0; j < waveLength; j++) {
            interpolatedFrequencyDomainSamples[2 * j + 0] = frequencyDomainSamples[2 * j + 0];
            interpolatedFrequencyDomainSamples[2 * j + 1] = frequencyDomainSamples[2 * j + 1];
        }
        fft.irfft(interpolatedFrequencyDomainSamples.data(), &interpolatedSamples[waveLength * i]);
    }
}

} // namespace ph