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

struct Light2 : TransparentWidget
{
    NVGcolor bgColor = nvgRGBf(0, 0, 0);
    NVGcolor color = nvgRGBf(1, 1, 1);
    void draw(NVGcontext *vg);
};

void Light2::draw(NVGcontext *vg)
{
    float radius = box.size.x / 2.0;
    float oradius = radius + 20.0;

    // Solid
    nvgBeginPath(vg);
    nvgCircle(vg, radius, radius, radius);
    nvgFillColor(vg, bgColor);
    nvgFill(vg);

    // Border
    nvgStrokeWidth(vg, 1.0);
    NVGcolor borderColor = bgColor;
    borderColor.a *= 0.5;
    nvgStrokeColor(vg, borderColor);
    nvgStroke(vg);

    // Inner glow
    nvgGlobalCompositeOperation(vg, NVG_LIGHTER);
    nvgFillColor(vg, color);
    nvgFill(vg);

    // Outer glow
    nvgBeginPath(vg);
    nvgRect(vg, radius - oradius, radius - oradius, 2 * oradius, 2 * oradius);
    NVGpaint paint;
    NVGcolor icol = color;
    icol.a *= 0.15;
    NVGcolor ocol = color;
    ocol.a = 0.0;
    paint = nvgRadialGradient(vg, radius, radius, radius, oradius, icol, ocol);
    nvgFillPaint(vg, paint);
    nvgFill(vg);
}

struct SEQ : Module
{
    enum ParamIds
    {
        CLOCK_PARAM,
        RUN_PARAM,
        RESET_PARAM,
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

    bool running = true;
    SchmittTrigger clockTrigger; // for external clock

    // For buttons
    SchmittTrigger runningTrigger;
    SchmittTrigger resetTrigger;
    float phase = 0.0;
    int index = 0;

    // TODO
    SchmittTrigger gateTriggers[MAX_STEPS];
    bool gateState[MAX_STEPS] = {0};
    float stepLights[MAX_STEPS] = {};

    enum GateMode
    {
        TRIGGER,
        RETRIGGER,
        CONTINUOUS,
    };
    GateMode gateMode = TRIGGER;
    PulseGenerator gatePulse;

    // Lights
    float runningLight = 0.0;
    float resetLight = 0.0;

    // TODO
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
    _frameCount++;

    const float lightLambda = 0.075;
    // Run
    if (runningTrigger.process(params[RUN_PARAM].value))
    {
        running = !running;
    }
    runningLight = running ? 1.0 : 0.0;

    bool nextStep = false;

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

    // Reset
    if (resetTrigger.process(params[RESET_PARAM].value + inputs[RESET_INPUT].value))
    {
        phase = 0.0;
        index = 8;
        nextStep = true;
        resetLight = 1.0;
    }

    if (nextStep)
    {
        // Advance step
        int numSteps = clampi(roundf(params[STEPS_PARAM].value + inputs[STEPS_INPUT].value), 1, 8);
        index += 1;
        if (index >= numSteps)
        {
            index = 0;
        }
        stepLights[index] = 1.0;
        gatePulse.trigger(1e-3);
    }

    resetLight -= resetLight / lightLambda / gSampleRate;

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
        gateLights[i] = gateState[i] ? 1.0 - stepLights[i] : stepLights[i];
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
    addParam(createParam<LEDButton>(Vec(60, 61 - 1), module, SEQ::RUN_PARAM, 0.0, 1.0, 0.0));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(65, 65), &module->runningLight));
    addParam(createParam<LEDButton>(Vec(99, 61 - 1), module, SEQ::RESET_PARAM, 0.0, 1.0, 0.0));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(104, 65), &module->resetLight));
    addParam(createParam<Davies1900hSmallBlackSnapKnob>(Vec(132, 56), module, SEQ::STEPS_PARAM, 1.0, MAX_STEPS, MAX_STEPS));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(180, 65), &module->gatesLight));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(219, 65), &module->rowLights[0]));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(257, 65), &module->rowLights[1]));
    addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(296, 65), &module->rowLights[2]));

    static const float portX[8] = {20, 58, 96, 135, 173, 212, 250, 289};
    addInput(createInput<PJ301MPort>(Vec(portX[0] - 1, 98), module, SEQ::CLOCK_INPUT));
    addInput(createInput<PJ301MPort>(Vec(portX[1] - 1, 98), module, SEQ::EXT_CLOCK_INPUT));
    addInput(createInput<PJ301MPort>(Vec(portX[2] - 1, 98), module, SEQ::RESET_INPUT));
    addInput(createInput<PJ301MPort>(Vec(portX[3] - 1, 98), module, SEQ::STEPS_INPUT));
    addOutput(createOutput<PJ301MPort>(Vec(portX[4] - 1, 98), module, SEQ::GATES_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(portX[5] - 1, 98), module, SEQ::ROW1_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(portX[6] - 1, 98), module, SEQ::ROW2_OUTPUT));
    addOutput(createOutput<PJ301MPort>(Vec(portX[7] - 1, 98), module, SEQ::ROW3_OUTPUT));

    static const float btn_x[4] = {20, 58, 96, 135};
    static const float btn_y[4] = {20, 58, 96, 135};
    int iZ = 0;
    for (int iY = 0; iY < 4; iY++)
    {
        for (int iX = 0; iX < 4; iX++)
        {
            int x = btn_x[iX] - 2;
            int y = btn_y[iY] + 157;
            addParam(createParam<RoundBlackKnob>(Vec(x, y), module, SEQ::ROW1_PARAM + iZ, 0.0, 6.0, 0.0));
            x += 10;
            y += 10;
            //addParam(createParam<LEDButton>(Vec(x, y), module, SEQ::GATE_PARAM + i, 0.0, 1.0, 0.0));
            addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(x + 5, y + 5), &module->gateLights[iZ]));

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
