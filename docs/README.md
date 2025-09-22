# DHCP Client Manager Documentation

## Overview

The DHCP Client Manager is a core component of the RDK-B (Reference Design Kit for Broadband) framework that provides centralized management of DHCP client operations for both IPv4 and IPv6 protocols. This implementation ensures efficient IP address management, seamless IPv4/IPv6 coexistence, and dynamic lease monitoring in RDK-B networking environments.

## Key Features

- **Dual Protocol Support**: Comprehensive support for both DHCPv4 (using udhcpc) and DHCPv6 (using dibbler) clients
- **Centralized Management**: Single point of control for all DHCP client operations across multiple interfaces
- **Real-time Lease Monitoring**: IPC-based system for receiving and processing lease updates from DHCP clients
- **TR-181 Integration**: Full integration with TR-181 Data Model for standardized management
- **Recovery and Persistence**: Automatic recovery of DHCP client states and lease information after system restarts
- **MAP-T Support**: Implementation of RFC 7599 for IPv4-over-IPv6 transition technology
- **Custom Options**: Support for vendor-specific and custom DHCP options
- **Event-driven Architecture**: Reactive system that responds to network state changes and lease events

## Architecture Overview

The DHCP Client Manager follows a modular, event-driven architecture with the following main components:

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Rbus Thread   │    │ Main Controller │    │ Lease Monitor   │
│                 │    │                 │    │                 │
│ • TR-181 DML    │◄──►│ • Client Control│◄──►│ • IPC Listener  │
│ • Set/Get Ops   │    │ • Status Monitor│    │ • Lease Updates │
│ • Subscriptions │    │ • Interface Mgmt│    │ • Quick Process │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ Recovery Handler│    │ Lease Handlers  │    │ Client Plugins  │
│                 │    │                 │    │                 │
│ • Persistence   │    │ • DHCPv4 Process│    │ • udhcpc Plugin │
│ • State Restore │    │ • DHCPv6 Process│    │ • dibbler Plugin│
│ • Crash Recovery│    │ • Event Updates │    │ • Lease Reports │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## Directory Structure

```
docs/
├── README.md                    # This overview document
├── configuration-guide.md       # Comprehensive PSM configuration guide
├── architecture/
│   ├── component-diagram.md     # Detailed component relationships
│   ├── data-flow.md            # Data flow and message passing
│   └── threading-model.md      # Threading architecture
├── components/
│   ├── lease-monitor.md        # Lease monitoring system
│   ├── main-controller.md      # Main controller logic
│   ├── lease-handlers.md       # DHCPv4/v6 lease processing
│   ├── recovery-handler.md     # Persistence and recovery
│   └── mapt-apis.md           # MAP-T implementation
└── api/
    ├── controller-api.md       # Main controller APIs
    ├── monitor-api.md         # Lease monitor APIs
    ├── handlers-api.md        # Lease handler APIs
    └── recovery-api.md        # Recovery handler APIs
```

## Quick Start

### Building the Component

The DHCP Client Manager is built as part of the RDK-B build system:

```bash
# Build the entire DHCP Manager
bitbake ccsp-dhcp-mgr

# Or build just the client utilities
cd source/DHCPMgrUtils
make
```

### Configuration

The component is configured through TR-181 Data Model entries:

- `Device.DHCPv4.Client.{i}.*` - DHCPv4 client configuration
- `Device.DHCPv6.Client.{i}.*` - DHCPv6 client configuration

### Key Interfaces

- **IPC Address**: `tcp://127.0.0.1:50324` (for lease monitoring)
- **Configuration Files**: Dynamic creation based on TR-181 settings
- **Log Output**: Standard RDK logging framework

## Configuration

The DHCP Client Manager is configured through PSM (Persistent Storage Manager) parameters and TR-181 Data Model entries. Configuration covers both DHCPv4 and DHCPv6 clients with support for custom options, vendor-specific information, and WAN Manager integration.

### Configuration Structure

