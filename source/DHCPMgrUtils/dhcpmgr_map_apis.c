/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sysevent/sysevent.h>
#include "cosa_apis.h"
#include "dhcpmgr_map_apis.h"

/*
 * Macro definitions
 */

#define STRING_TO_HEX(pStr) ( (pStr-'a'<0)? (pStr-'A'<0)? pStr-'0' : pStr-'A'+10 : pStr-'a'+10 )

#define ERR_CHK(x) 
#define CCSP_COMMON_FIFO                             "/tmp/ccsp_common_fifo"

extern int sysevent_fd;
extern token_t sysevent_token;

/*
 * Static function prototypes
 */
static RETURN_STATUS CosaDmlMaptParseResponse (PUCHAR pOptionBuf, UINT16 uiOptionBufLen);
static RETURN_STATUS CosaDmlMaptGetIPv6StringFromHex (PUCHAR pIPv6AddrH, PCHAR pIPv6AddrS);
static RETURN_STATUS CosaDmlMaptConvertStringToHexStream (PUCHAR pOptionBuf, PUINT16 uiOptionBufLen);
/*
 * Global definitions
 */
static COSA_DML_MAPT_DATA   g_stMaptData;

/*
 * Static function definitions
 */
static RETURN_STATUS
CosaDmlMaptGetIPv6StringFromHex
(
    PUCHAR         pIPv6AddrH,
    PCHAR          pIPv6AddrS
)
{
  MAPT_LOG_INFO("Entry");

  if ( !(pIPv6AddrH && pIPv6AddrS) )
  {
       MAPT_LOG_ERROR("NULL inputs(%s - %s) for IPv6 hex to string conversion!",
                       pIPv6AddrH, pIPv6AddrS);
       return STATUS_FAILURE;
  }

  memset(pIPv6AddrS, 0, BUFLEN_40);
  

  if ( !inet_ntop(AF_INET6, pIPv6AddrH, pIPv6AddrS, BUFLEN_40) )
  {
       MAPT_LOG_ERROR("Invalid IPv6 hex address");
       return STATUS_FAILURE;
  }

  return STATUS_SUCCESS;
}

