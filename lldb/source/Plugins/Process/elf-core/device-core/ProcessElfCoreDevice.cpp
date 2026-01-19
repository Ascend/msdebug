/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
*/

#ifdef MS_DEBUGGER

#include "ProcessElfCoreDevice.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend910B.h"
#include "Plugins/Process/elf-core/device-core/RegisterContextPOSIXCore_ascend.h"
#include "Plugins/Process/Utility/AuxVector.h"
#include "Plugins/Process/elf-core/ThreadElfCore.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContextUnwind.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/DataBufferHeap.h"
#include "Plugins/ObjectFile/ELF/ObjectFileELF.h"

using namespace lldb_private;
using namespace lldb;
using namespace std;
using namespace device_core;

LLDB_PLUGIN_DEFINE(ProcessElfCoreDevice)
constexpr uint64_t MAX_LOCAL_MEM_SIZE = 10ULL * 1024 * 1024 * 1024; // 10G local memory
constexpr uint64_t MAX_GM_SIZE = 60ULL * 1024 * 1024 * 1024; // 60G gm memory
static const ConstString DEVTBL_NAME{".ascend.devtbl"};
static const ConstString GLOBAL_AUXINFO_NAME{".ascend.auxinfo.global"};
static const ConstString LOCAL_AUXINFO_NAME{".ascend.auxinfo.local."};
static const ConstString REGS_NAME{".ascend.regs."};
static const ConstString GLOBAL_NAME{".ascend.global"};

llvm::StringRef ProcessElfCoreDevice::GetPluginDescriptionStatic() {
  return "ELF core device dump plug-in.";
}

ProcessElfCoreDevice::ProcessElfCoreDevice(TargetSP target_sp,
                               lldb::ListenerSP listener_sp,
                               const FileSpec &core_file)
    : ProcessElfCore(target_sp, listener_sp, core_file) {
}

ProcessElfCoreDevice::~ProcessElfCoreDevice() {
 
}

void ProcessElfCoreDevice::Terminate() {
  PluginManager::UnregisterPlugin(CreateInstance);
}

void ProcessElfCoreDevice::Initialize() {
  static llvm::once_flag g_once_flag;
  llvm::call_once(g_once_flag, []() {
    PluginManager::RegisterPlugin(GetPluginNameStatic(),
                                  GetPluginDescriptionStatic(), CreateInstance);
  });
}

ProcessSP ProcessElfCoreDevice::CreateInstance(TargetSP target_sp,
                                               ListenerSP listener_sp,
                                               const FileSpec *crash_file,
                                               bool can_connect) {
  ProcessSP process_sp;
  if (crash_file && !can_connect) {
    const size_t header_size = sizeof(llvm::ELF::Elf64_Ehdr);

    auto data_sp = FileSystem::Instance().CreateDataBuffer(
        crash_file->GetPath(), header_size, 0);
    if (data_sp && data_sp->GetByteSize() == header_size &&
        elf::ELFHeader::MagicBytesMatch(data_sp->GetBytes())) {
      elf::ELFHeader elf_header;
      DataExtractor data(data_sp, eByteOrderLittle, 4);
      offset_t data_offset = 0;
      if (elf_header.Parse(data, &data_offset)) {
        if (elf_header.e_ident[7] == 0xFF && elf_header.e_version == 0)
          return process_sp;
        if (elf_header.e_type == llvm::ELF::ET_CORE && elf_header.e_machine == llvm::ELF::EM_ASCEND)
          process_sp = std::make_shared<ProcessElfCoreDevice>(target_sp, listener_sp,
                                                          *crash_file);
      }
    }
  }
  return process_sp;
}

