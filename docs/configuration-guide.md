# DHCP Client Configuration Guide

## Overview

This guide provides comprehensive instructions for configuring DHCP clients using PSM (Persistent Storage Manager) parameters. The DHCP Client Manager supports both DHCPv4 and DHCPv6 clients with extensive customization options including vendor-specific options, custom parameters, and integration with WAN Manager.

## Configuration Architecture

The DHCP Client Manager configuration follows a hierarchical structure:

```
dmsb.dhcpmanager
├── ClientNoOfEntries                    # Number of DHCPv4 clients
├── Client.{i}                          # DHCPv4 client instances
│   ├── Alias                           # Client alias/name
│   ├── ReqOptionNoOfEntries            # Number of requested options
│   ├── ReqOption.{i}                   # Requested DHCP options
│   ├── SendOptionNoOfEntries           # Number of sent options
│   └── SendOption.{i}                  # Sent DHCP options
└── dhcpv6
    ├── ClientNoOfEntries               # Number of DHCPv6 clients
    └── Client.{i}                      # DHCPv6 client instances
        ├── Alias                       # Client alias/name
        ├── ReqAddr                     # Request IPv6 address (IANA)
        ├── ReqPrefix                   # Request prefix delegation (IAPD)
        ├── RequestedOptions            # Comma-separated option list
        ├── SentOptNoOfEntries          # Number of sent options
        └── SentOption.{i}              # Sent DHCPv6 options
```

## DHCPv4 Client Configuration

### Basic Client Setup

#### Client Instance Configuration
```xml
<!-- Number of DHCPv4 clients -->
<Record name="dmsb.dhcpmanager.ClientNoOfEntries" type="astr">2</Record>

<!-- Client 1: Cable Modem WAN -->
<Record name="dmsb.dhcpmanager.Client.1.Alias" type="astr">CABLE_WAN</Record>

<!-- Client 2: Ethernet WAN -->
<Record name="dmsb.dhcpmanager.Client.2.Alias" type="astr">ETHERNET_WAN</Record>
```

### Requested Options Configuration

DHCPv4 clients can request specific options from the DHCP server:

```xml
<!-- Client 1 Requested Options -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOptionNoOfEntries" type="astr">4</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.1.Tag" type="astr">1</Record>   <!-- Subnet Mask -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.1.Order" type="astr">1</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.2.Tag" type="astr">3</Record>   <!-- Router -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.2.Order" type="astr">2</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.3.Tag" type="astr">6</Record>   <!-- DNS Server -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.3.Order" type="astr">3</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.4.Tag" type="astr">42</Record>  <!-- NTP Server -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.4.Order" type="astr">4</Record>
```

#### Common DHCPv4 Options
| Option | Description | Use Case |
|--------|-------------|----------|
| 1 | Subnet Mask | Network configuration |
| 3 | Router | Default gateway |
| 6 | DNS Server | Name resolution |
| 15 | Domain Name | DNS domain |
| 42 | NTP Server | Time synchronization |
| 43 | Vendor Specific | Custom vendor data |
| 60 | Vendor Class ID | Client identification |
| 125 | Vendor-Identifying Vendor Class | Enhanced vendor info |

### Sent Options Configuration

DHCPv4 clients can send custom options to the server:

```xml
<!-- Client 1 Sent Options -->
<Record name="dmsb.dhcpmanager.Client.1.SendOptionNoOfEntries" type="astr">3</Record>

<!-- Option 60: Vendor Class Identifier -->
<Record name="dmsb.dhcpmanager.Client.1.SendOption.1.Tag" type="astr">60</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.1.Value" type="astr">4D79564E444F52313233</Record>

<!-- Option 43: Vendor Specific Information -->
<Record name="dmsb.dhcpmanager.Client.1.SendOption.2.Tag" type="astr">43</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.2.Value" type="astr">010C4D6F64656D5365726E756D</Record>

<!-- Option 61: Client Identifier -->
<Record name="dmsb.dhcpmanager.Client.1.SendOption.3.Tag" type="astr">61</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.3.Value" type="astr">0014220165498132</Record>
```

### Advanced DHCPv4 Example

