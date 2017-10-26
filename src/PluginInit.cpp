#include "PluginInit.h"
#include "SEQWidget.h"

// The plugin-wide instance of the Plugin class
Plugin *plugin;

void init(rack::Plugin *p)
{
    plugin = p;
    plugin->slug = "KarateSnoopy";
    plugin->name = "KarateSnoopy";
    plugin->homepageUrl = "https://github.com/KarateSnoopy/vcv-karatesnoopy";
#ifdef VERSION
//	plugin->version = TOSTRING(VERSION);
#endif

    createModel<SEQWidget>(plugin, "KSNPY 2D GRID SEQ", "KSNPY 2D GRID SEQ");
}