static RETURN_STATUS
CosaDmlMaptParseResponse
(
    PUCHAR         pOptionBuf,
    UINT16         uiOptionBufLen
)
{
  RETURN_STATUS retStatus = STATUS_SUCCESS;
  PCOSA_DML_MAPT_OPTION  pStartBuf  = NULL;
  PCOSA_DML_MAPT_OPTION  pNxtOption = NULL;
  PCOSA_DML_MAPT_OPTION  pEndBuf    = NULL;
  UINT16  uiOptionLen = 0;
  UINT16  uiOption    = 0;
  PUCHAR  pCurOption  = 0;

  MAPT_LOG_INFO("Entry");

  if ( !(pOptionBuf && *(pOptionBuf+1)) )
  {
       MAPT_LOG_ERROR("MAPT Option-95 Response is NULL !!");
       return STATUS_FAILURE;
  }

  pStartBuf = (PCOSA_DML_MAPT_OPTION)pOptionBuf;
  pEndBuf   = (PCOSA_DML_MAPT_OPTION)((PCHAR)pOptionBuf + uiOptionBufLen);
  MAPT_LOG_INFO("<<<TRACE>>> Start : %p | End : %p", pStartBuf,pEndBuf);
  for ( ;pStartBuf + 1 <= pEndBuf; pStartBuf = pNxtOption )
  {
       uiOptionLen = ntohs(pStartBuf->OptLen);
       uiOption    = ntohs(pStartBuf->OptType);

       pCurOption  = (PUCHAR)(pStartBuf + 1);
       pNxtOption  = (PCOSA_DML_MAPT_OPTION)(pCurOption + uiOptionLen);
       MAPT_LOG_INFO("<<<TRACE>>> Cur : %p | Nxt : %p", pCurOption,pNxtOption);
       MAPT_LOG_INFO("<<<TRACE>>> Opt : %u | OpLen : %u", uiOption,uiOptionLen);

       /* option length field overrun */
       if ( pNxtOption > pEndBuf )
       {
            MAPT_LOG_ERROR("Malformed MAP options!");
            retStatus = STATUS_FAILURE; //STATUS_BAD_PAYLOAD
            break;
       }

       switch ( uiOption )
       {
            case MAPT_OPTION_S46_RULE:
            {
               UINT8 bytesLeftOut  = 0;
               UINT8 v6ByteLen     = 0;
               UINT8 v6BitLen      = 0;
               UCHAR v6Addr[BUFLEN_24];

               g_stMaptData.bFMR    = (*pCurOption & 0x01)? TRUE : FALSE;
               g_stMaptData.EaLen   =  *++pCurOption;
               g_stMaptData.RuleIPv4PrefixLen =  *++pCurOption;
               pCurOption++;

               snprintf (g_stMaptData.RuleIPv4Prefix, sizeof(g_stMaptData.RuleIPv4Prefix),
                               "%d.%d.%d.%d",
                               pCurOption[0], pCurOption[1], pCurOption[2], pCurOption[3]);

               g_stMaptData.RuleIPv6PrefixLen = *(pCurOption += 4);
               v6ByteLen = g_stMaptData.RuleIPv6PrefixLen / 8;
               v6BitLen  = g_stMaptData.RuleIPv6PrefixLen % 8;
               pCurOption++;

               memset (&v6Addr, 0, sizeof(v6Addr));
               memcpy (&v6Addr,  pCurOption, (v6ByteLen+(v6BitLen?1:0))*sizeof(CHAR));

               if ( v6BitLen )
               {
                    *((PCHAR)&v6Addr + v6ByteLen) &= 0xFF << (8 - v6BitLen);
               }

               memcpy (&g_stMaptData.RuleIPv6PrefixH, v6Addr,
                                                         (v6ByteLen+(v6BitLen?1:0))*sizeof(CHAR));
               CosaDmlMaptGetIPv6StringFromHex (g_stMaptData.RuleIPv6PrefixH,
                       g_stMaptData.RuleIPv6Prefix);
               MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.bFMR              : %d", g_stMaptData.bFMR);
               MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.EaLen             : %u", g_stMaptData.EaLen);
               MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.RuleIPv4PrefixLen : %u", g_stMaptData.RuleIPv4PrefixLen);
               MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.RuleIPv4Prefix    : %s", g_stMaptData.RuleIPv4Prefix);
               MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.RuleIPv6PrefixLen : %u", g_stMaptData.RuleIPv6PrefixLen);
               MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.RuleIPv6Prefix    : %s", g_stMaptData.RuleIPv6Prefix);
               MAPT_LOG_INFO("Parsing OPTION_S46_RULE Successful.");

               /*
                * check port parameter option:
                * prefix6_len, located at 8th byte in rule option, specifies
                * the length of following v6prefix. So the length of port
                * param option, if any, must be rule_opt_len minus 8 minus v6ByteLen minus (1 or 0)
                */
               bytesLeftOut = uiOptionLen - 8 - v6ByteLen - (v6BitLen?1:0);

               g_stMaptData.Ratio = 1 << (g_stMaptData.EaLen -
                                                    (BUFLEN_32 - g_stMaptData.RuleIPv4PrefixLen));
               /* RFC default */
               g_stMaptData.PsidOffset = 6;
               MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.Ratio             : %u", g_stMaptData.Ratio);
               MAPT_LOG_INFO("<<<TRACE>>> bytesLeftOut                   : %u", bytesLeftOut);
               if ( bytesLeftOut > 0 )
               {
                    /* this rule option includes port param option */
                    if ( bytesLeftOut == 8 ) // Only support one port param option per rule option
                    {
                         UINT16 uiSubOptionLen = 0;
                         UINT16 uiSubOption    = 0;

                         pCurOption += v6ByteLen + (v6BitLen?1:0);
                         uiSubOptionLen = ntohs(((PCOSA_DML_MAPT_OPTION)pCurOption)->OptLen);
                         uiSubOption    = ntohs(((PCOSA_DML_MAPT_OPTION)pCurOption)->OptType);

                         if ( uiSubOption == MAPT_OPTION_S46_PORT_PARAMS &&
                              uiSubOptionLen == 4 )
                         {
                              g_stMaptData.PsidOffset  = (*(pCurOption += uiSubOptionLen))?
                                                                 *pCurOption:6;
                              g_stMaptData.PsidLen     = *++pCurOption;

                              if ( !g_stMaptData.EaLen )
                              {
                                   g_stMaptData.Ratio = 1 << g_stMaptData.PsidLen;
                              }
                              /*
                               * RFC 7598: 4.5: 16 bits long. The first k bits on the left of
                               * this field contain the PSID binary value. The remaining (16 - k)
                               * bits on the right are padding zeros.
                               */
                              g_stMaptData.Psid = ntohs(*((PUINT16)++pCurOption)) >>
                                  (16 - g_stMaptData.PsidLen);
                              MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.Psid       : %u", g_stMaptData.Psid);
                              MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.PsidLen    : %u", g_stMaptData.PsidLen);
                              MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.PsidOffset : %u", g_stMaptData.PsidOffset);
                              MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.Ratio      : %u", g_stMaptData.Ratio);
                              MAPT_LOG_INFO("Parsing OPTION_S46_PORT_PARAM Successful.");
                         }
                         else
                         {
                              MAPT_LOG_WARNING("OPTION_S46_PORT_PARAM option length(%d) varies!"
                                              , uiSubOptionLen);
                         }
                    }
                    else
                    {
                         MAPT_LOG_WARNING("Port param option length(%d) not equal to 8"
                                         , bytesLeftOut);
                    }
               }
               break;
            }

            case MAPT_OPTION_S46_BR:
            {
               MAPT_LOG_WARNING("Parsing OPTION_S46_BR is not supported !");
               //retStatus = STATUS_NOT_SUPPORTED;
               break;
            }

            case MAPT_OPTION_S46_DMR:
            {
               UCHAR ipv6Addr[BUFLEN_24];

               g_stMaptData.BrIPv6PrefixLen = *pCurOption++;

               /* RFC 6052: 2.2: g_stMaptData.BrIPv6PrefixLen%8 must be 0! */
               memset (&ipv6Addr, 0, sizeof(ipv6Addr));
               memcpy (&ipv6Addr, pCurOption, g_stMaptData.BrIPv6PrefixLen/8);
               
               CosaDmlMaptGetIPv6StringFromHex (ipv6Addr, g_stMaptData.BrIPv6Prefix);
               MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.BrIPv6PrefixLen : %u", g_stMaptData.BrIPv6PrefixLen);
               MAPT_LOG_INFO("<<<TRACE>>> g_stMaptData.BrIPv6Prefix    : %s", g_stMaptData.BrIPv6Prefix);
               MAPT_LOG_INFO("Parsing OPTION_S46_DMR Successful.");
               break;
            }

            default:
               MAPT_LOG_ERROR("Unknown or unexpected MAP option : %d | option len : %d"
                             , uiOption, uiOptionLen);
               //retStatus = STATUS_NOT_SUPPORTED;
               break;
       }
  }

  /* Check a parameter from each mandatory options */
  if ( !g_stMaptData.RuleIPv6PrefixLen || !g_stMaptData.BrIPv6PrefixLen)
  {
       MAPT_LOG_ERROR("Mandatory mapt options are missing !");
       retStatus = STATUS_FAILURE;
  }

  return retStatus;
}


