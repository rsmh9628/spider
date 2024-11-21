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
}

void Wavetable::openDialog() {
    char* path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);
    if (!path)
        return;

    std::string pathStr(path);
    std::free(path);
    open(pathStr);
}

} // namespace ph