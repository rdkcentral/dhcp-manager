#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sm_DhcpMgr.h"
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include "dhcpmgr_rbus_apis.h"
#include <errno.h>
#include <string.h>
#include "util.h"

/* Define the global MQ descriptors declared in mq_shared.h */
mqd_t mq_dispatch = (mqd_t)-1; /* MQ_NAME (/lan_sm_queue) */
mqd_t mq_fsm = (mqd_t)-1;      /* SM_MQ_NAME (/lan_fsm_queue) */

/* Cleanup handler: close and unlink message queues then exit */
static void cleanup_handler(int signo)
{
    DHCPMGR_LOG_INFO("%s:%d Received signal %d, cleaning up...\n", __FUNCTION__, __LINE__, signo);
    if (mq_dispatch != (mqd_t)-1) {
        mq_close(mq_dispatch);
        mq_unlink(MQ_NAME);
        mq_dispatch = (mqd_t)-1;
        DHCPMGR_LOG_INFO("%s:%d Closed and unlinked dispatch MQ %s\n", __FUNCTION__, __LINE__, MQ_NAME);
    }
    if (mq_fsm != (mqd_t)-1) {
        mq_close(mq_fsm);
        mq_unlink(SM_MQ_NAME);
        mq_fsm = (mqd_t)-1;
        DHCPMGR_LOG_INFO("%s:%d Closed and unlinked FSM MQ %s\n", __FUNCTION__, __LINE__, SM_MQ_NAME);
    }
    exit(0);
}


int dhcp_server_init_main() {
    
    DHCPMGR_LOG_INFO("%s:%d DHCP Manager starting...\n", __FUNCTION__, __LINE__);
    DhcpMgr_Rbus_Init(); // Initialize rbus
    /* Create/open the message queues centrally so other libraries can use the descriptors.
       mq_dispatch corresponds to MQ_NAME (dispatch queue, holds DhcpMgr_DispatchEvent values)
       mq_fsm corresponds to SM_MQ_NAME (fsm queue, holds DhcpManagerEvent values)
    */
    struct mq_attr attr;

    /* Create dispatch queue (MQ_NAME) */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(DhcpMgr_DispatchEvent);
    attr.mq_curmsgs = 0;
    mq_dispatch = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mq_dispatch == (mqd_t)-1) {
        DHCPMGR_LOG_ERROR("%s:%d mq_open MQ_NAME failed: %s\n", __FUNCTION__, __LINE__, strerror(errno));
    } else {
        DHCPMGR_LOG_INFO("%s:%d Opened dispatch MQ %s (msgsize=%ld)\n", __FUNCTION__, __LINE__, MQ_NAME, (long)attr.mq_msgsize);
    }

    /* Create FSM action queue (SM_MQ_NAME) */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(DhcpManagerEvent);
    attr.mq_curmsgs = 0;
    mq_fsm = mq_open(SM_MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mq_fsm == (mqd_t)-1) {
        DHCPMGR_LOG_ERROR("%s:%d mq_open SM_MQ_NAME failed: %s\n", __FUNCTION__, __LINE__, strerror(errno));
    } else {
        DHCPMGR_LOG_INFO("%s:%d Opened FSM MQ %s (msgsize=%ld)\n", __FUNCTION__, __LINE__, SM_MQ_NAME, (long)attr.mq_msgsize);
    }

    /* Start FSM threads declared in sm_DhcpMgr.h
       - FSMThread listens on MQ_NAME ("/lan_sm_queue") and posts dispatch events
       - FSM_Dispatch_Thread listens on SM_MQ_NAME ("/lan_fsm_queue") and executes actions
    */
    pthread_t fsm_tid, dispatch_tid;
    if (pthread_create(&fsm_tid, NULL, FSMThread, NULL) != 0) {
        DHCPMGR_LOG_ERROR("%s:%d pthread_create FSMThread failed: %s\n", __FUNCTION__, __LINE__, strerror(errno));
    } else {
        pthread_detach(fsm_tid);
    }

    if (pthread_create(&dispatch_tid, NULL, FSM_Dispatch_Thread, NULL) != 0) {
        DHCPMGR_LOG_ERROR("%s:%d pthread_create FSM_Dispatch_Thread failed: %s\n", __FUNCTION__, __LINE__, strerror(errno));
    } else {
        pthread_detach(dispatch_tid);
    }

    /* register handler for SIGINT and SIGTERM */
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);

    dhcp_server_signal_state_machine_ready(); // Signal that the state machine is ready
    dhcp_server_publish_state(DHCPS_STATE_IDLE); // Publish initial state

    /* Keep the main thread alive; threads run the FSM and MQ handling */
    while (1) {
        pause(); /* wait for signals; lightweight sleep */
    }


    return 0;
}