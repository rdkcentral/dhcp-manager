# DHCP Client Manager Architecture

## Component Overview

The DHCP Client Manager architecture is designed around a modular, event-driven approach that provides centralized management of DHCP operations while maintaining clear separation of concerns between different functional areas.

## High-Level Architecture

```mermaid
graph TB
    subgraph "Application Layer"
        TR181[TR-181 Data Model]
        RBUS[RBUS/DBUS Interface]
        MGMT[Management APIs]
    end
    
    subgraph "DHCP Manager Core"
        CONTROLLER[Main Controller]
        MONITOR[Lease Monitor]
        RECOVERY[Recovery Handler]
    end
    
    subgraph "Protocol Handlers"
        V4HANDLER[DHCPv4 Handler]
        V6HANDLER[DHCPv6 Handler]
        MAPT[MAP-T Processor]
    end
    
    subgraph "External Clients"
        UDHCPC[udhcpc Client]
        DIBBLER[dibbler Client]
        V4PLUGIN[udhcpc Plugin]
        V6PLUGIN[dibbler Plugin]
    end
    
    subgraph "System Integration"
        NETIF[Network Interfaces]
        SYSEVENT[System Events]
        WAN[WAN Manager]
    end
    
    TR181 --> CONTROLLER
    RBUS --> CONTROLLER
    MGMT --> CONTROLLER
    
    CONTROLLER --> V4HANDLER
    CONTROLLER --> V6HANDLER
    CONTROLLER --> RECOVERY
    
    MONITOR --> CONTROLLER
    V4PLUGIN --> MONITOR
    V6PLUGIN --> MONITOR
    
    V4HANDLER --> UDHCPC
    V6HANDLER --> DIBBLER
    V6HANDLER --> MAPT
    
    UDHCPC --> V4PLUGIN
    DIBBLER --> V6PLUGIN
    
    V4HANDLER --> NETIF
    V6HANDLER --> NETIF
    V4HANDLER --> SYSEVENT
    V6HANDLER --> SYSEVENT
    
    CONTROLLER --> WAN
```

## Component Responsibilities

### Main Controller
- **Central Coordination**: Orchestrates all DHCP client operations
- **State Management**: Maintains client operational states
- **Interface Monitoring**: Tracks network interface status
- **Configuration Management**: Processes TR-181 configuration changes
- **Error Handling**: Implements recovery and error handling strategies

### Lease Monitor
- **IPC Server**: Receives lease updates from DHCP client plugins
- **Message Routing**: Routes lease information to appropriate handlers
- **Protocol Detection**: Identifies DHCPv4 vs DHCPv6 messages
- **Fast Processing**: Minimal latency message processing

### Protocol Handlers
- **DHCPv4 Handler**: Processes IPv4 lease information and network configuration
- **DHCPv6 Handler**: Processes IPv6 lease information including IANA/IAPD
- **MAP-T Processor**: Handles IPv4-over-IPv6 transition technology

### Recovery Handler
- **Persistence**: Stores lease information for system restart recovery
- **Process Monitoring**: Monitors DHCP client process health
- **State Restoration**: Restores system state after crashes or restarts

## Data Flow Architecture

```mermaid
sequenceDiagram
    participant CONFIG as TR-181 Config
    participant CONTROLLER as Main Controller
    participant CLIENT as DHCP Client
    participant PLUGIN as Client Plugin
    participant MONITOR as Lease Monitor
    participant HANDLER as Lease Handler
    participant SYSTEM as System/Network
    
    CONFIG->>CONTROLLER: Configuration Change
    CONTROLLER->>CLIENT: Start/Configure Client
    CLIENT->>CLIENT: Acquire Lease
    CLIENT->>PLUGIN: Execute Plugin
    PLUGIN->>MONITOR: Send Lease Info (IPC)
    MONITOR->>CONTROLLER: Route Lease Data
    CONTROLLER->>HANDLER: Process Lease
    HANDLER->>SYSTEM: Configure Network
    HANDLER->>CONFIG: Update TR-181 Status
    HANDLER->>CONTROLLER: Processing Complete
```

## Threading Model

### Thread Architecture

