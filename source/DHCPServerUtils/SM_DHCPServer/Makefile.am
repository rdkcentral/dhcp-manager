##########################################################################
# Generated for SM_DHCPServer (based on DHCPv6Server Makefile.am)
##########################################################################

AM_CFLAGS = -fno-exceptions -Wall -Wextra
AM_LDFLAGS = -lz -lpthread -lrt -lccsp_common $(DBUS_LIBS)

noinst_LTLIBRARIES = libSMDhcpServer.la

libSMDhcpServer_la_CPPFLAGS = \
	-I$(top_srcdir)/source/DHCPServerUtils/SM_DHCPServer \
	-I$(top_srcdir)/source/DHCPServerUtils/utils \
	-I$(top_srcdir)/source/DHCPServerUtils/utils/include \
	-I$(top_srcdir)/source/TR-181/include \
	-I$(top_srcdir)/source/TR-181/middle_layer_src \
	-I$(top_srcdir)/source/DHCPMgrUtils/include \
	-I$(top_srcdir)/source/TR-181/board_sbapi \
	-I$(top_srcdir)/source//DHCPClientUtils/DHCPv6Client/include \
	-I$(top_srcdir)/source/DHCPMgrInterface \
	-I$(top_srcdir)/source/DHCPMgrInterface/include \
    -I$(STAGING_INCDIR)/target-layer
libSMDhcpServer_la_SOURCES = \
	sm_DhcpMgr.c \
	sm_DhcpMgr_apis.c

libSMDhcpServer_la_LIBADD = -lccsp_common -ltelemetry_msgsender -lulog -lsyscfg -lsysevent -ltime_conversion -lprint_uptime -lnet -lDhcpServerApis
