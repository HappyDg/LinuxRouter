#ifndef _IFMGMT_H
#define _IFMGMT_H
#include "linklist.h"
struct interface;
#include "lwip/netif.h"
#ifndef DONT_USE_LWIP
#include "lwip/inet.h"
#endif
#define ZEBRA_INTERFACE_ACTIVE     (1 << 0)
#define ZEBRA_INTERFACE_SUB        (1 << 1)
#define ZEBRA_INTERFACE_LINKDETECTION (1 << 2)

/*FIXME Temproary place holder, Already defined in zebra.h  To be removed */
#define CHECK_FLAG(V,F)      ((V) & (F))
#define SET_FLAG(V,F)        (V) |= (F)
#define UNSET_FLAG(V,F)      (V) &= ~(F)

struct interface {
	void      *platform;
	struct stp_port_entry *pstp_info;
	/** This function is called by the network device driver
	 *  to pass a packet up the TCP/IP stack. */
	netif_input_fn input;
	/** This function is called by the IP module when it wants
	 *  to send a packet on the interface. This function typically
	 *  first resolves the hardware address, then sends the packet. */
	netif_output_fn output;
	/** This function is called by the ARP module when it wants
	 *  to send a packet on the interface. This function outputs
	 *  the pbuf as-is on the link medium. */
	netif_linkoutput_fn linkoutput;
#if LWIP_IPV6
	/** This function is called by the IPv6 module when it wants
	 *  to send a packet on the interface. This function typically
	 *  first resolves the hardware address, then sends the packet. */
	netif_output_ip6_fn output_ip6;
#endif /* LWIP_IPV6 */

#if LWIP_NETIF_STATUS_CALLBACK
	/** This function is called when the netif state is set to up or down
	*/
	netif_status_callback_fn status_callback;
#endif /* LWIP_NETIF_STATUS_CALLBACK */
#if LWIP_NETIF_LINK_CALLBACK
	/** This function is called when the netif link is set to up or down
	*/
	netif_status_callback_fn link_callback;
#endif /* LWIP_NETIF_LINK_CALLBACK */
	/** This field can be set by the device driver and could point
	 *  to state information for the device. */
#if LWIP_IPV6
	/** Array of IPv6 addresses for this netif. */
	ip6_addr_t ip6_addr[LWIP_IPV6_NUM_ADDRESSES];
	/** The state of each IPv6 address (Tentative, Preferred, etc).
	 * @see ip6_addr.h */
	u8_t ip6_addr_state[LWIP_IPV6_NUM_ADDRESSES];
#endif /* LWIP_IPV6 */

	void *state;
#if LWIP_DHCP
	/** the DHCP client state information for this netif */
	struct dhcp *dhcp;
#endif /* LWIP_DHCP */
#if LWIP_AUTOIP
	/** the AutoIP client state information for this netif */
	struct autoip *autoip;
#endif
#if LWIP_IPV6_DHCP6
	/** the DHCPv6 client state information for this netif */
	struct dhcp6 *dhcp6;
#endif /* LWIP_IPV6_DHCP6 */
#if LWIP_NETIF_HOSTNAME
	char  hostname[MAX_PORT_NAME];
#endif /* LWIP_NETIF_HOSTNAME */
	/* Connected address list. */
	struct list *connected;

	void *info;
#if LWIP_IGMP
	/** This function could be called to add or delete a entry in the multicast
	  filter table of the ethernet MAC.*/
	netif_igmp_mac_filter_fn igmp_mac_filter;
#endif /* LWIP_IGMP */
#if LWIP_NETIF_HWADDRHINT
	u8_t *addr_hint;
#endif /* LWIP_NETIF_HWADDRHINT */
#if LWIP_IPV6 && LWIP_IPV6_MLD
	/** This function could be called to add or delete an entry in the IPv6 multicast
	  filter table of the ethernet MAC. */
	netif_mld_mac_filter_fn mld_mac_filter;
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
#if ENABLE_LOOPBACK
	/* List of packets to be queued for ourselves. */
	struct pbuf *loop_first;
	struct pbuf *loop_last;
#if LWIP_LOOPBACK_MAX_PBUFS
	u16_t loop_cnt_current;
#endif /* LWIP_LOOPBACK_MAX_PBUFS */
#endif /* ENABLE_LOOPBACK */
	unsigned long in_bytes;
	unsigned long in_packets;
	unsigned long in_mcast_pkts;
	unsigned long in_errors;
	unsigned long in_discards;
	unsigned long in_unknown_protos;

	unsigned long out_bytes;
	unsigned long out_packets;
	unsigned long out_mcast_pkts; 
	unsigned long out_errors;
	unsigned long out_discards;
	unsigned long out_collisions;
	char      ifDescr[MAX_PORT_NAME];          
	int32_t   ifIndex;
	int32_t   ifType;
	int32_t   ifMtu;   
	int32_t   ifSpeed;
	int32_t   ifAdminStatus;
	int32_t   ifOperStatus;
	uint32_t  ifLastChange; 
	uint32_t  ifInOctets; 
	uint32_t  ifInUcastPkts;
	uint32_t  ifInDiscards;    
	uint32_t  ifInErrors;     
	uint32_t  ifInUnknownProtos;
	uint32_t  ifOutOctets;
	uint32_t  ifOutUcastPkts;
	uint32_t  ifOutDiscards;   
	uint32_t  ifOutErrors;    
	uint32_t   metric;
	/** (estimate) link speed */
	u32_t link_speed;
	/** timestamp at last change made (up/down) */
	u32_t ts;
	/** counters */
	u32_t ifinoctets;
	u32_t ifinucastpkts;
	u32_t ifinnucastpkts;
	u32_t ifindiscards;
	u32_t ifoutoctets;
	u32_t ifoutucastpkts;
	u32_t ifoutnucastpkts;
	u32_t ifoutdiscards;
	/** flags (see NETIF_FLAG_ above) */
	u32_t flags;

