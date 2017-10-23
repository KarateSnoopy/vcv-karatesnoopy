#pragma once
#include "PluginInit.h"
#include "SEQWidget.h"
#include "ButtonWithLight.h"
#include "dsp/digital.hpp"
#include "utils.h"

#define MAX_STEPS 16

struct SEQ : Module
{
  public:
    enum ParamIds
    {
        CLOCK_PARAM,
        RUN_PARAM,
        RESET_PARAM,
        GATE_EDIT_PARAM,
        PITCH_EDIT_PARAM,
        STEPS_PARAM,
        ROW1_PARAM,
        NUM_PARAMS = ROW1_PARAM + MAX_STEPS
    };

    enum InputIds
    {
        CLOCK_INPUT,
        EXT_CLOCK_INPUT,
        RESET_INPUT,
        STEPS_INPUT,
        NUM_INPUTS
    };

    enum OutputIds
    {
        CV_OUTPUT,
        GATE_X_OUTPUT,
        GATE_Y_OUTPUT,
        NUM_OUTPUTS = GATE_Y_OUTPUT
    };

    enum GateMode
    {
        TRIGGER,
        RETRIGGER,
        CONTINUOUS,
    };

    ModuleWidget *m_moduleWidget;
    bool running = true;
    ButtonWithLight m_pitchEditButton;
    ButtonWithLight m_gateEditButton;
    ButtonWithLight m_runningButton;
    ButtonWithLight m_resetButton;
    SchmittTrigger clockTrigger; // for external clock
    SchmittTrigger runningTrigger;
    SchmittTrigger gateTriggers[MAX_STEPS];
    PulseGenerator gatePulse;

    float phase = 0.0;
    int index = 0;
    bool gateState[MAX_STEPS] = {0};
    float stepLights[MAX_STEPS] = {};
    GateMode gateMode = TRIGGER;
    std::vector<Widget *> m_editPitchUI;

    float cvLight = 0.0f;
    float gateXLight = 0.0f;
    float gateYLight = 0.0f;
    float gateLights[MAX_STEPS] = {};

    SEQ();
    void step();
    void InitUI(ModuleWidget *moduleWidget, Rect box);
    Widget *addChild(Widget *widget);
    ParamWidget *addParam(ParamWidget *param);
    Port *addInput(Port *input);
    Port *addOutput(Port *output);

    json_t *toJson();
    void fromJson(json_t *rootJ);
    void initialize();
    void randomize();
};
