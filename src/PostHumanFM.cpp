#include "plugin.hpp"
#include "DirectedGraph.hpp"
#include "Components.hpp"

#include <array>

namespace ph {

struct PostHumanFM : Module {

    static constexpr int OPERATOR_COUNT = 6;
    static constexpr int CONNECTION_COUNT = OPERATOR_COUNT * OPERATOR_COUNT;

    enum ParamId {
        ENUMS(SELECT_PARAMS, OPERATOR_COUNT),
        ENUMS(MULT_PARAMS, OPERATOR_COUNT),
        ENUMS(LEVEL_PARAMS, OPERATOR_COUNT),
        EDIT_PARAM,
        PARAMS_LEN
    };

    enum InputId { VOCT_INPUT, INPUTS_LEN };

    enum OutputId { AUDIO_OUTPUT, OUTPUTS_LEN };
    enum LightId { ENUMS(SELECT_LIGHTS, OPERATOR_COUNT), ENUMS(CONNECTION_LIGHTS, CONNECTION_COUNT), LIGHTS_LEN };

    struct FMOperator {
        void generate(float freq, float level, float sampleTime, float multiplier, float modulation) {
            // Calculate phase increment
            float phaseIncrement = multiplier * freq * sampleTime + modulation;

            // Update the phase with the new increment
            phase += phaseIncrement;

            // Wrap phase to stay within [0, 1) using fmod for better precision
            phase = std::fmod(phase, 1.f);
            if (phase < 0.f) {
                phase += 1.f;
            }

            // Generate output
            out = level * std::sin(2.f * M_PI * phase);
        }

        float phase = 0.f;
        float out = 0.f;
    };

    PostHumanFM() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configInput(VOCT_INPUT, "V/Oct");
        configOutput(AUDIO_OUTPUT, "Audio");
        configButton(EDIT_PARAM, "Edit");

        for (int op = 0; op < OPERATOR_COUNT; ++op) {
            configButton(SELECT_PARAMS + op, "Operator " + std::to_string(op + 1));
            configParam(MULT_PARAMS + op, -3, 3, 0.f, "Multiplier", "x", 2.f);
            getParamQuantity(MULT_PARAMS + op)->snapEnabled = true;
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

    void processSelect() {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            auto& trigger = operatorTriggers[i];
            int selectTriggered = trigger.process(getParam(SELECT_PARAMS + i).getValue());
            if (selectTriggered) {
                currentOperator = i;
            }

            auto& operatorLight = getLight(SELECT_LIGHTS + i);
            operatorLight.setBrightness(currentOperator == i);
        }
    }

    void processEdit() {
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            auto& trigger = operatorTriggers[i];
            int selectTriggered = trigger.process(getParam(SELECT_PARAMS + i).getValue());

            if (selectTriggered) {
                if (selectedOperator > -1) {
                    algorithmGraph.addEdge(selectedOperator, i);
                    getLight(CONNECTION_LIGHTS + selectedOperator).setBrightness(1.f);

                    printf("Added edge %d -> %d\n", selectedOperator, i);
                    selectedOperator = -1;
                    topologicalOrder = algorithmGraph.topologicalSort();

                } else {
                    selectedOperator = i;
                }

                printf("Selected operator %d\n", i);
            }
        }
    }

    void process(const ProcessArgs& args) override {
        float voct = inputs[VOCT_INPUT].getVoltage();
        float freq = dsp::FREQ_C4 * std::pow(2.f, voct);

        for (int conn = 0; conn < CONNECTION_COUNT; ++conn) {

            if (algorithmGraph.hasEdge(conn % OPERATOR_COUNT, conn / OPERATOR_COUNT)) {
            }
        }

        if (getParam(EDIT_PARAM).getValue()) {
            processEdit();
        } else {
            processSelect();
        }

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
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            int op = topologicalOrder[i];

            auto& multParam = getParam(MULT_PARAMS + op);
            auto& levelParam = getParam(LEVEL_PARAMS + op);

            auto level = levelParam.getValue();

            float mult = std::pow(2.f, multParam.getValue());

            const auto& modulators = algorithmGraph.getAdjacentVerticesRev(op);

            float modulation = 0.f;
            for (const auto& modulator : modulators) {
                modulation += operators[modulator].out;
            }

            operators[op].generate(freq, level, mult, args.sampleTime, modulation);
        }

