#include "plugin.hpp"
#include "DirectedGraph.hpp"
#include "Components.hpp"

#include "SignalGenerator.hpp"

#include <array>

namespace { // anonymous

constexpr int OPERATOR_COUNT = 6;
constexpr int CONNECTION_COUNT = OPERATOR_COUNT * OPERATOR_COUNT;
constexpr int BUFFER_SIZE = 512;
constexpr int MAX_CHANNEL_COUNT = 16;

} // anonymous namespace

namespace ph {

struct Spider : Module {

    enum ParamId {
        ENUMS(SELECT_PARAMS, OPERATOR_COUNT),
        ENUMS(WAVE_PARAMS, OPERATOR_COUNT),
        ENUMS(WAVE_CV_PARAMS, OPERATOR_COUNT),
        ENUMS(PITCH_PARAMS, OPERATOR_COUNT),
        ENUMS(PITCH_CV_PARAMS, OPERATOR_COUNT),
        ENUMS(LEVEL_PARAMS, OPERATOR_COUNT),
        ENUMS(LEVEL_CV_PARAMS, OPERATOR_COUNT),
        ENUMS(FEEDBACK_PARAMS, OPERATOR_COUNT),
        FREQ_PARAM,
        PARAMS_LEN
    };

    enum InputId {
        ENUMS(PITCH_INPUTS, OPERATOR_COUNT),
        ENUMS(LEVEL_INPUTS, OPERATOR_COUNT),
        ENUMS(WAVE_INPUTS, OPERATOR_COUNT),
        VOCT_INPUT,
        INPUTS_LEN
    };

    enum OutputId { AUDIO_OUTPUT, OUTPUTS_LEN };
    enum LightId { ENUMS(SELECT_LIGHTS, OPERATOR_COUNT * 3), ENUMS(CONNECTION_LIGHTS, CONNECTION_COUNT), LIGHTS_LEN };

    Spider() {
        configParameters();
        setConnectionLights();
    }

    void configParameters() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configInput(VOCT_INPUT, "1V/Oct");
        configOutput(AUDIO_OUTPUT, "Audio");
        configParam(FREQ_PARAM, -80, 80, 0.f, "Frequency", "Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);

        for (int op = 0; op < OPERATOR_COUNT; ++op) {
            std::string opStr = std::to_string(op + 1);

            configButton(SELECT_PARAMS + op, "Select operator " + opStr);
            configParam(PITCH_PARAMS + op, -12, 12, 0.f, "Operator " + opStr + " coarse pitch", " semitones");
            configParam(LEVEL_PARAMS + op, 0.f, 1.f, 0.f, "Operator " + opStr + " level", "%", 0.f, 100.f);
            configParam(WAVE_PARAMS + op, 0.f, 1.f, 0.f, "Operator " + opStr + " waveform blend", "%", 0.f, 100.f, 0.f);

            configParam(PITCH_CV_PARAMS + op, -1.f, 1.f, 0.f, "Operator " + opStr + " pitch CV", "%", 0.f, 100.f, 0.f);
            configParam(LEVEL_CV_PARAMS + op, -1.f, 1.f, 0.f, "Operator " + opStr + " level CV", "%", 0.f, 100.f, 0.f);
            configParam(WAVE_CV_PARAMS + op, -1.f, 1.f, 0.f, "Operator " + opStr + " waveform blend CV", "%", 0.f,
                        100.f, 0.f);
            configParam(FEEDBACK_PARAMS + op, 0.f, 1.f, 0.f, "Operator " + opStr + " feedback", "%", 0.f, 100.f);

            configInput(PITCH_INPUTS + op, "Operator " + opStr + " fine pitch");
            configInput(LEVEL_INPUTS + op, "Operator " + opStr + " level");
            configInput(WAVE_INPUTS + op, "Operator " + opStr + " waveform blend");

            getParamQuantity(PITCH_PARAMS + op)->snapEnabled = true;
        }
    }

