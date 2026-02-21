/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

#include "cosa_apis.h"
#include "cosa_dhcpv6_apis.h"
#include "util.h"
#include "cosa_dhcpv6_internal.h"
#include "plugin_main_apis.h"
#include "autoconf.h"
#include "secure_wrapper.h"
#include "safec_lib_common.h"
#include "cosa_common_util.h"
#include <ccsp_psm_helper.h>
#include <sys/stat.h>
#include "sysevent/sysevent.h"
#include "cosa_apis_util.h"
#include "ifl.h"
#ifdef _ONESTACK_PRODUCT_REQ_
#include <rdkb_feature_mode_gate.h>
#endif
extern void* g_pDslhDmlAgent;
extern ANSC_HANDLE bus_handle;
extern char g_Subsystem[32];
extern int executeCmd(char *cmd);

#define BUFF_LEN_8      8
#define BUFF_LEN_16    16
#define BUFF_LEN_32    32
#define BUFF_LEN_64    64
#define BUFF_LEN_128  128
#define BUFF_LEN_256  256

#if defined _COSA_SIM_

#elif (defined _COSA_INTEL_USG_ARM_) || (defined _COSA_BCM_MIPS_)

#include <net/if.h>
#include <sys/ioctl.h>
#include <ctype.h>
//#include <libgen.h>
#include <utapi.h>
#include <utapi_util.h>
#include "utctx/utctx_api.h"
#include "syscfg/syscfg.h"
#include "cosa_drg_common.h"
//#include "cosa_ip_apis.h"
#include "cosa_common_util.h"
#if (defined(CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION) && defined(_COSA_BCM_MIPS_)) \
     || defined(_ONESTACK_PRODUCT_REQ_)
#include <netinet/in.h>
#endif

#define SYSCFG_FORMAT_DHCP6C "tr_dhcpv6c"
#define CLIENT_DUID_FILE "/var/lib/dibbler/client-duid"
#define SERVER_DUID_FILE "/var/lib/dibbler/server-duid"

//for test
#define CLIENT_BIN     "dibbler-client"
#define SERVER_BIN     "dibbler-server"
//#define SERVER_BIN     "/usr/local/sbin//dibbler-server"

/* Server global variable  Begin*/

#if defined (MULTILAN_FEATURE)
#define IPV6_PREF_MAXLEN               128
#define DHCPV6S_POOL_NUM               64  /* this pool means interface. We just supported such number interfaces. Each interface can include many pools*/
#else
#define DHCPV6S_POOL_NUM               1
#endif
#define DHCPV6S_POOL_OPTION_NUM        16 /* each interface supports this number options */
#if defined (MULTILAN_FEATURE)
#define CMD_BUFF_SIZE                  256
#endif
#define DHCPV6S_NAME                   "dhcpv6s"

#if defined (INTEL_PUMA7)
#define NO_OF_RETRY 90
#if defined(MULTILAN_FEATURE)
#define IPV6_PREF_MAXLEN 128
#endif
#endif

#if defined (_HUB4_PRODUCT_REQ_)
    #define DHCPV6S_SERVER_PID_FILE   "/etc/dibbler/server.pid"
#else
    #define DHCPV6S_SERVER_PID_FILE   "/tmp/dibbler/server.pid"
#endif

#define DHCPVS_DEBUG_PRINT \
DHCPMGR_LOG_INFO("%s %d", __FUNCTION__, __LINE__);

#if defined(RDKB_EXTENDER_ENABLED) || defined(WAN_FAILOVER_SUPPORTED)
typedef enum deviceMode
{
    DEVICE_MODE_ROUTER,
    DEVICE_MODE_EXTENDER
}deviceMode;
#endif

void * dhcpv6s_dbg_thrd(void * in);

int _dibbler_server_operation(char * arg);
void _cosa_dhcpsv6_refresh_config();
int CosaDmlDHCPv6sTriggerRestart(BOOL OnlyTrigger);
//void CosaDmlDhcpv6sRestartOnLanStarted(void * arg);
void CosaDmlDhcpv6sRebootServer();

int CosaDmlDhcpv6s_format_DNSoption( char *option );
int format_dibbler_option(char *option);
int CosaDmlDHCPv6sGetDNS(char* Dns, char* output, int outputLen);
char * CosaDmlDhcpv6sGetStringFromHex(char * hexString);
char * CosaDmlDhcpv6sGetAddressFromString(char * address);

void dibbler_server_start();
void dibbler_server_stop();

enum {
    DHCPV6_SERVER_TYPE_STATEFUL  =1,
    DHCPV6_SERVER_TYPE_STATELESS
};

ULONG g_dhcpv6s_restart_count = 0;

extern BOOL  g_dhcpv6_server;
extern ULONG g_dhcpv6_server_type;

extern BOOL g_lan_ready;
extern BOOL g_dhcpv6_server_prefix_ready;
extern ULONG g_dhcpv6s_refresh_count;

extern ULONG                               uDhcpv6ServerPoolNum;
extern COSA_DML_DHCPSV6_POOL_FULL          sDhcpv6ServerPool[DHCPV6S_POOL_NUM];
extern ULONG                               uDhcpv6ServerPoolOptionNum[DHCPV6S_POOL_NUM];
extern COSA_DML_DHCPSV6_POOL_OPTION        sDhcpv6ServerPoolOption[DHCPV6S_POOL_NUM][DHCPV6S_POOL_OPTION_NUM];

struct DHCP_TAG tagList[] =
{
    {17, "vendor-spec"},
    {21, "sip-domain"},
    {22, "sip-server"},
    {23, "dns-server"},
    {24, "domain"},
    {27, "nis-server"},
    {28, "nis+-server"},
    {29, "nis-domain"},
    {30, "nis+-domain"},
/*    {39, "OPTION_FQDN"},*/
    {31, "ntp-server"},
    {42, "time-zone"}
};

extern COSA_DML_DHCPCV6_RECV * g_recv_options;

#if defined(_CBR_PRODUCT_REQ_) && !defined(_CBR2_PRODUCT_REQ_)
int serv_ipv6_init();
int serv_ipv6_start(void *args);
int serv_ipv6_stop(void *args);
int serv_ipv6_restart(void *args);
#endif

void dhcpv6_server_init();

#define DIBBLER_SERVER_OPERATION         "dibbler_server_operation"
#define DHCPV6S_TRIGGER_RESTART          "dhcpv6s_trigger_restart"

//#define DHCPV6S_RESTART_ONLANSTARTED     "dhcpv6s_restart_onlanstarted"
//#define DHCPV6S_REBOOT_SERVER            "dhcpv6s_reboot_server"
//#define DHCPV6S_REFRESH_CONFIG           "dhcpsv6_refresh_config"

#define DHCPV6S_CALLER_CTX               "dhcpv6_server"

void dhcpv6_server_init()
{

  //ifl_register_event_handler(DIBBLER_SERVER_OPERATION, IFL_EVENT_NOTIFY_TRUE, DHCPV6S_CALLER_CTX, _dibbler_server_operation);
  //ifl_register_event_handler(DHCPV6S_TRIGGER_RESTART, IFL_EVENT_NOTIFY_TRUE, DHCPV6S_CALLER_CTX, CosaDmlDHCPv6sTriggerRestart);

  //ifl_register_event_handler(DHCPV6S_REBOOT_SERVER, 1, DHCPV6S_CALLER_CTX, CosaDmlDhcpv6sRebootServer);
  //ifl_register_event_handler(DHCPV6S_REFRESH_CONFIG, 1, DHCPV6S_CALLER_CTX, _cosa_dhcpsv6_refresh_config);
#if defined(_CBR_PRODUCT_REQ_) && !defined(_CBR2_PRODUCT_REQ_)
  if (IFL_SUCCESS != ifl_init_ctx(DHCPV6S_CALLER_CTX, IFL_CTX_DYNAMIC))
  {
      DHCPMGR_LOG_ERROR("Failed to init ifl ctx for %s", DHCPV6S_CALLER_CTX);
  }
  serv_ipv6_init();
  ifl_register_event_handler("dhcpv6_server-start", IFL_EVENT_NOTIFY_TRUE, DHCPV6S_CALLER_CTX, serv_ipv6_start);
  ifl_register_event_handler("dhcpv6_server-stop", IFL_EVENT_NOTIFY_TRUE, DHCPV6S_CALLER_CTX, serv_ipv6_stop);
  ifl_register_event_handler("dhcpv6_server-restart", IFL_EVENT_NOTIFY_TRUE, DHCPV6S_CALLER_CTX, serv_ipv6_restart);
#endif
  DHCPMGR_LOG_INFO("SERVICE_DHCP6S : Server event registration completed\n");

}

#define DHCPS6V_SERVER_RESTART_FIFO "/tmp/ccsp-dhcpv6-server-restart-fifo.txt"

#if (defined(CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION) && \
     !defined(_CBR_PRODUCT_REQ_) && \
     !defined(_BCI_FEATURE_REQ_)) || \
    defined(_ONESTACK_PRODUCT_REQ_)


#else
/*
//Todo: Adapt for One stack Product 
void CosaDmlDhcpv6sRestartOnLanStarted(void * arg)
{
    UNREFERENCED_PARAMETER(arg);
    DHCPMGR_LOG_WARNING("%s -- lan status is started. \n", __FUNCTION__);
    g_lan_ready = TRUE;

    int val1 = TRUE;
//    int val2 = FALSE;

#if defined(_CBR_PRODUCT_REQ_)
    // TCCBR-3353: dibbler-server is failing because brlan's IP address is "tentative".
    // Add a delay so the IP address has time to become "prefered".
    sleep(2);
#endif

#ifdef RDKB_EXTENDER_ENABLED
    {
        char buf[8] ={0};
        int deviceMode = 0;

        memset(buf,0,sizeof(buf));
        if (0 == syscfg_get(NULL, "Device_Mode", buf, sizeof(buf)))
        {
            deviceMode = atoi(buf);
        }

        DHCPMGR_LOG_WARNING("%s -- %d Device Mode %d \n", __FUNCTION__, __LINE__,deviceMode);
        //dont start dibbler server in extender mode.
        if (deviceMode == DEVICE_MODE_EXTENDER)
        {
//            return 0;
        }
    }

#endif

#if defined(_HUB4_PRODUCT_REQ_)
    g_dhcpv6_server_prefix_ready = TRUE; // To start dibbler server while lan-statues value is 'started'
    // we have to start the dibbler client on Box bootup even without wan-connection
    // This needs to call dibbler-stop action, before dibbler-start
    int val2 = FALSE;
    CosaDmlDHCPv6sTriggerRestart(&val2);
#else
    CosaDmlDHCPv6sTriggerRestart(&val1);
#endif
    //the thread will start dibbler if need

//    return 0;
}

*/
#endif