```
dmsb.dhcpmanager
├── ClientNoOfEntries                    # Number of DHCPv4 clients
├── Client.{i}                          # DHCPv4 client instances
│   ├── Alias                           # Client alias/name
│   ├── ReqOptionNoOfEntries            # Number of requested options
│   ├── ReqOption.{i}.Tag               # DHCP option tag
│   ├── ReqOption.{i}.Order             # Option request order
│   ├── SendOptionNoOfEntries           # Number of sent options
│   ├── SendOption.{i}.Tag              # DHCP option tag
│   └── SendOption.{i}.Value            # Option value (hex-encoded)
└── dhcpv6
    ├── ClientNoOfEntries               # Number of DHCPv6 clients
    └── Client.{i}                      # DHCPv6 client instances
        ├── Alias                       # Client alias/name
        ├── ReqAddr                     # Request IPv6 address (IANA)
        ├── ReqPrefix                   # Request prefix delegation (IAPD)
        ├── RequestedOptions            # Comma-separated option list
        ├── SentOptNoOfEntries          # Number of sent options
        ├── SentOption.{i}.Tag          # DHCPv6 option tag
        └── SentOption.{i}.Value        # Option value (hex-encoded)
```

### Quick Configuration Examples

#### Basic DHCPv4 Client
```xml
<!-- Single DHCPv4 client for WAN interface -->
<Record name="dmsb.dhcpmanager.ClientNoOfEntries" type="astr">1</Record>
<Record name="dmsb.dhcpmanager.Client.1.Alias" type="astr">WAN_CLIENT</Record>

<!-- Request standard options -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOptionNoOfEntries" type="astr">3</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.1.Tag" type="astr">1</Record>  <!-- Subnet Mask -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.1.Order" type="astr">1</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.2.Tag" type="astr">3</Record>  <!-- Router -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.2.Order" type="astr">2</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.3.Tag" type="astr">6</Record>  <!-- DNS Server -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.3.Order" type="astr">3</Record>

<!-- Send vendor class identifier -->
<Record name="dmsb.dhcpmanager.Client.1.SendOptionNoOfEntries" type="astr">1</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.1.Tag" type="astr">60</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.1.Value" type="astr">526F757465725F763100</Record>
```

#### Basic DHCPv6 Client
```xml
<!-- Single DHCPv6 client with address request -->
<Record name="dmsb.dhcpmanager.dhcpv6.ClientNoOfEntries" type="astr">1</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.Alias" type="astr">WAN_IPV6</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.ReqAddr" type="astr">TRUE</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.ReqPrefix" type="astr">FALSE</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.RequestedOptions" type="astr">23,24</Record> <!-- DNS, Domain -->
```

#### WAN Manager Integration
```xml
<!-- Bind DHCP clients to WAN interfaces -->
<Record name="dmsb.wanmanager.if.1.VirtualInterface.1.IP.DHCPV4Interface" type="astr">Device.DHCPv4.Client.1</Record>
<Record name="dmsb.wanmanager.if.1.VirtualInterface.1.IP.DHCPV6Interface" type="astr">Device.DHCPv6.Client.1</Record>
```

### Custom Options Support

The DHCP Manager supports custom vendor-specific options through:

- **PSM Configuration**: Direct hex-encoded values in PSM parameters
- **Custom APIs**: Dynamic value generation through custom libraries
- **Weak/Strong Implementation**: Override default behavior with custom functions

### Advanced Configuration

For complex scenarios including:
- Multiple WAN interfaces (Cable, Ethernet, LTE)
- Vendor-specific option processing
- MAP-T configuration
- Enterprise deployments

Refer to the comprehensive [Configuration Guide](configuration-guide.md) for detailed examples and best practices.

### Configuration Validation

```bash
# Verify DHCP client configuration
dmcli eRT getv Device.DHCPv4.Client.
dmcli eRT getv Device.DHCPv6.Client.

# Check WAN Manager bindings
dmcli eRT getv Device.X_RDK_WanManager.Interface.

# Validate option configuration
dmcli eRT getv Device.DHCPv4.Client.1.ReqOption.
dmcli eRT getv Device.DHCPv6.Client.1.SentOption.
```

