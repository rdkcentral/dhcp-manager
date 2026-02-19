# DHCP Lease Handlers

## Overview

The DHCP Lease Handlers are specialized components responsible for processing lease information received from DHCP clients and updating the system accordingly. The implementation consists of separate handlers for DHCPv4 and DHCPv6, each tailored to the specific requirements and characteristics of their respective protocols.

## Architecture

The lease handlers implement a multi-layered processing architecture:

```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   Lease Monitor     │────►│  Lease Handlers     │────►│  System Updates     │
│                     │     │                     │     │                     │
│ • Message Router    │     │ • DHCPv4 Handler    │     │ • TR-181 DML        │
│ • Protocol Detect   │     │ • DHCPv6 Handler    │     │ • Network Config    │
│ • Data Validation   │     │ • Lease Processing  │     │ • Event Generation  │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
                                      │
                                      ▼
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   Recovery System   │◄────│  Persistence Layer  │────►│  External Systems   │
│                     │     │                     │     │                     │
│ • State Backup      │     │ • Lease Storage     │     │ • WAN Manager       │
│ • Crash Recovery    │     │ • Configuration     │     │ • System Events     │
│ • Data Integrity    │     │ • Recovery Data     │     │ • RBUS/DBUS         │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
```

## DHCPv4 Lease Handler

### Files
- **Source**: `source/DHCPMgrUtils/dhcpmgr_v4_lease_handler.c`
- **Related**: Functions in `dhcpmgr_controller.c`

### Key Functions

#### `DhcpMgr_ProcessV4Lease(PCOSA_DML_DHCPC_FULL pDhcpc)`

**Purpose**: Processes new DHCPv4 leases and updates system state

**Processing Flow**:
1. **Lease Retrieval**: Extract lease from pending queue
2. **Comparison**: Compare with current lease to detect changes
3. **Validation**: Verify lease data integrity and consistency
4. **TR-181 Update**: Update data model with new lease information
5. **Network Configuration**: Apply network settings to interface
6. **Event Generation**: Generate system events for lease changes
7. **Persistence**: Store lease data for recovery purposes
8. **Cleanup**: Free processed lease memory

**Critical Processing Steps**:
```c
while (pDhcpc->NewLeases != NULL) {
    DHCPv4_PLUGIN_MSG *processingLease = pDhcpc->NewLeases;
    pDhcpc->NewLeases = processingLease->next;
    
    // Process lease data
    if (lease_changed) {
        DhcpMgr_updateDHCPv4DML(pDhcpc);
        configureNetworkInterface(pDhcpc);
        DHCPMgr_storeDhcpv4Lease(pDhcpc);
    }
    
    free(processingLease);
}
```

#### `DhcpMgr_updateDHCPv4DML(PCOSA_DML_DHCPC_FULL pDhcpc)`

**Purpose**: Updates TR-181 Data Model with DHCPv4 lease information

**Updated Parameters**:
- **IP Address**: `pDhcpc->Info.IPAddress`
- **Subnet Mask**: `pDhcpc->Info.SubnetMask`
- **Default Gateway**: `pDhcpc->Info.IPRouters`
- **DNS Servers**: `pDhcpc->Info.DNSServers`
- **DHCP Server**: `pDhcpc->Info.DHCPServer`
- **Lease Time**: `pDhcpc->Info.LeaseTimeRemaining`
- **Status**: `pDhcpc->Info.Status`

**Special Handling**:
- **MTA Options**: Support for EROUTER_DHCP_OPTION_MTA
- **Custom Options**: Vendor-specific option processing
- **Status Validation**: Ensure lease status consistency

#### `configureNetworkInterface(PCOSA_DML_DHCPC_FULL pDhcpc)`

**Purpose**: Applies network configuration to the interface