#define TMP_SERVER_CONF "/tmp/.dibbler_server_conf"
#define SERVER_CONF_LOCATION  "/etc/dibbler/server.conf"

int CosaDmlDHCPv6sTriggerRestart(BOOL OnlyTrigger)
{
    //int* OnlyTrigger;
    //OnlyTrigger = (int *)arg;
    
    DHCPVS_DEBUG_PRINT
  #if (defined(CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION) && !defined(DHCPV6_PREFIX_FIX)) || defined(_ONESTACK_PRODUCT_REQ_).
    UNREFERENCED_PARAMETER(OnlyTrigger);
    ifl_set_event("dhcpv6_server-restart", "");
  #else
    int fd = 0;
    char str[32] = "restart";
    //not restart really.we only need trigger pthread to check whether there is pending action.
    if ( ! OnlyTrigger ) {
        g_dhcpv6s_restart_count++;
    }

    fd= open(DHCPS6V_SERVER_RESTART_FIFO, O_RDWR);

    if (fd < 0)
    {
        DHCPVS_DEBUG_PRINT
        DHCPMGR_LOG_INFO("open dhcpv6 server restart fifo when writing.");
        return 1;
    }
    DHCPMGR_LOG_DEBUG("%s -- %d: Writing %s to DHCPS6V_SERVER_RESTART_FIFO... \n", __FUNCTION__, __LINE__, str);
    write( fd, str, sizeof(str) );
    close(fd);

  #endif
    return 0;
}

/*SKYH4-3227 : variable to check if process is started already*/
#if defined (_HUB4_PRODUCT_REQ_)
    BOOL  g_dhcpv6_server_started = FALSE;
#endif

/*
 *  DHCP Server
 */
int _dibbler_server_operation(char * arg)
{
    //char* Parg;
    //Parg = (char*)arg;

    //char cmd[256] = {0};
    ULONG Index  = 0;
    int fd = 0;

    DHCPMGR_LOG_INFO("%s:%d\n",__FUNCTION__, __LINE__);
    if (!strncmp(arg, "stop", 4))
    {
        /*stop the process only if it is started*/
        #if defined (_HUB4_PRODUCT_REQ_)
            if ( !g_dhcpv6_server_started )
                goto EXIT;
        #endif
        fd = open(DHCPV6S_SERVER_PID_FILE, O_RDONLY);
        if (fd >= 0) {
            DHCPMGR_LOG_INFO("%s:%d stop dibbler.\n",__FUNCTION__, __LINE__);
            //DHCPMGR_LOG_INFO("%s -- %d stop", __LINE__);
            //dibbler stop seems to be hanging intermittently with v_secure_system
            //replacing with system call
            //v_secure_system(SERVER_BIN " stop >/dev/null");
            dibbler_server_stop();
            close(fd);
        }else{
            //this should not happen.
            DHCPMGR_LOG_INFO("%s:%d No PID server is not running.\n",__FUNCTION__, __LINE__);
            //DHCPMGR_LOG_INFO("%s -- %d server is not running. ", __LINE__);
        }
    }
    else if (!strncmp(arg, "start", 5))
    {
        /* We need judge current config file is right or not.
                    * There is not interface enabled. Not start
                    * There is not valid pool. Not start.
                */
        DHCPMGR_LOG_INFO("Dibbler Server Start %s Line (%d)\n", __FUNCTION__, __LINE__);
        if ( !g_dhcpv6_server )
        {
            DHCPMGR_LOG_INFO("%s Line (%d) g_dhcpv6_server %d \n", __FUNCTION__, __LINE__, g_dhcpv6_server);
            goto EXIT;
        }

        for ( Index = 0; Index < uDhcpv6ServerPoolNum; Index++ )
        {
            if ( sDhcpv6ServerPool[Index].Cfg.bEnabled )
                if ( ( !sDhcpv6ServerPool[Index].Info.IANAPrefixes[0] ) && ( !sDhcpv6ServerPool[Index].Info.IAPDPrefixes[0] ) )
                    break;
        }
        if ( Index < uDhcpv6ServerPoolNum )
        {
            DHCPMGR_LOG_INFO("Index %lu \n", Index);
            goto EXIT;
        }
#ifdef FEATURE_RDKB_WAN_MANAGER
        char prefix[64] = {0};
        ifl_get_event("ipv6_prefix", prefix, sizeof(prefix));
        if (strlen(prefix) > 3)
        {
            g_dhcpv6_server_prefix_ready = TRUE;
        }
#endif
        if (g_dhcpv6_server_prefix_ready && g_lan_ready)
        {
            DHCPMGR_LOG_INFO("%s:%d start dibbler %d\n",__FUNCTION__, __LINE__,g_dhcpv6_server);
            //DHCPMGR_LOG_INFO("%s -- %d start %d", __LINE__, g_dhcpv6_server);

            #if defined (_HUB4_PRODUCT_REQ_)
                g_dhcpv6_server_started = TRUE;
            #endif

            //v_secure_system(SERVER_BIN " start");
            dibbler_server_start();
        }
    }
    else if (!strncmp(arg, "restart", 7))
    {
        DHCPMGR_LOG_INFO("%s:%d restart dibbler.\n",__FUNCTION__, __LINE__);
        _dibbler_server_operation("stop");
        _dibbler_server_operation("start");
    }

EXIT:

    return 0;
}


void *
dhcpv6s_dbg_thrd(void * in)
{
    UNREFERENCED_PARAMETER(in);
    int v6_srvr_fifo_file_dscrptr=0;
    char msg[1024] = {0};
    fd_set rfds;
    struct timeval tm;

    if (IFL_SUCCESS != ifl_init_ctx(DHCPV6S_CALLER_CTX, IFL_CTX_STATIC))
    {
        DHCPMGR_LOG_ERROR("Failed to init ifl ctx for %s", DHCPV6S_CALLER_CTX);
    }

    v6_srvr_fifo_file_dscrptr = open(DHCPS6V_SERVER_RESTART_FIFO, O_RDWR);

    if (v6_srvr_fifo_file_dscrptr< 0)
    {
        DHCPMGR_LOG_INFO("open dhcpv6 server restart fifo!!!!!");
        goto EXIT;
    }

    while (1)
    {
        int retCode = 0;
        tm.tv_sec  = 60;
        tm.tv_usec = 0;

        FD_ZERO(&rfds);
        FD_SET(v6_srvr_fifo_file_dscrptr, &rfds);

        retCode = select(v6_srvr_fifo_file_dscrptr+1, &rfds, NULL, NULL, &tm);
        /* When return -1, it's error.
           When return 0, it's timeout
           When return >0, it's the number of valid fds */
        if (retCode < 0) {
            DHCPMGR_LOG_ERROR("dbg_thrd : select returns error " );

            if (errno == EINTR)
                continue;

            DHCPVS_DEBUG_PRINT
            DHCPMGR_LOG_WARNING("%s -- select(): %s", __FUNCTION__, strerror(errno));
            goto EXIT;
        }
        else if(retCode == 0 )
            continue;

        /* We need consume the data.
         * It's possible more than one triggering events are consumed in one time, which is expected.*/
        if (FD_ISSET(v6_srvr_fifo_file_dscrptr, &rfds)) {
            /* This sleep help do two things:
             * When GUI operate too fast, it gurantees more operations combine into one;
             * Not frequent dibbler start/stop. When do two start fast, dibbler will in bad status.
             */
            sleep(3);
            memset(msg, 0, sizeof(msg));
            read(v6_srvr_fifo_file_dscrptr, msg, sizeof(msg));
            DHCPMGR_LOG_DEBUG("%s,%d: Calling CosaDmlDhcpv6sRebootServer... \n", __FUNCTION__, __LINE__);
            CosaDmlDhcpv6sRebootServer();
            continue;
        }
    }

EXIT:
    if(v6_srvr_fifo_file_dscrptr>=0) {
        close(v6_srvr_fifo_file_dscrptr);
    }

    return NULL;
}

/* Dibbler reboot need 3 conditions.
    a get a global prefix.
    b need brlan0 is ready
    c need brlan0 has the link local address(this should be ready when brlan0 is ready)
*/
void CosaDmlDhcpv6sRebootServer()
{

    char event_value[64] = {0};
#ifdef FEATURE_RDKB_WAN_MANAGER
    ifl_get_event("ipv6_prefix", event_value, sizeof(event_value));
    if (strlen(event_value) > 3)
    {
        g_dhcpv6_server_prefix_ready = TRUE;
    }
    memset(event_value,0,sizeof(event_value));
#endif
    ifl_get_event("lan-status", event_value, sizeof(event_value));
    if ( !strncmp(event_value, "started", strlen("started") ) )
    {
        g_lan_ready = TRUE;
    }
    if (!g_dhcpv6_server_prefix_ready || !g_lan_ready)
        return;
#if defined (MULTILAN_FEATURE)
    ifl_set_event("dhcpv6s-restart", "");
#else
    FILE *fp = NULL;
    int fd = 0;
#if !defined(FEATURE_RDKB_CONFIGURABLE_WAN_INTERFACE)
    if (g_dhcpv6s_restart_count) 
#endif
    {
        g_dhcpv6s_restart_count=0;

        //when need stop, it's supposed the configuration file need to be updated.
        DHCPMGR_LOG_DEBUG("%s - Calling _cosa_dhcpsv6_refresh_config...\n",__FUNCTION__);
	_cosa_dhcpsv6_refresh_config();
        DHCPMGR_LOG_INFO("%s - Call _dibbler_server_operation stop\n",__FUNCTION__);
        _dibbler_server_operation("stop");
    }
    fd = open(DHCPV6S_SERVER_PID_FILE, O_RDONLY);
/* dibbler-server process start fix for HUB4 and ADA */
#if defined (_HUB4_PRODUCT_REQ_)
    FILE *fp_bin = NULL;
    char binbuff[64] = {0};
    fp_bin = v_secure_popen("r","ps|grep %s|grep -v grep", SERVER_BIN);
    _get_shell_output(fp_bin, binbuff, sizeof(binbuff));
    if ((fd < 0) || (!strstr(binbuff, SERVER_BIN))) {
#else
    if (fd < 0) {
#endif
        BOOL isBridgeMode = FALSE;
        char out[128] = {0};

/* dibbler-server process start fix for HUB4 and ADA */
#if defined (_HUB4_PRODUCT_REQ_)
        if(fd >= 0)
            close(fd);
#endif
        /* Unchecked return value*/
        if((ANSC_STATUS_SUCCESS == is_usg_in_bridge_mode(&isBridgeMode)) &&
           ( TRUE == isBridgeMode ))
            return;

        //make sure it's not in a bad status
        fp = v_secure_popen("r","busybox ps|grep %s|grep -v grep", SERVER_BIN);
        _get_shell_output(fp, out, sizeof(out));
        if (strstr(out, SERVER_BIN))
        {
            v_secure_system("kill -15 `pidof " SERVER_BIN "`");
            sleep(1);
        }
        DHCPMGR_LOG_INFO("%s - Call _dibbler_server_operation start\n",__FUNCTION__);
        _dibbler_server_operation("start");
    } else{
        close(fd);
    }
#endif
    // refresh lan if we were asked to
#if !defined(FEATURE_RDKB_CONFIGURABLE_WAN_INTERFACE)
    if (g_dhcpv6s_refresh_count)
#endif
    {
        g_dhcpv6s_refresh_count = 0;
        DHCPMGR_LOG_WARNING("%s: DBG calling  gw_lan_refresh\n", __func__);
        v_secure_system("gw_lan_refresh");
    }

    return;
}

#if defined(_XB6_PRODUCT_REQ_) && defined(_COSA_BCM_ARM_)
//static int sysevent_fd_global = 0;
//static token_t sysevent_token_global;
#endif

/*now we have 2 threads to access __cosa_dhcpsv6_refresh_config(), one is the big thread to process datamodel, the other is dhcpv6c_dbg_thrd(void * in),
 add a lock*/
void __cosa_dhcpsv6_refresh_config();
void _cosa_dhcpsv6_refresh_config()
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&mutex);
    __cosa_dhcpsv6_refresh_config();
    pthread_mutex_unlock(&mutex);
}

