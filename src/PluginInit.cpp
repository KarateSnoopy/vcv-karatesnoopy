#include "PluginInit.h"
#include "SEQWidget.h"

// The plugin-wide instance of the Plugin class
Plugin *plugin;

void init(rack::Plugin *p)
{
    plugin = p;
    plugin->slug = "Snoopy";
    plugin->name = "Snoopy";
    plugin->homepageUrl = "https://github.com/KarateSnoopy/vcv-snoopy";
#ifdef VERSION
//	plugin->version = TOSTRING(VERSION);
#endif

    createModel<SEQWidget>(plugin, "SEQ", "SEQ");
}
