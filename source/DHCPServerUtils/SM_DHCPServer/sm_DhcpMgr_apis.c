#include "sm_DhcpMgr.h"
#include <cjson/cJSON.h>
#include "dhcpmgr_rbus_apis.h"
#include "util.h"

const char* DhcpManagersState_Names[]={
    "DHCPS_STATE_IDLE",
    "DHCPS_STATE_PREPARINGv4",
    "DHCPS_STATE_STARTINGv4",
    "DHCPS_STATE_STOPPINGv4",
    "DHCPS_STATE_PREPARINGv6",
    "DHCPS_STATE_STARTINGv6",
    "DHCPS_STATE_STOPPINGv6"
};

const char *json_input = "{\
  \"num_entries\": 3,\
  \"dhcpPayload\": [\
    {\
      \"bridgeInfo\": {\
        \"networkBridgeType\": 0,\
        \"userBridgeCategory\": 1,\
        \"alias\": \"br-lan\",\
        \"stpEnable\": 1,\
        \"igdEnable\": 1,\
        \"bridgeLifeTime\": -1,\
        \"bridgeName\": \"brlan0\"\
      },\
      \"dhcpConfig\": {\
        \"dhcpv4Config\": {\
          \"Dhcpv4_Enable\": true,\
          \"Dhcpv4_Start_Addr\": \"192.168.1.2\",\
          \"Dhcpv4_End_Addr\": \"192.168.1.254\",\
          \"Dhcpv4_Lease_Time\": 86400\
        },\
        \"dhcpv6Config\": {\
          \"Ipv6Prefix\": \"fd00:1:1::/64\",\
          \"StateFull\": true,\
          \"StateLess\": false,\
          \"Dhcpv6_Start_Addr\": \"fd00:1:1::10\",\
          \"Dhcpv6_End_Addr\": \"fd00:1:1::ffff\",\
          \"addrType\": 1,\
          \"customConfig\": null\
        }\
      }\
    },\
    {\
      \"bridgeInfo\": {\
        \"networkBridgeType\": 0,\
        \"userBridgeCategory\": 3,\
        \"alias\": \"br-hotspot\",\
        \"stpEnable\": 0,\
        \"igdEnable\": 0,\
        \"bridgeLifeTime\": -1,\
        \"bridgeName\": \"brlan1\"\
      },\
      \"dhcpConfig\": {\
        \"dhcpv4Config\": {\
          \"Dhcpv4_Enable\": true,\
          \"Dhcpv4_Start_Addr\": \"10.0.0.2\",\
          \"Dhcpv4_End_Addr\": \"10.0.0.200\",\
          \"Dhcpv4_Lease_Time\": 43200\
        },\
        \"dhcpv6Config\": {\
          \"Ipv6Prefix\": \"2001:db8:100::/64\",\
          \"StateFull\": true,\
          \"StateLess\": false,\
          \"Dhcpv6_Start_Addr\": \"2001:db8:100::20\",\
          \"Dhcpv6_End_Addr\": \"2001:db8:100::200\",\
          \"addrType\": 0,\
          \"customConfig\": null\
        }\
      }\
    },\
    {\
      \"bridgeInfo\": {\
        \"networkBridgeType\": 0,\
        \"userBridgeCategory\": 3,\
        \"alias\": \"br-hotspot\",\
        \"stpEnable\": 0,\
        \"igdEnable\": 0,\
        \"bridgeLifeTime\": -1,\
        \"bridgeName\": \"br1an2\"\
      },\
      \"dhcpConfig\": {\
        \"dhcpv4Config\": {\
          \"Dhcpv4_Enable\": true,\
          \"Dhcpv4_Start_Addr\": \"172.168.0.2\",\
          \"Dhcpv4_End_Addr\": \"172.168.0.200\",\
          \"Dhcpv4_Lease_Time\": 43200\
        },\
        \"dhcpv6Config\": {\
          \"Ipv6Prefix\": \"2001:db8:100::/64\",\
          \"StateFull\": true,\
          \"StateLess\": false,\
          \"Dhcpv6_Start_Addr\": \"2001:db8:100::20\",\
          \"Dhcpv6_End_Addr\": \"2001:db8:100::200\",\
          \"addrType\": 0,\
          \"customConfig\": null\
        }\
      }\
    }\
  ]\
}";
//stub function to simulate fetching LAN configurations