#ifdef _HUB4_PRODUCT_REQ_
static int getLanUlaInfo(int *ula_enable)
{
    char  *pUla_enable=NULL;

    if (PSM_Get_Record_Value2(bus_handle,g_Subsystem,"dmsb.lanmanagemententry.lanulaenable", NULL, &pUla_enable) != CCSP_SUCCESS )
    {
        return -1;
    }

    if ( strncmp(pUla_enable, "TRUE", 4 ) == 0) {
        *ula_enable = 1;
    }
    else {
        *ula_enable = 0;
    }

    ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(pUla_enable);

    return 0;
}
#endif

#ifdef _COSA_INTEL_USG_ARM_
void __cosa_dhcpsv6_refresh_config()
{
    FILE * fp = fopen(SERVER_CONF_LOCATION, "w+");
    PCHAR pTmp1 = NULL;
    PCHAR pTmp2 = NULL;
    PCHAR pTmp3 = NULL;
    CHAR prefixValue[64] = {0};
    CHAR prefixFullName[64] = {0};
    ULONG Index  = 0;
    ULONG Index2 = 0;
    ULONG Index3 = 0;
    ULONG uSize = 0;
    ULONG preferedTime;
    ULONG validTime;
    int   returnValue = 0;
    BOOL  isInCaptivePortal = FALSE;
    char * saveptr = NULL;
    char *pServerOption = NULL;
    FILE *responsefd=NULL;
    char *networkResponse = "/var/tmp/networkresponse.txt";
    int iresCode = 0;
    char responseCode[10];
    int inWifiCp=0;
    char buf[20]={0};
    ULONG  T1 = 0;
    ULONG  T2 = 0;
    int Index4 = 0;
    struct stat check_ConfigFile;
    errno_t rc = -1;
    if (!fp)
        goto EXIT;

    /*Begin write configuration */
    {
        char buf[12];
        int log_level = 4;
        if (syscfg_get( NULL, "dibbler_log_level", buf, sizeof(buf)) == 0)
        {
            log_level = atoi(buf);
        }
        if (log_level < 1)
        {
            log_level = 1;
        }
        else if (log_level > 8)
        {
            log_level = 4;
        }
        /*
            1 : Emergency (not used - logging will be disabled)
            2 : Alert (not used - logging will be disabled)
            3 : Critical
            4 : Error
            5 : Warning
            6 : Notice
            7 : Info
            8 : Debug
        */
        fprintf(fp, "log-level %d\n", log_level);
    }
    /*
       Enable inactive mode: When server begins operation and it detects that
       required interfaces are not ready, error message is printed and server
       exits. However, if inactive mode is enabled, server sleeps instead and
       wait for required interfaces to become operational.
    */
    fprintf(fp, "inactive-mode\n");

    //strict RFC compliance rfc3315 Section 13
    fprintf(fp, "drop-unicast\n");

    //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
    fprintf(fp, "reconfigure-enabled 1\n");
    if ( g_dhcpv6_server_type != DHCPV6_SERVER_TYPE_STATEFUL )
        fprintf(fp, "stateless\n");
    for ( Index = 0; Index < uDhcpv6ServerPoolNum; Index++ )
    {
        /* We need get interface name according to Interface field*/
        if ( !sDhcpv6ServerPool[Index].Cfg.bEnabled )
            continue;
        fprintf(fp, "iface %s {\n", COSA_DML_DHCPV6_SERVER_IFNAME);
        if (g_dhcpv6_server_type != DHCPV6_SERVER_TYPE_STATEFUL )
            goto OPTIONS;
        if ( sDhcpv6ServerPool[Index].Cfg.RapidEnable ){
            fprintf(fp, "   rapid-commit yes\n");
        }
#ifdef CONFIG_CISCO_DHCP6S_REQUIREMENT_FROM_DPC3825
        if ( sDhcpv6ServerPool[Index].Cfg.UnicastEnable ){
            char globalAddress[64] = {0};
            ifl_get_event("lan_ipaddr_v6", globalAddress, sizeof(globalAddress));
            if ( strlen(globalAddress) > 0 )
                fprintf(fp, "   unicast %s\n", globalAddress);
        }
        fprintf(fp, "   iface-max-lease %d\n", sDhcpv6ServerPool[Index].Cfg.IANAAmount );
#endif
        /* on GUI, we will limit the order to be [1-256]*/
        fprintf(fp, "   preference %d\n", 255); /*256-(sDhcpv6ServerPool[Index].Cfg.Order%257));*/
        /*begin class
                    fc00:1:0:0:4::/80,fc00:1:0:0:5::/80,fc00:1:0:0:6::/80
                */
        if ( sDhcpv6ServerPool[Index].Cfg.IANAEnable && sDhcpv6ServerPool[Index].Info.IANAPrefixes[0] )
        {
            pTmp1 = AnscCloneString((char*)sDhcpv6ServerPool[Index].Info.IANAPrefixes);
            pTmp3 = pTmp1;
            for (pTmp2 = pTmp1; ; pTmp2 = NULL)
            {
                pTmp1 = strtok_r(pTmp2, ",", &saveptr);
                if (pTmp1 == NULL)
                    break;
                /* This pTmp1 is IP.Interface.{i}.IPv6Prefix.{i}., we need ipv6 address(eg:2000:1:0:0:6::/64) according to it*/
                rc = sprintf_s((char*)prefixFullName, sizeof(prefixFullName), "%sPrefix",pTmp1);
                if(rc < EOK)
                {
                    ERR_CHK(rc);
                }
                uSize = sizeof(prefixValue);
                returnValue = g_GetParamValueString(g_pDslhDmlAgent, prefixFullName, prefixValue, &uSize);
                if ( returnValue != 0 )
                {
                    DHCPMGR_LOG_WARNING("_cosa_dhcpsv6_refresh_config -- g_GetParamValueString for iana:%d\n", returnValue);
                }
                fprintf(fp, "   class {\n");
#ifdef CONFIG_CISCO_DHCP6S_REQUIREMENT_FROM_DPC3825
                /*When enable EUI64, the pool prefix value must use xxx/64 format*/
                if ( sDhcpv6ServerPool[Index].Cfg.EUI64Enable){
                    fprintf(fp, "       share 1000\n");
                }
                /*prefix value should be: xxxx/64 */
                fprintf(fp, "       pool %s\n", prefixValue );
#else
                /*prefix value May be: 2001:01:02::/64 or 2001:0001:0001:001::/64
                                    ::/ is necessary.
                                    We need translate them to valid value.
                                 */
                {
                    int i = _ansc_strlen((const char*)prefixValue);
                    int j = 0;
                    while( (prefixValue[i-1] != '/') && ( i > 0 ) )
                        i--;
                    if ( i == 0 ){
                        DHCPMGR_LOG_WARNING("_cosa_dhcpsv6_refresh_config -- iana:%s is error\n", prefixValue);
                    }
                    if ( ( prefixValue[i-2] != ':' ) || ( prefixValue[i-3] != ':' ) ){
                        DHCPMGR_LOG_WARNING("_cosa_dhcpsv6_refresh_config -- iana:%s is error\n", prefixValue);
                    }
                    /* We just delete last '/' here */
                    prefixValue[i-1] = '\0';
                    /* skip '::/'  */
                    i = i-3;
                    while(  i > 0 ) {
                        if ( prefixValue[i-1] == ':' )
                            j++;
                        i--;
                    }
                    /* delete one last ':' becaues there are 4 parts in this prefix*/
                    if ( j == 3 )
                        prefixValue[_ansc_strlen(prefixValue)-1] = '\0';
                }
                fprintf(fp, "       pool %s%s - %s%s\n", prefixValue, sDhcpv6ServerPool[Index].Cfg.PrefixRangeBegin, prefixValue, sDhcpv6ServerPool[Index].Cfg.PrefixRangeEnd );
#endif
                /*we need get two time values */
                {
                                char s_iapd_pretm[32] = {0};
                                char s_iapd_vldtm[32] = {0};
                                ULONG  iapd_pretm, iapd_vldtm =0;
#if defined(FEATURE_RDKB_CONFIGURABLE_WAN_INTERFACE)
                                /* This pTmp1 is IP.Interface.{i}.IPv6Prefix.{i}., Fetch Interface Name from it*/
                                int index, prefixIndex;
                                char dmName[256] ={0};
                                char dmValue[64] ={0};
                                char sysEventName[256] ={0};

                                sscanf (pTmp1,"Device.IP.Interface.%d.IPv6Prefix.%d.", &index, &prefixIndex);

                                rc = sprintf_s((char*)dmName, sizeof(dmName), "Device.IP.Interface.%d.Name",index);
                                if(rc < EOK)
                                {
                                    ERR_CHK(rc);
                                }

                                uSize = sizeof(dmValue);
                                returnValue = g_GetParamValueString(g_pDslhDmlAgent, dmName, dmValue, &uSize);
                                if ( returnValue != 0 )
                                {
                                    DHCPMGR_LOG_WARNING("_cosa_dhcpsv6_refresh_config -- g_GetParamValueString for iana:%d\n", returnValue);
                                }

                                memset( sysEventName, 0, sizeof(sysEventName));
                                snprintf(sysEventName, sizeof(sysEventName), COSA_DML_WANIface_PREF_PRETM_SYSEVENT_NAME, dmValue);
                                ifl_get_event(sysEventName, s_iapd_pretm, sizeof(s_iapd_pretm));

                                memset( sysEventName, 0, sizeof(sysEventName));
                                snprintf(sysEventName, sizeof(sysEventName), COSA_DML_WANIface_PREF_VLDTM_SYSEVENT_NAME, dmValue);
                                ifl_get_event(sysEventName, s_iapd_vldtm, sizeof(s_iapd_vldtm));
#else
                                ifl_get_event(COSA_DML_DHCPV6C_PREF_PRETM_SYSEVENT_NAME, s_iapd_pretm, sizeof(s_iapd_pretm));
                                ifl_get_event(COSA_DML_DHCPV6C_PREF_VLDTM_SYSEVENT_NAME, s_iapd_vldtm, sizeof(s_iapd_vldtm));
#endif
                                sscanf(s_iapd_pretm, "%lu", &iapd_pretm);
                                sscanf(s_iapd_vldtm, "%lu", &iapd_vldtm);
                                if (sDhcpv6ServerPool[Index].Cfg.LeaseTime <= -1 ) {
                                     T1 = T2 = preferedTime = validTime = 0xFFFFFFFF;
                                }else{
                                        T1           = iapd_pretm/2;
                                        T2           = (ULONG)(iapd_pretm*80.0/100);
                                        preferedTime = iapd_pretm;
                                        validTime    = iapd_vldtm;
                                }
                                fprintf(fp, "       T1 %lu\n", T1);
                                fprintf(fp, "       T2 %lu\n", T2);
                                fprintf(fp, "       prefered-lifetime %lu\n", preferedTime);
                                fprintf(fp, "       valid-lifetime %lu\n", validTime);
                }
                fprintf(fp, "   }\n");
                }
            AnscFreeMemory(pTmp3);
       }
OPTIONS:
        /* For options */
        for ( Index2 = 0 ; Index2 < uDhcpv6ServerPoolOptionNum[Index]; Index2++ )
        {
            if ( !sDhcpv6ServerPoolOption[Index][Index2].bEnabled )
                continue;
            for ( Index3 = 0; Index3 < sizeof(tagList)/sizeof(struct DHCP_TAG);Index3++ )
            {
                if (tagList[Index3].tag != (int)sDhcpv6ServerPoolOption[Index][Index2].Tag )
                    continue;
                else
                    break;
            }
            if ( Index3 >= sizeof(tagList)/sizeof(struct DHCP_TAG) )
                continue;
            // During captive portal no need to pass DNS
            // Check the reponse code received from Web Service
            iresCode = 0;
            if((responsefd = fopen(networkResponse, "r")) != NULL)
            {
                if(fgets(responseCode, sizeof(responseCode), responsefd) != NULL)
                {
                    iresCode = atoi(responseCode);
                }
                /* free unused resources before return */
                fclose(responsefd);
                responsefd = NULL;
            }
            // Get value of redirection_flag
            /* Array compared against 0*/
            if(!syscfg_get( NULL, "redirection_flag", buf, sizeof(buf)))
            {
                if ((strncmp(buf,"true",4) == 0) && iresCode == 204)
                {
                        inWifiCp = 1;
                        DHCPMGR_LOG_WARNING(" _cosa_dhcpsv6_refresh_config -- Box is in captive portal mode \n");
                }
                else
                {
                        //By default isInCaptivePortal is false
                        DHCPMGR_LOG_WARNING(" _cosa_dhcpsv6_refresh_config -- Box is not in captive portal mode \n");
                }
            }
#if defined (_XB6_PRODUCT_REQ_)
        char rfCpEnable[6] = {0};
        char rfCpMode[6] = {0};
        int inRfCaptivePortal = 0;
        /* Array compared against 0*/
        if(!syscfg_get(NULL, "enableRFCaptivePortal", rfCpEnable, sizeof(rfCpEnable)))
        {
          if (strncmp(rfCpEnable,"true",4) == 0)
          {
              /* Array compared against 0*/
              if(!syscfg_get(NULL, "rf_captive_portal", rfCpMode,sizeof(rfCpMode)))
              {
                 if (strncmp(rfCpMode,"true",4) == 0)
                 {
                    inRfCaptivePortal = 1;
                    DHCPMGR_LOG_WARNING(" _cosa_dhcpsv6_refresh_config -- Box is in RF captive portal mode \n");
                 }
              }
          }
        }
        if((inWifiCp == 1) || (inRfCaptivePortal == 1))
        {
            isInCaptivePortal = TRUE;
        }
#else
        if(inWifiCp == 1)
           isInCaptivePortal = TRUE;
#endif
            if ( sDhcpv6ServerPoolOption[Index][Index2].PassthroughClient[0] )
            {
                /* this content get from v6 client directly. If there is problem, change to
                                get from data model tree
                            */
#if defined(_XB6_PRODUCT_REQ_) && defined(_COSA_BCM_ARM_)
                char l_cSecWebUI_Enabled[8] = {0};
                syscfg_get(NULL, "SecureWebUI_Enable", l_cSecWebUI_Enabled, sizeof(l_cSecWebUI_Enabled));
                if (!strncmp(l_cSecWebUI_Enabled, "true", 4)) {
                  sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServersEnabled = 1;
                }
                if ( sDhcpv6ServerPoolOption[Index][Index2].Tag == 23 )
                {
                    char dns_str[256] = {0};
                    DHCPMGR_LOG_WARNING("_cosa_dhcpsv6_refresh_config -- Tag is 23 \n");
                                        /* Static DNS Servers */
                                        if( 1 == sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServersEnabled )
                                        {
                                           rc = strcpy_s( dns_str, sizeof(dns_str), (char*)sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServers );
                                           ERR_CHK(rc);
                                           CosaDmlDhcpv6s_format_DNSoption( dns_str );
                                           // Check device is in captive portal mode or not
                                           if( 1 == isInCaptivePortal )
                                           {
                                                  if (!strncmp(l_cSecWebUI_Enabled, "true", 4))
                                                  {
                                                      char static_dns[256] = {0};
                                                      ifl_get_event("lan_ipaddr_v6", static_dns, sizeof(static_dns));
                                                      if ( '\0' != dns_str[ 0 ] )
                                                      {
                                                          if ( '\0' != static_dns[ 0 ] )
                                                          {
                                                              fprintf(fp, "#     option %s %s,%s\n", tagList[Index3].cmdstring, static_dns, dns_str);
                                                          }
                                                          else
                                                          {
                                                              fprintf(fp, "#     option %s %s\n", tagList[Index3].cmdstring, dns_str);
                                                          }
                                                      }
                                                      else
                                                      {
                                                          char dyn_dns[256] = {0};
                                                          ifl_get_event ("wan6_ns", dyn_dns, sizeof(dyn_dns));
                                                          if ( '\0' != dyn_dns[ 0 ] )
                                                          {
                                                              format_dibbler_option(dyn_dns);
                                                              if ( '\0' != static_dns[ 0 ] )
                                                              {
                                                                  fprintf(fp, "#    option %s %s,%s\n", tagList[Index3].cmdstring, static_dns, dyn_dns);
                                                              }
                                                              else
                                                              {
                                                                  fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, dyn_dns);
                                                              }
                                                          }
                                                          else
                                                          {
                                                              if ( '\0' != static_dns[ 0 ] )
                                                              {
                                                                  fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, static_dns);
                                                              }
                                                          }
                                                      }
                                                  }
                                                  else
                                                  {
                                                      fprintf(fp, "#	 option %s %s\n", tagList[Index3].cmdstring, dns_str);
                                                  }
                                           }
                                           else
                                           {
                                                   if (!strncmp(l_cSecWebUI_Enabled, "true", 4))
                                                   {
                                                       char static_dns[256] = {0};
                                                       char dyn_dns[256] = {0};
                                                       ifl_get_event ("wan6_ns", dyn_dns, sizeof(dyn_dns));
                                                       if ( '\0' != dyn_dns[ 0 ] )
                                                       {
                                                           format_dibbler_option(dyn_dns);
                                                       }
                                                       ifl_get_event("lan_ipaddr_v6", static_dns, sizeof(static_dns));
                                                       if ( '\0' != dns_str[ 0 ] )
                                                       {
                                                            if ( '\0' != static_dns[ 0 ] )
                                                            {
                                                                if ( '\0' != dyn_dns[ 0 ] )
                                                                {
                                                                    fprintf(fp, "       option %s %s,%s,%s\n", tagList[Index3].cmdstring, static_dns, dns_str, dyn_dns);
                                                                }
                                                                else
                                                                {
                                                                    fprintf(fp, "       option %s %s,%s\n", tagList[Index3].cmdstring, static_dns, dns_str);
                                                                }
                                                            }
                                                            else
                                                            {
                                                                if ( '\0' != dyn_dns[ 0 ] )
                                                                {
                                                                     fprintf(fp, "      option %s %s,%s\n", tagList[Index3].cmdstring, dyn_dns, dns_str);
                                                                }
                                                                else
                                                                {
                                                                     fprintf(fp, "      option %s %s\n", tagList[Index3].cmdstring, dns_str);
                                                                }
                                                            }
                                                       }
                                                       else
                                                       {
                                                            if ( '\0' != static_dns[ 0 ] )
                                                            {
                                                                if ( '\0' != dyn_dns[ 0 ] )
                                                                {
                                                                    fprintf(fp, "      option %s %s,%s\n", tagList[Index3].cmdstring, static_dns,dyn_dns);
                                                                }
                                                                else
                                                                {
                                                                    fprintf(fp, "      option %s %s\n", tagList[Index3].cmdstring, static_dns);
                                                                }
                                                            }
                                                            else
                                                            {
                                                                if ( '\0' != dyn_dns[ 0 ] )
                                                                {
                                                                    fprintf(fp, "       option %s %s\n", tagList[Index3].cmdstring, dyn_dns);
                                                                }
                                                            }
                                                       }
                                                   }
                                                   else
                                                   {
                                                       if ( '\0' != dns_str[ 0 ] ){
                                                             fprintf(fp, "    option %s %s\n", tagList[Index3].cmdstring, dns_str);
                                                         }
                                                   }
                                            }
                                           DHCPMGR_LOG_WARNING("%s %d - DNSServersEnabled:%d DNSServers:%s\n", __FUNCTION__,
                                                                                                                                                                                 __LINE__,
                                                                                                                                                                                 sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServersEnabled,
                                                                                                                                                                                 sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServers );
                                        }
                                        else
                                        {
                                                ifl_get_event ("wan6_ns", dns_str, sizeof(dns_str));
                                                if (dns_str[0] != '\0') {
                                                        format_dibbler_option(dns_str);
                                                        if( isInCaptivePortal == TRUE )
                                                        {
                                                                fprintf(fp, "#  option %s %s\n", tagList[Index3].cmdstring, dns_str);
                                                        }
                                                        else
                                                        {
                                                                //By default isInCaptivePortal is false
                                                                fprintf(fp, "   option %s %s\n", tagList[Index3].cmdstring, dns_str);
                                                        }
                                                 }
                                        }
                }
                else if (sDhcpv6ServerPoolOption[Index][Index2].Tag == 24)
                {//domain
                    char domain_str[256] = {0};
                        DHCPMGR_LOG_WARNING("_cosa_dhcpsv6_refresh_config -- Tag is 24 \n");
                    ifl_get_event ("ipv6_dnssl", domain_str, sizeof(domain_str));
                    if (domain_str[0] != '\0') {
                        format_dibbler_option(domain_str);
                        if( isInCaptivePortal == TRUE )
                        {
                            fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, domain_str);
                        }
                        else
                        {
                            fprintf(fp, "     option %s %s\n", tagList[Index3].cmdstring, domain_str);
                        }
                    }
                }
#elif defined _HUB4_PRODUCT_REQ_
                 /* Static DNS Servers */
                if ( sDhcpv6ServerPoolOption[Index][Index2].Tag == 23 ) {
                    char dnsServer[ 256 ] = { 0 };
		    int ula_enable = 0;
		    int result = 0;
		    result = getLanUlaInfo(&ula_enable);
		    if(result != 0) {
			    fprintf(stderr, "getLanIpv6Info failed");
			    return;
		    }
                                            if( 1 == sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServersEnabled ) {
                                            //DHCPMGR_LOG_WARNING("Cfg.X_RDKCENTRAL_COM_DNSServersEnabled is 1 \n");
					     /* RDKB-50535 send ULA address as DNS address only when lan ULA is enabled */
					    if (ula_enable)
				            {
                                                rc = strcpy_s( dnsServer, sizeof( dnsServer ), (const char * restrict)sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServers );
                                                ERR_CHK(rc);
					    }
                                            CosaDmlDhcpv6s_format_DNSoption( dnsServer );
                                            char l_cSecWebUI_Enabled[8] = {0};
                                            syscfg_get(NULL, "SecureWebUI_Enable", l_cSecWebUI_Enabled, sizeof(l_cSecWebUI_Enabled));
                                            char static_dns[256] = {0};
                                            ifl_get_event("lan_ipaddr_v6", static_dns, sizeof(static_dns));
                                            // Check device is in captive portal mode or not
                                            if( 1 == isInCaptivePortal )
                                            {
                                                  if (!strncmp(l_cSecWebUI_Enabled, "true", 4) && (!ula_enable))
                                                  {
                                                      if ( '\0' != dnsServer[ 0 ] )
                                                      {
                                                          if ( '\0' != static_dns[ 0 ] )
                                                          {
                                                              fprintf(fp, "#    option %s %s,%s\n", tagList[Index3].cmdstring, static_dns, dnsServer);
                                                          }
                                                          else
                                                          {
                                                              fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, dnsServer);
                                                          }
                                                      }
                                                      else
                                                      {
                                                          if ( '\0' != static_dns[ 0 ] )
                                                          {
                                                              fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, static_dns);
                                                          }
                                                      }
                                                  }
                                                  else
                                                  {
                                                              fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, dnsServer);
                                                  }
                                            }
                                            else
                                            {
                                                  if (!strncmp(l_cSecWebUI_Enabled, "true", 4) && (!ula_enable))
                                                  {
                                                      if ( '\0' != dnsServer[ 0 ] )
                                                      {
                                                          if ( '\0' != static_dns[ 0 ] )
                                                          {
                                                              fprintf(fp, "     option %s %s,%s\n", tagList[Index3].cmdstring, static_dns, dnsServer);
                                                          }
                                                          else
                                                          {
                                                              fprintf(fp, "     option %s %s\n", tagList[Index3].cmdstring, dnsServer);
                                                          }
                                                      }
                                                      else
                                                      {
                                                          if ( '\0' != static_dns[ 0 ] )
                                                          {
                                                              fprintf(fp, "     option %s %s\n", tagList[Index3].cmdstring, static_dns);
                                                          }
                                                      }
                                                  }
                                                  else
                                                  {
                                                     fprintf(fp, "     option %s %s\n", tagList[Index3].cmdstring, dnsServer);
                                                  }
                                           }
                               }
                }
                else if (sDhcpv6ServerPoolOption[Index][Index2].Tag == 24) {
                    char domain_str[256] = {0};
                    ifl_get_event ("ipv6_domain_name", domain_str, sizeof(domain_str));
                    if (domain_str[0] != '\0') {
                        if( isInCaptivePortal == TRUE )
                        {
                            fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, domain_str);
                        }
                        else
                        {
                            fprintf(fp, "     option %s %s\n", tagList[Index3].cmdstring, domain_str);
                        }
                    }
                }
                else if (sDhcpv6ServerPoolOption[Index][Index2].Tag == 17) {
                    char vendor_spec[512] = {0};
                    ifl_get_event ("vendor_spec", vendor_spec, sizeof(vendor_spec));
                    if (vendor_spec[0] != '\0') {
                        fprintf(fp, "    option %s %s\n", tagList[Index3].cmdstring, vendor_spec);
                    }
                    else
                    {
                        DHCPMGR_LOG_WARNING("vendor_spec sysevent failed to get, so not updating vendor_spec information. \n");
                    }
                }
