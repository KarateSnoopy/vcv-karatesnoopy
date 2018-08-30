#pragma once
#include "rack.hpp"
using namespace rack;

struct SEQModule;

struct SEQWidget : ModuleWidget
{
    SEQWidget(SEQModule *module);
    Menu *createContextMenu() override;
};
