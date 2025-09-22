# DHCP Lease Monitor System

## Overview

The DHCP Lease Monitor is a critical component responsible for receiving lease updates from external DHCP clients and routing them to the appropriate processing handlers. It implements an Inter-Process Communication (IPC) system using nanomsg library to ensure reliable and efficient message passing between DHCP client plugins and the DHCP Manager.

## Architecture

The lease monitor operates as a separate thread within the DHCP Manager process and implements a producer-consumer pattern:

```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   DHCP Clients      │────►│  Lease Monitor      │────►│  Lease Handlers     │
│                     │     │                     │     │                     │
│ • udhcpc (DHCPv4)   │     │ • IPC Server        │     │ • DHCPv4 Handler    │
│ • dibbler (DHCPv6)  │     │ • Message Router    │     │ • DHCPv6 Handler    │
│ • Plugin Execution  │     │ • Quick Processing  │     │ • TR-181 Updates    │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
```

## Key Components

### Files
- **Source**: `source/DHCPMgrUtils/dhcp_lease_monitor_thrd.c`
- **Header**: `source/DHCPMgrUtils/dhcp_lease_monitor_thrd.h`

### Core Functions

#### `DhcpMgr_LeaseMonitor_Start()`
- **Purpose**: Initializes and starts the lease monitor service
- **Returns**: 0 on success, -1 on failure
- **Threading**: Creates a new detached thread for monitoring

#### `DhcpMgr_LeaseMonitor_Init()`
- **Purpose**: Sets up the IPC socket and binds to the listening address
- **Protocol**: Uses nanomsg NN_PULL socket for receiving messages
- **Address**: `tcp://127.0.0.1:50324`

#### `DhcpMgr_LeaseMonitor_Thrd()`
- **Purpose**: Main monitoring loop that processes incoming lease messages
- **Behavior**: Runs continuously until system shutdown
- **Processing**: Validates and routes messages based on DHCP version

## Message Protocol

### Message Structure

```c
typedef struct {
    char ifname[BUFLEN_32];          // Interface name (e.g., "erouter0")
    DHCP_SOURCE version;             // DHCP_VERSION_4 or DHCP_VERSION_6
    union {
        DHCPv4_PLUGIN_MSG dhcpv4;    // DHCPv4 lease information
        DHCPv6_PLUGIN_MSG dhcpv6;    // DHCPv6 lease information
    } data;
} PLUGIN_MSG;
```

### DHCP Version Enumeration

```c
typedef enum {
    DHCP_VERSION_4,    // DHCPv4 lease message
    DHCP_VERSION_6,    // DHCPv6 lease message
} DHCP_SOURCE;
```

### DHCPv4 Message Format

The DHCPv4 plugin message contains:
- IP address assignment information
- DHCP server details
- Lease timing information
- Gateway and DNS server data
- Custom DHCP options

### DHCPv6 Message Format

The DHCPv6 plugin message contains:
- IANA (Identity Association for Non-temporary Addresses) information
- IAPD (Identity Association for Prefix Delegation) information
- Lease timing parameters
- DNS server configuration
- Custom DHCPv6 options

## IPC Implementation Details

### Transport Protocol
- **Library**: nanomsg (next-generation messaging library)
- **Pattern**: Pipeline (NN_PULL socket)
- **Address**: `tcp://127.0.0.1:50324`
- **Benefits**:
  - Automatic message queuing
  - Built-in reliability
  - Language-agnostic protocol
  - Scalable message passing

### Message Flow

1. **Client Plugin**: DHCP client executes plugin binary on lease events
2. **Plugin Execution**: Plugin creates nanomsg NN_PUSH socket
3. **Message Send**: Plugin sends PLUGIN_MSG structure to monitor
4. **Monitor Receive**: Lease monitor receives message via NN_PULL socket
5. **Validation**: Monitor validates message size and structure
6. **Routing**: Monitor routes message to appropriate handler based on version
7. **Processing**: Handler processes lease and updates system state

### Error Handling

The monitor implements robust error handling:

```c
bytes = nn_recv(ipcListenFd, (PLUGIN_MSG *)&plugin_msg, msg_size, 0);
if (bytes == msg_size) {
    // Valid message - process normally
} else if (bytes < 0) {
    DHCPMGR_LOG_ERROR("Failed to receive message from IPC");
} else {
    DHCPMGR_LOG_ERROR("Message size unexpected");
}
```

