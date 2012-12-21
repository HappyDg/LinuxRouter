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


int pqueue_init (void);

#define MAX_Q_CTRL_BLOCKS    32
#define PKT_RX_ON_SOCK       0x1 

typedef struct _socket_queue_t {
	void            *buf;
	int              len;
	struct  list_head nbuf;
} socket_queue_t;

typedef struct queue_cb 
{
	struct list_head   queue;
	EVT_T              evt;
	int32_t            state;
}queue_cb_t;

static queue_cb_t  pqcb[MAX_Q_CTRL_BLOCKS];

static sync_lock_t  pq_lock;

int pqueue_init (void)
{
	create_sync_lock (&pq_lock);	
	sync_unlock (&pq_lock);

	return 0;
}

unsigned long  pqueue_create (void)
{
	int          i   = 0;

	sync_lock (&pq_lock);

	while (i < MAX_Q_CTRL_BLOCKS) {
		if (!pqcb[i]. state) {
			pqcb[i]. state = 1;
			INIT_LIST_HEAD (&pqcb[i].queue);
			EventInit (&pqcb[i].evt);
			sync_unlock (&pq_lock);
			return (unsigned long) &pqcb[i];
		}
		i++;
	}

	sync_unlock (&pq_lock);
	return 0;
}

int  pqueue_destroy (unsigned long pcb)
{
	queue_cb_t *p = (queue_cb_t *)pcb;

	if (!p)
		return -1;
	sync_lock (&pq_lock);

	p->state = 0;
	EventDeInit (&p->evt);
	sync_unlock (&pq_lock);
	return 0;
}

int  pqueue_valid (unsigned long pcb)
{
	queue_cb_t *p = (queue_cb_t *)pcb;

	if (!p)
		return 0;
	if (!p->state)
		return 0;
	return 1;
}

int queue_packet (unsigned long qblk , uint8_t *buf, int len)
{
	queue_cb_t * p = (queue_cb_t *)qblk;
	socket_queue_t  *qbuf = NULL;

	if (!p || !buf)
		return -1;

	qbuf = malloc (sizeof(socket_queue_t));

	if (!qbuf)
		return -1;

	INIT_LIST_HEAD (&qbuf->nbuf);

	qbuf->buf = buf;
	qbuf->len = len;

	list_add_tail (&qbuf->nbuf, &p->queue);

	EvtSnd (&p->evt, PKT_RX_ON_SOCK);

	return 0;
}

int dequeue_packet (unsigned long qcb, uint8_t **data, size_t datalen, int secs, int nsecs, int flags)
{
	queue_cb_t * p = (queue_cb_t *)qcb;
	socket_queue_t  *qbuf = NULL;
	int        err = -1;

	if (!p || !p->state)
		return -1;

	while (1) {

		int event = 0;
		if (list_empty (&p->queue)) {
			if (flags)
				return -1;
			if (!secs && !nsecs)
				err = EvtRx (&p->evt, &event, PKT_RX_ON_SOCK);
			else
				err = EvtRx_timed_wait (&p->evt, &event, PKT_RX_ON_SOCK, secs, nsecs);
			if (err < 0)
				return err;
			if ((event & PKT_RX_ON_SOCK))
				goto packet;

		}
packet:
		if (!list_empty (&p->queue)) {
			qbuf = list_first_entry (&p->queue, socket_queue_t, nbuf);
			if (!qbuf)
				return -1;
			if (datalen > qbuf->len)
				datalen = qbuf->len;
			*data = qbuf->buf;
			list_del (&qbuf->nbuf);
			free (qbuf);
			break;
		}
	}

	return 0;
}
