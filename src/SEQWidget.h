#pragma once
#include "rack.hpp"
#include "SeqModule.h"
using namespace rack;

struct SEQModule;

struct SEQWidget : ModuleWidget
{
    SEQWidget(SEQModule *module);
    Menu *createContextMenu() override;
};
