#!/bin/sh
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2024 Deutsche Telekom AG.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

log() {
    local level="$1"; shift
    local message="$*"
    local timestamp
    local log_file
    timestamp=$(date +'%Y-%m-%d %H:%M:%S')
    log_file="/tmp/notify_dhcp.log"
    echo "$timestamp [$level] $message" | tee -a "$log_file"
}

# Start logging
log INFO "Script started."

if [ "$v6Routes_Change" == "1" ]; then
    if [ "$DEFAULT_IPV6_ROUTES_COUNT" -gt 0 ]; then # Case when ipv6 default routes present in gateway
        sysevent set ipv6_wan_defrtr 1
    elif [ "$DEFAULT_IPV6_ROUTES_COUNT" -eq 0 ]; then # Case when no ipv6 default routes present in gateway
        sysevent set ipv6_wan_defrtr 0
    fi
    log INFO "DHCP Manager notify script. Starting zebra"
    sysevent set zebra-restart
fi

if [ "$AFlag" == "0" ]; then
   sysevent set a_flag_off false
fi

if [ "$FIFO_CREATED" == "1" ] && [ "$RA_Flags_Change" == "1" ] && [ "$AFlag" == "1" ]; then
    # Send notification to fifo
    printf "%s %s %s %-19s" "ra-flags" $MFlag $OFlag $RAIFNAME > /tmp/ra_common_fifo
    sysevent set a_flag_off true
fi