```xml
<!-- Enterprise WAN Configuration -->
<Record name="dmsb.dhcpmanager.Client.2.Alias" type="astr">ENTERPRISE_WAN</Record>

<!-- Request enterprise-specific options -->
<Record name="dmsb.dhcpmanager.Client.2.ReqOptionNoOfEntries" type="astr">6</Record>
<Record name="dmsb.dhcpmanager.Client.2.ReqOption.1.Tag" type="astr">1</Record>    <!-- Subnet Mask -->
<Record name="dmsb.dhcpmanager.Client.2.ReqOption.1.Order" type="astr">1</Record>
<Record name="dmsb.dhcpmanager.Client.2.ReqOption.2.Tag" type="astr">3</Record>    <!-- Router -->
<Record name="dmsb.dhcpmanager.Client.2.ReqOption.2.Order" type="astr">2</Record>
<Record name="dmsb.dhcpmanager.Client.2.ReqOption.3.Tag" type="astr">6</Record>    <!-- DNS Server -->
<Record name="dmsb.dhcpmanager.Client.2.ReqOption.3.Order" type="astr">3</Record>
<Record name="dmsb.dhcpmanager.Client.2.ReqOption.4.Tag" type="astr">15</Record>   <!-- Domain Name -->
<Record name="dmsb.dhcpmanager.Client.2.ReqOption.4.Order" type="astr">4</Record>
<Record name="dmsb.dhcpmanager.Client.2.ReqOption.5.Tag" type="astr">119</Record>  <!-- Domain Search -->
<Record name="dmsb.dhcpmanager.Client.2.ReqOption.5.Order" type="astr">5</Record>
<Record name="dmsb.dhcpmanager.Client.2.ReqOption.6.Tag" type="astr">121</Record>  <!-- Classless Route -->
<Record name="dmsb.dhcpmanager.Client.2.ReqOption.6.Order" type="astr">6</Record>

<!-- Send enterprise identification -->
<Record name="dmsb.dhcpmanager.Client.2.SendOptionNoOfEntries" type="astr">2</Record>
<Record name="dmsb.dhcpmanager.Client.2.SendOption.1.Tag" type="astr">60</Record>
<Record name="dmsb.dhcpmanager.Client.2.SendOption.1.Value" type="astr">456E746572707269736552000000</Record>
<Record name="dmsb.dhcpmanager.Client.2.SendOption.2.Tag" type="astr">125</Record>
<Record name="dmsb.dhcpmanager.Client.2.SendOption.2.Value" type="astr">456E746572707269736552000000</Record>
```

## DHCPv6 Client Configuration

### Basic DHCPv6 Setup

```xml
<!-- Number of DHCPv6 clients -->
<Record name="dmsb.dhcpmanager.dhcpv6.ClientNoOfEntries" type="astr">2</Record>

<!-- Client 1: Cable WAN with address request -->
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.Alias" type="astr">CABLE_IPV6</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.ReqAddr" type="astr">TRUE</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.ReqPrefix" type="astr">FALSE</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.RequestedOptions" type="astr">23,24,39</Record>
```

### DHCPv6 Options Configuration

#### Common DHCPv6 Options
| Option | Description | Use Case |
|--------|-------------|----------|
| 16 | Vendor Class | Client identification |
| 17 | Vendor Specific | Custom vendor data |
| 21 | SIP Server Domain | VoIP configuration |
| 23 | DNS Server | Name resolution |
| 24 | Domain Search | DNS search domains |
| 25 | Identity Association PD | Prefix delegation |
| 39 | Client FQDN | Fully qualified domain name |
| 95 | S46 MAP-T Container | IPv4-over-IPv6 transition |

### DHCPv6 Sent Options

```xml
<!-- DHCPv6 Client 1 Sent Options -->
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOptNoOfEntries" type="astr">4</Record>

<!-- Option 16: Vendor Class -->
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOption.1.Alias" type="astr">vendor-class</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOption.1.Tag" type="astr">16</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOption.1.Value" type="astr">0000118b000C526F757465725F563200</Record>

<!-- Option 17: Vendor Specific Information -->
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOption.2.Alias" type="astr">vendor-info</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOption.2.Tag" type="astr">17</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOption.2.Value" type="astr">0000118b0001000800260027000000010002000C526F757465725F563231</Record>

<!-- Option 25: Identity Association for Prefix Delegation -->
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOption.3.Alias" type="astr">ia-pd</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOption.3.Tag" type="astr">25</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOption.3.Value" type="astr">000000010000000000000000</Record>

<!-- Option 39: Client FQDN -->
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOption.4.Alias" type="astr">client-fqdn</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOption.4.Tag" type="astr">39</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.SentOption.4.Value" type="astr">0007726F7574657207657861E1706C6503636F6D00</Record>
```