Status ProcessElfCoreDevice::DoLoadCore() {
  Status error;
  if (!m_core_module_sp) {
    error.SetErrorString("invalid core module");
    return error;
  }
  auto module_spec = std::make_shared<ModuleSpec>(m_core_file);
  CheckInputValidClass flags =
            static_cast<CheckInputValidClass>(
              CheckInputValidClass::eCheckPathExists |
              CheckInputValidClass::eCheckOwnerAndWritablePermission |
              CheckInputValidClass::eCheckIsSymlink |
              CheckInputValidClass::eCheckReadablePermission |
              CheckInputValidClass::eCheckIsDir |
              CheckInputValidClass::eCheckFileSize |
              CheckInputValidClass::eCheckParentPathOwnerAndWritablePermission
            );
  error = module_spec->CheckInputFileValid(flags);
  if (error.Fail()) {
    return error;
  }
  ObjectFileELF *core = (ObjectFileELF *)(m_core_module_sp->GetObjectFile());
  if (core == nullptr) {
    error.SetErrorString("invalid core object file");
    return error;
  }

  ArchSpec target_arch = GetTarget().GetArchitecture();
  ArchSpec core_arch(m_core_module_sp->GetArchitecture());
  target_arch.MergeFrom(core_arch);
  GetTarget().SetArchitecture(target_arch);

  core->CreateSections(m_section_list);

  error = ParseSection();
  if (error.Fail()) {
    return error;
  }

  error = CheckSectionExist();
  if (error.Fail()) {
    return error;
  }
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "Enable device coredump mode, set process status to 'stop in device'.");
  SetDeviceCoredumpEnable(true);
  UpdateStopInfo();
  return error;
}

void ProcessElfCoreDevice::FocusToAnyKnownErrorAiCore() {
  Log *log = GetLog(LLDBLog::Process);
  ThreadSP thread = GetThreadList().GetSelectedThread();
  if (!thread) {
    LLDB_LOG(log, "Empty thread, stop focus to named error aicore");
    return;
  }
  RegisterContextSP reg_ctx_sp = thread->GetRegisterContext();
  auto core_reg_ctx = std::static_pointer_cast<RegisterContextPOSIXCore_ascend>(reg_ctx_sp);
  if (!core_reg_ctx) {
    LLDB_LOG(log, "Cast register context to posix core ascend register context failed.");
    return;
  }
  auto old_focus_core_id = m_summary_info.focus_core_id;
  auto old_focus_core_type = m_summary_info.focus_core_type;
  for (auto core_id: m_summary_info.aic_id) {
    m_summary_info.focus_core_id = core_id;
    m_summary_info.focus_core_type = CoreType::AIC;
    string reg_name = core_reg_ctx->GetStopErrorRegister();
    if (!reg_name.empty()) {
        thread->RefreshStackFramePC();
        return;
    }
  }
  for (auto core_id: m_summary_info.aiv_id) {
    m_summary_info.focus_core_id = core_id;
    m_summary_info.focus_core_type = CoreType::AIV;
    string reg_name = core_reg_ctx->GetStopErrorRegister();
    if (!reg_name.empty()) {
        thread->RefreshStackFramePC();
        return;
    }
  }
  m_summary_info.focus_core_id = old_focus_core_id;
  m_summary_info.focus_core_type = old_focus_core_type;
}
 
void ProcessElfCoreDevice::UpdateStopInfo(bool focus_named_error_core) {
  Log *log = GetLog(LLDBLog::Process);
  if (focus_named_error_core) {
    FocusToAnyKnownErrorAiCore();
  }
  DeviceStopInfo stop_info;
  stop_info.core_id = m_summary_info.focus_core_id;
  if (m_summary_info.focus_core_type == CoreType::AIV) {
    stop_info.core_id = stop_info.core_id - GetMaxAicID(m_summary_info.chip_type);
  }
  stop_info.core_type = m_summary_info.focus_core_type;
  ThreadSP thread = GetThreadList().GetSelectedThread();
  string desc = "Unknown Error";
  std::shared_ptr<void> defer(nullptr, [&stop_info, this, log, &desc] (std::nullptr_t&) {
    stop_info.stop_description = desc;
    SetDeviceStopInfoCached(stop_info);
    LLDB_LOG(log, "Set stop description={0}", stop_info.stop_description);
  });
  if (!thread) {
    LLDB_LOG(log, "Empty thread, stop update reason");
    return;
  }
  RegisterContextSP reg_ctx_sp = thread->GetRegisterContext();
  auto core_reg_ctx = std::static_pointer_cast<RegisterContextPOSIXCore_ascend>(reg_ctx_sp);
  if (!core_reg_ctx) {
    LLDB_LOG(log, "Cast register context to posix core ascend register context failed");
    return;
  }
  string reg_name = core_reg_ctx->GetStopErrorRegister();
  if (reg_name.empty()) {
    LLDB_LOG(log, "Got empty error description");
    return;
  }
  auto splits = llvm::StringRef(reg_name).split('_');
  desc = string(splits.first) + "_ERROR";
}

