#include <tm.h>
#include <sys/time.h>

int _gettimeofday (struct timeval * tp, void* zone)
{
	(void) zone;
	long long ts = tm_timestamp();
	tp->tv_sec = floor(ts / 1000000);
	tp->tv_usec = ts % 1000000;
	return 0;
}
