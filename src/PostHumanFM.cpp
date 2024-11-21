#include "plugin.hpp"
#include "DirectedGraph.hpp"
#include "Components.hpp"
#include "Wavetable.hpp"
#include "WavetableDisplay.hpp"

#include <array>

namespace { // anonymous

constexpr int OPERATOR_COUNT = 6;
constexpr int CONNECTION_COUNT = OPERATOR_COUNT * OPERATOR_COUNT;

} // anonymous namespace

namespace ph {

struct PostHumanFM : Module {

    enum ParamId {
        ENUMS(SELECT_PARAMS, OPERATOR_COUNT),
        ENUMS(WAVE_PARAMS, OPERATOR_COUNT),
        ENUMS(WAVE_CV_PARAMS, OPERATOR_COUNT),
        ENUMS(MULT_PARAMS, OPERATOR_COUNT),
        ENUMS(MULT_CV_PARAMS, OPERATOR_COUNT),
        ENUMS(LEVEL_PARAMS, OPERATOR_COUNT),
        ENUMS(LEVEL_CV_PARAMS, OPERATOR_COUNT),
        FREQ_PARAM,
        PARAMS_LEN
    };

    enum InputId {
        ENUMS(MULT_INPUTS, OPERATOR_COUNT),
        ENUMS(LEVEL_INPUTS, OPERATOR_COUNT),
        ENUMS(WAVE_INPUTS, OPERATOR_COUNT),
        VOCT_INPUT,
        INPUTS_LEN
    };

    enum OutputId { AUDIO_OUTPUT, OUTPUTS_LEN };
    enum LightId { ENUMS(SELECT_LIGHTS, OPERATOR_COUNT), ENUMS(CONNECTION_LIGHTS, CONNECTION_COUNT), LIGHTS_LEN };

    struct FMOperator {
        void generate(float freq, float wavePos, float level, float sampleTime, float multiplier, float modulation) {
            wavePos *= (wavetable.waveCount() - 1);

            lastWavePos = wavePos;

            float phaseIncrement = (multiplier * freq) * sampleTime;

            phase = phase + phaseIncrement + modulation;

            if (phase > 1.f) {
                phase -= 1.f;
            } else if (phase < 0.f) {
                phase += 1.f;
            }

            float index = phase * wavetable.wavelength;

            float indexF = index - std::trunc(index);
            size_t index0 = std::trunc(index);
            size_t index1 = (index0 + 1) % wavetable.wavelength;

            float pos = wavePos - std::trunc(wavePos);
            size_t wavePos0 = std::trunc(wavePos);
            size_t wavePos1 = wavePos + 1;

            float out0 = wavetable.sampleAt(wavePos0, index0);
            float out1 = wavetable.sampleAt(wavePos1, index1);

            // TODO: Only linearly interpolate if necessary otherwise it breaks
            // Linearly interpolate between the two waves
            out = level * crossfade(out0, out1, pos);
        }

        float phase = 0.f;
        float out = 0.f;
        float lastWavePos = 0.f;

        void reset() {
            phase = 0.f;
            out = 0.f;
        }

        Wavetable wavetable;
    };

    PostHumanFM() { configParameters(); }

    void configParameters() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configInput(VOCT_INPUT, "1V/Oct");
        configOutput(AUDIO_OUTPUT, "Audio");
        configParam(FREQ_PARAM, -80, 80, 0.f, "Frequency", "Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);

        for (int op = 0; op < OPERATOR_COUNT; ++op) {
            std::string opStr = std::to_string(op + 1);

            configButton(SELECT_PARAMS + op, "Select operator " + opStr);
            configParam(MULT_PARAMS + op, -3, 3, 0.f, "Operator " + opStr + " multiplier", "x", 2.f);
            configParam(LEVEL_PARAMS + op, 0.f, 1.f, 0.f, "Operator " + opStr + " level", "%", 0.f, 100.f);
            configParam(WAVE_PARAMS + op, 0.f, 1.f, 0.f, "Operator " + opStr + " wavetable position", "%", 0.f, 100.f,
                        0.f);

            configParam(MULT_CV_PARAMS + op, -1.f, 1.f, 0.f, "Operator " + opStr + " multiplier CV", "%", 0.f, 100.f,
                        0.f);
            configParam(LEVEL_CV_PARAMS + op, -1.f, 1.f, 0.f, "Operator " + opStr + " level CV", "%", 0.f, 100.f, 0.f);
            configParam(WAVE_CV_PARAMS + op, -1.f, 1.f, 0.f, "Operator " + opStr + " wavetable position CV", "%", 0.f,
                        100.f, 0.f);

            configInput(MULT_INPUTS + op, "Operator " + opStr + " multiplier");
            configInput(LEVEL_INPUTS + op, "Operator " + opStr + " level");
            configInput(WAVE_INPUTS + op, "Operator " + opStr + " wavetable position");

            getParamQuantity(MULT_PARAMS + op)->snapEnabled = true;
            // getParamQuantity(MULT_PARAMS + op)->snapEnabled = true;
        }

        for (int conn = 0; conn < CONNECTION_COUNT; ++conn) {
            configLight(CONNECTION_LIGHTS + conn);
        }
    }

