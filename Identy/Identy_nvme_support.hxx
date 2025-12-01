#pragma once

#ifndef UNC_IDENTY_NVME_SUPPORT_H
#define UNC_IDENTY_NVME_SUPPORT_H

#include "Identy_global.h"
#include "Identy_platform.hxx"

#include <cstdint>

namespace identy::nvme
{
// Ensure structures are tightly packed to match NVMe specification layout
#pragma pack(push, 1)

// NVMe Power State Descriptor (32 bytes per NVMe 1.4 spec, Figure 247)
struct NvmePowerStateDescriptor
{
    std::uint16_t MP; // bit 0:15.    Maximum Power (MP)

    std::uint8_t Reserved0; // bit 16:23

    std::uint8_t MPS : 1;       // bit 24: Max Power Scale (MPS)
    std::uint8_t NOPS : 1;      // bit 25: Non-Operational State (NOPS)
    std::uint8_t Reserved1 : 6; // bit 26:31

    std::uint32_t ENLAT; // bit 32:63.   Entry Latency (ENLAT)
    std::uint32_t EXLAT; // bit 64:95.   Exit Latency (EXLAT)

    std::uint8_t RRT : 5;       // bit 96:100.  Relative Read Throughput (RRT)
    std::uint8_t Reserved2 : 3; // bit 101:103

    std::uint8_t RRL : 5;       // bit 104:108  Relative Read Latency (RRL)
    std::uint8_t Reserved3 : 3; // bit 109:111

    std::uint8_t RWT : 5;       // bit 112:116  Relative Write Throughput (RWT)
    std::uint8_t Reserved4 : 3; // bit 117:119

    std::uint8_t RWL : 5;       // bit 120:124  Relative Write Latency (RWL)
    std::uint8_t Reserved5 : 3; // bit 125:127

    std::uint16_t IDLP; // bit 128:143  Idle Power (IDLP)

    std::uint8_t Reserved6 : 6; // bit 144:149
    std::uint8_t IPS : 2;       // bit 150:151  Idle Power Scale (IPS)

    std::uint8_t Reserved7; // bit 152:159

    std::uint16_t ACTP; // bit 160:175  Active Power (ACTP)

    std::uint8_t APW : 3;       // bit 176:178  Active Power Workload (APW)
    std::uint8_t Reserved8 : 3; // bit 179:181
    std::uint8_t APS : 2;       // bit 182:183  Active Power Scale (APS)

    std::uint8_t Reserved9[9]; // bit 184:255.
};

// NVMe Identify Controller Data Structure (4096 bytes per NVMe 1.4 spec, Figure 275)
struct NvmeIdentifyControllerData
{
    // Controller Capabilities and Features (bytes 0-255)
    std::uint16_t VID;    // PCI Vendor ID
    std::uint16_t SSVID;  // PCI Subsystem Vendor ID
    std::uint8_t SN[20];  // Serial Number (ASCII string)
    std::uint8_t MN[40];  // Model Number (ASCII string)
    std::uint8_t FR[8];   // Firmware Revision (ASCII string)
    std::uint8_t RAB;     // Recommended Arbitration Burst
    std::uint8_t IEEE[3]; // IEEE OUI Identifier

    struct
    {
        std::uint8_t MultiPCIePorts : 1;   // Multiple PCIe ports
        std::uint8_t MultiControllers : 1; // Multiple controllers
        std::uint8_t SRIOV : 1;            // SR-IOV support
        std::uint8_t ANAR : 1;             // ANA Reporting
        std::uint8_t Reserved : 4;
    } CMIC;

    std::uint8_t MDTS;    // Maximum Data Transfer Size
    std::uint16_t CNTLID; // Controller ID
    std::uint32_t VER;    // Version
    std::uint32_t RTD3R;  // RTD3 Resume Latency
    std::uint32_t RTD3E;  // RTD3 Entry Latency

    struct
    {
        std::uint32_t Reserved0 : 8;
        std::uint32_t NamespaceAttributeChanged : 1;
        std::uint32_t FirmwareActivation : 1;
        std::uint32_t Reserved1 : 1;
        std::uint32_t AsymmetricAccessChanged : 1;
        std::uint32_t PredictableLatencyAggregateLogChanged : 1;
        std::uint32_t LbaStatusChanged : 1;
        std::uint32_t EnduranceGroupAggregateLogChanged : 1;
        std::uint32_t Reserved2 : 12;
        std::uint32_t ZoneInformation : 1;
        std::uint32_t Reserved3 : 4;
    } OAES; // Optional Asynchronous Events Supported

