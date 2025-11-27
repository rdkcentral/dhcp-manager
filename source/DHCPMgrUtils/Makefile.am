##########################################################################
# Copyright 2020 Comcast Cable Communications Management, LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
##########################################################################
SUBDIRS = CustomOptions

AM_CFLAGS = -fno-exceptions -Wall -Wextra
AM_LDFLAGS = -lz -lpthread -lccsp_common $(DBUS_LIBS)

noinst_LTLIBRARIES = libCcspDhcpMgrUtils.la

libCcspDhcpMgrUtils_la_DEPENDENCIES = \
        ${top_builddir}/source/DHCPClientUtils/DHCPv4Client/libCcspDhcpMgr_Dhcpv4Client.la \
        ${top_builddir}/source/DHCPClientUtils/DHCPv6Client/libCcspDhcpMgr_Dhcpv6Client.la \
        ${top_builddir}/source/DHCPMgrUtils/CustomOptions/libcustomoptions.la 
        
libCcspDhcpMgrUtils_la_CPPFLAGS = -I$(top_srcdir)/source/DHCPMgrUtils/include \
                                  -I$(top_srcdir)/source/DHCPMgrUtils/CustomOptions \
                                  -I$(top_srcdir)/source/DHCPClientUtils/DHCPv4Client/ \
                                  -I$(top_srcdir)/source/DHCPClientUtils/DHCPv6Client/ \
                                  -I$(top_srcdir)/source/DHCPClientUtils/DHCPv4Client/include \
                                  -I$(top_srcdir)/source/DHCPClientUtils/ \
                                  -I$(top_srcdir)/source/TR-181/include \
                                  -I$(top_srcdir)/source/TR-181/middle_layer_src \
                                  -I$(top_srcdir)/source/DHCPMgrInterface/include \
                                  -I$(top_srcdir)/source/DHCPServerUtils/utils/include

libCcspDhcpMgrUtils_la_SOURCES = cosa_common_util.c helpers.c dhcpmgr_recovery_handler.c dhcpmgr_controller.c dhcp_lease_monitor_thrd.c dhcpmgr_v4_lease_handler.c dhcpmgr_v6_lease_handler.c

if FEATURE_SUPPORT_MAPT_NAT46
libCcspDhcpMgrUtils_la_SOURCES += dhcpmgr_map_apis.c
endif

libCcspDhcpMgrUtils_la_LIBADD = -lccsp_common -ltelemetry_msgsender -lulog -lsyscfg -lsysevent -ltime_conversion -lprint_uptime \
        ${top_builddir}/source/DHCPClientUtils/DHCPv4Client/libCcspDhcpMgr_Dhcpv4Client.la \
        ${top_builddir}/source/DHCPClientUtils/DHCPv6Client/libCcspDhcpMgr_Dhcpv6Client.la \
        -lhal_platform \
        ${top_builddir}/source/DHCPMgrUtils/CustomOptions/libcustomoptions.la

if CORE_NET_LIB_FEATURE_SUPPORT
libCcspDhcpMgrUtils_la_LIBADD += -lnet
endif