**Configuration Steps**:
1. **IP Address Assignment**: Set interface IP address
2. **Subnet Configuration**: Apply subnet mask
3. **Gateway Setup**: Configure default gateway
4. **DNS Configuration**: Set DNS server addresses
5. **Route Management**: Update routing table
6. **Broadcast Address**: Calculate and set broadcast address

**Network Operations**:
```c
// Example network configuration
struct ifreq ifr;
struct sockaddr_in *addr;

// Set IP address
addr = (struct sockaddr_in *)&ifr.ifr_addr;
addr->sin_family = AF_INET;
addr->sin_addr.s_addr = inet_addr(current->ipAddr);
ioctl(sockfd, SIOCSIFADDR, &ifr);
```

#### `set_inf_sysevents()`

**Purpose**: Updates system events based on lease changes

**Event Updates**:
- **DHCP Server ID**: Interface-specific server identification
- **Lease Time**: Remaining lease duration
- **DHCP State**: Current client state (bound, renewing, etc.)
- **Start Time**: Lease acquisition timestamp

**Event Format**:
```c
// Example sysevent updates
snprintf(syseventParam, sizeof(syseventParam), "ipv4_%s_dhcp_server", interface);
ifl_set_event(syseventParam, newLease->dhcpServerId);
```

#### `DhcpMgr_clearDHCPv4Lease(PCOSA_DML_DHCPC_FULL pDhcpc)`

**Purpose**: Clears current lease information and resets interface

**Cleanup Operations**:
1. **Memory Cleanup**: Free current lease structure
2. **TR-181 Reset**: Clear data model parameters
3. **Interface Reset**: Remove IP configuration
4. **Event Cleanup**: Generate lease release events
5. **State Reset**: Reset client operational state

## DHCPv6 Lease Handler

### Files
- **Source**: `source/DHCPMgrUtils/dhcpmgr_v6_lease_handler.c`
- **Related**: Functions in `dhcpmgr_controller.c`

### Key Functions

#### `DhcpMgr_ProcessV6Lease(PCOSA_DML_DHCPCV6_FULL pDhcp6c)`

**Purpose**: Processes new DHCPv6 leases with support for IANA and IAPD

**IPv6-Specific Processing**:
1. **Dual Association Handling**: Process IANA and IAPD separately
2. **Lease Comparison**: Detect changes in address/prefix assignments
3. **Validation**: Verify IPv6 address formats and prefixes
4. **TR-181 Update**: Update IPv6-specific data model parameters
5. **Network Configuration**: Apply IPv6 addresses and routes for the interface
6. **Event Generation**: Generate IPv6-specific system events
7. **Persistence**: Store IPv6 lease data for recovery

**Processing Logic**:
```c
while (pDhcp6c->NewLeases != NULL) {
    DHCPv6_PLUGIN_MSG *processingLease = pDhcp6c->NewLeases;
    
    // Compare with current lease
    if (!compare_dhcpv6_plugin_msg(pDhcp6c->currentLease, processingLease)) {
        leaseChanged = true;
        
        // Update current lease
        if (pDhcp6c->currentLease) {
            free(pDhcp6c->currentLease);
        }
        pDhcp6c->currentLease = processingLease;
        
        // Configure system
        configureNetworkInterface(pDhcp6c);
        ConfigureIpv6Sysevents(pDhcp6c);
        DHCPMgr_storeDhcpv6Lease(pDhcp6c);
    }
}
```

#### `compare_dhcpv6_plugin_msg()`

**Purpose**: Compares two DHCPv6 lease messages to detect changes

**Comparison Fields**:
- **Expiration Status**: `isExpired` flag
- **IANA Information**: Non-temporary address data
- **IAPD Information**: Prefix delegation data
- **Timing Parameters**: T1, T2, preferred/valid lifetimes
- **Server Information**: DHCP server details
- **Options**: Custom DHCPv6 options

**Return Value**: `true` if leases are identical, `false` if changes detected

#### `ConfigureIpv6Sysevents(PCOSA_DML_DHCPCV6_FULL pDhcp6c)`

