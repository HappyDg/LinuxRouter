/*
 *  Author:
 *  Sasikanth.V        <sasikanth@email.com>
 *
 *  $Id: tsk_main.c,v 1.6 2011/01/16 20:00:18 Sasi Exp $
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include "common_types.h"


tmtaskid_t          curtskid;

void * tsk_wrap (void *ptskarg);

#define istsk_selftsk(ptsk)    (ptsk->task_id  == tsk_selfid ())

/**
 *  task_create   -   creates task
 *  @
 *  @
 */

retval_t task_create (const char tskname[], int tsk_prio, int sched_alg, int stk_size,
		void *(*start_routine) (void *), void (*exit_routine) (void),
		void *arg, tmtaskid_t * rettskid)
{
	retval_t            Ret_val = TSK_FAILURE;
	tmtask_t           ptsk_info;

	fill_tsk_info (tskname, tsk_prio, sched_alg, stk_size,
			start_routine, exit_routine, arg, &ptsk_info);

	Ret_val = init_tsk_attr (&ptsk_info);

	if (Ret_val != TSK_FAILURE)
	{
		Ret_val = start_task (&ptsk_info, rettskid) ;

		if (Ret_val == TSK_FAILURE) {
			deinit_tsk_attr (&ptsk_info);
		}
	}
	return Ret_val;
}


void fill_tsk_info (const char *tskname, int tsk_prio, int sched_alg, int stk_size,
               void *(*start_routine) (void *), void (*exit_routine) (void),
               void *arg, tmtask_t * ptskinfo)
{
    strcpy (ptskinfo->task_name, tskname);
    ptskinfo->prio = tsk_prio;
    ptskinfo->schedalgo = sched_alg;
    ptskinfo->stksze = stk_size;
    ptskinfo->start_routine = start_routine;
    ptskinfo->exit_routine = exit_routine;
    ptskinfo->task_id = NO_TASK_ID;
    ptskinfo->tskarg = arg;
}