    void processEdit(float sampleTime) {
        if (cycleDetected) {
            if (cycleFlashTimer.process(sampleTime) > 0.45f) {
                cycleDetected = false;
            }
        }

        bool cycleDetectedLow = cycleDetectedTrigger.process(!cycleDetected);

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            if (cycleDetectedLow) {
                getLight(SELECT_LIGHTS + i * 3 + 2).setBrightness(0.f);
                setCarrierLights();
            }

            if (cycleDetected) {
                getLight(SELECT_LIGHTS + i * 3).setBrightness(0.0f);
                getLight(SELECT_LIGHTS + i * 3 + 1).setBrightness(0.0f);

                float flashBrightness = fmod(cycleFlashTimer.getTime(), 0.15f) > 0.075f ? 1.0f : 0.0f;
                getLight(SELECT_LIGHTS + i * 3 + 2).setBrightness(flashBrightness);
            }

            getLight(SELECT_LIGHTS + i * 3 + 1).setBrightnessSmooth(selectedOperator == i, sampleTime);

            auto& trigger = operatorTriggers[i];
            bool selectTriggered = trigger.process(getParam(SELECT_PARAMS + i).getValue());

            if (selectTriggered) {
                if (selectedOperator == i) {
                    carriers[i] = !carriers[i];
                    getLight(SELECT_LIGHTS + i * 3).setBrightness(carriers[i] ? 1.0f : 0.0f);
                    updateTooltips(i, false);
                    selectedOperator = -1;

                } else if (selectedOperator > -1) {
                    auto result = algorithmGraph.toggleEdge(selectedOperator, i);

                    if (result == ToggleEdgeResult::CYCLE) {
                        updateTooltips(i, false);
                        selectedOperator = -1;
                        cycleDetected = true;
                        cycleFlashTimer.reset();
                        return;
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
                (carriers[op] ? "Unset " : "Set ") + std::string("operator ") + std::to_string(op + 1) + " as carrier";
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

    void reset() {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            reset(i);
        }
    }

    void reset(int op) {
        for (int c = 0; c < MAX_CHANNEL_COUNT; c++) {
            signalGenerators[op * MAX_CHANNEL_COUNT + c].reset();
        }

        bufferIndex[op] = 0;
        frameIndex[op] = 0;
        std::fill(pointBuffer[op].begin(), pointBuffer[op].end(), 0.f);
    }

    void process(const ProcessArgs& args) override {
        int newChannels = std::max(1, getInput(VOCT_INPUT).getChannels());

        if (channels != newChannels) {
            channels = newChannels;
            freqs.resize(OPERATOR_COUNT * channels);
            outs.resize(OPERATOR_COUNT * channels);
            oldOuts.resize(OPERATOR_COUNT * channels);
        }

        for (int op = 0; op < OPERATOR_COUNT; ++op) {
            for (int c = 0; c < channels; c++) {
                float freqParam = getParam(FREQ_PARAM).getValue() / 12.f;
                float pitch = freqParam + getInput(VOCT_INPUT).getPolyVoltage(c);
                freqs[op * channels + c] = dsp::FREQ_C4 * dsp::exp2_taylor5(pitch);
            }
        }

        processEdit(args.sampleTime);
        processOperators(args);

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

            getOutput(AUDIO_OUTPUT).setVoltage(5.f * output, c);
        }
    }

    void processOperators(const ProcessArgs& args) {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            int op = topologicalOrder[i];
            processOperator(op, args);
        }
    }

    void processOperator(int op, const ProcessArgs& args) {
        float levelParam = getParam(LEVEL_PARAMS + op).getValue();
        float levelCv = getParam(LEVEL_CV_PARAMS + op).getValue();
        float levelInput = getInput(LEVEL_INPUTS + op).getVoltage() / 10.f;

        float level = levelParam + (levelInput * levelCv);

        level = clamp(level, 0.f, 1.f);

        // if (level == 0.f) {
        // TODO: this needs to set outs[op] to 0 for all channels
        // if it's going to be gbrought back
        //     reset(op);
        //     return;
        // }

        float pitchShiftParam = getParam(PITCH_PARAMS + op).getValue();
        float pitchShiftCv = getParam(PITCH_CV_PARAMS + op).getValue();
        float pitchShiftInput = getInput(PITCH_INPUTS + op).getVoltage() / 10.f;

        float octaveShift = pitchShiftParam / 12.0f;

        float wavePosParam = getParam(WAVE_PARAMS + op).getValue();
        float wavePosCv = getParam(WAVE_CV_PARAMS + op).getValue();
        float wavePosInput = getInput(WAVE_INPUTS + op).getVoltage() / 10.f;
        float wavePos = wavePosParam + (wavePosCv * wavePosInput);
        wavePos = clamp(wavePos, 0.f, 1.f);

        float feedbackParam = getParam(FEEDBACK_PARAMS + op).getValue();

        const auto& modulators = algorithmGraph.getAdjacentVerticesRev(op);

        for (int c = 0; c < channels; c++) {
            float& freq = freqs[op * channels + c];

            freq *= dsp::exp2_taylor5(octaveShift);                    // +- 12 semitones coarse pitch
            freq *= dsp::exp2_taylor5(pitchShiftInput * pitchShiftCv); // +-12 semitones pitch shift

            // resize buffer whenever samplesPerCycle changes
            int samplesPerCycle = (args.sampleRate / freq);

            for (int mod : modulators) {
                float modFreq = freqs[mod * channels + c];
                freq += 5.f * modFreq * outs[mod * channels + c];
            }

            float old0 = outs[op * channels + c];
            float old1 = oldOuts[op * channels + c];

            float avgOldSample = (old0 + old1) / 2;

            float feedback = 5.f * feedbackParam * avgOldSample;

            outs[op * channels + c] =
                level * signalGenerators[op * MAX_CHANNEL_COUNT + c].generate(args.sampleTime, freq, wavePos, feedback);

            if (c == 0) {
                if (bufferIndex[op] < BUFFER_SIZE) { // Capture only one period
                    if (++frameIndex[op] >= samplesPerCycle) {
                        frameIndex[op] = 0;

                        pointBuffer[op][bufferIndex[op]] = outs[op * channels + c];
                        bufferIndex[op]++;
                    }
                } else {
                    bufferIndex[op] = 0;
                }
            }
        }
    }

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
        for (size_t i = 0; i < algorithmGraph.adjList.size(); ++i) {
            for (size_t j = 0; j < algorithmGraph.adjList[i].size(); ++j) {
                int lightId = CONNECTION_LIGHTS + i * OPERATOR_COUNT + algorithmGraph.adjList[i][j];
                getLight(lightId).setBrightness(1.0f);
            }
        }

        setCarrierLights();
    }

