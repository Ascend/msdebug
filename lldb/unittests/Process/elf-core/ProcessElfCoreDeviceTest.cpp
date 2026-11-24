#ifdef MS_DEBUGGER
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "Plugins/ObjectFile/ELF/ObjectFileELF.h"
#include "Plugins/Platform/Linux/PlatformLinux.h"
#include "Plugins/Process/elf-core/device-core/ElfCoreDeviceUtilities.h"
#include "Plugins/Process/elf-core/device-core/ProcessElfCoreDevice.h"
#include "TestingSupport/TestUtilities.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/Module.h"
#include "lldb/Host/FileSystem.h"
#include "lldb/Host/HostInfo.h"
#include "lldb/Target/Platform.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/Target.h"
#include "lldb/Utility/ArchSpec.h"
#include "lldb/Utility/MessageDefines.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Testing/Support/SupportHelpers.h"

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

TEST_F(ProcessElfCoreDeviceInstanceTest,
       SetDeviceStopInfoCached_AivSimt_RemembersThreadFocus) {
  Process *process = m_process_sp.get();
  DeviceStopInfo info;
  info.core_type = CoreType::AIV;
  info.core_id = 3;
  info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  info.thread_pos = {1, 2, 3};
  process->SetDeviceStopInfoCached(info);

  ThreadPos pos;
  EXPECT_TRUE(process->GetAivThreadFocus(3, pos));
  EXPECT_EQ(pos.x, 1u);
  EXPECT_EQ(pos.y, 2u);
  EXPECT_EQ(pos.z, 3u);
}

TEST_F(ProcessElfCoreDeviceInstanceTest,
       SetDeviceStopInfoCached_NonSimt_DoesNotRemember) {
  Process *process = m_process_sp.get();
  DeviceStopInfo info;
  info.core_type = CoreType::AIV;
  info.core_id = 3;
  info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMD;
  info.thread_pos = {1, 2, 3};
  process->SetDeviceStopInfoCached(info);

  ThreadPos pos;
  EXPECT_FALSE(process->GetAivThreadFocus(3, pos));
}

TEST_F(ProcessElfCoreDeviceInstanceTest,
       SetDeviceStopInfoCached_AicCore_DoesNotRemember) {
  Process *process = m_process_sp.get();
  DeviceStopInfo info;
  info.core_type = CoreType::AIC;
  info.core_id = 3;
  info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  info.thread_pos = {1, 2, 3};
  process->SetDeviceStopInfoCached(info);

  ThreadPos pos;
  EXPECT_FALSE(process->GetAivThreadFocus(3, pos));
}

TEST_F(ProcessElfCoreDeviceInstanceTest,
       GetAivThreadFocus_NotRemembered_ReturnsFalse) {
  Process *process = m_process_sp.get();
  ThreadPos pos;
  EXPECT_FALSE(process->GetAivThreadFocus(99, pos));
}

TEST_F(ProcessElfCoreDeviceInstanceTest,
       SetDeviceStopInfoCached_SameCoreAgain_OverwritesFocus) {
  Process *process = m_process_sp.get();
  DeviceStopInfo info;
  info.core_type = CoreType::AIV;
  info.core_id = 3;
  info.pos_type = InterruptPosType::VEC_INTERRUPT_SIMT;
  info.thread_pos = {1, 2, 3};
  process->SetDeviceStopInfoCached(info);

  info.thread_pos = {4, 5, 6};
  process->SetDeviceStopInfoCached(info);

  ThreadPos pos;
  EXPECT_TRUE(process->GetAivThreadFocus(3, pos));
  EXPECT_EQ(pos.x, 4u);
  EXPECT_EQ(pos.y, 5u);
  EXPECT_EQ(pos.z, 6u);
}

