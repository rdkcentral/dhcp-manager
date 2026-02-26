#ifndef _UDHCPC_MSG_H_
#define _UDHCPC_MSG_H_

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include "dhcpv4_interface.h"
#include "dhcp_lease_monitor_thrd.h"

#define MAX_SEND_THRESHOLD 5

#define DHCP_INTERFACE_NAME "interface"
#define DHCP_IP_ADDRESS "ip"
#define DHCP_SUBNET "subnet"
#define DHCP_SUBNET_MASK "mask"
#define DHCP_ROUTER_GW "router"
#define DHCP_DNS_SERVER "dns"
#define DHCP_UPSTREAMRATE "upstreamrate"
#define DHCP_DOWNSTREAMRATE "downstreamrate"
#define DHCP_TIMEZONE "timezone"
#define DHCP_TIMEOFFSET "timeoffset"
#define DHCP_LEASETIME "lease"
#define DHCP_RENEWL_TIME "renewaltime"
#define DHCP_ACK_OPT58 "opt58"
#define DHCP_ACK_OPT59 "opt59"
#define DHCP_MTA_IPV4 "opt122"
#define DHCP_MTA_IPV6 "opt125"
#define DHCP_BOOT_FILE "bootfile" //Boot file name
#define DHCP_REBINDING_TIME "rebindingtime"
#define DHCP_SERVER_ID "serverid"
#define DHCP_SIPSRV "sipsrv"
#define DHCP_STATIC_ROUTES "staticroutes"
#define DHCP_TFTP_SERVER "siaddr"
#define DHCP_HOST_NAME "hostname"
#define DHCP_DOMAIN_NAME "domain"

#endif