## Custom Options and API Integration

### Hex String Encoding

All option values are encoded as hexadecimal strings:

#### Examples:
```
"Router2.0" → 526F75746572322E30
"VENDOR123" → 56454E444F52313233
```

### Custom API Integration

The DHCP Manager supports custom option APIs for dynamic value generation:

#### Weak Implementation
By default, the DHCP Manager provides weak implementations for custom options:
```c
// Weak implementation - can be overridden
__attribute__((weak)) 
int dhcpv4_get_vendor_class_id(char *buffer, size_t buflen) {
    // Default implementation
    return -1;
}

__attribute__((weak))
int dhcpv6_get_vendor_specific_info(char *buffer, size_t buflen) {
    // Default implementation  
    return -1;
}
```

#### Strong Implementation
Link with a library providing strong implementations:
```c
// Strong implementation in external library
int dhcpv4_get_vendor_class_id(char *buffer, size_t buflen) {
    // Custom implementation for option 60
    snprintf(buffer, buflen, "MyDevice_v1.0_%s", get_device_serial());
    return 0;
}

int dhcpv6_get_vendor_specific_info(char *buffer, size_t buflen) {
    // Custom implementation for option 17
    build_vendor_info_tlv(buffer, buflen);
    return 0;
}
```

#### Custom Option Behavior
- **With PSM Value**: If a PSM value is configured, it takes precedence
- **Without PSM Value**: Custom API is called to generate the value
- **API Failure**: Option is omitted from DHCP request/response

## WAN Manager Integration

### Interface Binding

Link DHCP clients to WAN Manager interfaces:

```xml
<!-- WAN Interface 1: Cable Modem -->
<Record name="dmsb.wanmanager.if.1.VirtualInterface.1.IP.DHCPV4Interface" type="astr">Device.DHCPv4.Client.1</Record>
<Record name="dmsb.wanmanager.if.1.VirtualInterface.1.IP.DHCPV6Interface" type="astr">Device.DHCPv6.Client.1</Record>

<!-- WAN Interface 2: Ethernet -->
<Record name="dmsb.wanmanager.if.2.VirtualInterface.1.IP.DHCPV4Interface" type="astr">Device.DHCPv4.Client.2</Record>
<Record name="dmsb.wanmanager.if.2.VirtualInterface.1.IP.DHCPV6Interface" type="astr">Device.DHCPv6.Client.2</Record>
```

### Interface Configuration Flow

1. **WAN Manager** reads DHCP interface bindings from PSM
2. **DHCP Manager** receives interface enable/disable events
3. **Client Selection** based on interface type and configuration
4. **Option Processing** using PSM configuration
5. **Lease Processing** and interface configuration
6. **Status Reporting** back to WAN Manager

## Configuration Examples

### Cable Modem Configuration

```xml
<!-- Cable Modem DHCP Configuration -->
<Record name="dmsb.dhcpmanager.ClientNoOfEntries" type="astr">1</Record>
<Record name="dmsb.dhcpmanager.Client.1.Alias" type="astr">CABLE_MODEM</Record>

<!-- Request cable-specific options -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOptionNoOfEntries" type="astr">5</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.1.Tag" type="astr">1</Record>   <!-- Subnet Mask -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.1.Order" type="astr">1</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.2.Tag" type="astr">3</Record>   <!-- Router -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.2.Order" type="astr">2</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.3.Tag" type="astr">6</Record>   <!-- DNS -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.3.Order" type="astr">3</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.4.Tag" type="astr">7</Record>   <!-- Log Server -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.4.Order" type="astr">4</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.5.Tag" type="astr">42</Record>  <!-- Time Server -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.5.Order" type="astr">5</Record>

<!-- Send cable modem identification -->
<Record name="dmsb.dhcpmanager.Client.1.SendOptionNoOfEntries" type="astr">2</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.1.Tag" type="astr">60</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.1.Value" type="astr">646F637369732D332E302D436162</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.2.Tag" type="astr">61</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.2.Value" type="astr">01AA00040000FF00</Record>

<!-- DHCPv6 Configuration -->
<Record name="dmsb.dhcpmanager.dhcpv6.ClientNoOfEntries" type="astr">1</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.Alias" type="astr">CABLE_IPV6</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.ReqAddr" type="astr">TRUE</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.ReqPrefix" type="astr">TRUE</Record>
<Record name="dmsb.dhcpmanager.dhcpv6.Client.1.RequestedOptions" type="astr">23,24</Record>
```

