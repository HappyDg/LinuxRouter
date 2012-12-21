#ifndef NT_H
#define NT_H
/* 
 *  Description: Network Trouble shooter 
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 */
#include "list.h"


enum GET_IF {
	GET_IF_BY_NAME = 1, 
	GET_IF_BY_IPADDR,
	GET_IF_BY_IFINDEX
};

if_t * get_next_if_info (if_t *p);
int make_if_up (if_t *p);
void display_interface_info (void);
int  read_interfaces (void);
int rtnl_init(void);
int nts_debug (char *fmt, ...);
void set_debug_enable (void);
int resolver_init (void);
int ping_me (struct in_addr );
int check_ns_state (void);
int resolve_hostname (const char *hostname);
int ping_setup (void);
int try_to_resolve_host (void);
if_t * get_if (void *key, uint8_t key_type);


#endif  /* NT_H */
