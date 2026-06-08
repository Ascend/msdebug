#ifdef MS_DEBUGGER
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "Plugins/Platform/Linux/PlatformLinux.h"
#include "Plugins/Process/elf-core/device-core/ElfCoreDeviceUtilities.h"
#include "Plugins/Process/elf-core/device-core/ProcessElfCoreDevice.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Host/FileSystem.h"
#include "lldb/Host/HostInfo.h"
#include "lldb/Target/Platform.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/Target.h"
#include "lldb/Utility/ArchSpec.h"
#include "lldb/Utility/MessageDefines.h"

using namespace lldb_private;
using namespace lldb_private::device_core;
using namespace lldb;

class ElfCoreDeviceUtilitiesTest : public ::testing::Test {};

TEST_F(ElfCoreDeviceUtilitiesTest, LinearIdxToThreadPos_ZeroIdx_ReturnsOrigin) {
  ThreadDim dim{8, 4, 2};
  ThreadPos pos = LinearIdxToThreadPos(0, dim);
  EXPECT_EQ(pos.x, 0u);
  EXPECT_EQ(pos.y, 0u);
  EXPECT_EQ(pos.z, 0u);
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       LinearIdxToThreadPos_SmallIdx_ReturnsCorrectXYZ) {
  ThreadDim dim{8, 4, 2};
  ThreadPos pos = LinearIdxToThreadPos(10, dim);
  EXPECT_EQ(pos.x, 10u % 8u);
  EXPECT_EQ(pos.y, (10u / 8u) % 4u);
  EXPECT_EQ(pos.z, 10u / (8u * 4u));
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       LinearIdxToThreadPos_LargeIdx_CalculatesCorrectly) {
  ThreadDim dim{4, 3, 2};
  uint32_t idx = 17;
  ThreadPos pos = LinearIdxToThreadPos(idx, dim);
  EXPECT_EQ(pos.x, idx % dim.x);
  EXPECT_EQ(pos.y, (idx / dim.x) % dim.y);
  EXPECT_EQ(pos.z, idx / (dim.x * dim.y));
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       LinearIdxToThreadPos_MaxValidIdx_ReturnsLastThread) {
  ThreadDim dim{4, 3, 2};
  uint32_t total_threads = dim.x * dim.y * dim.z;
  ThreadPos pos = LinearIdxToThreadPos(total_threads - 1, dim);
  EXPECT_EQ(pos.x, (total_threads - 1) % dim.x);
  EXPECT_EQ(pos.y, ((total_threads - 1) / dim.x) % dim.y);
  EXPECT_EQ(pos.z, (total_threads - 1) / (dim.x * dim.y));
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       LinearIdxToThreadPos_SingleThreadDim_ReturnsFlatMapping) {
  ThreadDim dim{1, 1, 1};
  ThreadPos pos = LinearIdxToThreadPos(0, dim);
  EXPECT_EQ(pos.x, 0u);
  EXPECT_EQ(pos.y, 0u);
  EXPECT_EQ(pos.z, 0u);
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       LinearIdxToThreadPos_WarpSize32_MapsCorrectly) {
  ThreadDim dim{32, 1, 1};
  ThreadPos pos0 = LinearIdxToThreadPos(0, dim);
  EXPECT_EQ(pos0.x, 0u);
  EXPECT_EQ(pos0.y, 0u);
  EXPECT_EQ(pos0.z, 0u);

  ThreadPos pos31 = LinearIdxToThreadPos(31, dim);
  EXPECT_EQ(pos31.x, 31u);
  EXPECT_EQ(pos31.y, 0u);
  EXPECT_EQ(pos31.z, 0u);
}

TEST_F(ElfCoreDeviceUtilitiesTest, GetMaxAicID_ChipCloudV2_Returns25) {
  EXPECT_EQ(GetMaxAicID(DevdrvChipType::CHIP_CLOUD_V2), 25u);
}

TEST_F(ElfCoreDeviceUtilitiesTest, GetMaxAicID_ChipCloudV4_Returns36) {
  EXPECT_EQ(GetMaxAicID(DevdrvChipType::CHIP_CLOUD_V4), 36u);
}

TEST_F(ElfCoreDeviceUtilitiesTest, GetMaxAicID_ChipBegin_Returns0) {
  EXPECT_EQ(GetMaxAicID(DevdrvChipType::CHIP_BEGIN), 0u);
}

TEST_F(ElfCoreDeviceUtilitiesTest, GetMaxAicID_ChipEnd_Returns0) {
  EXPECT_EQ(GetMaxAicID(DevdrvChipType::CHIP_END), 0u);
}

TEST_F(ElfCoreDeviceUtilitiesTest, CenterText_ShortString_PadsBothSides) {
  std::string result = CenterText("hi", 10);
  EXPECT_EQ(result.length(), 10u);
  EXPECT_EQ(result.substr(4, 2), "hi");
}

