#include "plugin.hpp"
#include "DirectedGraph.hpp"
#include "Components.hpp"
// #include "Wavetable.hpp"
// #include "WavetableDisplay.hpp"

#include <array>

namespace { // anonymous

constexpr int OPERATOR_COUNT = 6;
constexpr int CONNECTION_COUNT = OPERATOR_COUNT * OPERATOR_COUNT;

} // anonymous namespace

namespace ph {

struct Spider : Module {

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
    enum LightId {
        ENUMS(SELECT_LIGHTS, OPERATOR_COUNT),
        ENUMS(CARRIER_LIGHTS, OPERATOR_COUNT),
        ENUMS(CONNECTION_LIGHTS, CONNECTION_COUNT),
        LIGHTS_LEN
    };

    Spider() {
        configParameters();
        setConnectionLights();

        for (auto& p : phase) {
            p = rack::random::uniform();
        }
    }

    void configParameters() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configInput(VOCT_INPUT, "1V/Oct");
        configOutput(AUDIO_OUTPUT, "Audio");
        configParam(FREQ_PARAM, -80, 80, 0.f, "Frequency", "Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);

        for (int op = 0; op < OPERATOR_COUNT; ++op) {
            std::string opStr = std::to_string(op + 1);

            configButton(SELECT_PARAMS + op, "Select operator " + opStr);
            configParam(MULT_PARAMS + op, -12, 12, 0.f, "Operator " + opStr + " coarse pitch", " semitones");
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
    }

    void processEdit(float sampleTime) {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            getLight(SELECT_LIGHTS + i).setBrightnessSmooth(selectedOperator == i, sampleTime);

            auto& trigger = operatorTriggers[i];
            int selectTriggered = trigger.process(getParam(SELECT_PARAMS + i).getValue());

            if (selectTriggered) {
                if (selectedOperator == i) {
                    carriers[i] = !carriers[i];
                    getLight(CARRIER_LIGHTS + i).setBrightness(carriers[i] ? 1.0f : 0.0f);
                    updateTooltips(i, false);
                    selectedOperator = -1;

                } else if (selectedOperator > -1) {
                    auto result = algorithmGraph.toggleEdge(selectedOperator, i);

                    if (result == ToggleEdgeResult::CYCLE) {
                        selectedOperator = -1;
                        continue;
                    }

                    if (result == ToggleEdgeResult::ADDED) {
                        getLight(CONNECTION_LIGHTS + OPERATOR_COUNT * selectedOperator + i).setBrightness(1.0f);
                    } else if (result == ToggleEdgeResult::SWAPPED) {
                        getLight(CONNECTION_LIGHTS + OPERATOR_COUNT * i + selectedOperator).setBrightness(0.0f);
                        getLight(CONNECTION_LIGHTS + OPERATOR_COUNT * selectedOperator + i).setBrightness(1.0f);
                    } else if (result == ToggleEdgeResult::REMOVED) {
                        getLight(CONNECTION_LIGHTS + OPERATOR_COUNT * selectedOperator + i).setBrightness(0.0f);
                    }
                    topologicalOrder = algorithmGraph.topologicalSort();

                    // for (auto& op : operators) {
                    //     op.reset(); // Reset phase of all operators
                    // }
                    updateTooltips(selectedOperator, false);
                    selectedOperator = -1;

                } else {
                    selectedOperator = i;
                    updateTooltips(i, true);
                }
            }
        }
    }

    void updateTooltips(int op, bool select) {

        std::string operatorString = "operator " + std::to_string(op + 1);
        auto paramQuantity = getParamQuantity(SELECT_PARAMS + op);

        if (select) {
            paramQuantity->name =
                (carriers[op] ? "Disable " : "Enable ") + std::string("operator ") + std::to_string(op + 1) + " output";
        }

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            std::string operatorString = "operator " + std::to_string(i + 1);
            paramQuantity = getParamQuantity(SELECT_PARAMS + i);
            if (select) {
                if (i == op)
                    continue;

                paramQuantity->name =
                    algorithmGraph.hasEdge(op, i) ? "Demodulate " + operatorString : "Modulate " + operatorString;
            } else {
                paramQuantity->name = "Select " + operatorString;
            }
        }
    }