static RETURN_STATUS
CosaDmlMaptConvertStringToHexStream
(
    PUCHAR         pWriteBf,
    PUINT16        uiOptionBufLen
)
{
  PUCHAR pReadBf  = pWriteBf;
  MAPT_LOG_INFO("Entry");

  if ( !pWriteBf )
  {
       MAPT_LOG_ERROR("MAPT string buffer is empty !!");
       return STATUS_FAILURE;
  }

  if ( *pReadBf == '\'' && pReadBf++ ) {}

  if ( pReadBf[strlen((PCHAR)pReadBf)-1] == '\'' )
  {
       pReadBf[strlen((PCHAR)pReadBf)-1] = '\0';
  }

  MAPT_LOG_INFO("<<<Trace>>> pOptionBuf is %p : %s",pReadBf, pReadBf);
  while ( *pReadBf && *(pReadBf+1) )
  {
      if ( *pReadBf == ':' && pReadBf++ ) {}

      *pWriteBf  = (STRING_TO_HEX(*pReadBf) << 4) | STRING_TO_HEX(*(pReadBf+1));

      ++pWriteBf;
      ++pReadBf;
      ++pReadBf;
      ++*uiOptionBufLen;
  }
  *pWriteBf = '\0';
  MAPT_LOG_INFO("<<<Trace>>> BufLen : %d", *uiOptionBufLen);

  return STATUS_SUCCESS;
}