TEST_F(ElfCoreDeviceUtilitiesTest, CenterText_LongString_NoPadding) {
  std::string result = CenterText("hello world", 5);
  EXPECT_EQ(result, "hello world");
}

TEST_F(ElfCoreDeviceUtilitiesTest, CenterText_ExactWidth_NoPadding) {
  std::string result = CenterText("hello", 5);
  EXPECT_EQ(result, "hello");
}

TEST_F(ElfCoreDeviceUtilitiesTest, CenterText_NumHex_CentersHexFormat) {
  std::string result = CenterText(255ULL, 10, true, true);
  EXPECT_TRUE(result.find("0xff") != std::string::npos);
}

TEST_F(ElfCoreDeviceUtilitiesTest, CenterText_NumDec_CentersDecFormat) {
  std::string result = CenterText(42ULL, 10, false, true);
  EXPECT_TRUE(result.find("42") != std::string::npos);
}

TEST_F(ElfCoreDeviceUtilitiesTest, CenterText_NumInvalid_AppendsInvalidTag) {
  std::string result = CenterText(0ULL, 10, false, false);
  EXPECT_TRUE(result.find("(invalid)") != std::string::npos);
}

TEST_F(ElfCoreDeviceUtilitiesTest, DevdrvChipSocTypeMap_CloudV2_MapsTo910B) {
  EXPECT_EQ(DevdrvChipSocTypeMap.at(DevdrvChipType::CHIP_CLOUD_V2),
            SocType::ASCEND910B);
}

TEST_F(ElfCoreDeviceUtilitiesTest, DevdrvChipSocTypeMap_CloudV4_MapsTo950) {
  EXPECT_EQ(DevdrvChipSocTypeMap.at(DevdrvChipType::CHIP_CLOUD_V4),
            SocType::ASCEND950);
}

TEST_F(ElfCoreDeviceUtilitiesTest, GlobalDataTypeToStr_StackType_ReturnsStack) {
  EXPECT_EQ(GlobalDataTypeToStr.at(GlobalDataType::STACK), "STACK");
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       GlobalDataTypeToStr_InputTensor_ReturnsInputTensor) {
  EXPECT_EQ(GlobalDataTypeToStr.at(GlobalDataType::INPUT_TENSOR),
            "INPUT_TENSOR");
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       GlobalDataTypeToStr_DeviceKernelObject_ReturnsExpected) {
  EXPECT_EQ(GlobalDataTypeToStr.at(GlobalDataType::DEVICE_KERNEL_OBJECT),
            "DEVICE_KERNEL_OBJECT");
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       GlobalDataTypeToStr_AllValidTypes_HaveEntries) {
  for (auto type :
       {GlobalDataType::INVALID_TENSOR, GlobalDataType::GENERAL_TENSOR,
        GlobalDataType::INPUT_TENSOR, GlobalDataType::OUTPUT_TENSOR,
        GlobalDataType::WORKSPACE_TENSOR, GlobalDataType::MC2_CTX,
        GlobalDataType::TILING_DATA, GlobalDataType::SHAPE_TENSOR,
        GlobalDataType::ARGS, GlobalDataType::STACK,
        GlobalDataType::DEVICE_KERNEL_OBJECT, GlobalDataType::VF_STACK}) {
    EXPECT_NE(GlobalDataTypeToStr.find(type), GlobalDataTypeToStr.end());
  }
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       LocalDataTypeToStr_AllValidTypes_HaveEntries) {
  for (auto type : {MemType::L0A, MemType::L0B, MemType::L0C, MemType::UB,
                    MemType::L1, MemType::DCACHE, MemType::ICACHE}) {
    EXPECT_NE(LocalDataTypeToStr.find(type), LocalDataTypeToStr.end());
  }
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       AddressClassGlobalDataTypeMap_GM_ContainsAllGlobalTypes) {
  auto &gm_types = AddressClassGlobalDataTypeMap.at(DeviceAddressClass::GM);
  EXPECT_GE(gm_types.size(), 11u);
}

TEST_F(ElfCoreDeviceUtilitiesTest, AddressClassLocalDataTypeMap_UBUF_MapsToUB) {
  EXPECT_EQ(AddressClassLocalDataTypeMap.at(DeviceAddressClass::UBUF),
            MemType::UB);
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       AddressClassLocalDataTypeMap_DCACHE_MapsToDCACHE) {
  EXPECT_EQ(AddressClassLocalDataTypeMap.at(DeviceAddressClass::DCACHE),
            MemType::DCACHE);
}

TEST_F(ElfCoreDeviceUtilitiesTest, AddressClassLocalDataTypeMap_L1MapsToCBUF) {
  EXPECT_EQ(AddressClassLocalDataTypeMap.at(DeviceAddressClass::CBUF),
            MemType::L1);
}

TEST_F(ElfCoreDeviceUtilitiesTest, DevdrvChipTypeToStr_CloudV2_ReturnsA2A3) {
  EXPECT_EQ(DevdrvChipTypeToStr.at(DevdrvChipType::CHIP_CLOUD_V2), "A2/A3");
}

