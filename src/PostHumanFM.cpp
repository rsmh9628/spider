#include "FMOperator.hpp"
#include "plugin.hpp"

constexpr int OPERATOR_COUNT = 4;

namespace ph {

struct PostHumanFM : Module {
    enum ParamId {
        OP1_PARAM = 0,
        OP2_PARAM = 4,
        OP3_PARAM = 8,
        OP4_PARAM = 12,
        PARAMS_LEN = 16,
    };
    enum InputId { OP1_LEVEL_INPUT, OP2_LEVEL_INPUT, INPUT_INPUT, INPUTS_LEN };
    enum OutputId { AUDIO_OUTPUT, OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };

    PostHumanFM()
        : op1(this, 0)
        , op2(this, 1)
        , op3(this, 2)
        , op4(this, 3) {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        for (int i = 0; i < OPERATOR_COUNT; i++) {
            configOperatorParams(i);
        }

        // configInput(OP1_LEVEL_INPUT, "");
        // configInput(OP2_LEVEL_INPUT, "");
        configInput(INPUT_INPUT, "");
        configOutput(AUDIO_OUTPUT, "");
    }

    void process(const ProcessArgs& args) override {
        float voct = inputs[INPUT_INPUT].getVoltage();
        float freq = dsp::FREQ_C4 * std::pow(2.f, voct);

        op2.generate(args, freq);

        op1.modulate(args, op2, freq);

        // params[OP1_LEVEL_PARAM].getValue();

        // getParam(OP1_LEVEL_PARAM).getValue();

        getOutput(AUDIO_OUTPUT).setVoltage(op1.out());
    }

    void configOperatorParams(unsigned char op) {
        configParam(getOperatorParamId(op, FMOperator::PARAM_RATIO), 0.01f,
                    10.f, 0.f, "Ratio");
        configParam(getOperatorParamId(op, FMOperator::PARAM_DEPTH), 0.f, 1.f,
                    0.f, "Depth");
        configParam(getOperatorParamId(op, FMOperator::PARAM_FEEDBACK), 0.f,
                    1.f, 0.f, "Feedback");
        configParam(getOperatorParamId(op, FMOperator::PARAM_LEVEL), 0.f, 1.f,
                    0.f, "Level");
    }

    static int getOperatorParamId(unsigned char op,
                                  FMOperator::ParamType type) {
        return op * FMOperator::PARAM_COUNT + type;
    }

    FMOperator op1; // todo: make list
    FMOperator op2; // todo: make list
    FMOperator op3; // todo: make list
    FMOperator op4; // todo: make list
};

struct PostHumanFMWidget : ModuleWidget {
    PostHumanFMWidget(PostHumanFM* module) {
        setModule(module);
        setPanel(
            createPanel(asset::plugin(pluginInstance, "res/PostHumanFM.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(
            Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(
            Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(
            createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH,
                                          RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        for (int i = 0; i < OPERATOR_COUNT; ++i) {
            addParam(createParamCentered<RoundBlackKnob>(
                mm2px(Vec(15.028 + 30 * i, 30.374)), module,
                PostHumanFM::getOperatorParamId(
                    i,
                    FMOperator::PARAM_RATIO))); // TODO: 15*i

            addParam(createParamCentered<RoundBlackKnob>(
                mm2px(Vec(15.028 + 30 * i, 41.91)), module,
                PostHumanFM::getOperatorParamId(
                    i,
                    FMOperator::PARAM_DEPTH))); // TODO: 15*i

            addParam(createParamCentered<RoundBlackKnob>(
                mm2px(Vec(15.028 + 30 * i, 54.23)), module,
                PostHumanFM::getOperatorParamId(
                    i,
                    FMOperator::PARAM_FEEDBACK))); // TODO: 15*i

            addParam(createParamCentered<RoundBlackKnob>(
                mm2px(Vec(15.028 + 30 * i, 67.813)), module,
                PostHumanFM::getOperatorParamId(
                    i,
                    FMOperator::PARAM_LEVEL))); // TODO: 15*i
        }

        // addParam(createParamCentered<RoundBlackKnob>(
        //     mm2px(Vec(15.028, 30.374)), module,
        //     PostHumanFM::getOperatorParamId(0, PostHumanFM::RATIO)););
        // addParam(createParamCentered<RoundBlackKnob>(
        //     mm2px(Vec(35.518, 30.374)), module,
        //     PostHumanFM::OP2_RATIO_PARAM));
        // addParam(createParamCentered<RoundBlackKnob>(
        //     mm2px(Vec(15.14, 41.91)), module, PostHumanFM::OP1_DEPTH_PARAM));
        // addParam(createParamCentered<RoundBlackKnob>(
        //     mm2px(Vec(35.63, 41.91)), module, PostHumanFM::OP2_DEPTH_PARAM));
        // addParam(createParamCentered<RoundBlackKnob>(
        //     mm2px(Vec(15.14, 54.231)), module,
        //     PostHumanFM::OP1_FEEDBACK_PARAM));
        // addParam(createParamCentered<RoundBlackKnob>(
        //     mm2px(Vec(35.63, 54.231)), module,
        //     PostHumanFM::OP2_FEEDBACK_PARAM));
        // addParam(createParamCentered<RoundBlackKnob>(
        //     mm2px(Vec(15.14, 67.813)), module,
        //     PostHumanFM::OP1_LEVEL_PARAM));
        // addParam(createParamCentered<RoundBlackKnob>(
        //     mm2px(Vec(35.63, 67.813)), module,
        //     PostHumanFM::OP2_LEVEL_PARAM));

        addInput(createInputCentered<PJ301MPort>(
            mm2px(Vec(15.14, 77.47)), module, PostHumanFM::OP1_LEVEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            mm2px(Vec(35.63, 77.47)), module, PostHumanFM::OP2_LEVEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            mm2px(Vec(58.32, 104.14)), module, PostHumanFM::INPUT_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(
            mm2px(Vec(81.411, 104.14)), module, PostHumanFM::AUDIO_OUTPUT));
    }
};

} // namespace ph

Model* modelPostHumanFM =
    createModel<ph::PostHumanFM, ph::PostHumanFMWidget>("PostHumanFM");