    std::array<float, OPERATOR_COUNT> getOperatorLevels() {
        float totalLevel = 0.f;
        std::array<float, OPERATOR_COUNT> levels = {};

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            float level = getParam(LEVEL_PARAMS + i).getValue();
            float levelCv = getParam(LEVEL_CV_PARAMS + i).getValue();

            if (getInput(LEVEL_INPUTS + i).isConnected()) {
                level += levelCv * getInput(LEVEL_INPUTS + i).getVoltage() / 10.f;
            }

            levels[i] = level;
            totalLevel += level;
        }

        return levels;
    }

    void process(const ProcessArgs& args) override {
        int channels = std::max(1, getInput(VOCT_INPUT).getChannels());

        std::vector<float> freqs(OPERATOR_COUNT * channels);

        // TODO: SIMD
        for (int op = 0; op < OPERATOR_COUNT; ++op) {
            for (int c = 0; c < channels; c++) {
                float freqParam = getParam(FREQ_PARAM).getValue() / 12.f;
                float pitch = freqParam + getInput(VOCT_INPUT).getPolyVoltage(c);
                freqs[op * channels + c] = dsp::FREQ_C4 * std::pow(2.f, pitch);
            }
        }

        processEdit(args.sampleTime);

        auto levels = getOperatorLevels();
        auto outs = processOperators(channels, args.sampleTime, freqs);

        getOutput(AUDIO_OUTPUT).setChannels(channels);

        for (int c = 0; c < channels; c++) {
            float output = 0.f;

            for (int op = 0; op < OPERATOR_COUNT; ++op) {
                if (carriers[op]) {
                    output += outs[op * channels + c];
                }
            }

            if (output > 1.f) {
                output = 1.f;
            } else if (output < -1.f) {
                output = -1.f;
            }

            getOutput(AUDIO_OUTPUT).setVoltage(5 * output, c);
        }
    }

    std::vector<float> processOperators(int channels, float sampleTime, std::vector<float>& freqs) {
        std::vector<float> outs(OPERATOR_COUNT * channels);
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            int op = topologicalOrder[i];
            processOperator(op, channels, sampleTime, outs, freqs);
        }

        return outs;
    }

    void processOperator(int op, int channels, float sampleTime, std::vector<float>& outs, std::vector<float>& freqs) {
        // TODO: only process an operator if it has a level

        float pitchShift = getParam(MULT_PARAMS + op).getValue();
        float pitchShiftCv = getParam(MULT_CV_PARAMS + op).getValue();

        if (getInput(MULT_INPUTS + op).isConnected()) {
            pitchShift += pitchShiftCv * getInput(MULT_INPUTS + op).getVoltage() / 10.f;
        }

        float pitchShiftExponent = pitchShift / 12.0f;

        float wavePos = getParam(WAVE_PARAMS + op).getValue();
        float wavePosCv = getParam(WAVE_CV_PARAMS + op).getValue();

        if (getInput(WAVE_INPUTS + op).isConnected()) {
            wavePos += wavePosCv * getInput(WAVE_INPUTS + op).getVoltage() / 10.f;
        }

        wavePos = clamp(wavePos, 0.f, 1.f);

        float level = getParam(LEVEL_PARAMS + op).getValue();
        float levelCv = getParam(LEVEL_CV_PARAMS + op).getValue();

        if (getInput(LEVEL_INPUTS + op).isConnected()) {
            level += levelCv * getInput(LEVEL_INPUTS + op).getVoltage() / 10.f;
        }

        const auto& modulators = algorithmGraph.getAdjacentVerticesRev(op);

        for (int c = 0; c < channels; c++) {
            float modulation = 0.f;
            float& freq = freqs[op * channels + c];

            freq *= dsp::exp2_taylor5(pitchShiftExponent);

            // float freq = freqs[c] * dsp::exp2_taylor5(pitchShiftExponent);
            // freq += 5 * modulation;

            for (int mod : modulators) {
                float modFreq = freqs[mod * channels + c];
                freq += 5 * modFreq * outs[mod * channels + c];
            }

            float& ph = phase[op * channels + c];

            ph += freq * sampleTime;

            if (ph > 1)
                ph -= 1;
            if (ph < -1)
                ph += 1;

            // phase[op * channels + c] += freq * sampleTime;
            // TODO: check this is ocrrect because ive changed it to the sine example
            // if (ph > 1)
            //    ph -= 1;
            // if (ph < -1)
            //    ph += 1;

            // outs[op * channels + c] = level * getInterpolatedWtSample(op, phase[op * channels + c], wavePos);

            float sine = std::sin(2 * M_PI * ph);
            // Triangle
            float triangle = 4.f * std::abs(std::round(ph) - ph) - 1.f;

            // Saw
            float saw = 2.f * (ph - std::round(ph));

            // Square
            float square = (ph < 0.5f) ? 1.f : -1.f;

            float mix = 0;

            if (wavePos <= 0.25f) {
                mix = crossfade(sine, triangle, wavePos / 0.25f);
            } else if (wavePos <= 0.5f) {
                mix = crossfade(triangle, saw, (wavePos - 0.25f) / 0.25f);
            } else if (wavePos <= 0.75f) {
                mix = crossfade(saw, square, (wavePos - 0.5f) / 0.25f);
            } else {
                mix = crossfade(square, sine, (wavePos - 0.75f) / 0.25f);
            }

            outs[op * channels + c] = level * mix;
        }
    }

    // float getInterpolatedWtSample(int op, float phase, float wavePos) {
    //     float wtIndex = phase * wavetables[op].wavelength;
    //     float wtIndexF = wtIndex - std::trunc(wtIndex);
    //
    //    size_t wtIndex0 = std::trunc(wtIndex);
    //    size_t wtIndex1 = (wtIndex0 + 1) % wavetables[op].wavelength;
    //
    //    float pos = wavePos - std::trunc(wavePos);
    //    size_t wavePos0 = std::trunc(wavePos);
    //    size_t wavePos1 = wavePos + 1;
    //
    //    float out0 = wavetables[op].sampleAt(wavePos0, wtIndex0);
    //    float out1 = wavetables[op].sampleAt(wavePos1, wtIndex1);
    //
    //    // TODO: Only linearly interpolate if necessary otherwise it breaks
    //    // Linearly interpolate between the two waves
    //    float out = crossfade(out0, out1, pos);
    //    return out;
    //}

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "algorithm", algorithmGraph.toJson());

        json_t* topologicalOrderJ = json_array();
        for (int i : topologicalOrder) {
            json_array_append_new(topologicalOrderJ, json_integer(i));
        }
        json_object_set_new(rootJ, "topologicalOrder", topologicalOrderJ);

        json_t* carriersJ = json_array();
        for (bool carrier : carriers) {
            json_array_append_new(carriersJ, json_boolean(carrier));
        }
        json_object_set_new(rootJ, "carriers", carriersJ);

        // json_t* wavetableArrayJ = json_array();
        // for (const auto& wt : wavetables) {
        //     json_array_append_new(wavetableArrayJ, wt.toJson());
        // }
        // json_object_set_new(rootJ, "wavetables", wavetableArrayJ);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* algorithmJ = json_object_get(rootJ, "algorithm");
        algorithmGraph.fromJson(algorithmJ);

        json_t* topologicalOrderJ = json_object_get(rootJ, "topologicalOrder");
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            topologicalOrder[i] = json_integer_value(json_array_get(topologicalOrderJ, i));
        }

        json_t* carriersJ = json_object_get(rootJ, "carriers");
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            carriers[i] = json_boolean_value(json_array_get(carriersJ, i));
        }

        setConnectionLights();
    }

    void setConnectionLights() {
        for (int i = 0; i < algorithmGraph.adjList.size(); ++i) {
            for (int j = 0; j < algorithmGraph.adjList[i].size(); ++j) {
                int lightId = CONNECTION_LIGHTS + i * OPERATOR_COUNT + algorithmGraph.adjList[i][j];
                getLight(lightId).setBrightness(1.0f);
            }
        }

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            getLight(CARRIER_LIGHTS + i).setBrightness(carriers[i] ? 1.0f : 0.0f);
        }
    }

    void clearConnectionLights() {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            getLight(CARRIER_LIGHTS + i).setBrightness(0.0f);

            for (int j = 0; j < OPERATOR_COUNT; ++j) {
                getLight(CONNECTION_LIGHTS + i * OPERATOR_COUNT + j).setBrightness(0.0f);
            }
        }
    }

    void onReset() override {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            carriers[i] = false;
            operatorTriggers[i].reset();
        }

        clearConnectionLights();
        algorithmGraph.clear();
        topologicalOrder = {0, 1, 2, 3, 4, 5};
        updateTooltips(selectedOperator, false);
        selectedOperator = -1;

        Module::onReset();
    }

    void onRandomize() {
        algorithmGraph.clear();
        clearConnectionLights();

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            carriers[i] = random::uniform() > 0.5f;

            for (int j = 0; j < OPERATOR_COUNT; ++j) {
                if (random::uniform() > 0.75f) {
                    algorithmGraph.addEdge(i, j);
                }
            }
        }

        topologicalOrder = algorithmGraph.topologicalSort();
        setConnectionLights();

        Module::onRandomize();
    }

    std::array<float, OPERATOR_COUNT * 16> phase;
    // std::array<Wavetable, OPERATOR_COUNT> wavetables;
    std::array<float, OPERATOR_COUNT> lastWavePos;
    std::array<bool, OPERATOR_COUNT> carriers = {};
    std::array<dsp::BooleanTrigger, OPERATOR_COUNT> operatorTriggers;
    std::vector<int> topologicalOrder = {0, 1, 2, 3, 4, 5};
    DirectedGraph<int> algorithmGraph{OPERATOR_COUNT};

    int selectedOperator = -1;
};

