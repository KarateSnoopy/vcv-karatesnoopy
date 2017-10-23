#include "rack.hpp"
using namespace rack;
#include "dsp/digital.hpp"

extern long _frameCount;
void write_log(long freq, const char *format, ...);