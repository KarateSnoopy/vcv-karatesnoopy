#pragma once

#include "rack.hpp"
using namespace rack;
#include "dsp/digital.hpp"

class ButtonWithLight
{
  public:
    void Init(ModuleWidget *moduleWidget, Module *module, int x, int y, int paramId);
    void SetOnOff(bool onOff, bool currentState);
    void AddInput(int inputId);
    bool GetState();
    bool Process(std::vector<Param> &params);
    bool ProcessWithInput(std::vector<Param> &params, std::vector<Input> &input);
    bool ProcessHelper(float value);

  private:
    float m_light = 0.0;
    int m_paramId = 0;
    int m_inputId = -1;
    SchmittTrigger m_trigger;
    bool m_onOffType = false;
    bool m_currentState = false;
};
