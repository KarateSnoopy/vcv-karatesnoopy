#include "PluginInit.h"
#include "SEQWidget.h"
#include "SeqModule.h"

#include "rack.hpp"

using namespace rack;

// The plugin-wide instance of the Plugin class
rack::Plugin *plugin;

void init(rack::Plugin *p)
{
    plugin = p;
    p->slug = TOSTRING(SLUG);
    p->version = TOSTRING(VERSION);

    Model *modelMyModule = Model::create<SEQModule, SEQWidget>("KarateSnoopy", "KSnpy 2D Grid Seq", "KSnpy 2D Grid Seq", SEQUENCER_TAG);

    p->addModel(modelMyModule);
}
