# DHCP MAP-T APIs

## Overview

The DHCP MAP-T (Mapping of Address and Port using Translation) APIs implement RFC 7599 support for IPv4-over-IPv6 transition technology. This component processes DHCP option 95 (S46_CONT_MAPT) to configure MAP-T parameters, enabling IPv4 connectivity over IPv6-only networks through address and port mapping.

## Architecture

The MAP-T implementation follows the RFC 7599 specification with comprehensive parameter processing:

```
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   DHCPv6 Client     │────►│   DHCP Option 95    │────►│    MAP-T Parser     │
│                     │     │                     │     │                     │
│ • Option Request    │     │ • S46_CONT_MAPT     │     │ • Option Validation │
│ • Server Response   │     │ • Nested Options    │     │ • Parameter Extract │
│ • Raw Option Data   │     │ • TLV Format        │     │ • Rule Processing   │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
                                      │
                                      ▼
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│   Configuration     │◄────│   MAP-T Processor   │────►│   System Setup      │
│                     │     │                     │     │                     │
│ • Rule Parameters   │     │ • Rule Validation   │     │ • Tunnel Config     │
│ • Border Relay      │     │ • BR Processing     │     │ • Routing Setup     │
│ • Port Mapping      │     │ • Port Calculation  │     │ • Address Config    │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
```

## Key Components

### Files
- **Source**: `source/DHCPMgrUtils/dhcpmgr_map_apis.c`
- **Header**: `source/DHCPMgrUtils/dhcpmgr_map_apis.h`

### Core Functions

#### `DhcpMgr_MaptParseOpt95Response()`
- **Purpose**: Main entry point for processing DHCP option 95 (MAP-T container)
- **Input**: Raw DHCP option data from DHCPv6 client
- **Output**: Parsed MAP-T configuration parameters
- **Integration**: Called from DHCPv6 lease processing

## MAP-T DHCP Options

### Option Hierarchy (RFC 7599)

#### Option 95: S46_CONT_MAPT (MAP-T Container)
- **Purpose**: Contains all MAP-T related sub-options
- **Format**: TLV (Type-Length-Value) container
- **Sub-options**: Contains nested MAP-T specific options

#### Option 89: S46_RULE (MAP Rule)
- **Purpose**: Defines mapping rules for IPv4-to-IPv6 translation
- **Components**:
  - IPv6 prefix for MAP domain
  - IPv4 prefix for IPv4 pool
  - EA (Embedded Address) bits
  - Port mapping parameters

#### Option 90: S46_BR (Border Relay)
- **Purpose**: Specifies Border Relay IPv6 address
- **Usage**: Destination for encapsulated IPv4 traffic
- **Format**: IPv6 address

#### Option 93: S46_PORT_PARAMS (Port Parameters)
- **Purpose**: Defines port mapping parameters
- **Components**:
  - PSID (Port Set Identifier)
  - PSID length
  - PSID offset

## Data Structures

### Main MAP-T Data Structure

```c
typedef struct _COSA_DML_MAPT_DATA {
    // IPv4 Configuration
    CHAR       RuleIPv4Prefix[BUFLEN_16];     // IPv4 prefix string
    UINT16     RuleIPv4PrefixLen;             // IPv4 prefix length
    CHAR       IPv4AddrString[BUFLEN_16];     // Calculated IPv4 address
    UINT32     IPv4Suffix;                    // IPv4 address suffix
    
    // IPv6 Configuration
    CHAR       RuleIPv6Prefix[BUFLEN_40];     // IPv6 prefix string
    UCHAR      RuleIPv6PrefixH[BUFLEN_24];    // IPv6 prefix (hex format)
    UINT16     RuleIPv6PrefixLen;             // IPv6 prefix length
    CHAR       IPv6AddrString[BUFLEN_40];     // Calculated IPv6 address
    
    // Border Relay Configuration
    CHAR       BrIPv6Prefix[BUFLEN_40];       // Border Relay IPv6 address
    UINT16     BrIPv6PrefixLen;               // BR prefix length
    
    // Port Mapping Parameters
    UINT16     Psid;                          // Port Set Identifier
    UINT16     PsidLen;                       // PSID length in bits
    UINT32     PsidOffset;                    // PSID offset
    UINT16     IPv4Psid;                      // Calculated IPv4 PSID
    UINT16     IPv4PsidLen;                   // IPv4 PSID length
    
    // Derived Parameters
    CHAR       PdIPv6Prefix[BUFLEN_40];       // Prefix Delegation prefix
    UINT16     PdIPv6PrefixLen;               // PD prefix length
    UINT16     EaLen;                         // Embedded Address length
    UINT32     Ratio;                         // Sharing ratio
    BOOLEAN    bFMR;                          // Forwarding Mapping Rule flag
} COSA_DML_MAPT_DATA, *PCOSA_DML_MAPT_DATA;
```

### DHCP Option Structure

```c
typedef struct _COSA_DML_MAPT_OPTION {
    UINT16     OptType;                       // Option type (89, 90, 93, etc.)
    UINT16     OptLen;                        // Option length
} __attribute__ ((__packed__)) COSA_DML_MAPT_OPTION, *PCOSA_DML_MAPT_OPTION;
```

