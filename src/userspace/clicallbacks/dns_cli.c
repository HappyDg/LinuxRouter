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
#include "cparser.h"
#include "cparser_tree.h"
#include "lwip/opt.h"
#include "lwip/dns.h"

struct dns_server_tbl {
        ip_addr_t addr;
        uint32_t status;
};

extern struct dns_server_tbl  dns_servers[];
int set_dns_server (uint32_t addr, int primary);

cparser_result_t cparser_cmd_show_ip_dns(cparser_context_t *context UNUSED_PARAM)
{
#ifdef CONFIG_OPENSWITCH_TCP_IP
	int i = 0;
	cli_printf ("The default DNS domain name is: openswitch\n");
        cli_printf ("%-20s    %-16s\n", "DNS name server", "status");
	cli_printf ("%-20s    %-16s\n","---------------","-------");

	while (i < DNS_MAX_SERVERS) {
		if (dns_servers[i].status) {
			uint8_t  addr[4];
			char  saddr[16];
			uint32_2_ipstring (dns_servers[i].addr.addr, &addr[0]);
			sprintf (saddr, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
			cli_printf ("%-20s    %-16s\n", saddr, !i? "primary":"");
		}
		i++;
	}
#endif
	return CPARSER_OK;

}

cparser_result_t cparser_cmd_config_ip_dns_domain_name_domainname(cparser_context_t *context UNUSED_PARAM, char **domainname_ptr UNUSED_PARAM)
{
	cli_printf ("Not Implemented\n");
	return CPARSER_OK;
}

cparser_result_t cparser_cmd_config_ip_dns_server_address_dnsserver_primary(cparser_context_t *context UNUSED_PARAM, uint32_t *dnsserver_ptr)
{
#ifdef CONFIG_OPENSWITCH_TCP_IP
	if (!set_dns_server (ntohl(*dnsserver_ptr), 1))
		return CPARSER_NOT_OK;
#endif
	return CPARSER_OK;
}

cparser_result_t cparser_cmd_config_ip_dns_server_address_dnsserver(cparser_context_t *context UNUSED_PARAM, uint32_t *dnsserver_ptr)
{
#ifdef CONFIG_OPENSWITCH_TCP_IP
	if (!set_dns_server (ntohl(*dnsserver_ptr), 0))
		return CPARSER_NOT_OK;
#endif
	return CPARSER_OK;
}
