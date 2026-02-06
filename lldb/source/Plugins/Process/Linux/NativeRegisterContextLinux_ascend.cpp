/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#ifdef MS_DEBUGGER
#include "NativeRegisterContextLinux_ascend.h"

#include "Plugins/Process/Linux/AscendThreadLinux.h"
#include "lldb/Host/HostInfo.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Status.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend910B.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend310P.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend950.h"

using namespace lldb_private;
using namespace lldb_private::process_linux;

std::unique_ptr<NativeRegisterContextLinux>
NativeRegisterContextLinux::CreateDeviceNativeRegisterContextLinux(
    const ArchSpec &target_arch, const SocType &soc_type,
    NativeThreadLinux &native_thread) {
  std::unique_ptr<RegisterInfoPOSIX_ascend> register_info_up;
  if (soc_type == SocType::ASCEND910B) {
    register_info_up = std::make_unique<RegisterInfoPOSIX_ascend910B>(target_arch);
  } else if (soc_type == SocType::ASCEND310P) {
    register_info_up = std::make_unique<RegisterInfoPOSIX_ascend310P>(target_arch);
  } else if (soc_type == SocType::ASCEND950) {
    register_info_up = std::make_unique<RegisterInfoPOSIX_ascend950>(target_arch);
  } else {
    LLDB_LOG(GetLog(LLDBLog::Process), "invalid soc type={0}", static_cast<uint32_t>(soc_type));
  }
  return std::unique_ptr<NativeRegisterContextLinux>(
      new NativeRegisterContextLinux_ascend(target_arch, native_thread,
          std::move(register_info_up)));
}

NativeRegisterContextLinux_ascend::NativeRegisterContextLinux_ascend(
    const ArchSpec &target_arch, NativeThreadProtocol &native_thread,
    std::unique_ptr<RegisterInfoPOSIX_ascend> register_info_up)
    : NativeRegisterContextRegisterInfo(
          native_thread, register_info_up.release()),
      NativeRegisterContextLinux(native_thread) {}

uint32_t NativeRegisterContextLinux_ascend::GetRegisterSetCount() const {
  return 0;
}

const RegisterSet* NativeRegisterContextLinux_ascend::GetRegisterSet(uint32_t set_index) const {
  return nullptr;
}

Status NativeRegisterContextLinux_ascend::ReadRegister(
    const RegisterInfo *reg_info, RegisterValue &reg_value) {
  return static_cast<AscendProcessLinux*>(&m_thread.GetProcess())->ReadDeviceRegisterValue(reg_info, reg_value);
}

Status NativeRegisterContextLinux_ascend::WriteRegister(const RegisterInfo *reg_info,
    const RegisterValue &reg_value) {
  return Status("The device context does not support write device register.");
}

Status NativeRegisterContextLinux_ascend::ReadAllRegisterValues(
    lldb::WritableDataBufferSP &data_sp) {
  return Status("The device does not support read device register.");
}

Status NativeRegisterContextLinux_ascend::WriteAllRegisterValues(
    const lldb::DataBufferSP &data_sp) {
  return Status("The device context does not support write device register.");
}

void* NativeRegisterContextLinux_ascend::GetGPRBuffer() { return nullptr; }

void* NativeRegisterContextLinux_ascend::GetFPRBuffer() { return nullptr; }

size_t NativeRegisterContextLinux_ascend::GetFPRSize() { return 0; }
#endif // ifdef MS_DEBUGGER
