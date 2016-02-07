#include <config.h>

#ifdef CHE_WINDOWS

#include <che_time.h>
#include <time.h>
#include <windows.h>
#include <winbase.h>

/* TODO: this is just a quick copy paste that is not working well,
 *       we must review this and do it correctly */

int che_time_uptime(che_time_t *t)
{
	unsigned long long ms = GetTickCount();
	t->s = ms / 1000;
	t->ns = (ms - t->s * 1000) * 1000000;
	return 0;
}

void che_time_sleep(const che_time_t *t)
{
	Sleep(t->ns / 1000000);
}

#endif /* CHE_LINUX */
