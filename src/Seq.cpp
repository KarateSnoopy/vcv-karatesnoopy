#include "plugin.hpp"
#include "utils.h"

#define MAX_STEPS 16

struct KSnoopySEQ : Module 
{
    enum ParamIds 
    {
        CLOCK_PARAM,
        RUN_PARAM,
        RESET_PARAM,
        STEPS_PARAM,
        PATTERN_PARAM,
        ENUMS(PITCH_PARAM, MAX_STEPS),
        ENUMS(GATE_ON_PARAM, MAX_STEPS),
        ENUMS(SKIP_PARAM, MAX_STEPS),
        NUM_PARAMS
    };

    enum InputIds 
    {
        CLOCK_INPUT,
        EXT_CLOCK_INPUT,
        RESET_INPUT,
        STEPS_INPUT,
        PATTERN_INPUT,
        NUM_INPUTS
    };

    enum OutputIds 
    {
        PITCH_OUTPUT,
        GATE_X_OUTPUT,
        GATE_Y_OUTPUT,
        GATE_XORY_OUTPUT,
        NUM_OUTPUTS
    };

    enum LightIds 
    {
        RUNNING_LIGHT,
        RESET_LIGHT,
        GATES_LIGHT,
        PITCH_LIGHT,
        GATE_X_LIGHT,
        GATE_Y_LIGHT,
        GATE_X_OR_Y_LIGHT,
        ENUMS(IS_PITCH_ON_LIGHTS, MAX_STEPS),
        ENUMS(SKIP_LIGHTS, MAX_STEPS),
        ENUMS(GATE_PULSE_LIGHTS, MAX_STEPS),
        NUM_LIGHTS
    };

    enum GateMode
    {
        TRIGGER,
        RETRIGGER,
        CONTINUOUS,
    };