## Processing Functions

### Option Parsing

#### `CosaDmlMaptParseResponse(PUCHAR pOptionBuf, UINT16 uiOptionBufLen)`

**Purpose**: Parses MAP-T container option and extracts sub-options

**Processing Flow**:
1. **Option Iteration**: Walk through TLV structure
2. **Type Identification**: Identify each sub-option type
3. **Length Validation**: Verify option lengths
4. **Data Extraction**: Extract option-specific data
5. **Parameter Calculation**: Derive MAP-T parameters

**Supported Options**:
```c
switch (uiOption) {
    case MAPT_OPTION_S46_RULE:      // Option 89: MAP Rule
        // Process rule parameters
        break;
    case MAPT_OPTION_S46_BR:        // Option 90: Border Relay
        // Process BR IPv6 address
        break;
    case MAPT_OPTION_S46_PORT_PARAMS: // Option 93: Port Parameters
        // Process PSID parameters
        break;
    default:
        // Unknown option - skip
        break;
}
```

#### `CosaDmlMaptConvertStringToHexStream(PUCHAR pWriteBf, PUINT16 uiOptionBufLen)`

**Purpose**: Converts hex string representation to binary data

**Input Processing**:
- **String Format**: Hex string with optional quotes
- **Byte Conversion**: Convert hex pairs to binary bytes
- **Length Calculation**: Return actual binary length

**Conversion Logic**:
```c
while (*pReadBf && *(pReadBf+1)) {
    *pWriteBf = STRING_TO_HEX(*pReadBf) << 4;
    *pWriteBf |= STRING_TO_HEX(*(pReadBf+1));
    pReadBf += 2;
    pWriteBf++;
    (*uiOptionBufLen)++;
}
```

### Address Calculation

#### `CosaDmlMaptGetIPv6StringFromHex(PUCHAR pIPv6AddrH, PCHAR pIPv6AddrS)`

**Purpose**: Converts binary IPv6 address to string format

**Processing**:
- **Binary Input**: IPv6 address in binary format
- **String Conversion**: Use `inet_ntop()` for standard formatting
- **Error Handling**: Validate conversion success

### Parameter Validation

#### `CosaDmlMaptValidate()`

**Purpose**: Validates MAP-T parameters for consistency and correctness

**Validation Checks**:
1. **Prefix Lengths**: Verify IPv4 and IPv6 prefix lengths are valid
2. **EA Bits**: Ensure Embedded Address bits are calculated correctly
3. **PSID Parameters**: Validate Port Set Identifier configuration
4. **Sharing Ratio**: Verify port sharing calculations
5. **Address Ranges**: Ensure address calculations are within valid ranges

**Validation Formula**:
```c
// EA bits length calculation
ui8EaLen = ui16PdPrefixLen - ui16v6PrefixLen;

// PSID length calculation
ui8PsidBitIdxLen = ui8EaLen - ui8v4BitIdxLen;

// Validation checks
if (ui8PsidBitIdxLen < 0 || ui8PsidBitIdxLen > 16) {
    return STATUS_FAILURE;
}
```

## MAP-T Parameter Calculation

### Address Mapping

#### IPv4 Address Calculation
The IPv4 address is derived from the MAP rule and user prefix:
1. **Rule IPv4 Prefix**: Base IPv4 prefix from MAP rule
2. **EA Bits**: Embedded Address bits from IPv6 prefix
3. **Address Construction**: Combine prefix and EA bits

#### IPv6 Address Calculation
The IPv6 address embeds IPv4 information:
1. **MAP Domain Prefix**: IPv6 prefix for MAP domain
2. **Embedded IPv4**: IPv4 address embedded in IPv6
3. **PSID Embedding**: Port Set ID embedded in IPv6

### Port Mapping

#### PSID Calculation
Port Set Identifier determines available port ranges:
```c
// Extract PSID from IPv6 prefix
PSID = (EA_bits >> (32 - v4_prefix_len - PSID_len)) & ((1 << PSID_len) - 1);

// Calculate port range
port_range_start = (PSID << (16 - PSID_len - PSID_offset)) + PSID_offset;
port_range_size = 1 << (16 - PSID_len - PSID_offset);
```

#### Sharing Ratio
The sharing ratio determines how many users share one IPv4 address:
```c
sharing_ratio = 1 << PSID_len;
```

## Integration with DHCP System

### DHCPv6 Option Processing

#### Option Request
The DHCPv6 client must request MAP-T options:
```c
// Request MAP-T container option
OPTION_S46_CONT_MAPT = 95
```

#### Option Reception
When option 95 is received in DHCPv6 response:
1. **Extraction**: Extract option data from DHCPv6 response
2. **Parsing**: Call `DhcpMgr_MaptParseOpt95Response()`
3. **Validation**: Validate extracted parameters
4. **Configuration**: Apply MAP-T configuration to system

### System Configuration

