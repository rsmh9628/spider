#include "plugin.hpp"
#include "DirectedGraph.hpp"

#include <array>

namespace ph {

struct PostHumanFM : Module {

    enum OperatorParams : int { OP_PARAM_MULT, OP_PARAM_LEVEL, OP_PARAM_COUNT };

    static constexpr int OPERATOR_COUNT = 6;
    static constexpr int OPERATOR_PARAMS_LEN = OPERATOR_COUNT * OP_PARAM_COUNT;

    enum ParamId { OPERATOR_PARAM = OPERATOR_PARAMS_LEN, SELECT_PARAM, PARAMS_LEN };

    enum InputId { VOCT_INPUT, INPUTS_LEN };

    enum OutputId { AUDIO_OUTPUT, OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };

    struct FMOperator {
        void generate(float freq, float level, float sampleTime, float modulation) {
            phase += freq * sampleTime + modulation;

            if (phase >= 1.f) {
                phase -= 1.f;
            }

            out = level * std::sin(2.f * M_PI * phase);
        }

        float phase = 0.f;
        float out = 0.f;
    };

    static constexpr int opParamId(int operatorIndex, int param) { return operatorIndex * OP_PARAM_COUNT + param; }

    PostHumanFM() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configInput(VOCT_INPUT, "V/Oct");
        configOutput(AUDIO_OUTPUT, "Audio");

        for (int op = 0; op < OPERATOR_COUNT; ++op) {
            configParam(opParamId(op, OP_PARAM_MULT), -3, 3, 0.f, "Multiplier", "x", 2.f);
            getParamQuantity(opParamId(op, OP_PARAM_MULT))->snapEnabled = true;
            configParam(opParamId(op, OP_PARAM_LEVEL), 0.f, 1.f, 0.f, "Level");
        }

        configParam(OPERATOR_PARAM, 0.f, OPERATOR_COUNT - 1, 0.f, "Operator", "", 0.f, 1.f, 1.f);
        getParamQuantity(OPERATOR_PARAM)->snapEnabled = true;

        configButton(SELECT_PARAM, "Select");

        // for (int i = 0; i < OPERATOR_COUNT - 1; i++) {
        //     algorithmGraph.addEdge(i, i + 1);
        // }
        // topologicalOrder = algorithmGraph.topologicalSort();
    }

    void process(const ProcessArgs& args) override {
        float voct = inputs[VOCT_INPUT].getVoltage();
        float freq = dsp::FREQ_C4 * std::pow(2.f, voct);

        bool selectTriggered = selectTrigger.process(getParam(SELECT_PARAM).getValue());

        if (selectTriggered) {
            int newOperator = getParam(OPERATOR_PARAM).getValue();

            if (selectedOperator > -1) {
                algorithmGraph.addEdge(selectedOperator, newOperator);
                printf("Added edge %d -> %d\n", selectedOperator, newOperator);
                selectedOperator = -1;
                topologicalOrder = algorithmGraph.topologicalSort();
            } else {
                selectedOperator = newOperator;
            }

            printf("Selected operator %d\n", newOperator);
        }
        // todo: this can eventually be parallelised with SIMD once independent carriers can be found from the graph
        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            int op = topologicalOrder[i];

            auto& multParam = getParam(opParamId(op, OP_PARAM_MULT));
            auto& levelParam = getParam(opParamId(op, OP_PARAM_LEVEL));

            auto level = levelParam.getValue();

            float opFreq = freq * std::pow(2.f, multParam.getValue());

            const auto& modulators = algorithmGraph.getAdjacentVerticesRev(op);

            float modulation = 0.f;
            for (const auto& modulator : modulators) {
                modulation += operators[modulator].out;
            }

            operators[op].generate(opFreq, level, args.sampleTime, modulation);
        }

        getOutput(AUDIO_OUTPUT).setVoltage(5 * operators[5].out);
    }

    std::array<FMOperator, OPERATOR_COUNT> operators; // todo: DAG
    std::vector<int> topologicalOrder = {0, 1, 2, 3, 4, 5};
    DirectedGraph<int> algorithmGraph{OPERATOR_COUNT};

    dsp::BooleanTrigger selectTrigger;
    int selectedOperator = -1;
};