## Core Components

### 1. Main Controller (`dhcpmgr_controller.c/h`)
- Manages DHCP client lifecycle
- Monitors interface status changes
- Coordinates lease processing
- Handles client startup/shutdown

### 2. Lease Monitor (`dhcp_lease_monitor_thrd.c/h`)
- IPC server for receiving lease updates
- Processes messages from DHCP client plugins
- Routes lease information to appropriate handlers

### 3. Lease Handlers (`dhcpmgr_v*_lease_handler.c`)
- **DHCPv4 Handler**: Processes IPv4 lease information
- **DHCPv6 Handler**: Processes IPv6 lease information (IANA/IAPD)
- Updates TR-181 data model
- Configures network interfaces

### 4. Recovery Handler (`dhcpmgr_recovery_handler.c/h`)
- Persists lease information to filesystem
- Restores state after system restart
- Monitors client process health

### 5. MAP-T Support (`dhcpmgr_map_apis.c/h`)
- Implements RFC 7599 for IPv4-over-IPv6
- Processes DHCP option 95 for MAP-T configuration
- Validates MAP-T parameters

## Client Plugin Architecture

The DHCP Manager works with external DHCP clients through a plugin architecture:

### DHCPv4 (udhcpc)
- **Config Creator**: Generates udhcpc-compatible configuration
- **Plugin Binary**: `dhcpmgr_udhcpc_plugin` - executed on lease events
- **Communication**: IPC socket to lease monitor

### DHCPv6 (dibbler)
- **Config Creator**: Generates dibbler-compatible configuration
- **Plugin Binary**: `dhcpmgr_dibbler_plugin` - executed on lease events
- **Communication**: IPC socket to lease monitor

## Message Flow

1. **Initialization**: Main controller starts and initializes all components
2. **Client Start**: TR-181 configuration triggers DHCP client startup
3. **Lease Acquisition**: External DHCP client acquires lease
4. **Plugin Execution**: Client executes plugin with lease information
5. **IPC Communication**: Plugin sends lease data to lease monitor
6. **Processing**: Lease handlers process and validate lease information
7. **TR-181 Update**: Data model is updated with new lease information
8. **Persistence**: Lease information is stored for recovery
9. **Event Notification**: System events are generated for other components

## Integration Points

- **TR-181 Data Model**: Primary configuration and status interface
- **WAN Manager**: Lease status notifications
- **System Events**: Network configuration updates
- **RBUS/DBUS**: Management interface for external components

## Error Handling and Recovery

- **Process Monitoring**: Tracks DHCP client process health
- **Automatic Restart**: Restarts failed DHCP clients
- **Lease Persistence**: Maintains lease information across restarts
- **State Recovery**: Restores full system state after crashes

## Performance Considerations

- **Asynchronous Processing**: Non-blocking lease processing
- **Memory Management**: Efficient cleanup of lease structures
- **IPC Optimization**: Fast message processing in lease monitor
- **Thread Safety**: Proper synchronization across components

## Troubleshooting

Common issues and debugging approaches are documented in each component's specific documentation. Key areas to check:

1. **IPC Communication**: Verify lease monitor is listening
2. **Plugin Execution**: Check plugin binary permissions and paths
3. **Configuration**: Validate TR-181 settings
4. **Logs**: Review component logs for error messages
5. **Process Status**: Verify DHCP client processes are running

## Contributing

When modifying the DHCP Client Manager:

1. Follow existing code patterns and style
2. Update relevant documentation
3. Test with both IPv4 and IPv6 scenarios
4. Verify TR-181 compliance
5. Test recovery scenarios

## References

- [TR-181 Data Model](https://www.broadband-forum.org/technical/download/TR-181.pdf)
- [RFC 2131 - DHCP](https://tools.ietf.org/html/rfc2131)
- [RFC 3315 - DHCPv6](https://tools.ietf.org/html/rfc3315)
- [RFC 7599 - MAP-T](https://tools.ietf.org/html/rfc7599)
- [RDK-B Documentation](https://rdkcentral.github.io/rdkb/)