# DHCP Manager Main Controller

## Overview

The Main Controller (`dhcpmgr_controller.c/h`) serves as the central coordination hub for the DHCP Client Manager. It orchestrates DHCP client lifecycle management, monitors interface status changes, coordinates lease processing, and manages the interaction between various components of the system.

## Architecture

The controller implements a state-driven architecture that continuously monitors system conditions and responds to changes:

```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   TR-181 Events     │────►│  Main Controller    │────►│  DHCP Clients       │
│                     │     │                     │     │                     │
│ • Config Changes    │     │ • State Machine     │     │ • udhcpc            │
│ • Enable/Disable    │     │ • Interface Monitor │     │ • dibbler           │
│ • Option Updates    │     │ • Lease Coordinator │     │ • Config Generation │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
                                      │
                                      ▼
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   Lease Handlers    │◄────│  Lease Processing   │────►│  System Events      │
│                     │     │                     │     │                     │
│ • DHCPv4 Handler    │     │ • Lease Validation  │     │ • Network Config    │
│ • DHCPv6 Handler    │     │ • Status Updates    │     │ • WAN Manager       │
│ • Data Model Update │     │ • Event Generation  │     │ • Interface Events  │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
```

## Key Components

### Files
- **Source**: `source/DHCPMgrUtils/dhcpmgr_controller.c`
- **Header**: `source/DHCPMgrUtils/dhcpmgr_controller.h`

### Core Functions

#### `DhcpMgr_StartMainController()`
- **Purpose**: Initializes and starts the main controller thread
- **Returns**: 0 on success, negative error code on failure
- **Threading**: Creates main controller thread with proper error handling

#### `DhcpMgr_MainController(void *args)`
- **Purpose**: Main control loop managing DHCP client lifecycle
- **Behavior**: Continuous monitoring and state management
- **Responsibilities**:
  - Interface status monitoring
  - DHCP client startup/shutdown
  - Lease processing coordination
  - Error recovery handling

## DHCP Client Management

### DHCPv4 Client Control

The controller manages udhcpc clients with comprehensive lifecycle management:

#### Configuration Building
```c
static int DhcpMgr_build_dhcpv4_opt_list(
    PCOSA_CONTEXT_DHCPC_LINK_OBJECT hInsContext,
    dhcp_opt_list **req_opt_list,
    dhcp_opt_list **send_opt_list
)
```

**Function Details**:
- Constructs DHCP option lists from TR-181 DML entries
- Processes requested options (options to request from server)
- Processes sent options (options to send to server)
- Handles custom vendor-specific options
- Validates option formats and values

**Option Processing**:
1. **Requested Options**: Options the client wants from server
   - Standard options (subnet mask, router, DNS, etc.)
   - Vendor-specific options
   - Custom application options

2. **Sent Options**: Options the client sends to server
   - Client identifier
   - Vendor class identifier
   - Custom client information

#### Interface Status Monitoring
```c
static bool DhcpMgr_checkInterfaceStatus(const char *ifName)
```

**Functionality**:
- Verifies interface operational status
- Checks link state (up/down)
- Validates interface configuration
- Returns interface readiness for DHCP operations

### DHCPv6 Client Control

The controller manages dibbler clients with specialized IPv6 handling:

#### Configuration Building
```c
static int DhcpMgr_build_dhcpv6_opt_list(
    PCOSA_CONTEXT_DHCPCV6_LINK_OBJECT hInsContext,
    dhcp_opt_list **req_opt_list,
    dhcp_opt_list **send_opt_list
)
```

**Function Details**:
- Constructs DHCPv6 option lists from TR-181 DML entries
- Handles IANA (non-temporary addresses) options
- Handles IAPD (prefix delegation) options
- Processes vendor-specific options
- Validates IPv6-specific option formats

#### Link-Local Address Verification
```c
static bool DhcpMgr_checkLinkLocalAddress(const char *interfaceName)
```

**IPv6 Requirements**:
- Verifies link-local address presence
- Required for DHCPv6 client operation
- Checks address scope and validity
- Ensures proper IPv6 stack initialization

## Lease Processing Coordination

### DHCPv4 Lease Handling

#### `DHCPMgr_AddDhcpv4Lease(char *ifName, DHCPv4_PLUGIN_MSG *newLease)`

**Process Flow**:
1. **Interface Lookup**: Locate DHCP client by interface name
2. **Validation**: Verify lease data integrity
3. **Queue Management**: Add lease to processing queue
4. **Memory Management**: Allocate lease structure
5. **Error Handling**: Cleanup on failure

**Lease Structure Management**:
```c
typedef struct {
    DHCPv4_PLUGIN_MSG *NewLeases;     // Pending lease queue
    DHCPv4_PLUGIN_MSG *currentLease;  // Active lease
    // ... other DHCP client data
} COSA_DML_DHCPC_FULL;
```

#### `DhcpMgr_ProcessV4Lease(PCOSA_DML_DHCPC_FULL pDhcpc)`

**Processing Steps**:
1. **Lease Retrieval**: Get next lease from queue
2. **Comparison**: Compare with current lease
3. **Validation**: Verify lease parameters
4. **TR-181 Update**: Update data model
5. **Network Configuration**: Configure interface
6. **Event Generation**: Notify other components
7. **Persistence**: Store lease for recovery

### DHCPv6 Lease Handling

#### `DHCPMgr_AddDhcpv6Lease(char *ifName, DHCPv6_PLUGIN_MSG *newLease)`

