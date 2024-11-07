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
        EDIT_PARAM,
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

            // Linearly interpolate between the two waves
            out = level * crossfade(out0, out1, pos);
        }

        float phase = 0.f;
        float out = 0.f;
        float lastWavePos;

        void reset() {
            phase = 0.f;
            out = 0.f;
        }

        Wavetable wavetable;
    };

    PostHumanFM() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configInput(VOCT_INPUT, "V/Oct");
        configOutput(AUDIO_OUTPUT, "Audio");
        configButton(EDIT_PARAM, "Edit");

        for (int op = 0; op < OPERATOR_COUNT; ++op) {
            configButton(SELECT_PARAMS + op, "Operator " + std::to_string(op + 1));
            configParam(MULT_PARAMS + op, -3, 3, 0.f, "Multiplier", "x", 2.f);
            // getParamQuantity(MULT_PARAMS + op)->snapEnabled = true;
            configParam(LEVEL_PARAMS + op, 0.f, 1.f, 0.f, "Level");
        }

        for (int conn = 0; conn < CONNECTION_COUNT; ++conn) {
            configLight(CONNECTION_LIGHTS + conn);
        }

        // configParam(OPERATOR_PARAM, 0.f, OPERATOR_COUNT - 1, 0.f, "Operator", "", 0.f, 1.f, 1.f);
        // getParamQuantity(OPERATOR_PARAM)->snapEnabled = true;

        // configButton(SELECT_PARAM, "Select");

        // for (int i = 0; i < OPERATOR_COUNT - 1; i++) {
        //     algorithmGraph.addEdge(i, i + 1);
        // }
        // topologicalOrder = algorithmGraph.topologicalSort();
    }

    // void processSelect() {
    //     for (int i = 0; i < OPERATOR_COUNT; ++i) {
    //         auto& trigger = operatorTriggers[i];
    //         int selectTriggered = trigger.process(getParam(SELECT_PARAMS + i).getValue());
    //         if (selectTriggered) {
    //             currentOperator = i;
    //         }
    //
    //        auto& operatorLight = getLight(SELECT_LIGHTS + i);
    //        operatorLight.setBrightness(currentOperator == i);
    //    }
    //}

    void processEdit() {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            auto& trigger = operatorTriggers[i];
            int selectTriggered = trigger.process(getParam(SELECT_PARAMS + i).getValue());

            if (selectTriggered) {
                if (selectedOperator > -1) {
                    algorithmGraph.addEdge(selectedOperator, i);
                    getLight(CONNECTION_LIGHTS + OPERATOR_COUNT * selectedOperator + i).setBrightness(1.f);
                    printf("Light added for %d", OPERATOR_COUNT * selectedOperator + i);
                    printf("Added edge %d -> %d\n", selectedOperator, i);
                    selectedOperator = -1;
                    topologicalOrder = algorithmGraph.topologicalSort();

                    for (auto& op : operators) {
                        op.reset(); // Reset phase of all operators
                    }

                } else {
                    selectedOperator = i;
                }

                printf("Selected operator %d\n", i);
            }
            auto& operatorLight = getLight(SELECT_LIGHTS + i);
            operatorLight.setBrightness(selectedOperator == i);
        }
    }

    void process(const ProcessArgs& args) override {
        float voct = inputs[VOCT_INPUT].getVoltage();
        float freq = dsp::FREQ_C4 * std::pow(2.f, voct);

        for (int conn = 0; conn < CONNECTION_COUNT; ++conn) {
            // getLight(CONNECTION_LIGHTS + conn).setBrightness(1.f);
        }

        processEdit();

        // getLight(CONNECTION_LIGHTS + 8).setBrightness(1.f);
        //  getLight(CONNECTION_LIGHTS + OPERATOR_COUNT * 3 + 5).setBrightness(1.f);

        // bool selectTriggered = selectTrigger.process(getParam(SELECT_PARAM).getValue());

        // if (selectTriggered) {
        //     //int newOperator = getParam(OPERATOR_PARAM).getValue();
        //
        //    if (selectedOperator > -1) {
        //        algorithmGraph.addEdge(selectedOperator, newOperator);
        //        printf("Added edge %d -> %d\n", selectedOperator, newOperator);
        //        selectedOperator = -1;
        //        topologicalOrder = algorithmGraph.topologicalSort();
        //    } else {
        //        selectedOperator = newOperator;
        //    }
        //
        //    printf("Selected operator %d\n", newOperator);
        //}
        //// todo: this can eventually be parallelised with SIMD once independent carriers can be found from the graph

        // todo: temporarily disabling DSP
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            int op = topologicalOrder[i];

            auto& multParam = getParam(MULT_PARAMS + op);
            auto& levelParam = getParam(LEVEL_PARAMS + op);

            auto level = levelParam.getValue();
            auto wavePos = getParam(WAVE_PARAMS + op).getValue();

            if (getInput(LEVEL_INPUTS + op).isConnected()) {
                level = inputs[LEVEL_INPUTS + op].getVoltage() / 10.f;
                level = getParam(LEVEL_CV_PARAMS + op).getValue() * level;
            }

            float mult = std::pow(2.f, multParam.getValue());

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
        {mm2px(Vec(29.421, 13.254)), mm2px(Vec(15.007, 21.283)), mm2px(Vec(29.421, 29.582))},
        {mm2px(Vec(143.299, 13.195)), mm2px(Vec(157.713, 21.224)), mm2px(Vec(143.299, 29.588))},
        {mm2px(Vec(165.097, 44.598)), mm2px(Vec(150.682, 39.744)), mm2px(Vec(165.097, 31.380))},
        {mm2px(Vec(143.302, 107.098)), mm2px(Vec(157.717, 99.069)), mm2px(Vec(143.302, 90.705))},
        {mm2px(Vec(29.421, 107.098)), mm2px(Vec(15.007, 99.069)), mm2px(Vec(29.421, 90.705))},
        {mm2px(Vec(7.596, 44.598)), mm2px(Vec(22.011, 39.744)), mm2px(Vec(7.596, 31.380))}};

    const ParameterPositions levelPositions[OPERATOR_COUNT] = {
        {mm2px(Vec(68.772, 14.322)), mm2px(Vec(78.860, 30.447)), mm2px(Vec(66.430, 37.623))},
        {mm2px(Vec(103.983, 14.350)), mm2px(Vec(93.895, 30.475)), mm2px(Vec(106.325, 37.652))},
        {mm2px(Vec(165.097, 74.120)), mm2px(Vec(150.682, 78.974)), mm2px(Vec(165.097, 87.337))},
        {mm2px(Vec(106.632, 99.748)), mm2px(Vec(94.957, 89.289)), mm2px(Vec(107.386, 82.113))},
        {mm2px(Vec(66.091, 100.123)), mm2px(Vec(77.767, 89.289)), mm2px(Vec(65.337, 82.113))},
        {mm2px(Vec(7.596, 74.120)), mm2px(Vec(22.011, 78.974)), mm2px(Vec(7.596, 87.337))}};

    //    const Vec multParamPos[OPERATOR_COUNT] = {mm2px(Vec(29.421, 13.254))};
    // const Vec multInputPos[OPERATOR_COUNT] = {mm2px(Vec(15.007, 21.283))};
    // const Vec multCvParamPos[OPERATOR_COUNT] = {mm2px(Vec(29.421, 29.582))};
    //
    // const Vec waveParamPos[OPERATOR_COUNT] = {mm2px(Vec(42.211, 36.931))};
    // const Vec waveInputPos[OPERATOR_COUNT] = {mm2px(Vec(55.000, 27.319))};
    // const Vec waveCvParamPos[OPERATOR_COUNT] = {mm2px(Vec(55.000, 44.215))};
    //
    // const Vec levelParamPos[OPERATOR_COUNT] = {mm2px(Vec(68.737, 14.408))};
    // const Vec levelInputPos[OPERATOR_COUNT] = {mm2px(Vec(78.825, 30.534))};
    // const Vec levelCvParamPos[OPERATOR_COUNT] = {mm2px(Vec(66.386, 37.710))};

    PostHumanFMWidget(PostHumanFM* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/PostHumanFM.svg")));

        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // addChild(
        //     createParamCentered<RoundHugeBlackKnob>(Vec(box.size.x / 2, 250), module, PostHumanFM::OPERATOR_PARAM));

        // addChild(createParamCentered<VCVButton>(Vec(box.size.x / 2, 250), module, PostHumanFM::SELECT_PARAM));

        // auto* display = createWidget<Display>(mm2px(Vec(4.963, 13.049)));
        // display->module = module;
        // addChild(display);

        // addChild(createParamCentered<VCVLatch>(Vec(64, 64), module, PostHumanFM::EDIT_PARAM));

        if (!module)
            return;

        drawOpControls();

        // addChild(createInputCentered<PJ301MPort>(Vec(100, 256), module, PostHumanFM::VOCT_INPUT));
        // addChild(createOutputCentered<PJ301MPort>(Vec(100, 300), module, PostHumanFM::AUDIO_OUTPUT));

        // for (int i = 0; i < PostHumanFM::OPERATOR_COUNT; ++i) {
        //     for (int j = 0; j < PostHumanFM::OPERATOR_COUNT; ++j) {
        //         if (i == j)
        //             continue;
        //         addChild(createConnectionLight(Vec(x + opX(i), y + opY(i)), Vec(x + opX(j), y + opY(j)), module,
        //                                        PostHumanFM::CONNECTION_LIGHTS + i * j));
        //     }
        // }

        // todo: move to helper function to draw each operator

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
        }

        addChild(createInputCentered<PJ301MPort>(mm2px(Vec(10.f, 41.f + 70.f)), module, PostHumanFM::VOCT_INPUT));
        addChild(createOutputCentered<PJ301MPort>(mm2px(Vec(30.f, 41.f + 70.f)), module, PostHumanFM::AUDIO_OUTPUT));
    }

    void drawOpControls() {
        // const int x = mm2px(20.32f);
        // const int y = mm2px(45.72f);
        //
        // std::vector<Vec> opPositions;
        //
        // for (int op = 0; op < OPERATOR_COUNT; ++op) {
        //    // TODO: clean up and move to helper function
        //
        //    double angle = M_PI_2 + op * 2 * M_PI / OPERATOR_COUNT;
        //    float opX = std::cos(angle) * opRadius;
        //    float opY = std::sin(angle) * opRadius;
        //
        //    opPositions.push_back(Vec(x + opX, y + opY));
        //
        //    addParam(createLightParamCentered<VCVLightBezel<MediumSimpleLight<GreenLight>>>(
        //        Vec(x + opX, y + opY), module, PostHumanFM::SELECT_PARAMS + op, PostHumanFM::SELECT_LIGHTS + op));
        //
        //    // addChild(createParamCentered<RoundBlackKnob>(Vec(RACK_GRID_WIDTH + 40 * op, 240), module,
        //    //                                              PostHumanFM::MULT_PARAMS + op));
        //    // addChild(createParamCentered<RoundBlackKnob>(Vec(RACK_GRID_WIDTH + 40 * op, 270), module,
        //    //                                              PostHumanFM::LEVEL_PARAMS + op));
        //    //
        //    // addChild(createParamCentered<Trimpot>(Vec(RACK_GRID_WIDTH + 40 * op, 300), module,
        //    //                                      PostHumanFM::LEVEL_CV_PARAMS + op));
        //    // addChild(createInputCentered<PJ301MPort>(Vec(RACK_GRID_WIDTH + 40 * op, 350), module,
        //    //                                         PostHumanFM::LEVEL_INPUTS + op));)
        //}
        //
        //// for (int i = 0; i < OPERATOR_COUNT; ++i) {
        ////     drawOperator(51.6467f + 22.014f * i, 41.f, i);
        //// }
        //
        //// TODO: add the positions to a vector and do this in a loop

        // todo: tempo
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            // TODO: Clean this up can use only one set of addparam addinput etc. with clever

            addParam(
                createParamCentered<RoundBlackKnob>(multPositions[i].paramPos, module, PostHumanFM::MULT_PARAMS + i));
            addParam(
                createParamCentered<Trimpot>(multPositions[i].cvParamPos, module, PostHumanFM::MULT_CV_PARAMS + i));
            addInput(
                createInputCentered<DarkPJ301MPort>(multPositions[i].inputPos, module, PostHumanFM::MULT_INPUTS + i));

            addParam(
                createParamCentered<RoundBlackKnob>(levelPositions[i].paramPos, module, PostHumanFM::LEVEL_PARAMS + i));
            addParam(
                createParamCentered<Trimpot>(levelPositions[i].cvParamPos, module, PostHumanFM::LEVEL_CV_PARAMS + i));
            addInput(
                createInputCentered<DarkPJ301MPort>(levelPositions[i].inputPos, module, PostHumanFM::LEVEL_INPUTS + i));
        }

        // addParam(createParamCentered<RoundBlackKnob>(waveParamPos[0], module, PostHumanFM::WAVE_PARAMS + 0));
        // addParam(createParamCentered<Trimpot>(waveCvParamPos[0], module, PostHumanFM::WAVE_CV_PARAMS + 0));
        // addInput(createInputCentered<DarkPJ301MPort>(waveInputPos[0], module, PostHumanFM::WAVE_INPUTS + 0));
        //
        // addParam(createParamCentered<RoundBlackKnob>(levelParamPos[0], module, PostHumanFM::LEVEL_PARAMS + 0));
        // addParam(createParamCentered<Trimpot>(levelCvParamPos[0], module, PostHumanFM::LEVEL_CV_PARAMS + 0));
        // addInput(createInputCentered<DarkPJ301MPort>(levelInputPos[0], module, PostHumanFM::LEVEL_INPUTS + 0));

        // for (int i = 0; i < OPERATOR_COUNT; ++i) {
        //     for (int j = 0; j < OPERATOR_COUNT; ++j) {
        //         if (i == j)
        //             continue;
        //         addChild(createConnectionLight(opPositions[i], opPositions[j], module,
        //                                        PostHumanFM::CONNECTION_LIGHTS + OPERATOR_COUNT * i + j));
        //     }
        // }
    }

    void drawOperator(float x, float y, float op) {
        // TODO: CV percentage on a multi-knob like Serum

        auto* fmModule = dynamic_cast<PostHumanFM*>(module);

        auto* display = createWidgetCentered<WavetableDisplay>(mm2px(Vec(x, y - 12)));
        display->wavetable = &fmModule->operators[op].wavetable;
        display->wavePos = &fmModule->operators[op].lastWavePos;

        addChild(display);

        addChild(createInputCentered<PJ301MPort>(mm2px(Vec(x, y)), module, PostHumanFM::MULT_INPUTS + op));
        // addChild(createParamCentered<Trimpot>(mm2px(Vec(x - 7, y + 15)), module, PostHumanFM::WAVE_CV_PARAMS + op));
        addChild(createParamCentered<CKSSThreeHorizontal>(mm2px(Vec(x, y)), module, PostHumanFM::WAVE_PARAMS + op));
        addChild(
            createParamCentered<RoundBlackKnob>(mm2px(Vec(x + 5.5, y + 40)), module, PostHumanFM::LEVEL_PARAMS + op));
        addChild(
            createParamCentered<RoundBlackKnob>(mm2px(Vec(x - 5.5, y + 55)), module, PostHumanFM::MULT_PARAMS + op));

        // addChild(createParamCentered<Trimpot>(mm2px(Vec(x - 5.5, y + 57)), module, PostHumanFM::MULT_CV_PARAMS +
        // op)); addChild(createParamCentered<Trimpot>(mm2px(Vec(x + 5.5, y + 57)), module, PostHumanFM::LEVEL_CV_PARAMS
        // + op));

        addChild(createInputCentered<PJ301MPort>(mm2px(Vec(x + 5.5, y + 70)), module, PostHumanFM::LEVEL_INPUTS + op));
        addChild(createInputCentered<PJ301MPort>(mm2px(Vec(x - 5.5, y + 70)), module, PostHumanFM::MULT_INPUTS + op));

        // addChild(createParamCentered<RoundBlackKnob>(Vec(x0 + i * 35, y0), module, PostHumanFM::MULT_PARAMS + i));
        // addChild(createParamCentered<RoundBlackKnob>(Vec(x0 + i * 35, y0 + 40), module, PostHumanFM::LEVEL_PARAMS +
        // i)); addChild(createParamCentered<Trimpot>(Vec(x0 + i * 35, y0 + 80), module, PostHumanFM::LEVEL_CV_PARAMS +
        // i)); addChild(createInputCentered<PJ301MPort>(Vec(x0 + i * 35, y0 + 120), module, PostHumanFM::LEVEL_INPUTS +
        // i));
    }

    void drawOperators() {
        float x0 = 30;
        float y0 = 220;

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
        }
    }

    // void setKnobOperator(int newOperator) {
    //     if (!module)
    //         return;
    //
    //    // todo: loop through both knobs
    //
    //    for (int param = PostHumanFM::MULT_PARAMS; param < PostHumanFM::MULT_PARAMS_LAST; ++param) {
    //        auto* oldKnob = getParam(PostHumanFM::MULT_PARAMS + currentOperator);
    //        oldKnob->hide();
    //
    //        auto* newKnob = getParam(PostHumanFM::MULT_PARAMS + newOperator);
    //        newKnob->show();
    //    }
    //
    //    for (int param = PostHumanFM::LEVEL_PARAMS; param < PostHumanFM::LEVEL_PARAMS_LAST; ++param) {
    //        auto* oldKnob = getParam(PostHumanFM::LEVEL_PARAMS + currentOperator);
    //        oldKnob->hide();
    //
    //        auto* newKnob = getParam(PostHumanFM::LEVEL_PARAMS + newOperator);
    //        newKnob->show();
    //    }
    //}
    //
    // void step() override {
    //    if (!module)
    //        return;
    //
    //    auto* phModule = dynamic_cast<PostHumanFM*>(module);
    //
    //    auto moduleCurrentOperator = static_cast<PostHumanFM*>(module)->currentOperator;
    //
    //    if (moduleCurrentOperator != currentOperator) {
    //        setKnobOperator(moduleCurrentOperator);
    //        currentOperator = moduleCurrentOperator;
    //    }
    //
    //    ModuleWidget::step();
    //}

    // Layout

    constexpr static float opRadius = 48.f;

    constexpr static float pi = M_PI;

    // constexpr float opX(int op) {
    //     switch (op) {
    //     case 0:
    //         return opRadius * cos(pi / 6);
    //     case 1:
    //         return opRadius * cos(pi / 2);
    //     case 2:
    //         return opRadius * cos(5 * pi / 6);
    //     case 3:
    //         return opRadius * cos(7 * pi / 6);
    //     case 4:
    //         return opRadius * cos(3 * pi / 2);
    //     case 5:
    //         return opRadius * cos(11 * pi / 6);
    //     default:
    //         return 0;
    //     }
    // }
    //
    // constexpr float opY(int op) {
    //    switch (op) {
    //    case 0:
    //        return opRadius * sin(pi / 6);
    //    case 1:
    //        return opRadius * sin(pi / 2);
    //    case 2:
    //        return opRadius * sin(5 * pi / 6);
    //    case 3:
    //        return opRadius * sin(7 * pi / 6);
    //    case 4:
    //        return opRadius * sin(3 * pi / 2);
    //    case 5:
    //        return opRadius * sin(11 * pi / 6);
    //    default:
    //        return 0;
    //    }
    //}
};

} // namespace ph

Model* modelPostHumanFM = createModel<ph::PostHumanFM, ph::PostHumanFMWidget>("PostHumanFM");