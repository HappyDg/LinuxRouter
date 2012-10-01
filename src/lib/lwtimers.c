/*
 *  Author:
 *  Sasikanth.V        <sasikanth@email.com>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include "common_types.h"
#include "list.h"

#define    SET_TIMER_MGR_STATE(state)    tmrmgrstate = state

#define    MAX_TIMERS         2000
#define    QUERY_TIMER_EXPIRY  0x1
#define    QUERY_TIMER_INDEX   0x2
#define    TIMER_ONCE  	        0x1
#define    TIMER_REPEAT  	0x2 
#define    TIMER_FOREVER        0x4
#define    TIMER_DELETE         0x8
#define    SYS_MAX_TICKS_IN_SEC    100 /*Since tick timer runs for 10ms: 1 sec = 1000ms (10ms * 100) */
#define    TICK_TIMER_GRANULARITY  10  /*10 milli secs*/
#define    SUCCESS                 0
#define    FAILURE                 1

#define MILLISEC_2_NANOSEC(msec)  msec * 1000 * 1000

typedef struct tm_timer
{
	struct  list_head next;
	void           *data;
 	void           (*time_out_handler)(void *);
	unsigned int    exp;
	unsigned int    time;
	int 	        idx;
	int	        flags;
	int 		is_running;
}TIMER_T;

/************* Private func Prototype  *********************/
static TIMER_T * alloc_timer    (void);
static  int      alloc_timer_id (void);
void             show_uptime    (void);
void    *        tick_service   (void *unused) ;
void    *        tick_clock     (void *unused);
int              init_timer_mgr (void);
static inline void timer_expiry_action (TIMER_T * ptmr, int );
static unsigned int get_ticks (void);
static void free_timer (TIMER_T *p); 
static void update_times (int cpu);
static int tm_process_tick_and_update_timers (int cpu);
static int timer_restart  (TIMER_T *p);
static void handle_expired_timer (TIMER_T *ptmr);
/****************************************************************/

/************* Private Variable Declaration *********************/
struct timer_mgr {
	struct list_head timers_list;
	volatile unsigned long ticks;
	sync_lock_t  core_timer;
}*tmr_mgr;
static int          timers_count;
static int          indx;
/****************************************************************/


static void timer_lock (int cpu)
{
	sync_lock (&(tmr_mgr + cpu)->core_timer);
}

static void timer_unlock (int cpu)
{
	sync_unlock (&(tmr_mgr + cpu)->core_timer);
}

static void timer_lock_create (int cpu)
{
	create_sync_lock (&(tmr_mgr + cpu)->core_timer);
	timer_unlock (cpu);
}

#if 0
static void debug_timers (void)
{
	int i  = 0;
	TIMER_T *p = NULL;
	return ;

	printf ("\n\n");
	list_for_each_entry (p, &timers_list, next) {
		printf ("Timer value : %d\n", p->exp, i++);
		if (i > timers_count) {
			int *k = NULL;
			*k = 0;
		}
	}
	printf ("\n\n");
}
#endif

static void timer_add_sort (TIMER_T *new, int cpu)
{
	TIMER_T *cur = NULL;
	struct list_head *head = &(tmr_mgr + cpu)->timers_list;

	timers_count ++;
	if (!list_empty(head)) {
		list_for_each_entry (cur, head, next) {
			if (new->exp <= cur->exp) {
				if (cur->next.prev == head) {
					list_add (&new->next, head);
				} else {
					__list_add (&new->next, cur->next.prev, &cur->next);
				}
				break;
			}
			else if (new->exp > cur->exp) {
				TIMER_T *nxt = (TIMER_T *)cur->next.next;
				if (list_is_last(&cur->next, head)) {
					list_add_tail (&new->next, &nxt->next);  
					break;
				}
				else if ((new->exp <= nxt->exp)) {
					__list_add (&new->next, &cur->next, &nxt->next);  
					break;
				} 
			}
		}
	} else  
		list_add (&new->next, head);
}

static void timer_add (TIMER_T *p, int cpu)
{
	timer_add_sort (p, cpu);
}

void * start_timer (unsigned int tick, void *data, void (*handler) (void *), int flags)
{
	TIMER_T  *new = NULL;
	int idx = 0;
	int cpu = get_cpu ();


	if ( !(idx = alloc_timer_id ())  || 
             !(new = alloc_timer ())) {
		return NULL;
	}

	INIT_LIST_HEAD (&new->next);
	new->idx = idx;
	new->data = data;
	new->time_out_handler = handler;
	new->exp = (tmr_mgr + cpu)->ticks + tick;
	new->time = tick;

	if (flags)
		new->flags = flags;
	else
		new->flags = TIMER_ONCE;

	timer_lock (cpu);

	timer_add (new, cpu);

	new->is_running = 1;

	timer_unlock (cpu);

	return (void *)new;
}

int setup_timer (void **p, void (*handler) (void *), void *data)
{
	TIMER_T  *new = NULL;
	int idx = 0;
	

	if ( !(idx = alloc_timer_id ())  || 
             !(new = alloc_timer ())) {
		return -1;
	}

	new->idx = idx;
	new->data = data;
	new->time_out_handler = handler;
	new->flags = TIMER_FOREVER;

	*(TIMER_T **)p = new;

	return 0;
}