Status ProcessElfCoreDevice::ParseSection() {
  Status error;
  for (auto section : m_section_list) {
    ConstString name = section->GetName();
    if (name == DEVTBL_NAME) {
      error = ParseDevTable(section);
      if (error.Fail()) {
        return error;
      }
    }
    if (name == GLOBAL_AUXINFO_NAME) {
      error = ParseGlobalAuxInfo(section);
      if (error.Fail()) {
        return error;
      }
    }
    if (name.GetStringRef().starts_with(LOCAL_AUXINFO_NAME.GetStringRef())) {
      uint64_t core_id;
      if (!to_integer(name.GetStringRef().substr(LOCAL_AUXINFO_NAME.GetLength()), core_id, 10)) {
        error.SetErrorString("local auxinfo core id is invalid");
        return error;
      }
      error = ParseLocalAuxInfo(section, core_id);
      if (error.Fail()) {
        return error;
      }
    }
    if (name.GetStringRef().starts_with(REGS_NAME.GetStringRef())) {
      uint64_t core_id;
      if (!to_integer(name.GetStringRef().substr(REGS_NAME.GetLength()), core_id, 10)) {
        error.SetErrorString("reg info core id is invalid");
        return error;
      }
      if (m_summary_info.chip_type == DevdrvChipType::CHIP_CLOUD_V4) {
        error = ParseRegData<RegDataV4>(section, core_id);
      } else {
        error = ParseRegData<RegDataV2>(section, core_id);
      }
      if (error.Fail()) {
        return error;
      }
      m_thread_data_valid = true;
      m_thread_data.emplace_back(ThreadData{DataExtractor(), {}, 0, SIGSEGV, SIGSEGV, 0, ""});
    }
  }
  return error;
}

Status ProcessElfCoreDevice::CheckSectionExist() {
  Status error;
  if (m_summary_info.chip_type == DevdrvChipType::CHIP_BEGIN) {
    error.SetErrorString("coredump file has not devtbl");
  } else if (m_summary_info.global_mems.empty()) {
    error.SetErrorString("coredump file has not global aux info");
  } else if (m_summary_info.local_mems.empty()) {
    error.SetErrorString("coredump file has not local aux info");
  } else if (m_summary_info.reg_data.empty()) {
    error.SetErrorString("coredump file has not reg info");
  }
  return error;
}

Status ProcessElfCoreDevice::ParseDevTable(const SectionSP& section) {
  Status error;
  DataExtractor data;
  section->GetSectionData(data);
  offset_t offset = 0;
  uint64_t aic_bitmap0 = data.GetU64(&offset);
  uint64_t aic_bitmap1 = data.GetU64(&offset);
  uint64_t aiv_bitmap0 = data.GetU64(&offset);
  uint64_t aiv_bitmap1 = data.GetU64(&offset);
  m_summary_info.dev_id = data.GetU32(&offset);
  uint32_t chip_type = data.GetU32(&offset);
  if (DevdrvChipSocTypeMap.find(static_cast<DevdrvChipType>(chip_type)) == DevdrvChipSocTypeMap.end()) {
    error.SetErrorString("do not support this chip type");
    return error;
  }
  m_summary_info.chip_type = static_cast<DevdrvChipType>(chip_type);
  static constexpr size_t BIT_NUM = 64;
  size_t aic_num = GetMaxAicID(m_summary_info.chip_type);
  for (size_t i = 0; i < BIT_NUM; ++i) {
    if ((aic_bitmap0 & 0x1) == 0x1) {
      m_summary_info.aic_id.emplace_back(i);
    }
    if ((aic_bitmap1 & 0x1) == 0x1) {
      m_summary_info.aic_id.emplace_back(i + BIT_NUM);
    }
    if ((aiv_bitmap0 & 0x1) == 0x1) {
      m_summary_info.aiv_id.emplace_back(i + aic_num);
    }
    if ((aiv_bitmap1 & 0x1) == 0x1) {
      m_summary_info.aiv_id.emplace_back(i + BIT_NUM + aic_num);
    }
    aic_bitmap0 = aic_bitmap0 >> 1;
    aic_bitmap1 = aic_bitmap1 >> 1;
    aiv_bitmap0 = aiv_bitmap0 >> 1;
    aiv_bitmap1 = aiv_bitmap1 >> 1;
  }
  if (!m_summary_info.aic_id.empty()) {
    m_summary_info.focus_core_id = m_summary_info.aic_id[0];
    m_summary_info.focus_core_type = CoreType::AIC;
    UpdateStopInfo();
  } else if (!m_summary_info.aiv_id.empty()) {
    m_summary_info.focus_core_id = m_summary_info.aiv_id[0];
    m_summary_info.focus_core_type = CoreType::AIV;
  } else {
    error.SetErrorString("core id is empty");
  }
  return error;
}