    void processEdit(float sampleTime) {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            getLight(SELECT_LIGHTS + i).setBrightnessSmooth(selectedOperator == i, sampleTime);

            auto& trigger = operatorTriggers[i];
            int selectTriggered = trigger.process(getParam(SELECT_PARAMS + i).getValue());

            if (selectTriggered) {
                if (selectedOperator == i) {
                    selectedOperator = -1;
                } else if (selectedOperator > -1) {
                    auto result = algorithmGraph.toggleEdge(selectedOperator, i);

                    if (result == ToggleEdgeResult::CYCLE) {
                        printf("Cycle detected\n"); // todo: flash the lights red for a bit
                        selectedOperator = -1;
                        continue;
                    }

                    int lightIndex = (result == ToggleEdgeResult::REMOVED_REV)
                                         ? (CONNECTION_LIGHTS + OPERATOR_COUNT * i + selectedOperator)
                                         : (CONNECTION_LIGHTS + OPERATOR_COUNT * selectedOperator + i);

                    getLight(lightIndex).setBrightness((result == ToggleEdgeResult::ADDED) ? 1.0f : 0.0f);

                    topologicalOrder = algorithmGraph.topologicalSort();

                    for (auto& op : operators) {
                        op.reset(); // Reset phase of all operators
                    }

                    selectedOperator = -1;

                } else {
                    selectedOperator = i;
                }

                printf("Selected operator %d\n", i);
            }
        }
    }

    void process(const ProcessArgs& args) override {
        float freqParam = getParam(FREQ_PARAM).getValue() / 12.f;
        float pitch = freqParam + getInput(VOCT_INPUT).getVoltage();
        float freq = dsp::FREQ_C4 * std::pow(2.f, pitch);

        processEdit(args.sampleTime);

        //// todo: this can eventually be parallelised with SIMD once independent carriers can be found from the
        /// graph

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            int op = topologicalOrder[i];

            auto& multParam = getParam(MULT_PARAMS + op);

            float mult = dsp::exp2_taylor5(multParam.getValue());
            float level = getParam(LEVEL_PARAMS + op).getValue();
            float wavePos = getParam(WAVE_PARAMS + op).getValue();

            float multCv = getParam(MULT_CV_PARAMS + op).getValue();
            float levelCv = getParam(LEVEL_CV_PARAMS + op).getValue();
            float waveCv = getParam(WAVE_CV_PARAMS + op).getValue();

            if (getInput(MULT_INPUTS + op).isConnected()) {
                mult += multCv * getInput(MULT_INPUTS + op).getVoltage() / 10.f;
            }

            if (getInput(LEVEL_INPUTS + op).isConnected()) {
                level += levelCv * getInput(LEVEL_INPUTS + op).getVoltage() / 10.F;
            }

            if (getInput(WAVE_INPUTS + op).isConnected()) {
                wavePos += waveCv * getInput(WAVE_INPUTS + op).getVoltage() / 10.f;
            }

            mult = clamp(mult, 0.125, 8.0f);
            level = clamp(level, 0.f, 1.f);
            wavePos = clamp(wavePos, 0.f, 1.f);

            const auto& modulators = algorithmGraph.getAdjacentVerticesRev(op);

            float modulation = 0.f;
            for (const auto& modulator : modulators) {
                modulation += operators[modulator].out;
            }

            operators[op].generate(freq, wavePos, level, mult, args.sampleTime, modulation);
        }

