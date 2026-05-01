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
#include <stdint.h>
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
static RETURN_STATUS CosaDmlMapParseResponse (PUCHAR pOptionBuf, UINT16 uiOptionBufLen);
static RETURN_STATUS CosaDmlMapGetIPv6StringFromHex (PUCHAR pIPv6AddrH, PCHAR pIPv6AddrS);
static RETURN_STATUS CosaDmlMapConvertStringToHexStream (PUCHAR pOptionBuf, PUINT16 uiOptionBufLen);
/*
 * Global definitions
 */
static COSA_DML_MAP_DATA   g_stMapData;

/*
 * Static function definitions
 */
static RETURN_STATUS
CosaDmlMapGetIPv6StringFromHex
(
    PUCHAR         pIPv6AddrH,
    PCHAR          pIPv6AddrS
)
{
  MAP_LOG_INFO("Entry");

  if ( !(pIPv6AddrH && pIPv6AddrS) )
  {
       MAP_LOG_ERROR("NULL inputs(%s - %s) for IPv6 hex to string conversion!",
                       pIPv6AddrH, pIPv6AddrS);
       return STATUS_FAILURE;
  }

  memset(pIPv6AddrS, 0, BUFLEN_40);
  

  if ( !inet_ntop(AF_INET6, pIPv6AddrH, pIPv6AddrS, BUFLEN_40) )
  {
       MAP_LOG_ERROR("Invalid IPv6 hex address");
       return STATUS_FAILURE;
  }

  return STATUS_SUCCESS;
}