    std::vector<std::vector<int>> m_patterns = 
    {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},                                                // forward
        {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},                                                // backward
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1}, // ping pong
        {0, 1, 2, 3, 7, 6, 5, 4, 8, 9, 10, 11, 15, 14, 13, 12},                                                // snake
        {3, 2, 1, 0, 4, 5, 6, 7, 11, 10, 9, 8, 12, 13, 14, 15},                                                // opposite snake
        {15, 14, 13, 12, 8, 9, 10, 11, 7, 6, 5, 4, 0, 1, 2, 3},                                                // backward snake
        {3, 2, 1, 0, 4, 5, 6, 7, 11, 10, 9, 8, 12, 13, 14, 15, 14, 13, 12, 8, 9, 10, 11, 7, 6, 5, 4, 0, 1, 2}, // ping pong snake
        {0, 1, 2, 3, 7, 11, 15, 14, 13, 12, 8, 4, 5, 6, 10, 9}                                                 // circle
    };

    bool m_running = true;
    dsp::SchmittTrigger m_clockTrigger;
    dsp::SchmittTrigger m_runningTrigger;
    dsp::SchmittTrigger m_resetTrigger;
    dsp::SchmittTrigger m_gateTriggers[MAX_STEPS];
    dsp::SchmittTrigger m_skipTriggers[MAX_STEPS];
    dsp::PulseGenerator m_gatePulse;

    float m_phase = 0.f;
    int m_currentPattern = 0;
    int m_currentPatternIndex = 0;
    int m_currentStepIndex = 0;
    int m_lastStepIndex = 0;
    int index = 0;
    bool m_isPitchOn[MAX_STEPS] = {};
    bool m_isSkip[MAX_STEPS] = {};
    GateMode m_gateMode = TRIGGER;

    KSnoopySEQ() 
    {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(CLOCK_PARAM, -2.f, 6.f, 2.f, "Clock tempo", " bpm", 2.f, 60.f);
        configParam(RUN_PARAM, 0.f, 1.f, 0.f);
        configParam(RESET_PARAM, 0.f, 1.f, 0.f);
        configParam(STEPS_PARAM, 1.f, 10.f, 10.f);
        configParam(PATTERN_PARAM, 0.f, 10.0f, 0.f);
        for (int i = 0; i < MAX_STEPS; i++) 
        {
            configParam(PITCH_PARAM + i, 0.f, 10.f, 0.f);
            configParam(GATE_ON_PARAM + i, 0.f, 1.f, 0.f);
            configParam(SKIP_PARAM + i, 0.f, 1.f, 0.f);
        }

        onReset();
    }

    void onReset() override 
    {
        for (int i = 0; i < MAX_STEPS; i++)
        {
            m_isPitchOn[i] = 1.0f;
            m_isSkip[i] = false;
        }
    }

    void RandomizeHelper(bool randomPitch, bool randomGate, bool randomSkip)
    {
        if (randomPitch)
        {
            float dx = rescale(random::uniform(), 0.0, 1.0, 1.0f, 3.0f);
            for (int i = 0; i < MAX_STEPS; i++)
            {
                params[PITCH_PARAM + i].setValue(rescale(random::uniform(), 0.0, 1.0, 0.0f, 2.0f) + dx);
            }
        }

        if (randomGate)
        {
            for (int i = 0; i < MAX_STEPS; i++)
            {
                m_isPitchOn[i] = (random::uniform() > 0.5);
            }
        }

        if (randomSkip)
        {
            for (int i = 0; i < MAX_STEPS; i++)
            {
                m_isSkip[i] = (random::uniform() > 0.5);
            }
        }
    }

    void onRandomize() override 
    {
        RandomizeHelper(true, false, false);
    }

    json_t *dataToJson() override 
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

        // skip
        json_t *gatesS = json_array();
        for (int i = 0; i < MAX_STEPS; i++)
        {
            json_t *gateS = json_integer((int)m_isSkip[i]);
            json_array_append_new(gatesS, gateS);
        }
        json_object_set_new(rootJ, "skips", gatesS);

        // gateMode
        json_t *gateModeJ = json_integer((int)m_gateMode);
        json_object_set_new(rootJ, "gateMode", gateModeJ);

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override 
    {
        // running
        json_t *runningJ = json_object_get(rootJ, "running");
        if (runningJ)
        {
            m_running = json_is_true(runningJ);
        }

        // gates
        json_t *gatesJ = json_object_get(rootJ, "gates");
        if (gatesJ)
        {
            for (int i = 0; i < MAX_STEPS; i++)
            {
                json_t *gateJ = json_array_get(gatesJ, i);
                if (gateJ)
                {
                    m_isPitchOn[i] = !!json_integer_value(gateJ);
                }
            }
        }

        // skips
        json_t *gatesS = json_object_get(rootJ, "skips");
        if (gatesS)
        {
            for (int i = 0; i < MAX_STEPS; i++)
            {
                json_t *gateS = json_array_get(gatesS, i);
                if (gateS)
                {
                    m_isSkip[i] = !!json_integer_value(gateS);
                }
            }
        }

        // gateMode
        json_t *gateModeJ = json_object_get(rootJ, "gateMode");
        if (gateModeJ)
        {
            m_gateMode = (GateMode)json_integer_value(gateModeJ);
        }
    }

    bool ProcessClockAndReset(const ProcessArgs &args, bool& gateIn)
    {
        bool nextStep = false;
        if (inputs[EXT_CLOCK_INPUT].isConnected()) 
        {
            // External clock
            if (m_clockTrigger.process(inputs[EXT_CLOCK_INPUT].getVoltage())) 
            {
                m_phase = 0.0;
                nextStep = true;
            }
            gateIn = m_clockTrigger.isHigh();
        }
        else 
        {
            // Internal clock
            float clockTime = std::pow(2.f, params[CLOCK_PARAM].getValue() + inputs[CLOCK_INPUT].getVoltage());
            m_phase += clockTime * args.sampleTime;
            if (m_phase >= 1.f) 
            {
                m_phase = 0.0;
                nextStep = true;
            }
            gateIn = (m_phase < 0.5f);
        }

        if (m_resetTrigger.process(params[RESET_PARAM].getValue() + inputs[RESET_INPUT].getVoltage())) 
        {
            m_phase = 0.0;
            m_currentStepIndex = MAX_STEPS;
            nextStep = true;
        }

        return nextStep;
    }

    void AdvanceStep()
    {
        // TODO

        float patternScale = clamp(params[PATTERN_PARAM].getValue() + inputs[PATTERN_INPUT].getVoltage(), 0.0f, 10.0f);
        patternScale /= 10.0f;
        m_currentPattern = clamp((int)roundf(patternScale * m_patterns.size()), 1, m_patterns.size());
        //printf("m_currentPattern %d\n", m_currentPattern);

        m_lastStepIndex = m_currentStepIndex;
        float stepsScale = clamp(params[STEPS_PARAM].getValue() + inputs[STEPS_INPUT].getVoltage(), 0.0f, 10.0f);
        stepsScale /= 10.0f;
        //write_log(10, "stepsScale=%f params[STEPS_PARAM].getValue(): %f inputs[STEPS_INPUT].getVoltage(): %f\n", stepsScale, params[STEPS_PARAM].getValue(), inputs[STEPS_INPUT].getVoltage());

        int maxStepsInPattern = m_patterns[m_currentPattern - 1].size();
        //printf("maxStepsInPattern %d\n", maxStepsInPattern);
        int numSteps = clamp((int)roundf(stepsScale * maxStepsInPattern), 1, maxStepsInPattern);
        //printf("numSteps %d\n", numSteps);

        for (int skipAttempts = 0; skipAttempts < MAX_STEPS; skipAttempts++)
        {
            m_currentPatternIndex += 1;
            if (m_currentPatternIndex >= numSteps)
            {
                m_currentPatternIndex = 0;
            }
            m_currentPatternIndex %= maxStepsInPattern;
            m_currentStepIndex = m_patterns[m_currentPattern - 1][m_currentPatternIndex];
            //write_log(1, "stepsScale=%f m_currentStepIndex=%d m_currentPatternIndex=%d maxStepsInPattern=%d numSteps=%d\n", stepsScale, m_currentStepIndex, m_currentPatternIndex, maxStepsInPattern, numSteps);
            if (!m_isSkip[m_currentStepIndex])
            {
                break;
            }
            else
            {
                numSteps++; // ignore wrt # of steps
            }
        }

        // write_log(0, "maxStepsInPattern=%d numSteps=%d m_currentPatternIndex=%d m_currentStepIndex=%d\n",
        //           maxStepsInPattern,
        //           numSteps,
        //           m_currentPatternIndex,
        //           m_currentStepIndex);

        // TODO
        lights[GATE_PULSE_LIGHTS + m_currentStepIndex].value = 1.0;
        m_gatePulse.trigger(1e-3);
    }

    void ProcessXYTriggers(bool gateIn)
    {
        //bool pulse = m_gatePulse.process(1.0 / APP->engine->getSampleRate());

        // Rows
        int lastX = m_lastStepIndex % 4;
        int curX = m_currentStepIndex % 4;
        int lastY = m_lastStepIndex / 4;
        int curY = m_currentStepIndex / 4;

        // X row
        bool gateXChanged = (m_running && m_isPitchOn[m_currentStepIndex] > 0.0f && lastX != curX);
        gateXChanged = gateXChanged && gateIn;
        // if (m_gateMode == TRIGGER)
        // {
        //     gateXChanged = gateXChanged && pulse;
        // }
        // else if (m_gateMode == RETRIGGER)
        // {
        //     gateXChanged = gateXChanged && !pulse;
        // }

        outputs[GATE_X_OUTPUT].setVoltage(gateXChanged ? 10.0 : 0.0);
        if (gateXChanged)
        {
            lights[GATE_X_LIGHT].value = 1.0;
        }
        //write_log(0, "gateXChanged=%d m_running=%d m_isPitchOn[m_currentStepIndex]=%d lastX=%d curX=%d lastY=%d curY=%d\n", gateXChanged, m_running, m_isPitchOn[m_currentStepIndex], lastX, curX, lastY, curY);

        // Y row
        bool gateYChanged = (m_running && m_isPitchOn[m_currentStepIndex] > 0.0f && lastY != curY);
        gateYChanged = gateYChanged && gateIn;
        // if (m_gateMode == TRIGGER)
        // {
        //     gateYChanged = gateYChanged && pulse;
        // }
        // else if (m_gateMode == RETRIGGER)
        // {
        //     gateYChanged = gateYChanged && !pulse;
        // }
        
        outputs[GATE_Y_OUTPUT].setVoltage(gateYChanged ? 10.0 : 0.0);
        if (gateYChanged)
        {
            lights[GATE_Y_LIGHT].value = 1.0;
        }

        outputs[GATE_XORY_OUTPUT].setVoltage((gateXChanged || gateYChanged) ? 10.0 : 0.0);
        if (gateXChanged || gateYChanged)
        {
            lights[GATE_X_OR_Y_LIGHT].value = 1.0;
        }
    }

    void UpdateLights(const ProcessArgs &args)
    {
        lights[RUNNING_LIGHT].value = (m_running);
        lights[GATE_X_LIGHT].setSmoothBrightness(0, args.sampleTime);
        lights[GATE_Y_LIGHT].setSmoothBrightness(0, args.sampleTime);
        lights[GATE_X_OR_Y_LIGHT].setSmoothBrightness(0, args.sampleTime);
        lights[RESET_LIGHT].setSmoothBrightness(m_resetTrigger.isHigh(), args.sampleTime);
        lights[GATES_LIGHT].setSmoothBrightness(0, args.sampleTime);
        for (int i = 0; i < MAX_STEPS; i++)
        {
            lights[IS_PITCH_ON_LIGHTS + i].setSmoothBrightness(0, args.sampleTime);
            lights[SKIP_LIGHTS + i].setSmoothBrightness(0, args.sampleTime);
            lights[GATE_PULSE_LIGHTS + i].setSmoothBrightness(0, args.sampleTime);
        }
    }

    void process(const ProcessArgs &args) override 
    {
        log_increase_step_number();

        // Run
        if (m_runningTrigger.process(params[RUN_PARAM].getValue())) 
        {
            m_running = !m_running;
        }

        bool nextStep = false;
        bool gateIn = false;
        if (m_running)
        {
            nextStep = ProcessClockAndReset(args, gateIn);
        }
        if (nextStep)
        {
            AdvanceStep();
        }
        ProcessXYTriggers(gateIn);

        // Pitch output
        float currentPitch = params[PITCH_PARAM + m_currentStepIndex].getValue();
        outputs[PITCH_OUTPUT].setVoltage(currentPitch);
        lights[PITCH_LIGHT].value = currentPitch;
        UpdateLights(args);

        // Gate buttons
        for (int i = 0; i < MAX_STEPS; i++) 
        {
            if (m_gateTriggers[i].process(params[GATE_ON_PARAM + i].getValue())) 
            {
                m_isPitchOn[i] = !m_isPitchOn[i];
            }
            lights[IS_PITCH_ON_LIGHTS + i].setSmoothBrightness(m_isPitchOn[i] ? 10.0f : 0.0f, args.sampleTime);

            if (m_skipTriggers[i].process(params[SKIP_PARAM + i].getValue()))
            {
                m_isSkip[i] = !m_isSkip[i];
            }
            lights[SKIP_LIGHTS + i].setSmoothBrightness(m_isSkip[i] ? 10.0f : 0.0f, args.sampleTime);
        }
    }
};