    struct
    {
        std::uint32_t HostIdentifier128Bit : 1;
        std::uint32_t NOPSPMode : 1;
        std::uint32_t NVMSets : 1;
        std::uint32_t ReadRecoveryLevels : 1;
        std::uint32_t EnduranceGroups : 1;
        std::uint32_t PredictableLatencyMode : 1;
        std::uint32_t TBKAS : 1;
        std::uint32_t NamespaceGranularity : 1;
        std::uint32_t SQAssociations : 1;
        std::uint32_t UUIDList : 1;
        std::uint32_t Reserved0 : 22;
    } CTRATT; // Controller Attributes

    struct
    {
        std::uint16_t ReadRecoveryLevel0 : 1;
        std::uint16_t ReadRecoveryLevel1 : 1;
        std::uint16_t ReadRecoveryLevel2 : 1;
        std::uint16_t ReadRecoveryLevel3 : 1;
        std::uint16_t ReadRecoveryLevel4 : 1;
        std::uint16_t ReadRecoveryLevel5 : 1;
        std::uint16_t ReadRecoveryLevel6 : 1;
        std::uint16_t ReadRecoveryLevel7 : 1;
        std::uint16_t ReadRecoveryLevel8 : 1;
        std::uint16_t ReadRecoveryLevel9 : 1;
        std::uint16_t ReadRecoveryLevel10 : 1;
        std::uint16_t ReadRecoveryLevel11 : 1;
        std::uint16_t ReadRecoveryLevel12 : 1;
        std::uint16_t ReadRecoveryLevel13 : 1;
        std::uint16_t ReadRecoveryLevel14 : 1;
        std::uint16_t ReadRecoveryLevel15 : 1;
    } RRLS; // Read Recovery Levels Supported

    std::uint8_t Reserved0[9];
    std::uint8_t CNTRLTYPE; // Controller Type
    std::uint8_t FGUID[16]; // FRU Globally Unique Identifier
    std::uint16_t CRDT1;    // Command Retry Delay Time 1
    std::uint16_t CRDT2;    // Command Retry Delay Time 2
    std::uint16_t CRDT3;    // Command Retry Delay Time 3
    std::uint8_t Reserved0_1[106];
    std::uint8_t ReservedForManagement[16];

    // Admin Command Set Attributes (bytes 256-511)
    struct
    {
        std::uint16_t SecurityCommands : 1;
        std::uint16_t FormatNVM : 1;
        std::uint16_t FirmwareCommands : 1;
        std::uint16_t NamespaceCommands : 1;
        std::uint16_t DeviceSelfTest : 1;
        std::uint16_t Directives : 1;
        std::uint16_t NVMeMICommands : 1;
        std::uint16_t VirtualizationMgmt : 1;
        std::uint16_t DoorBellBufferConfig : 1;
        std::uint16_t GetLBAStatus : 1;
        std::uint16_t Reserved : 6;
    } OACS; // Optional Admin Command Support

    std::uint8_t ACL;  // Abort Command Limit
    std::uint8_t AERL; // Asynchronous Event Request Limit

    struct
    {
        std::uint8_t Slot1ReadOnly : 1;
        std::uint8_t SlotCount : 3;
        std::uint8_t ActivationWithoutReset : 1;
        std::uint8_t Reserved : 3;
    } FRMW; // Firmware Updates

    struct
    {
        std::uint8_t SmartPagePerNamespace : 1;
        std::uint8_t CommandEffectsLog : 1;
        std::uint8_t LogPageExtendedData : 1;
        std::uint8_t TelemetrySupport : 1;
        std::uint8_t PersistentEventLog : 1;
        std::uint8_t Reserved0 : 1;
        std::uint8_t TelemetryDataArea4 : 1;
        std::uint8_t Reserved1 : 1;
    } LPA; // Log Page Attributes

    std::uint8_t ELPE; // Error Log Page Entries
    std::uint8_t NPSS; // Number of Power States Support

