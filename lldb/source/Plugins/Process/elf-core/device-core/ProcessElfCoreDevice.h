/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifdef MS_DEBUGGER
#ifndef LLDB_SOURCE_PLUGINS_PROCESS_ELF_CORE_DEVICE_PROCESSELFCOREDEVICE_H
#define LLDB_SOURCE_PLUGINS_PROCESS_ELF_CORE_DEVICE_PROCESSELFCOREDEVICE_H
#include "Plugins/Process/elf-core/ProcessElfCore.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend.h"
#include "lldb/Core/Section.h"

namespace lldb_private {


class ProcessElfCoreDevice : public ProcessElfCore {
public:

  static lldb::ProcessSP
  CreateInstance(lldb::TargetSP target_sp, lldb::ListenerSP listener_sp,
                 const FileSpec *crash_file_path,
                 bool can_connect);

  static void Initialize();

  static void Terminate();

  static llvm::StringRef GetPluginNameStatic() { return "elf-core-device"; }

  static llvm::StringRef GetPluginDescriptionStatic();

  ProcessElfCoreDevice(lldb::TargetSP target_sp, lldb::ListenerSP listener_sp,
                   const FileSpec &core_file);

  ~ProcessElfCoreDevice() override = default;

  Status DoLoadCore() override;

  size_t ReadMemory(lldb::addr_t addr, void *buf, size_t size, Status &error) override;

  size_t DoReadMemory(lldb::addr_t addr, void *buf, size_t size,
                      const MemoryReaderParamClient &param, Status &error) override;
  bool CanDebug(lldb::TargetSP target_sp,
                bool plugin_specified_by_name) override;

  const device_core::SummaryInfo& GetSummaryInfo() override;

  Status SetAicOnFocus(const uint32_t &core_id) override;

  Status SetAivOnFocus(const uint32_t &core_id) override;

  DataExtractor GetAuxvData() override;

  Status GetCoresInfo(std::vector<CoreInfo> &info) override;

  Status GetWarpsInfo(std::vector<WarpInfo> &info) override;

  // Update which pipe_err we got, depends on register values.
  void UpdateStopInfo(bool focus_known_error_core = false) override;

  Status GetKernelInfo(KernelInfo &info) override;
private:
  Status ParseDevTable(const lldb::SectionSP& section, ConstString section_name);

  // try to change target module by m_kernel_name
  Status UpdateTargetKernel();

  Status ParseGlobalAuxInfo(const lldb::SectionSP& section, ConstString section_name);

  Status ParseLocalAuxInfo(const lldb::SectionSP& section, ConstString section_name);

  Status ParseOneLocalAuxInfo(const lldb::SectionSP& section, uint64_t core_id);

  Status ParseRegs(const lldb::SectionSP& section, ConstString section_name);

  Status ParseKernelInfo(const lldb::SectionSP& section, ConstString section_name);

  Status ParseHostKernelObject(const lldb::SectionSP& section, ConstString section_name);

  template<typename T>
  Status ParseRegData(const lldb::SectionSP& section, uint64_t core_id);

  size_t ReadGlobalMemory(DeviceAddressClass address_class, lldb::addr_t addr, size_t size, void *buf, Status &error);

  size_t ReadLocalMemory(MemType local_data_type, lldb::addr_t addr, size_t size, void *buf, Status &error);
  Status CheckSectionExist();

  Status ParseSection();

  Status GetGlobalMemData(const device_core::GlobalMemInfo& global_mem_info, bool valid);

  void FocusToAnyKnownErrorAiCore();

  Status GetThreadDim(lldb::RegisterContextSP reg_ctx_sp, device_core::ThreadDim &thread_dim);

  void FocusToActiveThreadInWarp(uint8_t warp_id);

  InterruptPosType GetPosType(lldb::RegisterContextSP reg_ctx_sp);

  Status SetThreadOnFocus(const uint32_t &linear_idx) override;

private:
  device_core::SummaryInfo m_summary_info;
  SectionList m_section_list;
  std::string m_kernel_name{""};
  std::shared_ptr<ModuleSpec> m_device_module_spec{};
};
}

#endif //LLDB_SOURCE_PLUGINS_PROCESS_ELF_CORE_DEVICE_PROCESSELFCOREDEVICE_H
#endif