static RETURN_STATUS
CosaDmlMapParseResponse
(
    PUCHAR         pOptionBuf,
    UINT16         uiOptionBufLen
)
{
  RETURN_STATUS retStatus = STATUS_SUCCESS;
  PCOSA_DML_MAP_OPTION  pStartBuf  = NULL;
  PCOSA_DML_MAP_OPTION  pNxtOption = NULL;
  PCOSA_DML_MAP_OPTION  pEndBuf    = NULL;
  UINT16  uiOptionLen = 0;
  UINT16  uiOption    = 0;
  PUCHAR  pCurOption  = 0;

  MAP_LOG_INFO("Entry");

  if ( !(pOptionBuf && *(pOptionBuf+1)) )
  {
       MAP_LOG_ERROR("MAP Option Response is NULL !!");
       return STATUS_FAILURE;
  }

  pStartBuf = (PCOSA_DML_MAP_OPTION)pOptionBuf;
  pEndBuf   = (PCOSA_DML_MAP_OPTION)((PCHAR)pOptionBuf + uiOptionBufLen);
  MAP_LOG_INFO("<<<TRACE>>> Start : %p | End : %p", pStartBuf,pEndBuf);
  for ( ;pStartBuf + 1 <= pEndBuf; pStartBuf = pNxtOption )
  {
       uiOptionLen = ntohs(pStartBuf->OptLen);
       uiOption    = ntohs(pStartBuf->OptType);

       pCurOption  = (PUCHAR)(pStartBuf + 1);
       pNxtOption  = (PCOSA_DML_MAP_OPTION)(pCurOption + uiOptionLen);
       MAP_LOG_INFO("<<<TRACE>>> Cur : %p | Nxt : %p", pCurOption,pNxtOption);
       MAP_LOG_INFO("<<<TRACE>>> Opt : %u | OpLen : %u", uiOption,uiOptionLen);

       /* option length field overrun */
       if ( pNxtOption > pEndBuf )
       {
            MAP_LOG_ERROR("Malformed MAP options!");
            retStatus = STATUS_FAILURE; //STATUS_BAD_PAYLOAD
            break;
       }

       switch ( uiOption )
       {
            case MAP_OPTION_S46_RULE:
            {
               UINT8 bytesLeftOut  = 0;
               UINT8 v6ByteLen     = 0;
               UINT8 v6BitLen      = 0;
               UCHAR v6Addr[BUFLEN_24];

               g_stMapData.bFMR    = (*pCurOption & 0x01)? TRUE : FALSE;
               g_stMapData.EaLen   =  *++pCurOption;
               g_stMapData.RuleIPv4PrefixLen =  *++pCurOption;
               pCurOption++;

               snprintf (g_stMapData.RuleIPv4Prefix, sizeof(g_stMapData.RuleIPv4Prefix),
                               "%d.%d.%d.%d",
                               pCurOption[0], pCurOption[1], pCurOption[2], pCurOption[3]);

               g_stMapData.RuleIPv6PrefixLen = *(pCurOption += 4);
               v6ByteLen = g_stMapData.RuleIPv6PrefixLen / 8;
               v6BitLen  = g_stMapData.RuleIPv6PrefixLen % 8;
               pCurOption++;

               memset (&v6Addr, 0, sizeof(v6Addr));
               memcpy (&v6Addr,  pCurOption, (v6ByteLen+(v6BitLen?1:0))*sizeof(CHAR));

               if ( v6BitLen )
               {
                    *((PCHAR)&v6Addr + v6ByteLen) &= 0xFF << (8 - v6BitLen);
               }

               memcpy (&g_stMapData.RuleIPv6PrefixH, v6Addr,
                                                         (v6ByteLen+(v6BitLen?1:0))*sizeof(CHAR));
               CosaDmlMapGetIPv6StringFromHex (g_stMapData.RuleIPv6PrefixH,
                       g_stMapData.RuleIPv6Prefix);
               MAP_LOG_INFO("<<<TRACE>>> g_stMapData.bFMR              : %d", g_stMapData.bFMR);
               MAP_LOG_INFO("<<<TRACE>>> g_stMapData.EaLen             : %u", g_stMapData.EaLen);
               MAP_LOG_INFO("<<<TRACE>>> g_stMapData.RuleIPv4PrefixLen : %u", g_stMapData.RuleIPv4PrefixLen);
               MAP_LOG_INFO("<<<TRACE>>> g_stMapData.RuleIPv4Prefix    : %s", g_stMapData.RuleIPv4Prefix);
               MAP_LOG_INFO("<<<TRACE>>> g_stMapData.RuleIPv6PrefixLen : %u", g_stMapData.RuleIPv6PrefixLen);
               MAP_LOG_INFO("<<<TRACE>>> g_stMapData.RuleIPv6Prefix    : %s", g_stMapData.RuleIPv6Prefix);
               MAP_LOG_INFO("Parsing OPTION_S46_RULE Successful.");

               /*
                * check port parameter option:
                * prefix6_len, located at 8th byte in rule option, specifies
                * the length of following v6prefix. So the length of port
                * param option, if any, must be rule_opt_len minus 8 minus v6ByteLen minus (1 or 0)
                */
               bytesLeftOut = uiOptionLen - 8 - v6ByteLen - (v6BitLen?1:0);

               g_stMapData.Ratio = 1 << (g_stMapData.EaLen -
                                                    (BUFLEN_32 - g_stMapData.RuleIPv4PrefixLen));
               /* RFC default */
               g_stMapData.PsidOffset = 6;
               MAP_LOG_INFO("<<<TRACE>>> g_stMapData.Ratio             : %u", g_stMapData.Ratio);
               MAP_LOG_INFO("<<<TRACE>>> bytesLeftOut                   : %u", bytesLeftOut);
               if ( bytesLeftOut > 0 )
               {
                    /* this rule option includes port param option */
                    if ( bytesLeftOut == 8 ) // Only support one port param option per rule option
                    {
                         UINT16 uiSubOptionLen = 0;
                         UINT16 uiSubOption    = 0;

                         pCurOption += v6ByteLen + (v6BitLen?1:0);
                         uiSubOptionLen = ntohs(((PCOSA_DML_MAP_OPTION)pCurOption)->OptLen);
                         uiSubOption    = ntohs(((PCOSA_DML_MAP_OPTION)pCurOption)->OptType);

                         if ( uiSubOption == MAP_OPTION_S46_PORT_PARAMS &&
                              uiSubOptionLen == 4 )
                         {
                              g_stMapData.PsidOffset  = (*(pCurOption += uiSubOptionLen))?
                                                                 *pCurOption:6;
                              g_stMapData.PsidLen     = *++pCurOption;

                              if ( !g_stMapData.EaLen )
                              {
                                   g_stMapData.Ratio = 1 << g_stMapData.PsidLen;
                              }
                              /*
                               * RFC 7598: 4.5: 16 bits long. The first k bits on the left of
                               * this field contain the PSID binary value. The remaining (16 - k)
                               * bits on the right are padding zeros.
                               */
                              g_stMapData.Psid = ntohs(*((PUINT16)++pCurOption)) >>
                                  (16 - g_stMapData.PsidLen);
                              MAP_LOG_INFO("<<<TRACE>>> g_stMapData.Psid       : %u", g_stMapData.Psid);
                              MAP_LOG_INFO("<<<TRACE>>> g_stMapData.PsidLen    : %u", g_stMapData.PsidLen);
                              MAP_LOG_INFO("<<<TRACE>>> g_stMapData.PsidOffset : %u", g_stMapData.PsidOffset);
                              MAP_LOG_INFO("<<<TRACE>>> g_stMapData.Ratio      : %u", g_stMapData.Ratio);
                              MAP_LOG_INFO("Parsing OPTION_S46_PORT_PARAM Successful.");
                         }
                         else
                         {
                              MAP_LOG_WARNING("OPTION_S46_PORT_PARAM option length(%d) varies!"
                                              , uiSubOptionLen);
                         }
                    }
                    else
                    {
                         MAP_LOG_WARNING("Port param option length(%d) not equal to 8"
                                         , bytesLeftOut);
                    }
               }
               break;
            }

            case MAP_OPTION_S46_BR:
            {
               UCHAR br_ipv6Addr[BUFLEN_24];  // Buffer to hold the BR IPv6 address

               //Directly copy the 16-byte IPv6 address from the current option buffer
               memset(&br_ipv6Addr, 0, sizeof(br_ipv6Addr));
               memcpy(&br_ipv6Addr, pCurOption, 16);

               CosaDmlMapGetIPv6StringFromHex(br_ipv6Addr, g_stMapData.BrIPv6Prefix);
               MAP_LOG_INFO("<<<TRACE>>> g_stMapData.BrIPv6Prefix  : %s", g_stMapData.BrIPv6Prefix);
               MAP_LOG_INFO("Parsing MAP_OPTION_S46_BR Successful.");
               break;
            }

            case MAP_OPTION_S46_DMR:
            {
               UCHAR ipv6Addr[BUFLEN_24];

               g_stMapData.BrIPv6PrefixLen = *pCurOption++;

               /* RFC 6052: 2.2: g_stMapData.BrIPv6PrefixLen%8 must be 0! */
               memset (&ipv6Addr, 0, sizeof(ipv6Addr));
               memcpy (&ipv6Addr, pCurOption, g_stMapData.BrIPv6PrefixLen/8);
               
               CosaDmlMapGetIPv6StringFromHex (ipv6Addr, g_stMapData.BrIPv6Prefix);
               MAP_LOG_INFO("<<<TRACE>>> g_stMapData.BrIPv6PrefixLen : %u", g_stMapData.BrIPv6PrefixLen);
               MAP_LOG_INFO("<<<TRACE>>> g_stMapData.BrIPv6Prefix    : %s", g_stMapData.BrIPv6Prefix);
               MAP_LOG_INFO("Parsing OPTION_S46_DMR Successful.");
               break;
            }

            default:
               MAP_LOG_ERROR("Unknown or unexpected MAP option : %d | option len : %d"
                             , uiOption, uiOptionLen);
               //retStatus = STATUS_NOT_SUPPORTED;
               break;
       }
  }

#if defined(FEATURE_MAPT) || defined(FEATURE_SUPPORT_MAPT_NAT46)
  /* Check a parameter from each mandatory options */
  if ( !g_stMapData.RuleIPv6PrefixLen || !g_stMapData.BrIPv6PrefixLen)
  {
       MAP_LOG_ERROR("Mandatory mapt options are missing !");
       retStatus = STATUS_FAILURE;
  }
#endif

  return retStatus;
}