#### Tunnel Interface Setup
MAP-T requires tunnel interface configuration:
1. **Interface Creation**: Create MAP-T tunnel interface
2. **Address Assignment**: Assign calculated IPv6 address
3. **Route Configuration**: Set up routing for IPv4 traffic
4. **Border Relay**: Configure BR as tunnel endpoint

#### Traffic Flow
1. **IPv4 Packets**: Application sends IPv4 packets
2. **Encapsulation**: Kernel encapsulates in IPv6
3. **Transmission**: IPv6 packets sent to Border Relay
4. **Decapsulation**: BR extracts IPv4 and forwards

## Error Handling

### Validation Errors

#### Parameter Validation
- **Invalid Prefixes**: Check prefix length validity
- **EA Bit Errors**: Verify EA bit calculations
- **PSID Errors**: Validate PSID parameters
- **Address Conflicts**: Check for address overlaps

#### Option Processing Errors
- **Malformed Options**: Handle corrupted option data
- **Missing Options**: Cope with incomplete option sets
- **Length Mismatches**: Validate option lengths
- **Unknown Options**: Skip unsupported options

### Recovery Procedures

#### Graceful Degradation
1. **Partial Configuration**: Use available parameters
2. **Default Values**: Apply sensible defaults
3. **Error Reporting**: Log configuration issues
4. **Fallback**: Disable MAP-T if critical errors

## Performance Considerations

### Processing Efficiency

#### Option Parsing Optimization
- **Single Pass**: Parse options in one iteration
- **Minimal Copying**: Avoid unnecessary data copying
- **Early Validation**: Validate data during parsing
- **Memory Efficiency**: Use stack allocation where possible

#### Calculation Optimization
- **Bit Operations**: Use efficient bit manipulation
- **Lookup Tables**: Cache common calculations
- **Lazy Evaluation**: Calculate only when needed

### Memory Management

#### Static Data Structure
- **Global Structure**: Single global MAP-T data structure
- **No Dynamic Allocation**: Avoid malloc/free overhead
- **Stack Variables**: Use stack for temporary calculations

## Debugging and Troubleshooting

### Debug Features

#### Comprehensive Logging
```c
#define MAPT_LOG_INFO(format, ...)     \
    CcspTraceInfo(("%s - "format"\n", __FUNCTION__, ##__VA_ARGS__))
#define MAPT_LOG_ERROR(format, ...)    \
    CcspTraceError(("%s - "format"\n", __FUNCTION__, ##__VA_ARGS__))
```

#### Parameter Dumping
- **Option Display**: Log received option data
- **Parameter Values**: Display calculated parameters
- **Validation Results**: Show validation outcomes
- **Configuration Status**: Report setup success/failure

### Common Issues

#### Option Processing Problems
1. **Missing Options**: Check DHCPv6 server configuration
2. **Malformed Data**: Verify option encoding
3. **Length Errors**: Check option length fields
4. **Parsing Failures**: Validate option structure

#### Parameter Calculation Issues
1. **Invalid Prefixes**: Verify prefix configurations
2. **EA Bit Errors**: Check prefix delegation settings
3. **PSID Problems**: Validate port parameter configuration
4. **Address Conflicts**: Check for overlapping ranges

### Diagnostic Tools

#### Option Analysis
- **Hex Dump**: Display raw option data
- **Structure Parsing**: Show TLV structure breakdown
- **Parameter Display**: Show calculated parameters
- **Validation Results**: Display validation outcomes

## Standards Compliance

### RFC 7599 Implementation

#### Mandatory Features
- ✅ **Option 95**: S46_CONT_MAPT container
- ✅ **Option 89**: S46_RULE processing
- ✅ **Option 90**: S46_BR processing  
- ✅ **Option 93**: S46_PORT_PARAMS processing
- ✅ **Parameter Validation**: RFC-compliant validation
- ✅ **Address Calculation**: Standard address mapping

#### Optional Features
- ⚠️ **Option 91**: S46_DMR (not implemented)
- ⚠️ **Multiple Rules**: Single rule support only
- ⚠️ **FMR Support**: Basic FMR flag support

### Interoperability

#### Tested Configurations
- **Standard MAP-T**: Basic MAP-T deployments
- **Provider Networks**: Common ISP configurations
- **Border Relays**: Various BR implementations

## Future Enhancements

### Planned Improvements
1. **Multiple Rules**: Support for multiple MAP rules
2. **DMR Support**: Add Default Mapping Rule support
3. **Enhanced Validation**: More comprehensive parameter checking
4. **Performance Optimization**: Faster option processing
5. **Configuration Caching**: Cache MAP-T parameters

### Advanced Features
1. **Dynamic Updates**: Handle MAP-T parameter updates
2. **Load Balancing**: Multiple Border Relay support
3. **Monitoring**: MAP-T tunnel monitoring and statistics
4. **Quality of Service**: QoS support for MAP-T traffic
5. **Security**: Enhanced security features for MAP-T

### Standards Evolution
- **RFC Updates**: Track RFC 7599 updates and errata
- **New Options**: Support for new DHCP options
- **Protocol Extensions**: Support for MAP-T extensions
- **IPv6 Evolution**: Adapt to IPv6 protocol changes