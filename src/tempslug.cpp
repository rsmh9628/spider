#if 0
#    include "plugin.hpp"


struct PostHumanFM : Module {
	enum ParamId {
		OP2_LEVEL_PARAM,
		OP2_MULT_PARAM,
		OP1_LEVEL_PARAM,
		OP1_MULT_PARAM,
		OP2_CV_LEVEL_PARAM,
		OP2_CV_MULT_PARAM,
		OP1_CV_LEVEL_PARAM,
		OP1_CV_MULT_PARAM,
		OP4_LEVEL_PARAM,
		OP4_MULT_PARAM,
		OP3_LEVEL_PARAM,
		OP3_MULT_PARAM,
		OP4_CV_LEVEL_PARAM,
		OP4_CV_MULT_PARAM,
		OP3_CV_LEVEL_PARAM,
		OP3_CV_MULT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		OP2_LEVEL_INPUT,
		OP2_MULT_INPUT,
		OP1_LEVEL_INPUT,
		OP1_MULT_INPUT,
		OP4_LEVEL_INPUT,
		OP4_MULT_INPUT,
		OP3_LEVEL_INPUT,
		OP3_MULT_INPUT,
		VOCT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	PostHumanFM() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(OP2_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP2_MULT_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP1_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP1_MULT_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP2_CV_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP2_CV_MULT_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP1_CV_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP1_CV_MULT_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP4_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP4_MULT_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP3_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP3_MULT_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP4_CV_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP4_CV_MULT_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP3_CV_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OP3_CV_MULT_PARAM, 0.f, 1.f, 0.f, "");
		configInput(OP2_LEVEL_INPUT, "");
		configInput(OP2_MULT_INPUT, "");
		configInput(OP1_LEVEL_INPUT, "");
		configInput(OP1_MULT_INPUT, "");
		configInput(OP4_LEVEL_INPUT, "");
		configInput(OP4_MULT_INPUT, "");
		configInput(OP3_LEVEL_INPUT, "");
		configInput(OP3_MULT_INPUT, "");
		configInput(VOCT_INPUT, "");
		configOutput(AUDIO_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct PostHumanFMWidget : ModuleWidget {
	PostHumanFMWidget(PostHumanFM* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/PostHumanFM.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-89.854, 22.209)), module, PostHumanFM::OP2_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-78.012, 22.209)), module, PostHumanFM::OP2_MULT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-21.994, 22.209)), module, PostHumanFM::OP1_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-10.152, 22.209)), module, PostHumanFM::OP1_MULT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-89.854, 37.354)), module, PostHumanFM::OP2_CV_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-78.012, 37.354)), module, PostHumanFM::OP2_CV_MULT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-21.994, 37.354)), module, PostHumanFM::OP1_CV_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-10.152, 37.354)), module, PostHumanFM::OP1_CV_MULT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-90.351, 74.815)), module, PostHumanFM::OP4_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-78.509, 74.815)), module, PostHumanFM::OP4_MULT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-21.758, 74.815)), module, PostHumanFM::OP3_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-9.916, 74.815)), module, PostHumanFM::OP3_MULT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-90.351, 89.961)), module, PostHumanFM::OP4_CV_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-78.509, 89.961)), module, PostHumanFM::OP4_CV_MULT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-21.758, 89.961)), module, PostHumanFM::OP3_CV_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(-9.916, 89.961)), module, PostHumanFM::OP3_CV_MULT_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-89.854, 50.877)), module, PostHumanFM::OP2_LEVEL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-78.012, 50.877)), module, PostHumanFM::OP2_MULT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-21.994, 50.877)), module, PostHumanFM::OP1_LEVEL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-10.152, 50.877)), module, PostHumanFM::OP1_MULT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-90.351, 103.483)), module, PostHumanFM::OP4_LEVEL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-78.509, 103.483)), module, PostHumanFM::OP4_MULT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-21.758, 103.483)), module, PostHumanFM::OP3_LEVEL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-9.916, 103.483)), module, PostHumanFM::OP3_MULT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(-40.852, 109.256)), module, PostHumanFM::VOCT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(-59.946, 109.256)), module, PostHumanFM::AUDIO_OUTPUT));

		addChild(createWidgetCentered<Widget>(mm2px(Vec(40.852, 59.445))));
		addChild(createWidgetCentered<Widget>(mm2px(Vec(59.415, 59.445))));
		addChild(createWidgetCentered<Widget>(mm2px(Vec(40.852, 77.841))));
		addChild(createWidgetCentered<Widget>(mm2px(Vec(59.415, 77.841))));
	}


};


Model* modelPostHumanFM = createModel<PostHumanFM, PostHumanFMWidget>("PostHumanFM");
#endif