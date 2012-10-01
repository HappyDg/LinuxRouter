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
#include <unistd.h>
#include <pthread.h>

static long max_cpus = 0;

int mcore_init (void)
{
	max_cpus = sysconf (_SC_NPROCESSORS_ONLN);

	if (max_cpus < 0) {
		max_cpus = 1;
	}

	return 0;
}

int get_cpu (void)
{
	return (max_cpus > 1)? sched_getcpu () : 0;
}

long get_max_cpus (void)
{
	return max_cpus;
}

int cpu_bind (int cpu)
{
	int s;
	cpu_set_t cpuset;
	pthread_t thread;

	thread = pthread_self();

	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);

	s = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

	if (s) {
		perror ("pthread_setaffinity_np: ");
		return -1;
	}

#ifdef MCORE_DEBUG
	s = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	if (s) {
		perror ("pthread_getaffinity_np: ");
	}
	if (CPU_ISSET(cpu, &cpuset))
		printf("    CPU %d\n", cpu);
#endif

	return 0;
}