void GetDhcpStateName(DHCPS_State state, char *stateName, size_t nameSize) {
    if (state >= 0 && state < sizeof(DhcpManagersState_Names)/sizeof(DhcpManagersState_Names[0])) {
        snprintf(stateName, nameSize, "%s", DhcpManagersState_Names[state]);
    } else {
        snprintf(stateName, nameSize, "UNKNOWN_STATE");
    }
}
int parseDhcpPayloadJson(const char *json, DhcpPayload *payloads, int *count) {
    DHCPMGR_LOG_INFO("%s:%d parseDhcpPayloadJson Enters \n", __FUNCTION__, __LINE__);
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        DHCPMGR_LOG_ERROR("%s:%d Failed to parse JSON\n", __FUNCTION__, __LINE__);
        return -1;
    }

    DHCPMGR_LOG_INFO("%s:%d JSON parsed successfully\n", __FUNCTION__, __LINE__);
    cJSON *numEntries = cJSON_GetObjectItem(root, "num_entries");
    if (!numEntries || !cJSON_IsNumber(numEntries)) {
        DHCPMGR_LOG_ERROR("%s:%d Invalid or missing num_entries\n", __FUNCTION__, __LINE__);
        cJSON_Delete(root);
        return -1;
    }
    *count = numEntries->valueint;
    DHCPMGR_LOG_INFO("%s:%d Number of entries: %d\n", __FUNCTION__, __LINE__, *count);

    cJSON *array = cJSON_GetObjectItem(root, "dhcpPayload");
    if (!array || !cJSON_IsArray(array)) {
        DHCPMGR_LOG_ERROR("%s:%d Invalid or missing dhcpPayload array\n", __FUNCTION__, __LINE__);
        cJSON_Delete(root);
        return -1;
    }
    DHCPMGR_LOG_INFO("%s:%d Retrieved dhcpPayload array\n", __FUNCTION__, __LINE__);

    for (int i = 0; i < *count; i++) {
        DHCPMGR_LOG_INFO("%s:%d Processing entry %d\n", __FUNCTION__, __LINE__, i);
        cJSON *item = cJSON_GetArrayItem(array, i);
        if (!item) {
            DHCPMGR_LOG_ERROR("%s:%d Missing entry at index %d\n", __FUNCTION__, __LINE__, i);
            continue;
        }

        cJSON *bridgeInfo = cJSON_GetObjectItem(item, "bridgeInfo");
        if (bridgeInfo) {
            payloads[i].bridgeInfo.networkBridgeType = cJSON_GetObjectItem(bridgeInfo, "networkBridgeType") ? cJSON_GetObjectItem(bridgeInfo, "networkBridgeType")->valueint : 0;
            payloads[i].bridgeInfo.userBridgeCategory = cJSON_GetObjectItem(bridgeInfo, "userBridgeCategory") ? cJSON_GetObjectItem(bridgeInfo, "userBridgeCategory")->valueint : 0;
            strcpy(payloads[i].bridgeInfo.alias, cJSON_GetObjectItem(bridgeInfo, "alias") ? cJSON_GetObjectItem(bridgeInfo, "alias")->valuestring : "");
            payloads[i].bridgeInfo.stpEnable = cJSON_GetObjectItem(bridgeInfo, "stpEnable") ? cJSON_GetObjectItem(bridgeInfo, "stpEnable")->valueint : 0;
            payloads[i].bridgeInfo.igdEnable = cJSON_GetObjectItem(bridgeInfo, "igdEnable") ? cJSON_GetObjectItem(bridgeInfo, "igdEnable")->valueint : 0;
            payloads[i].bridgeInfo.bridgeLifeTime = cJSON_GetObjectItem(bridgeInfo, "bridgeLifeTime") ? cJSON_GetObjectItem(bridgeInfo, "bridgeLifeTime")->valueint : 0;
            strcpy(payloads[i].bridgeInfo.bridgeName, cJSON_GetObjectItem(bridgeInfo, "bridgeName") ? cJSON_GetObjectItem(bridgeInfo, "bridgeName")->valuestring : "");
        }

        cJSON *dhcpConfig = cJSON_GetObjectItem(item, "dhcpConfig");
        if (dhcpConfig) {
            cJSON *dhcpv4 = cJSON_GetObjectItem(dhcpConfig, "dhcpv4Config");
            if (dhcpv4) {
                payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_Enable = cJSON_IsTrue(cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Enable"));
                strcpy(payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr, cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Start_Addr") ? cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Start_Addr")->valuestring : "");
                strcpy(payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_End_Addr, cJSON_GetObjectItem(dhcpv4, "Dhcpv4_End_Addr") ? cJSON_GetObjectItem(dhcpv4, "Dhcpv4_End_Addr")->valuestring : "");
                payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_Lease_Time = cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Lease_Time") ? cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Lease_Time")->valueint : 0;
                strcpy(payloads[i].dhcpConfig.dhcpv4Config.Subnet_Mask, cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Subnet") ? cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Subnet")->valuestring : "");
            }

            cJSON *dhcpv6 = cJSON_GetObjectItem(dhcpConfig, "dhcpv6Config");
            if (dhcpv6) {
                strcpy(payloads[i].dhcpConfig.dhcpv6Config.Ipv6Prefix, cJSON_GetObjectItem(dhcpv6, "Ipv6Prefix") ? cJSON_GetObjectItem(dhcpv6, "Ipv6Prefix")->valuestring : "");
                payloads[i].dhcpConfig.dhcpv6Config.StateFull = cJSON_IsTrue(cJSON_GetObjectItem(dhcpv6, "StateFull"));
                payloads[i].dhcpConfig.dhcpv6Config.StateLess = cJSON_IsTrue(cJSON_GetObjectItem(dhcpv6, "StateLess"));
                strcpy(payloads[i].dhcpConfig.dhcpv6Config.Dhcpv6_Start_Addr, cJSON_GetObjectItem(dhcpv6, "Dhcpv6_Start_Addr") ? cJSON_GetObjectItem(dhcpv6, "Dhcpv6_Start_Addr")->valuestring : "");
                strcpy(payloads[i].dhcpConfig.dhcpv6Config.Dhcpv6_End_Addr, cJSON_GetObjectItem(dhcpv6, "Dhcpv6_End_Addr") ? cJSON_GetObjectItem(dhcpv6, "Dhcpv6_End_Addr")->valuestring : "");
                payloads[i].dhcpConfig.dhcpv6Config.addrType = cJSON_GetObjectItem(dhcpv6, "addrType") ? cJSON_GetObjectItem(dhcpv6, "addrType")->valueint : 0;
                payloads[i].dhcpConfig.dhcpv6Config.LeaseTime = cJSON_GetObjectItem(dhcpv6, "LeaseTime") ? cJSON_GetObjectItem(dhcpv6, "LeaseTime")->valueint : 0;
                payloads[i].dhcpConfig.dhcpv6Config.RenewTime = cJSON_GetObjectItem(dhcpv6, "RenewTime") ? cJSON_GetObjectItem(dhcpv6, "RenewTime")->valueint : 0;  
                payloads[i].dhcpConfig.dhcpv6Config.RebindTime = cJSON_GetObjectItem(dhcpv6, "RebindTime") ? cJSON_GetObjectItem(dhcpv6, "RebindTime")->valueint : 0;
                payloads[i].dhcpConfig.dhcpv6Config.ValidLifeTime = cJSON_GetObjectItem(dhcpv6, "ValidLifeTime") ? cJSON_GetObjectItem(dhcpv6, "ValidLifeTime")->valueint : 0;
                payloads[i].dhcpConfig.dhcpv6Config.PreferredLifeTime = cJSON_GetObjectItem(dhcpv6, "PreferredLifeTime") ? cJSON_GetObjectItem(dhcpv6, "PreferredLifeTime")->valueint : 0;
                payloads[i].dhcpConfig.dhcpv6Config.customConfig = NULL;
            }
        }
    }

    cJSON_Delete(root);
    DHCPMGR_LOG_INFO("%s:%d JSON parsing completed successfully\n", __FUNCTION__, __LINE__);
    return 0;
}