// TODO: Organise
struct SpiderWidget : ModuleWidget {

    // Multiple repeated params for each parameter and operator
    struct ParameterPositions {
        Vec paramPos;
        Vec inputPos;
        Vec cvParamPos;
    };

    const ParameterPositions multPositions[OPERATOR_COUNT] = {
        {mm2px(Vec(29.421, 12.109)), mm2px(Vec(15.007, 20.196)), mm2px(Vec(29.421, 28.560))},
        {mm2px(Vec(143.299, 12.195)), mm2px(Vec(157.713, 21.224)), mm2px(Vec(143.299, 29.588))},
        {mm2px(Vec(165.097, 44.598)), mm2px(Vec(150.682, 39.744)), mm2px(Vec(165.097, 31.380))},
        {mm2px(Vec(143.302, 107.098)), mm2px(Vec(157.717, 99.069)), mm2px(Vec(143.302, 90.705))},
        {mm2px(Vec(29.421, 107.098)), mm2px(Vec(15.007, 99.069)), mm2px(Vec(29.421, 90.705))},
        {mm2px(Vec(7.596, 44.598)), mm2px(Vec(22.011, 39.744)), mm2px(Vec(7.596, 31.380))}};

    const ParameterPositions levelPositions[OPERATOR_COUNT] = {
        {mm2px(Vec(67.149, 19.280)), mm2px(Vec(78.825, 29.447)), mm2px(Vec(66.395, 36.623))},
        {mm2px(Vec(103.983, 14.350)), mm2px(Vec(93.895, 30.475)), mm2px(Vec(106.325, 37.652))},
        {mm2px(Vec(165.097, 74.120)), mm2px(Vec(150.682, 78.974)), mm2px(Vec(165.097, 87.337))},
        {mm2px(Vec(106.632, 100.123)), mm2px(Vec(94.957, 89.289)), mm2px(Vec(107.386, 82.113))},
        {mm2px(Vec(66.091, 100.123)), mm2px(Vec(77.767, 89.289)), mm2px(Vec(65.337, 82.113))},
        {mm2px(Vec(7.596, 74.120)), mm2px(Vec(22.011, 78.974)), mm2px(Vec(7.596, 87.337))}};

