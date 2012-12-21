#include "common_types.h"
#include "cli.h"
#include "ifmgmt.h"
#include "cparser.h"
#include "cparser_tree.h"

cparser_result_t cparser_cmd_show_interface(cparser_context_t *context)
{
	int idx = 0;

	const char *state[]  = {"UNKNWN", "UP", "DOWN"};
	const char *type []  = {"UNKNWN", "ETH", "LO"};

	cli_printf (" Port      Name       MTU    Type    Admin    Oper   LastChange\n");
	cli_printf (" ----     -----      -----  ------   ------  -----   ----------\n");
	while (idx < MAX_PORTS) {
		if (!port_cdb[idx].ifIndex) {
			idx++;
			continue;
		}
		cli_printf (" %-3d       %-8s   %-5d   %-6s  %-4s    %-4s        %-4d\n",
		idx+1, port_cdb[idx].ifDescr,
		port_cdb[idx].ifMtu, type[port_cdb[idx].ifType], state[port_cdb[idx].ifAdminStatus],
		state[port_cdb[idx].ifOperStatus], port_cdb[idx].ifLastChange);
		idx++;
	}

	return CPARSER_OK;
}
