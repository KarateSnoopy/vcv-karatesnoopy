#include "Snoopy.hpp"


// The plugin-wide instance of the Plugin class
Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	plugin->slug = "Snoopy";
	plugin->name = "Snoopy";
	plugin->homepageUrl = "https://github.com/KarateSnoopy/vcv-snoopy";
#ifdef VERSION
//	plugin->version = TOSTRING(VERSION);
#endif

	createModel<MyModuleWidget>(plugin, "Simple Seq", "Simple Seq");

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables within this file or the individual module files to reduce startup times of Rack.
}
