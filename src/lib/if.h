/* Interface related header.
   Copyright (C) 1997, 98, 99 Kunihiro Ishiguro

This file is part of GNU Zebra.

GNU Zebra is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2, or (at your
option) any later version.

GNU Zebra is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Zebra; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef _ZEBRA_IF_H
#define _ZEBRA_IF_H

#include "linklist.h"

/*
  Interface name length.

   Linux define value in /usr/include/linux/if.h.
   #define IFNAMSIZ        16

   FreeBSD define value in /usr/include/net/if.h.
   #define IFNAMSIZ        16
*/

#define INTERFACE_NAMSIZ      20
#define INTERFACE_HWADDR_MAX  20

#ifdef HAVE_PROC_NET_DEV
struct if_stats
{
  unsigned long rx_packets;   /* total packets received       */
  unsigned long tx_packets;   /* total packets transmitted    */
  unsigned long rx_bytes;     /* total bytes received         */
  unsigned long tx_bytes;     /* total bytes transmitted      */
  unsigned long rx_errors;    /* bad packets received         */
  unsigned long tx_errors;    /* packet transmit problems     */
  unsigned long rx_dropped;   /* no space in linux buffers    */
  unsigned long tx_dropped;   /* no space available in linux  */
  unsigned long rx_multicast; /* multicast packets received   */
  unsigned long rx_compressed;
  unsigned long tx_compressed;
  unsigned long collisions;

  /* detailed rx_errors: */
  unsigned long rx_length_errors;
  unsigned long rx_over_errors;       /* receiver ring buff overflow  */
  unsigned long rx_crc_errors;        /* recved pkt with crc error    */
  unsigned long rx_frame_errors;      /* recv'd frame alignment error */
  unsigned long rx_fifo_errors;       /* recv'r fifo overrun          */
  unsigned long rx_missed_errors;     /* receiver missed packet     */
  /* detailed tx_errors */
  unsigned long tx_aborted_errors;
  unsigned long tx_carrier_errors;
  unsigned long tx_fifo_errors;
  unsigned long tx_heartbeat_errors;
  unsigned long tx_window_errors;
};
#endif /* HAVE_PROC_NET_DEV */

/* Does the destination field contain a peer address? */
#define CONNECTED_PEER(C) CHECK_FLAG((C)->flags, ZEBRA_IFA_PEER)

/* Prefix to insert into the RIB */
#define CONNECTED_PREFIX(C) \
	(CONNECTED_PEER(C) ? (C)->destination : (C)->address)

/* Identifying address.  We guess that if there's a peer address, but the
   local address is in the same prefix, then the local address may be unique. */
#define CONNECTED_ID(C)	\
	((CONNECTED_PEER(C) && !prefix_match((C)->destination, (C)->address)) ?\
	 (C)->destination : (C)->address)

/* Interface hook sort. */
#define IF_NEW_HOOK   0
#define IF_DELETE_HOOK 1

/* There are some interface flags which are only supported by some
   operating system. */

#ifndef IFF_NOTRAILERS
#define IFF_NOTRAILERS 0x0
#endif /* IFF_NOTRAILERS */
#ifndef IFF_OACTIVE
#define IFF_OACTIVE 0x0
#endif /* IFF_OACTIVE */
#ifndef IFF_SIMPLEX
#define IFF_SIMPLEX 0x0
#endif /* IFF_SIMPLEX */
#ifndef IFF_LINK0
#define IFF_LINK0 0x0
#endif /* IFF_LINK0 */
#ifndef IFF_LINK1
#define IFF_LINK1 0x0
#endif /* IFF_LINK1 */
#ifndef IFF_LINK2
#define IFF_LINK2 0x0
#endif /* IFF_LINK2 */
#ifndef IFF_NOXMIT
#define IFF_NOXMIT 0x0
#endif /* IFF_NOXMIT */
#ifndef IFF_NORTEXCH
#define IFF_NORTEXCH 0x0
#endif /* IFF_NORTEXCH */
#ifndef IFF_IPV4
#define IFF_IPV4 0x0
#endif /* IFF_IPV4 */
#ifndef IFF_IPV6
#define IFF_IPV6 0x0
#endif /* IFF_IPV6 */
#ifndef IFF_VIRTUAL
#define IFF_VIRTUAL 0x0
#endif /* IFF_VIRTUAL */

/* Prototypes. */
extern int if_cmp_func (struct interface *, struct interface *);
extern struct interface *if_create (const char *name, int namelen);
extern struct interface *if_lookup_by_index (unsigned int);
extern struct interface *if_lookup_exact_address (struct in_addr);
extern struct interface *if_lookup_address (struct in_addr);

/* These 2 functions are to be used when the ifname argument is terminated
   by a '\0' character: */
extern struct interface *if_lookup_by_name (const char *ifname);
extern struct interface *if_get_by_name (const char *ifname);

/* For these 2 functions, the namelen argument should be the precise length
   of the ifname string (not counting any optional trailing '\0' character).
   In most cases, strnlen should be used to calculate the namelen value. */
extern struct interface *if_lookup_by_name_len(const char *ifname,
					       size_t namelen);
extern struct interface *if_get_by_name_len(const char *ifname, size_t namelen);


/* Delete the interface, but do not free the structure, and leave it in the
   interface list.  It is often advisable to leave the pseudo interface 
   structure because there may be configuration information attached. */
extern void if_delete_retain (struct interface *);

/* Delete and free the interface structure: calls if_delete_retain and then
   deletes it from the interface list and frees the structure. */
extern void if_delete (struct interface *);

extern int if_is_up (struct interface *);
extern int if_is_running (struct interface *);
extern int if_is_operative (struct interface *);
extern int if_is_loopback (struct interface *);
extern int if_is_broadcast (struct interface *);
extern int if_is_pointopoint (struct interface *);
extern int if_is_multicast (struct interface *);
extern void if_add_hook (int, int (*)(struct interface *));
extern void if_init (void);
extern void if_dump_all (void);
extern const char *if_flag_dump(unsigned long);

/* Please use ifindex2ifname instead of if_indextoname where possible;
   ifindex2ifname uses internal interface info, whereas if_indextoname must
   make a system call. */
extern const char *ifindex2ifname (unsigned int);

/* Please use ifname2ifindex instead of if_nametoindex where possible;
   ifname2ifindex uses internal interface info, whereas if_nametoindex must
   make a system call. */
extern unsigned int ifname2ifindex(const char *ifname);

/* Connected address functions. */
extern struct connected *connected_new (void);
extern void connected_free (struct connected *);
extern void connected_add (struct interface *, struct connected *);
extern struct connected  *connected_add_by_prefix (struct interface *,
                                            struct prefix *,
                                            struct prefix *);
extern struct connected  *connected_delete_by_prefix (struct interface *, 
                                               struct prefix *);
extern struct connected  *connected_lookup_address (struct interface *, 
                                             struct in_addr);

#ifndef HAVE_IF_NAMETOINDEX
extern unsigned int if_nametoindex (const char *);
#endif
#ifndef HAVE_IF_INDEXTONAME
extern char *if_indextoname (unsigned int, char *);
#endif

/* Exported variables. */
extern struct list *iflist;
extern struct cmd_element interface_desc_cmd;
extern struct cmd_element no_interface_desc_cmd;
extern struct cmd_element interface_cmd;
extern struct cmd_element no_interface_cmd;
extern struct cmd_element interface_pseudo_cmd;
extern struct cmd_element no_interface_pseudo_cmd;
extern struct cmd_element show_address_cmd;

#endif /* _ZEBRA_IF_H */