### Enterprise Router Configuration

```xml
<!-- Enterprise DHCP Configuration -->
<Record name="dmsb.dhcpmanager.ClientNoOfEntries" type="astr">1</Record>
<Record name="dmsb.dhcpmanager.Client.1.Alias" type="astr">ENTERPRISE</Record>

<!-- Enterprise-specific options -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOptionNoOfEntries" type="astr">7</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.1.Tag" type="astr">1</Record>    <!-- Subnet Mask -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.1.Order" type="astr">1</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.2.Tag" type="astr">3</Record>    <!-- Router -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.2.Order" type="astr">2</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.3.Tag" type="astr">6</Record>    <!-- DNS -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.3.Order" type="astr">3</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.4.Tag" type="astr">15</Record>   <!-- Domain -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.4.Order" type="astr">4</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.5.Tag" type="astr">119</Record>  <!-- Domain Search -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.5.Order" type="astr">5</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.6.Tag" type="astr">121</Record>  <!-- Classless Route -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.6.Order" type="astr">6</Record>
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.7.Tag" type="astr">249</Record>  <!-- Private/Classless Static Route -->
<Record name="dmsb.dhcpmanager.Client.1.ReqOption.7.Order" type="astr">7</Record>

<!-- Enterprise identification -->
<Record name="dmsb.dhcpmanager.Client.1.SendOptionNoOfEntries" type="astr">3</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.1.Tag" type="astr">60</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.1.Value" type="astr">456E746572707269736552000000</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.2.Tag" type="astr">77</Record>  <!-- User Class -->
<Record name="dmsb.dhcpmanager.Client.1.SendOption.2.Value" type="astr">06526F75746572</Record>
<Record name="dmsb.dhcpmanager.Client.1.SendOption.3.Tag" type="astr">125</Record> <!-- Vendor-Identifying Vendor Class -->
<Record name="dmsb.dhcpmanager.Client.1.SendOption.3.Value" type="astr">0000118b01020304050607080910</Record>
```

## Troubleshooting Configuration

### Common Issues

#### 1. Option Not Sent
- **Cause**: Missing PSM configuration or failed custom API
- **Solution**: Check PSM values and custom API implementation

#### 2. Invalid Hex Values
- **Cause**: Malformed hexadecimal strings
- **Solution**: Validate hex encoding (even number of hex digits)

#### 3. Option Order Issues
- **Cause**: Incorrect or duplicate order values
- **Solution**: Ensure sequential order numbering

#### 4. Interface Binding Failures
- **Cause**: Incorrect WAN Manager interface references
- **Solution**: Verify Device.DHCPv*.Client.* paths

### Validation Commands

```bash
# Check DHCP client configuration
dmcli eRT getv Device.DHCPv4.Client.

# Verify option configuration
dmcli eRT getv Device.DHCPv4.Client.1.ReqOption.

# Check DHCPv6 configuration
dmcli eRT getv Device.DHCPv6.Client.

# View WAN Manager bindings
dmcli eRT getv Device.X_RDK_WanManager.Interface.
```

## Best Practices

### 1. Option Management
- Use consistent hex encoding
- Implement proper error handling in custom APIs
- Document custom option meanings
- Test option combinations thoroughly

### 2. Configuration Validation
- Validate PSM parameters before deployment
- Test with actual DHCP servers
- Monitor lease acquisition success rates
- Verify network connectivity post-configuration

### 3. Custom API Development
- Implement comprehensive error handling
- Use appropriate buffer management
- Validate generated option data
- Provide fallback mechanisms

### 4. Integration Testing
- Test WAN Manager integration
- Verify interface state transitions
- Validate option processing
- Check lease renewal behavior

This configuration guide provides comprehensive coverage of DHCP Client Manager configuration using PSM parameters, enabling flexible and powerful DHCP client customization for various deployment scenarios.