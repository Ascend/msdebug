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

  ~ProcessElfCoreDevice() override;

  Status DoLoadCore() override;

  size_t DoReadMemory(lldb::addr_t addr, void *buf, size_t size,
                      const MemoryReaderParamClient &param, Status &error) override;
  bool CanDebug(lldb::TargetSP target_sp,
                bool plugin_specified_by_name) override;

  const device_core::SummaryInfo& GetSummaryInfo() override;

  Status SetAicOnFocus(const uint32_t &core_id) override;

  Status SetAivOnFocus(const uint32_t &core_id) override;

  DataExtractor GetAuxvData() override;

  Status GetCoresInfo(std::vector<CoreInfo> &info) override;
 
  // Update which pipe_err we got, depends on register values.
  void UpdateStopInfo(bool focus_known_error_core = false) override;

private:
  Status ParseDevTable(const lldb::SectionSP& section);
  Status ParseGlobalAuxInfo(const lldb::SectionSP& section);
  Status ParseLocalAuxInfo(const lldb::SectionSP& section, uint64_t core_id);
  template<typename T>Status ParseRegData(const lldb::SectionSP& section, uint64_t core_id);
  size_t ReadGlobalMemory(DeviceAddressClass address_class, lldb::addr_t addr, size_t size, void *buf, Status &error);
  size_t ReadLocalMemory(MemType local_data_type, lldb::addr_t addr, size_t size, void *buf, Status &error);
  Status CheckSectionExist();
  Status ParseSection();
  Status GetGlobalMemData(const device_core::GlobalMemInfo& global_mem_info, bool valid);
  void FocusToAnyKnownErrorAiCore();

private:
  device_core::SummaryInfo m_summary_info;
  SectionList m_section_list;
};
}

#endif //LLDB_SOURCE_PLUGINS_PROCESS_ELF_CORE_DEVICE_PROCESSELFCOREDEVICE_H
#endif