inline bool IsTensorType(GlobalDataType type) {
  return (type == GlobalDataType::INVALID_TENSOR ||
          type == GlobalDataType::GENERAL_TENSOR ||
          type == GlobalDataType::INPUT_TENSOR ||
          type == GlobalDataType::OUTPUT_TENSOR);
}

static Status ParseSingleGlobalAuxInfo(GlobalMemInfo &global_mem_info,
                                       DataExtractor &data, offset_t offset) {
  Status error;
  global_mem_info.addr = data.GetU64(&offset);
  global_mem_info.size = data.GetU64(&offset);
  global_mem_info.section_index = data.GetU32(&offset);
  global_mem_info.type = static_cast<GlobalDataType>(data.GetU16(&offset));
  if (GlobalDataTypeToStr.find(global_mem_info.type) == GlobalDataTypeToStr.end()) {
    error.SetErrorString("Do not support this global data type");
    return error;
  }
  global_mem_info.reserve = data.GetU16(&offset);
  if (global_mem_info.type == GlobalDataType::STACK) {
    global_mem_info.coreInfo.coreId = data.GetU16(&offset);
  } else if (IsTensorType(global_mem_info.type)) {
    global_mem_info.shape.dim = data.GetU32(&offset);
    if (global_mem_info.shape.dim > 25) {
      error.SetErrorStringWithFormat("%s tensor dim is more than 25",
      GlobalDataTypeToStr[global_mem_info.type].c_str());
      return error;
    }
    global_mem_info.shape.reserve = data.GetU32(&offset);
    for (size_t j = 0; j < global_mem_info.shape.dim; ++j) {
      global_mem_info.shape.dim_size[j] = data.GetU64(&offset);
    }
  }
  return error;
}

Status ProcessElfCoreDevice::ParseGlobalAuxInfo(const SectionSP& section) {
  Log *log = GetLog(LLDBLog::Process);
  Status error;
  DataExtractor data;
  section->GetSectionData(data);

  uint64_t section_size = section->GetFileSize();
  if (section_size == 0 || section_size % sizeof(GlobalMemInfo) != 0 ||
      section_size > FileSystem::Instance().GetByteSize(m_core_file)) {
    error.SetErrorString("global auxinfo section's size is wrong");
    return error;
  }
  uint64_t struct_num = section_size / sizeof(GlobalMemInfo);

  for (uint64_t i = 0; i < struct_num; ++i) {
    GlobalMemInfo global_mem_info;
    bool valid = true;
    offset_t offset = i * sizeof(GlobalMemInfo);
    error = ParseSingleGlobalAuxInfo(global_mem_info, data, offset);
    if (error.Fail()) {
      return error;
    }
    if (global_mem_info.size & (1LLU << 63U)) {
      global_mem_info.size &= ~(1LLU << 63U);
      valid = false;
    }
    if (global_mem_info.type == GlobalDataType::DEVICE_KERNEL_OBJECT) {
      m_summary_info.base_pc = global_mem_info.addr;
    }
    error = GetGlobalMemData(global_mem_info, valid);
    if (error.Fail()) {
      LLDB_LOG(log, "Get global memory data failed: {0}", error);
    }
  }
  return error;
}

