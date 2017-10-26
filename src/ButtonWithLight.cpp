#include "ButtonWithLight.h"
#include "utils.h"

void ButtonWithLight::Init(ModuleWidget *moduleWidget, Module *module, int x, int y, int paramId, float *pValue, bool bigButton)
{
    ParamWidget *p;
    if (bigButton)
    {
        p = createParam<PB61303>(Vec(x, y), module, paramId, 0.0, 1.0, 0.0);
    }
    else
    {
        p = createParam<LEDButton>(Vec(x, y), module, paramId, 0.0, 1.0, 0.0);
    }
    moduleWidget->addParam(p);
    m_controls.push_back(p);

    m_pValue = pValue;
    if (m_pValue == nullptr)
        m_pValue = &m_light;
    int delta = (bigButton) ? 10 : 5;
    auto p2 = createValueLight<SmallLight<GreenValueLight>>(Vec(x + delta, y + delta), m_pValue);
    moduleWidget->addChild(p2);
    m_controls.push_back(p2);

    m_paramId = paramId;
}

void ButtonWithLight::SetOnOff(bool onOff, bool currentState)
{
    m_onOffType = onOff;
    *m_pValue = currentState ? 1.0 : 0.0;
}

void ButtonWithLight::SetVisible(bool showUI)
{
    for (auto &c : m_controls)
    {
        c->visible = showUI;
    }
}

void ButtonWithLight::AddInput(int inputId)
{
    m_inputId = inputId;
}

float ButtonWithLight::GetState()
{
    return *m_pValue;
}

bool ButtonWithLight::Process(std::vector<Param> &params)
{
    return ProcessHelper(params[m_paramId].value);
}

bool ButtonWithLight::ProcessWithInput(std::vector<Param> &params, std::vector<Input> &input)
{
    return ProcessHelper(params[m_paramId].value + input[m_inputId].value);
}

bool ButtonWithLight::ProcessHelper(float value)
{
    bool returnValue = false;
    if (m_trigger.process(value))
    {
        returnValue = true;
    }

    const float lightLambda = 0.075;

    if (m_onOffType)
    {
        if (returnValue)
        {
            if (*m_pValue > 0.0f)
                *m_pValue = 0.0f;
            else
                *m_pValue = 1.0f;
        }

        if (m_log)
        {
            write_log(0, "returnValue: %d m_pValue:%f\n", returnValue, *m_pValue);
        }
    }
    else
    {
        if (returnValue)
        {
            *m_pValue = 1.0f;
        }
        *m_pValue -= *m_pValue / lightLambda / gSampleRate;

        if (m_log)
        {
            write_log(0, "returnValue: %d m_pValue:%f\n", returnValue, *m_pValue);
        }
    }
    return returnValue;
}
