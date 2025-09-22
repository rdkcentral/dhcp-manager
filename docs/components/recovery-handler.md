# DHCP Recovery Handler

## Overview

The DHCP Recovery Handler provides critical persistence and recovery functionality for the DHCP Client Manager. It ensures that DHCP client state and lease information survive system restarts, crashes, and other service interruptions. The component implements a comprehensive recovery system that monitors client processes, stores lease data, and restores operational state upon system restart.

## Architecture

The recovery handler implements a multi-layered persistence and monitoring architecture:

```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   Process Monitor   │────►│  Recovery Handler   │────►│  Persistence Layer  │
│                     │     │                     │     │                     │
│ • PID Tracking      │     │ • State Management  │     │ • File Storage      │
│ • Health Checks     │     │ • Recovery Logic    │     │ • Data Integrity    │
│ • Event Detection   │     │ • Restart Control   │     │ • Atomic Operations │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
                                      │
                                      ▼
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   System Startup    │◄────│  Recovery Process   │────►│  Client Restoration │
│                     │     │                     │     │                     │
│ • Service Init      │     │ • Data Validation   │     │ • State Restoration │
│ • File System Check │     │ • Lease Recovery    │     │ • Process Restart   │
│ • Resource Alloc    │     │ • Configuration     │     │ • TR-181 Update     │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
```

## Key Components

### Files
- **Source**: `source/DHCPMgrUtils/dhcpmgr_recovery_handler.c`
- **Header**: `source/DHCPMgrUtils/dhcpmgr_recovery_handler.h`

### Core Functions

#### `DhcpMgr_Dhcp_Recovery_Start()`
- **Purpose**: Initializes and starts the DHCP recovery system
- **Returns**: 0 on success, negative error code on failure
- **Initialization**: Sets up process monitoring and loads existing lease data

#### `DHCPMgr_loadDhcpLeases()`
- **Purpose**: Loads previously stored DHCP lease information during system startup
- **Scope**: Processes both DHCPv4 and DHCPv6 lease files
- **Recovery**: Restores TR-181 data model and client state

## Persistence System

### Storage Architecture

#### Directory Structure
```
/tmp/Dhcp_manager/
├── dhcpLease_1_v4    # DHCPv4 client instance 1
├── dhcpLease_1_v6    # DHCPv6 client instance 1
├── dhcpLease_2_v4    # DHCPv4 client instance 2
└── dhcpLease_2_v6    # DHCPv6 client instance 2
```

#### File Naming Convention
- **Format**: `dhcpLease_{instanceNumber}_{version}`
- **Instance Number**: TR-181 client instance identifier
- **Version**: `v4` for DHCPv4, `v6` for DHCPv6

### DHCPv4 Lease Storage

#### `DHCPMgr_storeDhcpv4Lease(PCOSA_DML_DHCPC_FULL data)`

**Purpose**: Persists DHCPv4 lease information to filesystem

**Storage Process**:
1. **Directory Creation**: Ensure storage directory exists
2. **File Creation**: Create instance-specific lease file
3. **Data Writing**: Write complete client structure
4. **Lease Storage**: Append current lease information
5. **Error Handling**: Cleanup on write failures

**Data Structure Storage**:
```c
// Main client structure
fwrite(data, sizeof(COSA_DML_DHCPC_FULL), 1, file);

// Current lease information (if available)
if (data->currentLease != NULL) {
    fwrite(data->currentLease, sizeof(DHCPv4_PLUGIN_MSG), 1, file);
}
```

**Stored Information**:
- **Client Configuration**: TR-181 client settings
- **Operational State**: Current client status
- **Lease Data**: Active lease information
- **Timing Information**: Lease acquisition and expiration times
- **Network Configuration**: IP address, gateway, DNS settings

### DHCPv6 Lease Storage

#### `DHCPMgr_storeDhcpv6Lease(PCOSA_DML_DHCPCV6_FULL data)`

**Purpose**: Persists DHCPv6 lease information to filesystem

**IPv6-Specific Storage**:
1. **Client Structure**: Complete DHCPv6 client configuration
2. **IANA Information**: Non-temporary address data
3. **IAPD Information**: Prefix delegation data
4. **Timing Parameters**: T1, T2, preferred/valid lifetimes
5. **Server Information**: DHCPv6 server details

**Storage Format**:
```c
// Main client structure
fwrite(data, sizeof(COSA_DML_DHCPCV6_FULL), 1, file);

// Current lease information (if available)
if (data->currentLease != NULL) {
    fwrite(data->currentLease, sizeof(DHCPv6_PLUGIN_MSG), 1, file);
}
```

## Recovery Process

### System Startup Recovery

#### `DHCPMgr_loadDhcpLeases()`

**Recovery Flow**:
1. **Directory Scan**: Check for existing lease files
2. **File Validation**: Verify file integrity and format
3. **Data Loading**: Read client and lease structures
4. **State Restoration**: Restore TR-181 data model
5. **Client Restart**: Restart DHCP clients with recovered state

