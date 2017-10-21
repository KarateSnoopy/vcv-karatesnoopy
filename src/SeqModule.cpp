#include "Snoopy.hpp"


struct MyModule : Module {
	enum ParamIds {
		PITCH_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SINE_OUTPUT,
		NUM_OUTPUTS
	};

	float phase = 0.0;

	MyModule() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
	void step();
	//void step() override;
    
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void MyModule::step() {
	// Implement a simple sine oscillator

	// Compute the frequency from the pitch parameter and input
	float pitch = params[PITCH_PARAM].value;
	pitch += inputs[PITCH_INPUT].value;
	pitch = clampf(pitch, -4.0, 4.0);
	float freq = 440.0 * powf(2.0, pitch);

	// Accumulate the phase
	phase += freq / gSampleRate; // engineGetSampleRate();
	if (phase >= 1.0)
		phase -= 1.0;

	// Compute the sine output
	float sine = sinf(2 * M_PI * phase);
	outputs[SINE_OUTPUT].value = 5.0 * sine;
}


MyModuleWidget::MyModuleWidget() {
	MyModule *module = new MyModule();
	setModule(module);
	box.size = Vec(6 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/MyModule.svg")));
		addChild(panel);
	}

	addChild(createScrew<ScrewSilver>(Vec(15, 0)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 0)));
	addChild(createScrew<ScrewSilver>(Vec(15, 365)));
	addChild(createScrew<ScrewSilver>(Vec(box.size.x - 30, 365)));

	addParam(createParam<Davies1900hBlackKnob>(Vec(28, 87), module, MyModule::PITCH_PARAM, -3.0, 3.0, 0.0));

	addInput(createInput<PJ301MPort>(Vec(33, 186), module, MyModule::PITCH_INPUT));

	addOutput(createOutput<PJ301MPort>(Vec(33, 275), module, MyModule::SINE_OUTPUT));
}
