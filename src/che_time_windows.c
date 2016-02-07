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
	HANDLE timer;	/* Timer handle */
	LARGE_INTEGER li;	/* Time defintion */
	/* Create timer */
	if (!(timer = CreateWaitableTimer(NULL, TRUE, NULL)))
		return;
	/* Set timer properties */
	li.QuadPart = -t->ns;
	if (!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)) {
		CloseHandle(timer);
		return;
	}
	/* Start & wait for timer */
	WaitForSingleObject(timer, INFINITE);
	/* Clean resources */
	CloseHandle(timer);
}

#endif /* CHE_LINUX */