#else
                for ( Index4 = 0; Index4 < g_recv_option_num; Index4++ )
                {
                    if ( g_recv_options[Index4].Tag != sDhcpv6ServerPoolOption[Index][Index2].Tag  )
                        continue;
                    else
                        break;
                }
                ULONG ret = 0;
                if ( Index4 >= g_recv_option_num )
                    continue;
                /* We need to translate hex to normal string */
                if ( g_recv_options[Index4].Tag == 23 )
                { //dns
                   char dnsStr[ 256 ] = { 0 };
                   char l_cSecWebUI_Enabled[8] = {0};
                   syscfg_get(NULL, "SecureWebUI_Enable", l_cSecWebUI_Enabled, sizeof(l_cSecWebUI_Enabled));
                                   /* Static DNS Servers */
                                   if( 1 == sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServersEnabled )
                                   {
                                          rc = strcpy_s( dnsStr, sizeof(dnsStr), (const char * __restrict__)sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServers );
                                          ERR_CHK(rc);
                                          CosaDmlDhcpv6s_format_DNSoption( dnsStr );
                                          // Check device is in captive portal mode or not
                                          if( 1 == isInCaptivePortal )
                                          {
                                                  if (!strncmp(l_cSecWebUI_Enabled, "true", 4))
                                                  {
                                                      char static_dns[256] = {0};
                                                      ifl_get_event("lan_ipaddr_v6", static_dns, sizeof(static_dns));
                                                      if ( '\0' != dnsStr[ 0 ] )
                                                      {
                                                          if ( '\0' != static_dns[ 0 ] )
                                                          {
                                                              fprintf(fp, "#    option %s %s,%s\n", tagList[Index3].cmdstring, static_dns, dnsStr);
                                                          }
                                                          else
                                                          {
                                                              fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, dnsStr);
                                                          }
                                                      }
                                                      else
                                                      {
                                                          char dyn_dns[256] = {0};
                                                          ret = CosaDmlDHCPv6sGetDNS((char*)g_recv_options[Index4].Value, dyn_dns, sizeof(dyn_dns) );
                                                          if ( '\0' != dyn_dns[ 0 ] )
                                                          {
                                                              if ( '\0' != static_dns[ 0 ] )
                                                              {
                                                                  fprintf(fp, "#    option %s %s,%s\n", tagList[Index3].cmdstring, static_dns, dyn_dns);
                                                              }
                                                              else
                                                              {
                                                                  fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, dyn_dns);
                                                              }
                                                          }
                                                          else
                                                          {
                                                              if ( '\0' != static_dns[ 0 ] )
                                                              {
                                                                  fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, static_dns);
                                                              }
                                                          }
                                                      }
                                                  }
                                                  else
                                                  {
                                                      fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, dnsStr);
                                                  }
                                          }
                                          else
                                          {
                                                  if (!strncmp(l_cSecWebUI_Enabled, "true", 4))
                                                  {
                                                      char static_dns[256] = {0};
                                                      ifl_get_event("lan_ipaddr_v6", static_dns, sizeof(static_dns));
                                                      char dyn_dns[256] = {0};
                                                      ret = CosaDmlDHCPv6sGetDNS((char*)g_recv_options[Index4].Value, dyn_dns, sizeof(dyn_dns) );
                                                      if ( '\0' != dnsStr[ 0 ] )
                                                      {
                                                          if ( '\0' != static_dns[ 0 ] )
                                                          {
                                                              if ( '\0' != dyn_dns[ 0 ] )
                                                              {
                                                                  fprintf(fp, "    option %s %s,%s,%s\n", tagList[Index3].cmdstring, static_dns, dnsStr, dyn_dns);
                                                              }
                                                              else
                                                              {
                                                                  fprintf(fp, "    option %s %s,%s\n", tagList[Index3].cmdstring, static_dns, dnsStr);
                                                              }
                                                          }
                                                          else
                                                          {
                                                              if ( '\0' != dyn_dns[ 0 ] )
                                                              {
                                                                  fprintf(fp, "    option %s %s,%s\n", tagList[Index3].cmdstring, dyn_dns, dnsStr);
                                                              }
                                                              else
                                                              {
                                                                  fprintf(fp, "     option %s %s\n", tagList[Index3].cmdstring, dnsStr);
                                                              }
                                                          }
                                                      }
                                                      else
                                                      {
                                                          if ( '\0' != static_dns[ 0 ] )
                                                          {
                                                              if ( '\0' != dyn_dns[ 0 ] )
                                                              {
                                                                  fprintf(fp, "    option %s %s,%s\n", tagList[Index3].cmdstring, static_dns, dyn_dns);
                                                              }
                                                              else
                                                              {
                                                                  fprintf(fp, "     option %s %s\n", tagList[Index3].cmdstring, static_dns);
                                                              }
                                                          }
                                                          else
                                                          {
                                                              if ( '\0' != dyn_dns[ 0 ] )
                                                              {
                                                                  fprintf(fp, "     option %s %s\n", tagList[Index3].cmdstring, dyn_dns);
                                                              }
                                                          }
                                                      }
                                                  }
                                                  else
                                                  {
                                                      fprintf(fp, "     option %s %s\n", tagList[Index3].cmdstring, dnsStr);
                                                  }
                                         }
                                         DHCPMGR_LOG_WARNING("%s %d - DNSServersEnabled:%d DNSServers:%s\n", __FUNCTION__,
                                                                                                                                                                                __LINE__,
                                                                                                                                                                                sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServersEnabled,
                                                                                                                                                                                sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServers );
                                   }
                                   else
                                   {
#if defined(_COSA_BCM_ARM_)
                       ret=ifl_get_event("wan6_ns", dnsStr, sizeof(dnsStr));
#else
                       ret = CosaDmlDHCPv6sGetDNS((char *)g_recv_options[Index4].Value, dnsStr, sizeof(dnsStr) );
#endif
                       if ( !ret )
                       {
                           if ( '\0' != dnsStr[ 0 ] )
                           {
#if defined(_COSA_BCM_ARM_)
                               format_dibbler_option(dnsStr);
#endif
                               // Check device is in captive portal mode or not
                               if ( 1 == isInCaptivePortal )
                               {
                                   fprintf(fp, "# option %s %s\n", tagList[Index3].cmdstring, dnsStr);
                               }
                               else
                               {
                                   fprintf(fp, "    option %s %s\n", tagList[Index3].cmdstring, dnsStr);
                               }
                           }
                       }
                   }
               }
                else if ( g_recv_options[Index4].Tag == 24 )
                { //domain
                    pServerOption =    CosaDmlDhcpv6sGetStringFromHex((char *)g_recv_options[Index4].Value);
                    if ( pServerOption )
                        fprintf(fp, "    option %s %s\n", tagList[Index3].cmdstring, pServerOption);
                }else{
                    if ( pServerOption )
                        fprintf(fp, "    option %s 0x%s\n", tagList[Index3].cmdstring, pServerOption);
                }
