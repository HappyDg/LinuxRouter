/* 
 *  Description: Network Trouble shooter 
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 */


#include "common_types.h"
#include <netinet/in.h>
#define DONT_USE_LWIP 1
#include "ifmgmt.h"
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <mqueue.h>
#include <stdint.h>
#include <getopt.h>
#include <linux/if.h>
#include <net/ethernet.h> /* the L2 protocols */


#include "nt.h"

#define _PATH_PROCNET_DEV "/proc/net/dev"

#define PF_PACKET       17      /* Packet family.  */
#define AF_PACKET PF_PACKET

extern char switch_mac[];
int set_ip_address (uint32_t ifindex, uint32_t ipaddress, uint32_t ipmask);
int fetch_and_update_if_info (if_t *ife);
int route_add_if (unsigned char *ipaddr, unsigned char masklen, if_t *netif);
int read_interfaces (void);
void update_linux_if_map (int port, int ifindex);
int make_if_down (if_t *p);
int make_if_up (if_t *p);
void * if_link_monitor (void *arg);
void send_interface_enable_or_disable (int port , int state);

if_t port_cdb[MAX_PORTS];

static int idx = 0;

int get_max_ports (void)
{
	return idx;
}

int get_max_port (void)
{
	return MAX_PORTS;
}

static int new_port_init (int id)
{
	//interface_init (&port_cdb[id], NULL, NULL);
	port_cdb[id].ifType = 1;
	port_cdb[id].ifMtu = 1500;
	port_cdb[id].ifSpeed = 10;
	port_cdb[id].ifAdminStatus = IF_DOWN;
	port_cdb[id].ifOperStatus = IF_DOWN;
	port_cdb[id].ifLastChange = 0;
	port_cdb[id].ifInOctets = 0;
	port_cdb[id].ifInUcastPkts = 0;
	port_cdb[id].ifInDiscards = 0;
	port_cdb[id].ifInErrors = 0;
	port_cdb[id].ifInUnknownProtos = 0;
	port_cdb[id].ifOutOctets = 0;
	port_cdb[id].ifOutUcastPkts = 0;
	port_cdb[id].ifOutDiscards = 0;
	port_cdb[id].ifOutErrors = 0;
	port_cdb[id].pstp_info = NULL;
	port_cdb[id].flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
	//if_connect_init (&port_cdb[id]);
	return 0;
}


static int create_raw_sock (char *name)
{
	int sd = -1;
        struct ifreq ifr;
	struct sockaddr_ll addr;
	int  fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
		return (-1);

	memset (&addr, 0, sizeof(addr));

	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));

	ioctl(fd, SIOCGIFINDEX, (char *)&ifr);

	close (fd);

	if ((sd =socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
		perror ("SOCKET");
		port_cdb[idx].platform = (void *)-1;
		return -1;
	}

	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = ifr.ifr_ifindex;
	addr.sll_protocol = htons(ETH_P_ALL);	

	if (bind (sd, (struct sockaddr *)&addr, sizeof(struct sockaddr_ll)) < 0)
		perror ("bind");

	port_cdb[idx].platform = (void *)sd;

	return 0;
}

static if_t *add_if_info(char *name)
{
    new_port_init (idx);

    strncpy(IF_DESCR(idx + 1), name, IFNAMSIZ);

    fetch_and_update_if_info (IF_INFO(idx + 1));

    return IF_INFO(idx + 1);
}
static char *get_name(char *name, char *p)
{
    while (isspace(*p))
        p++;
    while (*p) {
        if (isspace(*p))
            break;
        if (*p == ':') {        /* could be an alias */
            char *dot = p, *dotname = name;
            *name++ = *p++;
            while (isdigit(*p))
                *name++ = *p++;
            if (*p != ':') {    /* it wasn't, backup */
                p = dot;
                name = dotname;
            }
            if (*p == '\0')
                return NULL;
            p++;
            break;
        }
        *name++ = *p++;
    }
    *name++ = '\0';
    return p;
}


static int if_readconf(void)
{
	int numreqs = 30;
	struct ifconf ifc;
	struct ifreq *ifr;
	int n, err = -1;
	int   skfd      = socket(AF_INET, SOCK_DGRAM, 0);

	if (skfd < 0)
		return (-1);

	ifc.ifc_buf = NULL;
	for (;;) {
		ifc.ifc_len = sizeof(struct ifreq) * numreqs;
		ifc.ifc_buf = realloc(ifc.ifc_buf, ifc.ifc_len);

		if (ioctl(skfd, SIOCGIFCONF, &ifc) < 0) {
			perror("SIOCGIFCONF");
			goto out;
		}
		if (ifc.ifc_len == (int)(sizeof(struct ifreq) * numreqs)) {
			/* assume it overflowed and try again */
			numreqs += 10;
			continue;
		}
		break;
	}

	ifr = ifc.ifc_req;
	for (n = 0; n < ifc.ifc_len; n += sizeof(struct ifreq), ifr++) {
		add_if_info(ifr->ifr_name);
	}
	err = 0;

out:
	free(ifc.ifc_buf);
	return err;
}

static int if_readlist_proc(char *target)
{
	static int proc_read;
	FILE *fh;
	char buf[512];
	if_t *ife;
	int err;

	if (proc_read)
		return 0;
	if (!target)
		proc_read = 1;

	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh) {
		fprintf(stderr, ("Warning: cannot open %s (%s). Limited output.\n"),
				_PATH_PROCNET_DEV, strerror(errno));
		return if_readconf();
	}
	fgets(buf, sizeof buf, fh); /* eat line */
	fgets(buf, sizeof buf, fh);

	err = 0;
	while (fgets(buf, sizeof buf, fh)) {
		char *s, name[IFNAMSIZ];
		s = get_name(name, buf);
		if (!strncmp (name, "lo", strlen ("lo"))) /*Dont read linux lo interface*/
			continue; 
		ife = add_if_info(name);
		idx++;
		if (target && !strcmp(target,name))
			break;
	}
	if (ferror(fh)) {
		perror(_PATH_PROCNET_DEV);
		err = -1;
		proc_read = 0;
	}

	fclose(fh);
	return err;
}

