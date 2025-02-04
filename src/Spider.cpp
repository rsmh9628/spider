#include "plugin.hpp"
#include "DirectedGraph.hpp"
#include "Components.hpp"

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
    enum LightId {
        ENUMS(SELECT_LIGHTS, OPERATOR_COUNT * 2),
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
            configParam(PITCH_PARAMS + op, -12, 12, 0.f, "Operator " + opStr + " coarse pitch", " semitones");
            configParam(LEVEL_PARAMS + op, 0.f, 1.f, 0.f, "Operator " + opStr + " level", "%", 0.f, 100.f);
            configParam(WAVE_PARAMS + op, 0.f, 1.f, 0.f, "Operator " + opStr + " wave", "%", 0.f, 100.f,
                        0.f);

            configParam(PITCH_CV_PARAMS + op, -1.f, 1.f, 0.f, "Operator " + opStr + " pitch CV", "%", 0.f, 100.f,
                        0.f);
            configParam(LEVEL_CV_PARAMS + op, -1.f, 1.f, 0.f, "Operator " + opStr + " level CV", "%", 0.f, 100.f, 0.f);
            configParam(WAVE_CV_PARAMS + op, -1.f, 1.f, 0.f, "Operator " + opStr + " wave CV", "%", 0.f,
                        100.f, 0.f);

            configInput(PITCH_INPUTS + op, "Operator " + opStr + " fine pitch");
            configInput(LEVEL_INPUTS + op, "Operator " + opStr + " level");
            configInput(WAVE_INPUTS + op, "Operator " + opStr + " wave");

            getParamQuantity(PITCH_PARAMS + op)->snapEnabled = true;
        }
    }

    void processEdit(float sampleTime) {        
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            getLight(SELECT_LIGHTS + i*2 + 1).setBrightnessSmooth(selectedOperator == i, sampleTime);

            auto& trigger = operatorTriggers[i];
            int selectTriggered = trigger.process(getParam(SELECT_PARAMS + i).getValue());

            if (selectTriggered) {
                if (selectedOperator == i) {
                    carriers[i] = !carriers[i];
                    getLight(SELECT_LIGHTS + i*2).setBrightness(carriers[i] ? 1.0f : 0.0f);
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

    void reset() {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            reset(i);
        }
    }

    void reset(int op) {
        auto *opPhase = &phase[op * MAX_CHANNEL_COUNT];
        std::fill(opPhase, opPhase + MAX_CHANNEL_COUNT, 0.f);
        bufferIndex[op] = 0;
        frameIndex[op] = 0;
        std::fill(pointBuffer[op].begin
        (), pointBuffer[op].end(), 0.f);
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
        auto outs = processOperators(channels, args, freqs);

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

    std::vector<float> processOperators(int channels, const ProcessArgs& args, std::vector<float>& freqs) {
        std::vector<float> outs(OPERATOR_COUNT * channels);
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            int op = topologicalOrder[i];
            processOperator(op, channels, args, outs, freqs);
        }

        return outs;
    }

    void processOperator(int op, int channels, const ProcessArgs& args, std::vector<float>& outs, std::vector<float>& freqs) {
        float levelParam = getParam(LEVEL_PARAMS + op).getValue();
        float levelCv = getParam(LEVEL_CV_PARAMS + op).getValue();
        float levelInput = getInput(LEVEL_INPUTS + op).getVoltage() / 10.f;
        float level = levelParam + (levelInput * levelCv);

        level = clamp(level, 0.f, 1.f);

        if (level == 0.f) {
            reset(op);
            return;
        }
        
        float pitchShiftParam = getParam(PITCH_PARAMS + op).getValue();
        float pitchShiftCv = getParam(PITCH_CV_PARAMS + op).getValue();
        float pitchShiftInput = getInput(PITCH_INPUTS + op).getVoltage() / 10.f;

        //if (getInput(PITCH_INPUTS + op).isConnected()) {
        //    pitchShift += pitchShiftCv * getInput(PITCH_INPUTS + op).getVoltage() / 10.f;
        //}

        float pitchShiftExponent = pitchShiftParam / 12.0f;

        float wavePosParam = getParam(WAVE_PARAMS + op).getValue();
        float wavePosCv = getParam(WAVE_CV_PARAMS + op).getValue();
        float wavePosInput = getInput(WAVE_INPUTS + op).getVoltage() / 10.f;
        float wavePos = wavePosParam + (wavePosCv * wavePosInput);
        wavePos = clamp(wavePos, 0.f, 1.f);

        const auto& modulators = algorithmGraph.getAdjacentVerticesRev(op);

        for (int c = 0; c < channels; c++) {
            float modulation = 0.f;
            float& freq = freqs[op * channels + c];

            freq *= dsp::exp2_taylor5(pitchShiftExponent); // +- 12 semitones coarse pitch
            freq *= dsp::exp2_taylor5(pitchShiftInput * pitchShiftCv); // +-12 semitones pitch shift            
            // float freq = freqs[c] * dsp::exp2_taylor5(pitchShiftExponent);
            // freq += 5 * modulation;

            // calculate this before any modulation so we have a good baseline update for the scope


            // Resize buffer whenever samplesPerCycle changes
            int samplesPerCycle = (args.sampleRate / freq);

            for (int mod : modulators) {
                float modFreq = freqs[mod * channels + c];
                freq += 5 * modFreq * outs[mod * channels + c];
            }

            float& ph = phase[op * MAX_CHANNEL_COUNT + c];


            ph += freq * args.sampleTime;

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

            float normalizedPh = ph; 
            if (normalizedPh < 0) {
                normalizedPh += 1.0f;
            }
            float sine = sin2pi(normalizedPh);
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

            if (c == 0) {                
                if (bufferIndex[op] < BUFFER_SIZE) { // Capture only one period
                    if (++frameIndex[op] >= samplesPerCycle) {
                        frameIndex[op] = 0;

                        pointBuffer[op][bufferIndex[op]] = outs[op * channels + c];
                        bufferIndex[op]++;
                    }
                }
                else {
                    bufferIndex[op] = 0;
                }
            }
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
            getLight(SELECT_LIGHTS + i*2).setBrightness(carriers[i] ? 1.0f : 0.0f);
        }
    }

    void clearConnectionLights() {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            getLight(SELECT_LIGHTS + i*2).setBrightness(0.0f);

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

        reset();

        Module::onRandomize();
    }

    std::array<float, OPERATOR_COUNT * 16> phase;

    // only one channel is needed for the buffers
    
    // point buffer for the scope displays
    std::array<int, OPERATOR_COUNT> bufferIndex = {0,0,0,0,0,0};
    std::array<int, OPERATOR_COUNT> frameIndex = {0,0,0,0,0,0};
    std::array<std::array<float, BUFFER_SIZE>, OPERATOR_COUNT> pointBuffer = {};

    // std::array<Wavetable, OPERATOR_COUNT> wavetables;
    std::array<float, OPERATOR_COUNT> lastWavePos;
    std::array<bool, OPERATOR_COUNT> carriers = {};
    std::array<dsp::BooleanTrigger, OPERATOR_COUNT> operatorTriggers;
    std::vector<int> topologicalOrder = {0, 1, 2, 3, 4, 5};
    DirectedGraph<int> algorithmGraph{OPERATOR_COUNT};

    int selectedOperator = -1;
};

struct SpiderDisplay : public OpaqueWidget {
    Spider *module = nullptr;

    SpiderDisplay() {
        this->box.size = Vec(56, 28);
    }

    void draw(const DrawArgs& args) override {

    }

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

// TODO: Organise
struct SpiderWidget : ModuleWidget {

    // Multiple repeated params for each parameter and operator
    struct ParameterPositions {
        Vec paramPos;
        Vec inputPos;
        Vec cvParamPos;
    };

    const ParameterPositions multPositions[OPERATOR_COUNT] = {
        {Vec(86.414, 35.903), Vec(43.814, 58.803), Vec(86.414, 83.783)},
        {Vec(422.586, 35.903), Vec(465.186, 58.803), Vec(422.586, 83.783)},
        {Vec(484.68, 212.875), Vec(442.08, 235.775), Vec(484.68, 260.755)},
        {Vec(422.586, 320.253), Vec(465.186, 297.353), Vec(422.586, 272.373)},
        {Vec(86.414, 320.253), Vec(43.814, 297.353), Vec(86.414, 272.373)},
        {Vec(25.32, 212.875), Vec(67.92, 235.775), Vec(25.32, 260.756)}};

    const ParameterPositions levelPositions[OPERATOR_COUNT] = {
        {Vec(197.814, 56.903), Vec(232.414, 91.135), Vec(197.814, 109.703)},
        {Vec(311.186, 56.903), Vec(276.586, 91.135), Vec(311.186, 109.703)},
        {Vec(402.022, 155.209), Vec(367.422, 189.441), Vec(402.022, 208.009)},
        {Vec(311.186, 299.253), Vec(276.586, 265.022), Vec(311.186, 246.453)},
        {Vec(197.814, 299.253), Vec(232.414, 265.022), Vec(197.814, 246.453)},
        {Vec(107.978, 155.209), Vec(142.578, 189.441), Vec(107.978, 208.009)}};

    const ParameterPositions wavePositions[OPERATOR_COUNT] = {
        {Vec(124.114, 105.291), Vec(161.914, 82.003), Vec(161.914, 123.954)},
        {Vec(384.886, 105.291), Vec(347.086, 82.003), Vec(347.086, 123.954)},
        {Vec(484.669, 142.881), Vec(442.08, 119.803), Vec(484.68, 94.823)},
        {Vec(384.886, 250.865), Vec(347.086, 274.153), Vec(347.086, 232.202)},
        {Vec(124.114, 250.865), Vec(161.914, 274.153), Vec(161.914, 232.202)},
        {   Vec(25.331, 142.881), Vec(67.92, 119.803), Vec(25.32, 94.823)}};

    const Vec displayPositions[OPERATOR_COUNT] = {Vec(143.163, 35.903), Vec(366.837, 35.903),
                                                  Vec(472.746, 179.199), Vec(366.837, 318.14),
                                                  Vec(142.553, 318.14), Vec(37.254, 179.199)};

    const Vec opSelectorPositions[OPERATOR_COUNT] = {Vec(218.464, 144.534),  Vec(290.345, 144.534),
                                                     Vec(326.025, 177.914), Vec(290.345, 210.825),
                                                     Vec(218.464, 210.825),  Vec(183.003, 177.914)};
    SpiderWidget(Spider* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/PostHumanFM.svg")));

        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            // TODO: Clean this up can use only one set of addparam addinput etc. with clever
            SpiderDisplay* display = createWidgetCentered<SpiderDisplay>(displayPositions[i]);

            if (module) {
                // display->wavetable = &fmModule->wavetables[i];
                //  todo: wavepositioin for control
                // display->wavePos = &fmModule->lastWavePos[i];
                display->module = module;
                display->op = i;
            }
            // display->op = i;

            // addChild(display);
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

            addParam(createParamCentered<OperatorButton>(
                opSelectorPositions[i], module, Spider::SELECT_PARAMS + i));
            addChild(createHexLight(opSelectorPositions[i], module, Spider::SELECT_LIGHTS + i*2, i));

            if (module) {
                for (int j = 0; j < OPERATOR_COUNT; ++j) {
                    if (i == j)
                        continue;
                    addChild(createConnectionLight(opSelectorPositions[i], opSelectorPositions[j], module,
                                                   Spider::CONNECTION_LIGHTS + OPERATOR_COUNT * i + j, i));
                }
            }
        }

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
        }

        addChild(createInputCentered<HexJack>(mm2px(Vec(67.37f, 117.008f)), module, Spider::VOCT_INPUT));
        addChild(createOutputCentered<HexJack>(mm2px(Vec(105.375f, 117.008f)), module, Spider::AUDIO_OUTPUT));

        addChild(createParamCentered<ShinyBigKnob>(mm2px(Vec(86.360f, 113.318f)), module, Spider::FREQ_PARAM));
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