#endif
            }
            else
            {
                /* We need to translate hex to normal string */
                if ( sDhcpv6ServerPoolOption[Index][Index2].Tag == 23 )
                { //dns
                    char dns_str[256] = {0};
                                        /* Static DNS Servers */
                                        if( 1 == sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServersEnabled )
                                        {
                                           rc = strcpy_s( dns_str, sizeof(dns_str), (char*)sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServers );
                                           ERR_CHK(rc);
                                           CosaDmlDhcpv6s_format_DNSoption( dns_str );
                                           // Check device is in captive portal mode or not
                                           if( 1 == isInCaptivePortal )
                                           {
                                                   fprintf(fp, "#       option %s %s\n", tagList[Index3].cmdstring, dns_str);
                                           }
                                           else
                                           {
                                                   fprintf(fp, "        option %s %s\n", tagList[Index3].cmdstring, dns_str);
                                        }
                                                  DHCPMGR_LOG_WARNING("%s %d - DNSServersEnabled:%d DNSServers:%s\n", __FUNCTION__,
                                                                                                                                                                                 __LINE__,
                                                                                                                                                                                 sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServersEnabled,
                                                                                                                                                                                 sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServers );
                                        }
                                        else
                                        {
                                                if ( _ansc_strstr((const char*)sDhcpv6ServerPoolOption[Index][Index2].Value, ":") )
                                                        pServerOption = (char*)sDhcpv6ServerPoolOption[Index][Index2].Value;
                                                else
                                                        pServerOption = CosaDmlDhcpv6sGetAddressFromString((char*)sDhcpv6ServerPoolOption[Index][Index2].Value);
                                                if ( pServerOption ){
#if defined(_XB6_PRODUCT_REQ_) && defined(_COSA_BCM_ARM_)
                                               if( isInCaptivePortal == TRUE ) {
                                                        fprintf(fp, "#  option %s %s\n", tagList[Index3].cmdstring, pServerOption);
                                                }
#else
                                                        fprintf(fp, "   option %s %s\n", tagList[Index3].cmdstring, pServerOption);
#endif
                                              }
                                 }
                }
                else if ( g_recv_options[Index4].Tag == 24 )
                { //domain
                    pServerOption = CosaDmlDhcpv6sGetStringFromHex((char*)g_recv_options[Index4].Value);
                    if ( pServerOption ){
#if defined(_XB6_PRODUCT_REQ_) && defined(_COSA_BCM_ARM_)
                       if( isInCaptivePortal == TRUE ) {
                                fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, pServerOption);
                        }
#else
                        fprintf(fp, "    option %s %s\n", tagList[Index3].cmdstring, pServerOption);
#endif
                    }
                }else
                    if ( pServerOption ){
#if defined(_XB6_PRODUCT_REQ_) && defined(_COSA_BCM_ARM_)
                       if( isInCaptivePortal == TRUE ) {
                            fprintf(fp, "#    option %s 0x%s\n", tagList[Index3].cmdstring, pServerOption);
                        }
#else
                        fprintf(fp, "    option %s 0x%s\n", tagList[Index3].cmdstring, pServerOption);
#endif
                }
            }
        }
        fprintf(fp, "}\n");
    }
    if(fp != NULL)
      fclose(fp);
