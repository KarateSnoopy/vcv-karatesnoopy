#include "SeqModule.h"

SEQ::SEQ() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS)
{
}

void SEQ::InitUI(ModuleWidget *moduleWidget, Rect box)
{
    m_moduleWidget = moduleWidget;
    Module *module = this;

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/SeqModule.svg")));
        addChild(panel);
    }

    addParam(createParam<Davies1900hSmallBlackKnob>(Vec(18, 56), module, SEQ::CLOCK_PARAM, -2.0, 6.0, 2.0));
    addParam(createParam<Davies1900hSmallBlackSnapKnob>(Vec(132, 56), module, SEQ::STEPS_PARAM, 1.0, MAX_STEPS, MAX_STEPS));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(180, 65), &m_cvLight));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(219, 65), &m_gateXLight));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(257, 65), &m_gateYLight));

    m_runningButton.Init(m_moduleWidget, module, 60, 60, SEQ::RUN_PARAM);
    m_runningButton.SetOnOff(true, true);
    m_resetButton.Init(m_moduleWidget, module, 99, 60, SEQ::RESET_PARAM);
    m_resetButton.AddInput(SEQ::RESET_INPUT);

    int editButtonX = 50;
    int editButtonY = 150;
    m_pitchEditButton.Init(m_moduleWidget, module, editButtonX, editButtonY, SEQ::PITCH_EDIT_PARAM);
    m_pitchEditButton.SetOnOff(true, true);
    editButtonY += 20;
    m_gateEditButton.Init(m_moduleWidget, module, editButtonX, editButtonY, SEQ::GATE_EDIT_PARAM);
    m_gateEditButton.SetOnOff(true, false);

    static const float portX[8] = {20, 58, 96, 135, 173, 212, 250, 289};
    addInput(createInput<PJ301MPort>(Vec(portX[0] - 1, 98), module, SEQ::CLOCK_INPUT));
    addInput(createInput<PJ301MPort>(Vec(portX[1] - 1, 98), module, SEQ::EXT_CLOCK_INPUT));
    addInput(createInput<PJ301MPort>(Vec(portX[2] - 1, 98), module, SEQ::RESET_INPUT));
    addInput(createInput<PJ301MPort>(Vec(portX[3] - 1, 98), module, SEQ::STEPS_INPUT));

    addOutput(createOutput<PJ301MPort>(Vec(portX[4] - 1, 98), module, SEQ::CV_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(portX[5] - 1, 98), module, SEQ::GATE_X_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(portX[6] - 1, 98), module, SEQ::GATE_Y_OUTPUT));

    static const float btn_x[4] = {0, 38, 76, 115};
    static const float btn_y[4] = {0, 38, 76, 115};
    int iZ = 0;
    for (int iY = 0; iY < 4; iY++)
    {
        for (int iX = 0; iX < 4; iX++)
        {
            int x = btn_x[iX] + 100;
            int y = btn_y[iY] + 177;

            m_editPitchUI.push_back(addParam(createParam<RoundBlackKnob>(Vec(x, y), module, SEQ::PITCH_PARAM + iZ, 0.0, 6.0, 0.0)));
            m_editPitchUI.push_back(addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(x + 15, y + 15), &m_gateLights[iZ])));
            iZ++;
        }
    }
}

void SEQ::initialize()
{
    for (int i = 0; i < MAX_STEPS; i++)
    {
        m_isPitchOn[i] = true;
    }
}

void SEQ::randomize()
{
    for (int i = 0; i < MAX_STEPS; i++)
    {
        //m_isPitchOn[i] = (randomf() > 0.5);
    }
}

void SEQ::step()
{
    _frameCount++;

    if (m_runningButton.Process(params))
    {
        m_running = !m_running;
    }
    bool nextStep = false;
    if (m_running)
    {
        nextStep = ProcessClockAndReset();
    }
    ProcessUIButtons();
    if (nextStep)
    {
        AdvanceStep();
    }
    FadeGateLights();
    ProcessXYTriggers();

    // Pitch output
    float currentPitch = params[PITCH_PARAM + m_currentStepIndex].value;
    outputs[CV_OUTPUT].value = currentPitch;
    m_cvLight = currentPitch;
}

void SEQ::FadeGateLights()
{
    const float lightLambda = 0.075;
    for (int i = 0; i < MAX_STEPS; i++)
    {
        m_stepLights[i] -= m_stepLights[i] / lightLambda / gSampleRate;
        //m_gateLights[i] = m_isPitchOn[i] ? 1.0 - m_stepLights[i] : m_stepLights[i];
        m_gateLights[i] = m_stepLights[i];
    }
    m_gateXLight -= m_gateXLight / lightLambda / gSampleRate;
    m_gateYLight -= m_gateYLight / lightLambda / gSampleRate;
}

