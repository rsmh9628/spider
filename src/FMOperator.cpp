#include "FMOperator.hpp"

using namespace rack;

namespace ph {

void FMOperator::generate(const Args& args, float freq) {
    // TODO: Other waveforms for carrier

    float level = getParam(PARAM_LEVEL).getValue();

    phase_ += freq * args.sampleTime;
    if (phase_ >= 1.f) {
        phase_ -= 1.f;
    }

    out_ = level * sinf(2.f * M_PI * phase_);
}

void FMOperator::modulate(const Args& args, const FMOperator& modulator,
                          float freq) {
    // float ratio = modulator.ratio();
    // float depth = modulator.depth();
    // float feedback = modulator.feedback();
    // float level = modulator.level();
    float level = getParam(PARAM_LEVEL).getValue();

    phase_ += freq * args.sampleTime;
    phase_ += modulator.depth() * modulator.out();

    out_ = level * sinf(2.f * M_PI * phase_);
}

} // namespace ph