#if (!defined _COSA_INTEL_USG_ARM_) && (!defined _COSA_BCM_MIPS_)
    /*we will copy the updated conf file at once*/
    if (rename(TMP_SERVER_CONF, SERVER_CONF_LOCATION))
        DHCPMGR_LOG_WARNING("%s rename failed %s\n", __FUNCTION__, strerror(errno));
#endif
if (stat(SERVER_CONF_LOCATION, &check_ConfigFile) == -1) {
        ifl_set_event("dibbler_server_conf-status","");
}
else if (check_ConfigFile.st_size == 0) {
        ifl_set_event("dibbler_server_conf-status","empty");
}
else {
        ifl_set_event("dibbler_server_conf-status","ready");
}
EXIT:
    return;
}
#endif

#ifdef _COSA_BCM_MIPS_
void __cosa_dhcpsv6_refresh_config()
{
    FILE * fp = fopen(SERVER_CONF_LOCATION, "w+");
    PCHAR pTmp1 = NULL;
    PCHAR pTmp2 = NULL;
    PCHAR pTmp3 = NULL;
    CHAR prefixValue[64] = {0};
    CHAR prefixFullName[64] = {0};
    ULONG Index  = 0;
    ULONG Index2 = 0;
    ULONG Index3 = 0;
    int Index4 = 0;
    ULONG uSize = 0;
    ULONG preferedTime = 3600;
    ULONG validTime = 7200;
    int   returnValue = 0;
    ULONG ret = 0;
    char * saveptr = NULL;
    char *pServerOption = NULL;
    FILE *responsefd=NULL;
    char *networkResponse = "/var/tmp/networkresponse.txt";
    int iresCode = 0;
    char responseCode[10];
    struct stat check_ConfigFile;
    errno_t rc = -1;
#if defined(CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION) || defined(_ONESTACK_PRODUCT_REQ_)
    pd_pool_t           pd_pool;
    ia_pd_t             ia_pd;
#endif
    ULONG  T1 = 0;
    ULONG  T2 = 0;
        char buf[ 6 ];
        int IsCaptivePortalMode = 0;
    BOOL  bBadPrefixFormat = FALSE;
    if (!fp)
        goto EXIT;
#if defined(CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION) || defined(_ONESTACK_PRODUCT_REQ_)
    #if defined(_ONESTACK_PRODUCT_REQ_)
    if (isFeatureSupportedInCurrentMode(FEATURE_IPV6_DELEGATION))
    #endif
    {
    /* handle logic:
     *  1) divide the Operator-delegated prefix to sub-prefixes
     *  2) further break the first of these sub-prefixes into /64 interface-prefixes for lan interface
     *  3) assign IPv6 address from the corresponding interface-prefix for lan interfaces
     *  4) the remaining sub-prefixes are delegated via DHCPv6 to directly downstream routers
     *  5) Send RA, start DHCPv6 server
     */
    if (divide_ipv6_prefix() != 0) {
      // DHCPMGR_LOG_ERROR("[%s] ERROR divide the operator-delegated prefix to sub-prefix error.\n",  __FUNCTION__);
        DHCPMGR_LOG_ERROR("divide the operator-delegated prefix to sub-prefix error.");
        // sysevent_set(si6->sefd, si6->setok, "service_ipv6-status", "error", 0);
        ifl_set_event("service_ipv6-status", "error");
        return;
    }
    }
#endif
    /*Begin write configuration */
    fprintf(fp, "log-level 8\n");
    //Intel Proposed RDKB Generic Bug Fix from XB6 SDK
    fprintf(fp, "reconfigure-enabled 1\n");
    if ( g_dhcpv6_server_type != DHCPV6_SERVER_TYPE_STATEFUL )
        fprintf(fp, "stateless\n");
    for ( Index = 0; Index < uDhcpv6ServerPoolNum; Index++ )
    {
        /* We need get interface name according to Interface field*/
        if ( !sDhcpv6ServerPool[Index].Cfg.bEnabled )
            continue;
       // Skip emit any invalid iface entry to the dibbler server.conf file
       if (sDhcpv6ServerPool[Index].Cfg.PrefixRangeBegin[0] == '\0'  &&
           sDhcpv6ServerPool[Index].Cfg.PrefixRangeEnd[0] == '\0')
         {
           DHCPMGR_LOG_INFO("%s Skip Index: %lu, iface: %s, entry. Invalid PrefixRangeBegin: %s,  PrefixRangeEnd: %s, LeaseTime: %lu\n",
                          __func__, Index, COSA_DML_DHCPV6_SERVER_IFNAME, sDhcpv6ServerPool[Index].Cfg.PrefixRangeBegin,
                          sDhcpv6ServerPool[Index].Cfg.PrefixRangeEnd, sDhcpv6ServerPool[Index].Cfg.LeaseTime);
            continue;
         }
        fprintf(fp, "iface %s {\n", COSA_DML_DHCPV6_SERVER_IFNAME);
        if (g_dhcpv6_server_type != DHCPV6_SERVER_TYPE_STATEFUL )
            goto OPTIONS;
        if ( sDhcpv6ServerPool[Index].Cfg.RapidEnable ){
            fprintf(fp, "   rapid-commit yes\n");
        }
#ifdef CONFIG_CISCO_DHCP6S_REQUIREMENT_FROM_DPC3825
        if ( sDhcpv6ServerPool[Index].Cfg.UnicastEnable ){
            char globalAddress[64] = {0};
            ifl_get_event("lan_ipaddr_v6", globalAddress, sizeof(globalAddress));
            if ( strlen(globalAddress) > 0 )
                fprintf(fp, "   unicast %s\n", globalAddress);
        }
        fprintf(fp, "   iface-max-lease %d\n", sDhcpv6ServerPool[Index].Cfg.IANAAmount );
#endif
        /* on GUI, we will limit the order to be [1-256]*/
        fprintf(fp, "   preference %d\n", 255); /*256-(sDhcpv6ServerPool[Index].Cfg.Order%257));*/
        /*begin class
                    fc00:1:0:0:4::/80,fc00:1:0:0:5::/80,fc00:1:0:0:6::/80
                */
        if ( sDhcpv6ServerPool[Index].Cfg.IANAEnable && sDhcpv6ServerPool[Index].Info.IANAPrefixes[0] )
        {
            pTmp1 = AnscCloneString((char*)sDhcpv6ServerPool[Index].Info.IANAPrefixes);
            pTmp3 = pTmp1;
            pTmp2 = pTmp1;
            for (; ; pTmp2 = NULL)
            {
                pTmp1 = strtok_r(pTmp2, ",", &saveptr);
                if (pTmp1 == NULL)
                    break;
                /* This pTmp1 is IP.Interface.{i}.IPv6Prefix.{i}., we need ipv6 address(eg:2000:1:0:0:6::/64) according to it*/
                rc = sprintf_s(prefixFullName, sizeof(prefixFullName), "%sPrefix",pTmp1);
                if(rc < EOK)
                {
                    ERR_CHK(rc);
                }
                uSize = sizeof(prefixValue);
                returnValue = g_GetParamValueString(g_pDslhDmlAgent, prefixFullName, prefixValue, &uSize);
                if ( returnValue != 0 )
                {
                    DHCPMGR_LOG_WARNING("_cosa_dhcpsv6_refresh_config -- g_GetParamValueString for iana:%d\n", returnValue);
                }
                fprintf(fp, "   class {\n");
#ifdef CONFIG_CISCO_DHCP6S_REQUIREMENT_FROM_DPC3825
                /*When enable EUI64, the pool prefix value must use xxx/64 format*/
                if ( sDhcpv6ServerPool[Index].Cfg.EUI64Enable){
                    fprintf(fp, "       share 1000\n");
                }
                /*prefix value should be: xxxx/64 */
                fprintf(fp, "       pool %s\n", prefixValue );
#else
                /*prefix value May be: 2001:01:02::/64 or 2001:0001:0001:001::/64
                                    ::/ is necessary.
                                    We need translate them to valid value.
                                 */
                {
                    ULONG i = _ansc_strlen((const char*)prefixValue);
                    int j = 0;
                    while( (prefixValue[i-1] != '/') && ( i > 0 ) )
                        i--;
                    if ( i == 0 ){
                        DHCPMGR_LOG_WARNING("_cosa_dhcpsv6_refresh_config -- iana:%s is error\n", prefixValue);
                       bBadPrefixFormat = TRUE;
                    }
                    if ( ( prefixValue[i-2] != ':' ) || ( prefixValue[i-3] != ':' ) ){
                        DHCPMGR_LOG_WARNING("_cosa_dhcpsv6_refresh_config -- iana:%s is error\n", prefixValue);
                       bBadPrefixFormat = TRUE;
                    }
#if 0
                    /* We just delete last '/' here */
                    prefixValue[i-1] = '\0';
                    /* skip '::/'  */
                    i = i-3;
                    while(  i > 0 ) {
                        if ( prefixValue[i-1] == ':' )
                            j++;
                        i--;
                    }
                    /* delete one last ':' becaues there are 4 parts in this prefix*/
                    if ( j == 3 )
                        prefixValue[_ansc_strlen(prefixValue)-1] = '\0';
#endif
                    if (bBadPrefixFormat == FALSE && i > 2)
                     {
                       /* Need to convert A....::/xx to W:X:Y:Z: where X, Y or Z could be zero */
                       /* We just delete last ':/' here */
                       ULONG k;
                       for ( k = 0; k < i; k++ )
                       {
                           if ( prefixValue[k] == ':' )
                           {
                               j++;
                               if ( k && (k < (sizeof(prefixValue) - 6)) )
                               {
                                   if ( prefixValue[k+1] == ':' )
                                   {
                                       switch (j)
                                       {
                                           case 1:
                                               // A:: -> A:0:0:0:
                                               rc = strcpy_s( &prefixValue[k+1], sizeof(prefixValue) - k, "0:0:0:" );
                                               ERR_CHK(rc);
                                           break;
                                           case 2:
                                               // A:B:: -> A:B:0:0:
                                               rc = strcpy_s( &prefixValue[k+1], sizeof(prefixValue) - k, "0:0:" );
                                               ERR_CHK(rc);
                                           break;
                                           case 3:
                                               // A:B:C:: -> A:B:C:0:
                                               rc = strcpy_s( &prefixValue[k+1], sizeof(prefixValue) - k,  "0:" );
                                               ERR_CHK(rc);
                                           break;
                                           case 4:
                                               // A:B:C:D:: -> A:B:C:D:
                                               prefixValue[k+1] = 0;
                                           break;
                                       }
                                       break;
                                   }
                               }
                           }
                       }
                       DHCPMGR_LOG_INFO("%s Fixed prefixValue: %s\n", __func__, prefixValue);
                     }
                }
                fprintf(fp, "       pool %s%s - %s%s\n", prefixValue, sDhcpv6ServerPool[Index].Cfg.PrefixRangeBegin, prefixValue, sDhcpv6ServerPool[Index].Cfg.PrefixRangeEnd );
#endif
                /*we need get two time values */
                {
                    if (sDhcpv6ServerPool[Index].Cfg.LeaseTime <= -1 ) {
                        T1 = T2 = preferedTime = validTime = 0xFFFFFFFF;
                    }else{
                        T1           = (sDhcpv6ServerPool[Index].Cfg.LeaseTime)/2;
                        T2           = (ULONG)((sDhcpv6ServerPool[Index].Cfg.LeaseTime)*80.0/100);
                        preferedTime = (sDhcpv6ServerPool[Index].Cfg.LeaseTime);
                        validTime    = (sDhcpv6ServerPool[Index].Cfg.LeaseTime);
                    }
                    fprintf(fp, "       T1 %lu\n", T1);
                    fprintf(fp, "       T2 %lu\n", T2);
                    fprintf(fp, "       prefered-lifetime %lu\n", preferedTime);
                    fprintf(fp, "       valid-lifetime %lu\n", validTime);
                }
                fprintf(fp, "   }\n");
	    }
	    AnscFreeMemory(pTmp3);
#if defined(CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION) || defined(_ONESTACK_PRODUCT_REQ_)
#if defined(_ONESTACK_PRODUCT_REQ_)
	    if (isFeatureSupportedInCurrentMode(FEATURE_IPV6_DELEGATION))
#endif
	    {
            DHCPMGR_LOG_INFO("[%s]  %d - See if need to emit pd-class, sDhcpv6ServerPool[Index].Cfg.IAPDEnable: %d, Index: %d\n",
                           __FUNCTION__, __LINE__, sDhcpv6ServerPool[Index].Cfg.IAPDEnable, Index);
            if (sDhcpv6ServerPool[Index].Cfg.IAPDEnable) {
              /*pd pool*/
              if(get_pd_pool(&pd_pool) == 0) {
                DHCPMGR_LOG_INFO("[%s]  %d emit pd-class { pd-pool pd_pool.start: %s, pd_pool.prefix_length: %d\n",
                               __FUNCTION__, __LINE__, pd_pool.start, pd_pool.prefix_length);
                DHCPMGR_LOG_INFO("[%s]  %d emit            pd-length pd_pool.pd_length: %d\n",
                               __FUNCTION__, __LINE__, pd_pool.pd_length);
                fprintf(fp, "   pd-class {\n");
#if defined (_CBR_PRODUCT_REQ_) || defined (CISCO_CONFIG_DHCPV6_PREFIX_DELEGATION) || defined(_ONESTACK_PRODUCT_REQ_) 
                fprintf(fp, "       pd-pool %s /%d\n", pd_pool.start, pd_pool.prefix_length);
#else
                fprintf(fp, "       pd-pool %s - %s /%d\n", pd_pool.start, pd_pool.end, pd_pool.prefix_length);
#endif
                fprintf(fp, "       pd-length %d\n", pd_pool.pd_length);
                if(get_iapd_info(&ia_pd) == 0) {
                  fprintf(fp, "       T1 %s\n", ia_pd.t1);
                  fprintf(fp, "       T2 %s\n", ia_pd.t2);
                  fprintf(fp, "       prefered-lifetime %s\n", ia_pd.pretm);
                  fprintf(fp, "       valid-lifetime %s\n", ia_pd.vldtm);
		}
		fprintf(fp, "   }\n");
	      }
	    }
	    }
#endif
	}
OPTIONS:
        /* For options */
        for ( Index2 = 0 ; Index2 < uDhcpv6ServerPoolOptionNum[Index]; Index2++ )
        {
            if ( !sDhcpv6ServerPoolOption[Index][Index2].bEnabled )
                continue;
            for ( Index3 = 0; Index3 < sizeof(tagList)/sizeof(struct DHCP_TAG);Index3++ )
            {
                if ( tagList[Index3].tag != (int)sDhcpv6ServerPoolOption[Index][Index2].Tag )
                    continue;
                else
                    break;
            }
            if ( Index3 >= sizeof(tagList)/sizeof(struct DHCP_TAG) )
                continue;
                        // During captive portal no need to pass DNS
                        // Check the reponse code received from Web Service
                        iresCode = 0;
                        if( ( responsefd = fopen( networkResponse, "r" ) ) != NULL )
                        {
                                if( fgets( responseCode, sizeof( responseCode ), responsefd ) != NULL )
                                {
                                        iresCode = atoi( responseCode );
                                }
                                /* free unused resources before return */
                                fclose(responsefd);
                                responsefd = NULL;
                        }
                        syscfg_get( NULL, "redirection_flag", buf, sizeof(buf));
                        if( buf != NULL )
                        {
                                if ( ( strncmp( buf,"true",4 ) == 0 ) && iresCode == 204 )
                                {
                                         IsCaptivePortalMode = 1;
                                }
                        }
            if ( sDhcpv6ServerPoolOption[Index][Index2].PassthroughClient[0] )
            {
                /* this content get from v6 client directly. If there is problem, change to
                                get from data model tree
                            */
                for ( Index4 = 0; Index4 < g_recv_option_num; Index4++ )
                {
                    if ( g_recv_options[Index4].Tag != sDhcpv6ServerPoolOption[Index][Index2].Tag  )
                        continue;
                    else
                        break;
                }
                if ( Index4 >= g_recv_option_num )
                    continue;
                /* We need to translate hex to normal string */
                if ( g_recv_options[Index4].Tag == 23 )
                { //dns
                   char dnsStr[256] = {0};
                                   /* Static DNS Servers */
                                   if( 1 == sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServersEnabled )
                                   {
                                          rc = strcpy_s( dnsStr, sizeof(dnsStr), (const char*)sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServers );
                                          ERR_CHK(rc);
                                          CosaDmlDhcpv6s_format_DNSoption( dnsStr );
                                          // Check device is in captive portal mode or not
                                          if( 1 == IsCaptivePortalMode )
                                          {
                                                  fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, dnsStr);
                                          }
                                          else
                                          {
                                                  fprintf(fp, "    option %s %s\n", tagList[Index3].cmdstring, dnsStr);
                                          }
                                          DHCPMGR_LOG_INFO("%s %d - DNSServersEnabled:%d DNSServers:%s\n", __FUNCTION__,
                                                                                                                                                                                __LINE__,
                                                                                                                                                                                sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServersEnabled,
                                                                                                                                                                                sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServers );
                                   }
                                   else
                                   {
                                           ret = CosaDmlDHCPv6sGetDNS((char*)g_recv_options[Index4].Value, dnsStr, sizeof(dnsStr) );
                                                if ( !ret )
                                                {
                                                         // Check device is in captive portal mode or not
                                                        if ( 1 == IsCaptivePortalMode )
                                                        {
                                                                fprintf(fp, "#    option %s %s\n", tagList[Index3].cmdstring, dnsStr);
                                                        }
                                                        else
                                                        {
                                                                fprintf(fp, "    option %s %s\n", tagList[Index3].cmdstring, dnsStr);
                                                        }
                                                }
                                   }
                }
                else if ( g_recv_options[Index4].Tag == 24 )
                { //domain
                    pServerOption =    CosaDmlDhcpv6sGetStringFromHex((char*)g_recv_options[Index4].Value);
                    if ( pServerOption )
                        fprintf(fp, "    option %s %s\n", tagList[Index3].cmdstring, pServerOption);
                }else{
                    if ( pServerOption )
                        fprintf(fp, "    option %s 0x%s\n", tagList[Index3].cmdstring, pServerOption);
                }
            }
            else
            {
                /* We need to translate hex to normal string */
                if ( sDhcpv6ServerPoolOption[Index][Index2].Tag == 23 )
                { //dns
                                        char dnsStr[256] = {0};
                                        /* Static DNS Servers */
                                        if( 1 == sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServersEnabled )
                                        {
                                           memset( dnsStr, 0, sizeof( dnsStr ) );
                                           rc = strcpy_s( dnsStr, sizeof(dnsStr), (const char*)sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServers );
                                           ERR_CHK(rc);
                                           CosaDmlDhcpv6s_format_DNSoption( dnsStr );
                                           // Check device is in captive portal mode or not
                                           if( 1 == IsCaptivePortalMode )
                                           {
                                                   fprintf(fp, "#        option %s %s\n", tagList[Index3].cmdstring, dnsStr);
                                           }
                                           else
                                           {
                                                   fprintf(fp, "       option %s %s\n", tagList[Index3].cmdstring, dnsStr);
                                           }
                                           DHCPMGR_LOG_WARNING("%s %d - DNSServersEnabled:%d DNSServers:%s\n", __FUNCTION__,
                                                                                                                                                                                 __LINE__,
                                                                                                                                                                                 sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServersEnabled,
                                                                                                                                                                                 sDhcpv6ServerPool[Index].Cfg.X_RDKCENTRAL_COM_DNSServers );
                                        }
                                        else
                                        {
                                                if ( _ansc_strstr((const char*)sDhcpv6ServerPoolOption[Index][Index2].Value, ":") )
                                                        pServerOption = (char*)sDhcpv6ServerPoolOption[Index][Index2].Value;
                                                else
                                                        pServerOption = CosaDmlDhcpv6sGetAddressFromString((char*)sDhcpv6ServerPoolOption[Index][Index2].Value);
                                                if ( pServerOption )
                                                        fprintf(fp, "    option %s %s\n", tagList[Index3].cmdstring, pServerOption);
                                        }
                }
                else if ( g_recv_options[Index4].Tag == 24 )
                { //domain
                    pServerOption = CosaDmlDhcpv6sGetStringFromHex((char*)g_recv_options[Index4].Value);
                    if ( pServerOption )
                        fprintf(fp, "    option %s %s\n", tagList[Index3].cmdstring, pServerOption);
                }
                else
                {
                    if ( pServerOption )
                        fprintf(fp, "    option %s 0x%s\n", tagList[Index3].cmdstring, pServerOption);
                }
            }
        }
        fprintf(fp, "}\n");
    }
    if(fp != NULL)
      fclose(fp);
        if (stat(SERVER_CONF_LOCATION, &check_ConfigFile) == -1) {
              ifl_set_event("dibbler_server_conf-status","");
        }
        else if (check_ConfigFile.st_size == 0) {
                ifl_set_event("dibbler_server_conf-status","empty");
        }
        else {
                ifl_set_event("dibbler_server_conf-status","ready");
        }