void SEQ::ProcessXYTriggers()
{
    bool pulse = m_gatePulse.process(1.0 / gSampleRate);

    // Rows
    int lastX = m_lastStepIndex % 4;
    int curX = m_currentStepIndex % 4;
    int lastY = m_lastStepIndex / 4;
    int curY = m_currentStepIndex / 4;

    // X row
    bool gateXChanged = (m_running && m_isPitchOn[m_currentStepIndex] && lastX != curX);
    if (m_gateMode == TRIGGER)
        gateXChanged = gateXChanged && pulse;
    else if (m_gateMode == RETRIGGER)
        gateXChanged = gateXChanged && !pulse;
    outputs[GATE_X_OUTPUT].value = gateXChanged ? 10.0 : 0.0;
    if (gateXChanged)
        m_gateXLight = 1.0;
    //write_log(0, "gateXChanged=%d m_running=%d m_isPitchOn[m_currentStepIndex]=%d lastX=%d curX=%d lastY=%d curY=%d\n", gateXChanged, m_running, m_isPitchOn[m_currentStepIndex], lastX, curX, lastY, curY);

    // Y row
    bool gateYChanged = (m_running && m_isPitchOn[m_currentStepIndex] && lastY != curY);
    if (m_gateMode == TRIGGER)
        gateYChanged = gateYChanged && pulse;
    else if (m_gateMode == RETRIGGER)
        gateYChanged = gateYChanged && !pulse;
    outputs[GATE_Y_OUTPUT].value = gateYChanged ? 10.0 : 0.0;
    if (gateYChanged)
        m_gateYLight = 1.0;
}

bool SEQ::ProcessClockAndReset()
{
    bool nextStep = false;
    if (inputs[EXT_CLOCK_INPUT].active)
    {
        // External clock
        if (m_clockTrigger.process(inputs[EXT_CLOCK_INPUT].value))
        {
            m_phase = 0.0;
            nextStep = true;
        }
    }
    else
    {
        // Internal clock
        float clockTime = powf(2.0, params[CLOCK_PARAM].value + inputs[CLOCK_INPUT].value);
        m_phase += clockTime / gSampleRate;
        if (m_phase >= 1.0)
        {
            m_phase -= 1.0;
            nextStep = true;
        }
    }

    if (m_resetButton.ProcessWithInput(params, inputs))
    {
        m_phase = 0.0;
        m_currentStepIndex = MAX_STEPS;
        nextStep = true;
    }

    return nextStep;
}

void SEQ::ProcessUIButtons()
{
    if (m_pitchEditButton.Process(params))
    {
        m_gateEditButton.SetOnOff(true, false);
        m_pitchEditButton.SetOnOff(true, true);
        ShowEditPitchUI(true);
    }

    if (m_gateEditButton.Process(params))
    {
        m_gateEditButton.SetOnOff(true, true);
        m_pitchEditButton.SetOnOff(true, false);
        ShowEditPitchUI(false);
    }
}

void SEQ::ShowEditPitchUI(bool showUI)
{
    for (auto &param : m_editPitchUI)
    {
        param->visible = showUI;
    }
}

void SEQ::AdvanceStep()
{
    m_lastStepIndex = m_currentStepIndex;
    int numSteps = clampi(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1, MAX_STEPS);
    m_currentStepIndex += 1;
    if (m_currentStepIndex >= numSteps)
    {
        m_currentStepIndex = 0;
    }

    m_stepLights[m_currentStepIndex] = 1.0;
    m_gatePulse.trigger(1e-3);
}

Widget *SEQ::addChild(Widget *widget)
{
    m_moduleWidget->addChild(widget);
    return widget;
}

ParamWidget *SEQ::addParam(ParamWidget *param)
{
    m_moduleWidget->addParam(param);
    return param;
}

Port *SEQ::addInput(Port *input)
{
    m_moduleWidget->addInput(input);
    return input;
}

Port *SEQ::addOutput(Port *output)
{
    m_moduleWidget->addOutput(output);
    return output;
}

json_t *SEQ::toJson()
{
    json_t *rootJ = json_object();

    // running
    json_object_set_new(rootJ, "running", json_boolean(m_running));

    // gates
    json_t *gatesJ = json_array();
    for (int i = 0; i < MAX_STEPS; i++)
    {
        json_t *gateJ = json_integer((int)m_isPitchOn[i]);
        json_array_append_new(gatesJ, gateJ);
    }
    json_object_set_new(rootJ, "gates", gatesJ);

    // gateMode
    json_t *gateModeJ = json_integer((int)m_gateMode);
    json_object_set_new(rootJ, "gateMode", gateModeJ);

    return rootJ;
}

void SEQ::fromJson(json_t *rootJ)
{
    // running
    json_t *runningJ = json_object_get(rootJ, "running");
    if (runningJ)
        m_running = json_is_true(runningJ);

    // gates
    json_t *gatesJ = json_object_get(rootJ, "gates");
    if (gatesJ)
    {
        for (int i = 0; i < MAX_STEPS; i++)
        {
            json_t *gateJ = json_array_get(gatesJ, i);
            if (gateJ)
                m_isPitchOn[i] = !!json_integer_value(gateJ);
        }
    }

    // gateMode
    json_t *gateModeJ = json_object_get(rootJ, "gateMode");
    if (gateModeJ)
        m_gateMode = (GateMode)json_integer_value(gateModeJ);
}