	/** IP address configuration in network byte order */
	ip_addr_t ip_addr;
	ip_addr_t netmask;
	ip_addr_t gw;
	/** number of bytes used in ifPhysAddress */
	u8_t ifPhysAddress_len;
	/** link level hardware address of this interface */
	u8_t ifPhysAddress[NETIF_MAX_HWADDR_LEN];
	/** number of this interface */
	u8_t num;
#if LWIP_SNMP
	/** link type (from "snmp_ifType" enum from snmp.h) */
	u8_t link_type;
#endif /* LWIP_SNMP */
#if LWIP_IPV6_AUTOCONFIG
	/** is this netif enabled for IPv6 autoconfiguration */
	u8_t ip6_autoconfig_enabled;
#endif /* LWIP_IPV6_AUTOCONFIG */
#if LWIP_IPV6_SEND_ROUTER_SOLICIT
	/** Number of Router Solicitation messages that remain to be sent. */
	u8_t rs_count;
#endif /* LWIP_IPV6_SEND_ROUTER_SOLICIT */
}__attribute__ ((__packed__));

typedef struct interface if_t;

extern if_t port_cdb[];

#define   IF_INFO(port)             &port_cdb[port - 1]
#define   IF_INDEX(port)            port_cdb[port - 1].ifIndex
#define   IF_DESCR(port)            port_cdb[port - 1].ifDescr
#define   IF_TYPE(port)             port_cdb[port - 1].ifType
#define   IF_MTU(port)              port_cdb[port - 1].ifMtu
#define   IF_IFSPEED(port)          port_cdb[port - 1].ifSpeed
#define   IF_ADMIN_STATUS(port)     port_cdb[port - 1].ifAdminStatus
#define   IF_OPER_STATUS(port)      port_cdb[port - 1].ifOperStatus
#define   IF_LAST_CHGE(port)        port_cdb[port - 1].ifLastChange
#define   IF_IN_OCTS(port)          port_cdb[port - 1].ifInOctets
#define   IF_IN_UCAST_PKTS(port)    port_cdb[port - 1].ifInUcastPkts
#define   IF_IN_DISCARDS(port)      port_cdb[port - 1].ifInDiscards
#define   IF_IN_ERRORS(port)        port_cdb[port - 1].ifInErrors
#define   IF_IN_UNKNOWN_PROTO(port) port_cdb[port - 1].ifInUnknownProtos
#define   IF_OUT_OCTS(port)         port_cdb[port - 1].ifOutOctets
#define   IF_OUT_UCAST_PKTS(port)   port_cdb[port - 1].ifOutUcastPkts
#define   IF_OUT_DISCARDS(port)     port_cdb[port - 1].ifOutDiscards
#define   IF_OUT_ERRORS(port)       port_cdb[port - 1].ifOutErrors
#define   IF_STP_STATE(port)        port_cdb[port - 1].pstp_info->state
#define   IF_STP_INFO(port)         port_cdb[port - 1].pstp_info
#define   IF_IP_ADDRESS(port)	    port_cdb[port - 1].ip_addr.addr;
#define   IF_IP_NETMASK(port)	    port_cdb[port - 1].netmask.addr;
#define   IF_IP_GW(port)	    port_cdb[port - 1].gw.addr;


struct interface *if_lookup_by_index(unsigned int index);
const char *ifindex2ifname(unsigned int index);
struct interface *if_lookup_by_name(const char *name);
unsigned int ifname2ifindex(const char *name);
struct interface *if_lookup_exact_address(struct in_addr src);
struct interface *if_lookup_address(struct in_addr src);
struct interface *if_get_by_name(const char *name);
int if_is_running(struct interface *ifp);
int if_is_up(struct interface *ifp);
int if_is_operative(struct interface *ifp);
int if_is_loopback(struct interface *ifp);
int if_is_broadcast(struct interface *ifp);
int if_is_pointopoint(struct interface *ifp);
int if_is_multicast(struct interface *ifp);
struct interface * get_loopback_if (void);
err_t netif_loop_output (struct interface *netif, struct pbuf *p, ip_addr_t * ipaddr);
int set_ip_address (uint32_t ifindex, uint32_t ipaddress, uint32_t ipmask);
void if_set_addr (struct interface *netif, ip_addr_t * ipaddr, ip_addr_t * netmask,
                ip_addr_t * gw);
int connected_route_add (struct interface *ifp,  uint32_t *addr, uint32_t *mask, int flags);
void interface_init (struct interface *netif, void *state, netif_input_fn input);
struct interface * if_connect_init (struct interface *ifp);
err_t if_loopif_init(void);
#endif