static RETURN_STATUS
CosaDmlMapConvertStringToHexStream
(
    PUCHAR         pWriteBf,
    PUINT16        uiOptionBufLen
)
{
  PUCHAR pReadBf  = pWriteBf;
  MAP_LOG_INFO("Entry");

  if ( !pWriteBf )
  {
       MAP_LOG_ERROR("MAP string buffer is empty !!");
       return STATUS_FAILURE;
  }

  if ( *pReadBf == '\'' && pReadBf++ ) {}

  if ( pReadBf[strlen((PCHAR)pReadBf)-1] == '\'' )
  {
       pReadBf[strlen((PCHAR)pReadBf)-1] = '\0';
  }

  MAP_LOG_INFO("<<<Trace>>> pOptionBuf is %p : %s",pReadBf, pReadBf);
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
  MAP_LOG_INFO("<<<Trace>>> BufLen : %d", *uiOptionBufLen);

  return STATUS_SUCCESS;
}

static inline uint64_t bitmask(unsigned n)
{
    if (n == 0)  return 0;
    if (n >= 64) return UINT64_MAX;
    return (UINT64_C(1) << n) - 1;
}

/*
                :           :           ___/       :
                |  p bits   |          /  q bits   :
                +-----------+         +------------+
                |IPv4 suffix|         |Port Set ID |
                +-----------+         +------------+
                 \          /    ____/    ________/
                   \       :  __/   _____/
                     \     : /     /
 |     n bits         |  o bits   | s bits  |   128-n-o-s bits      |
 +--------------------+-----------+---------+------------+----------+
 |  Rule IPv6 prefix  |  EA bits  |subnet ID|     interface ID      |
 +--------------------+-----------+---------+-----------------------+
 |<---  End-user IPv6 prefix  --->|

EA-bits:
+-------------------+---------+
|IPV4 Address Suffix|   PSID  |
+-------------------+---------+
|--------p----------|----q----+
|--------------o--------------|
*/

