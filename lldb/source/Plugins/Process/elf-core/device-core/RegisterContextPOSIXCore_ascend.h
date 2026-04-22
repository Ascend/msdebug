/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#ifdef MS_DEBUGGER
#ifndef LLDB_SOURCE_PLUGINS_PROCESS_ELF_CORE_REGISTERCONTEXTPOSIXCORE_ASCEND_H
#define LLDB_SOURCE_PLUGINS_PROCESS_ELF_CORE_REGISTERCONTEXTPOSIXCORE_ASCEND_H

#include "Plugins/Process/Utility/RegisterContextPOSIX_arm64.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_arm.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend.h"
#include "Plugins/Process/elf-core/RegisterUtilities.h"
#include "Plugins/Process/elf-core/device-core/ElfCoreDeviceUtilities.h"

using namespace std;
using namespace lldb_private;

class RegisterContextPOSIXCore_ascend : public RegisterContext, RegisterDataInterface {
public:
  static unique_ptr<RegisterContextPOSIXCore_ascend>
  Create(Thread &thread, const ArchSpec &arch, SocType soc_type);
  ~RegisterContextPOSIXCore_ascend() override;

  RegisterContextPOSIXCore_ascend(Thread &thread,
    unique_ptr<RegisterInfoPOSIX_ascend> register_info);

  bool ReadRegister(const RegisterInfo *reg_info,
                  RegisterValue &value) override;

  Status ReadRegister(uint64_t addr, const RegisterInfo* reg_info, 
        uint32_t core_id, CoreType core_type, RegisterValue &value) const override;

  bool WriteRegister(const RegisterInfo *reg_info,
                     const RegisterValue &value) override;

  void InvalidateAllRegisters() override;

  size_t GetRegisterCount() override;

  const RegisterInfo *GetRegisterInfoAtIndex(size_t reg) override;

  size_t GetRegisterSetCount() override;

  const RegisterSet *GetRegisterSet(size_t set) override;

  const RegisterInfo *GetRegisterInfo();

  string GetStopErrorRegister();

private:
  void FixPC(uint64_t &pc);
 
  void FixPCByErrorInfoReg(const ErrRegMask &err_reg_mask, uint64_t &pc);
 
  bool CheckAicErrorRegisterIsValid(size_t num_registers,
                                    const vector<vector<ErrRegMask>> *error_table,
                                    const RegisterSet *const reg_set,
                                    ErrRegMask &err_reg_mask);

private:
  std::unique_ptr<RegisterInfoPOSIX_ascend> m_register_info;
  const device_core::SummaryInfo *m_summary_info{};
};

#endif //LLDB_SOURCE_PLUGINS_PROCESS_ELF_CORE_REGISTERCONTEXTPOSIXCORE_ASCEND_H
#endif
