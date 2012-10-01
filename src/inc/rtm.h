#ifndef __RTM__H__
#define __RTM__H__
#include <zebra.h>

#include "prefix.h"
#include "table.h"
#include "memory.h"
#include "sockunion.h"
#include "linklist.h"
#include "routemap.h"
#include "rib.h"
#include "vector.h"
/* Connected address structure. */
struct connected
{
  /* Attached interface. */
  struct interface *ifp;

  /* Flags for configuration. */
  u_char conf;
#define ZEBRA_IFC_REAL         (1 << 0)
#define ZEBRA_IFC_CONFIGURED   (1 << 1)
  /*
     The ZEBRA_IFC_REAL flag should be set if and only if this address
     exists in the kernel.
     The ZEBRA_IFC_CONFIGURED flag should be set if and only if this address
     was configured by the user from inside quagga.
   */

  /* Flags for connected address. */
  u_char flags;
#define ZEBRA_IFA_SECONDARY    (1 << 0)
#define ZEBRA_IFA_PEER         (1 << 1)
  /* N.B. the ZEBRA_IFA_PEER flag should be set if and only if
     a peer address has been configured.  If this flag is set,
     the destination field must contain the peer address.  
     Otherwise, if this flag is not set, the destination address
     will either contain a broadcast address or be NULL.
   */

  /* Address of connected network. */
  struct prefix *address;

  /* Peer or Broadcast address, depending on whether ZEBRA_IFA_PEER is set.
     Note: destination may be NULL if ZEBRA_IFA_PEER is not set. */
  struct prefix *destination;

  /* Label for Linux 2.2.X and upper. */
  char *label;
};

/* `zebra' daemon local interface structure. */
struct zebra_if
{
  /* Shutdown configuration. */
  u_char shutdown;

  /* Multicast configuration. */
  u_char multicast;

  /* Router advertise configuration. */
  u_char rtadv_enable;

  /* Installed addresses chains tree. */
  struct route_table *ipv4_subnets;

#ifdef RTADV
  struct rtadvconf rtadv;
#endif /* RTADV */

#ifdef HAVE_IRDP
  struct irdp_interface irdp;
#endif

#ifdef SUNOS_5
  /* the real IFF_UP state of the primary interface.
 *    * need this to differentiate between all interfaces being
 *       * down (but primary still plumbed) and primary having gone
 *          * ~IFF_UP, and all addresses gone.
 *             */
  u_char primary_state;
#endif /* SUNOS_5 */
};


/* Does the destination field contain a peer address? */
#define CONNECTED_PEER(C) CHECK_FLAG((C)->flags, ZEBRA_IFA_PEER)

/* Prefix to insert into the RIB */
#define CONNECTED_PREFIX(C) \
        (CONNECTED_PEER(C) ? (C)->destination : (C)->address)

/* Identifying address.  We guess that if there's a peer address, but the
 *    local address is in the same prefix, then the local address may be unique. */
#define CONNECTED_ID(C) \
        ((CONNECTED_PEER(C) && !prefix_match((C)->destination, (C)->address)) ?\
         (C)->destination : (C)->address)



/* Zebra instance */
struct zebra_t
{
  /* default table */
  int rtm_table_default;

  struct meta_queue *mq;
};

struct zebra_desc_table
{ 
  unsigned int type;
  const char *string;
  char chr;
};

/* Zebra route's types. */
#define ZEBRA_ROUTE_SYSTEM               0
#define ZEBRA_ROUTE_KERNEL               1
#define ZEBRA_ROUTE_CONNECT              2
#define ZEBRA_ROUTE_STATIC               3
#define ZEBRA_ROUTE_RIP                  4
#define ZEBRA_ROUTE_RIPNG                5
#define ZEBRA_ROUTE_OSPF                 6
#define ZEBRA_ROUTE_OSPF6                7
#define ZEBRA_ROUTE_ISIS                 8
#define ZEBRA_ROUTE_BGP                  9
#define ZEBRA_ROUTE_HSLS                 10
#define ZEBRA_ROUTE_OLSR                 11
#define ZEBRA_ROUTE_MAX                  12



#define DESC_ENTRY(T,S,C) [(T)] = { (T), (S), (C) }
static const struct zebra_desc_table route_types[] = {
  DESC_ENTRY    (ZEBRA_ROUTE_SYSTEM,     "system",      'X' ),
  DESC_ENTRY    (ZEBRA_ROUTE_KERNEL,     "kernel",      'K' ),
  DESC_ENTRY    (ZEBRA_ROUTE_CONNECT,    "connected",   'C' ),
  DESC_ENTRY    (ZEBRA_ROUTE_STATIC,     "static",      'S' ),
  DESC_ENTRY    (ZEBRA_ROUTE_RIP,        "rip", 'R' ),
  DESC_ENTRY    (ZEBRA_ROUTE_RIPNG,      "ripng",       'R' ),
  DESC_ENTRY    (ZEBRA_ROUTE_OSPF,       "ospf",        'O' ),
  DESC_ENTRY    (ZEBRA_ROUTE_OSPF6,      "ospf6",       'O' ),
  DESC_ENTRY    (ZEBRA_ROUTE_ISIS,       "isis",        'I' ),
  DESC_ENTRY    (ZEBRA_ROUTE_BGP,        "bgp", 'B' ),
  DESC_ENTRY    (ZEBRA_ROUTE_HSLS,       "hsls",        'H' ),
  DESC_ENTRY    (ZEBRA_ROUTE_OLSR,       "olsr",        'o' ),
};


#define zlog_debug printf
#define zlog_err   printf
int connected_route_add (struct interface *ifp,  uint32_t *addr, uint32_t *mask, int flags);
int connected_route_delete (struct interface *ifp,  uint32_t *addr, uint32_t *mask, int flags);
void show_ip_route (struct route_node *rn, struct rib *rib);
int if_subnet_delete (struct interface *ifp, struct connected *ifc);
void connected_free (struct connected *connected);
int if_subnet_add (struct interface *ifp, struct connected *ifc);

#endif
