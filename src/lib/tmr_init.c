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

#define MAX_TIMERS 2000

static void calc_time (APP_TIMER_T * ptmr);
static TIMER_T * alloc_timer (void);
static int alloc_timer_id (void);
void show_uptime (void);

static sync_lock_t core_timer;


/************* Private Variable Declaration *********************/
static int indx = 0;
/****************************************************************/

/************* Global Variable Declaration *********************/
struct active_timers  tmrrq;
unsigned  int clk[TIMER_WHEEL];
/****************************************************************/

void timer_lock (void)
{
	sync_lock (&core_timer);
}

void timer_unlock (void)
{
	sync_unlock (&core_timer);
}

void timer_lock_create (void)
{
	create_sync_lock (&core_timer);
	timer_unlock ();
}

static inline void update_wheel (TIMER_T *p, int wheel)
{
	p->wheel = wheel;
}

int find_tmr_slot_and_place (APP_TIMER_T * ptmr)
{
	unsigned int ct = ptmr->ctime;
	unsigned int t = 0;
	int wheel = -1;

	if (GET_HRS(ct, t)) 
		wheel = HRS;
	else if (GET_MINS (ct, t)) 
		wheel = MIN;
	else if (GET_SECS (ct, t)) 
		wheel = SEC;
	else if (GET_TICK (ct, t)) 
		wheel = TICK;
	
	if (wheel >= 0) {
		ptmr->rt = (clk[wheel] + t);
		update_wheel (ptmr->timer, wheel);
		ptmr->timer->is_running = 1;
		timer_add (ptmr, &tmrrq.root[wheel], QUERY_TIMER_EXPIRY);
	}
	return 0;
}

static void calc_time (APP_TIMER_T * ptmr)
{
	unsigned int tick = ptmr->timer->rmt;
	unsigned int secs = tick / tm_get_ticks_per_second ();
	unsigned int mins = secs / 60;
	unsigned int hrs =  mins / 60;
	unsigned int tick_offset = 0;
	unsigned int secs_offset = 0;
	unsigned int mins_offset = 0;

	SET_TICK (ptmr->ctime,  tick % SYS_MAX_TICKS_IN_SEC);

	if (secs) {
		tick_offset = get_ticks () % SYS_MAX_TICKS_IN_SEC;
		if (tick_offset) {
			tick += tick_offset;
			SET_TICK (ptmr->ctime,  tick % SYS_MAX_TICKS_IN_SEC);
			secs = tick / SYS_MAX_TICKS_IN_SEC;
		}
		SET_SECS (ptmr->ctime,  secs);
	}

	if (mins) {
		secs_offset = get_secs () % 60;
		if (secs_offset) {
			secs += secs_offset;
			SET_SECS (ptmr->ctime,  secs);
			mins = secs / 60;
		}
		SET_MINS (ptmr->ctime,  mins);
	}

	if (hrs) {
		mins_offset = get_mins () % 60;
		if (mins_offset) {
			mins += mins_offset;
			SET_SECS (ptmr->ctime,  mins);
			hrs = mins / 60;
		}
		SET_HRS  (ptmr->ctime,  hrs);
	}

	ptmr->timer->exp = tick + clk[TICK] - tick_offset;

#ifdef TIMER_DEBUG
	printf ("secs : %d ticks %d tick offset : %d curr ticks : %d expiry : %d act : %d\n", 
		 secs, tick % SYS_MAX_TICKS_IN_SEC, tick_offset, get_ticks (), ptmr->timer->exp, 
		 ptmr->timer->rmt);
#endif
}

void * start_timer (unsigned int ticks, void *data, void (*handler) (void *), int flags)
{
	TIMER_T  *new = NULL;
	APP_TIMER_T *apptimer = NULL;
	int idx = 0;

	timer_lock ();

	if ( !(idx = alloc_timer_id ())  || 
             !(new = alloc_timer ())) {
		timer_unlock ();
		return NULL;
	}

	new->idx = idx;
	new->rmt = ticks;
	new->data = data;
	new->time_out_handler = handler;

	if (flags)
		new->flags = flags;
	else
		new->flags = TIMER_ONCE;

	apptimer = tm_calloc (1, sizeof(APP_TIMER_T));

	if (!apptimer) {
		free_timer (new);
		timer_unlock ();
		return NULL;
	}

	apptimer->timer = new;

	new->apptimer = apptimer;

	INIT_LIST_HEAD (&apptimer->elist);

	calc_time (apptimer);

	find_tmr_slot_and_place (apptimer);

	INC_TIMER_COUNT ();

	timer_unlock ();

	return (void *)new;
}