    struct
    {
        std::uint8_t CommandFormatInSpec : 1;
        std::uint8_t Reserved : 7;
    } AVSCC; // Admin Vendor Specific Command Configuration

    struct
    {
        std::uint8_t Supported : 1;
        std::uint8_t Reserved : 7;
    } APSTA; // Autonomous Power State Transition Attributes

    std::uint16_t WCTEMP;     // Warning Composite Temperature Threshold
    std::uint16_t CCTEMP;     // Critical Composite Temperature Threshold
    std::uint16_t MTFA;       // Maximum Time for Firmware Activation
    std::uint32_t HMPRE;      // Host Memory Buffer Preferred Size
    std::uint32_t HMMIN;      // Host Memory Buffer Minimum Size
    std::uint8_t TNVMCAP[16]; // Total NVM Capacity (128-bit)
    std::uint8_t UNVMCAP[16]; // Unallocated NVM Capacity (128-bit)

    struct
    {
        std::uint32_t RPMBUnitCount : 3;
        std::uint32_t AuthenticationMethod : 3;
        std::uint32_t Reserved0 : 10;
        std::uint32_t TotalSize : 8;
        std::uint32_t AccessSize : 8;
    } RPMBS; // Replay Protected Memory Block Support

    std::uint16_t EDSTT; // Extended Device Self-test Time
    std::uint8_t DSTO;   // Device Self-test Options
    std::uint8_t FWUG;   // Firmware Update Granularity
    std::uint16_t KAS;   // Keep Alive Support

    struct
    {
        std::uint16_t Supported : 1;
        std::uint16_t Reserved : 15;
    } HCTMA; // Host Controlled Thermal Management Attributes

    std::uint16_t MNTMT; // Minimum Thermal Management Temperature
    std::uint16_t MXTMT; // Maximum Thermal Management Temperature

    struct
    {
        std::uint32_t CryptoErase : 1;
        std::uint32_t BlockErase : 1;
        std::uint32_t Overwrite : 1;
        std::uint32_t Reserved : 26;
        std::uint32_t NDI : 1;     // No-Deallocate Inhibited
        std::uint32_t NODMMAS : 2; // No-Deallocate Modifies Media After Sanitize
    } SANICAP;                     // Sanitize Capabilities

    std::uint32_t HMMINDS;   // Host Memory Buffer Minimum Descriptor Entry Size
    std::uint16_t HMMAXD;    // Host Memory Maximum Descriptors Entries
    std::uint16_t NSETIDMAX; // NVM Set Identifier Maximum
    std::uint16_t ENDGIDMAX; // Endurance Group Identifier Maximum
    std::uint8_t ANATT;      // ANA Transition Time

    struct
    {
        std::uint8_t OptimizedState : 1;
        std::uint8_t NonOptimizedState : 1;
        std::uint8_t InaccessibleState : 1;
        std::uint8_t PersistentLossState : 1;
        std::uint8_t ChangeState : 1;
        std::uint8_t Reserved : 1;
        std::uint8_t StaticANAGRPID : 1;
        std::uint8_t SupportNonZeroANAGRPID : 1;
    } ANACAP; // Asymmetric Namespace Access Capabilities

    std::uint32_t ANAGRPMAX; // ANA Group Identifier Maximum
    std::uint32_t NANAGRPID; // Number of ANA Group Identifiers
    std::uint32_t PELS;      // Persistent Event Log Size
    std::uint8_t Reserved1[156];

    // NVM Command Set Attributes (bytes 512-703)
    struct
    {
        std::uint8_t RequiredEntrySize : 4;
        std::uint8_t MaxEntrySize : 4;
    } SQES; // Submission Queue Entry Size

    struct
    {
        std::uint8_t RequiredEntrySize : 4;
        std::uint8_t MaxEntrySize : 4;
    } CQES; // Completion Queue Entry Size

    std::uint16_t MAXCMD; // Maximum Outstanding Commands
    std::uint32_t NN;     // Number of Namespaces