**Purpose**: Sets IPv6-specific system events

**IAPD Events** (Prefix Delegation):
- **Prefix**: `tr_%s_dhcpv6_client_v6pref`
- **IAID**: `tr_%s_dhcpv6_client_pref_iaid`
- **T1 Timer**: `tr_%s_dhcpv6_client_pref_t1`
- **T2 Timer**: `tr_%s_dhcpv6_client_pref_t2`
- **Preferred Time**: `tr_%s_dhcpv6_client_pref_pretm`
- **Valid Time**: `tr_%s_dhcpv6_client_pref_vldtm`

**IANA Events** (Address Assignment):
- **Address**: `tr_%s_dhcpv6_client_v6addr`
- **IAID**: `tr_%s_dhcpv6_client_addr_iaid`
- **T1 Timer**: `tr_%s_dhcpv6_client_addr_t1`
- **T2 Timer**: `tr_%s_dhcpv6_client_addr_t2`
- **Preferred Time**: `tr_%s_dhcpv6_client_addr_pretm`
- **Valid Time**: `tr_%s_dhcpv6_client_addr_vldtm`

#### `configureNetworkInterface(PCOSA_DML_DHCPCV6_FULL pDhcp6c)`

**Purpose**: Applies IPv6 network configuration

**IPv6 Configuration**:
1. **Address Assignment**: Add IPv6 addresses to interface
2. **Prefix Configuration**: (Handled by WAN Manager)
3. **Route Setup**: (Handled by WAN Manager)
4. **Neighbor Discovery**: (Handled by WAN Manager)
5. **DNS Setup**: (Handled by WAN Manager)

> **Note:** These configuration steps are performed by the WAN Manager component, not directly by the DHCPv6 lease handler.

#### `DhcpMgr_clearDHCPv6Lease(PCOSA_DML_DHCPCV6_FULL pDhcp6c)`

**Purpose**: Clears IPv6 lease information and resets configuration

**IPv6 Cleanup**:
1. **Address Removal**: Remove IPv6 addresses from interface
2. **Route Cleanup**: Remove IPv6 routes
3. **Prefix Cleanup**:(Handled by WAN Manager)
4. **Memory Cleanup**: Free lease structures
5. **Event Cleanup**: Clear IPv6 system events

## Lease Data Structures

### DHCPv4 Plugin Message

```c
typedef struct {
    int addressAssigned;           // Address assignment status
    char ipAddr[INET_ADDRSTRLEN];  // Assigned IP address
    char subnetMask[INET_ADDRSTRLEN]; // Subnet mask
    char gateway[INET_ADDRSTRLEN]; // Default gateway
    char dnsServer[INET_ADDRSTRLEN]; // Primary DNS server
    char dhcpServerId[INET_ADDRSTRLEN]; // DHCP server ID
    char dhcpState[32];           // DHCP client state
    uint32_t leaseTime;           // Lease duration
    // ... additional fields
} DHCPv4_PLUGIN_MSG;
```

### DHCPv6 Plugin Message

```c
typedef struct {
    int isExpired;                // Lease expiration status
    
    struct {
        int assigned;             // IANA assignment status
        char ipAddr[INET6_ADDRSTRLEN]; // IPv6 address
        uint32_t t1;             // T1 timer
        uint32_t t2;             // T2 timer
        uint32_t preferredTime;   // Preferred lifetime
        uint32_t validTime;      // Valid lifetime
    } ia_na;
    
    struct {
        int assigned;             // IAPD assignment status
        char prefix[INET6_ADDRSTRLEN]; // IPv6 prefix
        int prefixLen;           // Prefix length
        uint32_t t1;             // T1 timer
        uint32_t t2;             // T2 timer
        uint32_t preferredTime;   // Preferred lifetime
        uint32_t validTime;      // Valid lifetime
    } ia_pd;
    
    // ... additional fields
} DHCPv6_PLUGIN_MSG;
```

## System Integration

