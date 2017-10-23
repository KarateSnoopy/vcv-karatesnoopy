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
        PITCH_PARAM,
        NUM_PARAMS = PITCH_PARAM + MAX_STEPS
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
    bool m_running = true;
    ButtonWithLight m_pitchEditButton;
    ButtonWithLight m_gateEditButton;
    ButtonWithLight m_runningButton;
    ButtonWithLight m_resetButton;
    SchmittTrigger m_clockTrigger; // for external clock
    PulseGenerator m_gatePulse;

    float m_phase = 0.0;
    int m_currentPattern = 0;
    int m_currentPatternIndex = 0;
    int m_currentStepIndex = 0;
    int m_lastStepIndex = 0;
    bool m_isPitchOn[MAX_STEPS] = {0};
    float m_stepLights[MAX_STEPS] = {};
    GateMode m_gateMode = TRIGGER;
    std::vector<Widget *> m_editPitchUI;
    std::vector<std::vector<int>> m_patterns;

    float m_cvLight = 0.0f;
    float m_gateXLight = 0.0f;
    float m_gateYLight = 0.0f;
    float m_gateLights[MAX_STEPS] = {};

    SEQ();
    void step();
    bool ProcessClockAndReset();
    void ShowEditPitchUI(bool showUI);
    void ProcessUIButtons();
    void AdvanceStep();
    void ProcessXYTriggers();
    void FadeGateLights();

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