static RETURN_STATUS
CosaDmlMaptValidate
(
    PCHAR          pPdIPv6Prefix,
    UINT16         ui16PdPrefixLen,
    UINT16         ui16v6PrefixLen,
    UINT16         ui16v4PrefixLen
)
{
  UINT8 ui8v4BitIdxLen = 0, ui8PsidBitIdxLen = 0;
  UINT8 ui8EaLen       = 0 ;
  struct in6_addr ipv6Addr;

  MAPT_LOG_INFO("Entry");

  // V4 suffix bits length
  ui8v4BitIdxLen = BUFLEN_32 - ui16v4PrefixLen;

  // EA bits length
  ui8EaLen = ui16PdPrefixLen - ui16v6PrefixLen;

  // PSID length
  ui8PsidBitIdxLen = ui8EaLen - ui8v4BitIdxLen;

  MAPT_LOG_INFO("<<<Trace>>> ui8v4BitIdxLen(IPV4 Suffix Bits): %u", ui8v4BitIdxLen);
  MAPT_LOG_INFO("<<<Trace>>> ui8EaLen (EA bits)                    : %u", ui8EaLen);
  MAPT_LOG_INFO("<<<Trace>>> ui8PsidBitIdxLen(PSID length)         : %u", ui8PsidBitIdxLen);

  if (ui8EaLen != g_stMaptData.EaLen)
  {
       MAPT_LOG_INFO("Calculated EA-bits and received MAP EA-bits does not match!");
       return STATUS_FAILURE;
  }

  if ( ui16PdPrefixLen < ui16v6PrefixLen )
  {
       MAPT_LOG_ERROR("Invalid MAPT option, ui16PdPrefixLen(%d) < ui16v6PrefixLen(%d)",
               ui16PdPrefixLen, ui16v6PrefixLen);
       return STATUS_FAILURE;
  }

  if ( inet_pton(AF_INET6, pPdIPv6Prefix, &ipv6Addr) <= 0 )
  {
       MAPT_LOG_ERROR("Invalid IPv6 address = %s", pPdIPv6Prefix);
       return STATUS_FAILURE;
  }

   MAPT_LOG_INFO("MAPT validation successful.");
  return STATUS_SUCCESS;
}

/**
 * @brief Parses the MAPT option 95 response.
 *
 * This function processes the MAPT option 95 response, extracts the relevant information and updates ipc_dhcpv6_data_t struct with mapt details
 *
 * @param[in] pPdIPv6Prefix Pointer to the IPv6 prefix.
 * @param[in] pOptionBuf Pointer to the buffer containing the option 95 data.
 * @param[out] mapt Pointer to the structure where the parsed MAP-T data will be stored.

 *
 * @return ANSC_STATUS indicating the success or failure of the operation.
 */