    void setCarrierLights() {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {

            getLight(SELECT_LIGHTS + i * 3).setBrightness(carriers[i] ? 1.0f : 0.0f);
        }
    }

    void clearConnectionLights() {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            getLight(SELECT_LIGHTS + i * 3).setBrightness(0.0f);

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
        reset();

        Module::onReset();
    }

    void onRandomize() override {
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

    std::array<SpiderSignalGenerator, OPERATOR_COUNT * 16> signalGenerators;
    std::vector<float> freqs;
    std::vector<float> outs;

    // Store previous samples to calculate operator feedback
    std::vector<float> oldOuts;
    int channels = -1;

    // only one channel is needed for the buffers
    // point buffer for the scope displays
    std::array<int, OPERATOR_COUNT> bufferIndex = {0, 0, 0, 0, 0, 0};
    std::array<int, OPERATOR_COUNT> frameIndex = {0, 0, 0, 0, 0, 0};
    std::array<std::array<float, BUFFER_SIZE>, OPERATOR_COUNT> pointBuffer = {};

    std::array<float, OPERATOR_COUNT> lastWavePos;
    std::array<bool, OPERATOR_COUNT> carriers = {};
    std::array<dsp::BooleanTrigger, OPERATOR_COUNT> operatorTriggers;
    std::vector<int> topologicalOrder = {0, 1, 2, 3, 4, 5};
    DirectedGraph<int> algorithmGraph{OPERATOR_COUNT};

    int selectedOperator = -1;
    bool cycleDetected = false;
    rack::dsp::TTimer<float> cycleFlashTimer;

    dsp::BooleanTrigger cycleDetectedTrigger;
};

struct SpiderDisplay : public OpaqueWidget {
    Spider* module = nullptr;

