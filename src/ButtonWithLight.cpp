#include "ButtonWithLight.h"

void ButtonWithLight::Init(ModuleWidget *moduleWidget, Module *module, int x, int y, int paramId)
{
    moduleWidget->addParam(createParam<LEDButton>(Vec(x, y), module, paramId, 0.0, 1.0, 0.0));
    moduleWidget->addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(x + 5, y + 5), &m_light));

    m_paramId = paramId;
}

void ButtonWithLight::SetOnOff(bool onOff, bool currentState)
{
    m_onOffType = onOff;
    m_currentState = currentState;
    m_light = m_currentState ? 1.0 : 0.0;
}

void ButtonWithLight::AddInput(int inputId)
{
    m_inputId = inputId;
}

bool ButtonWithLight::GetState()
{
    return m_currentState;
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