### TR-181 Data Model Updates

#### DHCPv4 Parameters
- `Device.DHCPv4.Client.{i}.IPAddress`
- `Device.DHCPv4.Client.{i}.SubnetMask`
- `Device.DHCPv4.Client.{i}.IPRouters`
- `Device.DHCPv4.Client.{i}.DNSServers`
- `Device.DHCPv4.Client.{i}.DHCPServer`
- `Device.DHCPv4.Client.{i}.LeaseTimeRemaining`

#### DHCPv6 Parameters
- `Device.DHCPv6.Client.{i}.PrefixDelegation`
- `Device.DHCPv6.Client.{i}.ReceivedOptions`
- `Device.DHCPv6.Client.{i}.SentOptions`
- `Device.DHCPv6.Client.{i}.Status`

### System Event Integration

#### Event Types
1. **Lease Acquisition**: New lease obtained
2. **Lease Renewal**: Existing lease renewed
3. **Lease Release**: Lease terminated
4. **Address Change**: IP address modified
5. **Configuration Change**: Network settings updated

#### Event Consumers
- **WAN Manager**: Network status updates

## Error Handling

### Validation Procedures

#### DHCPv4 Validation
- **IP Address Format**: Valid IPv4 address format
- **Subnet Mask**: Valid subnet mask values
- **Gateway Reachability**: Gateway within subnet
- **DNS Server Format**: Valid DNS server addresses
- **Lease Time**: Reasonable lease duration

#### DHCPv6 Validation
- **Address Format**: Valid IPv6 address format
- **Prefix Validation**: Valid IPv6 prefix format
- **Lifetime Validation**: Reasonable timer values
- **Scope Validation**: Appropriate address scope

### Error Recovery

#### Lease Processing Errors
1. **Invalid Data**: Log error and discard lease
2. **Configuration Failure**: Retry with previous configuration
3. **Memory Allocation**: Implement graceful degradation
4. **System Call Failure**: Log error and continue operation

#### Network Configuration Errors
1. **Interface Down**: Wait for interface to become available
2. **Permission Denied**: Log error and continue
3. **Address Conflict**: Implement conflict resolution
4. **Route Installation**: Retry with exponential backoff

## Performance Optimization

### Memory Management

#### Efficient Processing
- **Zero-copy Operations**: Minimize data copying
- **Memory Pooling**: Reuse lease structures
- **Lazy Evaluation**: Process only changed parameters
- **Garbage Collection**: Timely cleanup of expired leases

#### Memory Leak Prevention
- **Automatic Cleanup**: Free memory on all code paths
- **Reference Counting**: Track structure usage
- **Timeout Cleanup**: Remove stale references
- **Debug Tracking**: Monitor memory usage

### Processing Efficiency

#### Fast Path Processing
- **Change Detection**: Quick comparison algorithms
- **Batch Updates**: Group related updates
- **Parallel Processing**: Process multiple leases concurrently

#### Network Optimization
- **Minimal Syscalls**: Reduce system call overhead
- **Batch Network Operations**: Group network configuration
- **Interface Polling**: Efficient status monitoring
- **Event Aggregation**: Combine related events

## Testing and Debugging

### Test Coverage

#### Integration Tests
- End-to-end lease processing
- Network configuration verification
- TR-181 update validation
- Event generation testing

#### Performance Tests
- Lease processing throughput
- Memory usage analysis
- Network configuration timing
- Concurrent operation testing

### Debugging Features

#### Logging Integration
- **Detailed Tracing**: Step-by-step processing logs
- **Error Reporting**: Comprehensive error messages
- **Performance Metrics**: Processing time measurements
- **State Dumping**: Runtime state inspection

#### Common Debug Scenarios
1. **Lease Not Applied**: Check validation and configuration steps
2. **Memory Leaks**: Monitor allocation and cleanup
3. **Performance Issues**: Profile processing bottlenecks
4. **Configuration Errors**: Verify network setup commands
