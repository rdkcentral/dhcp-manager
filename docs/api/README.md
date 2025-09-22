# DHCP Client Manager API Reference

## Overview

This document provides a comprehensive API reference for all public functions in the DHCP Client Manager implementation. The APIs are organized by component and provide detailed function signatures, parameters, return values, and usage examples.

## Table of Contents

1. [Main Controller APIs](#main-controller-apis)
2. [Lease Monitor APIs](#lease-monitor-apis)
3. [Lease Handler APIs](#lease-handler-apis)
4. [Recovery Handler APIs](#recovery-handler-apis)
5. [MAP-T APIs](#map-t-apis)
6. [Utility Functions](#utility-functions)

---

## Main Controller APIs

### `DhcpMgr_StartMainController()`

```c
int DhcpMgr_StartMainController(void);
```

**Purpose**: Initializes and starts the main controller thread for the DHCP Manager.

**Parameters**: None

**Return Values**:
- `0`: Success - Controller thread started successfully
- `negative`: Error code indicating failure reason

**Usage**:
```c
int result = DhcpMgr_StartMainController();
if (result != 0) {
    DHCPMGR_LOG_ERROR("Failed to start main controller: %d", result);
    return -1;
}
```

**Thread Safety**: Thread-safe, should be called once during initialization

**Related**: Called from main DHCP Manager initialization

---

### `DHCPMgr_AddDhcpv4Lease()`

```c
void DHCPMgr_AddDhcpv4Lease(char *ifName, DHCPv4_PLUGIN_MSG *newLease);
```

**Purpose**: Adds a new DHCPv4 lease to the processing queue for the specified interface.

**Parameters**:
- `ifName`: Interface name (e.g., "erouter0", "wan0")
- `newLease`: Pointer to DHCPv4 lease information structure

**Return Values**: None (void function)

**Usage**:
```c
DHCPv4_PLUGIN_MSG *lease = malloc(sizeof(DHCPv4_PLUGIN_MSG));
// Populate lease structure...
DHCPMgr_AddDhcpv4Lease("erouter0", lease);
```

**Thread Safety**: Thread-safe with internal synchronization

**Memory Management**: Takes ownership of `newLease` pointer

**Related**: Called from lease monitor thread

---

### `DHCPMgr_AddDhcpv6Lease()`

```c
void DHCPMgr_AddDhcpv6Lease(char *ifName, DHCPv6_PLUGIN_MSG *newLease);
```

**Purpose**: Adds a new DHCPv6 lease to the processing queue for the specified interface.

**Parameters**:
- `ifName`: Interface name (e.g., "erouter0", "wan0")
- `newLease`: Pointer to DHCPv6 lease information structure

**Return Values**: None (void function)

**Usage**:
```c
DHCPv6_PLUGIN_MSG *lease = malloc(sizeof(DHCPv6_PLUGIN_MSG));
// Populate lease structure...
DHCPMgr_AddDhcpv6Lease("erouter0", lease);
```

**Thread Safety**: Thread-safe with internal synchronization

**Memory Management**: Takes ownership of `newLease` pointer

**Related**: Called from lease monitor thread

---

### `DhcpMgr_ProcessV4Lease()`

```c
void DhcpMgr_ProcessV4Lease(PCOSA_DML_DHCPC_FULL pDhcpc);
```

**Purpose**: Processes new DHCPv4 leases from the queue and updates system state.

**Parameters**:
- `pDhcpc`: Pointer to DHCP client structure containing lease information

**Return Values**: None (void function)

**Usage**:
```c
PCOSA_DML_DHCPC_FULL pDhcpc = GetDhcpClientByInstance(instanceNum);
DhcpMgr_ProcessV4Lease(pDhcpc);
```

**Thread Safety**: Not thread-safe, called from main controller thread

**Side Effects**: Updates TR-181 model, configures network interface, generates events

**Related**: Called from main controller loop

---

### `DhcpMgr_ProcessV6Lease()`

```c
void DhcpMgr_ProcessV6Lease(PCOSA_DML_DHCPCV6_FULL pDhcp6c);
```

**Purpose**: Processes new DHCPv6 leases from the queue and updates system state.

**Parameters**:
- `pDhcp6c`: Pointer to DHCPv6 client structure containing lease information

**Return Values**: None (void function)

**Usage**:
```c
PCOSA_DML_DHCPCV6_FULL pDhcp6c = GetDhcpv6ClientByInstance(instanceNum);
DhcpMgr_ProcessV6Lease(pDhcp6c);
```

**Thread Safety**: Not thread-safe, called from main controller thread

**Side Effects**: Updates TR-181 model, configures IPv6 interface, generates events

**Related**: Called from main controller loop

---

### `DhcpMgr_clearDHCPv4Lease()`

```c
void DhcpMgr_clearDHCPv4Lease(PCOSA_DML_DHCPC_FULL pDhcpc);
```

**Purpose**: Clears current DHCPv4 lease information and resets interface configuration.

**Parameters**:
- `pDhcpc`: Pointer to DHCP client structure to clear

**Return Values**: None (void function)

**Usage**:
```c
DhcpMgr_clearDHCPv4Lease(pDhcpc);
```

**Thread Safety**: Not thread-safe, called from main controller thread

**Side Effects**: Frees memory, resets TR-181 parameters, clears interface configuration

---

### `DhcpMgr_clearDHCPv6Lease()`

```c
void DhcpMgr_clearDHCPv6Lease(PCOSA_DML_DHCPCV6_FULL pDhcp6c);
```

**Purpose**: Clears current DHCPv6 lease information and resets interface configuration.

**Parameters**:
- `pDhcp6c`: Pointer to DHCPv6 client structure to clear

**Return Values**: None (void function)

**Usage**:
```c
DhcpMgr_clearDHCPv6Lease(pDhcp6c);
```

**Thread Safety**: Not thread-safe, called from main controller thread

**Side Effects**: Frees memory, resets TR-181 parameters, clears IPv6 configuration

---

## Lease Monitor APIs

### `DhcpMgr_LeaseMonitor_Start()`

```c
int DhcpMgr_LeaseMonitor_Start(void);
```

**Purpose**: Starts the DHCP Lease Monitor service to listen for lease updates from DHCP client plugins.

**Parameters**: None

**Return Values**:
- `0`: Success - Lease monitor started successfully
- `-1`: Failure - Could not initialize or start monitor

**Usage**:
```c
int result = DhcpMgr_LeaseMonitor_Start();
if (result != 0) {
    DHCPMGR_LOG_ERROR("Failed to start lease monitor");
    return -1;
}
```

**Thread Safety**: Thread-safe, creates detached monitoring thread

**Network**: Binds to `tcp://127.0.0.1:50324`

**Related**: Called during DHCP Manager initialization

---

## Lease Handler APIs

### `DhcpMgr_updateDHCPv4DML()`

```c
ANSC_STATUS DhcpMgr_updateDHCPv4DML(PCOSA_DML_DHCPC_FULL pDhcpc);
```

**Purpose**: Updates the TR-181 Data Model with DHCPv4 lease information.

**Parameters**:
- `pDhcpc`: Pointer to DHCP client structure with current lease information

**Return Values**:
- `ANSC_STATUS_SUCCESS`: TR-181 model updated successfully
- `ANSC_STATUS_FAILURE`: Update failed

**Usage**:
```c
ANSC_STATUS status = DhcpMgr_updateDHCPv4DML(pDhcpc);
if (status != ANSC_STATUS_SUCCESS) {
    DHCPMGR_LOG_ERROR("Failed to update TR-181 model");
}
```

**Thread Safety**: Not thread-safe, called from main controller thread

**Side Effects**: Updates TR-181 parameters, may trigger parameter change events

---

### `sysinfo_getUpTime()`

```c
uint32_t sysinfo_getUpTime(void);
```

**Purpose**: Retrieves the system uptime in seconds.

**Parameters**: None

**Return Values**:
- `uint32_t`: System uptime in seconds

**Usage**:
```c
uint32_t uptime = sysinfo_getUpTime();
DHCPMGR_LOG_INFO("System uptime: %u seconds", uptime);
```

**Thread Safety**: Thread-safe

**System Call**: Uses `sysinfo()` system call

---

### `set_inf_sysevents()`

```c
void set_inf_sysevents(const DHCPv4_PLUGIN_MSG *const current, 
                       const DHCPv4_PLUGIN_MSG *const newLease, 
                       const char *const interface);
```

**Purpose**: Updates system events based on changes in DHCPv4 lease parameters.

**Parameters**:
- `current`: Current lease information (may be NULL)
- `newLease`: New lease information
- `interface`: Interface name

**Return Values**: None (void function)

**Usage**:
```c
set_inf_sysevents(pDhcpc->currentLease, newLease, "erouter0");
```

**Thread Safety**: Thread-safe

**Side Effects**: Generates system events for network configuration changes

---

### `DhcpMgr_GetBCastFromIpSubnetMask()`

```c
int DhcpMgr_GetBCastFromIpSubnetMask(const char* inIpStr, 
                                     const char* inSubnetMaskStr, 
                                     char *outBcastStr);
```

**Purpose**: Calculates broadcast address from IP address and subnet mask.

**Parameters**:
- `inIpStr`: IP address string (e.g., "192.168.1.100")
- `inSubnetMaskStr`: Subnet mask string (e.g., "255.255.255.0")
- `outBcastStr`: Output buffer for broadcast address

**Return Values**:
- `RETURN_OK`: Calculation successful
- `RETURN_ERR`: Invalid input parameters

**Usage**:
```c
char broadcast[INET_ADDRSTRLEN];
int result = DhcpMgr_GetBCastFromIpSubnetMask("192.168.1.100", 
                                              "255.255.255.0", 
                                              broadcast);
if (result == RETURN_OK) {
    printf("Broadcast: %s\n", broadcast);
}
```

**Thread Safety**: Thread-safe

**Buffer Requirements**: `outBcastStr` must be at least `INET_ADDRSTRLEN` bytes

---

## Recovery Handler APIs

### `DhcpMgr_Dhcp_Recovery_Start()`

```c
int DhcpMgr_Dhcp_Recovery_Start(void);
```

**Purpose**: Starts the DHCP recovery process, loading existing lease data and monitoring client processes.

**Parameters**: None

**Return Values**:
- `EXIT_SUCCESS` (0): Recovery process started successfully
- `EXIT_FAIL` (-1): Recovery initialization failed

**Usage**:
```c
int result = DhcpMgr_Dhcp_Recovery_Start();
if (result != EXIT_SUCCESS) {
    DHCPMGR_LOG_ERROR("Failed to start recovery process");
    return -1;
}
```

**Thread Safety**: Thread-safe, creates process monitoring thread

**File System**: Accesses `/tmp/Dhcp_manager/` directory

**Related**: Called during DHCP Manager initialization

---

### `DHCPMgr_storeDhcpv4Lease()`

```c
int DHCPMgr_storeDhcpv4Lease(PCOSA_DML_DHCPC_FULL data);
```

**Purpose**: Stores DHCPv4 lease information to persistent storage for recovery.

**Parameters**:
- `data`: Pointer to DHCP client structure with lease information

**Return Values**:
- `EXIT_SUCCESS` (0): Lease data stored successfully
- `EXIT_FAIL` (-1): Storage operation failed

**Usage**:
```c
int result = DHCPMgr_storeDhcpv4Lease(pDhcpc);
if (result != EXIT_SUCCESS) {
    DHCPMGR_LOG_ERROR("Failed to store DHCPv4 lease");
}
```

**Thread Safety**: Thread-safe

**File System**: Creates file in `/tmp/Dhcp_manager/`

**File Format**: Binary format with client structure followed by lease data

---

### `DHCPMgr_storeDhcpv6Lease()`

```c
int DHCPMgr_storeDhcpv6Lease(PCOSA_DML_DHCPCV6_FULL data);
```

**Purpose**: Stores DHCPv6 lease information to persistent storage for recovery.

**Parameters**:
- `data`: Pointer to DHCPv6 client structure with lease information

**Return Values**:
- `EXIT_SUCCESS` (0): Lease data stored successfully
- `EXIT_FAIL` (-1): Storage operation failed

**Usage**:
```c
int result = DHCPMgr_storeDhcpv6Lease(pDhcp6c);
if (result != EXIT_SUCCESS) {
    DHCPMGR_LOG_ERROR("Failed to store DHCPv6 lease");
}
```

**Thread Safety**: Thread-safe

**File System**: Creates file in `/tmp/Dhcp_manager/`

**File Format**: Binary format with client structure followed by lease data

---

### `remove_dhcp_lease_file()`

```c
void remove_dhcp_lease_file(int instanceNumber, int dhcpVersion);
```

**Purpose**: Removes stored lease file when DHCP client is deleted.

**Parameters**:
- `instanceNumber`: TR-181 client instance number
- `dhcpVersion`: `DHCP_v4` (0) for DHCPv4, `DHCP_v6` (1) for DHCPv6

**Return Values**: None (void function)

**Usage**:
```c
// Remove DHCPv4 client instance 1 lease file
remove_dhcp_lease_file(1, DHCP_v4);

// Remove DHCPv6 client instance 2 lease file
remove_dhcp_lease_file(2, DHCP_v6);
```

**Thread Safety**: Thread-safe

**File System**: Deletes file from `/tmp/Dhcp_manager/`

---

## MAP-T APIs

### `DhcpMgr_MaptParseOpt95Response()`

```c
ANSC_STATUS DhcpMgr_MaptParseOpt95Response(/* parameters omitted for brevity */);
```

**Purpose**: Parses DHCP option 95 (S46_CONT_MAPT) and extracts MAP-T configuration parameters.

**Parameters**: (Specific parameters depend on implementation context)

**Return Values**:
- `ANSC_STATUS_SUCCESS`: MAP-T option parsed successfully
- `ANSC_STATUS_FAILURE`: Parsing failed or invalid data

**Usage**:
```c
ANSC_STATUS status = DhcpMgr_MaptParseOpt95Response(/* params */);
if (status == ANSC_STATUS_SUCCESS) {
    DHCPMGR_LOG_INFO("MAP-T configuration applied");
} else {
    DHCPMGR_LOG_ERROR("Failed to parse MAP-T option");
}
```

**Thread Safety**: Thread-safe

**Standards**: Implements RFC 7599 MAP-T option processing

**Related**: Called from DHCPv6 lease processing when option 95 is present

---

## Utility Functions

### Process Management

#### `processKilled()`

```c
void processKilled(pid_t pid);
```

**Purpose**: Handles cleanup when a DHCP client process terminates unexpectedly.

**Parameters**:
- `pid`: Process ID of the terminated DHCP client

**Return Values**: None (void function)

**Usage**:
```c
// Called internally when process monitoring detects termination
processKilled(client_pid);
```

**Thread Safety**: Thread-safe

**Side Effects**: Updates client state, may trigger restart procedures

---

### Configuration Building

#### Internal Functions

The controller also provides internal configuration building functions:

```c
static int DhcpMgr_build_dhcpv4_opt_list(
    PCOSA_CONTEXT_DHCPC_LINK_OBJECT hInsContext,
    dhcp_opt_list **req_opt_list,
    dhcp_opt_list **send_opt_list
);

static int DhcpMgr_build_dhcpv6_opt_list(
    PCOSA_CONTEXT_DHCPCV6_LINK_OBJECT hInsContext,
    dhcp_opt_list **req_opt_list,
    dhcp_opt_list **send_opt_list
);
```

These functions are internal to the controller and build DHCP option lists from TR-181 configuration data.

---

## Data Structures

### DHCPv4 Plugin Message

```c
typedef struct {
    int addressAssigned;                  // 1 if address assigned, 0 otherwise
    char ipAddr[INET_ADDRSTRLEN];        // Assigned IP address
    char subnetMask[INET_ADDRSTRLEN];    // Subnet mask
    char gateway[INET_ADDRSTRLEN];       // Default gateway
    char dnsServer[INET_ADDRSTRLEN];     // Primary DNS server
    char dhcpServerId[INET_ADDRSTRLEN];  // DHCP server identifier
    char dhcpState[32];                  // DHCP client state
    uint32_t leaseTime;                  // Lease duration in seconds
    struct DHCPv4_PLUGIN_MSG *next;      // Next message in queue
    // Additional fields...
} DHCPv4_PLUGIN_MSG;
```

### DHCPv6 Plugin Message

```c
typedef struct {
    int isExpired;                       // 1 if lease expired, 0 otherwise
    
    struct {
        int assigned;                    // 1 if IANA assigned
        char ipAddr[INET6_ADDRSTRLEN];   // IPv6 address
        uint32_t t1;                     // T1 timer
        uint32_t t2;                     // T2 timer
        uint32_t preferredTime;          // Preferred lifetime
        uint32_t validTime;              // Valid lifetime
    } ia_na;
    
    struct {
        int assigned;                    // 1 if IAPD assigned
        char prefix[INET6_ADDRSTRLEN];   // IPv6 prefix
        int prefixLen;                   // Prefix length
        uint32_t t1;                     // T1 timer
        uint32_t t2;                     // T2 timer
        uint32_t preferredTime;          // Preferred lifetime
        uint32_t validTime;              // Valid lifetime
    } ia_pd;
    
    char aftr[INET6_ADDRSTRLEN];        // AFTR address for DS-Lite
    struct DHCPv6_PLUGIN_MSG *next;     // Next message in queue
    // Additional fields...
} DHCPv6_PLUGIN_MSG;
```

### Plugin Message Container

```c
typedef struct {
    char ifname[BUFLEN_32];             // Interface name
    DHCP_SOURCE version;                // DHCP_VERSION_4 or DHCP_VERSION_6
    union {
        DHCPv4_PLUGIN_MSG dhcpv4;       // DHCPv4 message data
        DHCPv6_PLUGIN_MSG dhcpv6;       // DHCPv6 message data
    } data;
} PLUGIN_MSG;
```

---

## Error Codes and Constants

### Return Values
- `EXIT_SUCCESS` / `0`: Operation completed successfully
- `EXIT_FAIL` / `-1`: Operation failed
- `ANSC_STATUS_SUCCESS`: ANSC operation successful
- `ANSC_STATUS_FAILURE`: ANSC operation failed
- `RETURN_OK`: Utility function success
- `RETURN_ERR`: Utility function error

### Constants
- `DHCP_MANAGER_ADDR`: `"tcp://127.0.0.1:50324"` - IPC listening address
- `BUFLEN_32`: 32 - Standard buffer size for short strings
- `INET_ADDRSTRLEN`: 16 - IPv4 address string length
- `INET6_ADDRSTRLEN`: 46 - IPv6 address string length
- `TMP_DIR_PATH`: `"/tmp/Dhcp_manager"` - Recovery file directory

### DHCP Versions
- `DHCP_v4`: 0 - DHCPv4 identifier
- `DHCP_v6`: 1 - DHCPv6 identifier
- `DHCP_VERSION_4`: DHCPv4 enum value
- `DHCP_VERSION_6`: DHCPv6 enum value

---

## Usage Examples

### Basic Initialization

```c
// Initialize DHCP Manager components
int main() {
    // Start recovery system
    if (DhcpMgr_Dhcp_Recovery_Start() != EXIT_SUCCESS) {
        return -1;
    }
    
    // Start lease monitor
    if (DhcpMgr_LeaseMonitor_Start() != 0) {
        return -1;
    }
    
    // Start main controller
    if (DhcpMgr_StartMainController() != 0) {
        return -1;
    }
    
    return 0;
}
```

### Processing DHCPv4 Lease

```c
void handle_dhcpv4_lease(PCOSA_DML_DHCPC_FULL pDhcpc) {
    // Process any pending leases
    DhcpMgr_ProcessV4Lease(pDhcpc);
    
    // Store lease for recovery
    if (pDhcpc->currentLease) {
        DHCPMgr_storeDhcpv4Lease(pDhcpc);
    }
    
    // Update TR-181 model
    DhcpMgr_updateDHCPv4DML(pDhcpc);
}
```

### Cleanup on Shutdown

```c
void cleanup_dhcp_client(PCOSA_DML_DHCPC_FULL pDhcpc, int instanceNum) {
    // Clear current lease
    DhcpMgr_clearDHCPv4Lease(pDhcpc);
    
    // Remove stored lease file
    remove_dhcp_lease_file(instanceNum, DHCP_v4);
}
```

---

## Thread Safety Summary

| Function | Thread Safety | Notes |
|----------|---------------|-------|
| `DhcpMgr_StartMainController()` | ✅ Safe | One-time initialization |
| `DhcpMgr_LeaseMonitor_Start()` | ✅ Safe | One-time initialization |
| `DHCPMgr_AddDhcpv4Lease()` | ✅ Safe | Internal synchronization |
| `DHCPMgr_AddDhcpv6Lease()` | ✅ Safe | Internal synchronization |
| `DhcpMgr_ProcessV4Lease()` | ⚠️ Not Safe | Controller thread only |
| `DhcpMgr_ProcessV6Lease()` | ⚠️ Not Safe | Controller thread only |
| `DHCPMgr_storeDhcpv4Lease()` | ✅ Safe | File system operations |
| `DHCPMgr_storeDhcpv6Lease()` | ✅ Safe | File system operations |
| `remove_dhcp_lease_file()` | ✅ Safe | File system operations |
| Utility functions | ✅ Safe | No shared state |

---

## Performance Considerations

### Memory Usage
- **Lease Structures**: Typically 1-2KB per active lease
- **Option Lists**: Variable size based on configuration
- **Recovery Files**: Match in-memory structure sizes

### Processing Time
- **Lease Processing**: ~1-5ms per lease (depending on configuration)
- **File I/O**: ~1-10ms per recovery operation
- **IPC Operations**: ~0.1-1ms per message

### Scalability
- **Maximum Interfaces**: Limited by system resources
- **Concurrent Leases**: Efficient queue-based processing
- **Recovery Time**: Proportional to number of stored leases

This API reference provides the foundation for integrating with and extending the DHCP Client Manager. For implementation details and advanced usage scenarios, refer to the component-specific documentation.