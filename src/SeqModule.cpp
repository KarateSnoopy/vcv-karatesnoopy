#include "Snoopy.hpp"
#include "dsp/digital.hpp"

#define MAX_STEPS 16
static long _frameCount = 0;
void write_log(long freq, const char *format, ...)
{
    if (freq == 0)
        freq++;

    if (_frameCount % freq == 0)
    {
        va_list args;
        va_start(args, format);

        printf("%ld: ", _frameCount);
        vprintf(format, args);
        fflush(stdout);

        va_end(args);
    }
}

class ButtonWithLight
{
  private:
    float m_light = 0.0;
    int m_paramId = 0;
    int m_inputId = -1;
    SchmittTrigger m_trigger;
    bool m_onOffType = false;
    bool m_currentState = false;

  public:
    void Init(ModuleWidget *moduleWidget, Module *module, int x, int y, int paramId)
    {
        moduleWidget->addParam(createParam<LEDButton>(Vec(x, y), module, paramId, 0.0, 1.0, 0.0));
        moduleWidget->addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(x + 5, y + 5), &m_light));

        m_paramId = paramId;
    }

    void SetOnOff(bool onOff, bool currentState)
    {
        m_onOffType = onOff;
        m_currentState = currentState;
    }

    void AddInput(int inputId)
    {
        m_inputId = inputId;
    }

    bool GetState()
    {
        return m_currentState;
    }

    bool Process(std::vector<Param> &params)
    {
        return ProcessHelper(params[m_paramId].value);
    }

    bool ProcessWithInput(std::vector<Param> &params, std::vector<Input> &input)
    {
        return ProcessHelper(params[m_paramId].value + input[m_inputId].value);
    }

    bool ProcessHelper(float value)
    {
        bool returnValue = false;
        if (m_trigger.process(value))
        {
            m_light = 1.0;
            returnValue = true;
        }

        const float lightLambda = 0.075;

        if (m_onOffType)
        {
            if (returnValue)
            {
                m_currentState = !m_currentState;
            }
            m_light = m_currentState ? 1.0 : 0.0;
        }
        else
        {
            m_light -= m_light / lightLambda / gSampleRate;
        }
        return returnValue;
    }
};

struct SEQ : Module
{
  public:
    enum ParamIds
    {
        CLOCK_PARAM,
        RUN_PARAM,
        RESET_PARAM,
        GATE_EDIT_PARAM,
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
        GATES_OUTPUT,
        ROW1_OUTPUT,
        ROW2_OUTPUT,
        ROW3_OUTPUT,
        GATE_OUTPUT,
        NUM_OUTPUTS = GATE_OUTPUT
    };

    enum GateMode
    {
        TRIGGER,
        RETRIGGER,
        CONTINUOUS,
    };

    bool running = true;
    ButtonWithLight m_gateEditButton;
    ButtonWithLight m_runningButton;
    ButtonWithLight m_resetButton;
    SchmittTrigger clockTrigger; // for external clock
    SchmittTrigger runningTrigger;
    SchmittTrigger gateTriggers[MAX_STEPS];
    float phase = 0.0;
    int index = 0;
    bool gateState[MAX_STEPS] = {0};
    float stepLights[MAX_STEPS] = {};
    GateMode gateMode = TRIGGER;
    PulseGenerator gatePulse;

    float gatesLight = 0.0;
    float rowLights[3] = {};
    float gateLights[MAX_STEPS] = {};

    SEQ() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
    void step();

    json_t *toJson()
    {
        json_t *rootJ = json_object();

        // running
        json_object_set_new(rootJ, "running", json_boolean(running));

        // gates
        json_t *gatesJ = json_array();
        for (int i = 0; i < MAX_STEPS; i++)
        {
            json_t *gateJ = json_integer((int)gateState[i]);
            json_array_append_new(gatesJ, gateJ);
        }
        json_object_set_new(rootJ, "gates", gatesJ);

        // gateMode
        json_t *gateModeJ = json_integer((int)gateMode);
        json_object_set_new(rootJ, "gateMode", gateModeJ);

        return rootJ;
    }

