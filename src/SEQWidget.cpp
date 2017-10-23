#include "PluginInit.h"
#include "ButtonWithLight.h"
#include "dsp/digital.hpp"
#include "utils.h"
#include "SeqModule.h"

SEQWidget::SEQWidget()
{
    SEQ *module = new SEQ();
    setModule(module);
    box.size = Vec(15 * 22, 380);
    module->InitUI(this, box);
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