void printLanDHCPConfig(DhcpPayload *lanConfigs, int LanConfig_count) {
    for (int i = 0; i < LanConfig_count; i++) {
        DHCPMGR_LOG_INFO("%s:%d Bridge Name: %s\n", __FUNCTION__, __LINE__, lanConfigs[i].bridgeInfo.bridgeName);
        DHCPMGR_LOG_INFO("%s:%d   DHCPv4 Enabled: %s\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv4Config.Dhcpv4_Enable ? "Yes" : "No");
        if (lanConfigs[i].dhcpConfig.dhcpv4Config.Dhcpv4_Enable) {
            DHCPMGR_LOG_INFO("%s:%d   DHCPv4 Start Address: %s\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr);
            DHCPMGR_LOG_INFO("%s:%d   DHCPv4 End Address: %s\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv4Config.Dhcpv4_End_Addr);
            DHCPMGR_LOG_INFO("%s:%d   DHCPv4 Lease Time: %d seconds\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv4Config.Dhcpv4_Lease_Time);
            DHCPMGR_LOG_INFO("%s:%d   DHCPv4 Subnet Mask: %s\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv4Config.Subnet_Mask);
        }
        DHCPMGR_LOG_INFO("%s:%d   DHCPv6 Prefix: %s\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv6Config.Ipv6Prefix);
        DHCPMGR_LOG_INFO("%s:%d   DHCPv6 Stateful: %s\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv6Config.StateFull ? "Yes" : "No");
        DHCPMGR_LOG_INFO("%s:%d   DHCPv6 Stateless: %s\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv6Config.StateLess ? "Yes" : "No");
        if (lanConfigs[i].dhcpConfig.dhcpv6Config.StateFull) {
            DHCPMGR_LOG_INFO("%s:%d   DHCPv6 Start Address: %s\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv6Config.Dhcpv6_Start_Addr);
            DHCPMGR_LOG_INFO("%s:%d   DHCPv6 End Address: %s\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv6Config.Dhcpv6_End_Addr);
            DHCPMGR_LOG_INFO("%s:%d   DHCPv6 Address Type: %d\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv6Config.LeaseTime);
            DHCPMGR_LOG_INFO("%s:%d   DHCPv6 Renew Time: %d\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv6Config.RenewTime);
            DHCPMGR_LOG_INFO("%s:%d   DHCPv6 Rebind Time: %d\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv6Config.RebindTime);
            DHCPMGR_LOG_INFO("%s:%d   DHCPv6 Valid Life Time: %d\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv6Config.ValidLifeTime);
            DHCPMGR_LOG_INFO("%s:%d   DHCPv6 Preferred Life Time: %d\n", __FUNCTION__, __LINE__, lanConfigs[i].dhcpConfig.dhcpv6Config.PreferredLifeTime);
        }
        DHCPMGR_LOG_INFO("%s:%d \n", __FUNCTION__, __LINE__);
    }
}

int GetLanDHCPConfig(DhcpPayload *lanConfigs, int *LanConfig_count)
{
    //RBUS call for LAN DHCP Config
    const char* payload = NULL;
    rbus_GetLanDHCPConfig(&payload);
    DHCPMGR_LOG_INFO("%s:%d received payload is %s\n", __FUNCTION__, __LINE__, payload);
    parseDhcpPayloadJson(payload, lanConfigs, LanConfig_count);

//    parseDhcpPayloadJson(json_input, lanConfigs, LanConfig_count);
    printLanDHCPConfig(lanConfigs, *LanConfig_count);

    return 0; // Success
}

void Add_inf_to_dhcp_config(DhcpPayload *pLanConfig, int numOfLanConfigs, DhcpInterfaceConfig **ppHeadDhcpIf,int pDhcpIfacesCount)
{
    if (pLanConfig == NULL || ppHeadDhcpIf == NULL || numOfLanConfigs <= 0)
    {
        return;
    }

    for (int i = 0; i < pDhcpIfacesCount; i++)
    {
        ppHeadDhcpIf[i]->bIsDhcpEnabled = pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Enable;
        snprintf(ppHeadDhcpIf[i]->cGatewayName, sizeof(ppHeadDhcpIf[i]->cGatewayName), "%s", pLanConfig[i].bridgeInfo.bridgeName);

        if (pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Enable)
        {
            snprintf(ppHeadDhcpIf[i]->sAddressPool.cStartAddress, sizeof(ppHeadDhcpIf[i]->sAddressPool.cStartAddress), "%s", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr);
            DHCPMGR_LOG_INFO("%s:%d Source Start Address: %s\n", __FUNCTION__, __LINE__, pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr);
            snprintf(ppHeadDhcpIf[i]->sAddressPool.cStartAddress, sizeof(ppHeadDhcpIf[i]->sAddressPool.cStartAddress), "%s", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr);
            DHCPMGR_LOG_INFO("%s:%d Copied Start Address: %s\n", __FUNCTION__, __LINE__, ppHeadDhcpIf[i]->sAddressPool.cStartAddress);

            DHCPMGR_LOG_INFO("%s:%d Source End Address: %s\n", __FUNCTION__, __LINE__, pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_End_Addr);
            snprintf(ppHeadDhcpIf[i]->sAddressPool.cEndAddress, sizeof(ppHeadDhcpIf[i]->sAddressPool.cEndAddress), "%s", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_End_Addr);
            DHCPMGR_LOG_INFO("%s:%d Copied End Address: %s\n", __FUNCTION__, __LINE__, ppHeadDhcpIf[i]->sAddressPool.cEndAddress);

            DHCPMGR_LOG_INFO("%s:%d Source Subnet Mask: %s\n", __FUNCTION__, __LINE__, pLanConfig[i].dhcpConfig.dhcpv4Config.Subnet_Mask);
            snprintf(ppHeadDhcpIf[i]->cSubnetMask, sizeof(ppHeadDhcpIf[i]->cSubnetMask), "%s", pLanConfig[i].dhcpConfig.dhcpv4Config.Subnet_Mask);
            DHCPMGR_LOG_INFO("%s:%d Copied Subnet Mask: %s\n", __FUNCTION__, __LINE__, ppHeadDhcpIf[i]->cSubnetMask);

            DHCPMGR_LOG_INFO("%s:%d Source Lease Duration: %d\n", __FUNCTION__, __LINE__, pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Lease_Time);
            snprintf(ppHeadDhcpIf[i]->cLeaseDuration, sizeof(ppHeadDhcpIf[i]->cLeaseDuration), "%d", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Lease_Time);
            DHCPMGR_LOG_INFO("%s:%d Copied Lease Duration: %s\n", __FUNCTION__, __LINE__, ppHeadDhcpIf[i]->cLeaseDuration);
        }
    }
}

void AllocateDhcpInterfaceConfig(DhcpInterfaceConfig ***ppDhcpCfgs,int LanConfig_count)
{
    *ppDhcpCfgs = (DhcpInterfaceConfig **)malloc(LanConfig_count * sizeof(DhcpInterfaceConfig *));
        if (*ppDhcpCfgs == NULL)
        {
            DHCPMGR_LOG_ERROR("%s:%d Memory allocation for ppDhcpCfgs failed\n", __FUNCTION__, __LINE__);
            return;
        }
        for (int i = 0; i < LanConfig_count; i++)
        {
            (*ppDhcpCfgs)[i] = (DhcpInterfaceConfig *)malloc(sizeof(DhcpInterfaceConfig));
            if ((*ppDhcpCfgs)[i] == NULL)
            {
                DHCPMGR_LOG_ERROR("%s:%d Memory allocation for ppDhcpCfgs[%d] failed\n", __FUNCTION__, __LINE__, i);
                return;
            }
            memset((*ppDhcpCfgs)[i], 0, sizeof(DhcpInterfaceConfig));
        }
}

int Construct_dhcp_configurationv4(char * dhcpOptions, char * dnsonly)
{
    if (dnsonly == NULL)     
    {
        strcpy(dhcpOptions, "-q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C /var/dnsmasq.conf --dhcp-authoritative --stop-dns-rebind --log-facility=/tmp/dnsmasq.log");
    }
    else if(strcmp(dnsonly, "true") == 0)
    {
        strcpy(dhcpOptions, " -P 4096 -C /var/dnsmasq.conf --log-facility=/tmp/dnsmasq.log");
    }
    return 0;
}

int dhcp_server_publish_state(DHCPS_State state)
{
    DHCPMGR_LOG_INFO("%s:%d rbus publish state called with state %d\n", __FUNCTION__, __LINE__, state);
    char stateStr[32] = {0};
    char paramName[20] = {0};

    switch(state)
    {
        case DHCPS_STATE_IDLE:
            snprintf(stateStr, sizeof(stateStr), "Idle");
            snprintf(paramName,sizeof(paramName),"DHCP_server_state");
            break;
        case DHCPS_STATE_PREPARINGv4:
            GetDhcpStateName(state, stateStr, sizeof(stateStr));
            snprintf(paramName,sizeof(paramName),"DHCP_server_v4");
            break;
        case DHCPS_STATE_STARTINGv4:
            GetDhcpStateName(state, stateStr, sizeof(stateStr));
            snprintf(paramName,sizeof(paramName),"DHCP_server_v4");
            break;
        case DHCPS_STATE_STOPPINGv4:
            GetDhcpStateName(state, stateStr, sizeof(stateStr));
            snprintf(paramName,sizeof(paramName),"DHCP_server_v4");
            break;
        case DHCPS_STATE_PREPARINGv6:
            GetDhcpStateName(state, stateStr, sizeof(stateStr));
            snprintf(paramName,sizeof(paramName),"DHCP_server_v6");
            break;
        case DHCPS_STATE_STARTINGv6:
            GetDhcpStateName(state, stateStr, sizeof(stateStr));
            snprintf(paramName,sizeof(paramName),"DHCP_server_v6");
            break;
        case DHCPS_STATE_STOPPINGv6:
            GetDhcpStateName(state, stateStr, sizeof(stateStr));
            snprintf(paramName,sizeof(paramName),"DHCP_server_v6");
            break;
        default:
            snprintf(stateStr, sizeof(stateStr), "Unknown");
            snprintf(paramName,sizeof(paramName),"DHCP_server_state");
            break;
    }

    if (DhcpMgr_Publish_Events(stateStr, paramName, DHCPMGR_SERVER_STATE) != 0)
    {
        DHCPMGR_LOG_ERROR("%s:%d Failed to publish state %s\n", __FUNCTION__, __LINE__, stateStr);
        return -1;
    }

    return 0;
}

int dhcp_server_signal_state_machine_ready()
{
    if (DhcpMgr_Publish_Events("Ready", "DHCP_Server_state", DHCPMGR_SERVER_READY) != 0)
    {
        DHCPMGR_LOG_ERROR("%s:%d Failed to signal state machine ready\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return 0;
}

void dns_only()
{
    DHCPMGR_LOG_INFO("%s:%d Writing dns_only configuration\n", __FUNCTION__, __LINE__);
    int iter=0;
        // Stub to comment the interface and dcprange lines in dhcp conf file
    FILE *dhcpConfFile = fopen("/var/dnsmasq.conf", "r+");
    if (dhcpConfFile == NULL) {
        DHCPMGR_LOG_ERROR("%s:%d Failed to open dhcpd.conf file\n", __FUNCTION__, __LINE__);
        return;
    }
    DHCPMGR_LOG_INFO("%s:%d Opened dhcpd.conf file successfully\n", __FUNCTION__, __LINE__);
    char line[1024];
    FILE *tempFile = fopen("/tmp/dnsmasq.conf.tmp", "w");
    if (tempFile == NULL) {
        DHCPMGR_LOG_ERROR("%s:%d Failed to create temporary file\n", __FUNCTION__, __LINE__);
        fclose(dhcpConfFile);
        return;
    }
    DHCPMGR_LOG_INFO("%s:%d Created temporary file successfully\n", __FUNCTION__, __LINE__);

    while (fgets(line, sizeof(line), dhcpConfFile)) {
        DHCPMGR_LOG_INFO("%s:%d Commented line: %s and iter=%d\n", __FUNCTION__, __LINE__, line, iter);
        if ((strstr(line, "interface")) && iter < 2) {
            DHCPMGR_LOG_INFO("%s:%d interface found %s", __FUNCTION__, __LINE__, line);
            fprintf(tempFile, "#%s", line); // Comment the line
            iter++;
        }
        else if (strstr(line, "dhcp-range") && iter < 3)
        {
            DHCPMGR_LOG_INFO("%s:%d dhcp-range found %s", __FUNCTION__, __LINE__, line);
            fprintf(tempFile, "#%s", line); // Comment the line
        }
        else {
            fprintf(tempFile, "%s", line); // Write the line as is
        }
    }
 /*   if (rename("/var/dnsmasq.conf.tmp", "/var/dnsmasq.conf") != 0) {
        printf("%s:%d Failed to replace dnsmasq.conf file\n", __FUNCTION__, __LINE__);
    } */
    DHCPMGR_LOG_INFO("%s:%d dns_only configuration written successfully\n", __FUNCTION__, __LINE__);
    fclose(dhcpConfFile);
    fclose(tempFile);
    //stub ends
}

int check_ipv6_received(DhcpPayload *lanConfigs, int LanConfig_count, bool *statefull_enabled, bool *stateless_enabled)
{
    IPv6Dhcpv6InterfaceConfig interface;

    memset(&interface, 0, sizeof(IPv6Dhcpv6InterfaceConfig));
    for (int i = 0; i < LanConfig_count; i++) {
      if (strcmp(lanConfigs[i].bridgeInfo.bridgeName, "brlan0") == 0 &&
        lanConfigs[i].dhcpConfig.dhcpv6Config.Ipv6Prefix[0] != '\0') 
      {
        *statefull_enabled = lanConfigs[i].dhcpConfig.dhcpv6Config.StateFull;
        *stateless_enabled = lanConfigs[i].dhcpConfig.dhcpv6Config.StateLess;
        return 0; // brlan0 has DHCPv6 enabled
      }
    }
    return -1; // No interfaces with DHCPv6 enabled
}

int create_dhcpsv6_config(DhcpPayload *lanConfigs, int LanConfig_count, bool statefull_enabled)
{
    if (!statefull_enabled) {
        DHCPMGR_LOG_INFO("%s:%d Stateful DHCPv6 not enabled globally, nothing to do.\n", __FUNCTION__, __LINE__);
        return 0;
    }

    if (lanConfigs == NULL || LanConfig_count <= 0) {
        DHCPMGR_LOG_ERROR("%s:%d Invalid parameters\n", __FUNCTION__, __LINE__);
        return -1;
    }

    IPv6Dhcpv6InterfaceConfig *interfaces = calloc(LanConfig_count, sizeof(IPv6Dhcpv6InterfaceConfig));
    if (!interfaces) {
        DHCPMGR_LOG_ERROR("%s:%d Memory allocation failed\n", __FUNCTION__, __LINE__);
        return -1;
    }

    int configured_count = 0;
    for (int i = 0; i < LanConfig_count; i++) {
        /* Only configure interfaces that have stateful DHCPv6 enabled in the payload */
        if (!lanConfigs[i].dhcpConfig.dhcpv6Config.StateFull)
            continue;

        IPv6Dhcpv6InterfaceConfig *iface = &interfaces[configured_count];
        memset(iface, 0, sizeof(*iface));

        /* Use bridgeName as interface name (fall back to alias if empty) */
        if (lanConfigs[i].bridgeInfo.bridgeName[0] != '\0') {
            snprintf(iface->interface_name, sizeof(iface->interface_name), "%s", lanConfigs[i].bridgeInfo.bridgeName);
        } else {
            snprintf(iface->interface_name, sizeof(iface->interface_name), "%s", lanConfigs[i].bridgeInfo.alias);
        }

        iface->server_type = DHCPV6_SERVER_TYPE_STATEFUL;
        iface->enable_dhcp = true;
        iface->iana_enable = 1;

        /* Extract prefix length from Ipv6Prefix (e.g. "fd00:1:1::/64") */
        char *prefix = lanConfigs[i].dhcpConfig.dhcpv6Config.Ipv6Prefix;
        if (prefix && prefix[0] != '\0') {
            char *slash = strchr(prefix, '/');
            if (slash != NULL) {
                iface->ipv6_prefix_length = atoi(slash + 1);
            } else {
                DHCPMGR_LOG_ERROR("%s:%d Invalid IPv6 prefix for %s, using /64\n", __FUNCTION__, __LINE__, iface->interface_name);
                iface->ipv6_prefix_length = 64;
            }
        } else {
            iface->ipv6_prefix_length = 64;
        }

        /* Address pool */
        snprintf(iface->ipv6_address_pool.startAddress, sizeof(iface->ipv6_address_pool.startAddress),
                 "%s", lanConfigs[i].dhcpConfig.dhcpv6Config.Dhcpv6_Start_Addr);
        snprintf(iface->ipv6_address_pool.endAddress, sizeof(iface->ipv6_address_pool.endAddress),
                 "%s", lanConfigs[i].dhcpConfig.dhcpv6Config.Dhcpv6_End_Addr);

        iface->lease_time = lanConfigs[i].dhcpConfig.dhcpv6Config.LeaseTime;
        iface->renew_time = lanConfigs[i].dhcpConfig.dhcpv6Config.RenewTime;
        iface->rebind_time = lanConfigs[i].dhcpConfig.dhcpv6Config.RebindTime;
        iface->valid_lifetime = lanConfigs[i].dhcpConfig.dhcpv6Config.ValidLifeTime;
        iface->preferred_lifetime = lanConfigs[i].dhcpConfig.dhcpv6Config.PreferredLifeTime;

        iface->log_level = 8;
        iface->num_options = 0;
        iface->rapid_enable = false;

         DHCPMGR_LOG_INFO("%s:%d Configured interface %s (start=%s end=%s prefixlen=%d)\n",
             __FUNCTION__, __LINE__,
             iface->interface_name, iface->ipv6_address_pool.startAddress,
             iface->ipv6_address_pool.endAddress, iface->ipv6_prefix_length);

        configured_count++;
    }

    if (configured_count == 0) {
         DHCPMGR_LOG_INFO("%s:%d No stateful DHCPv6 interfaces found in payload\n", __FUNCTION__, __LINE__);
        free(interfaces);
        return 0;
    }

    IPv6Dhcpv6ServerConfig config;
    memset(&config, 0, sizeof(config));
    config.interfaces = interfaces;
    config.num_interfaces = configured_count;

    int ret = dhcpv6_server_create(&config, DHCPV6_SERVER_TYPE_STATEFUL);
    if (ret != 0) {
        DHCPMGR_LOG_ERROR("%s:%d Failed to create DHCPv6 server configuration (ret=%d)\n", __FUNCTION__, __LINE__, ret);
        free(interfaces);
        return -1;
    }

    free(interfaces);
    return 0;
}