        float output = 0.f;
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            if (algorithmGraph.getAdjacentVertices(i).empty()) {
                output += operators[i].out;
            }
        }

        if (output > 1.f) {
            output = 1.f;
        } else if (output < -1.f) {
            output = -1.f;
        }

        getOutput(AUDIO_OUTPUT).setVoltage(5 * output);
    }

    std::array<FMOperator, OPERATOR_COUNT> operators; // todo: DAG
    std::array<dsp::BooleanTrigger, OPERATOR_COUNT> operatorTriggers;
    std::vector<int> topologicalOrder = {0, 1, 2, 3, 4, 5};
    DirectedGraph<int> algorithmGraph{OPERATOR_COUNT};

    int selectedOperator = -1;
};

// TODO: Organise
struct PostHumanFMWidget : ModuleWidget {

    // Multiple repeated params for each parameter and operator
    struct ParameterPositions {
        Vec paramPos;
        Vec inputPos;
        Vec cvParamPos;
    };

    const ParameterPositions multPositions[OPERATOR_COUNT] = {
        {mm2px(Vec(29.421, 13.167)), mm2px(Vec(15.007, 21.196)), mm2px(Vec(29.421, 29.560))},
        {mm2px(Vec(143.299, 13.195)), mm2px(Vec(157.713, 21.224)), mm2px(Vec(143.299, 29.588))},
        {mm2px(Vec(165.097, 44.598)), mm2px(Vec(150.682, 39.744)), mm2px(Vec(165.097, 31.380))},
        {mm2px(Vec(143.302, 107.098)), mm2px(Vec(157.717, 99.069)), mm2px(Vec(143.302, 90.705))},
        {mm2px(Vec(29.421, 107.098)), mm2px(Vec(15.007, 99.069)), mm2px(Vec(29.421, 90.705))},
        {mm2px(Vec(7.596, 44.598)), mm2px(Vec(22.011, 39.744)), mm2px(Vec(7.596, 31.380))}};

    const ParameterPositions levelPositions[OPERATOR_COUNT] = {
        {mm2px(Vec(68.737, 14.322)), mm2px(Vec(78.825, 30.447)), mm2px(Vec(66.395, 37.623))},
        {mm2px(Vec(103.983, 14.350)), mm2px(Vec(93.895, 30.475)), mm2px(Vec(106.325, 37.652))},
        {mm2px(Vec(165.097, 74.120)), mm2px(Vec(150.682, 78.974)), mm2px(Vec(165.097, 87.337))},
        {mm2px(Vec(106.632, 100.123)), mm2px(Vec(94.957, 89.289)), mm2px(Vec(107.386, 82.113))},
        {mm2px(Vec(66.091, 100.123)), mm2px(Vec(77.767, 89.289)), mm2px(Vec(65.337, 82.113))},
        {mm2px(Vec(7.596, 74.120)), mm2px(Vec(22.011, 78.974)), mm2px(Vec(7.596, 87.337))}};

    const ParameterPositions wavePositions[OPERATOR_COUNT] = {
        {mm2px(Vec(42.211, 36.844)), mm2px(Vec(55.000, 27.233)), mm2px(Vec(55.000, 44.128))},
        {mm2px(Vec(130.509, 36.844)), mm2px(Vec(117.720, 27.261)), mm2px(Vec(117.720, 44.156))},
        {mm2px(Vec(135.745, 51.447)), mm2px(Vec(125.657, 64.397)), mm2px(Vec(138.086, 71.573))},
        {mm2px(Vec(130.513, 83.421)), mm2px(Vec(117.724, 93.032)), mm2px(Vec(117.724, 76.137))},
        {mm2px(Vec(42.211, 83.421)), mm2px(Vec(55.000, 93.032)), mm2px(Vec(55.000, 76.137))},
        {mm2px(Vec(36.948, 51.447)), mm2px(Vec(47.036, 64.397)), mm2px(Vec(34.606, 71.573))}};