namespace lldb_private {
class ProcessElfCoreDeviceMemoryTest
    : public ::ProcessElfCoreDeviceInstanceTest {
public:
  void SetUp() override {
    ::ProcessElfCoreDeviceInstanceTest::SetUp();
    ObjectFileELF::Initialize();
    auto file = TestFile::fromYaml(R"(
--- !ELF
FileHeader:
  Class: ELFCLASS64
  Data: ELFDATA2LSB
  Type: ET_CORE
  Machine: EM_X86_64
Sections:
  - Name: .ascend.global
    Type: SHT_PROGBITS
    Content: 01020304
...
)");
    if (!file) {
      ADD_FAILURE() << llvm::toString(file.takeError());
      return;
    }
    m_file = std::make_unique<TestFile>(std::move(*file));
    m_module = std::make_shared<Module>(m_file->moduleSpec());
    ASSERT_NE(nullptr, m_module->GetSectionList());
    m_section = m_module->GetSectionList()->FindSectionByName(
        ConstString(".ascend.global"));
    ASSERT_NE(nullptr, m_section);
    Device()->m_section_list.AddSection(m_section);
  }

  void TearDown() override {
    Device()->m_section_list.Clear();
    m_section.reset();
    m_module.reset();
    m_file.reset();
    ObjectFileELF::Terminate();
    ::ProcessElfCoreDeviceInstanceTest::TearDown();
  }

  ProcessElfCoreDevice *Device() {
    return static_cast<ProcessElfCoreDevice *>(m_process_sp.get());
  }

  Status Insert(uint64_t size, uint64_t addr = 0x1000, uint32_t section = 1) {
    GlobalMemInfo info{};
    info.addr = addr;
    info.size = size;
    info.section_index = section;
    info.type = GlobalDataType::INPUT_TENSOR;
    if (section > 1)
      Device()->m_section_list.AddSection(m_section);
    return Device()->GetGlobalMemData(info, true);
  }

  Status ParseGlobals(llvm::ArrayRef<GlobalMemInfo> infos) {
    Device()->m_summary_info.global_mems.clear();
    Device()->m_summary_info.global_section_to_memory.clear();
    if (Device()->m_section_list.GetSize() < 2)
      Device()->m_section_list.AddSection(m_section);
    llvm::StringRef bytes(reinterpret_cast<const char *>(infos.data()),
                          infos.size() * sizeof(GlobalMemInfo));
    llvm::unittest::TempFile core("msdebug-global-aux", "core", bytes, true);
    Device()->m_core_file = FileSpec(core.path());
    auto file = TestFile::fromYaml(
        "--- !ELF\nFileHeader:\n  Class: ELFCLASS64\n  Data: ELFDATA2LSB\n"
        "  Type: ET_CORE\n  Machine: EM_X86_64\nSections:\n"
        "  - Name: .ascend.auxinfo.global\n    Type: SHT_PROGBITS\n"
        "    Content: " +
        llvm::toHex(bytes) + "\n...\n");
    if (!file) {
      Status error;
      error.SetErrorString(llvm::toString(file.takeError()));
      return error;
    }
    TestFile aux_file(std::move(*file));
    auto aux_module = std::make_shared<Module>(aux_file.moduleSpec());
    auto *object_file = aux_module->GetObjectFile();
    if (!object_file) {
      Status error;
      error.SetErrorString("failed to create auxinfo test ELF object");
      return error;
    }
    auto section = object_file->GetSectionList()->FindSectionByName(
        ConstString(".ascend.auxinfo.global"));
    return Device()->ParseGlobalAuxInfo(section,
                                        ConstString(".ascend.auxinfo.global"));
  }

  size_t Read(addr_t addr, size_t size, void *buf, Status &error) {
    return Device()->ReadGlobalMemory(DeviceAddressClass::GM, addr, size, buf,
                                      error);
  }

private:
  std::unique_ptr<TestFile> m_file;
  ModuleSP m_module;
  SectionSP m_section;
};

TEST_F(ProcessElfCoreDeviceMemoryTest,
       RejectsSizeLargerThanSectionWithoutCaching) {
  Status error = Insert(5);
  EXPECT_TRUE(error.Fail());
  EXPECT_TRUE(Device()->GetSummaryInfo().global_mems.empty());
  EXPECT_TRUE(Device()->GetSummaryInfo().global_section_to_memory.empty());
}

TEST_F(ProcessElfCoreDeviceMemoryTest, RejectsOversizedInfoBeforeAllocating) {
  Status error = Insert((120ULL << 30) + 1);
  EXPECT_TRUE(error.Fail());
  EXPECT_TRUE(Device()->GetSummaryInfo().global_mems.empty());
  EXPECT_TRUE(Device()->GetSummaryInfo().global_section_to_memory.empty());
}

TEST_F(ProcessElfCoreDeviceMemoryTest, ReadsPayloadAndRequestedAddressRange) {
  Status error = Insert(4);
  ASSERT_TRUE(error.Success());
  const auto &info = Device()->GetSummaryInfo();
  const auto &memory = info.global_mems.at(GlobalDataType::INPUT_TENSOR).at(0);
  EXPECT_EQ(memory.data, (std::vector<uint8_t>{1, 2, 3, 4}));
  EXPECT_EQ(info.global_section_to_memory.at(1),
            (std::make_pair(GlobalDataType::INPUT_TENSOR, size_t{0})));
  uint8_t bytes[2]{};
  error.Clear();
  EXPECT_EQ(Read(0x1001, 2, bytes, error), 2u);
  EXPECT_TRUE(error.Success());
  EXPECT_EQ(bytes[0], 2);
  EXPECT_EQ(bytes[1], 3);
}

TEST_F(ProcessElfCoreDeviceMemoryTest, PreservesZeroLengthEntryWithoutPayload) {
  Status error = Insert(0);
  ASSERT_TRUE(error.Success());
  const auto &info = Device()->GetSummaryInfo();
  ASSERT_EQ(info.global_mems.at(GlobalDataType::INPUT_TENSOR).size(), 1u);
  EXPECT_TRUE(
      info.global_mems.at(GlobalDataType::INPUT_TENSOR)[0].data.empty());
  EXPECT_EQ(info.global_section_to_memory.at(1),
            (std::make_pair(GlobalDataType::INPUT_TENSOR, size_t{0})));
}

TEST_F(ProcessElfCoreDeviceMemoryTest,
       ParseGlobalAuxInfoKeepsBadEntryAndContinues) {
  GlobalMemInfo bad{};
  bad.addr = 0x1000;
  bad.size = 5;
  bad.section_index = 1;
  bad.type = GlobalDataType::INPUT_TENSOR;
  GlobalMemInfo good{};
  good.addr = 0x2000;
  good.size = 4;
  good.section_index = 2;
  good.type = GlobalDataType::INPUT_TENSOR;
  for (bool bad_first : {true, false}) {
    GlobalMemInfo infos[] = {bad_first ? bad : good, bad_first ? good : bad};
    ASSERT_TRUE(ParseGlobals(infos).Success());
    const auto &summary = Device()->GetSummaryInfo();
    const auto &memories = summary.global_mems.at(GlobalDataType::INPUT_TENSOR);
    ASSERT_EQ(memories.size(), 2u);
    size_t bad_index = bad_first ? 0 : 1;
    size_t good_index = 1 - bad_index;
    EXPECT_FALSE(memories[bad_index].valid);
    EXPECT_TRUE(memories[bad_index].data.empty());
    EXPECT_EQ(summary.global_section_to_memory.at(1),
              (std::make_pair(GlobalDataType::INPUT_TENSOR, bad_index)));
    EXPECT_EQ(summary.global_section_to_memory.at(2),
              (std::make_pair(GlobalDataType::INPUT_TENSOR, good_index)));
    EXPECT_EQ(memories[good_index].data, (std::vector<uint8_t>{1, 2, 3, 4}));
    uint8_t bytes[4] = {9, 9, 9, 9};
    Status error;
    EXPECT_EQ(Read(0x1000, 4, bytes, error), 0u);
    EXPECT_TRUE(error.Fail());
    EXPECT_EQ(bytes[0], 9);
    error.Clear();
    EXPECT_EQ(Read(0x2000, 4, bytes, error), 4u);
    EXPECT_TRUE(error.Success());
    EXPECT_EQ(std::vector<uint8_t>(bytes, bytes + 4),
              (std::vector<uint8_t>{1, 2, 3, 4}));
  }
}

TEST_F(ProcessElfCoreDeviceMemoryTest, ParseGlobalAuxInfoAllBadStillSucceeds) {
  GlobalMemInfo bad{};
  bad.addr = 0x1000;
  bad.size = 5;
  bad.section_index = 1;
  bad.type = GlobalDataType::INPUT_TENSOR;
  ASSERT_TRUE(ParseGlobals({bad}).Success());
  const auto &summary = Device()->GetSummaryInfo();
  ASSERT_FALSE(summary.global_mems.empty());
  const auto &memory = summary.global_mems.at(GlobalDataType::INPUT_TENSOR)[0];
  EXPECT_FALSE(memory.valid);
  EXPECT_TRUE(memory.data.empty());
  uint8_t byte = 9;
  Status error;
  EXPECT_EQ(Read(0x1000, 1, &byte, error), 0u);
  EXPECT_TRUE(error.Fail());
  EXPECT_EQ(byte, 9);
}

TEST_F(ProcessElfCoreDeviceMemoryTest, ParseGlobalAuxInfoUnknownTypeFails) {
  GlobalMemInfo invalid{};
  invalid.type = static_cast<GlobalDataType>(999);
  EXPECT_TRUE(ParseGlobals({invalid}).Fail());
}

TEST_F(ProcessElfCoreDeviceMemoryTest,
       SectionIndexesSurvivePayloadVectorGrowth) {
  ASSERT_TRUE(Insert(4, 0x1000, 1).Success());
  for (uint32_t index = 2; index <= 32; ++index)
    ASSERT_TRUE(Insert(4, 0x1000 + (index - 1) * 4, index).Success());
  const auto &info = Device()->GetSummaryInfo();
  ASSERT_EQ(info.global_mems.at(GlobalDataType::INPUT_TENSOR).size(), 32u);
  for (uint32_t index = 1; index <= 32; ++index) {
    const auto &reference = info.global_section_to_memory.at(index);
    const auto &memory =
        info.global_mems.at(reference.first).at(reference.second);
    EXPECT_EQ(memory.global_mem_info.addr, 0x1000 + (index - 1) * 4);
    EXPECT_EQ(memory.data, (std::vector<uint8_t>{1, 2, 3, 4}));
  }
}
} // namespace lldb_private

#endif