    SpiderDisplay() { this->box.size = Vec(56, 28); }

    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer != 1)
            return;

        if (!module)
            return;

        nvgStrokeWidth(args.vg, 0.5f);
        nvgScissor(args.vg, RECT_ARGS(args.clipBox));

        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGBA(10, 10, 10, 255));
        nvgFill(args.vg);

        auto innerBox = box;
        innerBox.pos.x += 4;
        innerBox.pos.y += 4;
        innerBox.size.x -= 8;
        innerBox.size.y -= 8;

        nvgBeginPath(args.vg);
        nvgRect(args.vg, 2, 2, box.size.x - 4, box.size.y - 4);
        nvgStrokeColor(args.vg, OPERATOR_COLOURS[op]);
        nvgStroke(args.vg);

        auto innerClipBox = args.clipBox;
        innerClipBox.pos.x += 3;
        innerClipBox.pos.y += 3;
        innerClipBox.size.x -= 7;
        innerClipBox.size.y -= 7;

        nvgScissor(args.vg, RECT_ARGS(innerClipBox));

        nvgBeginPath(args.vg);

        for (size_t i = 0; i < BUFFER_SIZE; ++i) {
            float t = float(i) / (BUFFER_SIZE - 1);
            float voltage = module->pointBuffer[op][(module->bufferIndex[op] + i) % BUFFER_SIZE];

            Vec point;
            point.x = t;
            point.y = 0.5f - (voltage / 2); // -1 to 1

            point = innerBox.size * point;

            if (i == 0) {
                nvgMoveTo(args.vg, point.x + 4, point.y + 4);
            } else {
                nvgLineTo(args.vg, point.x + 4, point.y + 4);
            }
        }

        nvgStrokeWidth(args.vg, 1.f);
        nvgLineCap(args.vg, NVG_ROUND);
        nvgStrokeColor(args.vg, OPERATOR_COLOURS[op]);
        nvgStroke(args.vg);
    }

    int op = 0;
};

struct SpiderWidget : ModuleWidget {
    // Multiple repeated vecs for each parameter and operator
    struct ParameterPositions {
        Vec paramPos;
        Vec inputPos;
        Vec cvParamPos;
    };

    const ParameterPositions multPositions[OPERATOR_COUNT] = {
        {Vec(86.414f, 35.903f), Vec(43.814f, 58.803f), Vec(86.414f, 83.783f)},
        {Vec(423.586f, 35.903f), Vec(466.186f, 58.803f), Vec(423.586f, 83.783f)},
        {Vec(484.68f, 212.875f), Vec(442.08f, 235.775f), Vec(484.68, 260.755f)},
        {Vec(423.586f, 320.253f), Vec(466.186f, 297.353f), Vec(423.586f, 272.373f)},
        {Vec(86.414f, 320.253), Vec(43.814, 297.353), Vec(86.414, 272.373)},
        {Vec(25.32f, 212.875), Vec(67.92, 235.775), Vec(25.32, 260.756)}};

    const ParameterPositions levelPositions[OPERATOR_COUNT] = {
        {Vec(197.814f, 56.903f), Vec(232.414f, 91.135f), Vec(197.814f, 109.703f)},
        {Vec(312.186f, 56.903f), Vec(277.586f, 91.135f), Vec(312.186f, 109.703f)},
        {Vec(402.022f, 155.209f), Vec(367.422f, 189.441), Vec(402.022f, 208.009f)},
        {Vec(312.186f, 299.253f), Vec(277.586f, 265.022), Vec(312.186f, 246.453f)},
        {Vec(197.814f, 299.253f), Vec(232.414f, 265.022), Vec(197.814f, 246.453f)},
        {Vec(107.978f, 155.209f), Vec(142.578f, 189.441), Vec(107.978f, 208.009f)}};

