/*
 *  Author:
 *  Sasikanth.V        <sasikanth@email.com>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include "inc.h"


void * tick_service (void *unused) ;
void * tick_clock (void *unused);
int init_timer_mgr (void);

static EVT_T timer_event;

unsigned int tm_get_ticks_per_second (void) 
{
	return SYS_MAX_TICKS_IN_SEC;
}

unsigned int milli_secs_to_ticks (unsigned int msecs)
{
	return (msecs / TICK_TIMER_GRANULARITY);
}

void * tick_clock (void *unused)
{
	register clock_t    start, end;
	register int        tick = 0;

	for (;;) {

		start = times (NULL);
		tsk_delay (0, MILLISEC_2_NANOSEC (TICK_TIMER_GRANULARITY));
		end = times (NULL);
		
		tick = end - start;

		if (tick <= 0)
			tick = 1;

		while (tick--) {
			update_times (); 
		}
	}
	return NULL;
}

#ifdef TIMER_BTM_HALF
void * tick_service (void *unused) 
{
	int evt = 0;

	while (1) {
		EvtRx (&timer_event, &evt, TMR_SERVE_TIMERS);

		if (evt & TMR_SERVE_TIMERS)
			btm_hlf ();
	}
	return NULL;
}
#endif

static inline void timer_rq_init (void)
{
#ifdef TIMER_BTM_HALF
	INIT_LIST_HEAD (&expd_tmrs);
#endif
	tmrrq.count = 0;
}

int init_timer_mgr (void)
{
	int i = TIMER_WHEEL;
	tmtaskid_t btmhlftask_id = 0;
	tmtaskid_t task_id = 0;

	timer_rq_init ();

	EventInit (&timer_event);

#ifdef TIMER_BTM_HALF
	if (task_create ("TMRBHF", 99, TSK_SCHED_RR, 32000,
	  		  tick_service, NULL, NULL, &btmhlftask_id) == TSK_FAILURE) {
		return FAILURE;
	}
#endif

	if (task_create ("TMRTHF", 99, TSK_SCHED_RR, 32000,
			  tick_clock, NULL, NULL, &task_id) == TSK_FAILURE) {

		return FAILURE;
	}
	while (--i >= 0) {
		create_sync_lock (&tmrrq.root[i].lock);
		sync_unlock (&tmrrq.root[i].lock);
	}

	timer_lock_create ();

#ifdef TIMER_BTM_HALF
	bh_timer_lock_create ();
#endif

	return SUCCESS;
}


void service_timers (void)
{
        if (tm_process_tick_and_update_timers ()) {
#ifdef TIMER_BTM_HALF
		EvtSnd (&timer_event, TMR_SERVE_TIMERS);
#endif
		return;
	}
}
