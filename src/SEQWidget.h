#pragma once
#include "rack.hpp"
#include "SeqModule.h"

using namespace rack;

struct SEQWidget : ModuleWidget
{
    SEQWidget(SEQ *module);
    Menu *createContextMenu() override;
};

extern Model *modelSEQ;