ANSC_STATUS DhcpMgr_MaptParseOpt95Response
(
    const PCHAR          pPdIPv6Prefix,
    PUCHAR         pOptionBuf,
    ipc_map_data_t *mapt
)
{
  RETURN_STATUS ret = STATUS_SUCCESS;
  UINT16  uiOptionBufLen = 0;

  MAPT_LOG_INFO("Entry");

  /* Convert the received string buffer into hex stream */
  memset (&g_stMaptData, 0, sizeof(g_stMaptData));
  if ( CosaDmlMaptConvertStringToHexStream (pOptionBuf, &uiOptionBufLen) != STATUS_SUCCESS )
  {
       MAPT_LOG_ERROR("MAPT string buffer to HexStream conversion Failed !!");
       ret = STATUS_FAILURE;
  }

  /* Store IPv6 prefix and length */
  memcpy (&g_stMaptData.PdIPv6Prefix, pPdIPv6Prefix,
                                              (strchr(pPdIPv6Prefix, '/') - pPdIPv6Prefix));
  g_stMaptData.PdIPv6PrefixLen = strtol((strchr(pPdIPv6Prefix, '/') + 1), NULL, 10);
  MAPT_LOG_INFO("<<<Trace>>> Received PdIPv6Prefix : %s/%u", g_stMaptData.PdIPv6Prefix
                                                         , g_stMaptData.PdIPv6PrefixLen);
  if ( !ret && !g_stMaptData.PdIPv6Prefix[0] )
  {
       MAPT_LOG_ERROR("PdIPv6Prefix is NULL !!");
       ret = STATUS_FAILURE;
  }

  /* Parse the hex buffer for mapt options */
  if ( !ret && CosaDmlMaptParseResponse (pOptionBuf, uiOptionBufLen) != STATUS_SUCCESS )
  {
       MAPT_LOG_ERROR("MAPT Parsing Response Failed !!");
       ret = STATUS_FAILURE;
  }

    /* validate MAPT responce */
  if ( !ret && CosaDmlMaptValidate ( g_stMaptData.PdIPv6Prefix
                                                   , g_stMaptData.PdIPv6PrefixLen
                                                   , g_stMaptData.RuleIPv6PrefixLen
                                                   , g_stMaptData.RuleIPv4PrefixLen 
                                                  ) != STATUS_SUCCESS )
  {
       MAPT_LOG_ERROR("MAPT Psid and IPv4 Suffix Validation Failed !!");
       memset (&g_stMaptData, 0, sizeof(g_stMaptData));
       ret = STATUS_FAILURE;
  }

  if( ret == STATUS_SUCCESS )
  {
     //Fill Required MAP-T information
     mapt->v6Len = g_stMaptData.RuleIPv6PrefixLen;
     mapt->isFMR = g_stMaptData.bFMR ? TRUE : FALSE;
     mapt->eaLen = g_stMaptData.EaLen;
     mapt->v4Len = g_stMaptData.RuleIPv4PrefixLen;
     mapt->psidOffset = g_stMaptData.PsidOffset;
     mapt->psidLen = g_stMaptData.PsidLen;
     mapt->psid = g_stMaptData.Psid;
     mapt->iapdPrefixLen = g_stMaptData.PdIPv6PrefixLen;
     mapt->ratio = g_stMaptData.Ratio;

     snprintf (mapt->pdIPv6Prefix, BUFLEN_40, "%s", g_stMaptData.PdIPv6Prefix);
     snprintf (mapt->ruleIPv4Prefix, BUFLEN_40, "%s", g_stMaptData.RuleIPv4Prefix);
     snprintf (mapt->ruleIPv6Prefix, BUFLEN_40, "%s/%u", g_stMaptData.RuleIPv6Prefix
                                                                      , g_stMaptData.RuleIPv6PrefixLen);
     snprintf (mapt->brIPv6Prefix,   BUFLEN_40, "%s/%u", g_stMaptData.BrIPv6Prefix
                                                                      , g_stMaptData.BrIPv6PrefixLen);
     
     MAPT_LOG_INFO("MAPT configuration complete.\n");
  }

  return ((ret) ? ANSC_STATUS_FAILURE : ANSC_STATUS_SUCCESS);
}



