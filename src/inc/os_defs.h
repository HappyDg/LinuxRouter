/*
 *  Author:
 *  Sasikanth.V        <sasikanth@email.com>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>

#define  MIN_THREAD_STACK_SIZE   20000  /*should be atleast PTHREAD_STACK_MIN (bits/local_lim.h)*/
#define  MAX_TSKS            100

typedef sem_t               sync_lock_t;
typedef pthread_t           tmtaskid_t;
typedef time_t              tmclktk_t;
typedef pthread_mutex_t     tskmtx_t;
typedef pthread_cond_t      tskcnd_t;
typedef pthread_attr_t      tskattr_t;