    const Vec displayPositions[OPERATOR_COUNT] = {mm2px(Vec(38.278, 8.297)),   mm2px(Vec(112.840, 8.297)),
                                                  mm2px(Vec(146.850, 53.862)), mm2px(Vec(113.830, 100.283)),
                                                  mm2px(Vec(37.291, 100.283)), mm2px(Vec(4.241, 53.862))};

    const Vec opSelectorPositions[OPERATOR_COUNT] = {mm2px(Vec(73.961, 47.245)),  mm2px(Vec(98.758, 47.245)),
                                                     mm2px(Vec(111.157, 60.146)), mm2px(Vec(98.758, 73.047)),
                                                     mm2px(Vec(73.962, 73.047)),  mm2px(Vec(61.563, 60.146))};
    PostHumanFMWidget(PostHumanFM* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/PostHumanFM.svg")));

        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        if (!module)
            return;

        drawOpControls();

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
        }

        addChild(createInputCentered<PJ301MPort>(mm2px(Vec(65.193f, 117.008f)), module, PostHumanFM::VOCT_INPUT));
        addChild(createOutputCentered<PJ301MPort>(mm2px(Vec(107.527f, 117.008f)), module, PostHumanFM::AUDIO_OUTPUT));

        addChild(createParamCentered<ShinyBigKnob>(mm2px(Vec(86.360f, 113.318f)), module, PostHumanFM::FREQ_PARAM));
    }

    void drawOpControls() {
        auto* fmModule = dynamic_cast<PostHumanFM*>(module);

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            // TODO: Clean this up can use only one set of addparam addinput etc. with clever
            auto* display = createWidget<WavetableDisplay>(displayPositions[i]);
            display->wavetable = &fmModule->operators[i].wavetable;
            display->wavePos = &fmModule->operators[i].lastWavePos;
            display->op = i;

            addChild(display);

            addParam(createParamCentered<ShinyKnob>(multPositions[i].paramPos, module, PostHumanFM::MULT_PARAMS + i));
            addParam(createParamCentered<AttenuatorKnob>(multPositions[i].cvParamPos, module,
                                                         PostHumanFM::MULT_CV_PARAMS + i));
            addInput(
                createInputCentered<DarkPJ301MPort>(multPositions[i].inputPos, module, PostHumanFM::MULT_INPUTS + i));

            addParam(createParamCentered<ShinyKnob>(levelPositions[i].paramPos, module, PostHumanFM::LEVEL_PARAMS + i));
            addParam(createParamCentered<AttenuatorKnob>(levelPositions[i].cvParamPos, module,
                                                         PostHumanFM::LEVEL_CV_PARAMS + i));
            addInput(
                createInputCentered<DarkPJ301MPort>(levelPositions[i].inputPos, module, PostHumanFM::LEVEL_INPUTS + i));

            addParam(createParamCentered<ShinyKnob>(wavePositions[i].paramPos, module, PostHumanFM::WAVE_PARAMS + i));
            addParam(createParamCentered<AttenuatorKnob>(wavePositions[i].cvParamPos, module,
                                                         PostHumanFM::WAVE_CV_PARAMS + i));
            addInput(
                createInputCentered<DarkPJ301MPort>(wavePositions[i].inputPos, module, PostHumanFM::WAVE_INPUTS + i));

            for (int j = 0; j < OPERATOR_COUNT; ++j) {
                if (i == j)
                    continue;
                addChild(createConnectionLight(opSelectorPositions[i], opSelectorPositions[j], module,
                                               PostHumanFM::CONNECTION_LIGHTS + OPERATOR_COUNT * i + j, i, j));
            }

            addParam(createLightParamCentered<VCVLightBezel<RedLight>>(
                opSelectorPositions[i], module, PostHumanFM::SELECT_PARAMS + i, PostHumanFM::SELECT_LIGHTS + i));
        }
    }

    void drawOperators() {
        float x0 = 30;
        float y0 = 220;

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
        }
    }

    constexpr static float opRadius = 48.f;

    constexpr static float pi = M_PI;
};

} // namespace ph

Model* modelPostHumanFM = createModel<ph::PostHumanFM, ph::PostHumanFMWidget>("PostHumanFM");