Status ProcessElfCoreDevice::GetGlobalMemData(const GlobalMemInfo& global_mem_info,
                                              bool valid) {
  Status error;
  GlobalMemory global_memory{global_mem_info, valid};
  auto section_index= global_mem_info.section_index;
  auto mem_section = m_section_list.GetSectionAtIndex(section_index - 1);
  if (!mem_section || mem_section->GetName() != GLOBAL_NAME) {
    error.SetErrorString("Parse global data failed");
    return error;
  }
  DataExtractor mem_data;
  if (mem_section->GetSectionData(mem_data) == 0 && global_mem_info.size > 0) {
    error.SetErrorStringWithFormat("Get %s from section failed", GlobalDataTypeToStr[global_mem_info.type].c_str());
    return error;
  }

  if (global_mem_info.size > MAX_GM_SIZE) {
    error.SetErrorStringWithFormatv("global memory info size too larget {0}bytes > {1}bytes",
                                     global_mem_info.size, MAX_GM_SIZE);
    return error;
  }
  global_memory.data.resize(global_mem_info.size);
  mem_data.ExtractBytes(0, global_mem_info.size, eByteOrderLittle, global_memory.data.data());
  m_summary_info.global_mems[global_mem_info.type].emplace_back(global_memory);
  m_summary_info.global_section_to_memory[global_mem_info.section_index] = global_memory;
  return error;
}

inline Status GetLocalMemoryDataFromSection(const SectionSP &mem_section, const LocalMemInfo &local_mem_info,
                                            LocalMemory &local_memory) {
  DataExtractor mem_data;
  Status error;
  if (mem_section->GetSectionData(mem_data) == 0) {
    error.SetErrorStringWithFormatv("{0} data size is 0", LocalDataTypeToStr[local_mem_info.type]);
    return error;
  }
  if (local_mem_info.size > MAX_LOCAL_MEM_SIZE) {
    error.SetErrorStringWithFormatv("Local memory info size too large={0}bytes > {1}bytes",
                                    local_mem_info.size, MAX_LOCAL_MEM_SIZE);
    return error;
  }
  if (local_mem_info.size > mem_data.GetByteSize()) {
    error.SetErrorStringWithFormatv("Local memory info size={0}bytes, is larger than section size={1}bytes",
                                    local_mem_info.size, mem_data.GetByteSize());
    return error;
  }
  local_memory.data.resize(local_mem_info.size);
  mem_data.ExtractBytes(0, local_mem_info.size, eByteOrderLittle, local_memory.data.data());
  return error;
}

Status ProcessElfCoreDevice::ParseLocalAuxInfo(const SectionSP& section, uint64_t core_id) {
  Status error;
  DataExtractor data;
  section->GetSectionData(data);

  uint64_t section_size = section->GetFileSize();
  if (section_size == 0 || section_size % sizeof(LocalMemInfo) != 0 ||
      section_size > FileSystem::Instance().GetByteSize(m_core_file)) {
    error.SetErrorString("local auxinfo section's size is wrong");
    return error;
  }
  uint64_t struct_num = section_size / sizeof(LocalMemInfo);

  for (uint64_t i = 0; i < struct_num; ++i) {
    LocalMemInfo local_mem_info;
    offset_t offset = i * sizeof(LocalMemInfo);
    local_mem_info.size = data.GetU64(&offset);
    local_mem_info.section_index = data.GetU32(&offset);
    local_mem_info.global_section_index = data.GetU32(&offset);
    local_mem_info.type = static_cast<lldb_private::MemType>(data.GetU32(&offset));
    local_mem_info.reserve = data.GetU32(&offset);
    if (LocalDataTypeToStr.find(local_mem_info.type) == LocalDataTypeToStr.end()) {
      error.SetErrorString("Do not support this local data type");
      return error;
    }
    LocalMemory local_memory{local_mem_info, 0, true};
    if (local_mem_info.type == lldb_private::MemType::DCACHE) {
      if (m_summary_info.global_section_to_memory.find(local_mem_info.global_section_index) 
        != m_summary_info.global_section_to_memory.end()) {
        GlobalMemory global_mem = m_summary_info.global_section_to_memory[local_mem_info.global_section_index];
        local_memory.addr = global_mem.global_mem_info.addr;
        local_memory.valid = global_mem.valid;
      } else {
        local_memory.addr = 0;
        local_memory.valid = false;
      }
    }
    auto section_index= local_mem_info.section_index;
    auto mem_section = m_section_list.GetSectionAtIndex(section_index - 1);
    error = GetLocalMemoryDataFromSection(mem_section, local_mem_info, local_memory);
    if (error.Fail()) {
      return error;
    }
    m_summary_info.local_mems[core_id][local_mem_info.type].emplace_back(local_memory);
  }
  return Status();
}