int setup_timer (void **p, void (*handler) (void *), void *data)
{
	TIMER_T  *new = NULL;
	int idx = 0;
	
	timer_lock ();

	if ( !(idx = alloc_timer_id ())  || 
             !(new = alloc_timer ())) {
		timer_unlock ();
		return -1;
	}

	new->idx = idx;
	new->rmt = 0;
	new->data = data;
	new->time_out_handler = handler;

	new->flags = TIMER_FOREVER;

	update_wheel (new, WAIT_TIMERS);

	*(TIMER_T **)p = new;

	timer_unlock ();

	return 0;
}

int mod_timer (void *timer, unsigned int ticks)
{
	APP_TIMER_T *apptimer = NULL;
	TIMER_T  *p = (TIMER_T *)timer;

	if (!p)
		return -1;

	timer_lock ();
	
	apptimer = tm_calloc (1, sizeof(APP_TIMER_T));

	if (!apptimer) {
		return -1;
	}

	INIT_LIST_HEAD (&apptimer->elist);

	p->rmt = ticks;

	apptimer->timer = p;

	p->apptimer = apptimer;

	calc_time (apptimer);

	find_tmr_slot_and_place (apptimer);

	INC_TIMER_COUNT ();
	
	timer_unlock ();

	return 0;
}

int timer_restart  (TIMER_T *p)
{
	APP_TIMER_T *apptimer = NULL;

	if (!p)
		return -1;

	apptimer = tm_calloc (1, sizeof(APP_TIMER_T));

	if (!apptimer) {
		return -1;
	}

	timer_lock ();

	INIT_LIST_HEAD (&apptimer->elist);

	p->apptimer = apptimer;

	calc_time (apptimer);

	find_tmr_slot_and_place (apptimer);

	INC_TIMER_COUNT ();

	timer_unlock ();

	return 0;
}


static int alloc_timer_id (void)
{
	if (TIMER_COUNT() > MAX_TIMERS) {
		return 0;
	}
	return ++indx;
}

int stop_timer (void *timer)
{
	TIMER_T  *p = (TIMER_T *)timer;

	timer_lock ();

	if (p && p->apptimer && p->is_running) {

		timer_del (p->apptimer, &tmrrq.root[p->wheel]);

		p->is_running = 0;

		free (p->apptimer);

		p->apptimer = NULL;
	
		DEC_TIMER_COUNT ();
	}

	timer_unlock ();
	return 0;
}

int del_timer (void *timer)
{
	TIMER_T  *p = (TIMER_T *)timer;

	timer_lock ();

	p->flags |= TIMER_DELETE;

	timer_unlock ();

	return 0;
}

static inline TIMER_T * alloc_timer (void)
{
	return tm_calloc (1, sizeof(TIMER_T));
}

void free_timer (TIMER_T *p) 
{
	tm_free (p, sizeof(*p));
}

static inline int timers_pending_for_service (void)
{
	if (TIMER_COUNT ()) {
		if (rb_first (&tmrrq.root[TICK_TIMERS])) {
			goto service_req;
		}
		if (IS_NXT_SEC_HAPPEND) {
			if (rb_first (&tmrrq.root[SECS_TIMERS]))
				goto service_req;
		}
		if (IS_NXT_MIN_HAPPEND) {
			if (rb_first (&tmrrq.root[MIN_TIMERS])) 
				goto service_req;
		}
		if (IS_NXT_HR_HAPPEND) {
			if (rb_first (&tmrrq.root[HRS_TIMERS])) 
				goto service_req;
		}
	}
	return 0;
service_req:	
        return 1;
}

void update_times (void)
{
	timer_lock ();

	if (!(++clk[TICK] % tm_get_ticks_per_second ())) {

		if (!((++clk[SEC]) % 60) && !((++clk[MIN]) % 60)) {
			++clk[HRS];
		}
	}
	if (timers_pending_for_service ()) {
		service_timers ();
	}

	timer_unlock ();
}

unsigned int get_secs (void)
{
	return clk[SEC];
}
unsigned int get_ticks (void)
{
	return clk[TICK];
}

unsigned int get_mins (void)
{
	return clk[MIN];
}
unsigned int get_hrs (void)
{
	return clk[HRS];
}
unsigned int get_timers_count (void)
{
	return TIMER_COUNT();
}

void show_uptime (void)
{
	printf ("Uptime  %d hrs %d mins %d secs %d ticks\n",get_hrs(), 
		 get_mins() % 60, get_secs() % 60, get_ticks() % tm_get_ticks_per_second ());
}
void tm_test_timers_latency (void *unused)
{
	show_uptime ();
	start_timer (1 * tm_get_ticks_per_second (), NULL, tm_test_timers_latency , 0);
}
