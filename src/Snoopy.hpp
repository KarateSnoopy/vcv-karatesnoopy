#include "rack.hpp"


using namespace rack;


extern Plugin *plugin;

////////////////////
// module widgets
////////////////////

struct SEQWidget : ModuleWidget {
	SEQWidget();
	Menu *createContextMenu();
};