template<typename T>
Status ProcessElfCoreDevice::ParseRegData(const SectionSP& section, uint64_t core_id) {
  Status error;
  DataExtractor data;
  section->GetSectionData(data);

  uint64_t section_size = section->GetFileSize();
  constexpr size_t reg_data_size = sizeof(T) - 8; // The size of the virtual function table is 8 bytes.
  if (section_size == 0 || section_size % reg_data_size != 0 ||
      section_size > FileSystem::Instance().GetByteSize(m_core_file)) {
    error.SetErrorString("reg info section's size is wrong");
    return error;
  }
  uint64_t struct_num = section_size / reg_data_size;
  for (size_t i = 0; i < struct_num; ++i) {
    auto reg_data = make_unique<T>();
    data.CopyData(i * reg_data_size, reg_data_size, reinterpret_cast<uint8_t*>(reg_data.get()) + 8);
    m_summary_info.reg_data[core_id][reg_data->addr] = move(reg_data);
  }

  return error;
}

// for A2/3
static uint64_t TranslateStackVaToDmaAddr(CoreIDType core_id, CoreType core_type, uint64_t stack_va) {
  Log *log = GetLog(LLDBLog::Process);
  // 1. read SYS_VA_BASE, STACK_PHY_BASE
  uint64_t sys_va_base = 0; // when start securely, base is 0
  uint64_t stack_phy_base = 0xff000000000; // read from register will be more accurate
  // 2. transform
  uint64_t new_stack_va = stack_va;
  uint64_t dma_addr = stack_va;
  constexpr uint64_t va_begin_offset = 0x100000;
  constexpr uint64_t va_end_offset = 0x1000000;
  constexpr uint64_t valid_stack_addr_mask = 0xfffffffffe000000;
  if (stack_va > sys_va_base + va_begin_offset && stack_va < sys_va_base + va_end_offset) {
    if ((stack_phy_base & valid_stack_addr_mask) == (sys_va_base & valid_stack_addr_mask)) {
      // stack is mapped in UB case. basically we want match if the upper 39bits are the same
      LLDB_LOG(log, "stack should be accessed from UB");
    } else {
      LLDB_LOG(log, "stack is mapped at GM");
      dma_addr = new_stack_va - (sys_va_base + va_begin_offset) + stack_phy_base;
    }
  }
  LLDB_LOG(log, "stack_va={0:x}, dma_addr={1:x}", new_stack_va, dma_addr);
  return dma_addr;
}

const SummaryInfo& ProcessElfCoreDevice::GetSummaryInfo() {
  return m_summary_info;
}

size_t ProcessElfCoreDevice::ReadMemory(lldb::addr_t addr, void *buf, size_t size, Status &error) {
  MemoryReaderParamClient param{};
  param.arch_spec = ArchSpec("hiipu64");
  param.address_class = DeviceAddressClass::STACK;
  return DoReadMemory(addr, buf, size, param, error);
}

size_t ProcessElfCoreDevice::DoReadMemory(addr_t addr, void *buf, size_t size,
  const MemoryReaderParamClient &param, Status &error) {
  Log *log = GetLog(LLDBLog::Process);
  LLDB_LOG(log, "Read memory: addr={0:x}, size={1}, core_id={2}, core_type={3}, address_class={4}",
           addr, size, m_summary_info.focus_core_id, static_cast<uint8_t>(m_summary_info.focus_core_type),
           static_cast<uint32_t>(param.address_class));
  MemoryReaderParamClient newParam = param;
  if (param.address_class == DeviceAddressClass::STACK) {
    addr = TranslateStackVaToDmaAddr(m_summary_info.focus_core_id, m_summary_info.focus_core_type, addr);
    newParam.address_class = DeviceAddressClass::DCACHE;
    LLDB_LOG(log, "After translate stack addr, Read stack memory from dcache.");
  }
  size_t readSize = 0;
  if (AddressClassGlobalDataTypeMap.find(newParam.address_class) != AddressClassGlobalDataTypeMap.end()) {
    readSize = ReadGlobalMemory(newParam.address_class, addr, size, buf, error);
  } else if (AddressClassLocalDataTypeMap.find(newParam.address_class) != AddressClassLocalDataTypeMap.end()) {
    readSize = ReadLocalMemory(AddressClassLocalDataTypeMap[newParam.address_class], addr, size, buf, error);
  } else {
    error.SetErrorStringWithFormat(
        "Memory type is unknown, read memory from coredump file failed, please check command: ascend info summary");
  }
  LLDB_LOG(log, "Read memory: addr={0:x}, size={1}, core_id={2}, core_type={3}, address_class={4}, readSize={5}",
           addr, size, m_summary_info.focus_core_id, static_cast<uint8_t>(m_summary_info.focus_core_type),
           static_cast<uint8_t>(newParam.address_class), readSize);
  return readSize;
}

