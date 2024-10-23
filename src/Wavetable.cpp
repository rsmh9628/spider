#include "Wavetable.hpp"
#include "dr_wav.h"
#include <osdialog.h>

namespace ph {

Wavetable::Wavetable() {
    init();
}

void Wavetable::init() {
    wavelength = 2048;
    samples.resize(wavelength * 2);
    for (int i = 0; i < wavelength; i++) {
        float p = i / static_cast<float>(wavelength);
        samples[i] = std::sin(2 * M_PI * p);
    }
    for (int i = 0; i < wavelength; i++) {
        float p = i / static_cast<float>(wavelength);
        samples[wavelength + i] = (p < 0.5f) ? 1 : -1;
    }
    interpolate();
}

void Wavetable::open(const std::string& filename) {
    drwav wav;
    if (!drwav_init_file(&wav, filename.c_str(), nullptr)) {
        throw std::runtime_error("Failed to open wavetable"); // TODO: error handling
    }

    samples.clear();
    samples.resize(wav.totalPCMFrameCount);
    drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, samples.data());
    drwav_uninit(&wav);
    interpolate();
}

void Wavetable::openDialog() {
    char* path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);
    if (!path)
        return;

    std::string pathStr(path);
    std::free(path);
    open(pathStr);
}

void Wavetable::interpolate() {
    size_t length = samples.size();

    interpolatedSamples.clear();
    interpolatedSamples.resize(samples.size());

    std::vector<float> timeDomainSamples(wavelength);
    std::vector<float> frequencyDomainSamples(2 * wavelength);
    std::vector<float> interpolatedFrequencyDomainSamples(2 * wavelength);

    dsp::RealFFT fft(wavelength);

    float waveCnt = waveCount();
    for (size_t i = 0; i < waveCnt; i++) {
        // Compute FFT of wave
        for (size_t j = 0; j < wavelength; j++) {
            timeDomainSamples[j] = samples[wavelength * i + j] / wavelength;
        }

        fft.rfft(timeDomainSamples.data(), frequencyDomainSamples.data());

        // TODO: this is taken directly from vcv rack, change it before you push

        // Compute FFT-filtered version of the wave
        for (size_t j = 0; j < wavelength; j++) {
            interpolatedFrequencyDomainSamples[2 * j + 0] = frequencyDomainSamples[2 * j + 0];
            interpolatedFrequencyDomainSamples[2 * j + 1] = frequencyDomainSamples[2 * j + 1];
        }
        fft.irfft(interpolatedFrequencyDomainSamples.data(), &interpolatedSamples[wavelength * i]);
    }
}

} // namespace ph