```mermaid
graph TB
    subgraph "Main Process"
        MAIN[Main Thread]
        CONTROLLER_T[Controller Thread]
        MONITOR_T[Monitor Thread]
        RECOVERY_T[Recovery Thread]
    end
    
    subgraph "External Processes"
        UDHCPC_P[udhcpc Process]
        DIBBLER_P[dibbler Process]
        PLUGIN_P[Plugin Processes]
    end
    
    MAIN --> CONTROLLER_T
    MAIN --> MONITOR_T
    MAIN --> RECOVERY_T
    
    CONTROLLER_T --> UDHCPC_P
    CONTROLLER_T --> DIBBLER_P
    
    UDHCPC_P --> PLUGIN_P
    DIBBLER_P --> PLUGIN_P
    
    PLUGIN_P --> MONITOR_T
```

### Thread Responsibilities

#### Main Thread
- **Initialization**: System startup and component initialization
- **TR-181 Operations**: Handle data model operations
- **RBUS Management**: Manage RBUS/DBUS communications
- **Signal Handling**: Process system signals and shutdown

#### Controller Thread
- **Client Management**: Start/stop/configure DHCP clients
- **Lease Processing**: Process lease updates from monitor
- **State Monitoring**: Monitor interface and client states
- **Configuration Updates**: Apply configuration changes

#### Monitor Thread
- **IPC Listening**: Listen for plugin messages
- **Message Processing**: Parse and validate lease messages
- **Data Routing**: Route messages to controller
- **Error Handling**: Handle communication errors

#### Recovery Thread
- **Process Monitoring**: Monitor DHCP client processes
- **Health Checking**: Detect process failures
- **Recovery Actions**: Restart failed clients
- **Cleanup**: Remove dead process tracking

## Message Flow Patterns

### Lease Acquisition Flow

```mermaid
graph TD
    START[Interface Up] --> CHECK{Interface Ready?}
    CHECK -->|No| WAIT[Wait for Ready]
    WAIT --> CHECK
    CHECK -->|Yes| START_CLIENT[Start DHCP Client]
    START_CLIENT --> DISCOVER[DHCP Discover/Solicit]
    DISCOVER --> RESPONSE{Server Response?}
    RESPONSE -->|No| TIMEOUT[Timeout/Retry]
    TIMEOUT --> DISCOVER
    RESPONSE -->|Yes| PLUGIN[Execute Plugin]
    PLUGIN --> IPC[Send IPC Message]
    IPC --> PROCESS[Process Lease]
    PROCESS --> CONFIGURE[Configure Interface]
    CONFIGURE --> UPDATE[Update TR-181]
    UPDATE --> COMPLETE[Complete]
```

### Error Recovery Flow

```mermaid
graph TD
    ERROR[Error Detected] --> TYPE{Error Type?}
    TYPE -->|Process Died| RESTART[Restart Client]
    TYPE -->|Config Error| RECONFIG[Reconfigure Client]
    TYPE -->|Network Error| RETRY[Retry Operation]
    TYPE -->|Critical Error| DISABLE[Disable Client]
    
    RESTART --> CHECK[Check Recovery]
    RECONFIG --> CHECK
    RETRY --> CHECK
    
    CHECK --> SUCCESS{Recovery OK?}
    SUCCESS -->|Yes| NORMAL[Normal Operation]
    SUCCESS -->|No| ESCALATE[Escalate Error]
    
    DISABLE --> LOG[Log Error]
    ESCALATE --> LOG
    LOG --> COMPLETE[Complete]
    NORMAL --> COMPLETE
```

## Interface Architecture

### Network Interface Integration

```mermaid
graph TB
    subgraph "Network Stack"
        KERNEL[Kernel Network]
        NETDEV[Network Devices]
        ROUTES[Routing Table]
        DNS[DNS Configuration]
    end
    
    subgraph "DHCP Manager"
        CONTROLLER[Controller]
        V4HANDLER[DHCPv4 Handler]
        V6HANDLER[DHCPv6 Handler]
    end
    
    subgraph "System Integration"
        SYSEVENT[System Events]
        WAN[WAN Manager]
        FIREWALL[Firewall]
        ROUTING[Routing Daemon]
    end
    
    CONTROLLER --> NETDEV
    V4HANDLER --> NETDEV
    V4HANDLER --> ROUTES
    V4HANDLER --> DNS
    V6HANDLER --> NETDEV
    V6HANDLER --> ROUTES
    V6HANDLER --> DNS
    
    V4HANDLER --> SYSEVENT
    V6HANDLER --> SYSEVENT
    SYSEVENT --> WAN
    SYSEVENT --> FIREWALL
    SYSEVENT --> ROUTING
```