size_t ProcessElfCoreDevice::ReadGlobalMemory(DeviceAddressClass address_class,
                                              addr_t addr, size_t size, void *buf, Status &error) {
  vector<GlobalMemory> global_mems;
  for (GlobalDataType type : AddressClassGlobalDataTypeMap[address_class]) {
    if (m_summary_info.global_mems.find(type) != m_summary_info.global_mems.end()) {
      global_mems.insert(global_mems.end(), m_summary_info.global_mems[type].begin(),
      m_summary_info.global_mems[type].end());
    }
  }
  for (const auto &global_mem : global_mems) {
    if (addr >= global_mem.global_mem_info.addr && 
      global_mem.global_mem_info.addr <= std::numeric_limits<uint64_t>::max() - global_mem.global_mem_info.size &&
      addr < global_mem.global_mem_info.addr + global_mem.global_mem_info.size) {
      size = min(size, global_mem.global_mem_info.addr + global_mem.global_mem_info.size - addr);
      if (!global_mem.valid) {
        error.SetErrorString("this memory data is invalid, please check command: ascend info summary");
        return 0;
      }
      for (size_t i = 0; i < size; ++i) {
        *((uint8_t *)buf + i) = global_mem.data[i + (addr - global_mem.global_mem_info.addr)];
      }
      return size;
     }
  }
  error.SetErrorString("addr is not in range, please check command: ascend info summary");
  return 0;
}

size_t ProcessElfCoreDevice::ReadLocalMemory(lldb_private::MemType local_data_type,
                          addr_t addr, size_t size, void *buf, Status &error) {
  if (m_summary_info.local_mems.find(m_summary_info.focus_core_id) == m_summary_info.local_mems.end()) {
    error.SetErrorStringWithFormatv(
        "Focus core id local memory data is not in coredump file, core id is {0}", m_summary_info.focus_core_id);
    return 0;
  }
  auto mem_info = m_summary_info.local_mems[m_summary_info.focus_core_id];
  if (mem_info.find(local_data_type) == mem_info.end()) {
    error.SetErrorStringWithFormat(
        "Memory type is not in this core id data, local data type is %s", LocalDataTypeToStr[local_data_type].c_str());
    return 0;
  }
  auto local_mems = mem_info[local_data_type];
  for (const auto &local_mem : local_mems) {
    if (!local_mem.valid) {
      error.SetErrorString("this memory data is invalid, please check command: ascend info summary");
      return 0;
    }
    if (addr >= local_mem.addr && 
          local_mem.addr <= std::numeric_limits<uint64_t>::max() - local_mem.local_mem_info.size &&
          addr >= local_mem.addr &&addr < local_mem.addr + local_mem.local_mem_info.size) {
      size = min(size, local_mem.addr + local_mem.local_mem_info.size - addr);
      const uint8_t *src = local_mem.data.data() + (addr - local_mem.addr);
      uint8_t *dst = (uint8_t *)buf;
      std::copy(src, src + size, dst);
      return size;
    }
  }
  error.SetErrorString("addr is not in range, please check command: ascend info summary");
  return 0;
}

