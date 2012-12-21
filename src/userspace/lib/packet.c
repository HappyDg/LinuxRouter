#include "common_types.h"
#include "ifmgmt.h"
#undef PKT_DBG 
#define MAX_MTU 2048

struct linux_if_mapping {
	tmtaskid_t task_id;
	int linux_ifIndex;
};

extern struct linux_if_mapping linux_if_map[];

void * packet_processing_task (void *unused);
int rcv_pkt (int sockid, void *buf);
void process_pkt (void  *pkt, int len, uint16_t port);

void * packet_processing_task (void *unused)
{
	int len = 0;
	int sock_id = -1;
	int ifport = (int)unused;

	sock_id = (int)port_cdb[ifport].platform;

	if (sock_id < 0)
		sleep (sock_id);

	while (1) {
		char *buf = malloc (MAX_MTU);
		len = rcv_pkt (sock_id, buf);
		if (len > 0) {
#ifdef PKT_DBG
			printf (" Index : %d\n", linux_if_map[ifport].linux_ifIndex);
#endif
			process_pkt (buf, len, ifport + 1);
		}
	}
	return 0;
}

int rcv_pkt (int sockid, void *buf)
{
	int len = 0;
#if 0
        struct sockaddr_ll sl;
        socklen_t slen = sizeof sl;

	len = recvfrom (sockid, buf, MAX_MTU, 0, (struct sockaddr *) &sl, &slen);
#ifdef PKT_DBG
        printf("\nReceive Src ifindex %d %02x:%02x:%02x:%02x:%02x:%02x",
               sl.sll_ifindex,
               sl.sll_addr[0], sl.sll_addr[1], sl.sll_addr[2],
               sl.sll_addr[3], sl.sll_addr[4], sl.sll_addr[5]);
#endif

#else

	len = read (sockid, buf, MAX_MTU);
#endif
	return len;
}