    const ParameterPositions wavePositions[OPERATOR_COUNT] = {
        {mm2px(Vec(42.211, 35.844)), mm2px(Vec(55.000, 26.233)), mm2px(Vec(55.000, 43.128))},
        {mm2px(Vec(130.509, 36.844)), mm2px(Vec(117.720, 27.261)), mm2px(Vec(117.720, 44.156))},
        {mm2px(Vec(135.745, 51.447)), mm2px(Vec(125.657, 64.397)), mm2px(Vec(138.086, 71.573))},
        {mm2px(Vec(130.513, 83.421)), mm2px(Vec(117.724, 93.032)), mm2px(Vec(117.724, 76.137))},
        {mm2px(Vec(42.211, 83.421)), mm2px(Vec(55.000, 93.032)), mm2px(Vec(55.000, 76.137))},
        {mm2px(Vec(36.948, 51.447)), mm2px(Vec(47.036, 64.397)), mm2px(Vec(34.606, 71.573))}};

    const Vec displayPositions[OPERATOR_COUNT] = {mm2px(Vec(38.278, 7.297)),   mm2px(Vec(112.840, 8.297)),
                                                  mm2px(Vec(146.850, 53.862)), mm2px(Vec(113.830, 100.283)),
                                                  mm2px(Vec(37.291, 100.283)), mm2px(Vec(4.241, 53.862))};