struct SEQGateModeItem : MenuItem
{
    KSnoopySEQ* module;
    KSnoopySEQ::GateMode gateMode;
    void onAction(const event::Action &e) override 
    {
        module->m_gateMode = gateMode;
    }

    void step() override
    {
        rightText = (module->m_gateMode == gateMode) ? "✔" : "";
    }
};

struct SEQActionItem : MenuItem
{
    KSnoopySEQ* module;
    bool randomPitch = false;
    bool randomGate = false;
    bool randomSkip = false;
    void onAction(const event::Action &e) override 
    {
        module->RandomizeHelper(randomPitch, randomGate, randomSkip);
    }
};

struct KSnoopySEQWidget : ModuleWidget 
{
    KSnoopySEQWidget(KSnoopySEQ *module) 
    {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Seq.svg")));

        addChild(createWidget<ScrewSilver>(Vec(15, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
        addChild(createWidget<ScrewSilver>(Vec(15, 365)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

        addParam(createParam<RoundBlackKnob>(Vec(18, 56), module, KSnoopySEQ::CLOCK_PARAM));
        addParam(createParam<LEDButton>(Vec(60, 61-1), module, KSnoopySEQ::RUN_PARAM));
        addChild(createLight<MediumLight<GreenLight>>(Vec(64.4f, 64.4f), module, KSnoopySEQ::RUNNING_LIGHT));
        addParam(createParam<LEDButton>(Vec(99, 61-1), module, KSnoopySEQ::RESET_PARAM));
        addChild(createLight<MediumLight<GreenLight>>(Vec(103.4f, 64.4f), module, KSnoopySEQ::RESET_LIGHT));
        addParam(createParam<RoundBlackSnapKnob>(Vec(132, 56), module, KSnoopySEQ::STEPS_PARAM));
        addChild(createLight<MediumLight<GreenLight>>(Vec(179.4f, 64.4f), module, KSnoopySEQ::GATE_X_OR_Y_LIGHT));
        addChild(createLight<MediumLight<GreenLight>>(Vec(179.4f + 38.0f*1, 64.4f), module, KSnoopySEQ::GATE_X_LIGHT));
        addChild(createLight<MediumLight<GreenLight>>(Vec(179.4f + 38.0f*2, 64.4f), module, KSnoopySEQ::GATE_Y_LIGHT));
        addChild(createLight<MediumLight<GreenLight>>(Vec(179.4f + 38.0f*3, 64.4f), module, KSnoopySEQ::PITCH_LIGHT));

        static const float portX[8] = {20, 58, 96, 135, 173, 212, 250, 289};
        addInput(createInput<PJ301MPort>(Vec(portX[0]-1, 98), module, KSnoopySEQ::CLOCK_INPUT));
        addInput(createInput<PJ301MPort>(Vec(portX[1]-1, 98), module, KSnoopySEQ::EXT_CLOCK_INPUT));
        addInput(createInput<PJ301MPort>(Vec(portX[2]-1, 98), module, KSnoopySEQ::RESET_INPUT));
        addInput(createInput<PJ301MPort>(Vec(portX[3]-1, 98), module, KSnoopySEQ::STEPS_INPUT));

        addOutput(createOutput<PJ301MPort>(Vec(portX[4]-1, 98), module, KSnoopySEQ::GATE_XORY_OUTPUT));
        addOutput(createOutput<PJ301MPort>(Vec(portX[5]-1, 98), module, KSnoopySEQ::GATE_X_OUTPUT));
        addOutput(createOutput<PJ301MPort>(Vec(portX[6]-1, 98), module, KSnoopySEQ::GATE_Y_OUTPUT));
        addOutput(createOutput<PJ301MPort>(Vec(portX[7]-1, 98), module, KSnoopySEQ::PITCH_OUTPUT));

        addParam(createParam<RoundBlackKnob>(Vec(portX[7]-2, 148), module, KSnoopySEQ::PATTERN_PARAM));
        addInput(createInput<PJ301MPort>(Vec(portX[7], 188), module, KSnoopySEQ::PATTERN_INPUT));

        const float btn_x[4] = {0+10*0, 38 + 10*1, 76 + 10*2, 115 + 10*3};
        const float btn_y[4] = {0+10*0, 38 + 10*1, 76 + 10*2, 115 + 10*3};
        int iZ = 0;
        for (int iY = 0; iY < 4; iY++)
        {
            for (int iX = 0; iX < 4; iX++)
            {
                int x = btn_x[iX] + 80;
                int y = btn_y[iY] + 167;
                addParam(createParam<RoundBlackKnob>(Vec(x-10, y-10), module, KSnoopySEQ::PITCH_PARAM + iZ));
                addChild(createLight<MediumLight<GreenLight>>(Vec(x, y), module, KSnoopySEQ::GATE_PULSE_LIGHTS + iZ));

                x -= 9;
                y -= 3;
                int radius = 13;
                int xGateOn = x-radius;
                int yGateOn = y-radius;
                addParam(createParam<LEDButton>(Vec(xGateOn, yGateOn), module, KSnoopySEQ::GATE_ON_PARAM + iZ));
                addChild(createLight<MediumLight<GreenLight>>(Vec(xGateOn+4, yGateOn+4), module, KSnoopySEQ::IS_PITCH_ON_LIGHTS + iZ));

                int xSkip = x-radius;
                int ySkip = y+radius;
                addParam(createParam<LEDButton>(Vec(xSkip, ySkip), module, KSnoopySEQ::SKIP_PARAM + iZ));
                addChild(createLight<MediumLight<GreenLight>>(Vec(xSkip+4, ySkip+4), module, KSnoopySEQ::SKIP_LIGHTS + iZ));

                iZ++;
            }
        }
    }

    void appendContextMenu(Menu *menu) override 
    {
        KSnoopySEQ *module = dynamic_cast<KSnoopySEQ*>(this->module);

        SEQActionItem *triggerItem1 = new SEQActionItem();
        triggerItem1->text = "Randomize Pitch";
        triggerItem1->module = module;
        triggerItem1->randomPitch = true;
        menu->addChild(triggerItem1);

        SEQActionItem *triggerItem2 = new SEQActionItem();
        triggerItem2->text = "Randomize Gate";
        triggerItem2->module = module;
        triggerItem2->randomGate = true;
        menu->addChild(triggerItem2);

        SEQActionItem *triggerItem3 = new SEQActionItem();
        triggerItem3->text = "Randomize Skip";
        triggerItem3->module = module;
        triggerItem3->randomSkip = true;
        menu->addChild(triggerItem3);

        // MenuLabel *modeLabel = new MenuLabel();
        // modeLabel->text = "Gate Mode";
        // menu->addChild(modeLabel);

        // SEQGateModeItem *triggerItem = new SEQGateModeItem();
        // triggerItem->text = "Trigger";
        // triggerItem->module = module;
        // triggerItem->gateMode = KSnoopySEQ::TRIGGER;
        // menu->addChild(triggerItem);

        // SEQGateModeItem *retriggerItem = new SEQGateModeItem();
        // retriggerItem->text = "Retrigger";
        // retriggerItem->module = module;
        // retriggerItem->gateMode = KSnoopySEQ::RETRIGGER;
        // menu->addChild(retriggerItem);

        // SEQGateModeItem *continuousItem = new SEQGateModeItem();
        // continuousItem->text = "Continuous";
        // continuousItem->module = module;
        // continuousItem->gateMode = KSnoopySEQ::CONTINUOUS;
        // menu->addChild(continuousItem);
    }
};

Model* modelSeq = createModel<KSnoopySEQ, KSnoopySEQWidget>("KSnpy 2D Grid Seq");