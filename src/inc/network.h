#ifndef NETWORK_H
#define NETWORK_H

/** @file network.h
 *  @brief Main include file for network stack.

    Copyright 2007-2008 j. Arzi.

    This file is part of SDPOS.

    SDPOS is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SDPOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with SDPOS.  If not, see <http://www.gnu.org/licenses/>.
 **/


#include "common_types.h"
/** A MAC address structure (6 bytes) */
typedef struct mac_struct
{
  u8 v[6];
} 
__attribute__ ((__packed__)) mac_t;

/** An IP address structure (4 bytes) */
typedef union ip_union
{
  u32 val;
  //u16 w[2];
  u8  v[4];
} 
__attribute__ ((__packed__)) ip_t;


#include "ip_hdr.h"
#include "mac.h"

#if (USE_UDP == 1)
#include "udp.h"
#endif
#if (USE_ICMP == 1)
#include "icmp.h"
#endif
#if (USE_DHCP == 1)
#include "dhcp.h"
#endif
#if (USE_TCP == 1)
#include "tcp.h"
#endif
#if (USE_ARP == 1)
#include "arp.h"
#endif
#if(USE_TELNET_SERVER == 1)
#include "telnet.h"
#endif
#if(USE_DNS == 1)
#include "dns.h"
#endif
#ifdef USE_PPP
# include "ppp.h"
#endif
#if(USE_HTTP == 1)
# include "http.h"
#endif


/*******************************************
 ****** Helper functions               *****
 *******************************************/

/** Swap endianness of a 16 bit value */
extern u16 swaps(u16 v);

/** Swap endianness of a 32 bit value */
extern u32 swapl(u32 v);

/** @brief Swap content of 2 byte pointers */
extern void swap_bytes(u8 *b1,u8 *b2);


#endif

