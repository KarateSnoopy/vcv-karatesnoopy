#include "PluginInit.h"
#include "SEQWidget.h"

// The plugin-wide instance of the Plugin class
Plugin *plugin;

void init(rack::Plugin *p)
{
    plugin = p;
    plugin->slug = TOSTRING(SLUG);
    plugin->website = "https://github.com/KarateSnoopy/vcv-karatesnoopy";
    plugin->version = TOSTRING(VERSION);

    p->addModel(modelSEQ);
}
