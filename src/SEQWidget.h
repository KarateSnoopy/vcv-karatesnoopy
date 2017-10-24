#pragma once
#include "rack.hpp"
using namespace rack;

struct SEQWidget : ModuleWidget
{
    SEQWidget();
    Menu *createContextMenu();
};

struct SEQEuclidDisplay : TransparentWidget
{
    int *value;
    std::shared_ptr<Font> font;

    SEQEuclidDisplay(int x, int y, int *pvalue);
    void draw(NVGcontext *vg);
};
