#include <tm.h>

static double timestamp_base = 0;

double tm_timestamp ()
{
    return timestamp_base + tm_uptime_micro();
}

int tm_timestamp_update (double millis)
{
    timestamp_base = millis;
    return 1;
}