**IPv6-Specific Processing**:
1. **Dual Stack Management**: Handle IANA and IAPD separately
2. **Prefix Delegation**: Process IPv6 prefix assignments
3. **Address Assignment**: Handle individual IPv6 addresses
4. **Option Processing**: Handle IPv6-specific options

#### `DhcpMgr_ProcessV6Lease(PCOSA_DML_DHCPCV6_FULL pDhcp6c)`

**IPv6 Lease Processing**:
1. **IANA Processing**: Non-temporary address handling
2. **IAPD Processing**: Prefix delegation handling
3. **Lease Comparison**: Detect changes in assignments
4. **Route Configuration**: Set up IPv6 routing
5. **Event Notification**: Generate IPv6-specific events

## State Management

### Client State Machine

Each DHCP client maintains state through the controller:

```
┌─────────────┐    Enable    ┌─────────────┐    Interface    ┌─────────────┐
│  DISABLED   │─────────────►│   STARTING  │────────────────►│   RUNNING   │
└─────────────┘              └─────────────┘       Up        └─────────────┘
       ▲                            │                                │
       │                            │ Timeout                       │
       │         Disable            │                               │
       └────────────────────────────┼───────────────────────────────┘
                                    ▼                     Interface Down
                              ┌─────────────┐                       │
                              │   ERROR     │◄──────────────────────┘
                              └─────────────┘
```

### Interface Monitoring

The controller continuously monitors interface conditions:

#### Status Checks
- **Link State**: Physical link up/down status
- **Configuration**: IP configuration readiness
- **Driver Status**: Network driver operational state
- **Administrative State**: Interface enabled/disabled

#### Event Responses
- **Interface Up**: Start appropriate DHCP clients
- **Interface Down**: Stop clients and cleanup resources
- **Configuration Change**: Restart clients with new configuration
- **Error Conditions**: Implement recovery procedures

## Integration Points

### TR-181 Data Model Integration

#### Configuration Source
- `Device.DHCPv4.Client.{i}.*` - DHCPv4 client parameters
- `Device.DHCPv6.Client.{i}.*` - DHCPv6 client parameters
- Dynamic updates trigger client reconfiguration

#### Status Updates
- Lease information published to TR-181
- Client operational status
- Error conditions and diagnostics
- Performance metrics

### System Event Integration

#### Event Generation
The controller generates system events for:
- Lease acquisition/renewal/release
- IP address changes
- Client state transitions
- Error conditions

#### Event Consumption
The controller responds to:
- Interface state changes
- Network configuration updates
- Administrative commands
- Recovery triggers

## Error Handling and Recovery

### Client Process Management

#### Process Monitoring
```c
void processKilled(pid_t pid)
```

**Functionality**:
- Monitors DHCP client process health
- Detects abnormal process termination
- Initiates recovery procedures
- Updates client state appropriately

#### Recovery Procedures
1. **Process Restart**: Automatic client restart on failure
2. **State Recovery**: Restore previous lease information
3. **Configuration Rebuild**: Regenerate client configuration
4. **Interface Reset**: Reset interface if necessary

### Lease Validation

#### Data Integrity Checks
- Verify lease parameter validity
- Validate IP address formats
- Check lease timing parameters
- Ensure option consistency

#### Conflict Resolution
- Handle duplicate lease information
- Resolve timing conflicts
- Manage option precedence
- Coordinate multiple interfaces

## Performance Optimization

### Memory Management

#### Efficient Allocation
- Reuse lease structures when possible
- Implement proper cleanup procedures
- Minimize memory fragmentation
- Use memory pools for frequent allocations

#### Leak Prevention
- Automatic cleanup on errors
- Reference counting for shared structures
- Timeout-based resource cleanup
- Regular memory auditing

### Threading Efficiency

#### Non-blocking Operations
- Asynchronous lease processing
- Non-blocking interface monitoring
- Efficient event handling
- Minimal lock contention

#### Resource Sharing
- Thread-safe data structures
- Proper synchronization primitives
- Efficient communication mechanisms
- Load balancing across cores

## Configuration Management

### Option List Building

The controller builds comprehensive option lists for both DHCPv4 and DHCPv6:

#### Standard Options
- Network configuration (subnet, gateway, DNS)
- Timing parameters (lease time, renewal times)
- Server identification
- Client identification

#### Vendor-Specific Options
- Custom vendor extensions
- Proprietary configuration parameters
- Device-specific settings
- Application-specific data

#### Custom Options
- User-defined option codes
- Binary data encoding
- String parameter handling
- Structured data formats

### Dynamic Reconfiguration

#### Configuration Updates
- Real-time TR-181 changes
- Option list rebuilding
- Client restart coordination
- State preservation during updates

#### Validation
- Parameter range checking
- Format validation
- Dependency verification
- Consistency enforcement

## Testing and Debugging

### Debug Features

#### Logging Integration
- Comprehensive debug logging
- Configurable log levels
- Component-specific logging
- Performance metrics logging

#### State Inspection
- Runtime state examination
- Lease information display
- Configuration validation
- Process status monitoring

### Common Issues

#### Client Startup Failures
- Configuration validation errors
- Interface readiness issues
- Permission problems
- Resource constraints

#### Lease Processing Problems
- Message format errors
- Timing issues
- State synchronization problems
- Memory allocation failures

## Future Enhancements

### Planned Improvements
1. **Enhanced Monitoring**: Real-time performance metrics
2. **Load Balancing**: Multiple server support
3. **Advanced Recovery**: Intelligent failure detection
4. **Configuration Templates**: Predefined configurations
5. **API Extensions**: Enhanced management interfaces

### Scalability Considerations
- Multi-interface optimization
- Large-scale deployment support
- Resource usage optimization
- High-availability features