**Error Handling**:
- **Corrupted Files**: Skip corrupted lease files
- **Missing Data**: Use default configurations
- **Permission Issues**: Log errors and continue
- **Memory Allocation**: Graceful degradation

### DHCPv4 Recovery

#### `load_v4dhcp_leases()`

**Recovery Steps**:
1. **Client Enumeration**: Iterate through TR-181 DHCPv4 clients
2. **File Location**: Locate corresponding lease files
3. **Data Validation**: Verify lease data integrity
4. **Structure Restoration**: Rebuild client structures
5. **TR-181 Update**: Update data model with recovered information

**Validation Checks**:
```c
// File size validation
if (fileSize < sizeof(COSA_DML_DHCPC_FULL)) {
    DHCPMGR_LOG_ERROR("Invalid file size for client %lu", instanceNum);
    continue;
}

// Data integrity check
if (fread(&tempClient, sizeof(COSA_DML_DHCPC_FULL), 1, file) != 1) {
    DHCPMGR_LOG_ERROR("Failed to read client data");
    continue;
}
```

### DHCPv6 Recovery

#### `load_v6dhcp_leases()`

**IPv6-Specific Recovery**:
1. **Client Iteration**: Process all DHCPv6 client instances
2. **File Processing**: Load DHCPv6 lease files
3. **IANA/IAPD Recovery**: Restore address and prefix information
4. **Timer Restoration**: Recover lease timing parameters
5. **Configuration Apply**: Apply recovered IPv6 configuration

**IPv6 Validation**:
- **Address Format**: Validate IPv6 address formats
- **Prefix Length**: Verify prefix length validity
- **Timer Values**: Check timer consistency
- **Server Information**: Validate server data

## Process Monitoring

### PID Tracking System

#### Process Registration
```c
int pids[MAX_PIDS];  // Array of monitored PIDs
int pid_count = 0;   // Number of active PIDs
```

#### `dhcp_pid_mon(void *args)`

**Monitoring Process**:
1. **PID Collection**: Gather DHCP client process IDs
2. **Health Monitoring**: Continuously check process status
3. **Failure Detection**: Detect abnormal process termination
4. **Recovery Trigger**: Initiate recovery on process failure
5. **Cleanup**: Remove terminated processes from monitoring

**Process Status Check**:
```c
for (int i = 0; i < pid_count; i++) {
    if (pids[i] > 0) {
        int status;
        pid_t result = waitpid(pids[i], &status, WNOHANG);
        
        if (result == pids[i]) {
            // Process terminated
            DHCPMGR_LOG_INFO("Process %d terminated", pids[i]);
            processKilled(pids[i]);
            pids[i] = -1;
            active_pids--;
        }
    }
}
```

### Process Recovery

#### `processKilled(pid_t pid)`

**Recovery Actions**:
1. **Process Identification**: Determine which client process died
2. **State Assessment**: Evaluate impact of process termination
3. **Cleanup**: Clean up resources associated with dead process
4. **Restart Decision**: Determine if restart is appropriate
5. **Recovery Execution**: Restart client with recovered state

## Data Integrity

### File System Operations

#### Atomic Operations
- **Temporary Files**: Write to temporary files first
- **Atomic Rename**: Move temporary files to final location
- **Backup Strategy**: Maintain backup copies during updates
- **Rollback Capability**: Restore from backup on failure

#### `Create_Dir_ifnEx(const char *path)`

**Directory Management**:
```c
if (access(path, F_OK) == -1) {
    if (mkdir(path, 0755) == -1) {
        DHCPMGR_LOG_ERROR("Failed to create directory %s", path);
        return EXIT_FAIL;
    }
}
```

### Data Validation

#### File Integrity Checks
1. **Size Validation**: Verify file size matches expected structure size
2. **Magic Numbers**: Use file headers for format validation
3. **Checksum Verification**: Validate data integrity
4. **Version Compatibility**: Check file format versions

#### Structure Validation
1. **Pointer Validation**: Ensure pointers are valid or NULL
2. **Range Checking**: Validate numeric values within expected ranges
3. **String Validation**: Check string lengths and null termination
4. **Consistency Checks**: Verify related fields are consistent

## Error Handling and Resilience

### Recovery Strategies

#### Graceful Degradation
1. **Partial Recovery**: Recover what data is available
2. **Default Configuration**: Use defaults for missing data
3. **Continue Operation**: Don't let recovery failures stop service
4. **Error Reporting**: Log issues for later investigation

#### Failure Scenarios

##### Corrupted Lease Files
- **Detection**: File size or format validation fails
- **Action**: Skip corrupted file, use default configuration
- **Logging**: Record corruption for investigation

##### Missing Files
- **Detection**: Expected lease file not found
- **Action**: Start with clean client configuration
- **Logging**: Note missing file (may be normal)

##### Permission Issues
- **Detection**: File system access denied
- **Action**: Continue without persistence
- **Logging**: Record permission issues

