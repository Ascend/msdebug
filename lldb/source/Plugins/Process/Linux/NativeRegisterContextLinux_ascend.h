/*
 * -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
 */

#ifdef MS_DEBUGGER
#ifndef lldb_NativeRegisterContextLinux_ascend_h
#define lldb_NativeRegisterContextLinux_ascend_h

#include "Plugins/Process/Linux/NativeRegisterContextLinux.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ascend.h"

namespace lldb_private {
namespace process_linux {

class NativeProcessLinux;

class NativeRegisterContextLinux_ascend
    : public NativeRegisterContextLinux {
public:
  NativeRegisterContextLinux_ascend(
      const ArchSpec &target_arch, NativeThreadProtocol &native_thread,
      std::unique_ptr<RegisterInfoPOSIX_ascend> register_info_up);

  uint32_t GetRegisterSetCount() const override;

  const RegisterSet *GetRegisterSet(uint32_t set_index) const override;

  Status ReadRegister(const RegisterInfo *reg_info,
                      RegisterValue &reg_value) override;

  Status WriteRegister(const RegisterInfo *reg_info,
                       const RegisterValue &reg_value) override;

  Status ReadAllRegisterValues(lldb::WritableDataBufferSP &data_sp) override;

  Status WriteAllRegisterValues(const lldb::DataBufferSP &data_sp) override;

protected:
  void *GetGPRBuffer() override;

  void *GetFPRBuffer() override;

  size_t GetFPRSize() override;

};

} // namespace process_linux
} // namespace lldb_private

#endif // #ifndef lldb_NativeRegisterContextLinux_ascend.h
#endif // ifdef MS_DEBUGGER