    void fromJson(json_t *rootJ)
    {
        // running
        json_t *runningJ = json_object_get(rootJ, "running");
        if (runningJ)
            running = json_is_true(runningJ);

        // gates
        json_t *gatesJ = json_object_get(rootJ, "gates");
        if (gatesJ)
        {
            for (int i = 0; i < MAX_STEPS; i++)
            {
                json_t *gateJ = json_array_get(gatesJ, i);
                if (gateJ)
                    gateState[i] = !!json_integer_value(gateJ);
            }
        }

        // gateMode
        json_t *gateModeJ = json_object_get(rootJ, "gateMode");
        if (gateModeJ)
            gateMode = (GateMode)json_integer_value(gateModeJ);
    }

    void initialize()
    {
        for (int i = 0; i < MAX_STEPS; i++)
        {
            gateState[i] = false;
        }
    }

    void randomize()
    {
        for (int i = 0; i < MAX_STEPS; i++)
        {
            gateState[i] = (randomf() > 0.5);
        }
    }
};

void SEQ::step()
{
    const float lightLambda = 0.075;
    _frameCount++;
    bool nextStep = false;

    // Run
    if (m_runningButton.Process(params))
    {
        running = !running;
    }

    if (running)
    {
        if (inputs[EXT_CLOCK_INPUT].active)
        {
            // External clock
            if (clockTrigger.process(inputs[EXT_CLOCK_INPUT].value))
            {
                phase = 0.0;
                nextStep = true;
            }
        }
        else
        {
            // Internal clock
            float clockTime = powf(2.0, params[CLOCK_PARAM].value + inputs[CLOCK_INPUT].value);
            phase += clockTime / gSampleRate;
            if (phase >= 1.0)
            {
                phase -= 1.0;
                nextStep = true;
            }
        }
    }

    if (m_resetButton.ProcessWithInput(params, inputs))
    {
        phase = 0.0;
        index = MAX_STEPS;
        nextStep = true;
    }
    m_gateEditButton.Process(params);

    if (nextStep)
    {
        // Advance step
        int numSteps = clampi(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1, MAX_STEPS);
        index += 1;
        if (index >= numSteps)
        {
            index = 0;
        }
        stepLights[index] = 1.0;
        gatePulse.trigger(1e-3);
    }

    bool pulse = gatePulse.process(1.0 / gSampleRate);

    // Gate buttons
    for (int i = 0; i < MAX_STEPS; i++)
    {
        // if (gateTriggers[i].process(params[GATE_PARAM + i].value))
        // {
        //     //write_log(0, "v %d %f\n", i, params[GATE_PARAM + i].value);
        //     gateState[i] = !gateState[i];
        // }

        bool gateOn = (running && i == index && gateState[i]);
        if (gateMode == TRIGGER)
            gateOn = gateOn && pulse;
        else if (gateMode == RETRIGGER)
            gateOn = gateOn && !pulse;

        outputs[GATE_OUTPUT].value = gateOn ? 10.0 : 0.0; // TODO
        stepLights[i] -= stepLights[i] / lightLambda / gSampleRate;
        //gateLights[i] = gateState[i] ? 1.0 - stepLights[i] : stepLights[i];
        gateLights[i] = stepLights[i];

        //write_log(40000, "gateLights[%d]=%f\n", i, gateLights[i]);
    }

    //write_log(40000, "g trigger %d %d %d %d\n", gateState[0], gateState[1], gateState[2], gateState[3]);

    // Rows
    float row1 = params[ROW1_PARAM + index].value;
    float row2 = 0.0f; //params[ROW2_PARAM + index].value;
    float row3 = 0.0f; //params[ROW3_PARAM + index].value;
    bool gatesOn = (running && gateState[index]);
    if (gateMode == TRIGGER)
        gatesOn = gatesOn && pulse;
    else if (gateMode == RETRIGGER)
        gatesOn = gatesOn && !pulse;

    // Outputs
    outputs[ROW1_OUTPUT].value = row1;
    outputs[ROW2_OUTPUT].value = row2;
    outputs[ROW3_OUTPUT].value = row3;
    outputs[GATES_OUTPUT].value = gatesOn ? 10.0 : 0.0;
    gatesLight = gatesOn ? 1.0 : 0.0;
    rowLights[0] = row1;
    rowLights[1] = row2;
    rowLights[2] = row3;
}

