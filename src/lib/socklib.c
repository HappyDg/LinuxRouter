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
#include "lwip/sockets.h"
#include "socks.h"

#define MAX_SOCK_LAYER 12

#define SOCK_UDP 1
#define SOCK_TCP 2


struct sock_client {
	int       (*sock_data_cb) (void *data, void *arg);
	void      *arg;
	int       type;
	int       familiy;
	uint32_t  addr;
	int       flags;
	int       datalen;
	union __sock {    
		struct sock_tcp {
			int port;
			int listen;
		}tcp;
		struct sock_udp {
			int       lport;
			int       rport;
			uint32_t  raddr;
		}udp;
	}_sock;
	int       protocol;
};

enum {
	SOCK_CLIENT_BIND  = 0x1,
	SOCK_CLIENT_CONNECT = 0x2,
	SOCK_CLIENT_NEW_THREAD = 0x4,
	SOCK_CLIENT_BLOCKING = 0x8,
};

typedef struct sock_layer {
	sync_lock_t         lock;
	int                 sock_id;
	int 		    index;
	struct sock_client  client;	
}SOCK_T;

static SOCK_T *sock_layer[MAX_SOCK_LAYER];
static sync_lock_t  lock;
static tmtaskid_t sockmgrid;

static fd_set sock_rfds;
static fd_set sock_wfds;
static fd_set sock_efds;
static int maxsockfd;

static int udp_sock_register (struct sock_client *client);
static SOCK_T * alloc_new_sock_layer  (void);
int sock_mgr_init (void);
static void  * sockmgr (void *arg UNUSED_PARAM);
void sock_client_init (struct sock_client *client);
int sock_client_register (struct sock_client *client);
int tcp_sock_register (void);
static int udp_sock_register (struct sock_client *client);
int udp_v4_create (struct sockaddr_in *addr);
int udp_v4_connect (int sock, struct sockaddr_in *addr);
int udp_close (int sock);
void release_sock_layer (SOCK_T *p);

static void sock_process_rfds (fd_set *rfds)
{
	int i = 0;

	while (i < MAX_SOCK_LAYER) {
		if (sock_layer[i]) {
			if (FD_ISSET (sock_layer[i]->sock_id, rfds)) {
				void *buf = malloc (sock_layer[i]->client.datalen);
				read (sock_layer[i]->sock_id, buf, sock_layer[i]->client.datalen); 
				sock_layer[i]->client.sock_data_cb (buf, sock_layer[i]->client.arg); 
				free (buf);
			}
		}
	}
}

static void  * sockmgr (void *arg UNUSED_PARAM)
{
	fd_set rfds;
	fd_set wfds;
	fd_set efds;
	int retval;

	while (1) {
		FD_ZERO (&rfds);
		FD_ZERO (&wfds);
		FD_ZERO (&efds);

		memcpy (&rfds , &sock_rfds, sizeof(rfds));
		memcpy (&wfds , &sock_wfds, sizeof(wfds));
		memcpy (&efds , &sock_efds, sizeof(efds));

		retval = select (maxsockfd + 1,  &rfds, &wfds, &efds, NULL);

		if (retval < 0)
			continue;

		sock_process_rfds (&rfds);

#if 0
		sock_process_wfds (&wfds);

		sock_process_efds (&efds);
#endif
	}
	return NULL;
}

int sock_mgr_init (void)
{
	FD_ZERO(&sock_rfds);
	FD_ZERO(&sock_wfds);
	FD_ZERO(&sock_efds);

	if (task_create ("SockMgr", 90, 3, 20 * 1024, sockmgr, NULL, NULL, 
			 &sockmgrid) == TSK_FAILURE) {
		printf ("Task creation failed : SockMgr\n");
		return -1;
	}
	return 0;
}

void sock_client_init (struct sock_client *client)
{
	if (!client)
		return;
	memset (client, 0, sizeof (*client));
}

int sock_client_register (struct sock_client *client)
{
	SOCK_T  *new;
	int sock = -1;
	int rval = -1;

	if (!client)
		return -1;

	if ((client->familiy < AF_INET) || (client->familiy > 40)) {
		return -1;
	}

	sync_lock (&lock);

	if (client->type == SOCK_UDP) {

		sock = udp_sock_register (client);
		
		if (sock < 0)
			goto unlock;
		new = alloc_new_sock_layer ();

		if (!new) {
			/*TODO: deregister*/
			goto unlock;
		}
		new->sock_id = sock;

		create_sync_lock (&new->lock);

		memcpy (&new->client, client, sizeof (*client));
		
	} else if (client->type == SOCK_TCP) {

	}
unlock:
	sync_unlock (&lock);
	return rval;
}

int tcp_sock_register (void)
{
	return 0;
}

static int udp_sock_register (struct sock_client *client)
{
        int sock = -1;
	struct sockaddr_in addr;

        sock = socket(client->familiy, SOCK_DGRAM, IPPROTO_UDP);

        if (!sock) {
                perror ("Socket : ");
                return -1;
        }

	if (client->flags & SOCK_CLIENT_BIND) {
		memset (&addr, 0, sizeof (addr));
		addr.sin_family = client->familiy;
		addr.sin_port = htons (client->_sock.udp.lport);
		addr.sin_addr.s_addr = client->addr;

		if(bind(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr))) {
			close (sock);
                	perror ("Bind : ");
			return -1;
		}
	}

	if (client->flags & SOCK_CLIENT_CONNECT) {
		memset (&addr, 0, sizeof (addr));

		addr.sin_family = client->familiy;
		addr.sin_port = htons (client->_sock.udp.rport);
		addr.sin_addr.s_addr = client->_sock.udp.raddr;

		if (connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr))) {
			close (sock);
                	perror ("connect : ");
			return -1;
		}
	}

	FD_SET (sock, &sock_rfds);

	return sock;
}

/*Caller must hold the main sock layer lock*/
static SOCK_T * alloc_new_sock_layer  (void)
{
	int i = MAX_SOCK_LAYER;
	
	while (i >= 0) {
		if (!sock_layer[i]) {
			SOCK_T *p = tm_calloc (1, sizeof (SOCK_T));
			if (!p) {
				printf ("Error : Out of memory");
				return NULL;
			}
			sock_layer[i] = p;
			return p;
		}
	}
	return NULL;
}

void release_sock_layer (SOCK_T *p)
{
	if (p) {
		sock_layer[p->index] = NULL;
		tm_free (p, sizeof (*p));
	}
}


int udp_v4_create (struct sockaddr_in *addr)
{
	int sock = -1;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (!sock) {
		perror ("Socket : ");
		return -1;
	}

	if(bind(sock, (struct sockaddr *) addr, sizeof(struct sockaddr))) {
		close (sock);
		return -1;
	}

	return sock;
}

int udp_v4_connect (int sock, struct sockaddr_in *addr)
{
	int rval = -1;
	socklen_t size = sizeof(struct sockaddr);

	if (!getsockname(sock, (struct sockaddr *) addr, &size))
		if (!connect(sock, (struct sockaddr *) addr, sizeof(struct sockaddr)))
			rval = 0;

	return rval;
}

int udp_close (int sock)
{
	close (sock);
	return 0;
}
