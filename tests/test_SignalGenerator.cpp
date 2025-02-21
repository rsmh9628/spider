#include "test_constants.hpp"
#include <catch2/catch_all.hpp>
#include "../src/SignalGenerator.hpp"

using namespace Catch;
using namespace Catch::Matchers;
using namespace Catch::Generators;

namespace ph {

class SignalGeneratorFixture {
public:
    SpiderSignalGenerator gen;

    SignalGeneratorFixture() {}

    float runSignalGenTest(int samples, float freq, float wavePos, float phase = 0) {
        float expPhase = phase;

        for (int i = 0; i < samples; ++i) {
            float sample = gen.generate(SAMPLE_TIME, freq, wavePos);

            expPhase = incrementExpPhase(expPhase, freq);
            float expected = generateExpWaveform(expPhase, wavePos);

            REQUIRE_THAT(sample, WithinAbs(expected, 0.000001));
        }

        return expPhase;
    }

    float incrementExpPhase(float phase, float freq) {
        phase += SAMPLE_TIME * freq;
        if (phase > 1.f)
            phase -= 1.f;
        if (phase < -1.f)
            phase += 1.f;

        return phase;
    }

    float generateExpWaveform(float phase, float wavePos) {
        float normalisedPhase = (phase < 0.f) ? phase + 1.f : phase;

        if (wavePos == 0.f)
            return sin2pi(normalisedPhase);
        if (wavePos == 0.25f)
            return triangle(phase);
        if (wavePos == 0.5f)
            return saw(phase);
        if (wavePos == 0.75f)
            return square(phase);
        if (wavePos == 1.f)
            return sin2pi(normalisedPhase);
        return 0.f; // did a bad thing
    }
};

TEST_CASE_METHOD(SignalGeneratorFixture, "Sin LUT is initialised", "[SignalGenerator]") {
    REQUIRE_FALSE(ph::sinTable.empty());
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Sin LUT is within 1e-3 of sin(2pi * x) for 48000 samples",
                 "[SignalGenerator]") {
    float expPhase = 0.f;
    float freq = 1.f;

    for (int i = 0; i < 48000; ++i) {
        expPhase += SAMPLE_TIME * freq;
        if (expPhase > 1.f)
            expPhase -= 1.f;
        if (expPhase < -1.f)
            expPhase += 1.f;

        float normalisedPhase = (expPhase < 0.f) ? expPhase + 1.f : expPhase;

        float expected = std::sin(2.f * M_PI * expPhase);
        float actual = sin2pi(normalisedPhase);

        REQUIRE_THAT(actual, WithinAbs(expected, 0.0001));

        expPhase += 1.f / 48000.f;
    }
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator phase is 0 on init", "[SignalGenerator]") {
    ph::SpiderSignalGenerator gen;
    REQUIRE(gen.phase == 0.f);
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator phase is 0 on reset", "[SignalGenerator]") {
    ph::SpiderSignalGenerator gen;
    gen.phase = 0.5f;
    gen.reset();
    REQUIRE(gen.phase == 0.f);
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator generates sine wave", "[SignalGenerator]") {
    SECTION("wavePos == 0") {
        runSignalGenTest(256, 440.f, 0.f);
    }

    SECTION("wavePos == 1") {
        runSignalGenTest(256, 440.f, 1.f);
    }
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator generates triangle wave", "[SignalGenerator]") {
    runSignalGenTest(256, 440.f, 0.25f);
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator generates saw wave", "[SignalGenerator]") {
    runSignalGenTest(256, 440.f, 0.5f);
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator generates square wave", "[SignalGenerator]") {
    runSignalGenTest(256, 440.f, 0.75f);
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator blends sin and triangle", "[SignalGenerator]") {
    float expPhase = 0.f;
    const float freq = 440.f;
    for (int i = 0; i < 512; ++i) {
        expPhase = incrementExpPhase(expPhase, freq);
        float expected = crossfade(sin2pi(normalisePhase(expPhase)), triangle(expPhase), 0.5f);
        float sample = gen.generate(SAMPLE_TIME, freq, 0.125f);
        REQUIRE_THAT(sample, WithinAbs(expected, 0.000001));
    }
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator blends triangle and saw", "[SignalGenerator]") {
    float expPhase = 0.f;
    const float freq = 440.f;
    for (int i = 0; i < 512; ++i) {
        expPhase = incrementExpPhase(expPhase, freq);
        float expected = crossfade(triangle(expPhase), saw(expPhase), 0.5f);
        float sample = gen.generate(SAMPLE_TIME, freq, 0.375);
        REQUIRE_THAT(sample, WithinAbs(expected, 0.000001));
    }
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator blends saw and square", "[SignalGenerator]") {
    float expPhase = 0.f;
    const float freq = 440.f;
    for (int i = 0; i < 512; ++i) {
        expPhase = incrementExpPhase(expPhase, freq);
        float expected = crossfade(saw(expPhase), square(expPhase), 0.5f);
        float sample = gen.generate(SAMPLE_TIME, freq, 0.625);
        REQUIRE_THAT(sample, WithinAbs(expected, 0.000001));
    }
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator blends square and sin", "[SignalGenerator]") {
    float expPhase = 0.f;
    const float freq = 440.f;
    for (int i = 0; i < 512; ++i) {
        expPhase = incrementExpPhase(expPhase, freq);
        float expected = crossfade(square(expPhase), sin2pi(normalisePhase(expPhase)), 0.5f);
        float sample = gen.generate(SAMPLE_TIME, freq, 0.875);
        REQUIRE_THAT(sample, WithinAbs(expected, 0.000001));
    }
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator handles time-varying frequency", "[SignalGenerator]") {
    float phase = runSignalGenTest(128, 440.f, 0.f);
    phase = runSignalGenTest(64, 5.f, 0.f, phase);
    phase = runSignalGenTest(256, dsp::FREQ_C4, 0.f, phase);
    phase = runSignalGenTest(512, 18000.f, 0.f, phase);
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator handles time-varying waveform", "[SignalGenerator]") {
    float phase = runSignalGenTest(128, 440.f, 0.f);
    phase = runSignalGenTest(64, 440.f, 0.5f, phase);
    phase = runSignalGenTest(512, 440.f, 0.25f, phase);
    phase = runSignalGenTest(256, 440.f, 1.0f, phase);
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator handles zero frequency", "[SignalGenerator]") {
    runSignalGenTest(256, 0.f, 0.f);
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator handles high frequency", "[SignalGenerator]") {
    runSignalGenTest(256, 1e6f, 0.f);
}

TEST_CASE_METHOD(SignalGeneratorFixture, "Signal generator handles different sample rates", "[SignalGenerator]") {
    ph::SpiderSignalGenerator gen;
    const float freq = 440.f;
    const float sampleRates[] = {44100.f, 48000.f, 96000.f};
    float expPhase = 0.f;

    for (float sampleRate : sampleRates) {
        const float sampleTime = 1.f / sampleRate;
        for (int i = 0; i < 512; ++i) {
            float sample = gen.generate(sampleTime, freq, 0.f);
            expPhase += sampleTime * freq;
            if (expPhase > 1.f)
                expPhase -= 1.f;
            if (expPhase < -1.f)
                expPhase += 1.f;
            float expected = sin2pi(expPhase);
            REQUIRE_THAT(sample, WithinAbs(expected, 0.000001));
        }
    }
}

} // namespace ph