SEQWidget::SEQWidget()
{
    SEQ *module = new SEQ();
    setModule(module);
    box.size = Vec(15 * 22, 380);

    {
        SVGPanel *panel = new SVGPanel();
        panel->box.size = box.size;
        panel->setBackground(SVG::load(assetPlugin(plugin, "res/SeqModule.svg")));
        addChild(panel);
    }

    addParam(createParam<Davies1900hSmallBlackKnob>(Vec(18, 56), module, SEQ::CLOCK_PARAM, -2.0, 6.0, 2.0));
    addParam(createParam<Davies1900hSmallBlackSnapKnob>(Vec(132, 56), module, SEQ::STEPS_PARAM, 1.0, MAX_STEPS, MAX_STEPS));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(180, 65), &module->gatesLight));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(219, 65), &module->rowLights[0]));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(257, 65), &module->rowLights[1]));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(296, 65), &module->rowLights[2]));

    module->m_runningButton.Init(this, module, 60, 60, SEQ::RUN_PARAM);
    module->m_runningButton.SetOnOff(true, true);
    module->m_resetButton.Init(this, module, 99, 60, SEQ::RESET_PARAM);
    module->m_resetButton.AddInput(SEQ::RESET_INPUT);
    module->m_gateEditButton.Init(this, module, 296, 150, SEQ::GATE_EDIT_PARAM);

    static const float portX[8] = {20, 58, 96, 135, 173, 212, 250, 289};
    addInput(createInput<PJ301MPort>(Vec(portX[0] - 1, 98), module, SEQ::CLOCK_INPUT));
    addInput(createInput<PJ301MPort>(Vec(portX[1] - 1, 98), module, SEQ::EXT_CLOCK_INPUT));
    addInput(createInput<PJ301MPort>(Vec(portX[2] - 1, 98), module, SEQ::RESET_INPUT));
    addInput(createInput<PJ301MPort>(Vec(portX[3] - 1, 98), module, SEQ::STEPS_INPUT));
    addOutput(createOutput<PJ301MPort>(Vec(portX[4] - 1, 98), module, SEQ::GATES_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(portX[5] - 1, 98), module, SEQ::ROW1_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(portX[6] - 1, 98), module, SEQ::ROW2_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(portX[7] - 1, 98), module, SEQ::ROW3_OUTPUT));

    static const float btn_x[4] = {0, 38, 76, 115};
    static const float btn_y[4] = {0, 38, 76, 115};
    int iZ = 0;
    for (int iY = 0; iY < 4; iY++)
    {
        for (int iX = 0; iX < 4; iX++)
        {
            int x = btn_x[iX] + 18;
            int y = btn_y[iY] + 177;
            addParam(createParam<RoundBlackKnob>(Vec(x, y), module, SEQ::ROW1_PARAM + iZ, 0.0, 6.0, 0.0));
            addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(x + 15, y + 15), &module->gateLights[iZ]));
            iZ++;
        }
    }
}

struct SEQGateModeItem : MenuItem
{
    SEQ *seq;
    SEQ::GateMode gateMode;
    void onAction()
    {
        seq->gateMode = gateMode;
    }
    void step()
    {
        rightText = (seq->gateMode == gateMode) ? "✔" : "";
    }
};

Menu *SEQWidget::createContextMenu()
{
    Menu *menu = ModuleWidget::createContextMenu();

    MenuLabel *spacerLabel = new MenuLabel();
    menu->pushChild(spacerLabel);

    SEQ *seq = dynamic_cast<SEQ *>(module);
    assert(seq);

    MenuLabel *modeLabel = new MenuLabel();
    modeLabel->text = "Gate Mode";
    menu->pushChild(modeLabel);

    SEQGateModeItem *triggerItem = new SEQGateModeItem();
    triggerItem->text = "Trigger";
    triggerItem->seq = seq;
    triggerItem->gateMode = SEQ::TRIGGER;
    menu->pushChild(triggerItem);

    SEQGateModeItem *retriggerItem = new SEQGateModeItem();
    retriggerItem->text = "Retrigger";
    retriggerItem->seq = seq;
    retriggerItem->gateMode = SEQ::RETRIGGER;
    menu->pushChild(retriggerItem);

    SEQGateModeItem *continuousItem = new SEQGateModeItem();
    continuousItem->text = "Continuous";
    continuousItem->seq = seq;
    continuousItem->gateMode = SEQ::CONTINUOUS;
    menu->pushChild(continuousItem);

    return menu;
}