static RETURN_STATUS
CosaDmlMapComputePsidAndIPv4Suffix
(
    PCHAR          pPdIPv6Prefix,
    UINT16         pdPrefixLen,
    UINT16         v6PrefixLen,
    UINT16         v4PrefixLen,
    PUINT16        pPsid,
    PUINT16        pPsidLen,
    PUINT32        pIPv4Suffix
)
{
  struct in6_addr ipv6Addr;

  MAP_LOG_INFO("Entry");

  if (!pPdIPv6Prefix || !pPsid || !pPsidLen || !pIPv4Suffix)
  {
      MAP_LOG_ERROR("NULL parameter passed to MAP EA computation");
      return STATUS_FAILURE;
  }

  if ( pdPrefixLen < v6PrefixLen )
  {
      MAP_LOG_ERROR("Invalid MAP option, PD Prefix(%d) < IPv6 Prefix(%d)",
               pdPrefixLen, v6PrefixLen);
      return STATUS_FAILURE;
  }

  if (pdPrefixLen > 128 || v6PrefixLen > 128)
  {
      MAP_LOG_ERROR("Invalid IPv6 prefix length: pdPrefixLen=%u v6PrefixLen=%u",
              pdPrefixLen, v6PrefixLen);
      return STATUS_FAILURE;
  }

  if ( inet_pton(AF_INET6, pPdIPv6Prefix, &ipv6Addr) <= 0 )
  {
      MAP_LOG_ERROR("Invalid IPv6 address = %s", pPdIPv6Prefix);
      return STATUS_FAILURE;
  }

  if (g_stMapData.EaLen > 0)
  {
      UINT8 v4SuffixBitsLen = 0;
      UINT8 psidBitsLen = 0;
      UINT8 eaBitsLen = 0 ;

      // V4 suffix bits length
      v4SuffixBitsLen = BUFLEN_32 - v4PrefixLen;

      // EA bits length
      eaBitsLen = pdPrefixLen - v6PrefixLen;

      if (eaBitsLen < v4SuffixBitsLen)
      {
          MAP_LOG_ERROR("Invalid MAP rule: EA bits length %u is smaller than IPv4 suffix bits length %u", eaBitsLen, v4SuffixBitsLen);
          return STATUS_FAILURE;
      }

      // PSID length
      psidBitsLen = eaBitsLen - v4SuffixBitsLen;

      MAP_LOG_INFO("Calculated EA bits length   : %u", eaBitsLen);
      MAP_LOG_INFO("IPv4 suffix bits length (p) : %u", v4SuffixBitsLen);
      MAP_LOG_INFO("PSID bits length (q)        : %u", psidBitsLen);

      if (eaBitsLen != g_stMapData.EaLen)
      {
          MAP_LOG_ERROR("Calculated EA-bits and received MAP EA-bits does not match!");
          return STATUS_FAILURE;
      }

      if (eaBitsLen > 64)
      {
          MAP_LOG_ERROR("EA bits length %u exceeds supported limit (64 bits)", eaBitsLen);
          return STATUS_FAILURE;
      }

      if (psidBitsLen > 16)
      {
          MAP_LOG_ERROR("Invalid PSID length %u (must be <= 16)", psidBitsLen);
          return STATUS_FAILURE;
      }

      /*
       * Extract EA bits MSB-first from IPv6 delegated prefix
       */
      uint64_t eaBits = 0;
      UINT8 bitPos = v6PrefixLen;
      for (unsigned i = 0; i < eaBitsLen; i++)
      {
          UINT8 byteIndex = bitPos / 8;
          UINT8 bitIndex = 7 - (bitPos % 8);
          eaBits = (eaBits << 1) | ((ipv6Addr.s6_addr[byteIndex] >> bitIndex) & 0x01);
          bitPos++;
      }

      MAP_LOG_INFO("Extracted EA bits : 0x%llX", (unsigned long long)eaBits);

      /*
       * Split EA bits into IPv4 suffix and PSID
       */
      if (v4SuffixBitsLen > 0)
      {
          *pIPv4Suffix = (eaBits >> psidBitsLen) & bitmask(v4SuffixBitsLen);
      } 
      else 
      {
          *pIPv4Suffix = 0;
          MAP_LOG_INFO("No IPv4 suffix bits, set to 0");
      }

      if (psidBitsLen > 0)
      {
          *pPsid = eaBits & bitmask(psidBitsLen);
      }
      else
      {
          *pPsid = 0;
          MAP_LOG_INFO("No PSID bits, set to 0");
      }

      *pPsidLen = psidBitsLen;

      MAP_LOG_INFO("Computed IPv4 Suffix : %u", *pIPv4Suffix);
      MAP_LOG_INFO("Computed PSID        : %u", *pPsid);
      MAP_LOG_INFO("Computed PSID Length : %u", *pPsidLen);
  }
  else
  {
      MAP_LOG_INFO("EA bits not set, using PSID/PSIDLen from received OPTION_S46_PORT_PARAM");
  }
  return STATUS_SUCCESS;
}

