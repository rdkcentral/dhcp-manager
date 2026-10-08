#!/bin/sh
####################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:

#  Copyright 2018 RDK Management

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#######################################################################################
if [ -e /etc/log_timestamp.sh ]
then
    . /etc/log_timestamp.sh
else
    echo_t()
    {
	    echo "$(date +"%y%m%d-%T.%6N") $1"
    }
fi

LOG_FILE=/rdklogs/logs/Consolelog.txt.0

echo_t "[DHCP Manager PSM Check] Running the script to check psm records" >> $LOG_FILE

virtifcount=$(psmcli get dmsb.wanmanager.if.1.VirtualInterfaceifcount)
[ "$virtifcount" -eq 0 ] && { echo_t "[DHCP Manager PSM Check] virtual interface count is 0" >> $LOG_FILE; exit 1; }

echo_t "[DHCP Manager PSM Check] Number of virtual interfaces is ${virtifcount}" >> $LOG_FILE

cnt=1
while [ "$cnt" -le "$virtifcount" ]
do
	psmDHCPv4=$(psmcli get dmsb.wanmanager.if.1.VirtualInterface.${cnt}.IP.DHCPV4Interface)
	psmDHCPv6=$(psmcli get dmsb.wanmanager.if.1.VirtualInterface.${cnt}.IP.DHCPV6Interface)

	if [ -z "$psmDHCPv4" ]; then
		psmcli set dmsb.wanmanager.if.1.VirtualInterface.${cnt}.IP.DHCPV4Interface Device.DHCPv4.Client.1 2> /dev/null
		echo_t "[DHCP Manager PSM Check] PSM record \"dmsb.wanmanager.if.1.VirtualInterface.${cnt}.IP.DHCPV4Interface\" is set with \"Device.DHCPv4.Client.1\"" >> $LOG_FILE
	else
		echo_t "[DHCP Manager PSM Check] PSM record \"dmsb.wanmanager.if.1.VirtualInterface.${cnt}.IP.DHCPV4Interface\" already exists with value: \"${psmDHCPv4}\"" >> $LOG_FILE
	fi

	if [ -z "$psmDHCPv6" ]; then
		psmcli set dmsb.wanmanager.if.1.VirtualInterface.${cnt}.IP.DHCPV6Interface Device.DHCPv6.Client.1 2> /dev/null
		echo_t "[DHCP Manager PSM Check] PSM record \"dmsb.wanmanager.if.1.VirtualInterface.${cnt}.IP.DHCPV6Interface\" is set with \"Device.DHCPv6.Client.1\"" >> $LOG_FILE
	else
		echo_t "[DHCP Manager PSM Check] PSM record \"dmsb.wanmanager.if.1.VirtualInterface.${cnt}.IP.DHCPV6Interface\" already exists with value: \"${psmDHCPv6}\"" >> $LOG_FILE
	fi
	cnt=$((cnt + 1))
done
echo_t "[DHCP Manager PSM Check] End Of the Script" >> $LOG_FILE