### TR-181 Integration Points

The DHCP Manager integrates with TR-181 at multiple levels:

#### Configuration Parameters
- `Device.DHCPv4.Client.{i}.*` - DHCPv4 client configuration
- `Device.DHCPv6.Client.{i}.*` - DHCPv6 client configuration
- Custom vendor parameters and extensions

#### Status Parameters
- Lease information and status
- Client operational state
- Error conditions and diagnostics
- Performance metrics

#### Operational Commands
- Client enable/disable operations
- Lease renewal commands
- Reset and restart operations
- Diagnostic commands

## Security Architecture

### Process Isolation

```mermaid
graph TB
    subgraph "Privileged Context"
        MANAGER[DHCP Manager]
        CONTROLLER[Controller]
    end
    
    subgraph "Network Context"
        UDHCPC[udhcpc Client]
        DIBBLER[dibbler Client]
    end
    
    subgraph "Plugin Context"
        V4PLUGIN[udhcpc Plugin]
        V6PLUGIN[dibbler Plugin]
    end
    
    subgraph "System Context"
        NETCONFIG[Network Config]
        FILESYSTEM[File System]
    end
    
    MANAGER --> CONTROLLER
    CONTROLLER --> UDHCPC
    CONTROLLER --> DIBBLER
    
    UDHCPC --> V4PLUGIN
    DIBBLER --> V6PLUGIN
    
    V4PLUGIN --> MANAGER
    V6PLUGIN --> MANAGER
    
    CONTROLLER --> NETCONFIG
    CONTROLLER --> FILESYSTEM
```

### Security Considerations

#### Process Security
- **Privilege Separation**: Different components run with minimal required privileges
- **Process Isolation**: DHCP clients run as separate processes
- **Plugin Sandboxing**: Plugins have limited system access

#### Communication Security
- **Local IPC**: All communication uses local IPC mechanisms
- **Message Validation**: All messages are validated before processing
- **Error Isolation**: Errors in one component don't affect others

#### File System Security
- **Temporary Storage**: Recovery files stored in secure temporary directory
- **Permission Control**: Appropriate file permissions for lease data
- **Atomic Operations**: Atomic file operations prevent corruption

## Performance Architecture

### Optimization Strategies

#### Memory Management
- **Static Allocation**: Prefer stack allocation over dynamic allocation
- **Memory Pooling**: Reuse common data structures
- **Lazy Loading**: Load data only when needed
- **Garbage Collection**: Timely cleanup of unused resources

#### Processing Optimization
- **Asynchronous Processing**: Non-blocking lease processing
- **Batch Operations**: Group related operations
- **Caching**: Cache frequently accessed data
- **Fast Path**: Optimize common operation paths

#### I/O Optimization
- **Buffered I/O**: Use appropriate buffer sizes
- **Async I/O**: Non-blocking file operations where possible
- **Minimal Syscalls**: Reduce system call overhead
- **Efficient Protocols**: Use binary protocols for IPC

## Scalability Considerations

### Multi-Interface Support
- **Parallel Processing**: Handle multiple interfaces concurrently
- **Resource Scaling**: Scale resources based on interface count
- **State Isolation**: Isolate state between interfaces
- **Load Distribution**: Distribute processing across available cores

### Performance Limits
- **Interface Count**: Efficiently handle dozens of interfaces
- **Lease Volume**: Process high lease turnover rates
- **Memory Usage**: Bounded memory usage regardless of scale
- **Response Time**: Maintain consistent response times under load

## Extension Points

### Plugin Architecture
- **Client Plugins**: Extend support to new DHCP clients
- **Protocol Extensions**: Add support for new DHCP options
- **Custom Handlers**: Implement custom lease processing
- **Integration Hooks**: Add hooks for external system integration

### Configuration Extensions
- **Custom Options**: Support for vendor-specific options
- **Policy Engines**: Pluggable policy decision engines
- **Validation Rules**: Custom validation rule engines
- **Event Handlers**: Custom event processing extensions

This architectural documentation provides the foundation for understanding how the DHCP Client Manager components interact and can be extended or modified to meet specific deployment requirements.