/**
 * @brief Parses the MAPE/MAPT options 94 & 95 response respectively.
 *
 * This function processes the MAPE/MAPT options response, extracts the relevant information and updates ipc_map_data_t struct with map details
 *
 * @param[in] pPdIPv6Prefix Pointer to the IPv6 prefix.
 * @param[in] pOptionBuf Pointer to the buffer containing the option 94/95 data.
 * @param[out] map Pointer to the structure where the parsed MAP data will be stored.

 *
 * @return ANSC_STATUS indicating the success or failure of the operation.
 */

ANSC_STATUS DhcpMgr_MapParseOptResponse
(
    const PCHAR          pPdIPv6Prefix,
    PUCHAR               pOptionBuf,
    ipc_map_data_t       *map
)
{
  RETURN_STATUS ret = STATUS_SUCCESS;
  UINT16  uiOptionBufLen = 0;

  MAP_LOG_INFO("Entry");

  /* Convert the received string buffer into hex stream */
  memset (&g_stMapData, 0, sizeof(g_stMapData));
  if ( CosaDmlMapConvertStringToHexStream (pOptionBuf, &uiOptionBufLen) != STATUS_SUCCESS )
  {
       MAP_LOG_ERROR("MAP string buffer to HexStream conversion Failed !!");
       ret = STATUS_FAILURE;
  }

  /* Store IPv6 prefix and length */
  memcpy (&g_stMapData.PdIPv6Prefix, pPdIPv6Prefix,
                                              (strchr(pPdIPv6Prefix, '/') - pPdIPv6Prefix));
  g_stMapData.PdIPv6PrefixLen = strtol((strchr(pPdIPv6Prefix, '/') + 1), NULL, 10);
  MAP_LOG_INFO("<<<Trace>>> Received PdIPv6Prefix : %s/%u", g_stMapData.PdIPv6Prefix
                                                         , g_stMapData.PdIPv6PrefixLen);
  if ( !ret && !g_stMapData.PdIPv6Prefix[0] )
  {
       MAP_LOG_ERROR("PdIPv6Prefix is NULL !!");
       ret = STATUS_FAILURE;
  }

  /* Parse the hex buffer for mapt options */
  if ( !ret && CosaDmlMapParseResponse (pOptionBuf, uiOptionBufLen) != STATUS_SUCCESS )
  {
       MAP_LOG_ERROR("MAP Parsing Response Failed !!");
       ret = STATUS_FAILURE;
  }

    /* validate MAPT/MAPE response */
  if ( !ret && CosaDmlMapComputePsidAndIPv4Suffix ( g_stMapData.PdIPv6Prefix
                                                   , g_stMapData.PdIPv6PrefixLen
                                                   , g_stMapData.RuleIPv6PrefixLen
                                                   , g_stMapData.RuleIPv4PrefixLen
                                                   , &g_stMapData.Psid
                                                   , &g_stMapData.PsidLen
                                                   , &g_stMapData.IPv4Suffix
                                                  ) != STATUS_SUCCESS )
  {
       MAP_LOG_ERROR("MAP Psid and IPv4 Suffix Computation Failed !!");
       memset (&g_stMapData, 0, sizeof(g_stMapData));
       ret = STATUS_FAILURE;
  }

  if( ret == STATUS_SUCCESS )
  {
     //Fill Required MAP information
     map->v6Len = g_stMapData.RuleIPv6PrefixLen;
     map->isFMR = g_stMapData.bFMR ? TRUE : FALSE;
     map->eaLen = g_stMapData.EaLen;
     map->v4Len = g_stMapData.RuleIPv4PrefixLen;
     map->psidOffset = g_stMapData.PsidOffset;
     map->psidLen = g_stMapData.PsidLen;
     map->psid = g_stMapData.Psid;
     map->iapdPrefixLen = g_stMapData.PdIPv6PrefixLen;
     map->ratio = g_stMapData.Ratio;

     snprintf (map->pdIPv6Prefix, BUFLEN_40, "%s", g_stMapData.PdIPv6Prefix);
     snprintf (map->ruleIPv4Prefix, BUFLEN_40, "%s", g_stMapData.RuleIPv4Prefix);
     snprintf (map->ruleIPv6Prefix, BUFLEN_40, "%s/%u", g_stMapData.RuleIPv6Prefix
                                                                      , g_stMapData.RuleIPv6PrefixLen);
     snprintf (map->brIPv6Prefix,   BUFLEN_40, "%s/%u", g_stMapData.BrIPv6Prefix
                                                                      , g_stMapData.BrIPv6PrefixLen);
     
     MAP_LOG_INFO("MAP Options parsing complete!\n");
  }

  return ((ret) ? ANSC_STATUS_FAILURE : ANSC_STATUS_SUCCESS);
}