static int if_readlist(void)
{
	int err = if_readlist_proc (NULL);
	if (err < 0)
		err = if_readconf();
	return err;
}

int fetch_and_update_if_info (if_t *ife)
{
	struct ifreq ifr;
	char *ifname = ife->ifDescr; 
	int  fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
		return (-1);

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(fd, SIOCGIFINDEX, (char *)&ifr) == 0) {
		update_linux_if_map (idx, ifr.ifr_ifindex);
		ife->ifIndex = idx + 1;
	}

	if (ioctl(fd, SIOCGIFMTU, (char *)&ifr) == 0)
		ife->ifMtu = ifr.ifr_mtu;

	if (ioctl(fd, SIOCGIFADDR, (char *)&ifr) == 0) {
		struct ifreq ifr_tmp;
		struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
		struct sockaddr_in *mask = (struct sockaddr_in *)&ifr_tmp.ifr_addr;

		strncpy(ifr_tmp.ifr_name, ifname, sizeof(ifr.ifr_name));

		ife->ip_addr.addr = sin->sin_addr.s_addr;

		if (ioctl(fd, SIOCGIFNETMASK, &ifr_tmp) == 0) {
			ife->netmask.addr = mask->sin_addr.s_addr;
			;
		} 
		//if (!set_ip_address (idx + 1, sin->sin_addr.s_addr,  mask->sin_addr.s_addr))
	//	{
	//		connected_route_add (ife, &sin->sin_addr.s_addr, &mask->sin_addr.s_addr, 0);
	//	}

	}  	

	if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)  {
		ife->ifAdminStatus = (ifr.ifr_flags & IFF_UP)? IF_UP: IF_DOWN;
		ife->ifOperStatus  = (ifr.ifr_flags & IFF_RUNNING)?IF_UP:IF_DOWN;
	} 

	if (ioctl (fd, SIOCGIFHWADDR, &ifr) == 0) {
		//unsigned char  *p = (unsigned char *)ifr.ifr_hwaddr.sa_data;
		int i = 0;
		while (i < 6) {
			ife->ifPhysAddress[i] = switch_mac[i];
			i++;
		}
	}

	close(fd);

	return 0;
}

int read_interfaces (void)
{
	if (if_readlist () < 0)
		return -1;
	return 0;
}

int make_if_up (if_t *p)
{
	struct ifreq ifr;
	int  fd = -1;

	if (p->flags & NETIF_FLAG_LOOPBACK) {
	  p->ifAdminStatus = IF_UP;
	  p->ifOperStatus = IF_UP;
	  return 0;
	}

	fd =  socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return (-1);

	memset (&ifr, 0, sizeof(ifr));

	strncpy(ifr.ifr_name, p->ifDescr, sizeof(ifr.ifr_name));

	ifr.ifr_flags |= IFF_UP;
	ifr.ifr_flags |= IFF_RUNNING;

	/*make the interface UP and Running*/
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
		close(fd);
		return -1;
	}
	/*Read and update the interface states*/
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)  {
		p->ifAdminStatus = (ifr.ifr_flags & IFF_UP)? IF_UP: IF_DOWN;
		p->ifOperStatus  = (ifr.ifr_flags & IFF_RUNNING)?IF_UP:IF_DOWN;
	}

	close(fd);
	return 0;
}

int make_if_down (if_t *p)
{
	struct ifreq ifr;
	int  fd = -1;

	if (p->flags & NETIF_FLAG_LOOPBACK) {
	  p->ifAdminStatus = IF_DOWN;
	  p->ifOperStatus = IF_DOWN;
	  return 0;
	}
	  
	fd =  socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return (-1);

	memset (&ifr, 0, sizeof(ifr));

	strncpy(ifr.ifr_name, p->ifDescr, sizeof(ifr.ifr_name));

	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		close(fd);
		return -1;
	}

	ifr.ifr_flags &= ~ (IFF_UP | IFF_RUNNING);

	/*make the interface Down*/
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
		close(fd);
		return -1;
	}

	/*Read and update the interface states*/
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)  {
		p->ifAdminStatus = (ifr.ifr_flags & IFF_UP)? IF_UP: IF_DOWN;
		p->ifOperStatus  = (ifr.ifr_flags & IFF_RUNNING)?IF_UP:IF_DOWN;
	}

	close(fd);
	return 0;
}

void * if_link_monitor (void *arg UNUSED_PARAM)
{
	int max_ports = get_max_ports ();
	struct ifreq ifr;
	int  fd = -1;
	fd =  socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return NULL;
	}

	memset (&ifr, 0, sizeof(ifr));

	while (1) {
		int i = 0;

		while (i < max_ports) {

			struct interface *p = &port_cdb[i];

			strncpy(ifr.ifr_name, p->ifDescr, sizeof(ifr.ifr_name));

			if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)  {
				int oper = (ifr.ifr_flags & IFF_RUNNING)?IF_UP:IF_DOWN;
				if ((p->ifOperStatus  == oper)) {
					/*No Change*/
					i++;
					continue;
				}
				p->ifOperStatus = oper;
				send_interface_enable_or_disable (i+ 1, oper);
			}
			i++;
		}
		
		sleep (1); /*Check for every one second - Modify in future*/
	}
}