struct Display : LedDisplay {
    Display() { box.size = mm2px(Vec(51.034, 42.175)); }

    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer != 1)
            return;

        // drawGrid(args);
        drawNodes(args, box.size.x / 2, box.size.y / 2);
    }

    void drawGrid(const DrawArgs& args) {
        int size = box.size.x / 6;
        for (int i = 0; i < 6; ++i) {
            for (int j = 0; j < 5; ++j) {
                nvgBeginPath(args.vg);
                nvgRect(args.vg, i * size, j * size, size, size);
                nvgStrokeColor(args.vg, nvgRGB(0x00, 0x00, 0xdd));
                nvgFillColor(args.vg, nvgRGB(0x00, 0x00, 0x00));
                nvgFill(args.vg);
                nvgStroke(args.vg);
            }
        }
    }

    void drawOperator(const DrawArgs& args, int x, int y, int op) {
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, x, y, 8);
        const auto& currentOperator = module->getParam(PostHumanFM::OPERATOR_PARAM).getValue();

        nvgStrokeColor(args.vg, currentOperator == op ? selectColour : normalColour);
        nvgFillColor(args.vg, nvgRGB(0x00, 0x00, 0x00));
        nvgFill(args.vg);
        nvgStroke(args.vg);

        nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
        nvgFontSize(args.vg, 12);
        nvgText(args.vg, x - 4, y + 4, std::to_string(op + 1).c_str(), NULL);
    }

    void drawNodes(const DrawArgs& args, int x, int y) {
        const int radius = 40;

        std::vector<std::pair<int, int>> positions;

        // Calculate positions of all nodes
        for (int i = 0; i < PostHumanFM::OPERATOR_COUNT; ++i) {
            double angle = i * 2 * M_PI / PostHumanFM::OPERATOR_COUNT;
            int opX = std::cos(angle) * radius;
            int opY = std::sin(angle) * radius;
            positions.emplace_back(x + opX, y + opY);
        }

        nvgStrokeColor(args.vg, nvgRGB(0x40, 0x40, 0x40));

        const auto& algoGraph = module->algorithmGraph;

        // Draw lines between each pair of nodes
        for (size_t i = 0; i < positions.size(); ++i) {
            for (size_t j = i + 1; j < positions.size(); ++j) {
                // First line

                if (algoGraph.hasEdge(i, j)) {
                    nvgStrokeColor(args.vg, nvgRGB(0x00, 0xff, 0x00));
                } else {
                    nvgStrokeColor(args.vg, nvgRGB(0x40, 0x40, 0x40));
                }

                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, positions[i].first, positions[i].second);
                nvgLineTo(args.vg, positions[j].first, positions[j].second);
                nvgStroke(args.vg);
            }
        }

        // Draw nodes

        for (size_t i = 0; i < positions.size(); ++i) {
            drawOperator(args, positions[i].first, positions[i].second, (i + 4) % 6);
        }
    }

    PostHumanFM* module;

    const NVGcolor selectColour = nvgRGB(0x00, 0xff, 0x00);
    const NVGcolor normalColour = nvgRGB(0xff, 0xff, 0xff);
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

        addChild(
            createParamCentered<RoundHugeBlackKnob>(Vec(box.size.x / 2, 250), module, PostHumanFM::OPERATOR_PARAM));

        addChild(createParamCentered<VCVButton>(Vec(box.size.x / 2, 250), module, PostHumanFM::SELECT_PARAM));

        auto* display = createWidget<Display>(mm2px(Vec(4.963, 13.049)));
        display->module = module;
        addChild(display);

        if (!module)
            return;

        currentOperator = module->getParam(PostHumanFM::OPERATOR_PARAM).getValue();

        for (int op = 0; op < PostHumanFM::OPERATOR_COUNT; ++op) {
            auto* multParam = createParamCentered<RoundBlackKnob>(
                Vec(50, 200), module, PostHumanFM::opParamId(op, PostHumanFM::OP_PARAM_MULT));
            auto* levelParam = createParamCentered<RoundBlackKnob>(
                Vec(50, 300), module, PostHumanFM::opParamId(op, PostHumanFM::OP_PARAM_LEVEL));

            addParam(multParam);
            addParam(levelParam);

            if (op != currentOperator) {
                multParam->hide();
                levelParam->hide();
            }
        }

        addChild(createInputCentered<PJ301MPort>(Vec(100, 200), module, PostHumanFM::VOCT_INPUT));
        addChild(createOutputCentered<PJ301MPort>(Vec(100, 300), module, PostHumanFM::AUDIO_OUTPUT));
    }

    void setKnobOperator(int newOperator) {
        if (!module)
            return;

        // todo: loop through both knobs

        for (int param = 0; param < PostHumanFM::OP_PARAM_COUNT; ++param) {
            auto* oldKnob = getParam(PostHumanFM::opParamId(currentOperator, param));
            oldKnob->hide();

            auto* newKnob = getParam(PostHumanFM::opParamId(newOperator, param));
            newKnob->show();
        }
    }

    void step() override {
        if (!module)
            return;

        auto* phModule = dynamic_cast<PostHumanFM*>(module);

        auto moduleCurrentOperator =
            static_cast<PostHumanFM*>(module)->getParam(PostHumanFM::OPERATOR_PARAM).getValue();

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