Status ProcessElfCoreDevice::SetAicOnFocus(const uint32_t &core_id) {
  Status error;
  // aic core_id is always correct.
  if (find(m_summary_info.aic_id.begin(), m_summary_info.aic_id.end(),
           core_id) != m_summary_info.aic_id.end()) {
    m_summary_info.focus_core_id = core_id;
    m_summary_info.focus_core_type = CoreType::AIC;
  } else {
    error.SetErrorString("Switch AIC failed, please check command: ascend info summary");
  }
  return error;
}

Status ProcessElfCoreDevice::SetAivOnFocus(const uint32_t &core_id) {
  Status error;
  // aiv core_id need to add offset of MAX_AIC_NUM.
  CoreIDType aiv_core_id = core_id + GetMaxAicID(m_summary_info.chip_type);
  if (find(m_summary_info.aiv_id.begin(), m_summary_info.aiv_id.end(),
           aiv_core_id) != m_summary_info.aiv_id.end()) {
    m_summary_info.focus_core_id = aiv_core_id;
    m_summary_info.focus_core_type = CoreType::AIV;
    UpdateStopInfo();
  } else {
   error.SetErrorString("Switch AIV failed, please check command: ascend info summary");
  }
  return error;
}

/*
* This function used in DynamicLoaderPOSIXDYLD::DidAttach,
* device process should load at offset = 0
*/
DataExtractor ProcessElfCoreDevice::GetAuxvData() {
  // key: value format
  static vector<uint64_t> stub_auxv_data = {
      AuxVector::AUXV_AT_ENTRY, 0,
      AuxVector::AUXV_AT_NULL, AuxVector::AUXV_AT_NULL,
  };
  stub_auxv_data[1] = m_summary_info.base_pc;
  const uint8_t *start = reinterpret_cast<uint8_t*>(stub_auxv_data.data());
  size_t len = stub_auxv_data.size() * sizeof(uint64_t);
  lldb::DataBufferSP buffer(new lldb_private::DataBufferHeap(start, len));
  return DataExtractor(buffer, GetByteOrder(), GetAddressByteSize());
}

bool ProcessElfCoreDevice::CanDebug(lldb::TargetSP target_sp,
                                    bool plugin_specified_by_name) {
  if (!m_core_module_sp && FileSystem::Instance().Exists(m_core_file)) {
    ModuleSpec core_module_spec(m_core_file, ArchSpec("hiipu64"));
    Status error(ModuleList::GetSharedModule(core_module_spec, m_core_module_sp,
                                             nullptr, nullptr, nullptr));
    if (m_core_module_sp) {
      ObjectFile *core_objfile = m_core_module_sp->GetObjectFile();
      if (core_objfile && core_objfile->GetType() == ObjectFile::eTypeCoreFile)
        return true;
    }
  }
  return false;
}

Status ProcessElfCoreDevice::GetCoresInfo(std::vector<CoreInfo> &info) {
  Status error;
  ThreadSP thread_sp = GetThreadList().GetSelectedThread();
  if (!thread_sp) {
    error.SetErrorStringWithFormatv("Got empty thread.");
  }
  RegisterContextSP reg_ctx_sp = thread_sp->GetRegisterContext();
  if (!reg_ctx_sp) {
    error.SetErrorStringWithFormatv("Got empty register context.");
  }
  auto old_focus_core_id = m_summary_info.focus_core_id;
  auto old_focus_core_type = m_summary_info.focus_core_type;
  for (auto core_id: m_summary_info.aic_id) {
    m_summary_info.focus_core_id = core_id;
    m_summary_info.focus_core_type = CoreType::AIC;
    CoreInfo core_info;
    core_info.core_id = core_id;
    core_info.core_type = CoreType::AIC;
    core_info.pc = reg_ctx_sp->GetPC(UINT64_MAX);
    info.push_back(core_info);
  }
  for (auto core_id: m_summary_info.aiv_id) {
    m_summary_info.focus_core_id = core_id;
    m_summary_info.focus_core_type = CoreType::AIV;
    CoreInfo core_info;
    core_info.core_id = core_id - GetMaxAicID(m_summary_info.chip_type);
    core_info.core_type = CoreType::AIV;
    core_info.pc = reg_ctx_sp->GetPC(UINT64_MAX);
    info.push_back(core_info);
  }
  m_summary_info.focus_core_id = old_focus_core_id;
  m_summary_info.focus_core_type = old_focus_core_type;
  return error;
}

#endif