EXIT:
    return;
}
#endif

int CosaDmlDhcpv6s_format_DNSoption( char *option )
{
    if (option == NULL)
        return -1;
    unsigned int i;
    for (i = 0; i < strlen(option); i++) {
        if(option[i] == ' ')
            option[i] = ',';
    }
    return 0;
}

int format_dibbler_option(char *option)
{
    if (option == NULL)
        return -1;
    unsigned int i;
    size_t len = strlen(option);
    for (i = 0; i < len; i++) {
        if(option[i] == ' ')
            option[i] = ',';
    }
    return 0;
}

/*
    This function will translate DNS from flat string to dns strings by comma.
    length: 32*N
    2018CAFE00000000020C29FFFE97FCCC2222CAFE00000000020C29FFFE97FCCC
    will translate to
    2018:CAFE:0000:0000:020C:29FF:FE97:FCCC,2222:CAFE:0000:0000:020C:29FF:FE97:FCCC
    Fail when return 1
    Succeed when return 0
*/
int CosaDmlDHCPv6sGetDNS(char* Dns, char* output, int outputLen)
{
    char oneDns[64]  = {0};
    int  len         = _ansc_strlen(Dns);
    int  count       = len/32;
    int  i           = 0;
    char *pStr       = NULL;
    if ( outputLen < (count*(32+8)) )
    {
        count = outputLen/(32+8);
        if (!count)
            return 1;
    }
    output[0] = '\0';
    while(i < count){
        _ansc_strncpy(oneDns, &Dns[i*32], 32);
        pStr = CosaDmlDhcpv6sGetAddressFromString(oneDns);
        _ansc_strcat(output, pStr);
        _ansc_strcat(output, ",");
        i++;
    }
    DHCPMGR_LOG_INFO("!!!!!!! %d %d %s %s", outputLen, count, Dns, output);
    output[_ansc_strlen(output)-1] = '\0';
    return 0;
}