    const ParameterPositions wavePositions[OPERATOR_COUNT] = {
        {Vec(124.114f, 105.291f), Vec(161.914f, 82.003f), Vec(161.914f, 123.954f)},
        {Vec(385.886f, 105.291f), Vec(348.086f, 82.003f), Vec(348.086f, 124.954f)},
        {Vec(484.669f, 142.881f), Vec(442.08f, 119.803f), Vec(484.68f, 94.823f)},
        {Vec(385.886f, 250.865f), Vec(348.086f, 274.153f), Vec(348.086f, 232.202f)},
        {Vec(124.114f, 250.865f), Vec(161.914f, 274.153f), Vec(161.914f, 232.202f)},
        {Vec(25.331f, 142.881f), Vec(67.92f, 119.803f), Vec(25.32f, 94.823)}};

    const Vec displayPositions[OPERATOR_COUNT] = {Vec(143.163f, 35.903f),  Vec(367.837f, 35.903f),
                                                  Vec(472.746f, 179.199f), Vec(367.837f, 318.14f),
                                                  Vec(142.553f, 318.14f),  Vec(37.254f, 179.199f)};

    const Vec opSelectorPositions[OPERATOR_COUNT] = {Vec(218.464f, 144.534f), Vec(290.345f, 144.534f),
                                                     Vec(326.025f, 177.914f), Vec(290.345f, 210.825f),
                                                     Vec(218.464f, 210.825f), Vec(183.003f, 177.914f)};
    SpiderWidget(Spider* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Spider.svg")));

        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            SpiderDisplay* display = createWidgetCentered<SpiderDisplay>(displayPositions[i]);

            if (module) {
                display->module = module;
                display->op = i;
            }

            addChild(display);

            addParam(createParamCentered<ShinyKnob>(multPositions[i].paramPos, module, Spider::PITCH_PARAMS + i));
            addParam(
                createParamCentered<AttenuatorKnob>(multPositions[i].cvParamPos, module, Spider::PITCH_CV_PARAMS + i));
            addInput(createInputCentered<HexJack>(multPositions[i].inputPos, module, Spider::PITCH_INPUTS + i));

            addParam(createParamCentered<ShinyKnob>(levelPositions[i].paramPos, module, Spider::LEVEL_PARAMS + i));
            addParam(
                createParamCentered<AttenuatorKnob>(levelPositions[i].cvParamPos, module, Spider::LEVEL_CV_PARAMS + i));
            addInput(createInputCentered<HexJack>(levelPositions[i].inputPos, module, Spider::LEVEL_INPUTS + i));

            addParam(createParamCentered<ShinyKnob>(wavePositions[i].paramPos, module, Spider::WAVE_PARAMS + i));
            addParam(
                createParamCentered<AttenuatorKnob>(wavePositions[i].cvParamPos, module, Spider::WAVE_CV_PARAMS + i));
            addInput(createInputCentered<HexJack>(wavePositions[i].inputPos, module, Spider::WAVE_INPUTS + i));

            addParam(createParamCentered<PushKnob>(opSelectorPositions[i], module, Spider::FEEDBACK_PARAMS + i));
            addParam(createParamCentered<OperatorButton>(opSelectorPositions[i], module, Spider::SELECT_PARAMS + i));
            addChild(createOperatorLight(opSelectorPositions[i], module, Spider::SELECT_LIGHTS + i * 3, i));

            if (module) {
                for (int j = 0; j < OPERATOR_COUNT; ++j) {
                    if (i == j)
                        continue;
                    addChild(createConnectionLight(opSelectorPositions[i], opSelectorPositions[j], module,
                                                   Spider::CONNECTION_LIGHTS + OPERATOR_COUNT * i + j, i));
                }
            }
        }

        addChild(createInputCentered<HexJack>(Vec(198.6f, 345.0f), module, Spider::VOCT_INPUT));
        addChild(createOutputCentered<HexJack>(Vec(310.764f, 345.0f), module, Spider::AUDIO_OUTPUT));

        addChild(createParamCentered<ShinyBigKnob>(Vec(255.f, 334.59f), module, Spider::FREQ_PARAM));
    }
};

} // namespace ph

Model* modelPostHumanSpider = createModel<ph::Spider, ph::SpiderWidget>("PostHuman-Spider");

#ifdef PH_UNIT_TESTS
#    include <test_Spider.hpp>
#endif
