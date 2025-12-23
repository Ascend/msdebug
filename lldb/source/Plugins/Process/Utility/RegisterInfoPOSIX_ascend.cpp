/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#ifdef MS_DEBUGGER
#include "RegisterInfoPOSIX_ascend.h"

using namespace lldb_private;
using namespace lldb;

RegisterInfoPOSIX_ascend::RegisterInfoPOSIX_ascend(const ArchSpec &target_arch,
  uint32_t register_info_count, uint32_t register_gpr_count,
  const RegisterInfo *register_info_p,
  const std::map<std::string, DeviceRegisterInfo>& register_map)
    : RegisterInfoInterface(target_arch), m_register_info_count(register_info_count),
      m_register_gpr_count(register_gpr_count), m_register_info_p(register_info_p),
      m_register_map(register_map) {
}

size_t RegisterInfoPOSIX_ascend::GetGPRSize() const { return m_register_gpr_count; }

uint32_t RegisterInfoPOSIX_ascend::GetRegisterCount() const {
  return m_register_info_count;
}

const lldb_private::RegisterInfo* RegisterInfoPOSIX_ascend::GetRegisterInfo() const {
  return m_register_info_p;
}

const RegisterSet* RegisterInfoPOSIX_ascend::GetRegisterSet(size_t reg_set) const {
  return nullptr;
}

size_t RegisterInfoPOSIX_ascend::GetRegisterSetCount() const {
  return 1;
}

std::map<std::string, DeviceRegisterInfo> RegisterInfoPOSIX_ascend::GetRegisterMap() {
  return m_register_map;
}

Status RegisterInfoPOSIX_ascend::GetRegisterAddr(const llvm::StringRef reg_name, CoreType core_type, uint64_t &addr) {
  return Status();
}
#endif // ifdef MS_DEBUGGER
