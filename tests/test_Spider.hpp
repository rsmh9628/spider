#ifndef PH_TEST_SPIDER_HPP
#define PH_TEST_SPIDER_HPP

#include "test_constants.hpp"
#include <rack.hpp>
#include <catch2/catch_all.hpp>
#include <memory>
#include <iostream>
#include <iomanip>
#include <limits>

using namespace rack;
using namespace Catch;
using namespace Catch::Matchers;
using namespace Catch::Generators;

float freqToSemitone(float freq) {
    return 12.f * std::log2(freq / dsp::FREQ_C4);
}

namespace ph {
using Catch::Generators::random;

class SpiderFixture {
public:
    std::unique_ptr<Spider> spider;

    int sampleRate = SAMPLES_PER_SECOND;

    SpiderFixture() { spider = std::make_unique<Spider>(); }

    void doProcess(int samples, std::optional<std::function<void()>> postProcessFunction = std::nullopt,
                   std::optional<std::function<void()>> preProcessFunction = std::nullopt) {
        int frame = 0;

        Module::ProcessArgs processArgs;
        processArgs.sampleRate = sampleRate;
        processArgs.sampleTime = 1.f / sampleRate;
        processArgs.frame = frame;

        while (frame < samples) {
            if (preProcessFunction)
                (*preProcessFunction)();

            spider->process(processArgs);
            frame++;
            processArgs.frame = frame;

            if (postProcessFunction)
                (*postProcessFunction)();
        }
    }
};

TEST_CASE_METHOD(SpiderFixture, "No output when all levels are zero") {
    SECTION("No operators are carriers") {
        doProcess(256, [&] {
            REQUIRE(spider->getOutput(Spider::AUDIO_OUTPUT).getVoltage() == 0.f);
        });
    }

    SECTION("One operator is a carrier") {
        spider->getParam(Spider::LEVEL_PARAMS + 0).setValue(0.f);
        spider->carriers[0] = true;
        doProcess(256, [&] {
            REQUIRE(spider->getOutput(Spider::AUDIO_OUTPUT).getVoltage() == 0.f);
        });
    }

    SECTION("All operators are carriers") {
        for (int i = 0; i < 6; ++i) {
            spider->getParam(Spider::LEVEL_PARAMS + i).setValue(0.f);
            spider->carriers[i] = true;
        }
        doProcess(256, [&] {
            REQUIRE(spider->getOutput(Spider::AUDIO_OUTPUT).getVoltage() == 0.f);
        });
    }
}

TEST_CASE_METHOD(SpiderFixture, "Non-zero output when at least one level is non-zero") {
    SECTION("One operator is a carrier") {
        spider->getParam(Spider::LEVEL_PARAMS + 0).setValue(1.f);
        spider->carriers[0] = true;
        doProcess(256, [&] {
            REQUIRE(spider->getOutput(Spider::AUDIO_OUTPUT).getVoltage() != 0.f);
        });
    }

    SECTION("All operators are carriers") {
        for (int i = 0; i < 6; ++i) {
            spider->getParam(Spider::LEVEL_PARAMS + i).setValue(1.f);
            spider->carriers[i] = true;
        }
        doProcess(256, [&] {
            REQUIRE(spider->getOutput(Spider::AUDIO_OUTPUT).getVoltage() != 0.f);
        });
    }
}

TEST_CASE_METHOD(SpiderFixture, "Output is clamped to -5V, +5V") {
    // Turn all operators up to max
    for (int i = 0; i < 6; ++i) {
        spider->carriers[i] = true;
        spider->getParam(Spider::LEVEL_PARAMS + i).setValue(1.f);
    }
    doProcess(256, [&] {
        float voltage = spider->getOutput(Spider::AUDIO_OUTPUT).getVoltage();
        REQUIRE(voltage >= -5.f);
        REQUIRE(voltage <= 5.f);
    });
}

TEST_CASE_METHOD(SpiderFixture, "Audio output is brickwalled sum of carriers") {
    spider->carriers[0] = true;
    spider->carriers[1] = true;
    spider->carriers[2] = true;

    spider->getParam(Spider::LEVEL_PARAMS + 0).setValue(1.f);
    spider->getParam(Spider::LEVEL_PARAMS + 1).setValue(1.f);
    spider->getParam(Spider::LEVEL_PARAMS + 2).setValue(1.f);

    std::array<SpiderSignalGenerator, 3> gens;

    doProcess(256, [&] {
        float expected = 0.f;
        for (int i = 0; i < 3; ++i) {
            expected += gens[i].generate(SAMPLE_TIME, dsp::FREQ_C4, 0.f);
        }

        if (expected > 1.f)
            expected = 1.f;
        if (expected < -1.f)
            expected = -1.f;

        expected *= 5.f;

        float actual = spider->getOutput(Spider::AUDIO_OUTPUT).getVoltage();
        REQUIRE_THAT(actual, WithinAbs(expected, 0.000001));
    });
}

TEST_CASE_METHOD(SpiderFixture, "Frequency parameter varies operator frequency correctly", "[Integration]") {
    int op = GENERATE(0, 1, 2, 3, 4, 5);

    spider->getParam(Spider::LEVEL_PARAMS + op).setValue(1.f);
    spider->carriers[op] = true;

    float freqParam = GENERATE(take(4, random(-80.f, 80.f)));
    spider->getParam(Spider::FREQ_PARAM).setValue(freqParam);

    float expectedFreq = dsp::FREQ_C4 * dsp::exp2_taylor5(freqParam / 12.f);

    doProcess(256, [&] {
        float freq = spider->freqs[op];
        REQUIRE_THAT(freq, WithinAbs(expectedFreq, 0.000001));
    });
}

TEST_CASE_METHOD(SpiderFixture, "Monophonic 1/VOct input varies operator frequency correctly", "[Integration]") {
    int op = GENERATE(0, 1, 2, 3, 4, 5);

    spider->getParam(Spider::LEVEL_PARAMS + op).setValue(1.f);
    spider->carriers[op] = true;

    float pitchInput = GENERATE(take(4, random(0.f, 10.f)));
    spider->getInput(Spider::VOCT_INPUT).setVoltage(pitchInput);

    float expectedFreq = dsp::FREQ_C4 * dsp::exp2_taylor5(pitchInput);

    doProcess(256, [&] {
        float freq = spider->freqs[op];
        REQUIRE_THAT(freq, WithinAbs(expectedFreq, 0.000001));
    });
}

TEST_CASE_METHOD(SpiderFixture, "Polyphonic 1V/Oct input sets channels") {
    int channels = GENERATE(range(1, 16));

    // UNSTABLE API but setChannels() checks for whether the port is connected and we're faking it...
    spider->getInput(Spider::VOCT_INPUT).channels = channels;

    doProcess(1, [&] {
        REQUIRE(spider->channels == channels);
    });
}

TEST_CASE_METHOD(SpiderFixture, "Polyphonic 1V/Oct input varies operator frequency correctly", "[Integration]") {
    int op = GENERATE(0, 1, 2, 3, 4, 5);
    int channels = GENERATE(range(2, 16));

    spider->getParam(Spider::LEVEL_PARAMS + op).setValue(1.f);
    spider->carriers[op] = true;

    std::vector<float> expectedFreqs(channels);

    // UNSTABLE API but setChannels() checks for whether the port is connected and we're faking it...
    spider->getInput(Spider::VOCT_INPUT).channels = channels;
    for (int i = 0; i < channels; ++i) {
        float pitchInput = GENERATE(take(4, random(0.f, 10.f)));
        spider->getInput(Spider::VOCT_INPUT).setVoltage(pitchInput, i);

        expectedFreqs[i] = dsp::FREQ_C4 * dsp::exp2_taylor5(pitchInput);
    }

    doProcess(256, [&] {
        for (int i = 0; i < channels; ++i) {
            float freq = spider->freqs[op * channels + i];
            REQUIRE_THAT(freq, WithinAbs(expectedFreqs[i], 0.000001));
        }
    });
}

TEST_CASE_METHOD(SpiderFixture, "Pitch shift varies operator frequency correctly", "[Integration]") {
    int op = GENERATE(0, 1, 2, 3, 4, 5);

    spider->getParam(Spider::LEVEL_PARAMS + op).setValue(1.f);
    spider->carriers[op] = true;

    SECTION("Static input CV") {
        float pitchShiftParam = GENERATE(take(4, random(-12.f, 12.f)));
        float pitchShiftCv = GENERATE(take(4, random(-1.f, 1.f)));
        float pitchShiftInput = GENERATE(take(4, random(0.f, 10.f)));

        spider->getParam(Spider::PITCH_PARAMS + op).setValue(pitchShiftParam);
        spider->getParam(Spider::PITCH_CV_PARAMS + op).setValue(pitchShiftCv);
        spider->getInput(Spider::PITCH_INPUTS + op).setVoltage(pitchShiftInput);

        float octaveShift = pitchShiftParam / 12.f;
        float normalisedPitchShiftInput = pitchShiftInput / 10.f;

        float expectedFreq = dsp::FREQ_C4 * dsp::exp2_taylor5(octaveShift);
        expectedFreq *= dsp::exp2_taylor5(pitchShiftCv * normalisedPitchShiftInput);

        doProcess(256, [&] {
            float freq = spider->freqs[op];
            REQUIRE_THAT(freq, WithinAbs(expectedFreq, 0.000001));
        });
    }

    SECTION("Time-varying input CV") {
        float pitchShiftParam = 0.f;
        float pitchShiftCv = 1.f;
        float pitchShiftInput = 0.f;

        spider->getParam(Spider::PITCH_PARAMS + op).setValue(pitchShiftParam);
        spider->getParam(Spider::PITCH_CV_PARAMS + op).setValue(pitchShiftCv);

        float octaveShift = pitchShiftParam / 12.f;

        auto preProcess = [&] {
            pitchShiftInput = random(0.f, 10.f).get();
            spider->getInput(Spider::PITCH_INPUTS + op).setVoltage(pitchShiftInput);

            float normalisedPitchShiftInput = pitchShiftInput / 10.f;
        };

        auto postProcess = [&] {
            float expectedFreq = dsp::FREQ_C4 * dsp::exp2_taylor5(octaveShift);

            float normalisedPitchShiftInput = pitchShiftInput / 10.f;
            expectedFreq *= dsp::exp2_taylor5(pitchShiftCv * normalisedPitchShiftInput);

            float freq = spider->freqs[op];
            REQUIRE_THAT(freq, WithinAbs(expectedFreq, 0.000001));
        };

        doProcess(256, postProcess, preProcess);
    }
}

TEST_CASE_METHOD(SpiderFixture, "Level varies operator level correctly", "[Integration]") {
    int op = GENERATE(0, 1, 2, 3, 4, 5);

    spider->carriers[op] = true;

    // square wave will give us max and min output
    spider->getParam(Spider::WAVE_PARAMS + op).setValue(0.75);

    SECTION("Static input CV") {
        float levelParam = GENERATE(take(4, random(0.f, 1.f)));
        float levelCv = GENERATE(take(4, random(-1.f, 1.f)));
        float levelInput = GENERATE(take(4, random(0.f, 10.f)));

        spider->getParam(Spider::LEVEL_PARAMS + op).setValue(levelParam);
        spider->getParam(Spider::LEVEL_CV_PARAMS + op).setValue(levelCv);
        spider->getInput(Spider::LEVEL_INPUTS + op).setVoltage(levelInput);

        float normalisedLevelInput = levelInput / 10.f;
        float expectedLevel = levelParam + (normalisedLevelInput * levelCv);
        expectedLevel = clamp(expectedLevel, 0.f, 1.f);

        doProcess(512, [&] {
            float signal = spider->outs[op];
            REQUIRE_THAT(std::abs(signal), WithinAbs(expectedLevel, 0.000001));
        });
    }

    SECTION("Time-varying input CV") {
        float levelParam = GENERATE(take(4, random(0.f, 1.f)));
        float levelCv = GENERATE(take(4, random(-1.f, 1.f)));

        spider->getParam(Spider::LEVEL_PARAMS + op).setValue(levelParam);
        spider->getParam(Spider::LEVEL_CV_PARAMS + op).setValue(levelCv);

        float expectedLevel = 0.f;
        auto preProcess = [&] {
            float levelInput = random(0.f, 10.f).get();
            spider->getInput(Spider::LEVEL_INPUTS + op).setVoltage(levelInput);

            float normalisedLevelInput = levelInput / 10.f;
            expectedLevel = levelParam + (normalisedLevelInput * levelCv);
            expectedLevel = clamp(expectedLevel, 0.f, 1.f);
        };

        auto postProcess = [&] {
            float signal = spider->outs[op];
            REQUIRE_THAT(std::abs(signal), WithinAbs(expectedLevel, 0.000001));
        };

        doProcess(512, postProcess, preProcess);
    }
}

TEST_CASE_METHOD(SpiderFixture, "Waveform blend varies operator waveform correctly", "[Integration]") {
    int op = GENERATE(0, 1, 2, 3, 4, 5);

    spider->getParam(Spider::LEVEL_PARAMS + op).setValue(1.f);
    spider->carriers[op] = true;

    SpiderSignalGenerator gen;

    SECTION("Static input CV") {
        float waveParam = GENERATE(take(4, random(0.f, 1.f)));
        float waveCv = GENERATE(take(4, random(-1.f, 1.f)));
        float waveInput = GENERATE(take(4, random(0.f, 10.f)));

        spider->getParam(Spider::WAVE_PARAMS + op).setValue(waveParam);
        spider->getParam(Spider::WAVE_CV_PARAMS + op).setValue(waveCv);
        spider->getInput(Spider::WAVE_INPUTS + op).setVoltage(waveInput);

        float normalisedWaveInput = waveInput / 10.f;
        float expectedWave = waveParam + (normalisedWaveInput * waveCv);
        expectedWave = clamp(expectedWave, 0.f, 1.f);

        doProcess(512, [&] {
            float sample = gen.generate(SAMPLE_TIME, dsp::FREQ_C4, expectedWave);
            REQUIRE_THAT(spider->outs[op], WithinAbs(sample, 0.000001));
        });
    }

    SECTION("Time-varying input CV") {
        float waveParam = 0.f;
        float waveCv = 1.f;
        float waveInput = 0.f;

        spider->getParam(Spider::WAVE_PARAMS + op).setValue(waveParam);
        spider->getParam(Spider::WAVE_CV_PARAMS + op).setValue(waveCv);

        float expectedWave = 0.f;

        auto preProcess = [&] {
            waveInput = random(0.f, 10.f).get();
            spider->getInput(Spider::WAVE_INPUTS + op).setVoltage(waveInput);

            float normalisedWaveInput = waveInput / 10.f;
            expectedWave = waveParam + (normalisedWaveInput * waveCv);
            expectedWave = clamp(expectedWave, 0.f, 1.f);
        };

        auto postProcess = [&] {
            float sample = gen.generate(SAMPLE_TIME, dsp::FREQ_C4, expectedWave);
            REQUIRE_THAT(spider->outs[op], WithinAbs(sample, 0.000001));
        };

        doProcess(512, postProcess, preProcess);
    }
}

TEST_CASE_METHOD(SpiderFixture, "Random combination of parameters varies operator signal correctly", "[Integration]") {
    int op = GENERATE(0, 1, 2, 3, 4, 5);

    spider->carriers[op] = true;

    SpiderSignalGenerator gen;

    float levelParam = GENERATE(take(2, random(0.f, 1.f)));
    float levelCv = GENERATE(take(2, random(-1.f, 1.f)));
    float levelInput = GENERATE(take(2, random(0.f, 10.f)));

    float pitchShiftParam = GENERATE(take(2, random(-12.f, 12.f)));
    float pitchShiftCv = GENERATE(take(2, random(-1.f, 1.f)));
    float pitchShiftInput = GENERATE(take(2, random(0.f, 10.f)));

    float waveParam = GENERATE(take(2, random(0.f, 1.f)));
    float waveCv = GENERATE(take(2, random(-1.f, 1.f)));
    float waveInput = GENERATE(take(2, random(0.f, 10.f)));

    spider->getParam(Spider::LEVEL_PARAMS + op).setValue(levelParam);
    spider->getParam(Spider::LEVEL_CV_PARAMS + op).setValue(levelCv);
    spider->getInput(Spider::LEVEL_INPUTS + op).setVoltage(levelInput);

    spider->getParam(Spider::PITCH_PARAMS + op).setValue(pitchShiftParam);
    spider->getParam(Spider::PITCH_CV_PARAMS + op).setValue(pitchShiftCv);
    spider->getInput(Spider::PITCH_INPUTS + op).setVoltage(pitchShiftInput);

    spider->getParam(Spider::WAVE_PARAMS + op).setValue(waveParam);
    spider->getParam(Spider::WAVE_CV_PARAMS + op).setValue(waveCv);
    spider->getInput(Spider::WAVE_INPUTS + op).setVoltage(waveInput);

    float normalisedLevelInput = levelInput / 10.f;
    float expectedLevel = levelParam + (normalisedLevelInput * levelCv);
    expectedLevel = clamp(expectedLevel, 0.f, 1.f);

    float octaveShift = pitchShiftParam / 12.f;
    float expectedFreq = dsp::FREQ_C4 * dsp::exp2_taylor5(octaveShift);
    expectedFreq *= dsp::exp2_taylor5(pitchShiftCv * (pitchShiftInput / 10.f));

    float normalisedWaveInput = waveInput / 10.f;
    float expectedWave = waveParam + (normalisedWaveInput * waveCv);
    expectedWave = clamp(expectedWave, 0.f, 1.f);

    doProcess(512, [&] {
        float freq = spider->freqs[op];
        REQUIRE_THAT(freq, WithinAbs(expectedFreq, 0.000001));

        float signal = spider->outs[op];

        float sample = expectedLevel * gen.generate(SAMPLE_TIME, expectedFreq, expectedWave);
        REQUIRE_THAT(signal, WithinAbs(sample, 0.000001));
    });
}

// TODO: Polyphony tests

TEST_CASE_METHOD(SpiderFixture, "Frequency modulation of operator signals yields correct frequencies",
                 "[Integration]") {
    int op1 = GENERATE(0, 1, 2, 3, 4, 5);
    int op2 = GENERATE(0, 1, 2, 3, 4, 5);

    if (op1 == op2)
        return;

    spider->carriers[op1] = true;

    float op1level = 1.f;
    float op2level = 0.5f;

    spider->getParam(Spider::LEVEL_PARAMS + op1).setValue(op1level);
    spider->getParam(Spider::LEVEL_PARAMS + op2).setValue(op2level);

    spider->algorithmGraph.addEdge(op2, op1);
    spider->topologicalOrder = spider->algorithmGraph.topologicalSort();

    SpiderSignalGenerator gen;

    doProcess(512, [&] {
        float op1Freq = spider->freqs[op1];

        float expectedOp2Signal = op2level * gen.generate(SAMPLE_TIME, dsp::FREQ_C4, 0.f);

        float expectedFreq = dsp::FREQ_C4 + 5.f * dsp::FREQ_C4 * expectedOp2Signal;
        REQUIRE_THAT(op1Freq, WithinAbs(expectedFreq, 0.0001));
    });
}

TEST_CASE_METHOD(SpiderFixture, "Frequency modulation with zero depth does not affect carrier frequency",
                 "[Integration]") {
    int op1 = GENERATE(0, 1, 2, 3, 4, 5);
    int op2 = GENERATE(0, 1, 2, 3, 4, 5);

    if (op1 == op2)
        return;

    spider->carriers[op1] = true;

    float op1level = 1.f;
    float op2level = 0.f;

    spider->getParam(Spider::LEVEL_PARAMS + op1).setValue(op1level);
    spider->getParam(Spider::LEVEL_PARAMS + op2).setValue(op2level);

    spider->algorithmGraph.addEdge(op2, op1);
    spider->topologicalOrder = spider->algorithmGraph.topologicalSort();

    doProcess(512, [&] {
        float op1Freq = spider->freqs[op1];
        float expectedFreq = dsp::FREQ_C4;
        REQUIRE_THAT(op1Freq, WithinAbs(expectedFreq, 0.0001));
    });
}

TEST_CASE_METHOD(SpiderFixture, "Frequency modulation in polyphonic mode yields correct frequencies", "[Integration]") {
    int op1 = GENERATE(0, 1, 2, 3, 4, 5);
    int op2 = GENERATE(0, 1, 2, 3, 4, 5);

    int channels = random(2, 16).get();

    if (op1 == op2)
        return;

    spider->carriers[op1] = true;

    float op1level = 1.f;
    float op2level = 1.f;

    spider->getInput(Spider::VOCT_INPUT).channels = channels;

    std::vector<float> expectedFreqs(channels);

    for (int i = 0; i < channels; ++i) {
        float pitchInput = random(0.f, 10.f).get();
        spider->getInput(Spider::VOCT_INPUT).setVoltage(pitchInput, i);

        expectedFreqs[i] = dsp::FREQ_C4 * dsp::exp2_taylor5(pitchInput);
    }

    spider->getParam(Spider::LEVEL_PARAMS + op1).setValue(op1level);
    spider->getParam(Spider::LEVEL_PARAMS + op2).setValue(op2level);

    spider->algorithmGraph.addEdge(op2, op1);
    spider->topologicalOrder = spider->algorithmGraph.topologicalSort();

    std::vector<SpiderSignalGenerator> gens(channels);

    doProcess(512, [&] {
        for (int i = 0; i < channels; ++i) {
            float op1Freq = spider->freqs[op1 * channels + i];

            float expectedOp2Signal = op2level * gens[i].generate(SAMPLE_TIME, expectedFreqs[i], 0.f);

            float expectedFreq = expectedFreqs[i] + 5.f * expectedFreqs[i] * expectedOp2Signal;
            REQUIRE_THAT(op1Freq, WithinAbs(expectedFreq, 0.0001));
        }
    });
}

TEST_CASE_METHOD(SpiderFixture, "Frequency modulation with non-sine signals yields correct carrier frequencies",
                 "[Integration]") {
    int op1 = GENERATE(0, 1, 2, 3, 4, 5);
    int op2 = GENERATE(0, 1, 2, 3, 4, 5);

    if (op1 == op2)
        return;

    spider->carriers[op1] = true;

    float op1level = 1.f;
    float op2level = 0.f;

    float op1Wave = GENERATE(take(4, random(0.f, 1.f)));
    float op2Wave = GENERATE(take(4, random(0.f, 1.f)));

    spider->getParam(Spider::LEVEL_PARAMS + op1).setValue(op1level);
    spider->getParam(Spider::LEVEL_PARAMS + op2).setValue(op2level);

    spider->getParam(Spider::WAVE_PARAMS + op2).setValue(op1Wave);
    spider->getParam(Spider::WAVE_PARAMS + op2).setValue(op2Wave);

    spider->algorithmGraph.addEdge(op2, op1);
    spider->topologicalOrder = spider->algorithmGraph.topologicalSort();

    SpiderSignalGenerator gen;

    doProcess(512, [&] {
        float op1Freq = spider->freqs[op1];

        float expectedOp2Signal = op2level * gen.generate(SAMPLE_TIME, dsp::FREQ_C4, op2Wave);

        float expectedFreq = dsp::FREQ_C4 + 5.f * dsp::FREQ_C4 * expectedOp2Signal;
        REQUIRE_THAT(op1Freq, WithinAbs(expectedFreq, 0.0001));
    });
}

TEST_CASE_METHOD(SpiderFixture, "Frequency modulation with different sample rates yields correct carrier frequencies",
                 "[Integration]") {
    int op1 = GENERATE(0, 1, 2, 3, 4, 5);
    int op2 = GENERATE(0, 1, 2, 3, 4, 5);
    int rate = GENERATE(44100, 48000, 768000);

    sampleRate = rate;

    if (op1 == op2)
        return;

    spider->carriers[op1] = true;

    float op1level = 1.f;
    float op2level = 0.f;

    float op1Wave = GENERATE(take(4, random(0.f, 1.f)));
    float op2Wave = GENERATE(take(4, random(0.f, 1.f)));

    spider->getParam(Spider::LEVEL_PARAMS + op1).setValue(op1level);
    spider->getParam(Spider::LEVEL_PARAMS + op2).setValue(op2level);

    spider->algorithmGraph.addEdge(op2, op1);
    spider->topologicalOrder = spider->algorithmGraph.topologicalSort();

    SpiderSignalGenerator gen;

    doProcess(512, [&] {
        float op1Freq = spider->freqs[op1];

        float expectedOp2Signal = op2level * gen.generate(1.f / sampleRate, dsp::FREQ_C4, 0.f);

        float expectedFreq = dsp::FREQ_C4 + 5.f * dsp::FREQ_C4 * expectedOp2Signal;
        REQUIRE_THAT(op1Freq, WithinAbs(expectedFreq, 0.0001));
    });
}

TEST_CASE_METHOD(SpiderFixture, "Frequency modulation of modulator signals yields correct carrier frequencies",
                 "[Integration]") {
    int op1 = GENERATE(0, 1, 2, 3, 4, 5);
    int op2 = GENERATE(0, 1, 2, 3, 4, 5);
    int op3 = GENERATE(0, 1, 2, 3, 4, 5);

    if (op1 == op2 || op1 == op3 || op2 == op3)
        return;

    spider->carriers[op1] = true;

    float op1level = 1.f;
    float op2level = 0.5f;
    float op3level = 0.25f;

    spider->getParam(Spider::LEVEL_PARAMS + op1).setValue(op1level);
    spider->getParam(Spider::LEVEL_PARAMS + op2).setValue(op2level);
    spider->getParam(Spider::LEVEL_PARAMS + op3).setValue(op3level);

    spider->algorithmGraph.addEdge(op2, op1);
    spider->algorithmGraph.addEdge(op3, op2);
    spider->topologicalOrder = spider->algorithmGraph.topologicalSort();

    SpiderSignalGenerator genOp2;
    SpiderSignalGenerator genOp3;

    doProcess(512, [&] {
        float op1Freq = spider->freqs[op1];
        float op2Freq = spider->freqs[op2];

        float expectedOp3Signal = op3level * genOp3.generate(SAMPLE_TIME, dsp::FREQ_C4, 0.f);
        float expectedOp2Freq = dsp::FREQ_C4 + 5.f * dsp::FREQ_C4 * expectedOp3Signal;

        REQUIRE_THAT(op2Freq, WithinRel(expectedOp2Freq, 0.001));

        float expectedOp2Signal = op2level * genOp2.generate(SAMPLE_TIME, expectedOp2Freq, 0.f);

        float expectedOp1Freq = dsp::FREQ_C4 + 5.f * expectedOp2Freq * expectedOp2Signal;
        REQUIRE_THAT(op1Freq, WithinRel(expectedOp1Freq, 0.001));
    });
}

TEST_CASE_METHOD(SpiderFixture, "Frequency modulation from multiple modulators on a carrier yields correct frequencies",
                 "[Integration]") {
    int op1 = GENERATE(0, 1, 2, 3, 4, 5);
    int op2 = GENERATE(0, 1, 2, 3, 4, 5);
    int op3 = GENERATE(0, 1, 2, 3, 4, 5);

    if (op1 == op2 || op1 == op3 || op2 == op3)
        return;

    spider->carriers[op1] = true;

    float op1level = 1.f;
    float op2level = 1.f;
    float op3level = 1.f;

    spider->getParam(Spider::LEVEL_PARAMS + op1).setValue(op1level);
    spider->getParam(Spider::LEVEL_PARAMS + op2).setValue(op2level);
    spider->getParam(Spider::LEVEL_PARAMS + op3).setValue(op3level);

    spider->algorithmGraph.addEdge(op2, op1);
    spider->algorithmGraph.addEdge(op3, op1);
    spider->topologicalOrder = spider->algorithmGraph.topologicalSort();

    SpiderSignalGenerator gen;

    doProcess(512, [&] {
        float op1Freq = spider->freqs[op1];

        float expectedModSignal = op2level * gen.generate(SAMPLE_TIME, dsp::FREQ_C4, 0.f);

        // 2x modulation depth since 2 operators are modulating
        float modulation = 5.f * dsp::FREQ_C4 * 2.f * expectedModSignal;

        float expectedOp1Freq = dsp::FREQ_C4 + modulation;
        REQUIRE_THAT(op1Freq, WithinRel(expectedOp1Freq, 0.001));
    });
}

TEST_CASE_METHOD(SpiderFixture, "Module state is correctly preserved", "[JSON]") {
    spider->model = modelPostHumanSpider;
    spider->algorithmGraph.addEdge(1, 0);
    spider->algorithmGraph.addEdge(2, 1);
    spider->topologicalOrder = spider->algorithmGraph.topologicalSort();
    spider->carriers[0] = true;

    json_t* state = spider->toJson();

    auto newSpider = std::make_unique<Spider>();
    newSpider->model = modelPostHumanSpider;
    newSpider->fromJson(state);

    REQUIRE(newSpider->algorithmGraph == spider->algorithmGraph);
    REQUIRE(newSpider->carriers == spider->carriers);
    REQUIRE(newSpider->topologicalOrder == spider->topologicalOrder);
}

} // namespace ph

#endif // PH_TEST_SPIDER_HPP