## Thread Safety and Concurrency

### Thread Model
- **Main Thread**: Handles TR-181 operations and client management
- **Monitor Thread**: Dedicated to IPC message processing
- **Handler Threads**: May process leases asynchronously

### Synchronization
- **Lock-free Design**: Monitor thread operates independently
- **Memory Management**: Each message allocates separate memory for handlers
- **Cleanup**: Automatic cleanup on thread termination

### Performance Characteristics
- **Low Latency**: Immediate message processing
- **High Throughput**: Efficient message queuing
- **Minimal Blocking**: Non-blocking message reception
- **Resource Efficient**: Single thread handles all interfaces

## Integration Points

### DHCP Client Plugins

#### udhcpc Plugin
- **Location**: `source/DHCPClientUtils/DHCPv4Client/dhcpmgr_udhcpc_plugin/`
- **Execution**: Called by udhcpc on lease events
- **Message**: Sends DHCPv4_PLUGIN_MSG via IPC

#### dibbler Plugin
- **Location**: `source/DHCPClientUtils/DHCPv6Client/dhcpmgr_dibbler_plugin/`
- **Execution**: Called by dibbler on lease events
- **Message**: Sends DHCPv6_PLUGIN_MSG via IPC

### Lease Handlers

#### DHCPv4 Handler
- **Function**: `DHCPMgr_AddDhcpv4Lease()`
- **Processing**: Updates TR-181 model, configures interface
- **Location**: Controller manages lease processing

#### DHCPv6 Handler
- **Function**: `DHCPMgr_AddDhcpv6Lease()`
- **Processing**: Handles IANA/IAPD, updates model
- **Location**: Controller manages lease processing

## Configuration

### Socket Configuration
```c
#define DHCP_MANAGER_ADDR "tcp://127.0.0.1:50324"
```

### Buffer Sizes
```c
#define BUFLEN_32  32   // Interface name buffer
```

### Message Size
```c
int msg_size = sizeof(PLUGIN_MSG);  // Fixed message size
```

## Monitoring and Debugging

### Log Messages
The monitor provides detailed logging:
- Initialization success/failure
- Message reception status
- Processing decisions
- Error conditions

### Debug Information
- Interface name for each lease
- Message size validation
- DHCP version identification
- Processing time tracking

### Common Issues

1. **Socket Binding Failure**
   - Check if port 50324 is available
   - Verify network interface status
   - Check firewall settings

2. **Message Size Mismatch**
   - Verify plugin and monitor use same structures
   - Check for structure padding issues
   - Validate nanomsg library versions

3. **Plugin Execution Failure**
   - Verify plugin binary permissions
   - Check plugin executable path
   - Validate DHCP client configuration

## Performance Optimization

### Memory Management
- **Pre-allocated Structures**: Use fixed-size message structures
- **Immediate Processing**: Process messages quickly to avoid queuing
- **Cleanup**: Proper memory cleanup in handlers

### Network Optimization
- **Local IPC**: Uses localhost to minimize network overhead
- **Binary Protocol**: Efficient binary message format
- **Minimal Serialization**: Direct structure copying

### Threading Optimization
- **Dedicated Thread**: Separate thread for IPC operations
- **Non-blocking Operations**: Avoid blocking main controller
- **Detached Thread**: Automatic resource cleanup

## Testing and Validation

### Unit Testing
- Message structure validation
- IPC socket functionality
- Error handling scenarios
- Thread lifecycle management

### Integration Testing
- End-to-end lease processing
- Multiple client scenarios
- Concurrent message handling
- Recovery after failures

### Performance Testing
- Message throughput measurement
- Latency analysis
- Memory usage profiling
- CPU utilization tracking

## Future Enhancements

### Potential Improvements
1. **Message Acknowledgment**: Add confirmation mechanism
2. **Message Priorities**: Implement priority-based processing
3. **Compression**: Add message compression for efficiency
4. **Security**: Implement message authentication
5. **Monitoring**: Add metrics collection and reporting

### Scalability Considerations
- Support for multiple DHCP manager instances
- Load balancing across processors
- Dynamic port allocation
- Enhanced error recovery mechanisms