int mod_timer (void *timer, unsigned int tick)
{
	TIMER_T  *p = (TIMER_T *)timer;
	int cpu = 0;

	if (!p)
		return -1;

	if (p->is_running)
		return -1;
	
	cpu = get_cpu ();

	p->exp = (tmr_mgr + cpu)->ticks  + tick;
	p->time =  tick;

	timer_lock (cpu);

	timer_add (p, cpu);

	p->is_running = 1;

	timer_unlock (cpu);

	return 0;
}

static int alloc_timer_id (void)
{
	return ++indx;
}

int stop_timer (void *timer)
{
	TIMER_T  *p = (TIMER_T *)timer;
	int cpu = get_cpu ();

	timer_lock (cpu);

	if (p && p->is_running) {
		p->is_running = 0;
		list_del (&p->next);
	}
	timer_unlock (cpu);
	return 0;
}

int del_timer (void *timer)
{
	stop_timer (timer);
	return 0;
}

static inline TIMER_T * alloc_timer (void)
{
	return tm_calloc (1, sizeof(TIMER_T));
}

static void free_timer (TIMER_T *p) 
{
	tm_free (p, sizeof(*p));
}

static void update_times (int cpu)
{
	timer_lock (cpu);

	(tmr_mgr + cpu)->ticks++;

     	tm_process_tick_and_update_timers (cpu);

	timer_unlock (cpu);
}

unsigned int sys_now (void)
{
	return get_ticks ();
}
unsigned int get_secs (void)
{
	return get_ticks () / 100;
}

static unsigned int get_ticks (void)
{
	return (tmr_mgr + get_cpu ())->ticks ;
}

static unsigned int get_mins (void)
{
	return get_secs() / 60;
}
static unsigned int get_hrs (void)
{
	return get_mins () / (24);
}

void show_uptime (void)
{
	printf ("Uptime  %d hrs %d mins %d secs %d ticks\n",get_hrs(), 
		 get_mins() % 60, get_secs() % 60, get_ticks() % tm_get_ticks_per_second ());
}

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
	int                 cpu = (int) unused;
	
	cpu_bind (cpu);

	timer_lock_create (cpu);

	INIT_LIST_HEAD (&(tmr_mgr + cpu)->timers_list);

	for (;;) {

		start = times (NULL);
		tsk_delay (0, MILLISEC_2_NANOSEC (TICK_TIMER_GRANULARITY));
		end = times (NULL);
		
		tick = end - start;

		if (tick <= 0)
			tick = 1;

		while (tick--) {
			update_times (cpu); 
		}
	}
	return NULL;
}


int init_timer_mgr (void)
{
	tmtaskid_t task_id = 0;
	long      max_cpus = get_max_cpus ();
	int       i = 0;

	tmr_mgr = tm_malloc (sizeof (struct timer_mgr) * max_cpus);

	while (i < max_cpus) {
		char task_name[8];
		sprintf (task_name, "TMR-%d", i);
		if (task_create (task_name, 99, TSK_SCHED_RR, 32000,
				tick_clock, NULL, (void *)i, &task_id) == TSK_FAILURE) {
			return FAILURE;
		}
		i++;
	}

	return SUCCESS;
}



static int timer_restart  (TIMER_T *p)
{
	int cpu = get_cpu ();

	timer_lock (cpu);

	p->exp = p->time + (tmr_mgr + cpu)->ticks ;
	
	timer_add (p, cpu);

	p->is_running = 1;

	timer_unlock (cpu);

	return 0;
}


static void handle_expired_timer (TIMER_T *ptmr)
{
	if (ptmr->time_out_handler) {
		ptmr->time_out_handler (ptmr->data);
	}

	if (ptmr->flags & TIMER_ONCE) {
		free_timer (ptmr);
	} 
	else if (ptmr->flags & TIMER_REPEAT) {
		timer_restart (ptmr);
	}
}

static void timer_expiry_action (TIMER_T * ptmr, int cpu)
{
	ptmr->is_running = 0;
	timer_unlock (cpu);
	handle_expired_timer (ptmr);
	timer_lock (cpu);
}


static int tm_process_tick_and_update_timers (int cpu)
{
	TIMER_T *p, *n;
	struct list_head *head = &(tmr_mgr + cpu)->timers_list;

	list_for_each_entry_safe(p, n, head, next) {
		int diff = p->exp - (tmr_mgr + cpu)->ticks;
		if (diff <= 0) {
#ifdef TIMER_DBG
			printf ("Delete timer : %d %d %d\n", (tmr_mgr + cpu)->ticks, p->exp, diff);
#endif
			timers_count--;
			list_del (&p->next);
			INIT_LIST_HEAD (&p->next);
			timer_expiry_action (p, cpu);
			continue;
		} 
	}

	return 0;
}
int timer_pending (void *timer)
{
	TIMER_T  *p = (TIMER_T *)timer;
	
        if (!p)
                return 0;

        return p->is_running;
}

unsigned int timer_get_remaining_time (void *timer)
{
	TIMER_T  *p = (TIMER_T *)timer;
	int t = 0;

        if (!p || !p->is_running) {
                return 0;
        }

        t = p->exp - get_ticks();

        if (t < 0) {
                printf ("\nTIMERS : Oopss negative remainiting time %s\n",__FUNCTION__);
                t = 0;
        }
        return t;
}
