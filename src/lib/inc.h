
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
#include "rbtree.h"
#include "tmrtypes.h"

#define SUCCESS                 0
#define FAILURE                 1

#define TMR_SERVE_TIMERS 0x1

void  free_timer (TIMER_T *p); 
void  btm_hlf (void);