    struct
    {
        std::uint16_t Compare : 1;
        std::uint16_t WriteUncorrectable : 1;
        std::uint16_t DatasetManagement : 1;
        std::uint16_t WriteZeroes : 1;
        std::uint16_t FeatureField : 1;
        std::uint16_t Reservations : 1;
        std::uint16_t Timestamp : 1;
        std::uint16_t Verify : 1;
        std::uint16_t Reserved : 8;
    } ONCS; // Optional NVM Command Support

    struct
    {
        std::uint16_t CompareAndWrite : 1;
        std::uint16_t Reserved : 15;
    } FUSES; // Fused Operation Support

    struct
    {
        std::uint8_t FormatApplyToAll : 1;
        std::uint8_t SecureEraseApplyToAll : 1;
        std::uint8_t CryptographicEraseSupported : 1;
        std::uint8_t FormatSupportNSIDAllF : 1;
        std::uint8_t Reserved : 4;
    } FNA; // Format NVM Attributes

    struct
    {
        std::uint8_t Present : 1;
        std::uint8_t FlushBehavior : 2;
        std::uint8_t Reserved : 5;
    } VWC; // Volatile Write Cache

    std::uint16_t AWUN;  // Atomic Write Unit Normal
    std::uint16_t AWUPF; // Atomic Write Unit Power Fail

    struct
    {
        std::uint8_t CommandFormatInSpec : 1;
        std::uint8_t Reserved : 7;
    } NVSCC; // NVM Vendor Specific Command Configuration

    struct
    {
        std::uint8_t WriteProtect : 1;
        std::uint8_t UntilPowerCycle : 1;
        std::uint8_t Permanent : 1;
        std::uint8_t Reserved : 5;
    } NWPC; // Namespace Write Protection Capabilities

    std::uint16_t ACWU; // Atomic Compare & Write Unit
    std::uint8_t Reserved4[2];

    struct
    {
        std::uint32_t SGLSupported : 2;
        std::uint32_t KeyedSGLData : 1;
        std::uint32_t Reserved0 : 13;
        std::uint32_t BitBucketDescrSupported : 1;
        std::uint32_t ByteAlignedContiguousPhysicalBuffer : 1;
        std::uint32_t SGLLengthLargerThanDataLength : 1;
        std::uint32_t MPTRSGLDescriptor : 1;
        std::uint32_t AddressFieldSGLDataBlock : 1;
        std::uint32_t TransportSGLData : 1;
        std::uint32_t Reserved1 : 10;
    } SGLS; // SGL Support

    std::uint32_t MNAN; // Maximum Number of Allowed Namespaces
    std::uint8_t Reserved6[224];

    // I/O Command Set Attributes (bytes 704-2047)
    std::uint8_t SUBNQN[256]; // NVM Subsystem NVMe Qualified Name
    std::uint8_t Reserved7[768];
    std::uint8_t Reserved8[256]; // NVMe over Fabrics specific

    // Power State Descriptors (bytes 2048-3071)
    NvmePowerStateDescriptor PDS[32];

    // Vendor Specific (bytes 3072-4095)
    std::uint8_t VS[1024];
};

#pragma pack(pop)

// Static assertions to verify structure sizes match NVMe specification
static_assert(sizeof(NvmePowerStateDescriptor) == 32, "NvmePowerStateDescriptor must be 32 bytes per NVMe spec");
static_assert(sizeof(NvmeIdentifyControllerData) == 4096, "NvmeIdentifyControllerData must be 4096 bytes per NVMe spec");

#ifdef IDENTY_WIN32
enum StorageProtocolNvmeDataType {
    NVMeDataTypeUnknown = 0,
    NVMeDataTypeIdentify = 1,
    NVMeDataTypeLogPage = 2,
    NVMeDataTypeFeature = 3
};

struct StorageProtocolSpecificData
{
    STORAGE_PROTOCOL_TYPE ProtocolType;
    dword DataType;
    dword ProtocolDataRequestValue;
    dword ProtocolDataRequestSubValue;
    dword ProtocolDataOffset;
    dword ProtocolDataLength;
    dword FixedProtocolReturnData;
    dword ProtocolDataRequestSubValue2;
    dword Reserved[2];
};

struct StorageProtocolDataDescriptor
{
    dword Version;
    dword Size;
    StorageProtocolSpecificData ProtocolSpecificData;
};

constexpr dword CNS_CONTROLLER = 1;

#endif

} // namespace identy::nvme

#endif