    const Vec opSelectorPositions[OPERATOR_COUNT] = {mm2px(Vec(73.961, 47.245)),  mm2px(Vec(98.758, 47.245)),
                                                     mm2px(Vec(111.157, 60.146)), mm2px(Vec(98.758, 73.047)),
                                                     mm2px(Vec(73.962, 73.047)),  mm2px(Vec(61.563, 60.146))};
    SpiderWidget(Spider* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/PostHumanFM.svg")));

        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        drawOpControls();

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
        }

        addChild(createInputCentered<PJ301MPort>(mm2px(Vec(67.37f, 117.008f)), module, Spider::VOCT_INPUT));
        addChild(createOutputCentered<PJ301MPort>(mm2px(Vec(105.375f, 117.008f)), module, Spider::AUDIO_OUTPUT));

        addChild(createParamCentered<ShinyBigKnob>(mm2px(Vec(86.360f, 113.318f)), module, Spider::FREQ_PARAM));
    }

    void drawOpControls() {
        auto* fmModule = dynamic_cast<Spider*>(module);

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            // TODO: Clean this up can use only one set of addparam addinput etc. with clever
            // auto* display = createWidget<WavetableDisplay>(displayPositions[i]);

            if (module) {
                // display->wavetable = &fmModule->wavetables[i];
                //  todo: wavepositioin for control
                // display->wavePos = &fmModule->lastWavePos[i];
            }
            // display->op = i;

            // addChild(display);

            addParam(createParamCentered<ShinyKnob>(multPositions[i].paramPos, module, Spider::MULT_PARAMS + i));
            addParam(
                createParamCentered<AttenuatorKnob>(multPositions[i].cvParamPos, module, Spider::MULT_CV_PARAMS + i));
            addInput(createInputCentered<DarkPJ301MPort>(multPositions[i].inputPos, module, Spider::MULT_INPUTS + i));

            addParam(createParamCentered<ShinyKnob>(levelPositions[i].paramPos, module, Spider::LEVEL_PARAMS + i));
            addParam(
                createParamCentered<AttenuatorKnob>(levelPositions[i].cvParamPos, module, Spider::LEVEL_CV_PARAMS + i));
            addInput(createInputCentered<DarkPJ301MPort>(levelPositions[i].inputPos, module, Spider::LEVEL_INPUTS + i));

            addParam(createParamCentered<ShinyKnob>(wavePositions[i].paramPos, module, Spider::WAVE_PARAMS + i));
            addParam(
                createParamCentered<AttenuatorKnob>(wavePositions[i].cvParamPos, module, Spider::WAVE_CV_PARAMS + i));
            addInput(createInputCentered<DarkPJ301MPort>(wavePositions[i].inputPos, module, Spider::WAVE_INPUTS + i));

            addParam(createLightParamCentered<VCVLightButton<RedLight>>(
                opSelectorPositions[i], module, Spider::SELECT_PARAMS + i, Spider::SELECT_LIGHTS + i));
            addChild(createRingLight(opSelectorPositions[i], module, Spider::CARRIER_LIGHTS + i, i));

            if (module) {
                for (int j = 0; j < OPERATOR_COUNT; ++j) {
                    if (i == j)
                        continue;
                    addChild(createConnectionLight(opSelectorPositions[i], opSelectorPositions[j], module,
                                                   Spider::CONNECTION_LIGHTS + OPERATOR_COUNT * i + j, i, j));
                }
            }
        }
    }

    void drawOperators() {
        float x0 = 30;
        float y0 = 220;

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
        }
    }

    void step() override {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            for (int j = 0; j < OPERATOR_COUNT; ++j) {
                if (i == j)
                    continue;
            }
        }

        ModuleWidget::step();
    }

    constexpr static float opRadius = 48.f;

    constexpr static float pi = M_PI;
};

} // namespace ph

Model* modelPostHumanSpider = createModel<ph::Spider, ph::SpiderWidget>("Spider");