##### Memory Allocation Failures
- **Detection**: malloc() returns NULL
- **Action**: Skip affected client, continue with others
- **Logging**: Record memory pressure

### Cleanup and Resource Management

#### `remove_dhcp_lease_file(int instanceNumber, int dhcpVersion)`

**Purpose**: Removes lease files when clients are deleted

**Parameters**:
- **instanceNumber**: TR-181 client instance number
- **dhcpVersion**: DHCP_v4 (0) or DHCP_v6 (1)

**Cleanup Process**:
1. **File Path Construction**: Build path to lease file
2. **Existence Check**: Verify file exists before deletion
3. **File Removal**: Delete lease file from filesystem
4. **Error Handling**: Log but don't fail on deletion errors

## Configuration and Customization

### Storage Configuration

#### Directory Settings
```c
#define TMP_DIR_PATH "/tmp/Dhcp_manager"
```

#### File Permissions
- **Directory**: 0755 (readable/executable by all, writable by owner)
- **Files**: Default permissions (typically 0644)

#### Size Limits
```c
#define MAX_PIDS 20  // Maximum monitored processes
```

### Recovery Behavior

#### Startup Behavior
- **Auto-Recovery**: Automatically load existing lease data
- **Validation**: Verify data integrity before use
- **Fallback**: Use defaults if recovery fails

#### Runtime Behavior
- **Continuous Monitoring**: Monitor process health
- **Immediate Storage**: Store lease changes immediately
- **Periodic Cleanup**: Remove stale lease files

## Performance Considerations

### Storage Efficiency

#### Minimize I/O Operations
- **Batch Writes**: Group related write operations
- **Async I/O**: Use asynchronous file operations where possible
- **Buffer Management**: Optimize buffer sizes for performance

#### File System Optimization
- **Temporary Storage**: Use RAM-based filesystem (/tmp)
- **Small Files**: Keep lease files small and focused
- **Efficient Formats**: Use binary formats for speed

### Memory Management

#### Resource Conservation
- **Stack Allocation**: Use stack for temporary variables
- **Heap Minimization**: Minimize dynamic allocation
- **Resource Cleanup**: Immediate cleanup after use

### Process Monitoring Efficiency

#### Polling Optimization
- **Adaptive Intervals**: Adjust monitoring frequency
- **Batch Checks**: Check multiple processes together
- **Event-driven**: Use signals where possible

## Testing and Validation

### Recovery Testing

#### Simulated Failures
1. **Process Termination**: Kill DHCP client processes
2. **File Corruption**: Corrupt lease files
3. **System Restart**: Test complete system recovery
4. **Resource Exhaustion**: Test under low memory conditions

#### Data Integrity Testing
1. **File Format Validation**: Verify stored data format
2. **Recovery Accuracy**: Compare recovered vs. original state
3. **Partial Recovery**: Test with incomplete data
4. **Concurrent Access**: Test with multiple processes

### Performance Testing

#### Storage Performance
- **Write Throughput**: Measure lease storage speed
- **Read Performance**: Measure recovery time
- **File System Impact**: Monitor file system usage

#### Monitoring Overhead
- **CPU Usage**: Monitor process monitoring overhead
- **Memory Usage**: Track memory consumption
- **System Impact**: Measure overall system impact

## Debugging and Troubleshooting

### Debug Features

#### Comprehensive Logging
- **Recovery Events**: Log all recovery operations
- **File Operations**: Log file system operations
- **Process Events**: Log process monitoring events
- **Error Conditions**: Detailed error reporting

#### Common Issues

##### Recovery Failures
1. **Corrupted Files**: Check file integrity and format
2. **Permission Denied**: Verify file system permissions
3. **Memory Issues**: Check available memory
4. **Process Issues**: Verify process monitoring

##### Performance Issues
1. **Slow Recovery**: Check file system performance
2. **High CPU Usage**: Monitor process monitoring overhead
3. **Memory Leaks**: Check resource cleanup
4. **File System Full**: Monitor disk space usage

### Diagnostic Tools

#### File Inspection
- **File Size Check**: Verify lease file sizes
- **Content Validation**: Manually inspect file contents
- **Timestamp Analysis**: Check file modification times

#### Process Monitoring
- **PID Tracking**: Monitor active process IDs
- **Status Checking**: Verify process health
- **Resource Usage**: Monitor process resource consumption

## Future Enhancements

### Planned Improvements
1. **Database Storage**: Replace file-based storage with database
2. **Replication**: Add data replication for high availability
3. **Compression**: Compress stored lease data
4. **Encryption**: Encrypt sensitive lease information
5. **Remote Storage**: Support network-based storage

### Advanced Features
1. **Incremental Backup**: Only backup changed data
2. **Transaction Support**: Atomic operations across multiple files
3. **Version Control**: Maintain history of lease changes
4. **Hot Backup**: Backup without service interruption
5. **Monitoring Dashboard**: Real-time recovery status display