#include "utils.h"

long _frameCount = 0;
void write_log(long freq, const char *format, ...)
{
    if (freq == 0)
        freq++;

    if (_frameCount % freq == 0)
    {
        va_list args;
        va_start(args, format);

        printf("%ld: ", _frameCount);
        vprintf(format, args);
        fflush(stdout);

        va_end(args);
    }
}
