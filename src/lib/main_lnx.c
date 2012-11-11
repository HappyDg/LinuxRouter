#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>      
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdint.h>
#include <getopt.h>
#include <unistd.h>
#define LWIP_COMPAT_SOCKETS 0
#include "common_types.h"
#include "ifmgmt.h"
#include "cli.h"
//#include "lwip/sockets.h"
#include <mqueue.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <net/ethernet.h> /* the L2 protocols */


#define PF_PACKET       17      /* Packet family.  */
#define AF_PACKET PF_PACKET


int parse_cmdline (int argc, char *argv[]);
void * packet_processing_task (void *unused);
void * if_link_monitor (void *unused);
void update_linux_if_map (int port, int ifindex);
int create_raw_sock_for_pkt_capture (void);
void layer3switch_init (void);
int spawn_pkt_processing_task (void);
int mcore_init (void);
int br_init (void);
int lib_init (void);
int mem_init (void);
int rt_sock_create (void);
int start_cli_task (void);
int cli_init (const char *);
int port_init (void);

char switch_mac[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};

void dump_task_info (void);
void execute_system_call (char *arg);
static int32_t  sockid_pkt = 0;

struct linux_if_mapping {
	tmtaskid_t task_id;
	int linux_ifIndex;
}linux_if_map[CONFIG_MAX_PHY_PORTS];


void show_uptime (char *[]);

void execute_system_call (char *arg)
{
	system (arg);
}

int main (int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
	mcore_init ();

        lib_init ();

	mem_init ();

        cli_init ("LinuxRouter");

        port_init ();

	rt_sock_create();

	br_init ();

        start_cli_task ();

	while (1) {
		sleep (-1);
	}
	return 0;
}

int read_port_mac_address (int port, uint8_t *p) 
{
	int i = 0;
	while (i < 6) {
		p[i] = port_cdb[port - 1].ifPhysAddress[i];
		i++;
	}
	return 0;
}

int spawn_pkt_processing_task (void)
{
	tmtaskid_t  taskid = 0;
	char task_name[16];

	if (task_create ("linkmonitor", 98, 3, 32000, if_link_monitor, NULL, NULL, 
		          &taskid) == TSK_FAILURE) {
		printf ("Task creation failed : %s\n", task_name);
		exit (1);
	}

	
#if 0
	while (ifport < get_max_ports ()) {
		sprintf (task_name, "%s-%d", "PKTRX", ifport);
		if (task_create (task_name, 98, 3, 32000, packet_processing_task, NULL, (void *)ifport, 
			          &linux_if_map[ifport].task_id) == TSK_FAILURE) {
			printf ("Task creation failed : %s\n", task_name);
			exit (1);
		}
		ifport++;
	}
#endif
	return 0;
}

int create_raw_sock_for_pkt_capture (void)
{
	if ((sockid_pkt =socket(AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
		perror ("SOCKET");
		return -1;
	}

	return  0;
}

void update_linux_if_map (int port, int ifindex)
{
	linux_if_map[port].linux_ifIndex = ifindex;
}
#if 0
void send_packet (void *buf, uint16_t port, int len)
{
#if 1
	struct sockaddr_ll socket_address;

	memset (&socket_address, 0, sizeof(socket_address));

	socket_address.sll_family   = PF_PACKET;	
	socket_address.sll_protocol = htons(ETH_P_ALL);	
	socket_address.sll_ifindex  = linux_if_map[port].linux_ifIndex;
	socket_address.sll_halen    = ETH_ALEN;		

        if (sendto ((int)port_cdb[port].platform, buf, len, 0,(struct sockaddr *)&socket_address,
                                sizeof(socket_address)) < 0) {
		;
        }
#else
	write ((int)port_cdb[port].platform, buf, len);
#endif
	return;
}
#endif