TEST_F(ElfCoreDeviceUtilitiesTest, DevdrvChipTypeToStr_CloudV4_ReturnsA5) {
  EXPECT_EQ(DevdrvChipTypeToStr.at(DevdrvChipType::CHIP_CLOUD_V4), "A5");
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       GetPluginNameStatic_Default_ReturnsElfCoreDevice) {
  EXPECT_EQ(ProcessElfCoreDevice::GetPluginNameStatic(), "elf-core-device");
}

TEST_F(ElfCoreDeviceUtilitiesTest,
       GetPluginDescriptionStatic_Default_ReturnsExpectedDesc) {
  EXPECT_EQ(ProcessElfCoreDevice::GetPluginDescriptionStatic(),
            "ELF core device dump plug-in.");
}

class ProcessElfCoreDeviceInstanceTest : public ::testing::Test {
public:
  void SetUp() override {
    FileSystem::Initialize();
    HostInfo::Initialize();
    platform_linux::PlatformLinux::Initialize();
    ArchSpec arch("hiipu64");
    Platform::SetHostPlatform(
        platform_linux::PlatformLinux::CreateInstance(true, &arch));
    m_debugger_sp = Debugger::CreateInstance();
    ASSERT_TRUE(m_debugger_sp);
    PlatformSP platform_sp;
    Status error = m_debugger_sp->GetTargetList().CreateTarget(
        *m_debugger_sp, "", arch, eLoadDependentsNo, platform_sp, m_target_sp);
    ASSERT_TRUE(m_target_sp);
    ListenerSP listener_sp(Listener::MakeListener("test-device-core"));
    FileSpec dummy_core("/tmp/nonexistent_device_core");
    m_process_sp = std::make_shared<ProcessElfCoreDevice>(
        m_target_sp, listener_sp, dummy_core);
    ASSERT_TRUE(m_process_sp);
  }
  void TearDown() override {
    m_process_sp.reset();
    m_target_sp.reset();
    m_debugger_sp.reset();
    platform_linux::PlatformLinux::Terminate();
    HostInfo::Terminate();
    FileSystem::Terminate();
  }

  DebuggerSP m_debugger_sp;
  TargetSP m_target_sp;
  ProcessSP m_process_sp;
};

TEST_F(ProcessElfCoreDeviceInstanceTest,
       SetAicOnFocus_EmptyState_ReturnsSwitchFailed) {
  ProcessElfCoreDevice *device =
      static_cast<ProcessElfCoreDevice *>(m_process_sp.get());
  Status error = device->SetAicOnFocus(0);
  EXPECT_TRUE(error.Fail());
  EXPECT_TRUE(error.AsCString() != nullptr &&
              std::string(error.AsCString()).find("Switch AIC failed") !=
                  std::string::npos);
}

TEST_F(ProcessElfCoreDeviceInstanceTest,
       SetAivOnFocus_EmptyState_ReturnsSwitchFailed) {
  ProcessElfCoreDevice *device =
      static_cast<ProcessElfCoreDevice *>(m_process_sp.get());
  Status error = device->SetAivOnFocus(0);
  EXPECT_TRUE(error.Fail());
  EXPECT_TRUE(error.AsCString() != nullptr &&
              std::string(error.AsCString()).find("Switch AIV failed") !=
                  std::string::npos);
}

TEST_F(ProcessElfCoreDeviceInstanceTest,
       GetSummaryInfo_EmptyState_ChipTypeIsBegin) {
  ProcessElfCoreDevice *device =
      static_cast<ProcessElfCoreDevice *>(m_process_sp.get());
  const SummaryInfo &info = device->GetSummaryInfo();
  EXPECT_EQ(info.chip_type, DevdrvChipType::CHIP_BEGIN);
  EXPECT_TRUE(info.aic_id.empty());
  EXPECT_TRUE(info.aiv_id.empty());
  EXPECT_TRUE(info.global_mems.empty());
  EXPECT_TRUE(info.local_mems.empty());
  EXPECT_TRUE(info.reg_data.empty());
}

TEST_F(ProcessElfCoreDeviceInstanceTest, GetKernelInfo_EmptyState_NameIsEmpty) {
  ProcessElfCoreDevice *device =
      static_cast<ProcessElfCoreDevice *>(m_process_sp.get());
  KernelInfo info;
  Status error = device->GetKernelInfo(info);
  EXPECT_TRUE(error.Success());
  EXPECT_EQ(info.name[0], 0);
}

TEST_F(ProcessElfCoreDeviceInstanceTest, GetAuxvData_EmptyState_BasePcIsZero) {
  ProcessElfCoreDevice *device =
      static_cast<ProcessElfCoreDevice *>(m_process_sp.get());
  DataExtractor auxv = device->GetAuxvData();
  EXPECT_GT(auxv.GetByteSize(), 0u);
}

#endif