char * CosaDmlDhcpv6sGetStringFromHex(char * hexString){
    static char     newString[256];
    char    buff[8] = {0};
    ULONG   i =0;
    ULONG   j =0;
    ULONG   k =0;
    memset(newString,0, sizeof(newString));
    while( hexString[i] && (i< sizeof(newString)-1) )
    {
        buff[k++]        = hexString[i++];
        if ( k%2 == 0 ) {
             char c =  (char)strtol(buff, (char **)NULL, 16);
             if( !iscntrl(c) )
                 newString[j++] = c;
             memset(buff, 0, sizeof(buff));
             k = 0;
        }
    }
    DHCPMGR_LOG_WARNING("New normal string is %s from %s .\n", newString, hexString);
    return newString;
}

char * CosaDmlDhcpv6sGetAddressFromString(char * address){
    static char     ipv6Address[256] = {0};
    ULONG   i =0;
    ULONG   j =0;
    while( address[i] && (i< sizeof(ipv6Address)) )
    {
        ipv6Address[j++] = address[i++];
        if ( i%4 == 0 )
            ipv6Address[j++] = ':';
    }
    /*remove last ":"*/
    if ( j == 0 )
        ipv6Address[j] = '\0';
    else
        ipv6Address[j-1] = '\0';
    DHCPMGR_LOG_WARNING("New ipv6 address is %s from %s .\n", ipv6Address, address);
    return ipv6Address;
}

BOOL tagPermitted(int tag)
{
    unsigned int i = 0;

    for ( i =0; i < sizeof(tagList)/sizeof(struct DHCP_TAG); i++ )
    {
        if ( tag == tagList[i].tag )
            break;
    }

    if ( i < sizeof(tagList)/sizeof(struct DHCP_TAG) )
        return true;
    else
        return false;
}

#endif