        getOutput(AUDIO_OUTPUT).setVoltage(5 * operators[5].out);
    }

    int currentOperator = 0;

    std::array<FMOperator, OPERATOR_COUNT> operators; // todo: DAG
    std::array<dsp::BooleanTrigger, OPERATOR_COUNT> operatorTriggers;
    std::vector<int> topologicalOrder = {0, 1, 2, 3, 4, 5};
    DirectedGraph<int> algorithmGraph{OPERATOR_COUNT};

    int selectedOperator = -1;
};

// TODO: Organise
struct PostHumanFMWidget : ModuleWidget {
    PostHumanFMWidget(PostHumanFM* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/PostHumanFM.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // addChild(
        //     createParamCentered<RoundHugeBlackKnob>(Vec(box.size.x / 2, 250), module, PostHumanFM::OPERATOR_PARAM));

        // addChild(createParamCentered<VCVButton>(Vec(box.size.x / 2, 250), module, PostHumanFM::SELECT_PARAM));

        // auto* display = createWidget<Display>(mm2px(Vec(4.963, 13.049)));
        // display->module = module;
        // addChild(display);

        addChild(createParamCentered<VCVLatch>(Vec(64, 64), module, PostHumanFM::EDIT_PARAM));

        if (!module)
            return;
        currentOperator = module->currentOperator;

        const int radius = 64;
        std::vector<Vec> positions;

        int x = box.size.x / 2;
        int y = 96;

        for (int op = 0; op < PostHumanFM::OPERATOR_COUNT; ++op) {
            // TODO: clean up and move to helper function

            double angle = M_PI_2 + op * 2 * M_PI / PostHumanFM::OPERATOR_COUNT;
            int opX = std::cos(angle) * radius;
            int opY = std::sin(angle) * radius;
            positions.emplace_back(Vec(x + opX, y + opY));
            addParam(createLightParamCentered<VCVLightBezel<MediumSimpleLight<GreenLight>>>(
                Vec(x + opX, y + opY), module, PostHumanFM::SELECT_PARAMS + op, PostHumanFM::SELECT_LIGHTS + op));

            auto* multParam = createParamCentered<RoundBlackKnob>(Vec(50, 200), module, PostHumanFM::MULT_PARAMS + op);
            auto* levelParam =
                createParamCentered<RoundBlackKnob>(Vec(50, 300), module, PostHumanFM::LEVEL_PARAMS + op);

            addParam(multParam);
            addParam(levelParam);

            if (op != currentOperator) {
                multParam->hide();
                levelParam->hide();
            }
        }

        addChild(createInputCentered<PJ301MPort>(Vec(100, 200), module, PostHumanFM::VOCT_INPUT));
        addChild(createOutputCentered<PJ301MPort>(Vec(100, 300), module, PostHumanFM::AUDIO_OUTPUT));

        for (size_t i = 0; i < positions.size(); ++i) {
            for (size_t j = i + 1; j < positions.size(); ++j) {
                addChild(
                    createConnectionLight(positions[i], positions[j], module, PostHumanFM::CONNECTION_LIGHTS + i * j));
            }
        }
    }

    void setKnobOperator(int newOperator) {
        if (!module)
            return;

        // todo: loop through both knobs

        for (int param = PostHumanFM::MULT_PARAMS; param < PostHumanFM::MULT_PARAMS_LAST; ++param) {
            auto* oldKnob = getParam(PostHumanFM::MULT_PARAMS + currentOperator);
            oldKnob->hide();

            auto* newKnob = getParam(PostHumanFM::MULT_PARAMS + newOperator);
            newKnob->show();
        }

        for (int param = PostHumanFM::LEVEL_PARAMS; param < PostHumanFM::LEVEL_PARAMS_LAST; ++param) {
            auto* oldKnob = getParam(PostHumanFM::LEVEL_PARAMS + currentOperator);
            oldKnob->hide();

            auto* newKnob = getParam(PostHumanFM::LEVEL_PARAMS + newOperator);
            newKnob->show();
        }
    }

    void step() override {
        if (!module)
            return;

        auto* phModule = dynamic_cast<PostHumanFM*>(module);

        auto moduleCurrentOperator = static_cast<PostHumanFM*>(module)->currentOperator;

        if (moduleCurrentOperator != currentOperator) {
            setKnobOperator(moduleCurrentOperator);
            currentOperator = moduleCurrentOperator;
        }

        ModuleWidget::step();
    }

    int currentOperator;
};

} // namespace ph

Model* modelPostHumanFM = createModel<ph::PostHumanFM, ph::PostHumanFMWidget>("PostHumanFM");