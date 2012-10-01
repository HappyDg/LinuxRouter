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
#include "cli.h"
#include "ifmgmt.h"
#include "lwip/ip.h"
#include "cparser.h"
#include "cparser_tree.h"
#include "rtm.h"

int ping_me (char *host);

cparser_result_t cparser_cmd_if_ip_address_addr_mask(cparser_context_t *context UNUSED_PARAM,
    uint32_t *addr_ptr,
    uint32_t *mask_ptr)
{
	int port = cli_get_port ();

	*addr_ptr = ntohl (*addr_ptr);
	*mask_ptr = ntohl (*mask_ptr);

	if (!set_ip_address (port, *addr_ptr, *mask_ptr))
	{
		connected_route_add (IF_INFO (port), addr_ptr, mask_ptr, 0);
		return CPARSER_OK;
	}
	return CPARSER_NOT_OK;
}

cparser_result_t cparser_cmd_if_no_ip_address_addr_mask(cparser_context_t *context UNUSED_PARAM,
    uint32_t *addr_ptr,
    uint32_t *mask_ptr)
{
	int port = cli_get_port ();

	*addr_ptr = ntohl (*addr_ptr);
	*mask_ptr = ntohl (*mask_ptr);

	if(!connected_route_delete (IF_INFO (port), addr_ptr, mask_ptr, 0))
		return CPARSER_OK;
  	return CPARSER_NOT_OK;
}


cparser_result_t cparser_cmd_show_ip_interface(cparser_context_t *context UNUSED_PARAM)
{
	int i = 1;
	while (i <= MAX_PORTS) {
		const char *State[2] = {"UP", "DOWN"};
		uint8_t addr[4];
		uint8_t Mask[4];
		if (!port_cdb[i - 1].ifIndex) {
			i++;
			continue;
		}
		uint32_2_ipstring (port_cdb[i-1].ip_addr.addr, &addr[0]);
		if (!addr[0]) {
		  i++;
		  continue;
		}
		cli_printf ("\n%s is administratively %s, line protocol is %s\n", IF_DESCR(i), State[IF_ADMIN_STATUS(i) - 1],
				State[IF_OPER_STATUS(i) - 1]);
		cli_printf  ("Internet address is ");
		cli_printf("%u.%u.%u.%u", addr[0], addr[1],addr[2],addr[3]);
		uint32_2_ipstring (port_cdb[i-1].netmask.addr, &Mask[0]);
		cli_printf  (", subnet mask is ");
		cli_printf("%u.%u.%u.%u\n", Mask[0], Mask[1],Mask[2],Mask[3]);
		i++;
	}	
	return CPARSER_OK;
}

cparser_result_t cparser_cmd_ping_hostaddr(cparser_context_t *context UNUSED_PARAM, uint32_t *hostaddr_ptr)
{
        uint8_t  addr[4];
        char  str[10];

        uint32_2_ipstring (ntohl(*hostaddr_ptr), addr);

        sprintf (str, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);

	ping_me (str);

	return CPARSER_OK;
}

cparser_result_t cparser_cmd_ping_hostname(cparser_context_t *context UNUSED_PARAM, char **hostname_ptr)
{
	ping_me (*hostname_ptr);

	return CPARSER_OK;
}
