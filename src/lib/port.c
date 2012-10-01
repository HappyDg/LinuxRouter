#include "common_types.h"
#include "cli.h"
#include "ifmgmt.h"

int stp_send_event (int event, int port, int vlanid);
int port_init (void);
void send_interface_enable_or_disable (int port , int state);
int read_interfaces (void);
int cli_show_interfaces (int port);
struct interface * if_connect_init (struct interface *ifp);

if_t port_cdb[MAX_PORTS];

int port_init (void)
{
	read_interfaces ();

	return 0;
}

void send_interface_enable_or_disable (int port , int state)
{
	int vlanid = 1;
}

int get_port_oper_state (uint32_t port)
{
	return port_cdb[port - 1].ifOperStatus;
}

int get_port_mac_address (uint32_t port, uint8_t *mac)
{
	memcpy (mac, port_cdb[port-1].ifPhysAddress, 6);
	return 0;
}


