#ifndef SM_DHCPMGR_H
#define SM_DHCPMGR_H

#include <stdbool.h>
#include "dhcp_server_v4_apis.h"
#include "dhcp_server_v6_apis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>

#define SM_MQ_NAME "/lan_fsm_queue"
#define MQ_NAME    "/lan_sm_queue"

typedef int (*ActionHandler)(void *payload);

typedef struct {
    int curState;
    int event;
    int nextState;
    ActionHandler action;
} DHCPS_SM_Mapping;

typedef enum {
    EVENT_CONFIGUREv4 = 0,
    EVENT_CONFIG_CHANGEDv4,
    EVENT_CONFIG_SAMEv4,
    EVENT_STARTEDv4,
    EVENT_STOPv4,
    EVENT_STOPPEDv4,

    EVENT_CONFIGUREv6,
    EVENT_CONFIG_CHANGEDv6,
    EVENT_CONFIG_SAMEv6,
    EVENT_STARTEDv6,
    EVENT_STOPv6,
    EVENT_STOPPEDv6
} DhcpManagerEvent;

typedef enum {
    DHCPS_STATE_IDLE = 0,
    DHCPS_STATE_PREPARINGv4,
    DHCPS_STATE_STARTINGv4,
    DHCPS_STATE_STOPPINGv4,
    DHCPS_STATE_PREPARINGv6,
    DHCPS_STATE_STARTINGv6,
    DHCPS_STATE_STOPPINGv6
} DHCPS_State;

typedef enum {
    DM_EVENT_STARTv4,
    DM_EVENT_RESTARTv4,
    DM_EVENT_CONF_CHANGEDv4,
    DM_EVENT_STOPv4,
    DM_EVENT_STARTv6,
    DM_EVENT_RESTARTv6,
    DM_EVENT_CONF_CHANGEDv6,
    DM_EVENT_STOPv6
} DhcpMgr_DispatchEvent;



// STUB Structure
#define MAX_NAME_LEN      16
#define MAX_IFACE_COUNT   16
#define MAX_IP_LEN        32
#define MAX_PREFIX_LEN    32
#define ALIAS_MAX_LEN     64

typedef enum {
    IPV6_ADDR_TYPE_GLOBAL,
    IPV6_ADDR_TYPE_ULA
} IPv6AddrType;

typedef enum NetworkBridgeType
{
    LINUX_BRIDGE,
    OVS_BRIDGE
}NetworkBridgeType;

typedef enum UserBridgeCategory {
    PRIVATE_LAN = 1,
    HOME_SECURITY,
    HOTSPOT_OPEN_2G,
    HOTSPOT_OPEN_5G,
    LOST_N_FOUND,
    HOTSPOT_SECURE_2G,
    HOTSPOT_SECURE_5G,
    HOTSPOT_SECURE_6G,
    MOCA_ISOLATION,
    MESH_BACKHAUL,
    ETH_BACKHAUL,
    MESH_WIFI_BACKHAUL_2G,
    MESH_WIFI_BACKHAUL_5G,
    CONNECTED_BUILDING,
    CONNECTED_BUILDING_2G,
    CONNECTED_BUILDING_5G,
    CONNECTED_BUILDING_6G
   // MESH_ONBOARD,
   // MESH_WIFI_ONBOARD_2G
}UserBridgeCategory;

typedef struct {
    NetworkBridgeType networkBridgeType;
    UserBridgeCategory userBridgeCategory;
    char alias[ALIAS_MAX_LEN] ;
    int stpEnable;        // 1 = enabled, 0 = disabled
    int igdEnable;        // 1 = enabled, 0 = disabled
    int bridgeLifeTime;   // default: DEFAULT_BRIDGE_LIFETIME , in hours
    char bridgeName[MAX_NAME_LEN];
} BridgeInfo;

typedef struct {
    bool Dhcpv4_Enable; // 1 = enabled, 0 = disabled
    char Dhcpv4_Start_Addr[MAX_IP_LEN];
    char Dhcpv4_End_Addr[MAX_IP_LEN];
    char Subnet_Mask[MAX_IP_LEN];
    int  Dhcpv4_Lease_Time; // in seconds
} DHCPV4Config;

typedef struct {
    char Ipv6Prefix[MAX_PREFIX_LEN];     // e.g., 2001:db8::/64 or fd00::/64
    bool StateFull;                      // 1 = stateful DHCPv6 enabled
    bool StateLess;                      // 1 = stateless DHCPv6 enabled
    char Dhcpv6_Start_Addr[MAX_IP_LEN];  // Used only if StateFull is enabled
    char Dhcpv6_End_Addr[MAX_IP_LEN];    // Used only if StateFull is enabled
    IPv6AddrType addrType;               // Global or ULA addresses for clients
    int  LeaseTime;                      // in seconds
    int  RenewTime;                      // in seconds
    int  RebindTime;                     // in seconds
    int  ValidLifeTime;                  // in seconds
    int  PreferredLifeTime;              // in seconds
    int  num_options;                    // Number of custom options
    void *customConfig;                 // Pointer for future custom configurations --> PVD/FQDN etc 
} DHCPV6Config;

typedef struct {
    DHCPV4Config dhcpv4Config;
    DHCPV6Config dhcpv6Config;
} DHCPConfig;

typedef struct {
    BridgeInfo bridgeInfo;
    DHCPConfig dhcpConfig;
} DhcpPayload;

int GetLanDHCPConfig(DhcpPayload *lanConfigs, int *LanConfig_count);
int Construct_dhcp_configurationv4(char *dhcpOptions, char *dnsonly);
void AllocateDhcpInterfaceConfig(DhcpInterfaceConfig ***ppDhcpCfgs, int LanConfig_count);
void Add_inf_to_dhcp_config(DhcpPayload *pLanConfig, int numOfLanConfigs, DhcpInterfaceConfig **ppHeadDhcpIf, int pDhcpIfacesCount);
void dns_only();
int EventHandler_MainFSM(DhcpMgr_DispatchEvent event);
void* FSM_Dispatch_Thread(void* arg);
void *FSMThread(void *arg);
int dhcp_server_publish_state(DHCPS_State state);
int dhcp_server_signal_state_machine_ready(void);
int check_ipv6_received(DhcpPayload *lanConfigs, int LanConfig_count, bool *statefull_enabled, bool *stateless_enabled);
int create_dhcpsv6_config(DhcpPayload *lanConfigs,int LanConfig_count, bool statefull_enabled);
int dhcp_server_init_main();
/* Global MQ descriptors (defined/initialised in main program) */
extern mqd_t mq_dispatch; /* corresponds to MQ_NAME (dispatch queue) */
extern mqd_t mq_fsm;      /* corresponds to SM_MQ_NAME (fsm actions queue) */
// STUB Struct Ends HERE

#endif