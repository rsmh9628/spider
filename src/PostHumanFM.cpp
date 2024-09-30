#include "plugin.hpp"

#include <array>

namespace ph {

struct PostHumanFM : Module {
    enum OperatorParamId {
        LEVEL_PARAM,
        MULT_PARAM,
        CV_LEVEL_PARAM,
        CV_MULT_PARAM,
        ALGORITHM_CONTROL_PARAM,
        OPERATOR_PARAMS_LEN
    };

    enum OperatorInputId { LEVEL_INPUT, MULT_INPUT, OPERATOR_INPUTS_LEN };

    static constexpr int OPERATOR_LEN = 4;
    static constexpr int PARAMS_LEN = OPERATOR_LEN * OPERATOR_PARAMS_LEN;

    enum InputId { VOCT_INPUT = OPERATOR_INPUTS_LEN };

    static constexpr int INPUTS_LEN = OPERATOR_LEN * OPERATOR_INPUTS_LEN + 1;

    enum OutputId { AUDIO_OUTPUT, OUTPUTS_LEN };
    enum LightId { OP1_LIGHT, OP2_LIGHT, OP3_LIGHT, OP4_LIGHT, LIGHTS_LEN };

    struct FMOperator {
        void generate(float freq, float sampleTime, float modulation) {
            freq *= mult;

            phase += freq * sampleTime + modulation;

            if (phase >= 1.f) {
                phase -= 1.f;
            }

            out = level * std::sin(2.f * M_PI * phase);
        }

        float level = 1.f;
        float mult = 1.f;
        float phase = 0.f;
        float out = 0.f;
    };

    PostHumanFM() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configOperators();

        configInput(VOCT_INPUT, "V/Oct");
        configOutput(AUDIO_OUTPUT, "Audio");
    }

    void process(const ProcessArgs& args) override {
        float voct = inputs[VOCT_INPUT].getVoltage();
        float freq = dsp::FREQ_C4 * std::pow(2.f, voct);

        for (int i = 0; i < OPERATOR_LEN; i++) {
            // auto& multInput = getInput(multInputId(i));
            // auto& multParam = getParam(multParamId(i));
            //
            // auto& levelInput = getInput(levelInputId(i));
            // auto& levelParam = getParam(levelParamId(i));
            //
            // operators[i].mult = multInput.isConnected()
            //                        ? multInput.getVoltage() * 0.1
            //                        : multParam.getValue();
            //
            // operators[i].level =
            //    levelInput.isConnected()
            //        ? levelInput.getVoltage() * 0.1 // TODO: normalise
            //        : levelParam.getValue();
            //
            // operators[i].generate(freq, args.sampleTime,
            //                      0.f); // todo: modulation
        }

        getOutput(AUDIO_OUTPUT).setVoltage(5 * operators[0].out);
    }

    std::array<FMOperator, OPERATOR_LEN> operators; // todo: DAG

    static int opParamId(int op, OperatorParamId param) {
        return op * OPERATOR_PARAMS_LEN + param;
    }

    static int opInputId(int op, OperatorInputId input) {
        return op * OPERATOR_INPUTS_LEN + input;
    }

private:
    void configOperators() {
        for (int i = 0; i < OPERATOR_LEN; i++) {
            std::string suffix = " (Operator " + std::to_string(i + 1) + ")";
            configParam(opParamId(i, LEVEL_PARAM), 0.f, 1.f, 0.f,
                        "Level" + suffix);
            configParam(opParamId(i, MULT_PARAM), 0.f, 1.f, 0.f,
                        "Multiplier" + suffix);
            configParam(opParamId(i, CV_LEVEL_PARAM), 0.f, 1.f, 0.f,
                        "Level CV" + suffix);
            configParam(opParamId(i, CV_MULT_PARAM), 0.f, 1.f, 0.f,
                        "Multiplier CV" + suffix);
            configInput(opInputId(i, LEVEL_INPUT), "Level input" + suffix);
            configInput(opInputId(i, MULT_INPUT), "Multiplier input" + suffix);
        }
    }
};

// TODO: Organise
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

        addOperator(Vec(3.990f, 13.928f), 0);
        addOperator(Vec(72.110f, 13.928f), 1);
        addOperator(Vec(3.990f, 66.042f), 2);
        addOperator(Vec(72.110f, 66.042f), 3);

        addAlgorithmControls(Vec(36.862, 55.539));
    }

    void addAlgorithmControls(const Vec& pos) {
        addParam(createLightParam<VCVLightBezel<WhiteLight>>(
            mm2px(pos + Vec(0, 0)), module,
            PostHumanFM::opParamId(0, PostHumanFM::ALGORITHM_CONTROL_PARAM),
            PostHumanFM::OP1_LIGHT));

        addParam(createLightParam<VCVLightBezel<WhiteLight>>(
            mm2px(pos + Vec(18.563, 0)), module,
            PostHumanFM::opParamId(1, PostHumanFM::ALGORITHM_CONTROL_PARAM),
            PostHumanFM::OP1_LIGHT));

        addParam(createLightParam<VCVLightBezel<WhiteLight>>(
            mm2px(pos + Vec(0, 18.563)), module,
            PostHumanFM::opParamId(2, PostHumanFM::ALGORITHM_CONTROL_PARAM),
            PostHumanFM::OP1_LIGHT));

        addParam(createLightParam<VCVLightBezel<WhiteLight>>(
            mm2px(pos + Vec(18.563, 18.563)), module,
            PostHumanFM::opParamId(3, PostHumanFM::ALGORITHM_CONTROL_PARAM),
            PostHumanFM::OP1_LIGHT));
    }

    void addOperator(const Vec& pos, int op) {
        // TODO: these positions are not final

        addParam(createParam<RoundBlackKnob>(
            mm2px(pos + Vec(1.641f, 3.757f)), module,
            PostHumanFM::opParamId(op, PostHumanFM::LEVEL_PARAM)));

        addParam(createParam<RoundBlackKnob>(
            mm2px(pos + Vec(14.570F, 3.757f)), module,
            PostHumanFM::opParamId(op, PostHumanFM::MULT_PARAM)));

        addParam(createParam<Trimpot>(
            mm2px(pos + Vec(2.728f, 20.065f)), module,
            PostHumanFM::opParamId(op, PostHumanFM::CV_LEVEL_PARAM)));

        addParam(createParam<Trimpot>(
            mm2px(pos + Vec(14.570f, 20.065f)), module,
            PostHumanFM::opParamId(op, PostHumanFM::CV_MULT_PARAM)));

        addInput(createInput<PJ301MPort>(
            mm2px(pos + Vec(2.172f, 33.043f)), module,
            PostHumanFM::opInputId(op, PostHumanFM::MULT_INPUT)));

        addInput(createInput<PJ301MPort>(
            mm2px(pos + Vec(14.015f, 33.043f)), module,
            PostHumanFM::opInputId(op, PostHumanFM::LEVEL_INPUT)));
    }
};

} // namespace ph

Model* modelPostHumanFM =
    createModel<ph::PostHumanFM, ph::PostHumanFMWidget>("PostHumanFM");